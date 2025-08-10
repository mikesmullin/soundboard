#ifndef SOUNDBOARD_H
#define SOUNDBOARD_H

#include <windows.h>

#define MAX_SOUNDS 100
#define TILE_WIDTH 150.0f
#define TILE_HEIGHT 60.0f
#define TILE_SPACING 10.0f
#define REFRESH_BUTTON_WIDTH 80.0f
#define REFRESH_BUTTON_HEIGHT 30.0f
#define MAX_PATH 260

typedef struct {
  char name[MAX_PATH];
  char path[MAX_PATH];
  float marquee_offset;  // For scrolling text
} Sound;

typedef struct {
  Sound sounds[MAX_SOUNDS];
  int count;
  int grid_cols;
  float window_width;
  float window_height;
  float scroll_offset;
  int hovered_tile;  // Index of currently hovered tile (-1 if none)
  int hovered_refresh_button;  // 1 if hovered, 0 otherwise
  int playing_tile;  // Index of currently playing tile (-1 if none)
  DWORD play_start_time;  // Time when playback started
  DWORD sound_duration;  // Duration of currently playing sound in ms
} Soundboard;

// Load sound files from current directory
void load_sounds(Soundboard* sb);

// Play a sound file and track playback
void play_sound(const char* path, Soundboard* sb, int tile_index);

// Get the duration of a WAV file in milliseconds
DWORD get_sound_duration(const char* path);

// Check if a sound is currently playing
int is_sound_playing(void);

#endif  // SOUNDBOARD_H
