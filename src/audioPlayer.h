#include <atomic>
#include <string>
#include <iostream>
#include "../miniaudio.h"

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

class audioPlayer {
private:
    std::atomic<int>* frameCounter;
    std::string path;
    ma_decoder decoder;
    ma_device_config deviceConfig;
    ma_device device;
    bool* atEnd;
public:
    audioPlayer(std::string path_, std::atomic<int>* frameCounter_, bool* atEnd_);
    int startAudio();
    int stopAudio();
    int restart();
    int seekToFrame(int frame);
    ~audioPlayer();
};