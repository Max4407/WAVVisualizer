#include <fstream>
#include <stdexcept>
#include <cstdint>

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
    uint32_t blockAlign;
    uint16_t bitsPerSample;
    
    uint32_t subchunk2ID;
    uint32_t subchunk2Size;
}

int main() {
    std::ifstream file("./samples/synth.wav", std::ios::binary);
    if (!file.is_open())
        throw std::invalid_argument("File not found.");

    waveHeader synth;
    file.read(reinterpret_cast<char*>(&synth), sizeof(waveHeader));

    std::cout << synth << std::endl;

    return 0;
}