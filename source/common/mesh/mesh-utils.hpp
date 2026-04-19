#pragma once

#include "mesh.hpp"
#include <string>

namespace our::mesh_utils {
    // Load an ".obj" file into the mesh
    Mesh* loadOBJ(const std::string& filename);
    // Create a sphere (the vertex order in the triangles are CCW from the outside)
    // Segments define the number of divisions on the both the latitude and the longitude
    Mesh* sphere(const glm::ivec2& segments);

    // Generates a terrain mesh from a grayscale heightmap image.
    // width/depth = number of quads along each axis
    // heightScale = maximum world-space height of the terrain
    // uvScale = how many times textures tile across the terrain
    Mesh* heightmap(
        const std::string& heightmapPath,
        int width = 128,
        int depth = 128,
        float heightScale = 20.0f,
        float worldScale = 100.0f,
        float uvScale = 16.0f
    );
 
    // Generates a flat cloud quad (billboard or flat plane) for instanced clouds
    Mesh* cloudPlane(int segments = 4);
}