// access point for NetworkIntegration to stream geometry and textures remotely to Unity server
// Ilya Nalivaiko 2025

#include "Modeler/ModelDrawer.h"
#include <vector>
#include <list>
#include <utility>
#include <opencv2/core/core.hpp>

namespace ORB_SLAM2
{
    void ModelDrawer::SendModel(bool mbRGB, ChunkCache& cache, const std::string& ownAddress, const std::string& unityAddress)
    {
        int chunkId = 1337; // Replace or generate appropriately

        int numKFs = 1;
        std::vector<std::pair<cv::Mat, TextureFrame>> imAndTexFrame = mpModeler->GetTextures(numKFs);

        if (imAndTexFrame.size() < numKFs) {
            std::cerr << "[SendModel] Not enough keyframes for texture retrieval." << std::endl;
            return;
        }

        UpdateModel();  // presumably fills `points_` and `tris_`
        std::vector<dlovi::Matrix>& points = GetPoints();
        std::list<dlovi::Matrix>& tris = GetTris();

        // Convert texture images to filenames and URLs
        std::vector<std::string> textureUrls;
        std::map<std::string, cv::Mat> textureMap;

        for (size_t i = 0; i < imAndTexFrame.size(); ++i) {
            std::string filename = "tex_" + std::to_string(i) + ".png";
            textureUrls.push_back("http://" + ownAddress + "/texture/" + std::to_string(chunkId) + "/" + filename);
            textureMap[filename] = imAndTexFrame[i].first;
        }

        // Encode geometry + texture URLs to GLTF
        std::string gltf = encodePointsTrisToGltfWithTex(points, tris, textureUrls);

        // Assemble GltfChunk
        auto chunk = std::make_shared<GltfChunk>();
        chunk->gltf_json = std::move(gltf);
        chunk->textures = std::move(textureMap);
        // If you ever support EXR textures:
        // chunk->rawExrFiles[...] = ...

        cache.insert(chunkId, chunk);

        std::cout << "[SendModel] Model uploaded to chunk ID: " << chunkId << std::endl;

        // Send update notification to Unity
        if (!notifyUpdate(chunkId, unityAddress, ownAddress)) {
            std::cerr << "[SendModel] Failed to notify Unity." << std::endl;
        }
    }
}
