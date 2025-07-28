// Ilya Nalivaiko 2025

#pragma once
#include "GeometryChunk.h"
#include "external/json.hpp"
#include "external/tiny_gltf.h"
#include "Modeler/Matrix.h"
#include "Modeler/TextureFrame.h"
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <fstream>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <opencv2/core/eigen.hpp>
#include <numeric>


using json = nlohmann::json;

std::string encodeToGltf(
    const std::vector<dlovi::Matrix>& points,
    const std::list<dlovi::Matrix>& tris,
    const std::vector<std::string>& textureUrls,
    const tinygltf::Value::Object& extras);
bool PerformSVDCompression(
    const cv::Mat& imageMatrix,             // [H*W x N], CV_32F
    int maxComponents,                      // if <= 0, auto-select to preserve 95% energy
    std::vector<cv::Mat>& basisImagesOut,   // output basis images (grayscale)
    cv::Mat& meanImageOut,                  // output mean image (grayscale)
    cv::Mat& coefficientMatrixOut           // output coefficients [k x N], CV_32F
);
tinygltf::Value BuildPoseExtras(const std::vector<std::pair<cv::Mat, ORB_SLAM2::TextureFrame>>& rgbTexFrames);
bool BuildGrayscaleImageMatrix(
    const std::vector<std::pair<cv::Mat, ORB_SLAM2::TextureFrame>>& rgbTexFrames,
    cv::Mat& outMatrix,
    cv::Size& outImageSize);
void PostprocessSVDOutput(
    std::vector<cv::Mat>& basisImages,  // [flattened float32], modified in-place
    cv::Mat& meanImage,                 // [flattened float32], modified in-place
    cv::Mat& coefficients,              // [k x N float32], modified in-place
    int imageHeight,
    int imageWidth
);