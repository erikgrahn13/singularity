#include "SingularityVST3Editor.h"
#include PLUGIN_CLASS_HEADER
#include "controller.h"

#include "pluginterfaces/base/funknownimpl.h"


#include <fstream>

#if defined(__linux__)
#include <xcb/xcb.h>
#elif defined(__APPLE__) && defined(QT_QML_DEBUG)
#include <crt_externs.h>
#endif


#if defined(QT_QML_DEBUG)
std::string hotReload()
{
#if defined(__linux__)
    std::ifstream commandLine("/proc/self/cmdline", std::ios::binary);

    for(std::string argument; std::getline(commandLine, argument, '\0');)
    {
        if(argument.starts_with("-qmljsdebugger="))
        return argument;
    }
#elif defined(__APPLE__)
    const int argumentCount = *_NSGetArgc();
    char** arguments = *_NSGetArgv();

    for(int i = 1; i < argumentCount; ++i)
    {
        std::string argument = arguments[i];
        if(argument.starts_with("-qmljsdebugger="))
            return argument;
    }
#endif
    return {};
}

#endif


SingularityVST3Editor::SingularityVST3Editor(Steinberg::Vst::EditController* controller)
    : Steinberg::Vst::EditorView(controller),
    SingularityEditor(
#if defined (__linux__)
        "xcb"
#else 
        nullptr
#endif
#if defined(QT_QML_DEBUG)
        , hotReload()
#endif
    )

{
    initializeView();

    const auto size = getView().initialSize();
    setRect({0, 0, size.width(), size.height()});
}

double SingularityVST3Editor::getParameter(unsigned int id)
{
    return controller->normalizedParamToPlain(id, controller->getParamNormalized(id));
}

void SingularityVST3Editor::setParameter(unsigned int id, double value)
{
    const double normalized = controller->plainParamToNormalized(id, value);
    
    controller->beginEdit(id);
    controller->setParamNormalized(id, normalized);
    controller->performEdit(id, normalized);
    controller->endEdit(id);
}

void SingularityVST3Editor::attachedToParent()
{
    getView().setParent(QWindow::fromWinId(reinterpret_cast<WId>(systemWindow)));
    SingularityEditor::showWindow();
    Steinberg::Vst::EditorView::attachedToParent();
}

void SingularityVST3Editor::removedFromParent()
{
#if defined(__linux__)
    if(auto runLoop = Steinberg::U::cast<Steinberg::Linux::IRunLoop>(plugFrame))
    {
        runLoop->unregisterEventHandler(this);
    }
#endif
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
        if(strcmp(type, Steinberg::kPlatformTypeNSView) == 0)
            return Steinberg::kResultTrue;
    #elif SMTG_OS_LINUX
        if(strcmp(type, Steinberg::kPlatformTypeX11EmbedWindowID) == 0)
            return Steinberg::kResultTrue;
        if(strcmp(type, Steinberg::kPlatformTypeWaylandSurfaceID) == 0)
            return Steinberg::kResultTrue;
    #endif

    return Steinberg::kInvalidArgument;
}

Steinberg::tresult PLUGIN_API SingularityVST3Editor::canResize() 
{ 
    return PLUGIN_CLASS::isResizable ? Steinberg::kResultTrue : Steinberg::kResultFalse; 
}

Steinberg::tresult PLUGIN_API SingularityVST3Editor::checkSizeConstraint(Steinberg::ViewRect* rectToCheck)
{
    // TODO: investigate this function a little more and what constraints should exist
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SingularityVST3Editor::onSize(Steinberg::ViewRect* newSize)
{
    getView().resize(newSize->right - newSize->left, newSize->bottom - newSize->top);
    return EditorView::onSize (newSize);
}

void SingularityVST3Editor::requestResize(int width, int height)
{
    Steinberg::ViewRect requested{0, 0, width, height};
    plugFrame->resizeView(this, &requested);
}

void SingularityVST3Editor::dispatchAction(std::string_view name, const ActionValue& value)
{
    int hej;
    hej = 3;
    auto* vst3Controller = static_cast<VST3Controller*>(getController());
    vst3Controller->sendAction(name, value);
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
