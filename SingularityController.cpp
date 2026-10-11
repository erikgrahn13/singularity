#include "SingularityController.h"

#include <QDebug>
#include <QQmlContext>
#include <QQuickItem>

#include <algorithm>
#include <utility>

QmlParameter::QmlParameter(
    unsigned int id,
    QmlParameterCollection& collection,
    QObject* parent)
    : QObject(parent), id_(id) , collection_(collection)
{
}

double QmlParameter::value() const
{
    return collection_.value(id_);
}

void QmlParameter::setValue(double value)
{
    if (this->value() == value)
        return;
    collection_.setValue(id_, value);
    emit valueChanged();
}

void QmlParameter::notifyChanged()
{
    emit valueChanged();
}

QmlParameterCollection::QmlParameterCollection(
    Getter getter,
    Setter setter,
    QObject* parent)
    : getter_(std::move(getter)), setter_(std::move(setter)), QObject(parent)
{
}

QmlParameter* QmlParameterCollection::get(int id)
{
    if (id < 0)
        return nullptr;

    const auto parameterId = static_cast<unsigned int>(id);

    if (auto* parameter = parameters_.value(parameterId, nullptr))
        return parameter;

    auto* parameter = new QmlParameter(parameterId, *this, this);

    parameters_.insert(parameterId, parameter);
    return parameter;
}

QmlUiDataStream::QmlUiDataStream(
    quint32 streamId,
    QmlUiDataStreamCollection& collection,
    QObject* parent)
    : QObject(parent),
      streamId_(streamId),
      collection_(collection)
{
}

quint32 QmlUiDataStream::revision() const
{
    return revision_;
}

double QmlUiDataStream::sampleRate() const
{
    return sampleRate_;
}

quint32 QmlUiDataStream::rows() const
{
    return rows_;
}

quint32 QmlUiDataStream::columns() const
{
    return columns_;
}

QList<float> QmlUiDataStream::values() const
{
    return values_;
}

bool QmlUiDataStream::update()
{
    collection_.update();
    if (observedRevision_ == revision_)
        return false;

    observedRevision_ = revision_;
    return true;
}

void QmlUiDataStream::apply(
    const Singularity::UiDataExchange::DataBlock& block)
{
    sampleRate_ = block.sampleRate;
    rows_ = block.rows;
    columns_ = block.columns;
    const auto valueCount = std::min(
        block.valueCount,
        Singularity::UiDataExchange::kMaxValues);
    values_.clear();
    values_.reserve(static_cast<qsizetype>(valueCount));
    for (quint32 index = 0; index < valueCount; ++index)
        values_.append(block.values[index]);

    ++revision_;
    emit dataChanged();
}

QmlUiDataStreamCollection::QmlUiDataStreamCollection(
    Singularity::UiDataExchange::DataQueue* queue,
    QObject* parent)
    : QObject(parent), queue_(queue)
{
}

QmlUiDataStream* QmlUiDataStreamCollection::get(int streamId)
{
    if (streamId < 0)
        return nullptr;

    const auto id = static_cast<quint32>(streamId);
    auto* stream = streams_.value(id, nullptr);
    if (stream)
        return stream;

    stream = new QmlUiDataStream(id, *this, this);
    streams_.insert(id, stream);
    return stream;
}

bool QmlUiDataStreamCollection::update()
{
    if (!queue_)
        return false;

    Singularity::UiDataExchange::DataBlock block;
    bool received = false;
    while (queue_->popUiDataBlock(block))
    {
        auto* stream = streams_.value(block.streamId, nullptr);
        if (stream)
        {
            stream->apply(block);
            received = true;
        }
    }

    return received;
}

SingularityController::SingularityController(
    ParameterGetter getParameter,
    ParameterSetter setParameter,
    ActionCallback actionCallback,
    Singularity::UiDataExchange::DataQueue* uiDataQueue)
    : actionCallback_(std::move(actionCallback)),
      parameterCollection_(std::move(getParameter), std::move(setParameter), this),
      streamCollection_(uiDataQueue, this)
{
}

SingularityController::~SingularityController()
{
    detachView();
}

void SingularityController::attachToView(QQuickView& view, const QUrl& source)
{
    detachView();
    view_ = &view;
    view.rootContext()->setContextProperty(
        QStringLiteral("plugin"),
        this);

    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setTitle(QStringLiteral(PLUGIN_NAME));

    statusConnection_ = QObject::connect(
        &view,
        &QQuickView::statusChanged,
        &view,
        [&view](QQuickView::Status status)
        {
            if (status == QQuickView::Error)
            {
                for (const auto& error : view.errors())
                    qWarning().noquote() << error.toString();

                return;
            }

            if (status != QQuickView::Ready)
                return;

            auto* root = view.rootObject();
            if (!root)
                return;

            const auto version =
                root->property("singularityPluginViewVersion");

            if (!version.isValid() || version.toInt() != 1)
            {
                qCritical()
                    << "Main.qml must use PluginView as its root component";

                view.setSource({});
            }
        });

    if (source.isEmpty())
        view.loadFromModule(SINGULARITY_QML_MODULE_URI, "Main");
    else
        view.setSource(source);
}

void SingularityController::detachView()
{
    QObject::disconnect(statusConnection_);
    statusConnection_ = {};
    view_.clear();
}

QmlParameterCollection* SingularityController::parameters()
{
    return &parameterCollection_;
}

QmlUiDataStreamCollection* SingularityController::streams()
{
    return &streamCollection_;
}

QVariantMap SingularityController::data() const
{
    return data_;
}

void SingularityController::setData(std::string_view name, const ActionValue& value)
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

void SingularityController::sendAction(QString name)
{
    sendAction(std::move(name), {});
}

void SingularityController::sendAction(QString name, QVariant value)
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

    if(actionCallback_)
    {
        const auto utf8Name = name.toUtf8();
        const auto result = actionCallback_(std::string_view(utf8Name.constData(), utf8Name.size()), *actionValue);

        if(result)
        {
            const auto key = QString::fromUtf8(result->first.data(), result->first.size());

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
                result->second);

                if (data_.value(key) == dataValue)
                    return;

                data_.insert(key, dataValue);
                emit dataChanged();
        }
            // setData(result->first, result->second);
        // const auto result = actionCallback_(
        //     std::string_view(utf8Name.constData(), utf8Name.size()),
        //     std::string_view(utf8Value.constData(), utf8Value.size()));

        // if (result)
        //     setData(result->first, result->second);
    }



    // const auto utf8Name = name.toUtf8();
    // QString text;

    // if (value.metaType() == QMetaType::fromType<QUrl>())
    // {
    //     const auto url = value.toUrl();
    //     text = url.isLocalFile()
    //         ? url.toLocalFile()
    //         : url.toString(QUrl::FullyEncoded);
    // }
    // else if (value.isValid() && !value.isNull())
    // {
    //     text = value.toString();
    // }

    // const auto utf8Value = text.toUtf8();

    if (actionCallback_)
    {
        // const auto result = actionCallback_(
        //     std::string_view(utf8Name.constData(), utf8Name.size()),
        //     std::string_view(utf8Value.constData(), utf8Value.size()));

        // if (result)
        //     setData(result->first, result->second);
    }
}
