#pragma once

#include "../ecs/world.hpp"
#include "../components/camera.hpp"
#include "../components/free-camera-controller.hpp"
#include "audio-system.hpp"

#include "../application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>

namespace our
{

    // The free camera controller system is responsible for moving every entity which contains a FreeCameraControllerComponent.
    // This system is added as a slightly complex example for how use the ECS framework to implement logic.
    // For more information, see "common/components/free-camera-controller.hpp"
    class FreeCameraControllerSystem
    {
        Application *app;          // The application in which the state runs
        bool mouse_locked = false; // Is the mouse locked
        AudioSystem *audioSystem = nullptr;
        bool shiftWasPressed = false; // Tracks previous frame's shift state for edge detection

    public:
        // When a state enters, it should call this function and give it the pointer to the application
        void enter(Application *app)
        {
            this->app = app;
        }

        void setAudioSystem(AudioSystem *audio)
        {
            audioSystem = audio;
        }

        // This should be called every frame to update all entities containing a FreeCameraControllerComponent
        void update(World *world, float deltaTime)
        {
            // First of all, we search for an entity containing both a CameraComponent and a FreeCameraControllerComponent
            // As soon as we find one, we break
            CameraComponent *camera = nullptr;
            FreeCameraControllerComponent *controller = nullptr;
            for (auto entity : world->getEntities())
            {
                camera = entity->getComponent<CameraComponent>();
                controller = entity->getComponent<FreeCameraControllerComponent>();
                if (camera && controller)
                    break;
            }
            // If there is no entity with both a CameraComponent and a FreeCameraControllerComponent, we can do nothing so we return
            if (!(camera && controller))
                return;
            // Get the entity that we found via getOwner of camera (we could use controller->getOwner())
            Entity *entity = camera->getOwner();

            // If the left mouse button is pressed, we lock and hide the mouse. This common in First Person Games.
            if (app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1) && !mouse_locked)
            {
                app->getMouse().lockMouse(app->getWindow());
                mouse_locked = true;
                // If the left mouse button is released, we unlock and unhide the mouse.
            }
            else if (!app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1) && mouse_locked)
            {
                app->getMouse().unlockMouse(app->getWindow());
                mouse_locked = false;
            }

            // We get a reference to the entity's position and rotation
            glm::vec3 &position = entity->localTransform.position;
            glm::vec3 &rotation = entity->localTransform.rotation;

            // If the left mouse button is pressed, we get the change in the mouse location
            // and use it to update the camera rotation
            if (app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1))
            {
                glm::vec2 delta = app->getMouse().getMouseDelta();
                rotation.x -= delta.y * controller->rotationSensitivity; // The y-axis controls the pitch
                rotation.y -= delta.x * controller->rotationSensitivity; // The x-axis controls the yaw
            }

            // Remove pitch lock so the plane can easily perform 360 vertical loops
            rotation.x = glm::wrapAngle(rotation.x);
            rotation.y = glm::wrapAngle(rotation.y);

            // We update the camera fov based on the mouse wheel scrolling amount
            float fov = camera->fovY + app->getMouse().getScrollOffset().y * controller->fovSensitivity;
            fov = glm::clamp(fov, glm::pi<float>() * 0.01f, glm::pi<float>() * 0.99f); // We keep the fov in the range 0.01*PI to 0.99*PI
            camera->fovY = fov;

            // We get the camera model matrix (relative to its parent) to compute the front, up and right directions
            glm::mat4 matrix = entity->localTransform.toMat4();

            glm::vec3 front = glm::vec3(matrix * glm::vec4(0, 0, -1, 0)),
                      up = glm::vec3(matrix * glm::vec4(0, 1, 0, 0)),
                      right = glm::vec3(matrix * glm::vec4(1, 0, 0, 0));

            glm::vec3 current_sensitivity = controller->positionSensitivity *  controller->speedupFactor;
            // If the LEFT SHIFT key is pressed, we multiply the position sensitivity by the speed up factor
            bool shiftPressed = app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT);
           if(app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT)) {
                controller->speedupFactor = 3.0f;
                controller->tiltingSensitivity = 0.6f;
            } else if(app->getKeyboard().isPressed(GLFW_KEY_LEFT_CONTROL)) {
                controller->speedupFactor = 0.5f;
                controller->tiltingSensitivity = 0.1f;
            } else {
                controller->speedupFactor = 1.0f;
                controller->tiltingSensitivity = 0.2f;
            }

            // Start/stop motor sound on shift press/release edges
            if (audioSystem)
            {
                if (shiftPressed && !shiftWasPressed)
                    audioSystem->startMotorSound("assets/sounds/motor-speed2.mp3", 0.9f);
                else if (!shiftPressed && shiftWasPressed)
                    audioSystem->stopMotorSound();
            }
            shiftWasPressed = shiftPressed;

            // Moves the player forward automatically
            position += front * (deltaTime * current_sensitivity.z);

            // S & W moves the player pitch back and forth
            float max_pitch = glm::pi<float>() / 3.0f; // Limit tilt to 60 degrees
            float pitch_speed = controller->tiltingSensitivity; // How fast it tilts
            
            if(app->getKeyboard().isPressed(GLFW_KEY_W)) {
                rotation.x += (deltaTime * pitch_speed) * glm::cos(rotation.z); 
                rotation.y += (deltaTime * pitch_speed) * glm::sin(rotation.z);
            }
            else if (app->getKeyboard().isPressed(GLFW_KEY_S))
            {
                rotation.x -= (deltaTime * pitch_speed) * glm::cos(rotation.z);
                rotation.y -= (deltaTime * pitch_speed) * glm::sin(rotation.z);
            }
            else
            {
                float falling_speed = pitch_speed * 0.5f;
                if (glm::cos(rotation.x) > 0.0f)
                {
                    rotation.x -= (deltaTime * falling_speed) * glm::cos(rotation.z);
                    rotation.y -= (deltaTime * falling_speed) * glm::sin(rotation.z);
                }
                else
                {
                    rotation.x += (deltaTime * falling_speed) * glm::cos(rotation.z);
                    rotation.y += (deltaTime * falling_speed) * glm::sin(rotation.z);
                }
            }

            // A & D tilts & rotates the player left or right 
            float max_roll = glm::pi<float>(); // Limit tilt to 45 degrees
            float roll_speed = controller->tiltingSensitivity; // How fast it tilts
            bool turning = false;
            if (app->getKeyboard().isPressed(GLFW_KEY_D))
            {
                rotation.y -= (deltaTime * roll_speed);
                if (rotation.z > -max_roll)
                    rotation.z -= deltaTime * roll_speed; // Tilt right
                turning = true;
            }
            if (app->getKeyboard().isPressed(GLFW_KEY_A))
            {
                rotation.y += (deltaTime * roll_speed);
                if (rotation.z < max_roll)
                    rotation.z += deltaTime * roll_speed; // Tilt left
                turning = true;
            }

            // If we are not steering, restore the tilt back to 0
            if (!turning)
            {
                if (rotation.z > 0.0f)
                {
                    rotation.z -= deltaTime * roll_speed;
                    if (rotation.z < 0.0f)
                        rotation.z = 0.0f;
                }
                else if (rotation.z < 0.0f)
                {
                    rotation.z += deltaTime * roll_speed;
                    if (rotation.z > 0.0f)
                        rotation.z = 0.0f;
                }
            }
        }

        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit()
        {
            if (mouse_locked)
            {
                mouse_locked = false;
                app->getMouse().unlockMouse(app->getWindow());
            }
            if (audioSystem && shiftWasPressed)
                audioSystem->stopMotorSound();
            shiftWasPressed = false;
        }
    };

}
