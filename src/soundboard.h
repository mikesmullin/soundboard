#ifndef SOUNDBOARD_H
#define SOUNDBOARD_H

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sys/types.h>
#endif

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
  uint32_t play_start_time_ms;  // Time when playback started
  uint32_t sound_duration_ms;  // Duration of currently playing sound in ms

  // Filesystem watcher
  volatile int needs_refresh;
#ifdef _WIN32
  HANDLE watcher_thread;
  HANDLE watcher_stop_event;
#else
  pthread_t watcher_thread;
  volatile int watcher_stop;
  pid_t player_pid;
#endif
} Soundboard;

// Load sound files from current directory
void load_sounds(Soundboard* sb);

// Filesystem watcher thread function
#ifdef _WIN32
DWORD WINAPI file_watcher_thread(LPVOID lpParam);
#else
void* file_watcher_thread(void* lpParam);
#endif

// Play a sound file and track playback
void play_sound(const char* path, Soundboard* sb, int tile_index);

// Get the duration of a WAV file in milliseconds
uint32_t get_sound_duration(const char* path);

// Get current monotonic time in milliseconds
uint32_t get_time_ms(void);

#endif  // SOUNDBOARD_H
