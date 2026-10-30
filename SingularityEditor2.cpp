
#include "QGuiApplication"
#include <QQmlContext>
#include <QQuickItem>
#include <QTimer>
#include <QFile>


#include PLUGIN_CLASS_HEADER
#include "SingularityEditor2.h"
#include <type_traits>
#include <utility>
#include <iostream>

std::vector<std::byte> getResource(std::string_view name)
{
    const auto path = QStringLiteral(":/data_resources/")
        + QString::fromUtf8(name.data(), name.size());

    QFile file(path);

    if (!file.open(QIODevice::ReadOnly))
        return {};

    const auto data = file.readAll();

    if (file.error() != QFileDevice::NoError)
        return {};

    std::vector<std::byte> bytes(
        static_cast<std::size_t>(data.size()));

    if (!bytes.empty())
        std::memcpy(bytes.data(), data.constData(), bytes.size());

    return bytes;
}

SingularityEditor::SingularityEditor(const char* platform, std::string hotReloadArgument)
{
    if (platform)
        qputenv("QT_QPA_PLATFORM", QByteArray(platform));

    static std::string argument = std::move(hotReloadArgument);
    static int argc = argument.empty() ? 1 : 2;
    static char name[] = "singularity";

    static char* argv[] = { name, argument.empty() ? nullptr : argument.data(), nullptr};

    if (!QCoreApplication::instance())
    {
        QCoreApplication::setAttribute(Qt::AA_PluginApplication, true);
        // The plugin-created application belongs to the host process. Avoid a
        // static QObject destructor running after Qt's own shutdown routines.
        static auto* const application = new QGuiApplication(argc, argv);
        application->setQuitOnLastWindowClosed(false);
    }

    view_ = std::make_unique<QQuickView>();
    view_->rootContext()->setContextProperty("plugin", this);

    QObject::connect(view_->contentItem(), &QQuickItem::childrenChanged, view_.get(),
    [this]
    {
        auto* root = view_->rootObject();

        if(!root)
            return;

        const auto version = root->property("singularityPluginViewVersion");
        if(!version.isValid() || version.toInt() != 1)
        {
            std::cerr << "[Singularity] ERROR: Main.qml must use PluginView as its root component" << std::endl;
            view_->setSource({});
            return;
        }

        const auto size = view_->initialSize();
        requestResize(size.width(), size.height());

    }, Qt::QueuedConnection);

}

SingularityEditor::~SingularityEditor()
{

}

void SingularityEditor::updateAudio()
{
    UiAudioBlock packet;

    while(popUiAudioBlock(packet))
    {
        const auto count = packet.numSamples * packet.numChannels;

        // if(count > packet.samples.size
        audioSamples_.clear();
        audioSamples_.reserve(count);

        for(int i = 0; i < count; ++i)
        {
            audioSamples_.append(packet.samples[i]);
        }
        emit audioChanged();
    }
}

void SingularityEditor::initializeView()
{
#if defined(SINGULARITY_QML_SOURCE_FILE)
    view_->setSource(
        QUrl::fromLocalFile(QStringLiteral(SINGULARITY_QML_SOURCE_FILE)));
#else
    view_->loadFromModule(SINGULARITY_QML_MODULE_URI, "Main");
#endif
    view_->setResizeMode(QQuickView::SizeRootObjectToView);
    view_->setTitle(QStringLiteral(PLUGIN_NAME));

    auto* timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &SingularityEditor::updateAudio);
    timer->start(16);
}

void SingularityEditor::showWindow()
{
    view_->show();
}

QVariantMap SingularityEditor::data() const
{
    return data_;
}

void SingularityEditor::setData(std::string_view name, const ActionValue& value)
{
    const auto key = QString::fromUtf8(name.data(), name.size());

    const QVariant dataValue = std::visit(
        [](const auto& payload) -> QVariant
        {
            using T = std::decay_t<decltype(payload)>;

            if constexpr (std::is_same_v<T, std::monostate>)
                return QVariant{};
            else if constexpr (std::is_same_v<T, bool>)
                return payload;
            else if constexpr (std::is_same_v<T, std::int64_t>)
                return QVariant::fromValue(static_cast<qlonglong>(payload));
            else if constexpr (std::is_same_v<T, double>)
                return payload;
            else if constexpr (std::is_same_v<T, std::string>)
                return QString::fromUtf8(payload.data(), payload.size());
        }, 
        value);

    if (data_.value(key) == dataValue)
        return;

    data_.insert(key, dataValue);
    emit dataChanged();
}

void SingularityEditor::sendAction(QString name, QVariant value)
{
    std::optional<ActionValue> actionValue;

    if(value.isNull())
        actionValue = std::monostate{};

    const auto type = value.metaType();

    if(type == QMetaType::fromType<bool>())
        actionValue = value.toBool();

    if(type == QMetaType::fromType<int>() || type == QMetaType::fromType<unsigned int>() || type == QMetaType::fromType<qlonglong>())
        actionValue = value.toLongLong();

    if(type == QMetaType::fromType<double>() || type == QMetaType::fromType<float>())
        actionValue = value.toDouble();
    
    if(type == QMetaType::fromType<QString>())
    {
        const auto utf8 = value.toString().toUtf8();
        actionValue = std::string{utf8.constData(), static_cast<std::size_t>(utf8.size())};
    }

    if(type == QMetaType::fromType<QUrl>())
    {
        const auto url = value.toUrl();
        QString text = url.isLocalFile() ? url.toLocalFile() : url.toString(QUrl::FullyEncoded);
        const auto utf8 = text.toUtf8();
        actionValue = std::string{utf8.constData(), static_cast<std::size_t>(utf8.size())};
    }

    const auto utf8Name = name.toUtf8();

    dispatchAction(std::string_view{utf8Name.constData(), static_cast<std::size_t>(utf8Name.size())}, *actionValue);
}

QmlParameter* SingularityEditor::parameter(int id)
{
    if (auto it = parameters_.find(id); it != parameters_.end())
        return it->second;

    for (const auto& parameter : PLUGIN_CLASS::getParameters())
    {
        if (parameter.id != id)
            continue;

        const auto [minValue, maxValue] = std::visit(
            [](const auto& definition) -> std::pair<double, double>
            {
                using Type = std::remove_cvref_t<decltype(definition)>;

                if constexpr (
                    std::is_same_v<Type, ContinuousParameter> ||
                    std::is_same_v<Type, SteppedParameter>)
                {
                    return {definition.minValue, definition.maxValue};
                }
                else if constexpr (std::is_same_v<Type, BooleanParameter>)
                {
                    return {0.0, 1.0};
                }
                else if constexpr (std::is_same_v<Type, ChoiceParameter>)
                {
                    return {
                        0.0,
                        definition.choices.empty()
                            ? 0.0
                            : static_cast<double>(definition.choices.size() - 1)
                    };
                }
            },
            parameter.definition);

        auto* result = new QmlParameter(
            id, minValue, maxValue, *this, this);

        parameters_.emplace(id, result);
        return result;
    }

    return nullptr;
}

void SingularityEditor::notifyParameterChanged(unsigned int id)
{
    if(auto it = parameters_.find(id); it != parameters_.end())
        emit it->second->valueChanged();
}

std::optional<unsigned int> SingularityEditor::hitTestHelper(int xPos, int yPos)
{
    auto* item = getView().contentItem();
    QPointF point(xPos, yPos);

    while (auto* child = item->childAt(point.x(), point.y()))
    {
        point = child->mapFromItem(item, point);
        item = child;
        auto* object = item->property("parameter").value<QObject*>();
        auto* parameter = qobject_cast<QmlParameter*>(object);

        if (parameter)
        {
            return parameter->id();
        }
    }
    return std::nullopt;
}

QmlParameter::QmlParameter(unsigned int id, double minValue, double maxValue, SingularityEditor& editor, QObject* parent)
    : QObject(parent), id_(id), minValue_(minValue), maxValue_(maxValue), editor_(editor)
{

}

double QmlParameter::minValue() const
{
    return minValue_;
}

double QmlParameter::maxValue() const
{
    return maxValue_;;
}

double QmlParameter::value() const
{
    return editor_.getParameter(id_);
}

void QmlParameter::setValue(double value)
{
    const double previous = this->value();

    editor_.setParameter(id_, value);

    if(this->value() != previous)
        emit valueChanged();
}

QVariantList SingularityEditor::audioSamples() const
{
    return audioSamples_;
}
