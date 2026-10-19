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