// Encodes given geometry to GLTF to prepare for send
// Ilya Nalivaiko 2025


#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_ENABLE_FS   // <-- enable file system utilities

#include "Encoder.h"

// Take the given geometry and textureurls and convert to valid (if somewhat custom) GlTF
std::string encodeToGltf(
    const std::vector<dlovi::Matrix>& points,
    const std::list<dlovi::Matrix>& tris,
    const std::vector<std::string>& textureUrls,
    const tinygltf::Value::Object& extras)
{

    tinygltf::Model model;
    model.asset.version = "2.0";
    model.defaultScene = 0;
    model.scenes.push_back({});
    model.scenes[0].nodes = {0};
    model.nodes.push_back({});
    model.nodes[0].mesh = 0;
    model.meshes.push_back({});

    std::cout << "[Encoder_GLTF] model instantiated" << std::endl;

    // Flatten vertices
    std::vector<float> pos;
    for (auto& p : points) {
        pos.push_back(p(0));
        pos.push_back(p(1));
        pos.push_back(p(2));
    }

    // Flatten triangle indices
    std::vector<unsigned short> idx;
    for (auto& t : tris) {
        idx.push_back(static_cast<unsigned short>(t(0)));
        idx.push_back(static_cast<unsigned short>(t(1)));
        idx.push_back(static_cast<unsigned short>(t(2)));
    }

    // Construct buffer
    size_t offsetPos = 0;
    size_t offsetIdx = pos.size() * sizeof(float);
    size_t totalSize = offsetIdx + idx.size() * sizeof(unsigned short);
    tinygltf::Buffer rawBuf;
    rawBuf.data.resize(totalSize);
    memcpy(rawBuf.data.data() + offsetPos, pos.data(), pos.size() * sizeof(float));
    memcpy(rawBuf.data.data() + offsetIdx, idx.data(), idx.size() * sizeof(unsigned short));
    model.buffers.push_back(std::move(rawBuf));

    // BufferViews
    if (pos.empty() || idx.empty()) {
        std::cerr << "[ePTtGwT] Empty position or index array — cannot encode GLTF.\n";
        return "{}";
    }

    tinygltf::BufferView bvPos;
    bvPos.buffer = 0;
    bvPos.byteOffset = offsetPos;
    bvPos.byteLength = pos.size() * sizeof(float);
    bvPos.target = TINYGLTF_TARGET_ARRAY_BUFFER;

    tinygltf::BufferView bvIdx;
    bvIdx.buffer = 0;
    bvIdx.byteOffset = offsetIdx;
    bvIdx.byteLength = idx.size() * sizeof(unsigned short);
    bvIdx.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

    model.bufferViews.push_back(bvPos);
    model.bufferViews.push_back(bvIdx);

    std::cout << "[Encoder_GLTF] buffers populated" << std::endl;

    // Accessors
    tinygltf::Accessor ap;
    ap.bufferView = 0;
    ap.byteOffset = 0;
    ap.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    ap.count = static_cast<int>(pos.size() / 3);
    ap.type = TINYGLTF_TYPE_VEC3;

    tinygltf::Accessor ai;
    ai.bufferView = 1;
    ai.byteOffset = 0;
    ai.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
    ai.count = static_cast<int>(idx.size());
    ai.type = TINYGLTF_TYPE_SCALAR;

    model.accessors.push_back(ap);
    model.accessors.push_back(ai);

    // Textures
    if (!textureUrls.empty()) {
        for (auto& url : textureUrls) {
            tinygltf::Image img; img.uri = url; model.images.push_back(std::move(img));
        }
        tinygltf::Texture tex; tex.source = 0; model.textures.push_back(std::move(tex));
        tinygltf::Material mat; mat.pbrMetallicRoughness.baseColorTexture.index = 0; model.materials.push_back(std::move(mat));
    }

    // Primitive
    tinygltf::Primitive prim;
    prim.attributes["POSITION"] = 0;
    prim.indices = 1;
    prim.mode = TINYGLTF_MODE_TRIANGLES;
    if (!textureUrls.empty()) prim.material = 0;
    model.meshes[0].primitives.push_back(prim);

    // Attach extras if provided
    if (!extras.empty()) {

        //both are jsons but god forbid they be compatible
        model.extras = tinygltf::Value(extras);

    }

    // Serialize
    tinygltf::TinyGLTF gltfCtx;
    std::stringstream ss;
    if (!gltfCtx.WriteGltfSceneToStream(&model, ss, false, false)) {
        throw std::runtime_error("Failed to write glTF to string stream.");
    }

    std::cout << "[Encoder_GLTF] serialized" << std::endl;

    return ss.str();
}


// Take keyframes, output SVD-ready matrix
bool BuildGrayscaleImageMatrix(
    const std::vector<std::pair<cv::Mat, ORB_SLAM2::TextureFrame>>& rgbTexFrames,
    cv::Mat& outMatrix,
    cv::Size& outImageSize)
{
    if (rgbTexFrames.empty()) return false;

    cv::Size size = rgbTexFrames[0].first.size();
    int numImages = static_cast<int>(rgbTexFrames.size());
    int numPixels = size.width * size.height;

    outMatrix = cv::Mat(numPixels, numImages, CV_32F);  // [pixels, images]

    for (int i = 0; i < numImages; ++i) {
        cv::Mat gray, floatGray;
        const cv::Mat& rgb = rgbTexFrames[i].first;

        if (rgb.empty() || rgb.size() != size) {
            std::cerr << "[SVD] Image " << i << " is empty or size mismatch.\n";
            return false;
        }

        cv::cvtColor(rgb, gray, cv::COLOR_BGR2GRAY);
        gray.convertTo(floatGray, CV_32F);

        cv::Mat col = floatGray.reshape(1, numPixels); // Flatten to [pixels x 1]
        col.copyTo(outMatrix.col(i));
    }

    outImageSize = size;
    return true;
}



//helper
tinygltf::Value makeArray(const std::vector<double>& vals) {
    tinygltf::Value::Array arr;
    arr.reserve(vals.size());
    for (double v : vals) {
        arr.push_back(tinygltf::Value(v));
    }
    return tinygltf::Value(arr);
}


// Convert per-image pose information to JSON
tinygltf::Value BuildPoseExtras(
    const std::vector<std::pair<cv::Mat, ORB_SLAM2::TextureFrame>>& rgbTexFrames)
{
    tinygltf::Value::Object poseMap;

    for (size_t i = 0; i < rgbTexFrames.size(); ++i) {
        const auto& tex = rgbTexFrames[i].second;
        std::string id = std::to_string(i); // use index, not mFrameID

        const cv::Mat& R = tex.mRcw;
        const cv::Mat& t = tex.mtcw;

        // Build extrinsics as array-of-arrays
        tinygltf::Value::Array extrinsics;
        extrinsics.push_back(makeArray({R.at<float>(0,0), R.at<float>(0,1), R.at<float>(0,2), t.at<float>(0)}));
        extrinsics.push_back(makeArray({R.at<float>(1,0), R.at<float>(1,1), R.at<float>(1,2), t.at<float>(1)}));
        extrinsics.push_back(makeArray({R.at<float>(2,0), R.at<float>(2,1), R.at<float>(2,2), t.at<float>(2)}));
        extrinsics.push_back(makeArray({0.0, 0.0, 0.0, 1.0}));

        // Build intrinsics as array-of-arrays  
        tinygltf::Value::Array intrinsics;
        intrinsics.push_back(makeArray({tex.mfx, 0.0, tex.mcx}));
        intrinsics.push_back(makeArray({0.0, tex.mfy, tex.mcy}));
        intrinsics.push_back(makeArray({0.0, 0.0, 1.0}));

        tinygltf::Value::Object frameInfo;
        frameInfo["extrinsics"] = tinygltf::Value(extrinsics);
        frameInfo["intrinsics"] = tinygltf::Value(intrinsics);

        poseMap[id] = tinygltf::Value(frameInfo);
    }

    return tinygltf::Value(poseMap);
}


// SVD decompose grayscale image matrix to given number of outputs
// In-place, input args
bool PerformSVDCompression(
    const cv::Mat& imageMatrix, int maxComponents,
    std::vector<cv::Mat>& basisImagesOut,
    cv::Mat& meanImageOut,
    cv::Mat& coefficientMatrixOut)
{
    if (imageMatrix.empty() || imageMatrix.type() != CV_32F) {
        std::cerr << "[SVD] Input image matrix must be non-empty and CV_32F.\n";
        return false;
    }

    const int numPixels = imageMatrix.rows;
    const int numViews = imageMatrix.cols;

    // Convert to Eigen
    Eigen::MatrixXf M(numPixels, numViews);
    cv::cv2eigen(imageMatrix, M);

    // Compute mean
    Eigen::VectorXf meanVec = M.rowwise().mean();
    Eigen::MatrixXf Mz = M.colwise() - meanVec;

    // SVD: Mz = U * S * Vt
    Eigen::JacobiSVD<Eigen::MatrixXf> svd(Mz, Eigen::ComputeThinU | Eigen::ComputeThinV);
    const Eigen::VectorXf& S = svd.singularValues();
    const Eigen::MatrixXf& U = svd.matrixU();
    const Eigen::MatrixXf& Vt = svd.matrixV().transpose();

    // Auto-select k if needed (95% energy)
    int k = maxComponents;
    if (k <= 0) {
        float total = S.array().square().sum();
        float running = 0.0f;
        for (int i = 0; i < S.size(); ++i) {
            running += S(i) * S(i);
            if (running / total >= 0.95f) {
                k = i + 1;
                break;
            }
        }
        std::cout << "[SVD] Auto-selected k = " << k << " to preserve 95% energy\n";
    } else {
        k = std::min(k, static_cast<int>(S.size()));
    }

    // Truncate
    Eigen::MatrixXf B = U.leftCols(k);                  // [pixels x k]
    Eigen::MatrixXf Sk = S.head(k).asDiagonal();        // [k x k]
    Eigen::MatrixXf Y = Sk * Vt.topRows(k);             // [k x views]

    // === Output ===

    // Mean image
    cv::Mat meanImgFloat(numPixels, 1, CV_32F);
    for (int i = 0; i < numPixels; ++i)
        meanImgFloat.at<float>(i, 0) = meanVec(i);
    meanImageOut = meanImgFloat.reshape(1, 0); // still flattened

    // Basis images
    basisImagesOut.clear();
    for (int i = 0; i < k; ++i) {
        Eigen::VectorXf basisVec = B.col(i);
        cv::Mat basisImg(numPixels, 1, CV_32F);
        for (int j = 0; j < numPixels; ++j)
            basisImg.at<float>(j, 0) = basisVec(j);

        basisImagesOut.push_back(basisImg.clone().reshape(1, 0)); // flattened
    }

    // Coefficients matrix
    cv::eigen2cv(Y, coefficientMatrixOut);  // [k x views], CV_32F

    return true;
}

// Raw SVD output is unusable as a Unity texture (or even an image), needs to be processed a bit
void PostprocessSVDOutput(
    std::vector<cv::Mat>& basisImages,  // [flattened float32], modified in-place
    cv::Mat& meanImage,                 // [flattened float32], modified in-place
    cv::Mat& coefficients,              // [k x N float32], modified in-place
    int imageHeight,
    int imageWidth
)
{
    // === Compute max absolute value across all basis vectors ===
    float maxAbs = 0.0f;
    for (const auto& basis : basisImages) {
        double minVal, maxVal;
        cv::minMaxLoc(cv::abs(basis), &minVal, &maxVal);
        maxAbs = std::max(maxAbs, static_cast<float>(maxVal));
    }

    if (maxAbs < 1e-6f) {
        std::cerr << "[PostprocessSVD] Warning: maxAbs nearly zero — skipping normalization.\n";
        maxAbs = 1.0f;
    }

    float basisScale = 127.0f / maxAbs;
    float coeffScale = maxAbs / 127.0f;

    // === Normalize basis vectors and reshape ===
    for (auto& basis : basisImages) {
        basis = basis * basisScale;                       // float32 in [-127,127]
        basis = basis.reshape(1, imageHeight);            // reshape to H×W
        basis += 127.0f;                                  // shift to [0,254]
        cv::threshold(basis, basis, 255.0, 255.0, cv::THRESH_TRUNC); // clip
        basis.convertTo(basis, CV_8U);                    // convert in-place to uint8
    }

    // === Normalize and reshape mean image for viewing ===
    meanImage = meanImage.reshape(1, imageHeight);        // float32 [H×W]
    cv::normalize(meanImage, meanImage, 0.0, 255.0, cv::NORM_MINMAX);
    meanImage.convertTo(meanImage, CV_8U);                // uint8 [H×W]

    // === Rescale coefficients to match new basis ===
    coefficients *= coeffScale;                           // float32 [k x N]
}
