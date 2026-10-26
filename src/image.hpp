#pragma once

#include <string>

namespace assistant {

// Encodes raw bytes as base64 text (the format Ollama expects for images).
std::string base64_encode(const std::string& bytes);

// Reads an image file and returns its contents base64-encoded.
// Throws std::runtime_error if the file can't be read, is empty, or is too big.
std::string load_image_base64(const std::string& path);

}  // namespace assistant