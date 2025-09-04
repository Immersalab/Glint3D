#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "gl_platform.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class InputHandler {
public:
    static void initialize(GLFWwindow* window);
    static void processInput(GLFWwindow* window);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    static glm::vec3 cameraPos;
    static glm::vec3 cameraFront;
    static glm::vec3 cameraUp;
    static float cameraSpeed;
    static float sensitivity;
    static bool firstMouse;
    static double lastX, lastY;
    static float pitch, yaw;
    static bool rightMousePressed;
};

#endif // INPUT_HANDLER_H
