#include "SingularityView.h"
#include "vst3controller.h"
#include "base/source/fdebug.h"
#include <filesystem>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace Steinberg {

SingularityView::SingularityView(Vst::EditController* editController)
    : Vst::EditorView(editController)
{
    static int anchor;

#if defined(_WIN32)
    HMODULE hModule = NULL;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&anchor),
        &hModule);
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(hModule, modulePath, MAX_PATH);
    std::filesystem::path dllPath(modulePath);
#else
    Dl_info info;
    dladdr(&anchor, &info);
    std::filesystem::path dllPath(info.dli_fname);
#endif

    std::filesystem::path resourcePath = dllPath.parent_path().parent_path() / "Resources";

    auto& params = static_cast<IParameterProvider&>(*static_cast<VST3Controller*>(editController));
    controller_ = std::make_unique<SingularityController>(params, resourcePath.string());

    controller_->setLogger([](const std::string& msg) {
        SMTG_DBPRT1("%s\n", msg.c_str());
        fprintf(stderr, "[singularity] %s\n", msg.c_str());
        fflush(stderr);
    });

    controller_->initialize();
    setRect({0, 0, static_cast<int32>(controller_->width()), static_cast<int32>(controller_->height())});
}

SingularityView::~SingularityView()
{
    controller_.reset();
    window_.reset();
}

tresult PLUGIN_API SingularityView::isPlatformTypeSupported(FIDString type)
{
#if _WIN32
    if (strcmp(type, kPlatformTypeHWND) == 0) return kResultTrue;
#elif __APPLE__
    if (strcmp(type, kPlatformTypeNSView) == 0) return kResultTrue;
#else
    if (strcmp(type, kPlatformTypeX11EmbedWindowID) == 0) return kResultTrue;
#endif
    return kResultFalse;
}

void SingularityView::attachedToParent()
{
    auto width = controller_->width();
    auto height = controller_->height();
    window_ = IWindow::createWindow(width, height, systemWindow);
    controller_->attachToWindow(*window_);



#if defined(__linux__)
    if (plugFrame && window_) {
        Linux::IRunLoop* runLoop = nullptr;
        if (plugFrame->queryInterface(Linux::IRunLoop::iid, (void**)&runLoop) == kResultOk && runLoop) {
            runLoop->registerEventHandler(this, window_->fd());
            runLoop->registerTimer(this, 1000 / window_->refreshRate());
            runLoop->release();
        }
    }
#endif

    Vst::EditorView::attachedToParent(); // notifies EditController
}

void SingularityView::removedFromParent()
{
#if defined(__linux__)
    if (plugFrame && window_) {
        Linux::IRunLoop* runLoop = nullptr;
        if (plugFrame->queryInterface(Linux::IRunLoop::iid, (void**)&runLoop) == kResultOk && runLoop) {
            runLoop->unregisterTimer(this);
            runLoop->unregisterEventHandler(this);
            runLoop->release();
        }
    }
#endif

    Vst::EditorView::removedFromParent(); // notifies EditController
}

tresult PLUGIN_API SingularityView::onSize(ViewRect* newSize)
{
    tresult res = CPluginView::onSize(newSize);
    if (res == kResultTrue && window_)
    {
        window_->resize(rect.right - rect.left, rect.bottom - rect.top);
    }
    return res;
}

} // namespace Steinberg
