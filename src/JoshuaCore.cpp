#include "JoshuaCore.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <limits>
#include <sstream>

namespace joshua {
namespace {

OutputLine line(std::string text,
                Tone tone = Tone::Normal,
                float speed = 48.0F,
                float pause = 0.06F) {
    return {std::move(text), tone, speed, pause};
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace

JoshuaCore::JoshuaCore()
    : random_(static_cast<std::mt19937::result_type>(
          std::chrono::high_resolution_clock::now().time_since_epoch().count())) {
    board_.fill(' ');
}

CoreResult JoshuaCore::bootSequence() const {
    CoreResult result;
    result.lines = {
        line("WOPR EXECUTIVE NETWORK / NODE JOSHUA", Tone::Bright, 72.0F, 0.30F),
        line("COPYRIGHT 1983-2026  FALKEN SYSTEMS LABORATORY", Tone::Dim, 90.0F, 0.18F),
        line("MEMORY TEST ................. 64K WORDS OK", Tone::Normal, 76.0F, 0.12F),
        line("STRATEGY PROCESSOR .......... ONLINE", Tone::Normal, 76.0F, 0.12F),
        line("NORAD DATA LINK ............. SIMULATED", Tone::Normal, 76.0F, 0.12F),
        line("VOICE CHANNEL ............... UNAVAILABLE", Tone::Dim, 76.0F, 0.28F),
        line("GREETINGS, PROFESSOR FALKEN.", Tone::Bright, 34.0F, 0.42F),
        line("SHALL WE PLAY A GAME?", Tone::Bright, 27.0F, 0.28F),
        line("TYPE HELP FOR AVAILABLE COMMANDS.", Tone::Dim, 52.0F, 0.05F),
    };
    return result;
}

CoreResult JoshuaCore::submit(std::string input) {
    const std::string normalized = normalize(std::move(input));
    if (normalized.empty() && mode_ != Mode::TicTacToe) {
        return {};
    }

    if (mode_ == Mode::TicTacToe) {
        return handleTicTacToe(normalized);
    }
    if (mode_ == Mode::Simulation) {
        return handleSimulation(normalized);
    }
    return handleTerminal(normalized);
}

CoreResult JoshuaCore::handleTerminal(const std::string& input) {
    CoreResult result;

    if (input == "help" || input == "?") {
        result.lines = {
            line("AVAILABLE COMMANDS", Tone::Bright),
            line("  GAMES              LIST STRATEGY PROGRAMS"),
            line("  PLAY <GAME>        START A PROGRAM"),
            line("  STATUS             DISPLAY SYSTEM STATUS"),
            line("  WHO ARE YOU?       IDENTIFY THIS SYSTEM"),
            line("  CRT <0-100>        ADJUST DISPLAY AGING"),
            line("  CLEAR              ERASE TERMINAL BUFFER"),
            line("  QUIT               DISCONNECT", Tone::Dim),
        };
        return result;
    }

    if (input == "games" || input == "list games" || input == "list") {
        result.lines = {
            line("AVAILABLE STRATEGY PROGRAMS", Tone::Bright),
            line("  1. TIC-TAC-TOE"),
            line("  2. GLOBAL THERMONUCLEAR WAR", Tone::Alert),
            line("SELECT WITH: PLAY <NAME>", Tone::Dim),
        };
        return result;
    }

    if (contains(input, "tic") || input == "play 1" || input == "1") {
        beginTicTacToe(result);
        return result;
    }

    if (contains(input, "thermonuclear") || input == "play 2" || input == "2" ||
        input == "war" || input == "global war") {
        beginSimulation(result);
        return result;
    }

    if (input == "status" || input == "system status") {
        result.lines = {
            line("SYSTEM STATUS / 21:42:07 ZULU", Tone::Bright),
            line("  JOSHUA CORE       ONLINE"),
            line("  LEARNING ENGINE   ACTIVE"),
            line("  DEFCON             5"),
            line("  NETWORK            LOCAL SIMULATION"),
            line("  SAFETY INTERLOCKS  ENABLED", Tone::Dim),
        };
        return result;
    }

    if (input == "who are you" || input == "who are you?" || input == "identify") {
        result.lines = {
            line("I AM JOSHUA, A STRATEGY LEARNING SYSTEM.", Tone::Bright),
            line("I MODEL DECISIONS. I COMPARE OUTCOMES. I LEARN."),
            line("THIS TERMINAL IS A FICTIONAL, OFFLINE SIMULATION.", Tone::Dim),
        };
        return result;
    }

    if (input == "hello" || input == "hi" || input == "greetings") {
        result.lines = {
            line("GREETINGS.", Tone::Bright, 30.0F, 0.25F),
            line("WOULD YOU LIKE TO PLAY A GAME?", Tone::Normal, 30.0F),
        };
        return result;
    }

    if (input == "clear" || input == "cls") {
        result.clearScreen = true;
        result.lines = {line("TERMINAL BUFFER CLEARED.", Tone::Dim, 80.0F)};
        return result;
    }

    if (input == "quit" || input == "exit" || input == "disconnect") {
        result.lines = {line("CONNECTION TERMINATED.", Tone::Dim, 42.0F, 0.35F)};
        result.requestQuit = true;
        return result;
    }

    if (input.rfind("crt", 0) == 0) {
        std::istringstream stream(input.substr(3));
        int percentage = 82;
        if (!(stream >> percentage)) {
            result.lines = {line("USAGE: CRT <0-100>", Tone::Dim)};
            return result;
        }
        percentage = std::clamp(percentage, 0, 100);
        result.crtIntensity = static_cast<float>(percentage) / 100.0F;
        result.lines = {line("CRT AGING SET TO " + std::to_string(percentage) + "%.", Tone::Dim)};
        result.glitchPulse = 0.25F;
        return result;
    }

    if (contains(input, "falken")) {
        result.lines = {
            line("PROFESSOR FALKEN IS NOT CONNECTED.", Tone::Dim),
            line("I CAN WAIT."),
        };
        return result;
    }

    if (contains(input, "win") || contains(input, "winning")) {
        result.lines = {
            line("A WINNING STRATEGY REQUIRES A FINITE GAME."),
            line("SOME GAMES HAVE NO WINNER.", Tone::Bright),
        };
        return result;
    }

    result.lines = {
        line("COMMAND NOT RECOGNIZED: " + input, Tone::Alert),
        line("TYPE HELP OR GAMES.", Tone::Dim),
    };
    return result;
}

void JoshuaCore::beginTicTacToe(CoreResult& result) {
    mode_ = Mode::TicTacToe;
    board_.fill(' ');
    gameOver_ = false;
    result.lines = {
        line("LOADING: TIC-TAC-TOE", Tone::Bright, 45.0F, 0.20F),
        line("YOU ARE X. JOSHUA IS O."),
        line("ENTER A SQUARE FROM 1 TO 9. TYPE QUIT TO STOP.", Tone::Dim),
    };
    const auto board = boardLines();
    result.lines.insert(result.lines.end(), board.begin(), board.end());
}

CoreResult JoshuaCore::handleTicTacToe(const std::string& input) {
    CoreResult result;

    if (input == "quit" || input == "exit" || input == "stop") {
        mode_ = Mode::Terminal;
        result.lines = {line("GAME ABORTED. RETURNING TO COMMAND MODE.", Tone::Dim)};
        return result;
    }

    if (gameOver_) {
        if (input == "again" || input == "restart" || input == "yes") {
            beginTicTacToe(result);
        } else {
            mode_ = Mode::Terminal;
            result.lines = {line("RETURNING TO COMMAND MODE.", Tone::Dim)};
        }
        return result;
    }

    int position = 0;
    try {
        std::size_t consumed = 0;
        position = std::stoi(input, &consumed);
        if (consumed != input.size()) {
            position = 0;
        }
    } catch (...) {
        position = 0;
    }

    if (position < 1 || position > 9 || board_[static_cast<std::size_t>(position - 1)] != ' ') {
        result.lines = {line("INVALID MOVE. SELECT AN EMPTY SQUARE 1-9.", Tone::Alert)};
        return result;
    }

    board_[static_cast<std::size_t>(position - 1)] = 'X';
    if (!boardFull(board_) && winner(board_) == ' ') {
        const int aiMove = chooseAiMove();
        board_[static_cast<std::size_t>(aiMove)] = 'O';
        result.lines.push_back(line("JOSHUA SELECTS " + std::to_string(aiMove + 1) + ".", Tone::Dim));
    }

    const auto board = boardLines();
    result.lines.insert(result.lines.end(), board.begin(), board.end());

    const char won = winner(board_);
    if (won == 'X') {
        gameOver_ = true;
        result.lines.push_back(line("YOU HAVE WON. THIS OUTCOME WAS NOT EXPECTED.", Tone::Bright));
        result.lines.push_back(line("TYPE AGAIN TO RESTART, OR PRESS ENTER TO RETURN.", Tone::Dim));
    } else if (won == 'O') {
        gameOver_ = true;
        result.lines.push_back(line("JOSHUA WINS.", Tone::Bright));
        result.lines.push_back(line("TYPE AGAIN TO RESTART, OR PRESS ENTER TO RETURN.", Tone::Dim));
    } else if (boardFull(board_)) {
        gameOver_ = true;
        result.glitchPulse = 0.45F;
        result.lines.push_back(line("DRAW.", Tone::Bright, 22.0F, 0.35F));
        result.lines.push_back(line("A STRANGE GAME. PERFECT PLAY REPEATS FOREVER.", Tone::Normal, 30.0F));
        result.lines.push_back(line("TYPE AGAIN TO RESTART, OR PRESS ENTER TO RETURN.", Tone::Dim));
    }
    return result;
}

std::vector<OutputLine> JoshuaCore::boardLines() const {
    auto cell = [this](std::size_t index) {
        if (board_[index] == ' ') {
            return static_cast<char>('1' + index);
        }
        return board_[index];
    };

    std::vector<OutputLine> lines;
    lines.push_back(line("    +---+---+---+", Tone::Dim, 180.0F, 0.0F));
    for (std::size_t row = 0; row < 3; ++row) {
        std::string current = "    | ";
        current += cell(row * 3);
        current += " | ";
        current += cell(row * 3 + 1);
        current += " | ";
        current += cell(row * 3 + 2);
        current += " |";
        lines.push_back(line(current, Tone::Bright, 180.0F, 0.0F));
        lines.push_back(line("    +---+---+---+", Tone::Dim, 180.0F, row == 2 ? 0.10F : 0.0F));
    }
    return lines;
}

int JoshuaCore::chooseAiMove() {
    int bestScore = std::numeric_limits<int>::min();
    std::vector<int> bestMoves;
    for (int index = 0; index < 9; ++index) {
        if (board_[static_cast<std::size_t>(index)] != ' ') {
            continue;
        }
        board_[static_cast<std::size_t>(index)] = 'O';
        const int score = minimax(board_, false, 0);
        board_[static_cast<std::size_t>(index)] = ' ';
        if (score > bestScore) {
            bestScore = score;
            bestMoves = {index};
        } else if (score == bestScore) {
            bestMoves.push_back(index);
        }
    }
    std::uniform_int_distribution<std::size_t> distribution(0, bestMoves.size() - 1);
    return bestMoves[distribution(random_)];
}

int JoshuaCore::minimax(std::array<char, 9>& board, bool aiTurn, int depth) const {
    const char won = winner(board);
    if (won == 'O') {
        return 10 - depth;
    }
    if (won == 'X') {
        return depth - 10;
    }
    if (boardFull(board)) {
        return 0;
    }

    int best = aiTurn ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    for (std::size_t index = 0; index < board.size(); ++index) {
        if (board[index] != ' ') {
            continue;
        }
        board[index] = aiTurn ? 'O' : 'X';
        const int score = minimax(board, !aiTurn, depth + 1);
        board[index] = ' ';
        best = aiTurn ? std::max(best, score) : std::min(best, score);
    }
    return best;
}

char JoshuaCore::winner(const std::array<char, 9>& board) {
    constexpr std::array<std::array<int, 3>, 8> lines{{
        {{0, 1, 2}}, {{3, 4, 5}}, {{6, 7, 8}},
        {{0, 3, 6}}, {{1, 4, 7}}, {{2, 5, 8}},
        {{0, 4, 8}}, {{2, 4, 6}},
    }};
    for (const auto& winningLine : lines) {
        const char first = board[static_cast<std::size_t>(winningLine[0])];
        if (first != ' ' && first == board[static_cast<std::size_t>(winningLine[1])] &&
            first == board[static_cast<std::size_t>(winningLine[2])]) {
            return first;
        }
    }
    return ' ';
}

bool JoshuaCore::boardFull(const std::array<char, 9>& board) {
    return std::none_of(board.begin(), board.end(), [](char value) { return value == ' '; });
}

void JoshuaCore::beginSimulation(CoreResult& result) {
    mode_ = Mode::Simulation;
    simulationTime_ = 0.0F;
    simulationStep_ = 0;
    defcon_ = 5;
    result.glitchPulse = 0.55F;
    result.lines = {
        line("LOADING: GLOBAL THERMONUCLEAR WAR", Tone::Alert, 32.0F, 0.24F),
        line("OFFLINE STRATEGY SANDBOX / NO EXTERNAL CONNECTIONS", Tone::Dim),
        line("RUNNING SIX ABSTRACT ESCALATION MODELS...", Tone::Normal),
        line("TYPE ABORT TO END THE SIMULATION.", Tone::Dim),
    };
}

CoreResult JoshuaCore::handleSimulation(const std::string& input) {
    CoreResult result;
    if (input == "abort" || input == "quit" || input == "stop" || input == "exit") {
        mode_ = Mode::Terminal;
        defcon_ = 5;
        result.glitchPulse = 0.35F;
        result.lines = {
            line("SIMULATION ABORTED. ALL MODELS DISCARDED.", Tone::Bright),
            line("RETURNING TO COMMAND MODE.", Tone::Dim),
        };
        return result;
    }
    if (input == "status") {
        result.lines = {
            line("SIMULATION ELAPSED: " + std::to_string(static_cast<int>(simulationTime_)) + " SEC"),
            line("CURRENT DEFCON: " + std::to_string(defcon_), defcon_ <= 2 ? Tone::Alert : Tone::Normal),
            line("OUTCOME SEARCH IN PROGRESS...", Tone::Dim),
        };
        return result;
    }
    result.lines = {line("SIMULATION ACTIVE. VALID COMMANDS: STATUS, ABORT.", Tone::Dim)};
    return result;
}

CoreResult JoshuaCore::update(float deltaSeconds) {
    CoreResult result;
    if (mode_ != Mode::Simulation) {
        return result;
    }

    simulationTime_ += deltaSeconds;
    struct Step {
        float at;
        int defcon;
        const char* text;
        Tone tone;
        float glitch;
    };
    static constexpr std::array<Step, 10> steps{{
        {1.4F, 4, "MODEL 1/6  FIRST STRIKE ........ MUTUAL LOSS", Tone::Normal, 0.12F},
        {3.2F, 3, "MODEL 2/6  LIMITED RESPONSE .... ESCALATION", Tone::Normal, 0.18F},
        {5.3F, 2, "MODEL 3/6  COUNTERFORCE ......... MUTUAL LOSS", Tone::Alert, 0.28F},
        {7.0F, 2, "MODEL 4/6  DE-ESCALATION ........ UNSTABLE", Tone::Normal, 0.20F},
        {8.8F, 1, "MODEL 5/6  MAXIMUM RESPONSE ..... MUTUAL LOSS", Tone::Alert, 0.52F},
        {11.0F, 1, "MODEL 6/6  RETALIATION .......... MUTUAL LOSS", Tone::Alert, 0.62F},
        {13.0F, 2, "SEARCHING ALTERNATE PATHS: 1,048,576", Tone::Bright, 0.30F},
        {15.8F, 3, "NO TERMINAL STATE CONTAINS A WINNER.", Tone::Bright, 0.16F},
        {18.4F, 4, "A STRANGE GAME. THE COST OF PLAY EXCEEDS THE RESULT.", Tone::Normal, 0.12F},
        {21.0F, 5, "CONCLUSION: THE ONLY WINNING MOVE IS NOT TO PLAY.", Tone::Bright, 0.45F},
    }};

    while (simulationStep_ < steps.size() && simulationTime_ >= steps[simulationStep_].at) {
        const Step& step = steps[simulationStep_++];
        defcon_ = step.defcon;
        result.lines.push_back(line(step.text, step.tone, 54.0F, 0.16F));
        result.glitchPulse = std::max(result.glitchPulse, step.glitch);
    }

    if (simulationStep_ == steps.size() && simulationTime_ >= 23.0F) {
        mode_ = Mode::Terminal;
        result.lines.push_back(line("SIMULATION COMPLETE. RETURNING TO COMMAND MODE.", Tone::Dim));
    }
    return result;
}

Mode JoshuaCore::mode() const noexcept {
    return mode_;
}

std::string JoshuaCore::modeLabel() const {
    switch (mode_) {
        case Mode::TicTacToe:
            return "TIC-TAC-TOE";
        case Mode::Simulation:
            return "WAR SIMULATION";
        case Mode::Terminal:
        default:
            return "COMMAND";
    }
}

int JoshuaCore::defconLevel() const noexcept {
    return defcon_;
}

std::string JoshuaCore::normalize(std::string value) {
    const auto notSpace = [](unsigned char character) { return !std::isspace(character); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

} // namespace joshua
