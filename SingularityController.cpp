#include "SingularityController.h"

#include <QDebug>
#include <QQmlContext>
#include <QQuickItem>

#include <algorithm>
#include <utility>

QmlParameter::QmlParameter(
    int id,
    IParameterBackend& backend,
    QObject* parent)
    : QObject(parent), id_(id), backend_(backend)
{
}

double QmlParameter::value() const
{
    return backend_.getParameter(id_);
}

void QmlParameter::setValue(double value)
{
    if (this->value() == value)
        return;
    backend_.setParameter(id_, value);
    emit valueChanged();
}

void QmlParameter::notifyChanged()
{
    emit valueChanged();
}

QmlParameterCollection::QmlParameterCollection(
    IParameterBackend& backend,
    QObject* parent)
    : QObject(parent)
{
    for (const auto& definition : backend.parameterDefinitions())
    {
        const int id = static_cast<int>(definition.id);
        parameters_.insert(id, new QmlParameter(id, backend, this));
    }
}

QmlParameter* QmlParameterCollection::get(int id) const
{
    return parameters_.value(id, nullptr);
}

QmlAudioData::QmlAudioData(
    Singularity::AudioDataExchange::AudioDataQueue* queue,
    QObject* parent)
    : QObject(parent), queue_(queue)
{
}

quint32 QmlAudioData::revision() const
{
    return revision_;
}

quint32 QmlAudioData::sampleRate() const
{
    return sampleRate_;
}

int QmlAudioData::numChannels() const
{
    return numChannels_;
}

QList<float> QmlAudioData::samples() const
{
    return samples_;
}

bool QmlAudioData::update()
{
    if (!queue_)
        return false;

    using Singularity::AudioDataExchange::AudioDataBlock;
    using Singularity::AudioDataExchange::kMaxFloatSamples;

    AudioDataBlock block;
    QList<float> samples;
    bool received = false;
    quint32 sampleRate = sampleRate_;
    int numChannels = numChannels_;

    while (queue_->popAudioDataBlock(block))
    {
        const int channels = static_cast<int>(block.numChannels);
        const int availableSamples = static_cast<int>(std::min<std::uint32_t>(
            block.numSamples,
            kMaxFloatSamples));
        const int blockSamples = channels > 0
            ? availableSamples - availableSamples % channels
            : 0;
        if (blockSamples <= 0 || block.sampleSize != sizeof(float))
            continue;

        const bool formatChanged = received &&
            (sampleRate != block.sampleRate ||
             numChannels != channels);
        if (formatChanged)
            samples.clear();

        sampleRate = block.sampleRate;
        numChannels = channels;

        const int capacity = static_cast<int>(kMaxFloatSamples) -
            static_cast<int>(kMaxFloatSamples) % channels;
        const int currentSize = static_cast<int>(samples.size());
        const int overflow = currentSize + blockSamples - capacity;
        if (overflow > 0)
            samples.remove(0, std::min(overflow, currentSize));

        const int sourceOffset = static_cast<int>(block.numSamples) - blockSamples;
        samples.reserve(std::min(
            capacity,
            static_cast<int>(samples.size()) + blockSamples));
        for (int index = 0; index < blockSamples; ++index)
            samples.append(block.samples[sourceOffset + index]);

        received = true;
    }

    if (!received)
        return false;

    sampleRate_ = sampleRate;
    numChannels_ = numChannels;
    samples_ = std::move(samples);
    ++revision_;
    emit dataChanged();
    return true;
}

SingularityController::SingularityController(
    IParameterBackend& parameterBackend,
    ActionCallback actionCallback,
    Singularity::AudioDataExchange::AudioDataQueue* audioDataQueue)
    : actionCallback_(std::move(actionCallback)),
      parameterCollection_(parameterBackend, this),
      audioData_(audioDataQueue, this)
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

QmlAudioData* SingularityController::audioData()
{
    return &audioData_;
}

void SingularityController::sendAction(QString name)
{
    sendAction(std::move(name), {});
}

void SingularityController::sendAction(QString name, QVariant value)
{
    const auto utf8Name = name.toUtf8();
    QString text;

    if (value.metaType() == QMetaType::fromType<QUrl>())
    {
        const auto url = value.toUrl();
        text = url.isLocalFile()
            ? url.toLocalFile()
            : url.toString(QUrl::FullyEncoded);
    }
    else if (value.isValid() && !value.isNull())
    {
        text = value.toString();
    }

    const auto utf8Value = text.toUtf8();

    if (actionCallback_)
    {
        actionCallback_(
            std::string_view(utf8Name.constData(), utf8Name.size()),
            std::string_view(utf8Value.constData(), utf8Value.size()));
    }
}
