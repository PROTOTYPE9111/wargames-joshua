#include "Application.hpp"

#include <SFML/System/Exception.hpp>

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    try {
        const std::filesystem::path executable = argc > 0 ? argv[0] : "wargames_joshua";
        joshua::Application application(executable);
        return application.run();
    } catch (const sf::Exception& error) {
        std::cerr << "SFML error: " << error.what() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Startup error: " << error.what() << '\n';
    }
    return 1;
}
