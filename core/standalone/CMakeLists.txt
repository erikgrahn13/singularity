message("erik13: ${WebView2_INCLUDE_DIR}")

set(target_name ${target}_App)
add_executable(${target_name}
    main.cpp

    # ../gui/singularity_ResourceManager.h
    # ../gui/singularity_ResourceManager.cpp
    # ../gui/singularity_Webview.h
    # ../gui/singularity_Webview.cpp
)

target_link_libraries(${target_name} ${PLATFORM_LIBS} ${target})

# target_include_directories(${target_name} PRIVATE ${WebView2_INCLUDE_DIR})
