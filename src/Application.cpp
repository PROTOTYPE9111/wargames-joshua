#include "Application.hpp"

#include <SFML/Window/Clipboard.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace joshua {
namespace {

constexpr sf::Color Phosphor{96, 255, 145};
constexpr sf::Color PhosphorBright{181, 255, 202};
constexpr sf::Color PhosphorDim{44, 135, 77};
constexpr sf::Color Amber{255, 183, 78};
constexpr sf::Color BlackGreen{1, 9, 4};

} // namespace

Application::Application(std::filesystem::path executablePath)
    : executablePath_(std::filesystem::absolute(std::move(executablePath))),
      window_(sf::VideoMode({1280, 800}), "JOSHUA // WOPR STRATEGY TERMINAL", sf::Style::Default),
      terminalTexture_({VirtualWidth, VirtualHeight}) {
    window_.setVerticalSyncEnabled(true);
    window_.setKeyRepeatEnabled(true);
    terminalTexture_.setSmooth(true);

    const auto fontPath = locateAsset("fonts/VT323-Regular.ttf");
    if (!font_.openFromFile(fontPath)) {
        throw std::runtime_error("Unable to load terminal font: " + fontPath.string());
    }

    if (sf::Shader::isAvailable()) {
        const auto shaderPath = locateAsset("shaders/crt.frag");
        shaderEnabled_ = crtShader_.loadFromFile(shaderPath, sf::Shader::Type::Fragment);
        if (shaderEnabled_) {
            crtShader_.setUniform("u_texture", sf::Shader::CurrentTexture);
            crtShader_.setUniform("u_sourceResolution",
                                  sf::Vector2f(static_cast<float>(VirtualWidth),
                                               static_cast<float>(VirtualHeight)));
        }
    }

    enqueue(core_.bootSequence());
}

int Application::run() {
    sf::Clock frameClock;
    while (window_.isOpen()) {
        processEvents();
        const float deltaSeconds = std::min(frameClock.restart().asSeconds(), 0.1F);
        update(deltaSeconds);
        render();
    }
    return EXIT_SUCCESS;
}

void Application::processEvents() {
    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
            continue;
        }
        if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            processKey(*key);
            continue;
        }
        if (const auto* text = event->getIf<sf::Event::TextEntered>()) {
            if (text->unicode >= 32 && text->unicode <= 126 && input_.size() < 78) {
                input_.push_back(static_cast<char>(text->unicode));
            }
        }
    }
}

void Application::processKey(const sf::Event::KeyPressed& event) {
    using Key = sf::Keyboard::Key;
    if (event.code == Key::Enter) {
        submitInput();
    } else if (event.code == Key::Backspace && !input_.empty()) {
        input_.pop_back();
    } else if (event.code == Key::Escape) {
        if (core_.mode() == Mode::Terminal) {
            enqueue(core_.submit("quit"));
            quitTimer_ = 2.0F;
        } else {
            enqueue(core_.submit("quit"));
        }
    } else if (event.code == Key::Up && !inputHistory_.empty()) {
        if (historyCursor_ > 0) {
            --historyCursor_;
        }
        input_ = inputHistory_[historyCursor_];
    } else if (event.code == Key::Down && !inputHistory_.empty()) {
        if (historyCursor_ + 1 < inputHistory_.size()) {
            ++historyCursor_;
            input_ = inputHistory_[historyCursor_];
        } else {
            historyCursor_ = inputHistory_.size();
            input_.clear();
        }
    } else if (event.code == Key::F2) {
        shaderEnabled_ = !shaderEnabled_ && sf::Shader::isAvailable()
                             ? crtShader_.loadFromFile(locateAsset("shaders/crt.frag"),
                                                       sf::Shader::Type::Fragment)
                             : false;
        if (shaderEnabled_) {
            crtShader_.setUniform("u_texture", sf::Shader::CurrentTexture);
        }
    } else if (event.code == Key::F12) {
        saveScreenshot();
    } else if (event.code == Key::V && (event.control || event.system)) {
        std::string pasted = sf::Clipboard::getString().toAnsiString();
        pasted.erase(std::remove_if(pasted.begin(), pasted.end(), [](unsigned char character) {
                         return character < 32 || character > 126;
                     }),
                     pasted.end());
        if (input_.size() + pasted.size() > 78) {
            pasted.resize(78 - input_.size());
        }
        input_ += pasted;
    }
}

void Application::submitInput() {
    if (!input_.empty()) {
        history_.push_back({"> " + input_, Tone::Bright});
        inputHistory_.push_back(input_);
        historyCursor_ = inputHistory_.size();
    } else if (core_.mode() == Mode::Terminal) {
        return;
    }

    CoreResult result = core_.submit(input_);
    input_.clear();
    if (result.clearScreen) {
        history_.clear();
        pending_.clear();
    }
    if (result.crtIntensity) {
        crtIntensity_ = *result.crtIntensity;
    }
    if (result.requestQuit) {
        quitTimer_ = 2.0F;
    }
    enqueue(result);
}

void Application::update(float deltaSeconds) {
    elapsedTime_ += deltaSeconds;
    glitchPulse_ = std::max(0.0F, glitchPulse_ - deltaSeconds * 0.85F);
    systemLoad_ = std::clamp(0.40F + 0.25F * std::sin(elapsedTime_ * 0.73F) +
                                 (core_.mode() == Mode::Simulation ? 0.28F : 0.0F),
                             0.05F,
                             0.98F);

    CoreResult timedResult = core_.update(deltaSeconds);
    enqueue(timedResult);

    if (!pending_.empty()) {
        PendingLine& current = pending_.front();
        if (!current.complete) {
            current.characterAccumulator += deltaSeconds * current.output.charactersPerSecond;
            const auto newCharacters = static_cast<std::size_t>(current.characterAccumulator);
            if (newCharacters > 0) {
                current.characterAccumulator -= static_cast<float>(newCharacters);
                current.visibleCharacters = std::min(current.visibleCharacters + newCharacters,
                                                     current.output.text.size());
            }
            if (current.visibleCharacters >= current.output.text.size()) {
                current.complete = true;
                current.pauseRemaining = current.output.pauseAfter;
            }
        } else {
            current.pauseRemaining -= deltaSeconds;
            if (current.pauseRemaining <= 0.0F) {
                history_.push_back({current.output.text, current.output.tone});
                pending_.pop_front();
                while (history_.size() > 220) {
                    history_.pop_front();
                }
            }
        }
    }

    if (quitTimer_ >= 0.0F) {
        quitTimer_ -= deltaSeconds;
        if (quitTimer_ <= 0.0F || (pending_.empty() && quitTimer_ < 1.2F)) {
            window_.close();
        }
    }
}

void Application::enqueue(const CoreResult& result) {
    for (const OutputLine& output : result.lines) {
        enqueueLine(output);
    }
    glitchPulse_ = std::max(glitchPulse_, result.glitchPulse);
    if (result.crtIntensity) {
        crtIntensity_ = *result.crtIntensity;
    }
}

void Application::enqueueLine(OutputLine output) {
    const std::vector<std::string> wrapped = wrap(output.text, 70);
    for (std::size_t index = 0; index < wrapped.size(); ++index) {
        OutputLine part = output;
        part.text = wrapped[index];
        if (index + 1 < wrapped.size()) {
            part.pauseAfter = 0.0F;
        }
        pending_.push_back({std::move(part)});
    }
}

void Application::render() {
    renderTerminal();
    terminalTexture_.display();

    window_.clear(sf::Color(2, 2, 2));
    sf::Sprite terminalSprite(terminalTexture_.getTexture());
    const sf::Vector2u windowSize = window_.getSize();
    const float scale = std::min(static_cast<float>(windowSize.x) / static_cast<float>(VirtualWidth),
                                 static_cast<float>(windowSize.y) / static_cast<float>(VirtualHeight));
    const sf::Vector2f renderedSize{static_cast<float>(VirtualWidth) * scale,
                                    static_cast<float>(VirtualHeight) * scale};
    terminalSprite.setScale({scale, scale});
    terminalSprite.setPosition({(static_cast<float>(windowSize.x) - renderedSize.x) * 0.5F,
                                (static_cast<float>(windowSize.y) - renderedSize.y) * 0.5F});

    if (shaderEnabled_) {
        crtShader_.setUniform("u_time", elapsedTime_);
        crtShader_.setUniform("u_intensity", crtIntensity_);
        crtShader_.setUniform("u_glitch", glitchPulse_);
        crtShader_.setUniform("u_outputScale", scale);
        window_.draw(terminalSprite, sf::RenderStates(&crtShader_));
    } else {
        window_.draw(terminalSprite);
    }
    window_.display();
}

void Application::renderTerminal() {
    terminalTexture_.clear(BlackGreen);

    sf::RectangleShape frame({1160.0F, 710.0F});
    frame.setPosition({20.0F, 20.0F});
    frame.setFillColor(sf::Color(1, 12, 5));
    frame.setOutlineColor(sf::Color(42, 142, 75));
    frame.setOutlineThickness(2.0F);
    terminalTexture_.draw(frame);

    drawText(terminalTexture_, "WOPR / JOSHUA", {42.0F, 29.0F}, 31, PhosphorBright);
    drawText(terminalTexture_, "AUTONOMOUS STRATEGY PROCESSOR", {42.0F, 59.0F}, 19, PhosphorDim);
    drawText(terminalTexture_, "SESSION 7A-114", {956.0F, 37.0F}, 21, Phosphor);

    sf::RectangleShape headerRule({1116.0F, 1.0F});
    headerRule.setPosition({42.0F, 86.0F});
    headerRule.setFillColor(sf::Color(47, 155, 83));
    terminalTexture_.draw(headerRule);

    const std::size_t visibleLineCount = 24;
    const bool hasTypingLine = !pending_.empty();
    const std::size_t committedSlots = visibleLineCount - (hasTypingLine ? 1 : 0);
    const std::size_t first = history_.size() > committedSlots ? history_.size() - committedSlots : 0;
    float y = 100.0F;
    for (std::size_t index = first; index < history_.size(); ++index) {
        drawText(terminalTexture_, history_[index].text, {45.0F, y}, 24, colorFor(history_[index].tone));
        y += 22.5F;
    }

    if (hasTypingLine) {
        const PendingLine& current = pending_.front();
        const std::string visible = current.output.text.substr(0, current.visibleCharacters);
        drawText(terminalTexture_, visible, {45.0F, y}, 24, colorFor(current.output.tone));
        if (!current.complete && std::fmod(elapsedTime_ * 6.0F, 2.0F) < 1.0F) {
            const float cursorX = 45.0F + static_cast<float>(visible.size()) * 12.0F;
            sf::RectangleShape typingCursor({9.0F, 19.0F});
            typingCursor.setPosition({cursorX, y + 4.0F});
            typingCursor.setFillColor(colorFor(current.output.tone));
            terminalTexture_.draw(typingCursor);
        }
    }

    sf::RectangleShape inputRule({850.0F, 1.0F});
    inputRule.setPosition({42.0F, 662.0F});
    inputRule.setFillColor(sf::Color(31, 104, 57));
    terminalTexture_.draw(inputRule);

    const std::string prompt = core_.mode() == Mode::TicTacToe ? "MOVE> " :
                               core_.mode() == Mode::Simulation ? "SIM>  " : "JOSH> ";
    drawText(terminalTexture_, prompt + input_, {45.0F, 673.0F}, 27, PhosphorBright);
    if (std::fmod(elapsedTime_, 1.0F) < 0.53F) {
        const float cursorX = 45.0F + static_cast<float>(prompt.size() + input_.size()) * 13.3F;
        sf::RectangleShape cursor({10.0F, 21.0F});
        cursor.setPosition({cursorX, 678.0F});
        cursor.setFillColor(PhosphorBright);
        terminalTexture_.draw(cursor);
    }

    renderSidebar();

    const float scanY = 25.0F + std::fmod(elapsedTime_ * 71.0F, 700.0F);
    sf::RectangleShape scanGlow({1150.0F, 3.0F});
    scanGlow.setPosition({25.0F, scanY});
    scanGlow.setFillColor(sf::Color(90, 255, 130, 5));
    terminalTexture_.draw(scanGlow);

    drawText(terminalTexture_, "F2 CRT  |  F12 SCREENSHOT  |  ESC DISCONNECT",
             {43.0F, 711.0F}, 15, sf::Color(35, 105, 61));
}

void Application::renderSidebar() {
    sf::RectangleShape divider({1.0F, 572.0F});
    divider.setPosition({918.0F, 98.0F});
    divider.setFillColor(sf::Color(31, 112, 60));
    terminalTexture_.draw(divider);

    drawText(terminalTexture_, "SYSTEM MONITOR", {942.0F, 104.0F}, 23, PhosphorBright);
    drawText(terminalTexture_, "MODE", {942.0F, 145.0F}, 17, PhosphorDim);
    drawText(terminalTexture_, core_.modeLabel(), {942.0F, 165.0F}, 23,
             core_.mode() == Mode::Simulation ? Amber : Phosphor);
    drawText(terminalTexture_, "NETWORK", {942.0F, 205.0F}, 17, PhosphorDim);
    drawText(terminalTexture_, "LOCAL / AIR-GAPPED", {942.0F, 225.0F}, 20, Phosphor);
    drawText(terminalTexture_, "DEFCON", {942.0F, 267.0F}, 17, PhosphorDim);
    drawText(terminalTexture_, std::to_string(core_.defconLevel()), {942.0F, 286.0F}, 38,
             core_.defconLevel() <= 2 ? Amber : PhosphorBright);

    drawText(terminalTexture_, "PROCESSOR LOAD", {942.0F, 344.0F}, 17, PhosphorDim);
    sf::RectangleShape loadBorder({192.0F, 16.0F});
    loadBorder.setPosition({942.0F, 370.0F});
    loadBorder.setFillColor(sf::Color::Transparent);
    loadBorder.setOutlineColor(PhosphorDim);
    loadBorder.setOutlineThickness(1.0F);
    terminalTexture_.draw(loadBorder);
    sf::RectangleShape loadFill({188.0F * systemLoad_, 12.0F});
    loadFill.setPosition({944.0F, 372.0F});
    loadFill.setFillColor(core_.defconLevel() <= 2 ? Amber : Phosphor);
    terminalTexture_.draw(loadFill);

    drawText(terminalTexture_, "STRATEGY MATRIX", {942.0F, 414.0F}, 17, PhosphorDim);
    constexpr float cell = 18.0F;
    for (int row = 0; row < 6; ++row) {
        for (int column = 0; column < 10; ++column) {
            const float wave = std::sin(elapsedTime_ * 2.2F + static_cast<float>(row * 2 + column));
            const bool active = wave > (core_.mode() == Mode::Simulation ? -0.2F : 0.55F);
            sf::RectangleShape matrixCell({11.0F, 9.0F});
            matrixCell.setPosition({943.0F + static_cast<float>(column) * cell,
                                    441.0F + static_cast<float>(row) * 17.0F});
            matrixCell.setFillColor(active ? sf::Color(74, 221, 117, 170)
                                           : sf::Color(15, 58, 30, 150));
            terminalTexture_.draw(matrixCell);
        }
    }

    drawText(terminalTexture_, "DISPLAY", {942.0F, 561.0F}, 17, PhosphorDim);
    std::ostringstream display;
    display << "CRT " << static_cast<int>(crtIntensity_ * 100.0F) << "%";
    drawText(terminalTexture_, display.str(), {942.0F, 581.0F}, 21, Phosphor);
    drawText(terminalTexture_, shaderEnabled_ ? "SHADER ONLINE" : "SHADER BYPASS",
             {942.0F, 609.0F}, 19, shaderEnabled_ ? Phosphor : Amber);
    drawText(terminalTexture_, "SAFE SIMULATION", {942.0F, 641.0F}, 17, PhosphorDim);
}

void Application::drawText(sf::RenderTarget& target,
                           const std::string& value,
                           sf::Vector2f position,
                           unsigned int size,
                           sf::Color color) const {
    sf::Text text(font_, value, size);
    text.setPosition(position);
    text.setFillColor(color);
    target.draw(text);
}

sf::Color Application::colorFor(Tone tone) const {
    switch (tone) {
        case Tone::Bright:
            return PhosphorBright;
        case Tone::Dim:
            return PhosphorDim;
        case Tone::Alert:
            return Amber;
        case Tone::Normal:
        default:
            return Phosphor;
    }
}

std::vector<std::string> Application::wrap(const std::string& value, std::size_t width) const {
    if (value.size() <= width || value.find("+---") != std::string::npos ||
        value.rfind("    |", 0) == 0) {
        return {value};
    }

    std::vector<std::string> lines;
    std::istringstream words(value);
    std::string current;
    std::string word;
    while (words >> word) {
        if (current.empty()) {
            current = word;
        } else if (current.size() + 1 + word.size() <= width) {
            current += " " + word;
        } else {
            lines.push_back(current);
            current = word;
        }
    }
    if (!current.empty()) {
        lines.push_back(current);
    }
    return lines.empty() ? std::vector<std::string>{value} : lines;
}

std::filesystem::path Application::locateAsset(const std::filesystem::path& relative) const {
    std::vector<std::filesystem::path> roots = {
        std::filesystem::current_path() / "assets",
        executablePath_.parent_path() / "assets",
        executablePath_.parent_path().parent_path() / "Resources" / "assets",
    };
#ifdef WARGAMES_ASSET_DIR
    roots.emplace_back(WARGAMES_ASSET_DIR);
#endif
    for (const auto& root : roots) {
        const auto candidate = root / relative;
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    throw std::runtime_error("Required asset not found: " + relative.string());
}

void Application::saveScreenshot() {
    const auto image = terminalTexture_.getTexture().copyToImage();
    if (image.saveToFile("joshua-screenshot.png")) {
        enqueueLine({"SCREENSHOT SAVED: JOSHUA-SCREENSHOT.PNG", Tone::Dim, 80.0F, 0.05F});
    } else {
        enqueueLine({"UNABLE TO SAVE SCREENSHOT.", Tone::Alert, 60.0F, 0.05F});
    }
}

} // namespace joshua
