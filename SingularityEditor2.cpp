#include "SingularityEditor2.h"

#include "QGuiApplication"
#include <QQmlContext>

SingularityEditor::SingularityEditor(QmlParameter::Getter getter, QmlParameter::Setter setter, const char* platform, std::string hotReloadArgument)
    : getter_(std::move(getter)), setter_(std::move(setter))
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

#if defined(SINGULARITY_QML_SOURCE_FILE)
    view_->setSource(
        QUrl::fromLocalFile(QStringLiteral(SINGULARITY_QML_SOURCE_FILE)));
#else
    view_->loadFromModule(SINGULARITY_QML_MODULE_URI, "Main");
#endif
    view_->setResizeMode(QQuickView::SizeRootObjectToView);
    view_->setTitle(QStringLiteral(PLUGIN_NAME));

}

SingularityEditor::~SingularityEditor()
{

}

void SingularityEditor::showWindow()
{
    view_->show();
}

QmlParameter* SingularityEditor::parameter(int id)
{
    if(auto it = parameters_.find(id); it != parameters_.end())
        return it->second;

    auto* parameter = new QmlParameter(id, getter_, setter_, this);
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