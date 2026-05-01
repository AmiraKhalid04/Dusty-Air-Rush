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
#include <glm/gtx/euler_angles.hpp>

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
        bool inEmote = false;
        float emoteProgress = 0.0f;
        float initialEmotePitch = 0.0f;

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

            GLFWgamepadstate gamepadState;
            bool gamepadActive = false;
            int activeJoystick = -1;
            
            /* Check for connected joysticks/gamepads. We prioritize gamepads recognized by GLFW's 
            Gamepad API, but will also attempt to read axes/buttons from generic joysticks if no gamepad is found.*/
            for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_16; ++i) {
                if (glfwJoystickPresent(i)) {
                    activeJoystick = i;
                    if (glfwJoystickIsGamepad(i) && glfwGetGamepadState(i, &gamepadState)) {
                        gamepadActive = true;
                    }
                    break;
                }
            }

            float rawLeftX = 0, rawLeftY = 0, rawRightX = 0, rawRightY = 0;
            float rawL2 = -1.0f, rawR2 = -1.0f;
            bool rawL1 = false, rawR1 = false;

            if (activeJoystick != -1) {
                if (gamepadActive) {
                    rawLeftX = gamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
                    rawLeftY = gamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
                    rawRightX = gamepadState.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
                    rawRightY = gamepadState.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
                    rawL2 = gamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
                    rawR2 = gamepadState.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
                    rawL1 = gamepadState.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
                    rawR1 = gamepadState.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
                } else {
                    // For generic joysticks that aren't recognized as gamepads, we can still attempt to read axes and buttons.
                    int acount;
                    const float* axes = glfwGetJoystickAxes(activeJoystick, &acount);
                    if (axes && acount >= 4) {
                        rawLeftX = axes[0];
                        rawLeftY = axes[1];
                        
                        // Heuristic for Generic Linux Joysticks (often Xbox/PS controller without Gamepad DB)
                        if (acount >= 6) {
                            rawRightX = axes[2]; 
                            rawRightY = axes[3];
                            rawR2 = axes[4];
                            rawL2 = axes[5];
                        } else {
                            rawRightX = axes[2];
                            rawRightY = axes[3];
                        }
                    }
                    int bcount;
                    const unsigned char* buttons = glfwGetJoystickButtons(activeJoystick, &bcount);
                    if (buttons && bcount >= 6) {
                        rawL1 = buttons[6] == GLFW_PRESS;
                        rawR1 = buttons[7] == GLFW_PRESS;
                    }
                }
            }

            if (activeJoystick != -1) {
                // deadzone to ignore imprecise sticks, increase if you find the camera drifting when the stick is released
                float deadzone = 0.05f;
                
                // Scale factor to roughly match mouse speed (pixels vs normalized axes)
                float joySens = controller->rotationSensitivity * 500.0f * deltaTime;
                
                if (std::abs(rawRightX) > deadzone) {
                    rotation.y -= rawRightX * joySens;
                }
                if (std::abs(rawRightY) > deadzone) {
                    rotation.x -= rawRightY * joySens;
                }
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

            glm::vec3 current_sensitivity = controller->positionSensitivity * controller->speedupFactor;
            // If the LEFT SHIFT key is pressed, we multiply the position sensitivity by the speed up factor
            bool shiftPressed = app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT);
            if (activeJoystick != -1 && rawR2 > 0.5f) {
                shiftPressed = true;
            }

            if (shiftPressed)
            {
                controller->speedupFactor = 3.0f;
                controller->tiltingSensitivity = 0.6f;
            }
            else if (app->getKeyboard().isPressed(GLFW_KEY_LEFT_CONTROL) || (activeJoystick != -1 && rawL2 > 0.5f))
            {
                controller->speedupFactor = 0.5f;
                controller->tiltingSensitivity = 0.1f;
            }
            else
            {
                controller->speedupFactor = 1.0f;
                controller->tiltingSensitivity = 0.2f;
            }

            // Start/stop motor sound on shift press/release edges
            if (audioSystem)
            {
                if (shiftPressed && !shiftWasPressed)
                    audioSystem->startMotorSound("assets/sounds/motor-loop.mp3", 0.5f);
                else if (!shiftPressed && shiftWasPressed)
                    audioSystem->stopMotorSound();
            }
            shiftWasPressed = shiftPressed;

            // Moves the player forward automatically
            position += front * (deltaTime * current_sensitivity.z);

            // A & D tilts & rotates the player left or right
            float max_roll = glm::pi<float>();                 // Limit tilt to 45 degrees
            float roll_speed = controller->tiltingSensitivity; // How fast it tilts
            bool turning = false;

            bool turningRight = app->getKeyboard().isPressed(GLFW_KEY_D) || app->getKeyboard().isPressed(GLFW_KEY_RIGHT);
            bool turningLeft = app->getKeyboard().isPressed(GLFW_KEY_A) || app->getKeyboard().isPressed(GLFW_KEY_LEFT);

            if (activeJoystick != -1) {
                if (rawLeftX > 0.2f) turningRight = true;
                if (rawLeftX < -0.2f) turningLeft = true;
            }

            if (turningRight)
            {
                rotation.y -= (deltaTime * roll_speed);
                if (rotation.z > -max_roll)
                    rotation.z -= deltaTime * roll_speed; // Tilt right
                turning = true;
            }
            if (turningLeft)
            {
                rotation.y += (deltaTime * roll_speed);
                if (rotation.z < max_roll)
                    rotation.z += deltaTime * roll_speed; // Tilt left
                turning = true;
            }

            // Q & E just do direct rotation around Z (rolling)
            if (app->getKeyboard().isPressed(GLFW_KEY_E) || (activeJoystick != -1 && rawR1))
            {
                rotation.z -= deltaTime * roll_speed * 2.5f; // Pure roll right
                turning = true;
            }
            if (app->getKeyboard().isPressed(GLFW_KEY_Q) || (activeJoystick != -1 && rawL1))
            {
                rotation.z += deltaTime * roll_speed * 2.5f; // Pure roll left
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

            // If SPACE is pressed, fix the pitch
            if (app->getKeyboard().isPressed(GLFW_KEY_SPACE))
            {
                return;
            }

            // Handle the emote (back flip and speed boost)
            if (app->getKeyboard().justPressed(GLFW_KEY_UP) && !inEmote) {
                inEmote = true;
                emoteProgress = 0.0f;
            }

            Entity* planeVisuals = nullptr;
            for (auto child : world->getEntities()) {
                if (child->parent == entity && child->name == "Plane Visuals") {
                    planeVisuals = child;
                    break;
                }
            }

            if (inEmote && planeVisuals) {
                float emoteDuration = 0.7f;
                emoteProgress += deltaTime;

                float t = emoteProgress / emoteDuration;
                if (t > 1.0f) t = 1.0f;

                float angle = t * 2.0f * glm::pi<float>();

                glm::vec3 basePos = glm::vec3(0.0f, -1.5f, -4.0f);
                float radius = 0.2f;

                // Peaks at midpoint (t=0.5), returns to 0 at end
                float forwardShift = 1.0f * glm::sin(t * glm::pi<float>());
                float topShift     = 1.0f * glm::sin(t * glm::pi<float>()); // tune the 1.0f multiplier

                glm::vec3 loopOffset = glm::vec3(
                    0.0f,
                    radius * glm::sin(angle),
                -radius * (1.0f - glm::cos(angle))
                );

                planeVisuals->localTransform.rotation.x = -glm::pi<float>() / 2.0f + angle;
                planeVisuals->localTransform.position    =  basePos
                                                        + loopOffset
                                                        + glm::vec3(0.0f, forwardShift + topShift, 0.0f);

                if (emoteProgress >= emoteDuration) {
                    inEmote = false;
                    planeVisuals->localTransform.rotation.x = -glm::pi<float>() / 2.0f;
                    planeVisuals->localTransform.position    =  basePos;
                }
            }

            // S & W moves the player pitch back and forth (swapped and with arrows)
            float max_pitch = glm::pi<float>() / 3.0f;          // Limit tilt to 60 degrees
            float pitch_speed = controller->tiltingSensitivity; // How fast it tilts

            bool pitchingUp = app->getKeyboard().isPressed(GLFW_KEY_S) || app->getKeyboard().isPressed(GLFW_KEY_DOWN);
            bool pitchingDown = app->getKeyboard().isPressed(GLFW_KEY_W); // UP key is now for emote!

            if (activeJoystick != -1) {
                if (rawLeftY > 0.2f) pitchingUp = true;
                if (rawLeftY < -0.2f) pitchingDown = true;
            }

            if (pitchingUp)
            {
                rotation.x += (deltaTime * pitch_speed) * glm::cos(rotation.z);
                rotation.y += (deltaTime * pitch_speed) * glm::sin(rotation.z);
            }
            else if (pitchingDown)
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
