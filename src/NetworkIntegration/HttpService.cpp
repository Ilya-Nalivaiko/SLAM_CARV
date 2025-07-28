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

        auto it = chunk->textures.find(filename);
        if (it == chunk->textures.end()) {
            res.status = 404;
            res.set_content("Texture not found", "text/plain");
            return;
        }


        //TODO. encoding it every serve may not be optimal, should do it before storing
        // change gltfchunk implementation to have encoded pngs and exrs rather than cv mat

        std::vector<uchar> encoded;
        if (ends_with(filename, ".exr")) {
            if (it->second.type() != CV_32FC1) {
                res.status = 500;
                res.set_content("Only single-channel float mats supported", "text/plain");
                return;
            }

            cv::Mat img = it->second; 
            int width = img.cols;
            int height = img.rows;

            EXRHeader header;
            InitEXRHeader(&header);

            EXRImage image;
            InitEXRImage(&image);

            image.num_channels = 1;
            float* images[1];
            images[0] = (float*)img.data;
            image.images = (unsigned char**)images;
            image.width = width;
            image.height = height;

            const char* channel_names[] = { "Y" };
            int pixel_types[] = { TINYEXR_PIXELTYPE_FLOAT };  // input type
            int requested_types[] = { TINYEXR_PIXELTYPE_FLOAT }; // keep as float

            header.num_channels = 1;
            header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * 1);
            strcpy(header.channels[0].name, channel_names[0]);
            header.pixel_types = pixel_types;
            header.requested_pixel_types = requested_types;

            unsigned char* out = nullptr;
            const char* err = nullptr;

            size_t out_size = SaveEXRImageToMemory(&image, &header, &out, &err);

            if (out_size == 0) {
                res.status = 500;
                res.set_content(std::string("EXR encode failed: ") + (err ? err : "unknown"), "text/plain");
                if (err) FreeEXRErrorMessage(err);
                return;
            }

            // Serve response
            std::string content(reinterpret_cast<char*>(out), out_size);
            res.set_content(content.data(), content.size(), "image/exr");

            // Cleanup
            // TODO this is a memory leak but it (at least the first free()) crashes program
            // free(out);
            // FreeEXRHeader(&header);
            // FreeEXRImage(&image);


        } else {
            std::vector<int> params = {cv::IMWRITE_PNG_COMPRESSION, 3};
            cv::imencode(".png", it->second, encoded, params);
        }

        res.set_content(reinterpret_cast<const char*>(encoded.data()), encoded.size(), mime);
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
