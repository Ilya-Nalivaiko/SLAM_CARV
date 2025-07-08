// access point for NetworkIntegration to stream geometry and textures remotely to Unity server
// Ilya Nalivaiko 2025

#include "Modeler/ModelDrawer.h"
#include <vector>
#include <list>
#include <utility>
#include <opencv2/core/core.hpp>

namespace ORB_SLAM2
{
    void ModelDrawer::SendModel(bool mbRGB)
    {
        int numKFs = 1;
        std::vector<std::pair<cv::Mat, TextureFrame>> imAndTexFrame = mpModeler->GetTextures(numKFs);

        if (imAndTexFrame.size() >= numKFs)
        {
            UpdateModel();

            // Retrieve geometry
            std::vector<dlovi::Matrix>& points = GetPoints();
            std::list<dlovi::Matrix>& tris = GetTris();

            // Retrieve textures
            std::vector<cv::Mat> images;
            for (auto& pair : imAndTexFrame)
            {
                images.push_back(pair.first);
            }

            // === FOR NOW: simply print counts to confirm correct access ===
            std::cout << "[SendModel] Points: " << points.size() << ", Tris: " << tris.size()
                      << ", Textures: " << images.size() << ", mbRGB: " << mbRGB << std::endl;

            // === PLACEHOLDER ===
            // Replace the below with your server streaming call, e.g.
            // myServer.SendModelWithTextures(points, tris, images);
        }
        else
        {
            std::cerr << "[SendModel] Not enough keyframes for texture retrieval." << std::endl;
        }
    }
}
