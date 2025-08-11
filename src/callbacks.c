#include "callbacks.h"

#include <math.h>

#include "renderer.h"
#include "soundboard.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  Soundboard* sb = (Soundboard*)glfwGetWindowUserPointer(window);
  sb->window_width = (float)width;
  sb->window_height = (float)height;

  // Recalculate grid columns based on new window width
  sb->grid_cols = floor((sb->window_width - 50.0f) / (TILE_WIDTH + TILE_SPACING));
  if (sb->grid_cols < 1) {
    sb->grid_cols = 1;
  }

  glViewport(0, 0, width, height);
  set_projection((float)width, (float)height);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
  (void)xoffset;  // Suppress unused parameter warning
  Soundboard* sb = (Soundboard*)glfwGetWindowUserPointer(window);
  sb->scroll_offset += (float)yoffset * 20.0f;

  // Calculate total rows needed for grid layout
  int total_rows = (sb->count + sb->grid_cols - 1) / sb->grid_cols;
  float max_offset = (total_rows * (TILE_HEIGHT + TILE_SPACING)) - sb->window_height + 50.0f;

  if (sb->scroll_offset < 0.0f)
    sb->scroll_offset = 0.0f;
  if (sb->scroll_offset > max_offset && max_offset > 0.0f)
    sb->scroll_offset = max_offset;
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
  Soundboard* sb = (Soundboard*)glfwGetWindowUserPointer(window);
  ypos = sb->window_height - ypos;  // Flip y-coordinate

  int old_hovered = sb->hovered_tile;
  sb->hovered_tile = -1;
  //sb->hovered_refresh_button = 0;

  // Check for refresh button hover
  /*
  float refresh_button_x = sb->window_width - REFRESH_BUTTON_WIDTH - 10.0f;
  float refresh_button_y = sb->window_height - REFRESH_BUTTON_HEIGHT - 10.0f;

  if (xpos >= refresh_button_x && xpos <= refresh_button_x + REFRESH_BUTTON_WIDTH &&
      ypos >= refresh_button_y && ypos <= refresh_button_y + REFRESH_BUTTON_HEIGHT) {
    sb->hovered_refresh_button = 1;
  } else {
  */
  for (int i = 0; i < sb->count; i++) {
    int row = i / sb->grid_cols;
    int col = i % sb->grid_cols;
    float tile_x = 50.0f + col * (TILE_WIDTH + TILE_SPACING);
    float tile_y =
        sb->window_height - (row * (TILE_HEIGHT + TILE_SPACING) + 50.0f) - sb->scroll_offset;
    if (xpos >= tile_x && xpos <= tile_x + TILE_WIDTH && ypos >= tile_y &&
        ypos <= tile_y + TILE_HEIGHT) {
      sb->hovered_tile = i;
      break;
    }
  }
  //}

  // Reset marquee offset when hovering changes
  if (old_hovered != sb->hovered_tile) {
    if (old_hovered >= 0 && old_hovered < sb->count) {
      sb->sounds[old_hovered].marquee_offset = 0.0f;
    }
    if (sb->hovered_tile >= 0 && sb->hovered_tile < sb->count) {
      sb->sounds[sb->hovered_tile].marquee_offset = 0.0f;
    }
  }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
  (void)mods;  // Suppress unused parameter warning
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    Soundboard* sb = (Soundboard*)glfwGetWindowUserPointer(window);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    ypos = sb->window_height - ypos;  // Flip y-coordinate (OpenGL origin is bottom-left)

    // Check for refresh button click
    /*
    float refresh_button_x = sb->window_width - REFRESH_BUTTON_WIDTH - 10.0f;
    float refresh_button_y = sb->window_height - REFRESH_BUTTON_HEIGHT - 10.0f;

    if (xpos >= refresh_button_x && xpos <= refresh_button_x + REFRESH_BUTTON_WIDTH &&
        ypos >= refresh_button_y && ypos <= refresh_button_y + REFRESH_BUTTON_HEIGHT) {
      load_sounds(sb);
      return;
    }
    */

    for (int i = 0; i < sb->count; i++) {
      int row = i / sb->grid_cols;
      int col = i % sb->grid_cols;

      float tile_x = 50.0f + col * (TILE_WIDTH + TILE_SPACING);
      float tile_y =
          sb->window_height - (row * (TILE_HEIGHT + TILE_SPACING) + 50.0f) - sb->scroll_offset;

      if (xpos >= tile_x && xpos <= tile_x + TILE_WIDTH && ypos >= tile_y &&
          ypos <= tile_y + TILE_HEIGHT) {
        play_sound(sb->sounds[i].path, sb, i);
        break;
      }
    }
  }
}
