// Ilya Nalivaiko 2025

#pragma once
#include <vector>
#include <string>

struct Vec3 { float x,y,z; };

struct Vec2 {
    float u, v;
};

//Easier way to input geometry
// NOTE: may change depending on what CARV already stores it as
struct GeometryChunk {
    int id;
    std::vector<Vec3> vertices;
    std::vector<Vec2> uvs;
    std::vector<std::string> textureUrls;  // <-- added
};
