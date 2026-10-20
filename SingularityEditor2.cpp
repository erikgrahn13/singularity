#include "SingularityEditor2.h"

#include "QGuiApplication"
#include <QQmlContext>
#include <QQuickItem>

#include <iostream>

SingularityEditor::SingularityEditor(const char* platform, std::string hotReloadArgument)
{
    if (platform)
        qputenv("QT_QPA_PLATFORM", QByteArray(platform));

    static std::string argument = std::move(hotReloadArgument);
    static int argc = argument.empty() ? 1 : 2;
    static char name[] = "singularity";

    static char* argv[] = { name, argument.empty() ? nullptr : argument.data(), nullptr};
    static QGuiApplication application(argc, argv);

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
    if(auto it = parameters_.find(id); it != parameters_.end())
        return it->second;

    auto* parameter = new QmlParameter(
        id, 
        [this](unsigned int id){ return getParameter(id); }, 
        [this](unsigned int id, double value){ setParameter(id, value); }, 
        this);
    parameters_.emplace(id, parameter);
    return parameter;
}

QmlParameter::QmlParameter(unsigned int id, Getter getter, Setter setter, QObject* parent)
    : QObject(parent), id_(id), getter_(std::move(getter)), setter_(std::move(setter))
{

}

double QmlParameter::value() const
{
    return getter_(id_);
}

void QmlParameter::setValue(double value)
{
    const double previous = this->value();

    setter_(id_, value);

    if(this->value() != previous)
        emit valueChanged();
}
