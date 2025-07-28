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
            std::cout << "Serving EXR matrix";

            cv::Mat img = it->second;

            if (!img.isContinuous()) {
                img = img.clone(); // ensure continuous memory
            }







            
            // TODO temp debug

            std::ostringstream oss;
            oss << std::hex << std::setfill('0');

            const unsigned char* data = img.data;
            size_t dataSize = img.total() * img.elemSize();

            for (size_t i = 0; i < dataSize; i++) {
                oss << std::setw(2) << static_cast<int>(data[i]);
                if ((i + 1) % img.elemSize() == 0) oss << " "; // space between elements
            }

            // for some reason this doesnt work
            std::cout << "== [HTTP Server] == cv::Mat image contains hex: " << oss.str();

            // this does give stuff
            // res.set_content(oss.str(), "text/plain");
            // return;







            if (img.type() != CV_32FC1) {
                res.status = 500;
                res.set_content("Matrix must be CV_32FC1 (single channel float)", "text/plain");
                return;
            }

            int width = img.cols;
            int height = img.rows;

            EXRImage exr_image;
            InitEXRImage(&exr_image);

            EXRHeader exr_header;
            InitEXRHeader(&exr_header);

            exr_image.num_channels = 1;
            exr_image.width = width;
            exr_image.height = height;

            // TinyEXR expects array of channel pointers
            float* img_data = (float*)img.data;
            float* channels[1] = { img_data };
            exr_image.images = reinterpret_cast<unsigned char**>(channels);

            // Set up channel info
            exr_header.num_channels = 1;
            exr_header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo));
            strcpy(exr_header.channels[0].name, "Y");  // Gray channel name

            int pixel_type[1] = { TINYEXR_PIXELTYPE_FLOAT };
            int requested_type[1] = { TINYEXR_PIXELTYPE_FLOAT };
            exr_header.pixel_types = pixel_type;
            exr_header.requested_pixel_types = requested_type;

            // Encode to memory
            unsigned char* out = nullptr;
            const char* err = nullptr;
            size_t out_size = SaveEXRImageToMemory(&exr_image, &exr_header, &out, &err);

            if (out_size == 0 || !out) {
                std::string errmsg = "[EXR ERROR] ";
                if (err) { errmsg += err; FreeEXRErrorMessage(err); }
                res.status = 500;
                res.set_content(errmsg, "text/plain");
                return;
            }

            // Send as HTTP response
            std::string content(reinterpret_cast<char*>(out), out_size);
            res.set_content(content.data(), content.size(), "image/exr");

            // Cleanup
            free(out);
            free(exr_header.channels);

        } else {
            std::cout << "Serving PNG texture";
            std::vector<int> params = {cv::IMWRITE_PNG_COMPRESSION, 3};
            cv::imencode(".png", it->second, encoded, params);
        }

        res.set_content(reinterpret_cast<const char*>(encoded.data()), encoded.size(), mime);
        return;
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
