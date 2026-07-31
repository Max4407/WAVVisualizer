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

    std::vector<std::vector<int16_t>> frames;
    for (uint32_t i = 0; i < synth.subchunk2Size / synth.blockAlign; ++i) {
        std::vector<int16_t> frame;
        for (uint16_t j = 0; j < synth.numChannels; ++j) {
            int16_t sample;
            file.read(reinterpret_cast<char*>(&sample), sizeof(int16_t));
            frame.push_back(sample);
        }
        frames.push_back(frame);
    }

    std::atomic<int> frameCounter(0);
    bool atEnd = false; //has to be external because you cant stop in data_callback
    audioPlayer player(path, &frameCounter, &atEnd);

    sf::RenderWindow window(sf::VideoMode({1500,750}), "visualizer");
    
    sf::VertexArray oscilloscope(sf::PrimitiveType::LineStrip, 101);
    for (int i = 0; i < 101; ++i) {
        oscilloscope[i].position = sf::Vector2f((i * 5) + 125, 375);
        oscilloscope[i].color = sf::Color::Green;
    }

    bool isPlaying = false;
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                player.stopAudio(); 
                isPlaying = false;
                window.close();
            }
            if (auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key -> code == sf::Keyboard::Key::Space) {
                    if (isPlaying) {
                        player.stopAudio();
                        isPlaying = false;
                    } else {
                        player.startAudio();
                        isPlaying = true;
                    }

                }
            }
        }   
        if (isPlaying) {
            int currentFrame = frameCounter.load();
            if (currentFrame < frames.size() - 101) {
                for (int i = 0; i < 101; ++i) {
                    int16_t sample = frames[currentFrame + i][0];
                    oscilloscope[i].position.y = 375 - (sample / 32768.0f) * 250;
                }
            }
        }
        if (atEnd) {
            player.restart();
            isPlaying = false;
            atEnd = false;
        }
        window.clear();
        window.draw(oscilloscope);
        window.display();
    }

    return 0;
}