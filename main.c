#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <dirent.h>
#include <freetype-gl/freetype-gl.h>
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

// FreeType-GL globals
texture_atlas_t *atlas = NULL;
texture_font_t *font = NULL;

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
  if (!font || !atlas)
    return;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(r, g, b, 1.0f);

  // Bind the atlas texture
  glBindTexture(GL_TEXTURE_2D, atlas->id);

  float pen_x = x;
  float pen_y = y;

  for (int i = 0; text[i] != '\0'; i++) {
    char character[2] = {text[i], '\0'};
    texture_glyph_t *glyph = texture_font_get_glyph(font, character);
    if (!glyph)
      continue;

    float x0 = pen_x + glyph->offset_x;
    float y0 = pen_y + glyph->offset_y;
    float x1 = x0 + glyph->width;
    float y1 = y0 - glyph->height;

    float s0 = glyph->s0;
    float t0 = glyph->t0;
    float s1 = glyph->s1;
    float t1 = glyph->t1;

    glBegin(GL_QUADS);
    glTexCoord2f(s0, t0);
    glVertex2f(x0, y0);
    glTexCoord2f(s0, t1);
    glVertex2f(x0, y1);
    glTexCoord2f(s1, t1);
    glVertex2f(x1, y1);
    glTexCoord2f(s1, t0);
    glVertex2f(x1, y0);
    glEnd();

    pen_x += glyph->advance_x;
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_BLEND);
}

void init_font_system() {
  atlas = texture_atlas_new(512, 512, 1);

  // Try to load a system font, fallback to basic if not available
  const char *font_paths[] = {"C:\\Windows\\Fonts\\arial.ttf",
                              "C:\\Windows\\Fonts\\calibri.ttf",
                              "C:\\Windows\\Fonts\\verdana.ttf", NULL};

  font = NULL;
  for (int i = 0; font_paths[i] != NULL; i++) {
    font = texture_font_new_from_file(atlas, 16, font_paths[i]);
    if (font)
      break;
  }

  if (!font) {
    fprintf(stderr,
            "Warning: Could not load any system fonts, text may not display\n");
  }
}

void cleanup_font_system() {
  if (font) {
    texture_font_delete(font);
    font = NULL;
  }
  if (atlas) {
    texture_atlas_delete(atlas);
    atlas = NULL;
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

  // Initialize font system after OpenGL is ready
  init_font_system();

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

  // Enable textures for font rendering
  glEnable(GL_TEXTURE_2D);

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

  cleanup_font_system();
  glfwTerminate();
  return 0;
}