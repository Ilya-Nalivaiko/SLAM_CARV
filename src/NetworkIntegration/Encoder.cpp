// Encodes given geometry to GLTF to prepare for send
// Ilya Nalivaiko 2025


#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_ENABLE_FS   // <-- enable file system utilities

#include "Encoder.h"

// Shared mesh encoding logic (helper function)
// Encodes the geometry data [chunk] into gltf file [model]
// Specify [materialIndex] if using several
void populateBasicModelFromChunk(tinygltf::Model& model, const GeometryChunk& chunk, int materialIndex) {
    model.asset.version = "2.0";
    model.defaultScene = 0;

    model.scenes.push_back({});
    model.scenes[0].nodes = {0};
    model.nodes.push_back({}); model.nodes[0].mesh = 0;
    model.meshes.push_back({});

    // Flatten vertex position and UV data
    std::vector<float> pos;
    for (auto& v : chunk.vertices) {
        pos.push_back(v.x); pos.push_back(v.y); pos.push_back(v.z);
    }

    std::vector<float> uv;
    bool hasUVs = chunk.uvs.size() == chunk.vertices.size();
    if (hasUVs) {
        for (auto& t : chunk.uvs) {
            uv.push_back(t.u); uv.push_back(t.v);
        }
    }

    std::vector<unsigned short> idx(pos.size() / 3);
    for (size_t i = 0; i < idx.size(); ++i) idx[i] = (unsigned short)i;

    // Allocate combined buffer (positions + indices + uvs)
    size_t offsetPos = 0;
    size_t offsetIdx = pos.size() * sizeof(float);
    size_t offsetUV = offsetIdx + idx.size() * sizeof(unsigned short);
    size_t totalSize = offsetUV + (hasUVs ? uv.size() * sizeof(float) : 0);

    tinygltf::Buffer rawBuf;
    rawBuf.data.resize(totalSize);
    memcpy(rawBuf.data.data() + offsetPos, pos.data(), pos.size() * sizeof(float));
    memcpy(rawBuf.data.data() + offsetIdx, idx.data(), idx.size() * sizeof(unsigned short));
    if (hasUVs) {
        memcpy(rawBuf.data.data() + offsetUV, uv.data(), uv.size() * sizeof(float));
    }
    model.buffers.push_back(std::move(rawBuf));

    // BufferView: Positions
    tinygltf::BufferView bvPos{};
    bvPos.buffer = 0;
    bvPos.byteOffset = offsetPos;
    bvPos.byteLength = pos.size() * sizeof(float);
    bvPos.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(bvPos);
    int bvIdxPos = model.bufferViews.size() - 1;

    // BufferView: Indices
    tinygltf::BufferView bvIdx{};
    bvIdx.buffer = 0;
    bvIdx.byteOffset = offsetIdx;
    bvIdx.byteLength = idx.size() * sizeof(unsigned short);
    bvIdx.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
    model.bufferViews.push_back(bvIdx);
    int bvIdxIdx = model.bufferViews.size() - 1;

    // BufferView: UVs (if any)
    int bvIdxUV = -1;
    if (hasUVs) {
        tinygltf::BufferView bvUV{};
        bvUV.buffer = 0;
        bvUV.byteOffset = offsetUV;
        bvUV.byteLength = uv.size() * sizeof(float);
        bvUV.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        model.bufferViews.push_back(bvUV);
        bvIdxUV = model.bufferViews.size() - 1;
    }

    // Accessor: Positions
    tinygltf::Accessor ap{};
    ap.bufferView = bvIdxPos;
    ap.byteOffset = 0;
    ap.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    ap.count = pos.size() / 3;
    ap.type = TINYGLTF_TYPE_VEC3;
    model.accessors.push_back(ap);
    int accessorIdxPos = model.accessors.size() - 1;

    // Accessor: Indices
    tinygltf::Accessor ai{};
    ai.bufferView = bvIdxIdx;
    ai.byteOffset = 0;
    ai.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
    ai.count = idx.size();
    ai.type = TINYGLTF_TYPE_SCALAR;
    model.accessors.push_back(ai);
    int accessorIdxIdx = model.accessors.size() - 1;

    // Accessor: UVs
    int accessorIdxUV = -1;
    if (hasUVs) {
        tinygltf::Accessor auv{};
        auv.bufferView = bvIdxUV;
        auv.byteOffset = 0;
        auv.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        auv.count = uv.size() / 2;
        auv.type = TINYGLTF_TYPE_VEC2;
        model.accessors.push_back(auv);
        accessorIdxUV = model.accessors.size() - 1;
    }

    // Primitive with material and attributes
    tinygltf::Primitive prim{};
    prim.attributes["POSITION"] = accessorIdxPos;
    if (accessorIdxUV >= 0)
        prim.attributes["TEXCOORD_0"] = accessorIdxUV;
    prim.indices = accessorIdxIdx;
    prim.mode = TINYGLTF_MODE_TRIANGLES;
    if (materialIndex >= 0)
        prim.material = materialIndex;

    model.meshes[0].primitives.push_back(prim);
}

// Encode geometry without texture
std::string encodeToGlTF(const GeometryChunk& chunk) {
    tinygltf::Model model;
    populateBasicModelFromChunk(model, chunk, -1);

    tinygltf::TinyGLTF gltfCtx;
    std::stringstream ss;
    if (!gltfCtx.WriteGltfSceneToStream(&model, ss, false, false)) {
        throw std::runtime_error("Failed to write glTF to string stream.");
    }
    return ss.str();
}

// Encode with texture URL list
// First material will be used as object's only diffuse map
// Later materials are for use in multitexturing with the Unity app
std::string encodeToGltfWithTex(const GeometryChunk& chunk, const std::vector<std::string>& textureUrls) {
    tinygltf::Model model;

    if (textureUrls.empty()) {
        throw std::runtime_error("encodeToGltfWithTex: textureUrls is empty. Use encodeToGlTF if no texture is associated");
    }

    // Add all images
    for (const auto& url : textureUrls) {
        tinygltf::Image img;
        img.uri = url;
        model.images.push_back(std::move(img));
        std::cerr << "Texture URL used in GLTF: " << url << "\n";
    }

    // Texture: Use the first one as the formal PBR texture
    tinygltf::Texture tex;
    tex.source = 0;
    model.textures.push_back(std::move(tex));

    // Material
    tinygltf::Material mat;
    mat.pbrMetallicRoughness.baseColorTexture.index = 0;
    model.materials.push_back(std::move(mat));

    // Mesh
    populateBasicModelFromChunk(model, chunk, 0);

    // Serialize
    tinygltf::TinyGLTF gltfCtx;
    std::stringstream ss;
    if (!gltfCtx.WriteGltfSceneToStream(&model, ss, false, false)) {
        throw std::runtime_error("Failed to write glTF to string stream.");
    }
    return ss.str();
}

// Adds images to an existing GLTF file (for Blender tests)
void addImagesToGltf(json& gltf_json, const std::vector<std::string>& textureUrls) {
    if (textureUrls.empty()) {
        std::cerr << "[HTTP/GLTF] WARNING: textureUrls is empty, no images will be added.\n";
        return;
    }

    // Add images
    for (const auto& url : textureUrls) {
        json image_entry;
        image_entry["uri"] = url;
        gltf_json["images"].push_back(image_entry);
        std::cerr << "[HTTP/GLTF] Texture URL added: " << url << "\n";
    }

    // Add textures — 1 per image
    for (size_t i = 0; i < textureUrls.size(); ++i) {
        json tex_entry;
        tex_entry["source"] = static_cast<int>(i);
        gltf_json["textures"].push_back(tex_entry);
    }

    // Add material — reference first texture
    json mat_entry;
    mat_entry["pbrMetallicRoughness"]["baseColorTexture"]["index"] = 0;
    gltf_json["materials"].push_back(mat_entry);

    std::cerr << "[HTTP/GLTF] Materials and textures added.\n";
}




std::string encodePointsTrisToGltfWithTex(
    const std::vector<dlovi::Matrix>& points,
    const std::list<dlovi::Matrix>& tris,
    const std::vector<std::string>& textureUrls)
{

    //TODO there is an error in this function

    // terminate called after throwing an instance of 'std::logic_error'
    // what():  basic_string::_M_construct null not valid

    // ./run.sh: line 3:   498 Aborted (core dumped)
    // rosrun ORB_CARV_Pub Mono Vocabulary/ORBvoc.txt config_files/Logitech_c270_HD720p.yaml 192.168.1.75 8080 192.168.1.75 5555

    std::cout << "[ePTtGwT_DEBUG] Function called" << std::endl;

    tinygltf::Model model;
    model.asset.version = "2.0";
    model.defaultScene = 0;
    model.scenes.push_back({});
    model.scenes[0].nodes = {0};
    model.nodes.push_back({});
    model.nodes[0].mesh = 0;
    model.meshes.push_back({});

    std::cout << "[ePTtGwT_DEBUG] model instantiated" << std::endl;

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

    std::cout << "[ePTtGwT_DEBUG] data flattened" << std::endl;

    // Construct buffer
    size_t offsetPos = 0;
    size_t offsetIdx = pos.size() * sizeof(float);
    size_t totalSize = offsetIdx + idx.size() * sizeof(unsigned short);
    tinygltf::Buffer rawBuf;
    rawBuf.data.resize(totalSize);
    memcpy(rawBuf.data.data() + offsetPos, pos.data(), pos.size() * sizeof(float));
    memcpy(rawBuf.data.data() + offsetIdx, idx.data(), idx.size() * sizeof(unsigned short));
    model.buffers.push_back(std::move(rawBuf));

    std::cout << "[ePTtGwT_DEBUG] buffers constructed" << std::endl;

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

    std::cout << "[ePTtGwT_DEBUG] buffers populated" << std::endl;

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

    std::cout << "[ePTtGwT_DEBUG] accessors" << std::endl;

    // Textures
    if (!textureUrls.empty()) {
        for (auto& url : textureUrls) {
            tinygltf::Image img; img.uri = url; model.images.push_back(std::move(img));
        }
        tinygltf::Texture tex; tex.source = 0; model.textures.push_back(std::move(tex));
        tinygltf::Material mat; mat.pbrMetallicRoughness.baseColorTexture.index = 0; model.materials.push_back(std::move(mat));
    }

    std::cout << "[ePTtGwT_DEBUG] tex" << std::endl;

    // Primitive
    tinygltf::Primitive prim;
    prim.attributes["POSITION"] = 0;
    prim.indices = 1;
    prim.mode = TINYGLTF_MODE_TRIANGLES;
    if (!textureUrls.empty()) prim.material = 0;
    model.meshes[0].primitives.push_back(prim);

    std::cout << "[ePTtGwT_DEBUG] prim" << std::endl;

    // Serialize
    tinygltf::TinyGLTF gltfCtx;
    std::stringstream ss;
    if (!gltfCtx.WriteGltfSceneToStream(&model, ss, false, false)) {
        throw std::runtime_error("Failed to write glTF to string stream.");
    }

    std::cout << "[ePTtGwT_DEBUG] serialized" << std::endl;

    return ss.str();
}
