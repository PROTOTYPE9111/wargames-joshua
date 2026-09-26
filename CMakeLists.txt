cmake_minimum_required(VERSION 3.22)

project(wargames_joshua
    VERSION 1.0.0
    DESCRIPTION "A WarGames-inspired Joshua strategy terminal"
    LANGUAGES CXX
)

include(FetchContent)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WARGAMES_FETCH_SFML "Download SFML when it is not installed" ON)

find_package(SFML 3 COMPONENTS Graphics Window System CONFIG QUIET)

if(NOT SFML_FOUND)
    if(NOT WARGAMES_FETCH_SFML)
        message(FATAL_ERROR "SFML 3 was not found. Install it or enable WARGAMES_FETCH_SFML.")
    endif()

    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.2
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SFML)
endif()

add_library(joshua_core
    src/JoshuaCore.cpp
    src/JoshuaCore.hpp
)
target_include_directories(joshua_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(wargames_joshua
    src/main.cpp
    src/Application.cpp
    src/Application.hpp
)
target_link_libraries(wargames_joshua PRIVATE joshua_core SFML::Graphics SFML::Window SFML::System)
target_compile_definitions(wargames_joshua PRIVATE
    WARGAMES_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

if(MSVC)
    target_compile_options(joshua_core PRIVATE /W4 /permissive-)
    target_compile_options(wargames_joshua PRIVATE /W4 /permissive-)
else()
    target_compile_options(joshua_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(wargames_joshua PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_command(TARGET wargames_joshua POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:wargames_joshua>/assets"
    COMMENT "Copying fonts and CRT shader"
)

install(TARGETS wargames_joshua RUNTIME DESTINATION bin)
install(DIRECTORY assets DESTINATION bin)

if(BUILD_TESTING)
    add_executable(joshua_core_tests tests/JoshuaCoreTests.cpp)
    target_link_libraries(joshua_core_tests PRIVATE joshua_core)
    add_test(NAME joshua_core_tests COMMAND joshua_core_tests)
endif()
