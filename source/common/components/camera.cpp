#include "camera.hpp"
#include "../ecs/entity.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 

namespace our {
    // Reads camera parameters from the given json object
    void CameraComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;
        std::string cameraTypeStr = data.value("cameraType", "perspective");
        if(cameraTypeStr == "orthographic"){
            cameraType = CameraType::ORTHOGRAPHIC;
        } else {
            cameraType = CameraType::PERSPECTIVE;
        }
        near = data.value("near", 0.01f);
        far = data.value("far", 100.0f);
        fovY = data.value("fovY", 90.0f) * (glm::pi<float>() / 180);
        orthoHeight = data.value("orthoHeight", 1.0f);
    }

    // Creates and returns the camera view matrix
    glm::mat4 CameraComponent::getViewMatrix() const {
        auto owner = getOwner();
        auto M = owner->getLocalToWorldMatrix();
        glm::vec3 eye = glm::vec3(M[3]);
        glm::vec3 center = glm::vec3(M[3]) - glm::vec3(M[2]);
        glm::vec3 up = glm::vec3(M[1]);
        return glm::lookAt(eye, center, up);
    }

    // Creates and returns the camera projection matrix
    // "viewportSize" is used to compute the aspect ratio
    glm::mat4 CameraComponent::getProjectionMatrix(glm::ivec2 viewportSize) const {
        float aspect = (float)viewportSize.x / (float)viewportSize.y;
        if(cameraType == CameraType::ORTHOGRAPHIC){
            float top = orthoHeight / 2.0f;
            float bottom = -top;
            float right = top * aspect;
            float left = -right;
            return glm::ortho(left, right, bottom, top, near, far);
        } else {
            return glm::perspective(fovY, aspect, near, far);
        }
    }
}