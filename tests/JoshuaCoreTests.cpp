#include "JoshuaCore.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool hasText(const joshua::CoreResult& result, const std::string& text) {
    for (const auto& output : result.lines) {
        if (output.text.find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    joshua::JoshuaCore core;

    require(core.bootSequence().lines.size() >= 8, "boot sequence should be substantial");
    require(core.mode() == joshua::Mode::Terminal, "initial mode should be terminal");

    const auto games = core.submit("  LiSt GaMeS  ");
    require(hasText(games, "TIC-TAC-TOE"), "game list should include tic-tac-toe");
    require(hasText(games, "GLOBAL THERMONUCLEAR WAR"), "game list should include the simulation");

    (void)core.submit("play tic-tac-toe");
    require(core.mode() == joshua::Mode::TicTacToe, "tic-tac-toe should start");
    const auto firstMove = core.submit("1");
    require(hasText(firstMove, "JOSHUA SELECTS"), "AI should answer a legal first move");
    const auto repeatedMove = core.submit("1");
    require(hasText(repeatedMove, "INVALID MOVE"), "occupied cells should be rejected");
    (void)core.submit("quit");
    require(core.mode() == joshua::Mode::Terminal, "quit should leave tic-tac-toe");

    const auto crt = core.submit("crt 150");
    require(crt.crtIntensity && *crt.crtIntensity == 1.0F, "CRT setting should clamp to 100 percent");

    (void)core.submit("play global thermonuclear war");
    require(core.mode() == joshua::Mode::Simulation, "simulation should start");
    auto simulation = core.update(12.0F);
    require(simulation.lines.size() >= 6, "large time steps should emit every elapsed simulation event");
    require(core.defconLevel() == 1, "simulation should reach DEFCON 1");
    simulation = core.update(12.0F);
    require(hasText(simulation, "ONLY WINNING MOVE"), "simulation should reach its conclusion");
    require(core.mode() == joshua::Mode::Terminal, "completed simulation should return to terminal");

    require(hasText(core.submit("nonsense"), "COMMAND NOT RECOGNIZED"),
            "unknown commands should receive useful feedback");

    std::cout << "All Joshua core tests passed.\n";
    return EXIT_SUCCESS;
}
