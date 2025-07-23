// Ilya Nalivaiko 2025

#pragma once
#include <string>
#include <unordered_map>
#include <opencv2/core.hpp>

struct GltfChunk {
    std::string gltf_json;
    std::unordered_map<std::string, cv::Mat> textures; // for PNG/JPG
    std::unordered_map<std::string, std::vector<uint8_t>> rawExrFiles; // for EXR raw bytes
};