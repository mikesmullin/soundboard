#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define str_casecmp _stricmp
#else
#include <strings.h>
#define str_casecmp strcasecmp
#endif

#include "callbacks.h"
#include "renderer.h"
#include "soundboard.h"

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#error "This program requires a C99-compliant compiler."
#endif

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  (void)hInstance;
  (void)hPrevInstance;
  (void)lpCmdLine;
  (void)nCmdShow;
#else
int main(void) {
#endif

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

  // Start filesystem watcher
#ifdef _WIN32
  sb.watcher_stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  sb.watcher_thread = CreateThread(NULL, 0, file_watcher_thread, &sb, 0, NULL);
#else
  sb.watcher_stop = 0;
  sb.player_pid = 0;
  if (pthread_create(&sb.watcher_thread, NULL, file_watcher_thread, &sb) != 0) {
    fprintf(stderr, "Failed to create file watcher thread\n");
  }
#endif

  while (!glfwWindowShouldClose(window)) {
    if (sb.needs_refresh) {
      load_sounds(&sb);
      sb.needs_refresh = 0;
    }

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Update playback status
    if (sb.playing_tile >= 0) {
      uint32_t current_time = get_time_ms();
      uint32_t elapsed = current_time - sb.play_start_time_ms;
      if (elapsed >= sb.sound_duration_ms) {
        sb.playing_tile = -1;
        sb.play_start_time_ms = 0;
        sb.sound_duration_ms = 0;
      }
    }

    // Draw refresh button
    /*
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
    */

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
      if (sb.playing_tile == i && sb.sound_duration_ms > 0) {
        uint32_t current_time = get_time_ms();
        uint32_t elapsed = current_time - sb.play_start_time_ms;
        float progress = (float)elapsed / (float)sb.sound_duration_ms;
        if (progress > 1.0f)
          progress = 1.0f;

        float progress_width = TILE_WIDTH * progress;
        draw_rect(tile_x, tile_y, progress_width, TILE_HEIGHT, 0.2f, 0.2f, 0.6f);
      }

      // Prepare filename for display
      char display_name[32];
      snprintf(display_name, sizeof(display_name), "%s", sb.sounds[i].name);
      display_name[sizeof(display_name) - 1] = '\0';
      char* ext = strrchr(display_name, '.');
      if (ext && str_casecmp(ext, ".wav") == 0)
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

  // Stop filesystem watcher
#ifdef _WIN32
  SetEvent(sb.watcher_stop_event);
  WaitForSingleObject(sb.watcher_thread, INFINITE);
  CloseHandle(sb.watcher_thread);
  CloseHandle(sb.watcher_stop_event);
#else
  sb.watcher_stop = 1;
  pthread_join(sb.watcher_thread, NULL);
#endif

  cleanup_renderer();
  glfwTerminate();
  return 0;
}