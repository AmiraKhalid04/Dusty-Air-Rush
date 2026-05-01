#pragma once

#include <GLFW/glfw3.h>
#include <cstring>

namespace our {

    // A convenience class to read keyboard input
    class Keyboard {
    private:
        bool enabled; // Is this class enabled (allowed to read user input)
        bool currentKeyStates[GLFW_KEY_LAST + 1];
        bool previousKeyStates[GLFW_KEY_LAST + 1];

    public:
        // Enable this object and capture current keyboard state from window
        void enable(GLFWwindow* window){
            enabled = true;
            for(int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++){
                currentKeyStates[key] = previousKeyStates[key] = glfwGetKey(window, key);
            }
        }

        // Disable this object and clear the state
        void disable(){
            for(int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++){
                currentKeyStates[key] = previousKeyStates[key] = false;
            }
        }

        // update the keyboard state (moves current frame state to become the previous frame state)
        void update(){
            if(!enabled) return;
            std::memcpy(previousKeyStates, currentKeyStates, sizeof(previousKeyStates));

            bool gamepadSpace = false;
            bool gamepadEsc = false;
            bool gamepadL = false;
            bool gamepadH = false;
            for (int i = 0; i <= 15; ++i) { // GLFW_JOYSTICK_1 to GLFW_JOYSTICK_16
                if (glfwJoystickPresent(i)) {
                    if (glfwJoystickIsGamepad(i)) {
                        GLFWgamepadstate state;
                        if (glfwGetGamepadState(i, &state)) {
                            // X (bottom) -> A in GLFW, O (right) -> B in GLFW
                            if (state.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS) gamepadSpace = true;
                            if (state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS) gamepadEsc = true;
                            if (state.buttons[GLFW_GAMEPAD_BUTTON_Y] == GLFW_PRESS) gamepadH = true;
                        }
                    } else {
                        int bcount;
                        const unsigned char* buttons = glfwGetJoystickButtons(i, &bcount);
                        if (buttons && bcount >= 4) {
                            if (buttons[0] == GLFW_PRESS) gamepadSpace = true;
                            if (buttons[1] == GLFW_PRESS) gamepadEsc = true;
                            if (buttons[3] == GLFW_PRESS) gamepadL = true;
                            if (buttons[4] == GLFW_PRESS) gamepadH = true;
                        }
                    }
                    break; // Just use the first available joystick
                }
            }

            GLFWwindow* win = glfwGetCurrentContext();
            if (win) {
                currentKeyStates[GLFW_KEY_SPACE] = (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) || gamepadSpace;
                currentKeyStates[GLFW_KEY_ESCAPE] = (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) || gamepadEsc;
                currentKeyStates[GLFW_KEY_L] = (glfwGetKey(win, GLFW_KEY_L) == GLFW_PRESS) || gamepadL;
                currentKeyStates[GLFW_KEY_H] = (glfwGetKey(win, GLFW_KEY_H) == GLFW_PRESS) || gamepadH;
            }
        }

        // Event functions called from GLFW callbacks in "application.cpp"
        void keyEvent(int key, int, int action, int){
            if(!enabled) return;
            if(action == GLFW_PRESS){
                currentKeyStates[key] = true;
            } else if(action == GLFW_RELEASE){
                currentKeyStates[key] = false;
            }
        }

        // Is the key currently pressed
        [[nodiscard]] bool isPressed(int key) const {return currentKeyStates[key]; }
        // Was the key unpressed in the previous frame but became pressed in the current frame
        [[nodiscard]] bool justPressed(int key) const {return currentKeyStates[key] && !previousKeyStates[key];}
        // Was the key pressed in the previous frame but became unpressed in the current frame
        [[nodiscard]] bool justReleased(int key) const {return !currentKeyStates[key] && previousKeyStates[key];}

        [[nodiscard]] bool isEnabled() const { return enabled; }
        void setEnabled(bool enabled, GLFWwindow* window) {
            if(this->enabled != enabled) {
                if (enabled) enable(window);
                else disable();
            }
        }
    };

}
