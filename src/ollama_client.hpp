#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.hpp"

namespace assistant {

// One message in a conversation. role is "user" or "assistant".
struct Message {
    std::string role;
    std::string content;
    // Base64-encoded images attached to this message (usually empty).
    std::vector<std::string> images;
};

// Thrown when talking to Ollama fails (not running, model missing, bad reply...).
class OllamaError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Everything about talking to Ollama lives in this class. The rest of the
// program never sees HTTP or JSON; it just calls chat().
class OllamaClient {
public:
    explicit OllamaClient(Config config);

    // Called with each piece of text as it arrives from the model.
    using TokenCallback = std::function<void(const std::string&)>;

    // Sends the conversation (the system prompt from the config is added
    // automatically), calls on_token as text streams in, and returns the
    // complete reply. If any message contains images, the configured
    // vision_model is used (when set). Throws OllamaError on failure.
    std::string chat(const std::vector<Message>& history,
                     const TokenCallback& on_token) const;

private:
    Config config_;
};

}  // namespace assistant