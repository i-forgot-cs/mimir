#include "config.hpp"

#include <stdexcept>

#include <toml++/toml.hpp>

namespace assistant {

Config load_config(const std::string& path) {
    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (const toml::parse_error& err) {
        throw std::runtime_error("Could not read config '" + path + "' (line " +
                                 std::to_string(err.source().begin.line) + "): " +
                                 std::string(err.description()));
    }

    // Start from the defaults, then override whatever the file provides.
    // tbl["ollama"]["host"] is safe even if the section or key is missing:
    // it gives an "empty view", and value_or() then returns the fallback.
    Config cfg;
    cfg.host = tbl["ollama"]["host"].value_or(cfg.host);
    cfg.port = tbl["ollama"]["port"].value_or(cfg.port);
    cfg.model = tbl["ollama"]["model"].value_or(cfg.model);
    cfg.vision_model = tbl["ollama"]["vision_model"].value_or(cfg.vision_model);
    cfg.think = tbl["ollama"]["think"].value_or(cfg.think);
    cfg.timeout_seconds = tbl["ollama"]["timeout_seconds"].value_or(cfg.timeout_seconds);
    cfg.system_prompt = tbl["assistant"]["system_prompt"].value_or(cfg.system_prompt);

    return cfg;
}

}  // namespace assistant