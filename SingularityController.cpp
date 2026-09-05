#include "SingularityController.h"

#include <iostream>

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QFileInfo>
#include <QQmlEngine>
#include <QUrl>

SingularityController::SingularityController(
    IParameterProvider& parameterProvider,
    std::string_view resourcePath,
    Singularity::AudioDataExchange::AudioDataQueue* audioDataQueue)
    : parameterProvider_(parameterProvider)
{
#if defined(SINGULARITY_QML_SOURCE_FILE)
    qmlFile_ = QFileInfo(
        QStringLiteral(SINGULARITY_QML_SOURCE_FILE)).absoluteFilePath();
    qmlWatcher_.addPath(qmlFile_);
    qmlWatcher_.addPath(QFileInfo(qmlFile_).absolutePath());

    reloadTimer_.setInterval(100);
    reloadTimer_.setSingleShot(true);

    QObject::connect(
        &qmlWatcher_,
        &QFileSystemWatcher::fileChanged,
        &reloadTimer_,
        [this](const QString&) { reloadTimer_.start(); });
    QObject::connect(
        &qmlWatcher_,
        &QFileSystemWatcher::directoryChanged,
        &reloadTimer_,
        [this](const QString&) { reloadTimer_.start(); });
    QObject::connect(
        &reloadTimer_,
        &QTimer::timeout,
        &qmlWatcher_,
        [this]()
        {
            if (QFileInfo::exists(qmlFile_) &&
                !qmlWatcher_.files().contains(qmlFile_))
                qmlWatcher_.addPath(qmlFile_);

            if (!view_)
                return;

            const auto qmlUrl = QUrl::fromLocalFile(qmlFile_);
            view_->setSource({});
            view_->engine()->clearComponentCache();
            view_->setSource(qmlUrl);

            if (logger_)
                logger_("Loaded " + qmlUrl.toString().toStdString());
        });
#endif
}

SingularityController::~SingularityController()
{
    detachView();
}

void SingularityController::setLogger(LogCallback callback)
{
    logger_ = std::move(callback);
}

void SingularityController::tick()
{
    QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

void SingularityController::registerImage(
    const std::string& name,
    const uint8_t* data,
    int size)
{
}

void SingularityController::attachToView(QQuickView& view)
{
    detachView();
    view_ = &view;

    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.setTitle(QStringLiteral("Hello World"));

    statusConnection_ = QObject::connect(
        &view,
        &QQuickView::statusChanged,
        &view,
        [&view](QQuickView::Status status)
        {
            if (status != QQuickView::Error)
                return;

            for (const auto& error : view.errors())
                qWarning().noquote() << error.toString();
        });

#if defined(SINGULARITY_QML_SOURCE_FILE)
    view.setSource(QUrl::fromLocalFile(qmlFile_));
#else
    view.loadFromModule(SINGULARITY_QML_MODULE_URI, "Main");
#endif
}

void SingularityController::detachView()
{
    QObject::disconnect(statusConnection_);
    statusConnection_ = {};
    view_.clear();
}
