// #ifdef _WIN32

#include "singularityGUI_Windows.h"
#include "../../singularity_ResourceManager.h"
#include <iostream>
#include <shlobj.h>
#include <shlwapi.h>

// Platform-specific factory function implementation
std::unique_ptr<ISingularityGUI> ISingularityGUI::createView(void *windowHandle)
{
    return std::make_unique<WebViewWindows>(windowHandle);
}

WebViewWindows::WebViewWindows(void *windowHandle) : m_hwnd(static_cast<HWND>(windowHandle))
{
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

    CreateCoreWebView2EnvironmentWithOptions(
        nullptr,                // Use default WebView2 runtime
        userDataFolder.c_str(), // Use writable user data folder
        nullptr,                // Default options
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([this](HRESULT result,
                                                                                    ICoreWebView2Environment *env)
                                                                                 -> HRESULT {
            m_environment = env;
            env->CreateCoreWebView2Controller(
                m_hwnd,
                Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([this](HRESULT result,
                                                                                           ICoreWebView2Controller
                                                                                               *controller) -> HRESULT {
                    if (controller)
                    {
                        m_webviewController = controller;
                        m_webviewController->get_CoreWebView2(&m_webview);
                    }

                    if (m_webview == nullptr)
                    {
                        return S_OK;
                    }

                    wil::com_ptr<ICoreWebView2Settings> settings;
                    m_webview->get_Settings(&settings);
                    settings->put_IsScriptEnabled(TRUE);
                    settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                    settings->put_IsWebMessageEnabled(TRUE);

                    RECT bounds;
                    GetClientRect(m_hwnd, &bounds);
                    m_webviewController->put_Bounds(bounds);

                    // Add init scripts to execute on document creation
                    // for (const auto &script : m_initScripts)
                    // {
                    //     std::wstring wideScript(script.begin(), script.end());
                    //     m_webview->AddScriptToExecuteOnDocumentCreated(wideScript.c_str(), nullptr);
                    // }
                    // if (m_webviewReady)
                    // {
                    //     m_viewReadyCallback();
                    // }

                    m_webview->AddScriptToExecuteOnDocumentCreated(
                        L"window.requestAudioDevicesFromCpp = function() { /* C++ callback here */ }", nullptr);

                    // Add navigation completed handler
                    m_webview->add_NavigationCompleted(
                        Callback<ICoreWebView2NavigationCompletedEventHandler>(
                            [this](ICoreWebView2 *sender, ICoreWebView2NavigationCompletedEventArgs *args) -> HRESULT {
                                // // NOW send the message - page is loaded
                                // std::wstring message = L"{\"name\": \"John\", \"age\": 30}";
                                // m_webview->PostWebMessageAsJson(message.c_str());

                                for (const auto &script : m_pendingScripts)
                                {
                                    std::wstring wideScript(script.begin(), script.end());
                                    m_webview->ExecuteScript(wideScript.c_str(), nullptr);
                                }
                                m_pendingScripts.clear();
                                return S_OK;
                            })
                            .Get(),
                        nullptr);

                    if (m_webview)
                    {
                        m_viewReadyCallback();
                    }

                    return S_OK;
                }).Get());
            return S_OK;
        }).Get());
}

WebViewWindows::~WebViewWindows()
{
    // close();
}

void WebViewWindows::executeScript(const std::string &script)
{
    if (m_webview)
    {
        std::wstring wideScript(script.begin(), script.end());
        m_webview->ExecuteScript(wideScript.c_str(), nullptr);
    }
    else
    {
        m_pendingScripts.push_back(script);
    }
}

void WebViewWindows::create(int width, int height, const std::string &title)
{
    // m_width = width;
    // m_height = height;

    // // Convert title properly
    // std::wstring wideTitle;
    // if (!title.empty())
    // {
    //     // Use MultiByteToWideChar for proper UTF-8 to UTF-16 conversion
    //     int wideLength = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
    //     if (wideLength > 0)
    //     {
    //         wideTitle.resize(wideLength - 1); // -1 to exclude null terminator
    //         MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wideTitle[0], wideLength);
    //     }
    //     else
    //     {
    //         wideTitle = L"Singularity WebView";
    //     }
    // }
    // else
    // {
    //     wideTitle = L"Singularity WebView";
    // }

    // // Debug output
    // std::string debugMsg = "Creating window with title: '" + title + "'\n";
    // OutputDebugStringA(debugMsg.c_str());

    // // Use a hybrid approach: create with BUTTON class (which supports titles well)
    // // but make it look like a normal window
    // m_hwnd = CreateWindowExW(0,                            // Extended styles
    //                          L"BUTTON",                    // Use BUTTON class (handles titles properly)
    //                          wideTitle.c_str(),            // Window title
    //                          WS_OVERLAPPEDWINDOW,          // Standard window styles
    //                          CW_USEDEFAULT, CW_USEDEFAULT, // Position
    //                          width, height,                // Size
    //                          nullptr,                      // Parent
    //                          nullptr,                      // Menu
    //                          GetModuleHandle(nullptr),     // Instance
    //                          nullptr                       // No lpParam for now
    // );

    // if (m_hwnd)
    // {
    //     OutputDebugStringA("Window created successfully\n");

    //     // Store our object pointer for later retrieval
    //     SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    //     // Subclass the window to use our window procedure
    //     SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);

    //     // Verify the window title
    //     wchar_t actualTitle[256] = {0};
    //     GetWindowTextW(m_hwnd, actualTitle, 256);

    //     char debugTitle[512] = {0};
    //     WideCharToMultiByte(CP_UTF8, 0, actualTitle, -1, debugTitle, 512, nullptr, nullptr);
    //     std::string titleDebug = "Final window title: '" + std::string(debugTitle) + "'\n";
    //     OutputDebugStringA(titleDebug.c_str());

    //     ShowWindow(m_hwnd, SW_SHOW);
    //     UpdateWindow(m_hwnd);
    //     createWebView();
    // }
    // else
    // {
    //     DWORD error = GetLastError();
    //     std::string errorMsg = "Window creation failed, error: " + std::to_string(error) + "\n";
    //     OutputDebugStringA(errorMsg.c_str());
    // }
}

void WebViewWindows::navigate(const std::string &url)
{
    if (m_webview)
    {
        std::wstring wideUrl(url.begin(), url.end());
        m_webview->Navigate(wideUrl.c_str());
    }
}

void WebViewWindows::resize(int width, int height)
{
    // m_width = width;
    // m_height = height;

    // if (m_hwnd)
    // {
    //     SetWindowPos(m_hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    //     setWebViewBounds();
    // }
}

void WebViewWindows::run()
{
    // MSG msg = {};
    // while (GetMessage(&msg, nullptr, 0, 0))
    // {
    //     TranslateMessage(&msg);
    //     DispatchMessage(&msg);
    // }
}

void WebViewWindows::close()
{
    // if (m_webview)
    // {
    //     m_webview = nullptr;
    // }

    // if (m_controller)
    // {
    //     m_controller->Close();
    //     m_controller = nullptr;
    // }

    // if (m_environment)
    // {
    //     m_environment = nullptr;
    // }

    // if (m_hwnd)
    // {
    //     DestroyWindow(m_hwnd);
    //     m_hwnd = nullptr;
    // }

    // m_webviewReady = false;
}
