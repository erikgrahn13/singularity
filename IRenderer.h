#pragma once

#if __has_include(<swift/bridging>)
#  include <swift/bridging>
#else
#  define SWIFT_SELF_CONTAINED
#  define SWIFT_RETURNS_INDEPENDENT_VALUE
#endif

#include <memory>

    struct DrawingContent {
        const void* contentAddres{nullptr};
        size_t contentBytes{0};
        int width{0};
        int height{0};
    };

class IRenderer {
    public:

    virtual ~IRenderer() = default;
    virtual DrawingContent getDrawingContent() = 0; 
};
