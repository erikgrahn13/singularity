#pragma once

#include "IParameterBackend.h"

#include "AudioDataExchange.h"

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QQuickView>
#include <QString>
#include <QUrl>
#include <QVariant>

#include <functional>
#include <string_view>

class QmlParameter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)

public:
    explicit QmlParameter(
        int id,
        IParameterBackend& backend,
        QObject* parent = nullptr);

    double value() const;
    void setValue(double value);
    void notifyChanged();

signals:
    void valueChanged();

private:
    int id_;
    IParameterBackend& backend_;
};

class QmlParameterCollection : public QObject
{
    Q_OBJECT

public:
    explicit QmlParameterCollection(
        IParameterBackend& backend,
        QObject* parent = nullptr);

    Q_INVOKABLE QmlParameter* get(int id) const;

private:
    QHash<int, QmlParameter*> parameters_;
};

class QmlAudioData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 revision READ revision NOTIFY dataChanged)
    Q_PROPERTY(quint32 sampleRate READ sampleRate NOTIFY dataChanged)
    Q_PROPERTY(int numChannels READ numChannels NOTIFY dataChanged)
    Q_PROPERTY(QList<float> samples READ samples NOTIFY dataChanged)

public:
    explicit QmlAudioData(
        Singularity::AudioDataExchange::AudioDataQueue* queue,
        QObject* parent = nullptr);

    quint32 revision() const;
    quint32 sampleRate() const;
    int numChannels() const;
    QList<float> samples() const;

    Q_INVOKABLE bool update();

signals:
    void dataChanged();

private:
    Singularity::AudioDataExchange::AudioDataQueue* queue_;
    quint32 revision_ = 0;
    quint32 sampleRate_ = 0;
    int numChannels_ = 0;
    QList<float> samples_;
};

class SingularityController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
        QmlParameterCollection* parameters
        READ parameters
        CONSTANT)
    Q_PROPERTY(
        QmlAudioData* audioData
        READ audioData
        CONSTANT)

public:
    using ActionCallback =
        std::function<void(std::string_view, std::string_view)>;

    SingularityController(
        IParameterBackend& parameterBackend,
        ActionCallback actionCallback,
        Singularity::AudioDataExchange::AudioDataQueue* audioDataQueue = nullptr);
    ~SingularityController();

    void attachToView(QQuickView& view, const QUrl& source = {});
    void detachView();

    QmlParameterCollection* parameters();
    QmlAudioData* audioData();

    Q_INVOKABLE void sendAction(QString name);
    Q_INVOKABLE void sendAction(QString name, QVariant value);

private:
    QPointer<QQuickView> view_;
    QMetaObject::Connection statusConnection_;
    ActionCallback actionCallback_;
    QmlParameterCollection parameterCollection_;
    QmlAudioData audioData_;
};
