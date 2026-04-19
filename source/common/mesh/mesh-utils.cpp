#include "mesh-utils.hpp"

// We will use "Tiny OBJ Loader" to read and process '.obj" files
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobj/tiny_obj_loader.h>

#include <iostream>
#include <vector>
#include <unordered_map>

#include <stb/stb_image.h>
#include <glm/glm.hpp>

our::Mesh *our::mesh_utils::loadOBJ(const std::string &filename)
{

    // The data that we will use to initialize our mesh
    std::vector<our::Vertex> vertices;
    std::vector<GLuint> elements;

    // Since the OBJ can have duplicated vertices, we make them unique using this map
    // The key is the vertex, the value is its index in the vector "vertices".
    // That index will be used to populate the "elements" vector.
    std::unordered_map<our::Vertex, GLuint> vertex_map;

    // The data loaded by Tiny OBJ Loader
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str()))
    {
        std::cerr << "Failed to load obj file \"" << filename << "\" due to error: " << err << std::endl;
        return nullptr;
    }
    if (!warn.empty())
    {
        std::cout << "WARN while loading obj file \"" << filename << "\": " << warn << std::endl;
    }

    // An obj file can have multiple shapes where each shape can have its own material
    // Ideally, we would load each shape into a separate mesh or store the start and end of it in the element buffer to be able to draw each shape separately
    // But we ignored this fact since we don't plan to use multiple materials in the examples
    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {
            Vertex vertex = {};

            // Read the data for a vertex from the "attrib" object
            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]};

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]};

            vertex.tex_coord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                attrib.texcoords[2 * index.texcoord_index + 1]};

            vertex.color = {
                attrib.colors[3 * index.vertex_index + 0] * 255,
                attrib.colors[3 * index.vertex_index + 1] * 255,
                attrib.colors[3 * index.vertex_index + 2] * 255,
                255};

            // See if we already stored a similar vertex
            auto it = vertex_map.find(vertex);
            if (it == vertex_map.end())
            {
                // if no, add it to the vertices and record its index
                auto new_vertex_index = static_cast<GLuint>(vertices.size());
                vertex_map[vertex] = new_vertex_index;
                elements.push_back(new_vertex_index);
                vertices.push_back(vertex);
            }
            else
            {
                // if yes, just add its index in the elements vector
                elements.push_back(it->second);
            }
        }
    }

    return new our::Mesh(vertices, elements);
}

// Create a sphere (the vertex order in the triangles are CCW from the outside)
// Segments define the number of divisions on the both the latitude and the longitude
our::Mesh *our::mesh_utils::sphere(const glm::ivec2 &segments)
{
    std::vector<our::Vertex> vertices;
    std::vector<GLuint> elements;

    // We populate the sphere vertices by looping over its longitude and latitude
    for (int lat = 0; lat <= segments.y; lat++)
    {
        float v = (float)lat / segments.y;
        float pitch = v * glm::pi<float>() - glm::half_pi<float>();
        float cos = glm::cos(pitch), sin = glm::sin(pitch);
        for (int lng = 0; lng <= segments.x; lng++)
        {
            float u = (float)lng / segments.x;
            float yaw = u * glm::two_pi<float>();
            glm::vec3 normal = {cos * glm::cos(yaw), sin, cos * glm::sin(yaw)};
            glm::vec3 position = normal;
            glm::vec2 tex_coords = glm::vec2(u, v);
            our::Color color = our::Color(255, 255, 255, 255);
            vertices.push_back({position, color, tex_coords, normal});
        }
    }

    for (int lat = 1; lat <= segments.y; lat++)
    {
        int start = lat * (segments.x + 1);
        for (int lng = 1; lng <= segments.x; lng++)
        {
            int prev_lng = lng - 1;
            elements.push_back(lng + start);
            elements.push_back(lng + start - segments.x - 1);
            elements.push_back(prev_lng + start - segments.x - 1);
            elements.push_back(prev_lng + start - segments.x - 1);
            elements.push_back(prev_lng + start);
            elements.push_back(lng + start);
        }
    }

    return new our::Mesh(vertices, elements);
}

our::Mesh *our::mesh_utils::heightmap(
    const std::string &heightmapPath,
    int width,
    int depth,
    float heightScale,
    float worldScale,
    float uvScale)
{
    // ----------------------------------------------------------------
    // 1. Load the grayscale heightmap image
    // ----------------------------------------------------------------
    int imgW, imgH, channels;
    unsigned char *data = stbi_load(heightmapPath.c_str(), &imgW, &imgH, &channels, 1);

    bool usedFallback = false;
    if (!data)
    {
        std::cerr << "[Heightmap] FAILED to load: " << heightmapPath << " — using fallback\n";
        imgW = imgH = 256;
        data = new unsigned char[imgW * imgH];
        usedFallback = true;
        for (int z = 0; z < imgH; z++)
        {
            for (int x = 0; x < imgW; x++)
            {
                float fx = (float)x / imgW;
                float fz = (float)z / imgH;
                float h = 0.5f + 0.25f * sinf(fx * 6.28f * 3.0f) * sinf(fz * 6.28f * 3.0f) + 0.15f * sinf(fx * 6.28f * 7.0f + 1.0f) + 0.10f * sinf(fz * 6.28f * 5.0f);
                h = glm::clamp(h, 0.0f, 1.0f);
                data[z * imgW + x] = static_cast<unsigned char>(h * 255.0f);
            }
        }
    }

    // Helper: sample raw pixel → float [0,1]
    auto sampleHeight = [&](int ix, int iz) -> float
    {
        ix = glm::clamp(ix, 0, imgW - 1);
        iz = glm::clamp(iz, 0, imgH - 1);
        return static_cast<float>(data[iz * imgW + ix]) / 255.0f;
    };

    int numVertsX = width + 1;
    int numVertsZ = depth + 1;

    // ----------------------------------------------------------------
    // 2. Pass 1 — find min/max to normalize the full range to [0,1]
    // ----------------------------------------------------------------
    float minH = 1.0f, maxH = 0.0f;
    for (int iz = 0; iz < numVertsZ; iz++)
    {
        for (int ix = 0; ix < numVertsX; ix++)
        {
            int hmX = static_cast<int>((float)ix / width * (imgW - 1));
            int hmZ = static_cast<int>((float)iz / depth * (imgH - 1));
            float h = sampleHeight(hmX, hmZ);
            minH = std::min(minH, h);
            maxH = std::max(maxH, h);
        }
    }
    float hRange = (maxH - minH) > 0.0001f ? (maxH - minH) : 1.0f;
    std::cout << "[Heightmap] " << (usedFallback ? "(fallback) " : "")
              << imgW << "x" << imgH
              << "  raw h: " << minH << " -> " << maxH
              << "  world Y: 0 -> " << heightScale << "\n";

    // ----------------------------------------------------------------
    // 3. Pass 2 — build vertices
    //    hNorm is always [0,1] regardless of source image brightness.
    //    world_pos.y = hNorm * heightScale → [0, heightScale]
    //    The fragment shader recovers h = world_pos.y / heightScale
    //    giving full float precision — zero stepping artifacts.
    // ----------------------------------------------------------------
    std::vector<our::Vertex> vertices;
    vertices.reserve(numVertsX * numVertsZ);

    for (int iz = 0; iz < numVertsZ; iz++)
    {
        for (int ix = 0; ix < numVertsX; ix++)
        {

            int hmX = static_cast<int>((float)ix / width * (imgW - 1));
            int hmZ = static_cast<int>((float)iz / depth * (imgH - 1));

            float hNorm = (sampleHeight(hmX, hmZ) - minH) / hRange;

            float wx = ((float)ix / width - 0.5f) * worldScale;
            float wy = hNorm * heightScale; // 0=valley → heightScale=peak
            float wz = ((float)iz / depth - 0.5f) * worldScale;

            // Central-difference normals on normalized heights
            float hL = (sampleHeight(hmX - 1, hmZ) - minH) / hRange;
            float hR = (sampleHeight(hmX + 1, hmZ) - minH) / hRange;
            float hD = (sampleHeight(hmX, hmZ - 1) - minH) / hRange;
            float hU = (sampleHeight(hmX, hmZ + 1) - minH) / hRange;

            float step = worldScale / (float)width;
            glm::vec3 normal = glm::normalize(glm::vec3(
                (hL - hR) * heightScale,
                2.0f * step,
                (hD - hU) * heightScale));

            float u = ((float)ix / width) * uvScale;
            float v = ((float)iz / depth) * uvScale;

            our::Vertex vert;
            vert.position = glm::vec3(wx, wy, wz);
            vert.normal = normal;
            vert.tex_coord = glm::vec2(u, v);
            vert.color = glm::u8vec4(255, 255, 255, 255); // not used by terrain shader
            vertices.push_back(vert);
        }
    }

    // ----------------------------------------------------------------
    // 4. Indices — two triangles per quad
    // ----------------------------------------------------------------
    std::vector<GLuint> indices;
    indices.reserve(width * depth * 6);

    for (int iz = 0; iz < depth; iz++)
    {
        for (int ix = 0; ix < width; ix++)
        {
            GLuint tl = iz * numVertsX + ix;
            GLuint tr = tl + 1;
            GLuint bl = tl + numVertsX;
            GLuint br = bl + 1;
            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);
            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }

    if (usedFallback)
        delete[] data;
    else
        stbi_image_free(data);

    // Find actual world Y range of generated vertices
    float minY = FLT_MAX, maxY = -FLT_MAX;
    for (auto &v : vertices)
    {
        minY = std::min(minY, v.position.y);
        maxY = std::max(maxY, v.position.y);
    }
    std::cout << "[Heightmap] world Y range: " << minY << " -> " << maxY << "\n";

    return new our::Mesh(vertices, indices);
}

// ----------------------------------------------------------------
// Cloud plane: a simple flat grid mesh (billboarded in the shader)
// ----------------------------------------------------------------
our::Mesh *our::mesh_utils::cloudPlane(int segments)
{
    std::vector<our::Vertex> vertices;
    std::vector<GLuint> indices;

    int n = segments + 1;
    for (int iz = 0; iz < n; iz++)
    {
        for (int ix = 0; ix < n; ix++)
        {
            float u = (float)ix / segments;
            float v = (float)iz / segments;
            our::Vertex vert;
            vert.position = glm::vec3(u - 0.5f, 0.0f, v - 0.5f);
            vert.normal = glm::vec3(0, 1, 0);
            vert.tex_coord = glm::vec2(u, v);
            vert.color = glm::vec4(1.0f);
            vertices.push_back(vert);
        }
    }
    for (int iz = 0; iz < segments; iz++)
    {
        for (int ix = 0; ix < segments; ix++)
        {
            GLuint tl = iz * n + ix;
            GLuint tr = tl + 1;
            GLuint bl = tl + n;
            GLuint br = bl + 1;
            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);
            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }
    return new our::Mesh(vertices, indices);
}