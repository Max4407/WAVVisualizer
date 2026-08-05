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
    std::string path = "../samples/C418 - Minecraft - Volume Alpha - 06 Moog City.wav";
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::invalid_argument("File not found.");
    
    waveHeader wave;
    file.read(reinterpret_cast<char*>(&wave), sizeof(waveHeader));

    std::vector<std::vector<int16_t>> frames;
    for (uint32_t i = 0; i < wave.subchunk2Size / wave.blockAlign; ++i) {
        std::vector<int16_t> frame;
        for (uint16_t j = 0; j < wave.numChannels; ++j) {
            int16_t sample;
            file.read(reinterpret_cast<char*>(&sample), sizeof(int16_t));
            frame.push_back(sample);
        }
        frames.push_back(frame);
    }

    std::atomic<int> frameCounter(0);
    bool atEnd = false; //has to be external because you cant stop in data_callback
    audioPlayer player(path, &frameCounter, &atEnd);

    sf::RenderWindow window(sf::VideoMode({1250,750}), "visualizer");
    
    sf::VertexArray oscilloscope(sf::PrimitiveType::LineStrip, 101);
    for (int i = 0; i < 101; ++i) {
        oscilloscope[i].position = sf::Vector2f((i * 5) + 125, 375);
        oscilloscope[i].color = sf::Color::Green;
    }

    sf::RectangleShape outline;
    outline.setSize(sf::Vector2f(500.f, 500.f));
    outline.setFillColor(sf::Color::Transparent);
    outline.setOutlineColor(sf::Color(70,70,70));    
    outline.setOutlineThickness(10);   
    outline.setPosition(sf::Vector2f(125, 125));

    sf::RectangleShape progressBarBackground;
    progressBarBackground.setSize(sf::Vector2f(375.f, 8.f));
    progressBarBackground.setFillColor(sf::Color(70,70,70));
    progressBarBackground.setPosition(sf::Vector2f(750, 426));
    progressBarBackground.setOutlineColor(sf::Color(40, 40, 40));
    progressBarBackground.setOutlineThickness(1);

    sf::RectangleShape progressBar;
    progressBar.setSize(sf::Vector2f(0.f, 8.f));
    progressBar.setFillColor(sf::Color::Green);
    progressBar.setPosition(sf::Vector2f(750, 426));
    progressBar.setOutlineColor(sf::Color(40, 40, 40));
    progressBar.setOutlineThickness(1);

    sf::RectangleShape progressBarButton;
    progressBarButton.setSize(sf::Vector2f(12.f, 12.f));
    progressBarButton.setFillColor(sf::Color(100, 100, 100));
    progressBarButton.setPosition(sf::Vector2f(750, 424));
    progressBarButton.setOutlineColor(sf::Color(40, 40, 40));
    progressBarButton.setOutlineThickness(1);

    sf::Texture playButtontexture;
    if (!playButtontexture.loadFromFile("../assets/play_button.png")) {
        throw std::runtime_error("Failed to load play button texture");
    }
    sf::Sprite playButton(playButtontexture);
    playButton.setPosition(sf::Vector2f(912, 450));
    playButton.setScale({
        50 / playButton.getLocalBounds().size.x,
        50 / playButton.getLocalBounds().size.y
    });

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
            if (event->is<sf::Event::MouseButtonPressed>()) {
                auto mouseCoord = window.mapPixelToCoords(
                    sf::Mouse::getPosition(window)
                );
                
                if (playButton.getGlobalBounds().contains(mouseCoord)) {
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

            float progress = static_cast<float>(currentFrame) / frames.size();
            progressBar.setSize(sf::Vector2f(progress * 375, 8));
            progressBarButton.setPosition(sf::Vector2f(750 + progress * 375, 424));
        }
        if (atEnd) {
            player.restart();
            for (int i = 0; i < 101; ++i) 
                oscilloscope[i].position.y = 375;
            isPlaying = false;
            atEnd = false;
        }
        window.clear();
        window.draw(playButton);
        window.draw(outline);
        window.draw(progressBarBackground);
        window.draw(progressBar);
        window.draw(progressBarButton);
        window.draw(oscilloscope);
        window.display();
    }

    return 0;
}