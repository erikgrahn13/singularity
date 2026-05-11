add_library(SingularityView STATIC)

if(WIN32)
    target_sources(SingularityView PRIVATE platform/windows/singularityGUI_Windows.cpp)
    target_link_libraries(SingularityView PRIVATE "${webview2_SOURCE_DIR}/build/native/x64/WebView2LoaderStatic.lib" shlwapi)
    target_include_directories(SingularityView PRIVATE "${wil_SOURCE_DIR}/include" "${webview2_SOURCE_DIR}/build/native/include" "${miniz_SOURCE_DIR}")
elseif(APPLE)
    target_sources(SingularityView PRIVATE
        platform/macos/WebViewMacos.swift
        platform/macos/WebViewMacosAdapter.cpp
        ../plugins/VST3/platform/macos/VST3WindowMacos.swift)

    set_target_properties(SingularityView PROPERTIES
        Swift_LANGUAGE_VERSION 6
        Swift_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/SingularityView"
    )
    target_compile_options(SingularityView PUBLIC
        "$<$<COMPILE_LANGUAGE:Swift>:-cxx-interoperability-mode=default>")

    _swift_generate_cxx_header(SingularityView
        SingularityView/SingularityView-swift.h)
elseif(UNIX)
    # find_package(PkgConfig REQUIRED)
    # pkg_check_modules(WEBKIT REQUIRED webkitgtk-6.0)
    target_sources(SingularityView PRIVATE 
        platform/linux/singularity_Webview_Linux.cpp)
    target_include_directories(SingularityView PUBLIC ${WEBKIT_INCLUDE_DIRS})
    target_link_libraries(SingularityView PUBLIC ${PLATFORM_LIBS})
    set_target_properties(SingularityView PROPERTIES
        LINKER_LANGUAGE CXX
    )
    set_target_properties(SingularityView PROPERTIES POSITION_INDEPENDENT_CODE ON)
    
endif()