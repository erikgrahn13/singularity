#include "SingularityView.h"
#include "vst3controller.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFileInfo>
#include <QGuiApplication>

#if defined(__linux__)
#  include <dlfcn.h>
#  include <xcb/xcb.h>
#endif

namespace {

void ensureQtApplication()
{
    if (QCoreApplication::instance())
        return;

    QCoreApplication::setAttribute(Qt::AA_PluginApplication);

#if defined(__linux__)
    static QByteArray platformPluginPath = []
    {
        Dl_info moduleInfo {};
        if (dladdr(reinterpret_cast<void*>(&ensureQtApplication), &moduleInfo) == 0)
            return QByteArray {};

        return (QFileInfo(QString::fromLocal8Bit(moduleInfo.dli_fname)).absolutePath()
                + QStringLiteral("/qt"))
            .toLocal8Bit();
    }();

    static int argc = 5;
    static char applicationName[] = "singularity-vst3";
    static char platformOption[] = "-platform";
    static char platformName[] = "xcb";
    static char platformPluginPathOption[] = "-platformpluginpath";
    static char* argv[] = {
        applicationName,
        platformOption,
        platformName,
        platformPluginPathOption,
        platformPluginPath.data(),
        nullptr
    };
#else
    static int argc = 1;
    static char applicationName[] = "singularity-vst3";
    static char* argv[] = {applicationName, nullptr};
#endif

    static QGuiApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);
}

} // namespace

namespace Steinberg {

SingularityView::SingularityView(Vst::EditController* editController)
    : Vst::EditorView(editController)
{
    ensureQtApplication();

    auto* vstController = static_cast<VST3Controller*>(editController);
    controller_ = std::make_unique<SingularityController>(*vstController);

    view_ = std::make_unique<QQuickView>();
    QObject::connect(
        view_.get(),
        &QQuickView::statusChanged,
        view_.get(),
        [this](QQuickView::Status status)
        {
            if (status != QQuickView::Ready || !plugFrame)
                return;

            const QSize size = view_->initialSize();
            if (size.width() <= 0 || size.height() <= 0)
                return;

            ViewRect requested {
                0,
                0,
                static_cast<int32>(size.width()),
                static_cast<int32>(size.height())
            };
            plugFrame->resizeView(this, &requested);
        });

    controller_->attachToView(*view_);

    const QSize size = view_->initialSize();
    setRect({
        0,
        0,
        static_cast<int32>(size.width()),
        static_cast<int32>(size.height())
    });
}

tresult PLUGIN_API SingularityView::isPlatformTypeSupported(FIDString type)
{
#if defined(_WIN32)
    if (strcmp(type, kPlatformTypeHWND) == 0)
        return kResultTrue;
#elif defined(__APPLE__)
    if (strcmp(type, kPlatformTypeNSView) == 0)
        return kResultTrue;
#else
    if (strcmp(type, kPlatformTypeX11EmbedWindowID) == 0)
        return kResultTrue;
#endif
    return kResultFalse;
}

void SingularityView::attachedToParent()
{
#if defined(__linux__)
    if (plugFrame)
    {
        Linux::IRunLoop* runLoop = nullptr;
        if (plugFrame->queryInterface(
                Linux::IRunLoop::iid,
                reinterpret_cast<void**>(&runLoop)) == kResultOk &&
            runLoop)
        {
            if (auto* x11Application =
                    qGuiApp->nativeInterface<QNativeInterface::QX11Application>())
            {
                qtXcbFd_ = xcb_get_file_descriptor(x11Application->connection());
                if (qtXcbFd_ >= 0)
                {
                    eventHandlerRegistered_ =
                        runLoop->registerEventHandler(this, qtXcbFd_) == kResultOk;
                }
            }

            timerRegistered_ = runLoop->registerTimer(this, 16) == kResultOk;
            runLoop->release();
        }
    }
#endif

    parentWindow_.reset(
        QWindow::fromWinId(reinterpret_cast<WId>(systemWindow)));
    if (!parentWindow_)
        return;

    view_->setParent(parentWindow_.get());
    view_->setGeometry(
        0,
        0,
        rect.right - rect.left,
        rect.bottom - rect.top);
    view_->show();

    Vst::EditorView::attachedToParent();
}

void SingularityView::removedFromParent()
{
#if defined(__linux__)
    if (plugFrame)
    {
        Linux::IRunLoop* runLoop = nullptr;
        if (plugFrame->queryInterface(
                Linux::IRunLoop::iid,
                reinterpret_cast<void**>(&runLoop)) == kResultOk &&
            runLoop)
        {
            if (eventHandlerRegistered_)
                runLoop->unregisterEventHandler(this);
            if (timerRegistered_)
                runLoop->unregisterTimer(this);
            runLoop->release();
        }
    }

    eventHandlerRegistered_ = false;
    timerRegistered_ = false;
    qtXcbFd_ = -1;
#endif

    view_->hide();
    view_->setParent(nullptr);
    parentWindow_.reset();

    Vst::EditorView::removedFromParent();
}

#if defined(__linux__)
void PLUGIN_API SingularityView::onFDIsSet(Linux::FileDescriptor fd)
{
    if (fd == qtXcbFd_)
        QCoreApplication::processEvents(QEventLoop::AllEvents);
}

void PLUGIN_API SingularityView::onTimer()
{
    QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}
#endif

tresult PLUGIN_API SingularityView::onSize(ViewRect* newSize)
{
    if (!newSize)
        return kInvalidArgument;

    const tresult result = CPluginView::onSize(newSize);
    if (result == kResultTrue && view_)
    {
        view_->setGeometry(
            0,
            0,
            newSize->right - newSize->left,
            newSize->bottom - newSize->top);
    }
    return result;
}

} // namespace Steinberg
