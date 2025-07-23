// Ilya Nalivaiko 2025

#include "HttpService.h"

// Helper function because this isnt built in for some reason
// See if a string ends with another (substring)
bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

HttpService::HttpService(int port, ChunkCache& cache) : port_(port), cache_(cache) {
    server_.Get(R"(/chunk/(\d+))", [&](const auto& req, auto& res){
        int id = std::stoi(req.matches[1]);
        auto chunk = cache_.get(id);
        if (!chunk) {
            res.status = 404;
            res.set_content("Chunk not found", "text/plain");
            return;
        }
        res.set_content(chunk->gltf_json, "application/json");
    });

    server_.Get(R"(/texture/(\d+)/(.*))", [&](const auto& req, auto& res){
        int id = std::stoi(req.matches[1]);
        std::string filename = req.matches[2];

        auto chunk = cache_.get(id);
        if (!chunk) {
            res.status = 404;
            res.set_content("Chunk not found", "text/plain");
            return;
        }

        std::string mime = "application/octet-stream";
        if (ends_with(filename, ".png")) mime = "image/png";
        else if (ends_with(filename, ".jpg") || ends_with(filename, ".jpeg")) mime = "image/jpeg";
        else if (ends_with(filename, ".exr")) mime = "image/exr";

        if (ends_with(filename, ".exr")) {
            auto exrIt = chunk->rawExrFiles.find(filename);
            if (exrIt == chunk->rawExrFiles.end()) {
                res.status = 404;
                res.set_content("EXR file not found", "text/plain");
                return;
            }
            res.set_content(reinterpret_cast<const char*>(exrIt->second.data()), exrIt->second.size(), mime);
        } else {
            auto it = chunk->textures.find(filename);
            if (it == chunk->textures.end()) {
                res.status = 404;
                res.set_content("Texture not found", "text/plain");
                return;
            }
            std::vector<uchar> encoded;
            std::vector<int> params = {cv::IMWRITE_PNG_COMPRESSION, 3};
            cv::imencode(".png", it->second, encoded, params);
            res.set_content(reinterpret_cast<const char*>(encoded.data()), encoded.size(), mime);
        }
    });
}

void HttpService::start() {
    server_thread_ = std::thread([this] {
        std::cout << "[HTTP] Server starting on port " << port_ << "\n";
        server_.listen("0.0.0.0", port_);
    });
}

void HttpService::stop() {
    server_.stop();
    if (server_thread_.joinable()) server_thread_.join();
}
