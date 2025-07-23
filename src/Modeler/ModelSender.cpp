// access point for NetworkIntegration to stream geometry and textures remotely to Unity server
// Ilya Nalivaiko 2025

#include "Modeler/ModelDrawer.h"
#include "NetworkIntegration/Encoder.h"
#include "NetworkIntegration/Notifier.h"
#include <vector>
#include <list>
#include <utility>
#include <unordered_map>
#include <opencv2/core/core.hpp>

namespace ORB_SLAM2
{
    void ModelDrawer::SendModel(bool mbRGB, ChunkCache& cache, const std::string& ownAddress, const std::string& unityAddress)
    {
        std::cout << "[SendModel_DEBUG] Send request recieved" << std::endl;
        int chunkId = 1337; // Replace or generate appropriately

        int numKFs = 1;
        std::vector<std::pair<cv::Mat, TextureFrame>> imAndTexFrame = mpModeler->GetTextures(numKFs);

        if (imAndTexFrame.size() < numKFs) {
            std::cerr << "[SendModel] Not enough keyframes for texture retrieval." << std::endl;
            return;
        }

        UpdateModel();

        std::cout << "[SendModel_DEBUG] Model Updated" << std::endl;

        std::vector<dlovi::Matrix>& points = GetPoints();
        std::list<dlovi::Matrix>& tris = GetTris();

        // Convert texture images to filenames and URLs
        std::vector<std::string> textureUrls;
        std::unordered_map<std::string, cv::Mat> textureMap;

        std::cout << "[SendModel_DEBUG] Images converted" << std::endl;

        for (size_t i = 0; i < imAndTexFrame.size(); ++i) {
            std::string filename = "tex_" + std::to_string(i) + ".png";
            textureUrls.push_back("http://" + ownAddress + "/texture/" + std::to_string(chunkId) + "/" + filename);
            textureMap[filename] = imAndTexFrame[i].first;
        }

        std::cout << "[SendModel_DEBUG] Texture map created" << std::endl;

        // Encode geometry + texture URLs to GLTF
        std::string gltf = encodePointsTrisToGltfWithTex(points, tris, textureUrls);

        std::cout << "[SendModel_DEBUG] GLTF encoded" << std::endl;

        // Assemble GltfChunk
        auto chunk = std::make_shared<GltfChunk>();
        chunk->gltf_json = std::move(gltf);
        chunk->textures = std::move(textureMap);
        // currently does not have EXR textures, needed later TODO

        std::cout << "[SendModel_DEBUG] Texture map created" << std::endl;

        cache.insert(chunkId, chunk);

        std::cout << "[SendModel] Model uploaded to chunk ID: " << chunkId << std::endl;

        // Send update notification to Unity
        if (!notifyUpdate(chunkId, unityAddress, ownAddress)) {
            std::cerr << "[SendModel] Failed to notify Unity." << std::endl;
        }

        std::cout << "[SendModel_DEBUG] ZMQ notification sent" << std::endl;
    }
}
