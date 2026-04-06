#include "ISingularityAudio.h"

#if defined(__APPLE__)
    #include "coreAudio.h"
#elif defined(_WIN32)
    #include "ASIO.h"
#endif

// std::vector<std::string> ISingularityAudio::backends;
std::vector<std::string> ISingularityAudio::backends;

std::unique_ptr<ISingularityAudio> ISingularityAudio::createSingularityAudio()
{
#if defined(__APPLE__)
    return std::make_unique<CoreAudio>();
#elif defined(_WIN32)
    return std::make_unique<ASIO>();
#endif
}