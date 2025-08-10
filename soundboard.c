#include "soundboard.h"

#include <dirent.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>


#pragma comment(lib, "winmm.lib")

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
