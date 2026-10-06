#include "SingularityController.h"

#include <QDebug>
#include <QQmlContext>
#include <QQuickItem>

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

SingularityController::SingularityController(
    IParameterBackend& parameterBackend,
    ActionCallback actionCallback)
    : actionCallback_(std::move(actionCallback)),
      parameterCollection_(parameterBackend, this)
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
