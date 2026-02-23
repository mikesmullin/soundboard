#include "renderer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define file_readable(path) (_access((path), 4) == 0)
#else
#include <unistd.h>
#define file_readable(path) (access((path), R_OK) == 0)
#endif

#include "shaders.h"

// FreeType-GL globals
static texture_atlas_t* atlas = NULL;
static texture_font_t* font = NULL;

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
      fprintf(stderr, "Shader compile error: %s\n", log);
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
      fprintf(stderr, "Shader link error: %s\n", log);
      free(log);
    }
    glDeleteProgram(p);
    return 0;
  }
  return p;
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

static int create_text_program(void) {
  if (text_program)
    return 1;

  GLuint vs = compile_shader(GL_VERTEX_SHADER, TEXT_VERTEX_SHADER);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, TEXT_FRAGMENT_SHADER);
  if (!vs || !fs) {
    if (vs)
      glDeleteShader(vs);
    if (fs)
      glDeleteShader(fs);
    return 0;
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
    return 1;
  }
  return 0;
}

static int create_rect_program(void) {
  if (rect_program)
    return 1;

  GLuint vs = compile_shader(GL_VERTEX_SHADER, RECT_VERTEX_SHADER);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, RECT_FRAGMENT_SHADER);
  if (!vs || !fs) {
    if (vs)
      glDeleteShader(vs);
    if (fs)
      glDeleteShader(fs);
    return 0;
  }

  rect_program = link_program(vs, fs);
  glDeleteShader(vs);
  glDeleteShader(fs);

  if (rect_program) {
    glUseProgram(rect_program);
    uRectProjLoc = glGetUniformLocation(rect_program, "uProj");
    uRectColorLoc = glGetUniformLocation(rect_program, "uColor");
    glUseProgram(0);
    return 1;
  }
  return 0;
}

static int init_font_system() {
  atlas = texture_atlas_new(512, 512, 1);

  // Try to load a system font, fallback to basic if not available
#ifdef _WIN32
  const char* font_paths[] = {
      "C:\\Windows\\Fonts\\arial.ttf",
      "C:\\Windows\\Fonts\\calibri.ttf",
      "C:\\Windows\\Fonts\\verdana.ttf",
      NULL};
#else
  const char* font_paths[] = {
      "/usr/share/fonts/TTF/DejaVuSans.ttf",
      "/usr/share/fonts/dejavu/DejaVuSans.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
      NULL};
#endif

  font = NULL;
  for (int i = 0; font_paths[i] != NULL; i++) {
    if (!file_readable(font_paths[i])) {
      continue;
    }

    font = texture_font_new_from_file(atlas, 16, font_paths[i]);
    if (font)
      break;
  }

  if (!font) {
    fprintf(stderr, "Warning: Could not load any system fonts, text may not display\n");
    return 1;
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

  printf(
      "Font loaded successfully. Atlas size: %zux%zu (tex %u)\n",
      atlas->width,
      atlas->height,
      atlas->id);

  return 1;
}

int init_renderer(void) {
  if (!create_rect_program()) {
    fprintf(stderr, "Failed to create rectangle shader program\n");
    return 0;
  }

  if (!create_text_program()) {
    fprintf(stderr, "Failed to create text shader program\n");
    return 0;
  }

  ensure_rect_buffers();
  ensure_text_buffers();

  if (!init_font_system()) {
    fprintf(stderr, "Failed to initialize font system\n");
    return 0;
  }

  return 1;
}

void cleanup_renderer(void) {
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

void set_projection(float width, float height) {
  mat4_ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f, gProj);
}

void draw_rect(float x, float y, float w, float h, float r, float g, float b) {
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

void draw_text(float x, float y, const char* text, float r, float g, float b) {
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

void draw_text_clipped(
    float x,
    float y,
    const char* text,
    float r,
    float g,
    float b,
    float clip_x,
    float clip_y,
    float clip_w,
    float clip_h) {
  if (!font || !atlas || !text_program)
    return;

  // Enable scissor test for clipping
  glEnable(GL_SCISSOR_TEST);
  // Convert from OpenGL coordinates (bottom-left origin) to scissor coordinates
  // Note: clip_y is the bottom of the clipping area in OpenGL coordinates
  glScissor((GLint)clip_x, (GLint)clip_y, (GLsizei)clip_w, (GLsizei)clip_h);

  // Draw the text normally
  draw_text(x, y, text, r, g, b);

  // Disable scissor test
  glDisable(GL_SCISSOR_TEST);
}
