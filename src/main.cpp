// Streaming terminal chat with image support. Settings come from config.toml.
//
// Usage:  assistant.exe [path-to-config.toml]
// Commands while chatting:
//   /image <path>   attach an image to your NEXT message
//   /clear          forget the conversation
//   /quit           exit

#include <cctype>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "config.hpp"
#include "image.hpp"
#include "ollama_client.hpp"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>  // for SetConsoleOutputCP / SetConsoleCP
#endif

namespace {

// Removes surrounding spaces and quotes. Windows' "Copy as path" wraps paths
// in double quotes, so this lets you paste them straight in.
std::string strip_quotes_and_space(std::string s) {
    auto is_junk = [](unsigned char c) { return std::isspace(c) || c == '"' || c == '\''; };
    while (!s.empty() && is_junk(static_cast<unsigned char>(s.back()))) s.pop_back();
    std::size_t start = 0;
    while (start < s.size() && is_junk(static_cast<unsigned char>(s[start]))) ++start;
    return s.substr(start);
}

}  // namespace

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Make the console handle UTF-8 so accents and emoji display correctly.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    const std::string config_path = (argc > 1) ? argv[1] : "config.toml";

    assistant::Config config;
    try {
        config = assistant::load_config(config_path);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n"
                  << "(Run the program from the folder containing config.toml, "
                     "or pass the path as an argument.)\n";
        return 1;
    }

    const assistant::OllamaClient client(config);
    std::vector<assistant::Message> history;
    std::vector<std::string> pending_images;  // waiting to go with the next message

    std::cout << "Chatting with " << config.model << ".\n"
              << "/image <path> attaches an image, /clear resets, /quit exits.\n";

    std::string input;
    while (true) {
        std::cout << "\nyou> " << std::flush;
        if (!std::getline(std::cin, input) || input == "/quit") break;
        if (input.empty()) continue;

        if (input == "/clear") {
            history.clear();
            pending_images.clear();
            std::cout << "(conversation cleared)\n";
            continue;
        }

        if (input.rfind("/image ", 0) == 0) {  // true if input starts with "/image "
            const std::string path = strip_quotes_and_space(input.substr(7));
            try {
                pending_images.push_back(assistant::load_image_base64(path));
                std::cout << "(image attached; type your question about it)\n";
            } catch (const std::exception& e) {
                std::cerr << e.what() << "\n";
            }
            continue;
        }

        // Send any attached images along with this message.
        history.push_back({"user", input, std::move(pending_images)});
        pending_images.clear();  // a moved-from vector is valid but unspecified; reset it

        std::cout << "\nai> " << std::flush;
        try {
            const std::string reply = client.chat(history, [](const std::string& piece) {
                std::cout << piece << std::flush;  // show text as it arrives
            });
            std::cout << "\n";
            history.push_back({"assistant", reply, {}});
        } catch (const assistant::OllamaError& e) {
            std::cerr << "\n[error] " << e.what() << "\n";
            // Drop the failed turn, but keep its images so you can just retry.
            pending_images = std::move(history.back().images);
            history.pop_back();
        }
    }

    return 0;
}