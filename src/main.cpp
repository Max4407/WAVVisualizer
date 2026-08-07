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

    std::string name;
    for (int i = path.size() - 1; i >= 0; --i) {
        if (path[i] == '/' || path[i] == '\\') {
            name = path.substr(i + 1);
            break;
        }
    }
    for (int i = 0; i < name.size(); ++i) {
        if (name[i] == '.') {
            name = name.substr(0, i);
            break;
        }
    }
    
    waveHeader wave;
    file.read(reinterpret_cast<char*>(&wave), sizeof(waveHeader));

    size_t totalSamples = wave.subchunk2Size / sizeof(int16_t);
    std::vector<int16_t> samples(totalSamples);
    file.read(reinterpret_cast<char*>(samples.data()), wave.subchunk2Size);
    int frames = totalSamples / wave.numChannels;

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
    progressBarButton.setPosition(sf::Vector2f(744, 424));
    progressBarButton.setOutlineColor(sf::Color(40, 40, 40));
    progressBarButton.setOutlineThickness(1);
    bool scrubbing = false;

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

    sf::Font font("../assets/VCR_OSD_MONO_1.001.ttf");
    sf::Text titleText(font, name, 18);
    titleText.setFillColor(sf::Color::White);
    sf::FloatRect bounds = titleText.getLocalBounds();
    titleText.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f));
    titleText.setPosition(sf::Vector2f(938, 350));

    bool isPlaying = false;
    while (window.isOpen()) {
        auto mouseCoord = window.mapPixelToCoords(
                sf::Mouse::getPosition(window)
            );

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
                
                if (playButton.getGlobalBounds().contains(mouseCoord)) {
                    if (isPlaying) {
                        player.stopAudio();
                        isPlaying = false;
                    } else {
                        player.startAudio();
                        isPlaying = true;
                    }
                }

                if (progressBarButton.getGlobalBounds().contains(mouseCoord)) {
                    scrubbing = true;
                } else if (progressBarBackground.getGlobalBounds().contains(mouseCoord)) {
                    float progress = (mouseCoord.x - 750) / 375;
                    if (progress < 0) progress = 0;
                    if (progress > 1) progress = 1;
                    int newFrame = static_cast<int>(progress * frames);
                    progressBar.setSize(sf::Vector2f(progress * 375, 8));
                    progressBarButton.setPosition(sf::Vector2f(744 + progress * 375, 424));
                    frameCounter.store(newFrame);
                    player.seekToFrame(newFrame);
                }
            }

            if (event->is<sf::Event::MouseButtonReleased>()) {
                if (scrubbing) {
                    scrubbing = false;
                    float progress = (mouseCoord.x - 750) / 375;
                    if (progress < 0) progress = 0;
                    if (progress > 1) progress = 1;
                    progressBarButton.setPosition(sf::Vector2f(744 + progress * 375, 424));
                    progressBar.setSize(sf::Vector2f(progress * 375, 8));
                    int newFrame = static_cast<int>(progress * frames);
                    frameCounter.store(newFrame);
                    player.seekToFrame(newFrame);
                }
            }

            if (event->is<sf::Event::MouseMoved>()) {
                if (scrubbing) {
                    float progress = (mouseCoord.x - 750) / 375;
                    if (progress < 0) progress = 0;
                    if (progress > 1) progress = 1;
                    progressBarButton.setPosition(sf::Vector2f(744 + progress * 375, 424));
                    progressBar.setSize(sf::Vector2f(progress * 375, 8));
                }
            }
        }   

        if (isPlaying) {
            int currentFrame = frameCounter.load();
            if (currentFrame < frames - 101) {
                for (int i = 0; i < 101; ++i) {
                    int16_t sample = samples[currentFrame + i * wave.numChannels];
                    oscilloscope[i].position.y = 375 - (sample / 32768.0f) * 250;
                }
            }
            if (!scrubbing) {
                float progress = static_cast<float>(currentFrame) / (frames);
                progressBar.setSize(sf::Vector2f(progress * 375, 8));
                progressBarButton.setPosition(sf::Vector2f(744 + progress * 375, 424));
            }
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
        window.draw(titleText);
        window.draw(oscilloscope);
        window.display();
    }

    return 0;
}