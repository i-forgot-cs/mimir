cmake_minimum_required(VERSION 3.20)
project(local_assistant LANGUAGES CXX)

# Use modern C++ and don't allow compiler-specific extensions.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Dependencies (downloaded automatically at configure time) ---
include(FetchContent)

# JSON parsing/building
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)

# Header-only HTTP client (plain http:// to localhost needs no OpenSSL)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.0
)

FetchContent_MakeAvailable(json httplib)

# --- Our program ---
add_executable(assistant src/main.cpp)

target_link_libraries(assistant PRIVATE
  nlohmann_json::nlohmann_json
  httplib::httplib
)

# Turn on warnings early; they teach you a lot.
if(MSVC)
  target_compile_options(assistant PRIVATE /W4 /permissive-)
else()
  target_compile_options(assistant PRIVATE -Wall -Wextra -Wpedantic)
endif()