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
#define TILE_HEIGHT 50.0f
#define TILE_SPACING 10.0f
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
  float max_offset =
      (sb->count * (TILE_HEIGHT + TILE_SPACING)) - 600.0f + 50.0f;
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
      float tile_y = 600.0f - (i * (TILE_HEIGHT + TILE_SPACING) + 50.0f) -
                     sb->scroll_offset;
      if (xpos >= 50.0 && xpos <= 50.0 + TILE_WIDTH && ypos >= tile_y &&
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
      float tile_y = 600.0f - (i * (TILE_HEIGHT + TILE_SPACING) + 50.0f) -
                     sb.scroll_offset;
      if (tile_y + TILE_HEIGHT < 0 || tile_y > 600.0f)
        continue; // Skip tiles outside viewport
      draw_rect(50.0f, tile_y, TILE_WIDTH, TILE_HEIGHT, 0.3f, 0.3f,
                0.8f); // Blue tiles
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}