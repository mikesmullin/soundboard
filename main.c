#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <dirent.h>
#include <math.h>
#include <mmsystem.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#pragma comment(lib, "winmm.lib")

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#error "This program requires a C99-compliant compiler."
#endif

#define MAX_SOUNDS 100
#define TILE_WIDTH 150.0f
#define TILE_HEIGHT 60.0f
#define TILE_SPACING 10.0f
#define GRID_COLS 4
#define MAX_PATH 260

typedef struct {
  char name[MAX_PATH];
  char path[MAX_PATH];
} Sound;

typedef struct {
  Sound sounds[MAX_SOUNDS];
  int count;
  float scroll_offset;
} Soundboard;

void draw_rect(float x, float y, float w, float h, float r, float g, float b) {
  glBegin(GL_QUADS);
  glColor3f(r, g, b);
  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);
  glEnd();
}

void draw_text_simple(float x, float y, const char *text, float r, float g,
                      float b) {
  // Simple bitmap text rendering using GLFW
  glColor3f(r, g, b);
  glRasterPos2f(x, y);

  // For now, we'll draw a simple representation
  // In a full implementation, you'd use a proper font rendering library
  // This is a placeholder that draws small rectangles for each character
  float char_width = 6.0f;
  float char_height = 8.0f;

  for (int i = 0; text[i] != '\0' && i < 20; i++) { // Limit to 20 chars
    if (text[i] != '.' && text[i] != ' ') {
      draw_rect(x + i * char_width, y, char_width - 1, char_height, r, g, b);
    }
  }
}

void load_sounds(Soundboard *sb) {
  DIR *dir;
  struct dirent *entry;
  sb->count = 0;
  sb->scroll_offset = 0.0f;

  dir = opendir(".");
  if (!dir) {
    fprintf(stderr, "Failed to open directory\n");
    return;
  }

  while ((entry = readdir(dir)) && sb->count < MAX_SOUNDS) {
    char *ext = strrchr(entry->d_name, '.');
    if (ext && _stricmp(ext, ".wav") == 0) {
      strncpy_s(sb->sounds[sb->count].name, MAX_PATH, entry->d_name,
                MAX_PATH - 1);
      snprintf(sb->sounds[sb->count].path, MAX_PATH, ".\\%s", entry->d_name);
      sb->count++;
    }
  }
  closedir(dir);
}

void play_sound(const char *path) {
  PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  (void)xoffset; // Suppress unused parameter warning
  Soundboard *sb = (Soundboard *)glfwGetWindowUserPointer(window);
  sb->scroll_offset += (float)yoffset * 20.0f;

  // Calculate total rows needed for grid layout
  int total_rows = (sb->count + GRID_COLS - 1) / GRID_COLS;
  float max_offset =
      (total_rows * (TILE_HEIGHT + TILE_SPACING)) - 600.0f + 50.0f;

  if (sb->scroll_offset < 0.0f)
    sb->scroll_offset = 0.0f;
  if (sb->scroll_offset > max_offset && max_offset > 0.0f)
    sb->scroll_offset = max_offset;
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  (void)mods; // Suppress unused parameter warning
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    Soundboard *sb = (Soundboard *)glfwGetWindowUserPointer(window);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    ypos = 600.0 - ypos; // Flip y-coordinate (OpenGL origin is bottom-left)

    for (int i = 0; i < sb->count; i++) {
      int row = i / GRID_COLS;
      int col = i % GRID_COLS;

      float tile_x = 50.0f + col * (TILE_WIDTH + TILE_SPACING);
      float tile_y = 600.0f - (row * (TILE_HEIGHT + TILE_SPACING) + 50.0f) -
                     sb->scroll_offset;

      if (xpos >= tile_x && xpos <= tile_x + TILE_WIDTH && ypos >= tile_y &&
          ypos <= tile_y + TILE_HEIGHT) {
        play_sound(sb->sounds[i].path);
        break;
      }
    }
  }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  (void)hInstance;     // Suppress unused parameter warning
  (void)hPrevInstance; // Suppress unused parameter warning
  (void)lpCmdLine;     // Suppress unused parameter warning
  (void)nCmdShow;      // Suppress unused parameter warning
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }

  GLFWwindow *window = glfwCreateWindow(800, 600, "Soundboard", NULL, NULL);
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

  Soundboard sb = {0};
  glfwSetWindowUserPointer(window, &sb);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);

  load_sounds(&sb);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, 800, 0, 600, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark background

    for (int i = 0; i < sb.count; i++) {
      int row = i / GRID_COLS;
      int col = i % GRID_COLS;

      float tile_x = 50.0f + col * (TILE_WIDTH + TILE_SPACING);
      float tile_y = 600.0f - (row * (TILE_HEIGHT + TILE_SPACING) + 50.0f) -
                     sb.scroll_offset;

      if (tile_y + TILE_HEIGHT < 0 || tile_y > 600.0f)
        continue; // Skip tiles outside viewport

      // Draw tile background
      draw_rect(tile_x, tile_y, TILE_WIDTH, TILE_HEIGHT, 0.3f, 0.3f,
                0.8f); // Blue tiles

      // Prepare filename for display (remove .wav extension and truncate if
      // needed)
      char display_name[32];
      strncpy_s(display_name, sizeof(display_name), sb.sounds[i].name,
                sizeof(display_name) - 1);
      display_name[sizeof(display_name) - 1] = '\0';

      // Remove .wav extension
      char *ext = strrchr(display_name, '.');
      if (ext && _stricmp(ext, ".wav") == 0) {
        *ext = '\0';
      }

      // Truncate if too long (roughly 18 characters fit in tile width)
      if (strlen(display_name) > 18) {
        display_name[15] = '.';
        display_name[16] = '.';
        display_name[17] = '.';
        display_name[18] = '\0';
      }

      // Draw filename text
      draw_text_simple(tile_x + 5.0f, tile_y + TILE_HEIGHT - 15.0f,
                       display_name, 1.0f, 1.0f, 1.0f);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}