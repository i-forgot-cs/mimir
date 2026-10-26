#include "ollama_client.hpp"

#include <algorithm>
#include <utility>

#include <httplib.h>
#include <nlohmann/json.hpp>

namespace assistant {

namespace {

using json = nlohmann::json;

json to_json(const Message& m) {
    json j;
    j["role"] = m.role;
    j["content"] = m.content;
    if (!m.images.empty()) {
        j["images"] = m.images;  // a JSON array of base64 strings
    }
    return j;
}

}  // namespace

OllamaClient::OllamaClient(Config config) : config_(std::move(config)) {}

std::string OllamaClient::chat(const std::vector<Message>& history,
                               const TokenCallback& on_token) const {
    // Build the message list: system prompt first, then the conversation.
    json messages = json::array();
    if (!config_.system_prompt.empty()) {
        messages.push_back(to_json(Message{"system", config_.system_prompt, {}}));
    }
    for (const Message& m : history) {
        messages.push_back(to_json(m));
    }

    // Use the vision model if any message in the conversation has an image.
    const bool has_images = std::any_of(history.begin(), history.end(),
                                        [](const Message& m) { return !m.images.empty(); });
    const std::string& model =
        (has_images && !config_.vision_model.empty()) ? config_.vision_model : config_.model;

    json body;
    body["model"] = model;
    body["messages"] = messages;
    body["stream"] = true;
    body["think"] = config_.think;

    std::string reply;          // the complete answer, built piece by piece
    std::string pending;        // received bytes that aren't a full line yet
    std::string error_message;  // set if Ollama reports an error mid-stream

    httplib::Client client(config_.host, config_.port);
    client.set_read_timeout(config_.timeout_seconds, 0);

    httplib::Request req;
    req.method = "POST";
    req.path = "/api/chat";
    req.set_header("Content-Type", "application/json");
    req.body = body.dump();

    // Ollama streams one JSON object per line, but a network chunk can end in
    // the middle of a line, so we buffer and only parse complete lines.
    // Note: we don't throw inside this callback; we record the problem and
    // throw after the request returns.
    req.content_receiver = [&](const char* data, size_t len, uint64_t, uint64_t) {
        pending.append(data, len);

        size_t newline = 0;
        while ((newline = pending.find('\n')) != std::string::npos) {
            const std::string line = pending.substr(0, newline);
            pending.erase(0, newline + 1);
            if (line.empty()) continue;

            try {
                const json chunk = json::parse(line);

                if (chunk.contains("error")) {
                    error_message = chunk["error"].dump();
                    return false;  // stop receiving
                }
                if (chunk.contains("message")) {
                    const std::string text = chunk["message"].value("content", "");
                    if (!text.empty()) {
                        reply += text;
                        on_token(text);
                    }
                }
            } catch (const json::exception&) {
                // A malformed line: skip it rather than crash the whole chat.
            }
        }
        return true;
    };

    const auto res = client.send(req);

    if (!error_message.empty()) {
        throw OllamaError("Ollama error: " + error_message);
    }
    if (!res) {
        throw OllamaError("Could not reach Ollama at " + config_.host + ":" +
                          std::to_string(config_.port) + " (" +
                          httplib::to_string(res.error()) + "). Is it running?");
    }
    if (res->status != 200) {
        throw OllamaError("Ollama returned HTTP " + std::to_string(res->status) +
                          (res->body.empty() ? "" : ": " + res->body));
    }
    return reply;
}

}  // namespace assistant