add_library(SingularityView STATIC)

if(WIN32)
    target_sources(SingularityView PRIVATE platform/windows/singularityGUI_Windows.cpp)
    target_link_libraries(SingularityView PRIVATE "${webview2_SOURCE_DIR}/build/native/x64/WebView2LoaderStatic.lib" shlwapi)
    target_include_directories(SingularityView PRIVATE "${wil_SOURCE_DIR}/include" "${webview2_SOURCE_DIR}/build/native/include" "${miniz_SOURCE_DIR}")
elseif(APPLE)
    target_sources(SingularityView PRIVATE platform/macos/WebViewMacos.cpp)
endif()