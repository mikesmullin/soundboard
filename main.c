#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "renderer.h"
#include "soundboard.h"


#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#error "This program requires a C99-compliant compiler."
#endif

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
  (void)xoffset;  // Suppress unused parameter warning
  Soundboard* sb = (Soundboard*)glfwGetWindowUserPointer(window);
  sb->scroll_offset += (float)yoffset * 20.0f;

  // Calculate total rows needed for grid layout
  int total_rows = (sb->count + GRID_COLS - 1) / GRID_COLS;
  float max_offset = (total_rows * (TILE_HEIGHT + TILE_SPACING)) - 600.0f + 50.0f;

  if (sb->scroll_offset < 0.0f)
    sb->scroll_offset = 0.0f;
  if (sb->scroll_offset > max_offset && max_offset > 0.0f)
    sb->scroll_offset = max_offset;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
  (void)mods;  // Suppress unused parameter warning
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    Soundboard* sb = (Soundboard*)glfwGetWindowUserPointer(window);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    ypos = 600.0 - ypos;  // Flip y-coordinate (OpenGL origin is bottom-left)

    for (int i = 0; i < sb->count; i++) {
      int row = i / GRID_COLS;
      int col = i % GRID_COLS;

      float tile_x = 50.0f + col * (TILE_WIDTH + TILE_SPACING);
      float tile_y = 600.0f - (row * (TILE_HEIGHT + TILE_SPACING) + 50.0f) - sb->scroll_offset;

      if (xpos >= tile_x && xpos <= tile_x + TILE_WIDTH && ypos >= tile_y &&
          ypos <= tile_y + TILE_HEIGHT) {
        play_sound(sb->sounds[i].path);
        break;
      }
    }
  }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  (void)hInstance;
  (void)hPrevInstance;
  (void)lpCmdLine;
  (void)nCmdShow;

  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }

  // Request OpenGL 3.3 Core profile for RenderDoc compatibility
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(800, 600, "Soundboard", NULL, NULL);
  if (!window) {
    glfwTerminate();
    fprintf(stderr, "Failed to create GLFW window\n");
    return -1;
  }

  glfwMakeContextCurrent(window);
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    glfwTerminate();
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
  }

  // Initialize renderer after OpenGL is ready
  if (!init_renderer()) {
    fprintf(stderr, "Failed to initialize renderer\n");
    glfwTerminate();
    return -1;
  }

  // Set projection matrix for window size
  set_projection(800.0f, 600.0f);

  Soundboard sb = {0};
  glfwSetWindowUserPointer(window, &sb);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);

  load_sounds(&sb);

  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < sb.count; i++) {
      int row = i / GRID_COLS;
      int col = i % GRID_COLS;

      float tile_x = 50.0f + col * (TILE_WIDTH + TILE_SPACING);
      float tile_y = 600.0f - (row * (TILE_HEIGHT + TILE_SPACING) + 50.0f) - sb.scroll_offset;

      if (tile_y + TILE_HEIGHT < 0 || tile_y > 600.0f)
        continue;

      // Draw tile background
      draw_rect(tile_x, tile_y, TILE_WIDTH, TILE_HEIGHT, 0.3f, 0.3f, 0.8f);

      // Prepare filename
      char display_name[32];
      strncpy_s(display_name, sizeof(display_name), sb.sounds[i].name, sizeof(display_name) - 1);
      display_name[sizeof(display_name) - 1] = '\0';
      char* ext = strrchr(display_name, '.');
      if (ext && _stricmp(ext, ".wav") == 0)
        *ext = '\0';
      if (strlen(display_name) > 18) {
        display_name[15] = '.';
        display_name[16] = '.';
        display_name[17] = '.';
        display_name[18] = '\0';
      }

      // Draw filename text
      draw_text(tile_x + 5.0f, tile_y + TILE_HEIGHT - 15.0f, display_name, 1.0f, 1.0f, 1.0f);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  cleanup_renderer();
  glfwTerminate();
  return 0;
}