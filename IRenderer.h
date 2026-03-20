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
    virtual void clear() = 0;
    virtual void setFillStyle(const std::string& color) = 0;
    virtual void fillRect(float x, float y, float width, float height) = 0;
    virtual void beginPath() = 0;
    virtual void stroke() = 0;
    virtual void arc(float x, float y, float radius, float startAngle, float endAngle) = 0;

    virtual ~IRenderer() = default;
    virtual DrawingContent getDrawingContent() = 0; 
};
