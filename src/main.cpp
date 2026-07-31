#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <iostream>
#include <vector>
#include <atomic>
#include <string>

#include "SFML/Graphics.hpp"
#include "./audioPlayer.h"

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
    std::string path = "../samples/synth.wav";
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::invalid_argument("File not found.");
    
    waveHeader synth;
    file.read(reinterpret_cast<char*>(&synth), sizeof(waveHeader));

    std::vector<std::vector<std::vector<int16_t>>> frames;
    for (uint32_t i = 0; i < synth.subchunk2Size / synth.blockAlign; ++i) {
        std::vector<std::vector<int16_t>> frame;
        for (uint16_t j = 0; j < synth.numChannels; ++j) {
            std::vector<int16_t> channel;
            int16_t sample;
            file.read(reinterpret_cast<char*>(&sample), sizeof(int16_t));
            channel.push_back(sample);
            frame.push_back(channel);
        }
        frames.push_back(frame);
    }

    std::atomic<int> frameCounter(0);
    audioPlayer player(path, &frameCounter);
    sf::RenderWindow window(sf::VideoMode({1000,1000}), "Minesweeper");

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (event->is<sf::Event::KeyPressed>()) {
                player.startAudio();
            }
        }   
        window.clear();
        window.display();
    }

    return 0;
}