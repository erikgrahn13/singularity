set(target_name ${target}_App)


if(WIN32)
    # TMP solution, audiomanager should be used by all platforms later
    add_library(SingularityAudio STATIC)

    add_subdirectory(windows)
    target_sources(SingularityAudio PRIVATE
        windows/audio/AudioManager_Win.cpp
    )
    target_link_libraries(SingularityAudio PRIVATE SingularityASIO)
    target_include_directories(SingularityAudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
elseif(APPLE)
    add_subdirectory(macos)
elseif(UNIX)
    add_subdirectory(linux)
endif()
