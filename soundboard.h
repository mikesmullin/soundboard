#ifndef SOUNDBOARD_H
#define SOUNDBOARD_H

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

// Load sound files from current directory
void load_sounds(Soundboard* sb);

// Play a sound file
void play_sound(const char* path);

#endif  // SOUNDBOARD_H
