#pragma once

#include "JoshuaCore.hpp"

#include <SFML/Graphics.hpp>

#include <deque>
#include <filesystem>
#include <string>
#include <vector>

namespace joshua {

class Application {
public:
    explicit Application(std::filesystem::path executablePath);
    int run();

private:
    struct DisplayLine {
        std::string text;
        Tone tone{Tone::Normal};
    };

    struct PendingLine {
        OutputLine output;
        std::size_t visibleCharacters{0};
        float characterAccumulator{0.0F};
        float pauseRemaining{0.0F};
        bool complete{false};
    };

    static constexpr unsigned int VirtualWidth = 1200;
    static constexpr unsigned int VirtualHeight = 750;

    void processEvents();
    void processKey(const sf::Event::KeyPressed& event);
    void submitInput();
    void update(float deltaSeconds);
    void enqueue(const CoreResult& result);
    void enqueueLine(OutputLine line);
    void render();
    void renderTerminal();
    void renderSidebar();
    void drawText(sf::RenderTarget& target,
                  const std::string& value,
                  sf::Vector2f position,
                  unsigned int size,
                  sf::Color color) const;
    [[nodiscard]] sf::Color colorFor(Tone tone) const;
    [[nodiscard]] std::vector<std::string> wrap(const std::string& value, std::size_t width) const;
    [[nodiscard]] std::filesystem::path locateAsset(const std::filesystem::path& relative) const;
    void saveScreenshot();

    std::filesystem::path executablePath_;
    sf::RenderWindow window_;
    sf::RenderTexture terminalTexture_;
    sf::Font font_;
    sf::Shader crtShader_;
    bool shaderEnabled_{false};
    float crtIntensity_{0.82F};
    float glitchPulse_{0.0F};
    float elapsedTime_{0.0F};
    float quitTimer_{-1.0F};
    float systemLoad_{0.18F};

    JoshuaCore core_;
    std::deque<DisplayLine> history_;
    std::deque<PendingLine> pending_;
    std::string input_;
    std::vector<std::string> inputHistory_;
    std::size_t historyCursor_{0};
};

} // namespace joshua
