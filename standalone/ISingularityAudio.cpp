#include "ISingularityAudio.h"

#if defined(__APPLE__)
    #include "coreAudio.h"
#endif

enum class AudioBackendType { CoreAudio, WASAPI, ASIO };

std::unique_ptr<ISingularityAudio> ISingularityAudio::createSingularityAudio() {
#if defined(__APPLE__)
    return std::make_unique<CoreAudio>();
#endif
}