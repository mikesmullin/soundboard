#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Forward declaration to avoid circular dependency
struct Soundboard;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

#endif  // CALLBACKS_H
