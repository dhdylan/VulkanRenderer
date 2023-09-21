#pragma once
#include "main.cpp"

class Window
{
public:
    GLFWwindow* window;
    VkSurfaceKHR surface;

    /// <summary>
    /// Uses "glfwCreateSurface" to create a VkSurfaceKHR. Populates private member "surface".
    /// </summary>
    void createSurface(VkInstance instance)
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface))
        {
            throw std::runtime_error("Failed to create window surface.");
        }
    }

    /// <summary>
/// Initializes GLFW and creates a window. Sotres window in private class member "window"
/// </summary>
    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
    }
private:
};

