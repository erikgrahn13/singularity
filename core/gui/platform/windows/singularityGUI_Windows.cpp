// #ifdef _WIN32

#include "singularityGUI_Windows.h"
#include "../../singularity_ResourceManager.h"
#include <iostream>
#include <shlobj.h>
#include <shlwapi.h>

// Platform-specific factory function implementation
std::unique_ptr<ISingularityGUI> ISingularityGUI::createView()
{
    return std::make_unique<WebViewWindows>();
}

WebViewWindows::WebViewWindows() : m_hwnd(nullptr), m_webviewReady(false), m_width(800), m_height(600)
{
}

WebViewWindows::~WebViewWindows()
{
    close();
}

void WebViewWindows::create(int width, int height, const std::string &title)
{
    m_width = width;
    m_height = height;

    // Convert title properly
    std::wstring wideTitle;
    if (!title.empty())
    {
        // Use MultiByteToWideChar for proper UTF-8 to UTF-16 conversion
        int wideLength = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
        if (wideLength > 0)
        {
            wideTitle.resize(wideLength - 1); // -1 to exclude null terminator
            MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wideTitle[0], wideLength);
        }
        else
        {
            wideTitle = L"Singularity WebView";
        }
    }
    else
    {
        wideTitle = L"Singularity WebView";
    }

    // Debug output
    std::string debugMsg = "Creating window with title: '" + title + "'\n";
    OutputDebugStringA(debugMsg.c_str());

    // Use a hybrid approach: create with BUTTON class (which supports titles well)
    // but make it look like a normal window
    m_hwnd = CreateWindowExW(0,                            // Extended styles
                             L"BUTTON",                    // Use BUTTON class (handles titles properly)
                             wideTitle.c_str(),            // Window title
                             WS_OVERLAPPEDWINDOW,          // Standard window styles
                             CW_USEDEFAULT, CW_USEDEFAULT, // Position
                             width, height,                // Size
                             nullptr,                      // Parent
                             nullptr,                      // Menu
                             GetModuleHandle(nullptr),     // Instance
                             nullptr                       // No lpParam for now
    );

    if (m_hwnd)
    {
        OutputDebugStringA("Window created successfully\n");

        // Store our object pointer for later retrieval
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

        // Subclass the window to use our window procedure
        SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);

        // Verify the window title
        wchar_t actualTitle[256] = {0};
        GetWindowTextW(m_hwnd, actualTitle, 256);

        char debugTitle[512] = {0};
        WideCharToMultiByte(CP_UTF8, 0, actualTitle, -1, debugTitle, 512, nullptr, nullptr);
        std::string titleDebug = "Final window title: '" + std::string(debugTitle) + "'\n";
        OutputDebugStringA(titleDebug.c_str());

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        createWebView();
    }
    else
    {
        DWORD error = GetLastError();
        std::string errorMsg = "Window creation failed, error: " + std::to_string(error) + "\n";
        OutputDebugStringA(errorMsg.c_str());
    }
}

void WebViewWindows::createAsChild(void *parentWindow, int width, int height)
{
    if (!parentWindow)
        return;

    m_width = width;
    m_height = height;

    HWND parentHwnd = static_cast<HWND>(parentWindow);

    // Register a proper window class for WebView2 child windows
    const wchar_t *className = L"SingularityWebViewChild";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    // Register class (ignore if already exists)
    RegisterClassW(&wc);

    // Create child window with proper window class and styles for WebView2
    m_hwnd =
        CreateWindowExW(0, className, L"Singularity WebView", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                        0, 0, width, height, parentHwnd, nullptr, GetModuleHandle(nullptr), this);

    if (m_hwnd)
    {
        // Ensure window is visible and updated
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        createWebView();
    }
}

void WebViewWindows::navigate(const std::string &url)
{
    // Debug output to see if navigate is being called
    std::string debugMsg = "Navigate called with URL: " + url + "\n";
    OutputDebugStringA(debugMsg.c_str());

    if (m_webview)
    {
        OutputDebugStringA("WebView ready, navigating now\n");
        std::wstring wideUrl(url.begin(), url.end());
        m_webview->Navigate(wideUrl.c_str());
    }
    else
    {
        OutputDebugStringA("WebView not ready, storing pending URL\n");
        m_pendingUrl = url; // Store for later when WebView is ready
    }
}

void WebViewWindows::resize(int width, int height)
{
    m_width = width;
    m_height = height;

    if (m_hwnd)
    {
        SetWindowPos(m_hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
        setWebViewBounds();
    }
}

void WebViewWindows::run()
{
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void WebViewWindows::close()
{
    if (m_webview)
    {
        m_webview = nullptr;
    }

    if (m_controller)
    {
        m_controller->Close();
        m_controller = nullptr;
    }

    if (m_environment)
    {
        m_environment = nullptr;
    }

    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    m_webviewReady = false;
}

void WebViewWindows::createWebView()
{
    OutputDebugStringA("Creating WebView2 environment...\n");

    // Get a writable user data folder path
    std::wstring userDataFolder;
    PWSTR appDataPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &appDataPath)))
    {
        userDataFolder = std::wstring(appDataPath) + L"\\Singularity\\WebView2";
        CoTaskMemFree(appDataPath);
    }
    else
    {
        // Fallback to temp directory
        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);
        userDataFolder = std::wstring(tempPath) + L"SingularityWebView2";
    }

    OutputDebugStringA("Using user data folder for WebView2\n");

    // Create WebView2 environment with specific user data folder
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,                // Use default WebView2 runtime
        userDataFolder.c_str(), // Use writable user data folder
        nullptr,                // Default options
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment *environment) -> HRESULT {
                if (SUCCEEDED(result))
                {
                    OutputDebugStringA("WebView2 environment created successfully\n");
                    m_environment = environment;

                    // Create WebView2 controller
                    environment->CreateCoreWebView2Controller(
                        m_hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                                    [this](HRESULT result, ICoreWebView2Controller *controller) -> HRESULT {
                                        if (SUCCEEDED(result))
                                        {
                                            OutputDebugStringA("WebView2 controller created successfully\n");
                                            onWebViewCreated(controller);
                                        }
                                        else
                                        {
                                            OutputDebugStringA("WebView2 controller creation FAILED\n");
                                        }
                                        return S_OK;
                                    })
                                    .Get());
                }
                else
                {
                    OutputDebugStringA("WebView2 environment creation FAILED\n");
                }
                return S_OK;
            })
            .Get());
}

void WebViewWindows::onWebViewCreated(ICoreWebView2Controller *controller)
{
    OutputDebugStringA("onWebViewCreated called\n");

    m_controller = controller;
    m_controller->get_CoreWebView2(&m_webview);

    setWebViewBounds();
    m_webviewReady = true;

    OutputDebugStringA("WebView is now ready\n");

    // Navigate to pending URL if any
    if (!m_pendingUrl.empty())
    {
        OutputDebugStringA("Found pending URL, navigating...\n");
        navigate(m_pendingUrl);
        m_pendingUrl.clear();
    }
    else
    {
        OutputDebugStringA("No pending URL found\n");
    }
}

void WebViewWindows::setWebViewBounds()
{
    if (m_controller && m_hwnd)
    {
        RECT bounds;
        GetClientRect(m_hwnd, &bounds);
        m_controller->put_Bounds(bounds);
    }
}

LRESULT CALLBACK WebViewWindows::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WebViewWindows *pThis = nullptr;

    if (uMsg == WM_NCCREATE)
    {
        // For subclassed windows, we need to get the pointer differently
        pThis = (WebViewWindows *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (!pThis)
        {
            // If not set yet, try the traditional way
            LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
            pThis = (WebViewWindows *)pcs->lpCreateParams;
            if (pThis)
            {
                SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            }
        }
    }
    else
    {
        pThis = (WebViewWindows *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis)
    {
        switch (uMsg)
        {
        case WM_SIZE:
            if (pThis->m_controller)
            {
                pThis->setWebViewBounds();
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        // Let the default window procedure handle title-related messages
        case WM_SETTEXT:
        case WM_GETTEXT:
        case WM_GETTEXTLENGTH:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// #endif
