#include <atomic>
#include <string>
#include "../miniaudio.h"

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
int startAudio(std::atomic<int>* frameCounter, std::string path);
