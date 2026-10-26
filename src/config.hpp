#pragma once

#include <string>

namespace assistant {

// All user-adjustable settings live here. The values below are the defaults,
// used for any key that is missing from config.toml.
struct Config {
    std::string host = "localhost";
    int port = 11434;
    std::string model = "qwen3:4b";
    // Model used when the conversation contains images.
    // Leave empty to use `model` for everything.
    std::string vision_model = "";
    bool think = false;
    int timeout_seconds = 300;
    std::string system_prompt = "You are a helpful assistant.";
};

// Reads a TOML file and returns the settings.
// Throws std::runtime_error if the file is missing or isn't valid TOML.
Config load_config(const std::string& path);

}  // namespace assistant