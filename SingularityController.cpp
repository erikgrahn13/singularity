#include "SingularityController.h"

#include <QDebug>
#include <QQmlContext>
#include <QQuickItem>

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
}

SingularityController::~SingularityController()
{
    detachView();
}

void SingularityController::attachToView(QQuickView& view, const QUrl& source)
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
