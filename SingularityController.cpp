#include "SingularityController.h"

#include <iostream>

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QFileInfo>
#include <QQmlEngine>
#include <QQmlContext>
#include <QUrl>

QmlParameter::QmlParameter(int id, SingularityController& controller, QObject* parent)
    : QObject(parent), id_(id), controller_(controller)
{
    
}

double QmlParameter::value() const
{
    return controller_.getParameterValue(id_);
}

void QmlParameter::setValue(double value)
{
    if (this->value() == value)
        return;
    controller_.setParameterValue(id_, value);
    emit valueChanged();
}

void QmlParameter::notifyChanged()
{
    emit valueChanged();
}

SingularityController::SingularityController(IParameterBackend& parameterBackend)
    : parameterBackend_(parameterBackend)
{

    for (const auto& definition :parameterBackend_.parameterDefinitions())
    {
        const int id = static_cast<int>(definition.id);
        auto* parameter = new QmlParameter(id, *this, this);
        qmlParameters_.insert(id, parameter);
    }

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

void SingularityController::attachToView(QQuickView& view)
{
    detachView();
    view_ = &view;
    view.rootContext()->setContextProperty(QStringLiteral("parameters"), this);

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

// QObject* SingularityController::parameter(int id) const
// {
//     auto* object = parameters_.value(id, nullptr);
//     qWarning() << "parameter ID:" << id;

//     if (!object)
//         qWarning() << "Unknown parameter ID:" << id;
//     return object;
// }



// double SingularityController::value()
// {
//     return 0.0;
// }

// void SingularityController::setValue(double value)
// {
// }
QObject* SingularityController::get(int id) const
{
    return qmlParameters_.value(id, nullptr);
}


double SingularityController::getParameterValue(int id) const
{
    return parameterBackend_.getParameter(id);
}

void SingularityController::setParameterValue(int id, double value)
{
    parameterBackend_.setParameter(id, value);
}
