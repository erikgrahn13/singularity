// #include "Editor.h"
#include "../SingularityEditor.h"
#include "../gui/singularity_ResourceManager.h"
#include "../gui/singularity_Webview.h"
#include <iostream>

int main()
{
    try
    {
        std::unique_ptr<SingularityEditor> editor = createEditorInstance();
        // std::unique_ptr<ISingularityGUI> webview = ISingularityGUI::createView();

        // Create window
        // webview->create(800, 600, "Native WebView - React App");

        // Choose URL based on build mode
#if !defined NDEBUG
        std::cout << "Running in development mode - connecting to React dev server\n";
        // webview->navigate("http://localhost:5173/");
#else
        std::cout << "Running in production mode - loading from embedded resources\n";

        // Debug: Check if any resources are registered
        auto testResource = ResourceManager::getResource("/index.html");
        if (testResource)
        {
            std::cout << "Found index.html resource, size: " << testResource->size << " bytes\n";
        }
        else
        {
            std::cout << "ERROR: index.html resource not found!\n";
            return 1;
        }

        // Load from custom app:// scheme (handled by WKURLSchemeHandler)
        webview->navigate("app://localhost/");
#endif

        // Start the application
        // webview->run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
