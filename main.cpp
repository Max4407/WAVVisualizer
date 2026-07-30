#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <iostream>

struct waveHeader {
    char chunkDescriptor[4];
    uint32_t chunkSize;
    char format[4];

    uint32_t subchunk1ID;
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    
    uint32_t subchunk2ID;
    uint32_t subchunk2Size;
};

int main() {
    std::ifstream file("./samples/synth.wav", std::ios::binary);
    if (!file.is_open())
        throw std::invalid_argument("File not found.");

    waveHeader synth;
    file.read(reinterpret_cast<char*>(&synth), sizeof(waveHeader));

    std::cout << "Chunk Size: " << synth.chunkSize << std::endl <<
                 "Audio Format: " << synth.audioFormat << std::endl <<
                 "Format: " << synth.format << std::endl <<
                 "numChannels: " << synth.numChannels << std::endl << 
                 "subchunk1Size: " << synth.subchunk1Size << std::endl <<
                 "subchunk2Size: " << synth.subchunk2Size << std::endl <<
                 "sampleRate: " << synth.sampleRate << std::endl <<
                 "bitsPerSample: " << synth.bitsPerSample << std::endl;

    int16_t* data = new int16_t[synth.subchunk2Size / sizeof(int16_t)];
    file.read(reinterpret_cast<char*>(data), synth.subchunk2Size);
    for (uint32_t i = 0; i < synth.subchunk2Size / (int) sizeof(int16_t); i++) {
        std::cout << data[i] << std::endl;
    }
    delete[] data;
    data = nullptr;
    return 0;
}