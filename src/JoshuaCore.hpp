#pragma once

#include <array>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace joshua {

enum class Tone {
    Normal,
    Bright,
    Dim,
    Alert,
};

enum class Mode {
    Terminal,
    TicTacToe,
    Simulation,
};

struct OutputLine {
    std::string text;
    Tone tone{Tone::Normal};
    float charactersPerSecond{48.0F};
    float pauseAfter{0.06F};
};

struct CoreResult {
    std::vector<OutputLine> lines;
    bool clearScreen{false};
    bool requestQuit{false};
    float glitchPulse{0.0F};
    std::optional<float> crtIntensity;
};

class JoshuaCore {
public:
    JoshuaCore();

    [[nodiscard]] CoreResult bootSequence() const;
    [[nodiscard]] CoreResult submit(std::string input);
    [[nodiscard]] CoreResult update(float deltaSeconds);
    [[nodiscard]] Mode mode() const noexcept;
    [[nodiscard]] std::string modeLabel() const;
    [[nodiscard]] int defconLevel() const noexcept;

private:
    [[nodiscard]] CoreResult handleTerminal(const std::string& input);
    [[nodiscard]] CoreResult handleTicTacToe(const std::string& input);
    [[nodiscard]] CoreResult handleSimulation(const std::string& input);

    void beginTicTacToe(CoreResult& result);
    void beginSimulation(CoreResult& result);
    [[nodiscard]] std::vector<OutputLine> boardLines() const;
    [[nodiscard]] int chooseAiMove();
    [[nodiscard]] int minimax(std::array<char, 9>& board, bool aiTurn, int depth) const;
    [[nodiscard]] static char winner(const std::array<char, 9>& board);
    [[nodiscard]] static bool boardFull(const std::array<char, 9>& board);
    [[nodiscard]] static std::string normalize(std::string value);

    Mode mode_{Mode::Terminal};
    std::array<char, 9> board_{};
    bool gameOver_{false};
    float simulationTime_{0.0F};
    std::size_t simulationStep_{0};
    int defcon_{5};
    std::mt19937 random_;
};

} // namespace joshua
