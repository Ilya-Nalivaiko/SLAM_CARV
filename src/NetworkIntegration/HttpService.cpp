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
        else{
            res.status = 404;
            res.set_content("Unsupported texture format", "text/plain");
            return;
        }

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

            cv::Mat img = it->second;

            std::ostringstream dbg;
            dbg << "== [HTTP Server Debug] ==\n";

            // Ensure contiguous memory
            if (!img.isContinuous()) {
                dbg << "Matrix not continuous, cloning...\n";
                img = img.clone();
            } else {
                dbg << "Matrix is already continuous.\n";
            }

            size_t dataSize = img.total() * img.elemSize();
            dbg << "Total data size (bytes): " << dataSize << "\n";
            dbg << "Element size (bytes): " << img.elemSize() << "\n";
            dbg << "Number of elements: " << img.total() << "\n";

            // Hex dump first few floats
            dbg << "First 10 floats (hex + float):\n";
            const float* fdata = reinterpret_cast<const float*>(img.data);
            for (size_t i = 0; i < std::min<size_t>(10, img.total()); i++) {
                const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&fdata[i]);
                dbg << "  [" << i << "] "
                    << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(bytes[0]) << " "
                    << static_cast<int>(bytes[1]) << " "
                    << static_cast<int>(bytes[2]) << " "
                    << static_cast<int>(bytes[3]) << std::dec
                    << "  -> float=" << fdata[i] << "\n";
            }

            // Check type
            if (img.type() != CV_32FC1) {
                dbg << "Matrix type is NOT CV_32FC1. Got type=" << img.type() << "\n";
                res.status = 500;
                res.set_content(dbg.str(), "text/plain");
                return;
            } else {
                dbg << "Matrix type confirmed CV_32FC1.\n";
            }

            // Dimensions
            int width = img.cols;
            int height = img.rows;
            dbg << "Matrix dimensions: " << width << " x " << height << "\n";

            // TinyEXR setup
            EXRImage exr_image;
            InitEXRImage(&exr_image);

            EXRHeader exr_header;
            InitEXRHeader(&exr_header);

            exr_image.num_channels = 1;
            exr_image.width = width;
            exr_image.height = height;

            float* img_data = reinterpret_cast<float*>(img.data);
            float* channels[1] = { img_data };
            exr_image.images = reinterpret_cast<unsigned char**>(channels);

            dbg << "Assigned channel pointer: " << static_cast<void*>(channels[0]) << "\n";
            dbg << "First float value in channel: " << img_data[0] << "\n";

            // Channel info
            exr_header.num_channels = 1;
            exr_header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo));
            strcpy(exr_header.channels[0].name, "Y");

            dbg << "Channel name set to 'Y'\n";

            int pixel_type[1] = { TINYEXR_PIXELTYPE_FLOAT };
            int requested_type[1] = { TINYEXR_PIXELTYPE_FLOAT };
            exr_header.pixel_types = pixel_type;
            exr_header.requested_pixel_types = requested_type;

            dbg << "Pixel types assigned: FLOAT\n";

            // Encode
            unsigned char* out = nullptr;
            const char* err = nullptr;
            size_t out_size = SaveEXRImageToMemory(&exr_image, &exr_header, &out, &err);

            dbg << "SaveEXRImageToMemory returned size: " << out_size << "\n";
            dbg << "Output pointer: " << static_cast<void*>(out) << "\n";

            if (err) {
                dbg << "TinyEXR reported error: " << err << "\n";
                FreeEXRErrorMessage(err);
            }

            if (out_size == 0 || !out) {
                dbg << "Encoding failed, no output buffer.\n";
                res.status = 500;
                res.set_content(dbg.str(), "text/plain");
                free(exr_header.channels);
                return;
            }

            // Copy to string for serving
            std::string content(reinterpret_cast<char*>(out), out_size);
            dbg << "Copied " << content.size() << " bytes into response string.\n";

            //res.set_content(dbg.str(), "text/plain");
            res.set_content(reinterpret_cast<const char*>(out), out_size, "image/exr");

            // Cleanup
            free(out);
            free(exr_header.channels);
            return;

        } else {
            std::cout << "Serving PNG texture";
            std::vector<int> params = {cv::IMWRITE_PNG_COMPRESSION, 3};
            cv::imencode(".png", it->second, encoded, params);
            res.set_content(reinterpret_cast<const char*>(encoded.data()), encoded.size(), mime);
            return;
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
