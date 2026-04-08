#include "mesh-renderer.hpp"
#include "../asset-loader.hpp"

namespace our {
    // Receives the mesh & material from the AssetLoader by the names given in the json object
    void MeshRendererComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;
        mesh = AssetLoader<Mesh>::get(data["mesh"].get<std::string>());
        material = AssetLoader<Material>::get(data["material"].get<std::string>());
    }
}