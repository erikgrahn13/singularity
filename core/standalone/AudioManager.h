#include "ISingularityAudioBackend.h"
#include <memory>
#include <vector>

class AudioManager
{
  public:
    AudioManager();
    ~AudioManager() = default;

    const std::vector<AudioDeviceInfo> &getDeviceList() const;

  private:
    void probeDevices();
    std::vector<std::unique_ptr<ISingularityAudioBackend>> backends;
    std::vector<AudioDeviceInfo> allDevices;
};