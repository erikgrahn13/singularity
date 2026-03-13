#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>

#include "include/core/SkPixmap.h"


#include "../SingularityGraphics.h"
#include "../SingularityGraphicsWin.h"
#include "../TestWidget.h"

// #include "include/core/SkData.h"
// #include "include/core/SkImage.h"
// #include "include/core/SkStream.h"
// #include "include/core/SkSurface.h"
// #include "include/core/SkCanvas.h"
// #include "include/core/SkRRect.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
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
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // All painting occurs here, between BeginPaint and EndPaint.

            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

            // graphics->DrawRectangle();
            TestWidget testWidget;
            testWidget.draw(*graphics);

            SingularityGraphicsWin::Blit(hdc, *graphics); 


            EndPaint(hwnd, &ps);
        }
        return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}