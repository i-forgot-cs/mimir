#include "image.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace assistant {

namespace {

constexpr char kAlphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

constexpr std::size_t kMaxImageBytes = 20 * 1024 * 1024;  // 20 MB safety limit

}  // namespace

// Base64 turns every 3 bytes (24 bits) into 4 characters (6 bits each).
// If the input length isn't a multiple of 3, the output is padded with '='.
std::string base64_encode(const std::string& bytes) {
    std::string out;
    out.reserve(((bytes.size() + 2) / 3) * 4);

    // Read byte i as an unsigned number (plain `char` can be negative).
    auto byte_at = [&](std::size_t i) -> std::uint32_t {
        return static_cast<unsigned char>(bytes[i]);
    };

    std::size_t i = 0;
    for (; i + 3 <= bytes.size(); i += 3) {
        const std::uint32_t n = (byte_at(i) << 16) | (byte_at(i + 1) << 8) | byte_at(i + 2);
        out += kAlphabet[(n >> 18) & 63];
        out += kAlphabet[(n >> 12) & 63];
        out += kAlphabet[(n >> 6) & 63];
        out += kAlphabet[n & 63];
    }

    const std::size_t remaining = bytes.size() - i;  // 0, 1 or 2 bytes left over
    if (remaining == 1) {
        const std::uint32_t n = byte_at(i) << 16;
        out += kAlphabet[(n >> 18) & 63];
        out += kAlphabet[(n >> 12) & 63];
        out += "==";
    } else if (remaining == 2) {
        const std::uint32_t n = (byte_at(i) << 16) | (byte_at(i + 1) << 8);
        out += kAlphabet[(n >> 18) & 63];
        out += kAlphabet[(n >> 12) & 63];
        out += kAlphabet[(n >> 6) & 63];
        out += '=';
    }
    return out;
}

std::string load_image_base64(const std::string& path) {
    // Treat the path as UTF-8 so file names with accents work on Windows too.
    const std::u8string utf8_path(reinterpret_cast<const char8_t*>(path.data()), path.size());
    const std::filesystem::path fs_path(utf8_path);

    std::ifstream file(fs_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open image file: " + path);
    }

    const std::string bytes((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

    if (bytes.empty()) {
        throw std::runtime_error("Image file is empty: " + path);
    }
    if (bytes.size() > kMaxImageBytes) {
        throw std::runtime_error("Image is larger than 20 MB: " + path);
    }
    return base64_encode(bytes);
}

}  // namespace assistant