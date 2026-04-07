#include "entity.hpp"
#include "../deserialize-utils.hpp"

#include <glm/gtx/euler_angles.hpp>

namespace our {

    // This function computes and returns a matrix that represents this transform
    glm::mat4 Transform::toMat4() const {
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
        glm::mat4 rotMat = glm::yawPitchRoll(rotation.y, rotation.x, rotation.z);
        glm::mat4 transMat = glm::translate(glm::mat4(1.0f), position);
        return transMat * rotMat * scaleMat;
    }

     // Deserializes the entity data and components from a json object
    void Transform::deserialize(const nlohmann::json& data){
        position = data.value("position", position);
        rotation = glm::radians(data.value("rotation", glm::degrees(rotation)));
        scale    = data.value("scale", scale);
    }

}