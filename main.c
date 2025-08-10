#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "renderer.h"
#include "soundboard.h"

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#error "This program requires a C99-compliant compiler."
#endif

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
  sb->hovered_refresh_button = 0;

  // Check for refresh button hover
  float refresh_button_x = sb->window_width - REFRESH_BUTTON_WIDTH - 10.0f;
  float refresh_button_y = sb->window_height - REFRESH_BUTTON_HEIGHT - 10.0f;

  if (xpos >= refresh_button_x && xpos <= refresh_button_x + REFRESH_BUTTON_WIDTH &&
      ypos >= refresh_button_y && ypos <= refresh_button_y + REFRESH_BUTTON_HEIGHT) {
    sb->hovered_refresh_button = 1;
  } else {
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
  }

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
    float refresh_button_x = sb->window_width - REFRESH_BUTTON_WIDTH - 10.0f;
    float refresh_button_y = sb->window_height - REFRESH_BUTTON_HEIGHT - 10.0f;

    if (xpos >= refresh_button_x && xpos <= refresh_button_x + REFRESH_BUTTON_WIDTH &&
        ypos >= refresh_button_y && ypos <= refresh_button_y + REFRESH_BUTTON_HEIGHT) {
      load_sounds(sb);
      return;
    }

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
  sb.window_width = 800.0f;
  sb.window_height = 600.0f;
  sb.grid_cols = floor((sb.window_width - 50.0f) / (TILE_WIDTH + TILE_SPACING));
  if (sb.grid_cols < 1) {
    sb.grid_cols = 1;
  }

  glfwSetWindowUserPointer(window, &sb);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);

  load_sounds(&sb);

  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Update playback status
    if (sb.playing_tile >= 0) {
      if (!is_sound_playing()) {
        sb.playing_tile = -1;
        sb.play_start_time = 0;
        sb.sound_duration = 0;
      }
    }

    // Draw refresh button
    float refresh_button_x = sb.window_width - REFRESH_BUTTON_WIDTH - 10.0f;
    float refresh_button_y = sb.window_height - REFRESH_BUTTON_HEIGHT - 10.0f;
    float r = 0.5f, g = 0.5f, b = 0.5f;
    if (sb.hovered_refresh_button) {
      r = 0.7f;
      g = 0.7f;
      b = 0.7f;
    }
    draw_rect(
        refresh_button_x,
        refresh_button_y,
        REFRESH_BUTTON_WIDTH,
        REFRESH_BUTTON_HEIGHT,
        r,
        g,
        b);
    draw_text(refresh_button_x + 10.0f, refresh_button_y + 10.0f, "Refresh", 1.0f, 1.0f, 1.0f);

    for (int i = 0; i < sb.count; i++) {
      int row = i / sb.grid_cols;
      int col = i % sb.grid_cols;

      float tile_x = 50.0f + col * (TILE_WIDTH + TILE_SPACING);
      float tile_y =
          sb.window_height - (row * (TILE_HEIGHT + TILE_SPACING) + 50.0f) - sb.scroll_offset;

      if (tile_y + TILE_HEIGHT < 0 || tile_y > sb.window_height)
        continue;

      // Draw tile background
      draw_rect(tile_x, tile_y, TILE_WIDTH, TILE_HEIGHT, 0.3f, 0.3f, 0.8f);

      // Draw playback progress overlay if this tile is playing
      if (sb.playing_tile == i && sb.sound_duration > 0) {
        DWORD current_time = GetTickCount();
        DWORD elapsed = current_time - sb.play_start_time;
        float progress = (float)elapsed / (float)sb.sound_duration;
        if (progress > 1.0f)
          progress = 1.0f;

        float progress_width = TILE_WIDTH * progress;
        draw_rect(tile_x, tile_y, progress_width, TILE_HEIGHT, 0.2f, 0.2f, 0.6f);
      }

      // Prepare filename for display
      char display_name[32];
      strncpy_s(display_name, sizeof(display_name), sb.sounds[i].name, sizeof(display_name) - 1);
      display_name[sizeof(display_name) - 1] = '\0';
      char* ext = strrchr(display_name, '.');
      if (ext && _stricmp(ext, ".wav") == 0)
        *ext = '\0';

      // Handle marquee scrolling for hovered tile
      float text_x = tile_x + 5.0f;

      if (sb.hovered_tile == i && strlen(display_name) > 18) {
        // Update marquee offset
        sb.sounds[i].marquee_offset += 30.0f * (1.0f / 60.0f);  // Assume 60 FPS

        // Calculate text width (approximate)
        float text_width = strlen(display_name) * 8.0f;  // Approximate character width
        float visible_width = TILE_WIDTH - 10.0f;  // Available width for text

        // Reset offset when text has scrolled completely
        if (sb.sounds[i].marquee_offset > text_width + visible_width) {
          sb.sounds[i].marquee_offset = -visible_width;
        }

        text_x = tile_x + 5.0f - sb.sounds[i].marquee_offset;
      } else {
        // Truncate text if not hovered
        if (strlen(display_name) > 18) {
          display_name[15] = '.';
          display_name[16] = '.';
          display_name[17] = '.';
          display_name[18] = '\0';
        }
      }

      // Draw filename text (with clipping for marquee effect)
      if (sb.hovered_tile == i && strlen(sb.sounds[i].name) > 18) {
        draw_text_clipped(
            text_x,
            tile_y + TILE_HEIGHT - 15.0f,
            display_name,
            1.0f,
            1.0f,
            1.0f,
            tile_x + 5.0f,
            tile_y,
            TILE_WIDTH - 10.0f,
            TILE_HEIGHT);
      } else {
        draw_text(text_x, tile_y + TILE_HEIGHT - 15.0f, display_name, 1.0f, 1.0f, 1.0f);
      }
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  cleanup_renderer();
  glfwTerminate();
  return 0;
}