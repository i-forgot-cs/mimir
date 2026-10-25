// Milestone 2: a streaming terminal chat with conversation history.
//
// Commands while chatting:  /clear  (forget the conversation)   /quit  (exit)

#include <iostream>
#include <string>

#include <httplib.h>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>  // for SetConsoleOutputCP / SetConsoleCP
#endif

using json = nlohmann::json;

namespace {

constexpr const char* kHost = "localhost";
constexpr int kPort = 11434;
const std::string kModel = "qwen3:4b";  // change me

// Thinking models (like qwen3) spend time "reasoning" before answering, which
// feels slow in a chat. false asks Ollama to skip it. If your model complains
// about this setting, delete the line that uses kThink below.
constexpr bool kThink = false;

json make_message(const std::string& role, const std::string& content) {
    json m;
    m["role"] = role;
    m["content"] = content;
    return m;
}

// Sends the whole conversation to Ollama and prints tokens as they arrive.
// Returns the full reply text, or an empty string if something went wrong.
std::string stream_chat(httplib::Client& client, const json& history) {
    json body;
    body["model"] = kModel;
    body["messages"] = history;
    body["stream"] = true;
    body["think"] = kThink;

    std::string reply;    // the complete answer, built up piece by piece
    std::string pending;  // received bytes that don't form a full line yet
    bool failed = false;

    httplib::Request req;
    req.method = "POST";
    req.path = "/api/chat";
    req.set_header("Content-Type", "application/json");
    req.body = body.dump();

    // httplib calls this every time a chunk of the response arrives.
    // Ollama streams one JSON object per line ("NDJSON"), but a chunk can end
    // in the middle of a line, so we buffer and only parse complete lines.
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
                    std::cerr << "\n[Ollama error] " << chunk["error"].dump() << "\n";
                    failed = true;
                    return false;  // returning false stops the download
                }

                if (chunk.contains("message")) {
                    const std::string text = chunk["message"].value("content", "");
                    std::cout << text << std::flush;  // show it immediately
                    reply += text;
                }
            } catch (const json::exception& e) {
                std::cerr << "\n[could not parse a chunk: " << e.what() << "]\n";
            }
        }
        return true;  // keep receiving
    };

    const auto res = client.send(req);

    if (!res) {
        std::cerr << "\nCould not reach Ollama at " << kHost << ":" << kPort << " ("
                  << httplib::to_string(res.error()) << "). Is it running?\n";
        return "";
    }
    if (failed || res->status != 200) {
        if (res->status != 200) {
            std::cerr << "\n[HTTP " << res->status << "]\n";
        }
        return "";
    }
    return reply;
}

}  // namespace

int main() {
#ifdef _WIN32
    // Make the console handle UTF-8 so accents and emoji display correctly.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    httplib::Client client(kHost, kPort);
    client.set_read_timeout(300, 0);  // first request may be slow (model loading)

    json history = json::array();

    std::cout << "Chatting with " << kModel << ".  /clear resets, /quit exits.\n";

    std::string input;
    while (true) {
        std::cout << "\nyou> " << std::flush;
        if (!std::getline(std::cin, input) || input == "/quit") break;
        if (input.empty()) continue;
        if (input == "/clear") {
            history = json::array();
            std::cout << "(conversation cleared)\n";
            continue;
        }

        history.push_back(make_message("user", input));

        std::cout << "\nai> " << std::flush;
        const std::string reply = stream_chat(client, history);
        std::cout << "\n";

        if (reply.empty()) {
            history.erase(history.size() - 1);  // drop the failed turn
        } else {
            history.push_back(make_message("assistant", reply));
        }
    }

    return 0;
}