#include "audioPlayer.h"

struct context {
    ma_decoder* decoder;
    std::atomic<int>* frameCounter;
    bool* atEnd;
    context(ma_decoder* decoder, std::atomic<int>* frameCounter, bool* atEnd) : decoder(decoder), frameCounter(frameCounter), atEnd(atEnd) {}
};

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    context* c = (context*)pDevice -> pUserData;
    ma_decoder* pDecoder =  c -> decoder;
    if (pDecoder == NULL) 
        return;

    c -> frameCounter -> fetch_add(frameCount);
    ma_uint64 framesRead;

    ma_result result = ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &framesRead);
    if (framesRead < frameCount) {
        *(c -> atEnd) = true;
    }
    (void)pInput;
}

audioPlayer::audioPlayer(std::string path_, std::atomic<int>* frameCounter_, bool* atEnd_) : path(path_), frameCounter(frameCounter_), atEnd(atEnd_) {
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
    deviceConfig.pUserData         = new context(&decoder, frameCounter, atEnd);

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
    return 0;
}

int audioPlayer::stopAudio() {
    if (ma_device_stop(&device) != MA_SUCCESS) {
        printf("Failed to stop playback device.\n");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        return -1;
    }
    return 0;
}

int audioPlayer::restart() {
    stopAudio();
    ma_decoder_seek_to_pcm_frame(&decoder, 0);
    frameCounter -> store(0);
    return 0;
}

audioPlayer::~audioPlayer() {
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
}