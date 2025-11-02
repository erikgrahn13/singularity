set(target_name ${target}_App)

add_library(SingularityAudio STATIC
    AudioManager.h
)

if(WIN32)
    add_subdirectory(windows)
    target_sources(SingularityAudio PRIVATE
        windows/audio/AudioManager_Win.cpp
    )
    target_link_libraries(SingularityAudio PRIVATE SingularityASIO)
    target_include_directories(SingularityAudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
elseif(APPLE)
    add_subdirectory(macos)

    # this is TMP until i make a real soluton and include cpp files
    set_target_properties(SingularityAudio PROPERTIES
        LINKER_LANGUAGE CXX
    )
elseif(UNIX)
    add_subdirectory(linux)

    # this is TMP until i make a real soluton and include cpp files
    set_target_properties(SingularityAudio PROPERTIES
        LINKER_LANGUAGE CXX
    )
endif()
