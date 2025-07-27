// Encodes given geometry to GLTF to prepare for send
// Ilya Nalivaiko 2025


#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_ENABLE_FS   // <-- enable file system utilities

#include "Encoder.h"

// // Shared mesh encoding logic (helper function)
// // Encodes the geometry data [chunk] into gltf file [model]
// // Specify [materialIndex] if using several
// void populateBasicModelFromChunk(tinygltf::Model& model, const GeometryChunk& chunk, int materialIndex) {
//     model.asset.version = "2.0";
//     model.defaultScene = 0;

//     model.scenes.push_back({});
//     model.scenes[0].nodes = {0};
//     model.nodes.push_back({}); model.nodes[0].mesh = 0;
//     model.meshes.push_back({});

//     // Flatten vertex position and UV data
//     std::vector<float> pos;
//     for (auto& v : chunk.vertices) {
//         pos.push_back(v.x); pos.push_back(v.y); pos.push_back(v.z);
//     }

//     std::vector<float> uv;
//     bool hasUVs = chunk.uvs.size() == chunk.vertices.size();
//     if (hasUVs) {
//         for (auto& t : chunk.uvs) {
//             uv.push_back(t.u); uv.push_back(t.v);
//         }
//     }

//     std::vector<unsigned short> idx(pos.size() / 3);
//     for (size_t i = 0; i < idx.size(); ++i) idx[i] = (unsigned short)i;

//     // Allocate combined buffer (positions + indices + uvs)
//     size_t offsetPos = 0;
//     size_t offsetIdx = pos.size() * sizeof(float);
//     size_t offsetUV = offsetIdx + idx.size() * sizeof(unsigned short);
//     size_t totalSize = offsetUV + (hasUVs ? uv.size() * sizeof(float) : 0);

//     tinygltf::Buffer rawBuf;
//     rawBuf.data.resize(totalSize);
//     memcpy(rawBuf.data.data() + offsetPos, pos.data(), pos.size() * sizeof(float));
//     memcpy(rawBuf.data.data() + offsetIdx, idx.data(), idx.size() * sizeof(unsigned short));
//     if (hasUVs) {
//         memcpy(rawBuf.data.data() + offsetUV, uv.data(), uv.size() * sizeof(float));
//     }
//     model.buffers.push_back(std::move(rawBuf));

//     // BufferView: Positions
//     tinygltf::BufferView bvPos{};
//     bvPos.buffer = 0;
//     bvPos.byteOffset = offsetPos;
//     bvPos.byteLength = pos.size() * sizeof(float);
//     bvPos.target = TINYGLTF_TARGET_ARRAY_BUFFER;
//     model.bufferViews.push_back(bvPos);
//     int bvIdxPos = model.bufferViews.size() - 1;

//     // BufferView: Indices
//     tinygltf::BufferView bvIdx{};
//     bvIdx.buffer = 0;
//     bvIdx.byteOffset = offsetIdx;
//     bvIdx.byteLength = idx.size() * sizeof(unsigned short);
//     bvIdx.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
//     model.bufferViews.push_back(bvIdx);
//     int bvIdxIdx = model.bufferViews.size() - 1;

//     // BufferView: UVs (if any)
//     int bvIdxUV = -1;
//     if (hasUVs) {
//         tinygltf::BufferView bvUV{};
//         bvUV.buffer = 0;
//         bvUV.byteOffset = offsetUV;
//         bvUV.byteLength = uv.size() * sizeof(float);
//         bvUV.target = TINYGLTF_TARGET_ARRAY_BUFFER;
//         model.bufferViews.push_back(bvUV);
//         bvIdxUV = model.bufferViews.size() - 1;
//     }

//     // Accessor: Positions
//     tinygltf::Accessor ap{};
//     ap.bufferView = bvIdxPos;
//     ap.byteOffset = 0;
//     ap.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
//     ap.count = pos.size() / 3;
//     ap.type = TINYGLTF_TYPE_VEC3;
//     model.accessors.push_back(ap);
//     int accessorIdxPos = model.accessors.size() - 1;

//     // Accessor: Indices
//     tinygltf::Accessor ai{};
//     ai.bufferView = bvIdxIdx;
//     ai.byteOffset = 0;
//     ai.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
//     ai.count = idx.size();
//     ai.type = TINYGLTF_TYPE_SCALAR;
//     model.accessors.push_back(ai);
//     int accessorIdxIdx = model.accessors.size() - 1;

//     // Accessor: UVs
//     int accessorIdxUV = -1;
//     if (hasUVs) {
//         tinygltf::Accessor auv{};
//         auv.bufferView = bvIdxUV;
//         auv.byteOffset = 0;
//         auv.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
//         auv.count = uv.size() / 2;
//         auv.type = TINYGLTF_TYPE_VEC2;
//         model.accessors.push_back(auv);
//         accessorIdxUV = model.accessors.size() - 1;
//     }

//     // Primitive with material and attributes
//     tinygltf::Primitive prim{};
//     prim.attributes["POSITION"] = accessorIdxPos;
//     if (accessorIdxUV >= 0)
//         prim.attributes["TEXCOORD_0"] = accessorIdxUV;
//     prim.indices = accessorIdxIdx;
//     prim.mode = TINYGLTF_MODE_TRIANGLES;
//     if (materialIndex >= 0)
//         prim.material = materialIndex;

//     model.meshes[0].primitives.push_back(prim);
// }

// // Encode geometry without texture
// std::string encodeToGlTF(const GeometryChunk& chunk) {
//     tinygltf::Model model;
//     populateBasicModelFromChunk(model, chunk, -1);

//     tinygltf::TinyGLTF gltfCtx;
//     std::stringstream ss;
//     if (!gltfCtx.WriteGltfSceneToStream(&model, ss, false, false)) {
//         throw std::runtime_error("Failed to write glTF to string stream.");
//     }
//     return ss.str();
// }

// // Encode with texture URL list
// // First material will be used as object's only diffuse map
// // Later materials are for use in multitexturing with the Unity app
// std::string encodeToGltfWithTex(const GeometryChunk& chunk, const std::vector<std::string>& textureUrls) {
//     tinygltf::Model model;

//     if (textureUrls.empty()) {
//         throw std::runtime_error("encodeToGltfWithTex: textureUrls is empty. Use encodeToGlTF if no texture is associated");
//     }

//     // Add all images
//     for (const auto& url : textureUrls) {
//         tinygltf::Image img;
//         img.uri = url;
//         model.images.push_back(std::move(img));
//         std::cerr << "Texture URL used in GLTF: " << url << "\n";
//     }

//     // Texture: Use the first one as the formal PBR texture
//     tinygltf::Texture tex;
//     tex.source = 0;
//     model.textures.push_back(std::move(tex));

//     // Material
//     tinygltf::Material mat;
//     mat.pbrMetallicRoughness.baseColorTexture.index = 0;
//     model.materials.push_back(std::move(mat));

//     // Mesh
//     populateBasicModelFromChunk(model, chunk, 0);

//     // Serialize
//     tinygltf::TinyGLTF gltfCtx;
//     std::stringstream ss;
//     if (!gltfCtx.WriteGltfSceneToStream(&model, ss, false, false)) {
//         throw std::runtime_error("Failed to write glTF to string stream.");
//     }
//     return ss.str();
// }

// // Adds images to an existing GLTF file (for Blender tests)
// void addImagesToGltf(json& gltf_json, const std::vector<std::string>& textureUrls) {
//     if (textureUrls.empty()) {
//         std::cerr << "[HTTP/GLTF] WARNING: textureUrls is empty, no images will be added.\n";
//         return;
//     }

//     // Add images
//     for (const auto& url : textureUrls) {
//         json image_entry;
//         image_entry["uri"] = url;
//         gltf_json["images"].push_back(image_entry);
//         std::cerr << "[HTTP/GLTF] Texture URL added: " << url << "\n";
//     }

//     // Add textures — 1 per image
//     for (size_t i = 0; i < textureUrls.size(); ++i) {
//         json tex_entry;
//         tex_entry["source"] = static_cast<int>(i);
//         gltf_json["textures"].push_back(tex_entry);
//     }

//     // Add material — reference first texture
//     json mat_entry;
//     mat_entry["pbrMetallicRoughness"]["baseColorTexture"]["index"] = 0;
//     gltf_json["materials"].push_back(mat_entry);

//     std::cerr << "[HTTP/GLTF] Materials and textures added.\n";
// }



// Take the given geometry and textureurls and convert to valid (if somewhat custom) GlTF
std::string encodeToGltf(
    const std::vector<dlovi::Matrix>& points,
    const std::list<dlovi::Matrix>& tris,
    const std::vector<std::string>& textureUrls,
    const nlohmann::json& extras)
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

    // Serialize
    tinygltf::TinyGLTF gltfCtx;
    std::stringstream ss;
    if (!gltfCtx.WriteGltfSceneToStream(&model, ss, false, false)) {
        throw std::runtime_error("Failed to write glTF to string stream.");
    }

    // Attach extras if provided
    if (!extras.is_null()) {

        //both are jsons but god forbid they be compatible
        model.extras = tinygltf::Value(extras.dump());

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


// Convert per-image pose information to JSON
nlohmann::json BuildPoseJson(const std::vector<std::pair<cv::Mat, ORB_SLAM2::TextureFrame>>& rgbTexFrames)
{
    nlohmann::json poseJson;

    for (const auto& [image, tex] : rgbTexFrames) {
        std::string id = std::to_string(tex.mFrameID);

        const cv::Mat& R = tex.mRcw;
        const cv::Mat& t = tex.mtcw;

        std::vector<std::vector<double>> extrinsics = {
            {
                static_cast<double>(R.at<float>(0,0)),
                static_cast<double>(R.at<float>(0,1)),
                static_cast<double>(R.at<float>(0,2)),
                static_cast<double>(t.at<float>(0))
            },
            {
                static_cast<double>(R.at<float>(1,0)),
                static_cast<double>(R.at<float>(1,1)),
                static_cast<double>(R.at<float>(1,2)),
                static_cast<double>(t.at<float>(1))
            },
            {
                static_cast<double>(R.at<float>(2,0)),
                static_cast<double>(R.at<float>(2,1)),
                static_cast<double>(R.at<float>(2,2)),
                static_cast<double>(t.at<float>(2))
            },
            {0.0, 0.0, 0.0, 1.0}
        };

        std::vector<std::vector<double>> intrinsics = {
            {static_cast<double>(tex.mfx), 0.0, static_cast<double>(tex.mcx)},
            {0.0, static_cast<double>(tex.mfy), static_cast<double>(tex.mcy)},
            {0.0, 0.0, 1.0}
        };


        poseJson[id] = {
            {"extrinsics", extrinsics},
            {"intrinsics", intrinsics}
        };
    }

    return poseJson;
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
