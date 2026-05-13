cmake_minimum_required(VERSION 3.10)
project(Compiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

include(FetchContent)
FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.6.0
)
FetchContent_MakeAvailable(cli11)

add_library(scanner src/scanner/scan.cpp)
target_include_directories(scanner PUBLIC include)

add_library(parser
    src/parser/parse.cpp
    src/parser/semantics.cpp
)
target_link_libraries(parser PUBLIC scanner)
target_include_directories(parser PUBLIC include)

add_executable(compiler src/main.cpp)
target_link_libraries(compiler PRIVATE scanner parser CLI11::CLI11)