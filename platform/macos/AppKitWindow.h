#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Takes an NSView* (as void*), makes it layer-backed with a CAMetalLayer,
/// and returns the CAMetalLayer* (as void*) suitable for Dawn surface creation.
void* createMetalLayerForView(void* nsView);

#ifdef __cplusplus
}
#endif
