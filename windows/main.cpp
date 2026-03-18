#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>

#include "include/core/SkPixmap.h"
#include "../SingularityGraphics2.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // Attach to parent console for debug output (remove for release)
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);
    }

    // Register the window class.
    const wchar_t CLASS_NAME[]  = L"Sample Window Class";
    
    WNDCLASS wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Learn to Program Windows",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    auto graphics = std::make_unique<SingularityGraphics>();
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)graphics.get());

    // graphics->setOnFileChanged([hwnd]() {
    //     InvalidateRect(hwnd, nullptr, FALSE);
    // });

    // Run the message loop.

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SingularityGraphics* graphics = (SingularityGraphics*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!graphics) return DefWindowProc(hwnd, uMsg, wParam, lParam);


    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        {
            // graphics->reloadScript(); // no-op if nothing changed
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // All painting occurs here, between BeginPaint and EndPaint.

            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

            // blit Skia surface to window
            auto render = graphics->getRenderData();
            if (render.contentAddres) {
                BITMAPINFO bmi = {};
                bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth       = render.width;
                bmi.bmiHeader.biHeight      = -render.height; // top-down
                bmi.bmiHeader.biPlanes      = 1;
                bmi.bmiHeader.biBitCount    = 32;
                bmi.bmiHeader.biCompression = BI_RGB;
                SetDIBitsToDevice(hdc,
                    0, 0, render.width, render.height,
                    0, 0, 0, render.height,
                    render.contentAddres, &bmi, DIB_RGB_COLORS);
            }


            EndPaint(hwnd, &ps);
        }
        return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}