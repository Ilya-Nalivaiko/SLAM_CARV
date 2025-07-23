// Ilya Nalivaiko 2025

#pragma once
#include "GeometryChunk.h"
#include "external/json.hpp"
#include "external/tiny_gltf.h"
#include "Modeler/Matrix.h"
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

std::string encodeToGlTF(const GeometryChunk& chunk);
std::string encodeToGltfWithTex(const GeometryChunk& chunk, const std::vector<std::string>& textureUrls);
void addImagesToGltf(json& gltf_json, const std::vector<std::string>& textureUrls);
std::string encodePointsTrisToGltfWithTex(
    const std::vector<dlovi::Matrix>& points,
    const std::list<dlovi::Matrix>& tris,
    const std::vector<std::string>& textureUrls);