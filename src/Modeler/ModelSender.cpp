// access point for NetworkIntegration to stream geometry and textures remotely to Unity server
// Ilya Nalivaiko 2025

#include "Modeler/ModelDrawer.h"
#include "NetworkIntegration/Encoder.h"
#include "NetworkIntegration/Notifier.h"
#include "Modeler/TextureFrame.h"
#include <vector>
#include <list>
#include <utility>
#include <unordered_map>
#include <opencv2/core/core.hpp>

namespace ORB_SLAM2
{
    // Mirrors DrawModel, but sends it to unity instead
    void ModelDrawer::SendModel(bool mbRGB, ChunkCache& cache, const std::string& ownAddress, const std::string& unityAddress)
    {
        std::cout << "[SendModel_DEBUG] Send request recieved" << std::endl;
        int chunkId = 1337; // Replace or generate appropriately


        // ===== get images and points from model ===== 

        //safe to ask for more keyframes than there are in the queue. the harder control is mnMaxTextureQueueSize in Modeler.cc
        int numKFs = 50;
        std::vector<std::pair<cv::Mat, ORB_SLAM2::TextureFrame>> imAndTexFrame = mpModeler->GetTextures(numKFs);

        if (imAndTexFrame.size() < numKFs) {
            std::cerr << "[SendModel] (Warning) Retrieved " << std::to_string(imAndTexFrame.size()) << " keyframes, less than " << std::to_string(numKFs) << " maximum." << std::endl;
            //return;
        }

        std::vector<dlovi::Matrix>& points = GetPoints();
        std::list<dlovi::Matrix>& tris = GetTris();




        // ===== Convert texture images to SVD, add filenames and URLs ===== 

        std::vector<std::string> textureUrls;
        std::unordered_map<std::string, cv::Mat> textureMap;

        // Step 1: Build image matrix and poses
        cv::Mat imageMatrix;
        cv::Size imageSize;
        if (!BuildGrayscaleImageMatrix(imAndTexFrame, imageMatrix, imageSize)) {
            std::cerr << "[SendModel] Failed to build grayscale image matrix.\n";
            return;
        }
        tinygltf::Value originalPoses = BuildPoseExtras(imAndTexFrame);

        // Step 2: Perform SVD
        std::vector<cv::Mat> basisImages;
        cv::Mat meanImage;
        cv::Mat coefficients;
        if (!PerformSVDCompression(imageMatrix, /* auto-k */ 0, basisImages, meanImage, coefficients)) {
            std::cerr << "[SendModel] SVD compression failed.\n";
            return;
        }
        PostprocessSVDOutput(basisImages, meanImage, coefficients, imageSize.height, imageSize.width);

        std::cout << "[SendModel_DEBUG] Textures compressed" << std::endl;

        // Step 3: Package into memory and create filenames/URLs
        std::string baseUrl = "http://" + ownAddress + "/texture/" + std::to_string(chunkId) + "/";

        // Add mean
        std::string meanFilename = "mean.png";
        textureUrls.push_back(baseUrl + meanFilename);
        textureMap[meanFilename] = meanImage;

        // Add basis images
        for (size_t i = 0; i < basisImages.size(); ++i) {
            std::string basisFilename = "basis_" + std::to_string(i) + ".png";
            textureUrls.push_back(baseUrl + basisFilename);
            textureMap[basisFilename] = basisImages[i];
        }

        // Add coefficients
        std::string coeffFilename = "coefficients.exr";
        textureUrls.push_back(baseUrl + coeffFilename);
        textureMap[coeffFilename] = coefficients;

        // Step 4: Build extras JSON
        tinygltf::Value::Object svd;
        svd["mean"] = tinygltf::Value(meanFilename);
        svd["coefficients"] = tinygltf::Value(coeffFilename);
        {
            tinygltf::Value::Array coeffShape;
            coeffShape.push_back(tinygltf::Value(static_cast<int>(coefficients.rows)));
            coeffShape.push_back(tinygltf::Value(static_cast<int>(coefficients.cols)));
            svd["coefficient_shape"] = tinygltf::Value(coeffShape);
        }
        {
            tinygltf::Value::Array basisArray;
            for (size_t i = 0; i < basisImages.size(); ++i) {
                basisArray.push_back(tinygltf::Value("basis_" + std::to_string(i) + ".png"));
            }
            svd["basis"] = tinygltf::Value(basisArray);
        }
        svd["original_poses"] = BuildPoseExtras(imAndTexFrame);
        tinygltf::Value::Object extras;
        extras["svd"] = tinygltf::Value(svd);

        // Step 5: Encode with extras
        std::string gltf = encodeToGltf(points, tris, textureUrls, extras);

        std::cout << "[SendModel_DEBUG] GLTF encoded" << std::endl;



        // ===== add given GLTF with corresponding images to HTTP cache and send update ===== 

        // Assemble GltfChunk
        auto chunk = std::make_shared<GltfChunk>();
        chunk->gltf_json = std::move(gltf);
        chunk->textures = std::move(textureMap);

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
