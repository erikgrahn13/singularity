#include "SingularityVST3Editor.h"

#include "pluginterfaces/base/funknownimpl.h"

#include <fstream>

#if defined(__linux__)
#include <xcb/xcb.h>
#endif


std::string hotReload()
{
    std::ifstream commandLine("/proc/self/cmdline", std::ios::binary);

    for(std::string argument; std::getline(commandLine, argument, '\0');)
    {
        if(argument.starts_with("-qmljsdebugger="))
            return argument;
    }

    return {};
}


SingularityVST3Editor::SingularityVST3Editor(Steinberg::Vst::EditController* controller)
    : Steinberg::Vst::EditorView(controller),
#if defined (__linux__)
    SingularityEditor(
        [controller](unsigned int id)
        {
            return controller->normalizedParamToPlain(id, controller->getParamNormalized(id));
        },
        [controller](unsigned int id, double value)
        {
            const double normalized = controller->plainParamToNormalized(id, value);

            controller->beginEdit(id);
            controller->setParamNormalized(id, normalized);
            controller->performEdit(id, normalized);
            controller->endEdit(id);
        },
        "xcb", hotReload())
#endif

{
    const auto size = getView().initialSize();
    setRect({0, 0, size.width(), size.height()});
}

void SingularityVST3Editor::attachedToParent()
{
    getView().setParent(QWindow::fromWinId(reinterpret_cast<WId>(systemWindow)));
    SingularityEditor::showWindow();
    Steinberg::Vst::EditorView::attachedToParent();
}

void SingularityVST3Editor::removedFromParent()
{
    if(auto runLoop = Steinberg::U::cast<Steinberg::Linux::IRunLoop>(plugFrame))
    {
        runLoop->unregisterEventHandler(this);
    }
    Steinberg::Vst::EditorView::removedFromParent();
}

Steinberg::tresult PLUGIN_API SingularityVST3Editor::setContentScaleFactor(Steinberg::IPlugViewContentScaleSupport::ScaleFactor factor)
{
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API SingularityVST3Editor::isPlatformTypeSupported(Steinberg::FIDString type)
{
    #if SMTG_OS_WINDOWS
        if(strcmp(type, Steinberg::kPlatformTypeHWND) == 0)
            return Steinberg::kResultTrue;
    #elif SMTG_OS_MACOS
        if(strcmp(type, kPlatformTypeNSView) == 0)
            return Steinberg::kResultTrue;
    #elif SMTG_OS_LINUX
        if(strcmp(type, Steinberg::kPlatformTypeX11EmbedWindowID) == 0)
            return Steinberg::kResultTrue;
        if(strcmp(type, Steinberg::kPlatformTypeWaylandSurfaceID) == 0)
            return Steinberg::kResultTrue;
    #endif

    return Steinberg::kInvalidArgument;
}

#if defined(__linux__)
Steinberg::tresult PLUGIN_API SingularityVST3Editor::setFrame(Steinberg::IPlugFrame* frame)
{
    const auto result = Steinberg::Vst::EditorView::setFrame(frame);
    if(auto runLoop = Steinberg::U::cast<Steinberg::Linux::IRunLoop>(plugFrame))
    {
        auto x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
        auto x11FileDescriptor = xcb_get_file_descriptor(x11->connection());
        runLoop->registerEventHandler(this, x11FileDescriptor);
    }
    return result;
}

void PLUGIN_API SingularityVST3Editor::onFDIsSet(Steinberg::Linux::FileDescriptor fd)
{
    QCoreApplication::processEvents(QEventLoop::AllEvents);
}

#endif
