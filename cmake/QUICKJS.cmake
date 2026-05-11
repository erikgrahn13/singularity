FetchContent_Declare(
    quickjs
    URL https://github.com/quickjs-ng/quickjs/archive/refs/tags/v0.12.1.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(quickjs)

# On Linux, QuickJS is linked into a VST3 .so — must be compiled with -fPIC.
if(UNIX AND NOT APPLE)
    set_target_properties(qjs qjs-libc PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
