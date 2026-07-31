#include "audioPlayer.h"

struct context {
    ma_decoder* decoder;
    std::atomic<int>* frameCounter;
    context(ma_decoder* decoder, std::atomic<int>* frameCounter) : decoder(decoder), frameCounter(frameCounter) {}
};

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    context* c = (context*)pDevice -> pUserData;
    ma_decoder* pDecoder =  c -> decoder;
    if (pDecoder == NULL) 
        return;

    c -> frameCounter -> fetch_add(frameCount);
    std::cout << "Frames processed: " << c -> frameCounter -> load() << std::endl;
    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);

    (void)pInput;
}

audioPlayer::audioPlayer(std::string path_, std::atomic<int>* frameCounter_) : path(path_), frameCounter(frameCounter_) {
    ma_result result;

    result = ma_decoder_init_file(path.c_str(), NULL, &decoder);
    if (result != MA_SUCCESS) {
        printf("Could not load file: %s\n", path.c_str());
        return;
    }

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate        = decoder.outputSampleRate;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = new context(&decoder, frameCounter);

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        ma_decoder_uninit(&decoder);
        return;
    }
}

int audioPlayer::startAudio() {
    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        return -1;
    }

    printf("Press Enter to quit...");
    getchar();

    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);

    return 0;
}