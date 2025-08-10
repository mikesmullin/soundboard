#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <freetype-gl/freetype-gl.h>

// Initialize the rendering system
int init_renderer(void);

// Cleanup the rendering system
void cleanup_renderer(void);

// Set the projection matrix for the current window size
void set_projection(float width, float height);

// Draw a filled rectangle
void draw_rect(float x, float y, float w, float h, float r, float g, float b);

// Draw text at the specified position
void draw_text(float x, float y, const char* text, float r, float g, float b);

// Draw text with clipping to a rectangular area
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
    float clip_h);

#endif  // RENDERER_H
