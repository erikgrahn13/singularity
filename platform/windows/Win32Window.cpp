#include "Win32Window.h"

std::unique_ptr<IWindow> IWindow::createWindow(int width, int height, void* parentWindow) {
    return std::make_unique<Win32Window>("Singularity", width, height, parentWindow, parentWindow != nullptr);
}
