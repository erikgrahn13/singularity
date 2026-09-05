#pragma once

#include "IParameterProvider.h"
#include "AudioDataExchange.h"

#include <functional>
#include <string>
#include <string_view>

#include <QFileSystemWatcher>
#include <QMetaObject>
#include <QPointer>
#include <QQuickView>
#include <QString>
#include <QTimer>

class SingularityController
{
public:
    using LogCallback = std::function<void(const std::string&)>;

    SingularityController(
        IParameterProvider& parameterProvider,
        std::string_view resourcePath = "",
        Singularity::AudioDataExchange::AudioDataQueue* audioDataQueue = nullptr);
    ~SingularityController();

    void tick();
    void setLogger(LogCallback callback);
    void registerImage(const std::string& name, const uint8_t* data, int size);

    void attachToView(QQuickView& view);
    void detachView();

private:
    QPointer<QQuickView> view_;
    QFileSystemWatcher qmlWatcher_;
    QTimer reloadTimer_;
    QString qmlFile_;
    QMetaObject::Connection statusConnection_;
    LogCallback logger_;
    IParameterProvider& parameterProvider_;
};
