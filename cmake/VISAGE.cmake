FetchContent_Declare(
    visage
    GIT_REPOSITORY https://github.com/VitalAudio/visage.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(visage)

# On Linux, visage is linked into a VST3 .so — all targets must be compiled with -fPIC.
if(UNIX AND NOT APPLE)
    foreach(_visage_target IN ITEMS visage VisageApp VisageWindowing VisageGraphics VisageWidgets VisageUi VisageUtils)
        if(TARGET ${_visage_target})
            set_target_properties(${_visage_target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        endif()
    endforeach()
endif()
