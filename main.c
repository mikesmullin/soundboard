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
texture_atlas_t* atlas = NULL;
texture_font_t* font = NULL;

// Core GL resources
static GLuint text_program = 0;
static GLint uTexLoc = -1, uTextProjLoc = -1, uTextColorLoc = -1;
static GLuint textVAO = 0, textVBO = 0;

static GLuint rect_program = 0;
static GLint uRectProjLoc = -1, uRectColorLoc = -1;
static GLuint rectVAO = 0, rectVBO = 0;

static float gProj[16];

static void mat4_ortho(float l, float r, float b, float t, float n, float f, float* m) {
  // Column-major
  m[0] = 2.0f / (r - l);
  m[4] = 0.0f;
  m[8] = 0.0f;
  m[12] = -(r + l) / (r - l);
  m[1] = 0.0f;
  m[5] = 2.0f / (t - b);
  m[9] = 0.0f;
  m[13] = -(t + b) / (t - b);
  m[2] = 0.0f;
  m[6] = 0.0f;
  m[10] = -2.0f / (f - n);
  m[14] = -(f + n) / (f - n);
  m[3] = 0.0f;
  m[7] = 0.0f;
  m[11] = 0.0f;
  m[15] = 1.0f;
}

static void ensure_rect_buffers(void) {
  if (rectVAO)
    return;
  glGenVertexArrays(1, &rectVAO);
  glBindVertexArray(rectVAO);
  glGenBuffers(1, &rectVBO);
  glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(6 * 2 * sizeof(float)), NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)(2 * sizeof(float)), (void*)0);
  glBindVertexArray(0);
}

static void ensure_text_buffers(void) {
  if (textVAO)
    return;
  glGenVertexArrays(1, &textVAO);
  glBindVertexArray(textVAO);
  glGenBuffers(1, &textVBO);
  glBindBuffer(GL_ARRAY_BUFFER, textVBO);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(6 * 4 * sizeof(float)), NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)(4 * sizeof(float)), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
      1,
      2,
      GL_FLOAT,
      GL_FALSE,
      (GLsizei)(4 * sizeof(float)),
      (void*)(2 * sizeof(float)));
  glBindVertexArray(0);
}

static GLuint compile_shader(GLenum type, const char* src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, NULL);
  glCompileShader(s);
  GLint ok = GL_FALSE;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint len = 0;
    glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
    char* log = (char*)malloc(len > 1 ? (size_t)len : 1);
    if (log) {
      glGetShaderInfoLog(s, len, NULL, log);
      fprintf(stderr, "Text shader compile error: %s\n", log);
      free(log);
    }
    glDeleteShader(s);
    return 0;
  }
  return s;
}

static GLuint link_program(GLuint vs, GLuint fs) {
  GLuint p = glCreateProgram();
  glAttachShader(p, vs);
  glAttachShader(p, fs);
  glLinkProgram(p);
  GLint ok = GL_FALSE;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    GLint len = 0;
    glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
    char* log = (char*)malloc(len > 1 ? (size_t)len : 1);
    if (log) {
      glGetProgramInfoLog(p, len, NULL, log);
      fprintf(stderr, "Text shader link error: %s\n", log);
      free(log);
    }
    glDeleteProgram(p);
    return 0;
  }
  return p;
}

static void create_text_program(void) {
  if (text_program)
    return;
  const char* vs_src =
      "#version 330 core\n"
      "layout(location=0) in vec2 aPos;\n"
      "layout(location=1) in vec2 aUV;\n"
      "uniform mat4 uProj;\n"
      "out vec2 vUV;\n"
      "void main(){\n"
      "  gl_Position = uProj * vec4(aPos, 0.0, 1.0);\n"
      "  vUV = aUV;\n"
      "}\n";
  const char* fs_src =
      "#version 330 core\n"
      "in vec2 vUV;\n"
      "uniform sampler2D uTex;\n"
      "uniform vec3 uColor;\n"
      "out vec4 FragColor;\n"
      "void main(){\n"
      "  float coverage = texture(uTex, vUV).r;\n"
      "  FragColor = vec4(uColor, coverage);\n"
      "}\n";
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
  if (!vs || !fs) {
    if (vs)
      glDeleteShader(vs);
    if (fs)
      glDeleteShader(fs);
    return;
  }
  text_program = link_program(vs, fs);
  glDeleteShader(vs);
  glDeleteShader(fs);
  if (text_program) {
    glUseProgram(text_program);
    uTexLoc = glGetUniformLocation(text_program, "uTex");
    uTextProjLoc = glGetUniformLocation(text_program, "uProj");
    uTextColorLoc = glGetUniformLocation(text_program, "uColor");
    if (uTexLoc >= 0)
      glUniform1i(uTexLoc, 0);
    glUseProgram(0);
  }
}

static void create_rect_program(void) {
  if (rect_program)
    return;
  const char* vs_src =
      "#version 330 core\n"
      "layout(location=0) in vec2 aPos;\n"
      "uniform mat4 uProj;\n"
      "void main(){\n"
      "  gl_Position = uProj * vec4(aPos, 0.0, 1.0);\n"
      "}\n";
  const char* fs_src =
      "#version 330 core\n"
      "uniform vec3 uColor;\n"
      "out vec4 FragColor;\n"
      "void main(){\n"
      "  FragColor = vec4(uColor, 1.0);\n"
      "}\n";
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
  if (!vs || !fs) {
    if (vs)
      glDeleteShader(vs);
    if (fs)
      glDeleteShader(fs);
    return;
  }
  rect_program = link_program(vs, fs);
  glDeleteShader(vs);
  glDeleteShader(fs);
  if (rect_program) {
    glUseProgram(rect_program);
    uRectProjLoc = glGetUniformLocation(rect_program, "uProj");
    uRectColorLoc = glGetUniformLocation(rect_program, "uColor");
    glUseProgram(0);
  }
}

void draw_rect(float x, float y, float w, float h, float r, float g, float b) {
  glBegin(GL_QUADS);
  glColor3f(r, g, b);
  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);
  glEnd();
}

void draw_text_simple(float x, float y, const char* text, float r, float g, float b) {
  if (!font || !atlas)
    return;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);

  // Bind the atlas texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, atlas->id);

  // Use text shader that reads RED channel as alpha (works regardless of swizzle support)
  if (text_program)
    glUseProgram(text_program);

  float pen_x = x;
  float pen_y = y;

  for (int i = 0; text[i] != '\0'; i++) {
    char character[2] = {text[i], '\0'};
    texture_glyph_t* glyph = texture_font_get_glyph(font, character);
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
    glColor4f(r, g, b, 1.0f);
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

  // Restore state
  if (text_program)
    glUseProgram(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}

void init_font_system() {
  atlas = texture_atlas_new(512, 512, 1);

  // Try to load a system font, fallback to basic if not available
  const char* font_paths[] = {
      "C:\\Windows\\Fonts\\arial.ttf",
      "C:\\Windows\\Fonts\\calibri.ttf",
      "C:\\Windows\\Fonts\\verdana.ttf",
      NULL};

  font = NULL;
  for (int i = 0; font_paths[i] != NULL; i++) {
    font = texture_font_new_from_file(atlas, 16, font_paths[i]);
    if (font)
      break;
  }

  if (!font) {
    fprintf(stderr, "Warning: Could not load any system fonts, text may not display\n");
    return;
  }

  // Ensure pixel rows are tightly packed (important for 1-channel glyph uploads)
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // Make sure we have a valid GL texture name in the atlas
  if (atlas->id == 0) {
    glGenTextures(1, &atlas->id);
  }

  // Configure atlas texture params
  glBindTexture(GL_TEXTURE_2D, atlas->id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  // Tell OpenGL to use the R channel of the texture as the alpha value for rendering
  if (GLEW_VERSION_3_3 || GLEW_ARB_texture_swizzle) {
    GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
  }
  glBindTexture(GL_TEXTURE_2D, 0);

  // Pre-load ASCII characters to ensure they're in the atlas
  const char* cache_text =
      " !\"#$%&'()*+,-./"
      "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
      "abcdefghijklmnopqrstuvwxyz{|}~";
  texture_font_load_glyphs(font, cache_text);

  // Upload atlas data to GPU (support both legacy GL_ALPHA and modern GL_RED)
  glBindTexture(GL_TEXTURE_2D, atlas->id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  GLint internal_format;
  GLenum format;
  if (GLEW_VERSION_3_0 || GLEW_ARB_texture_rg) {
    internal_format = GL_R8;
    format = GL_RED;
  } else {
    internal_format = GL_ALPHA;
    format = GL_ALPHA;
  }
  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      internal_format,
      (GLsizei)atlas->width,
      (GLsizei)atlas->height,
      0,
      format,
      GL_UNSIGNED_BYTE,
      atlas->data);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Create GL programs and buffers
  create_rect_program();
  create_text_program();
  ensure_rect_buffers();
  ensure_text_buffers();

  printf(
      "Font loaded successfully. Atlas size: %zux%zu (tex %u)\n",
      atlas->width,
      atlas->height,
      atlas->id);
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
  if (textVBO) {
    glDeleteBuffers(1, &textVBO);
    textVBO = 0;
  }
  if (textVAO) {
    glDeleteVertexArrays(1, &textVAO);
    textVAO = 0;
  }
  if (rectVBO) {
    glDeleteBuffers(1, &rectVBO);
    rectVBO = 0;
  }
  if (rectVAO) {
    glDeleteVertexArrays(1, &rectVAO);
    rectVAO = 0;
  }
  if (text_program) {
    glDeleteProgram(text_program);
    text_program = 0;
  }
  if (rect_program) {
    glDeleteProgram(rect_program);
    rect_program = 0;
  }
}

void load_sounds(Soundboard* sb) {
  DIR* dir;
  struct dirent* entry;
  sb->count = 0;
  sb->scroll_offset = 0.0f;

  dir = opendir(".");
  if (!dir) {
    fprintf(stderr, "Failed to open directory\n");
    return;
  }

  while ((entry = readdir(dir)) && sb->count < MAX_SOUNDS) {
    char* ext = strrchr(entry->d_name, '.');
    if (ext && _stricmp(ext, ".wav") == 0) {
      strncpy_s(sb->sounds[sb->count].name, MAX_PATH, entry->d_name, MAX_PATH - 1);
      snprintf(sb->sounds[sb->count].path, MAX_PATH, ".\\%s", entry->d_name);
      sb->count++;
    }
  }
  closedir(dir);
}

void play_sound(const char* path) {
  PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC);
}

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

static void draw_rect_modern(float x, float y, float w, float h, float r, float g, float b) {
  ensure_rect_buffers();
  glUseProgram(rect_program);
  glUniformMatrix4fv(uRectProjLoc, 1, GL_FALSE, gProj);
  glUniform3f(uRectColorLoc, r, g, b);
  float x0 = x, y0 = y, x1 = x + w, y1 = y + h;
  float verts[12] = {
      x0,
      y0,
      x1,
      y0,
      x1,
      y1,  // tri1
      x0,
      y0,
      x1,
      y1,
      x0,
      y1  // tri2
  };
  glBindVertexArray(rectVAO);
  glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
  glUseProgram(0);
}

static void draw_text_modern(float x, float y, const char* text, float r, float g, float b) {
  if (!font || !atlas || !text_program)
    return;
  ensure_text_buffers();
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, atlas->id);
  glUseProgram(text_program);
  glUniformMatrix4fv(uTextProjLoc, 1, GL_FALSE, gProj);
  glUniform3f(uTextColorLoc, r, g, b);

  float pen_x = x;
  float pen_y = y;
  glBindVertexArray(textVAO);

  for (int i = 0; text[i] != '\0'; i++) {
    char character[2] = {text[i], '\0'};
    texture_glyph_t* glyph = texture_font_get_glyph(font, character);
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

    float quad[24] = {
        x0, y0, s0, t0, x1, y0, s1, t0, x1, y1, s1, t1,
        x0, y0, s0, t0, x1, y1, s1, t1, x0, y1, s0, t1,
    };

    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    pen_x += glyph->advance_x;
  }

  glBindVertexArray(0);
  glUseProgram(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_BLEND);
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

  // Build orthographic projection for core pipeline
  mat4_ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f, gProj);

  // Initialize font system after OpenGL is ready
  init_font_system();

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

      // Draw tile background (modern path)
      draw_rect_modern(tile_x, tile_y, TILE_WIDTH, TILE_HEIGHT, 0.3f, 0.3f, 0.8f);

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

      // Draw filename text (modern path)
      draw_text_modern(tile_x + 5.0f, tile_y + TILE_HEIGHT - 15.0f, display_name, 1.0f, 1.0f, 1.0f);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  cleanup_font_system();
  glfwTerminate();
  return 0;
}