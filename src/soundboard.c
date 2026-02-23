#include "soundboard.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#define str_casecmp _stricmp
#else
#include <spawn.h>
#include <signal.h>
#include <strings.h>
#include <sys/wait.h>
#include <unistd.h>
#define str_casecmp strcasecmp
extern char** environ;
#endif

static uint32_t read_u32_le(const unsigned char* bytes) {
  return ((uint32_t)bytes[0]) | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) |
         ((uint32_t)bytes[3] << 24);
}

static int is_directory_mode(mode_t mode) {
  return S_ISDIR(mode);
}

static int is_regular_mode(mode_t mode) {
  return S_ISREG(mode);
}

void find_sounds_recursive(const char* base_path, Soundboard* sb) {
  DIR* dir;
  struct dirent* entry;
  char path[MAX_PATH];

  if (!(dir = opendir(base_path)))
    return;

  while ((entry = readdir(dir)) != NULL && sb->count < MAX_SOUNDS) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);

    struct stat s;
    if (stat(path, &s) == 0) {
      if (is_directory_mode(s.st_mode)) {
        find_sounds_recursive(path, sb);
      } else if (is_regular_mode(s.st_mode)) {
        char* ext = strrchr(entry->d_name, '.');
        if (ext && str_casecmp(ext, ".wav") == 0) {
          snprintf(sb->sounds[sb->count].name, MAX_PATH, "%s", path);
          snprintf(sb->sounds[sb->count].path, MAX_PATH, "%s", path);
          sb->sounds[sb->count].marquee_offset = 0.0f;
          sb->count++;
        }
      }
    }
  }

  closedir(dir);
}

void load_sounds(Soundboard* sb) {
  sb->count = 0;
  sb->hovered_tile = -1;
  sb->playing_tile = -1;
  sb->play_start_time_ms = 0;
  sb->sound_duration_ms = 0;

  find_sounds_recursive(".", sb);
}

#ifdef _WIN32
DWORD WINAPI file_watcher_thread(LPVOID lpParam) {
  Soundboard* sb = (Soundboard*)lpParam;
  char path[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, path);

  HANDLE hDir = CreateFile(
      path,
      FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
      NULL);

  if (hDir == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Failed to create file handle for watcher\n");
    return 1;
  }

  OVERLAPPED overlapped;
  overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (!overlapped.hEvent) {
    fprintf(stderr, "Failed to create event for watcher\n");
    CloseHandle(hDir);
    return 1;
  }

  BYTE buffer[4096];
  DWORD bytes_returned;

  while (1) {
    if (!ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytes_returned,
            &overlapped,
            NULL)) {
      fprintf(stderr, "ReadDirectoryChangesW failed\n");
      break;
    }

    HANDLE handles[2] = {overlapped.hEvent, sb->watcher_stop_event};
    DWORD wait_status = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

    if (wait_status == WAIT_OBJECT_0) {
      sb->needs_refresh = 1;
      ResetEvent(overlapped.hEvent);
    } else if (wait_status == WAIT_OBJECT_0 + 1) {
      break;
    } else {
      fprintf(stderr, "WaitForMultipleObjects failed\n");
      break;
    }
  }

  CloseHandle(hDir);
  CloseHandle(overlapped.hEvent);
  return 0;
}
#else
static uint64_t compute_tree_signature(const char* base_path) {
  DIR* dir = opendir(base_path);
  if (!dir)
    return 0;

  struct dirent* entry;
  uint64_t signature = 1469598103934665603ULL;
  char path[MAX_PATH];

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);

    struct stat s;
    if (stat(path, &s) != 0)
      continue;

    signature ^= (uint64_t)s.st_mtime;
    signature *= 1099511628211ULL;
    signature ^= (uint64_t)s.st_size;
    signature *= 1099511628211ULL;

    if (is_directory_mode(s.st_mode)) {
      signature ^= compute_tree_signature(path);
      signature *= 1099511628211ULL;
    }
  }

  closedir(dir);
  return signature;
}

void* file_watcher_thread(void* lpParam) {
  Soundboard* sb = (Soundboard*)lpParam;
  uint64_t last_signature = compute_tree_signature(".");

  while (!sb->watcher_stop) {
    uint64_t current_signature = compute_tree_signature(".");
    if (current_signature != last_signature) {
      sb->needs_refresh = 1;
      last_signature = current_signature;
    }

    struct timespec sleep_interval;
    sleep_interval.tv_sec = 0;
    sleep_interval.tv_nsec = 500000000L;
    nanosleep(&sleep_interval, NULL);
  }

  return NULL;
}
#endif

void play_sound(const char* path, Soundboard* sb, int tile_index) {
  sb->sound_duration_ms = get_sound_duration(path);
  sb->playing_tile = tile_index;
  sb->play_start_time_ms = get_time_ms();

#ifdef _WIN32
  PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC);
#else
  if (sb->player_pid > 0) {
    int status = 0;
    pid_t wait_result = waitpid(sb->player_pid, &status, WNOHANG);
    if (wait_result == 0) {
      kill(sb->player_pid, SIGTERM);
      waitpid(sb->player_pid, NULL, 0);
    }
    sb->player_pid = 0;
  }

  struct {
    const char* cmd;
    char* const* argv;
  } backends[5];

  char* const paplay_argv[] = {"paplay", (char*)path, NULL};
  char* const mpv_argv[] = {"mpv", "--no-video", "--really-quiet", (char*)path, NULL};
  char* const pw_play_argv[] = {"pw-play", (char*)path, NULL};
  char* const aplay_argv[] = {"aplay", "-q", (char*)path, NULL};
  char* const ffplay_argv[] = {
      "ffplay", "-nodisp", "-autoexit", "-loglevel", "quiet", (char*)path, NULL};

  backends[0].cmd = "paplay";
  backends[0].argv = paplay_argv;
  backends[1].cmd = "mpv";
  backends[1].argv = mpv_argv;
  backends[2].cmd = "pw-play";
  backends[2].argv = pw_play_argv;
  backends[3].cmd = "aplay";
  backends[3].argv = aplay_argv;
  backends[4].cmd = "ffplay";
  backends[4].argv = ffplay_argv;

  int launched = 0;
  for (int i = 0; i < 5; i++) {
    pid_t pid = 0;
    int spawn_result = posix_spawnp(&pid, backends[i].cmd, NULL, NULL, backends[i].argv, environ);
    if (spawn_result == 0) {
      sb->player_pid = pid;
      launched = 1;
      break;
    }
  }

  if (!launched) {
    fprintf(stderr, "Failed to start audio player (tried paplay, mpv, pw-play, aplay, ffplay)\n");
  }
#endif
}

uint32_t get_sound_duration(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f)
    return 0;

  unsigned char header[12];
  if (fread(header, 1, sizeof(header), f) != sizeof(header)) {
    fclose(f);
    return 0;
  }

  if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
    fclose(f);
    return 0;
  }

  uint32_t byte_rate = 0;
  uint32_t data_size = 0;

  while (!feof(f)) {
    unsigned char chunk_header[8];
    if (fread(chunk_header, 1, sizeof(chunk_header), f) != sizeof(chunk_header))
      break;

    uint32_t chunk_size = read_u32_le(chunk_header + 4);

    if (memcmp(chunk_header, "fmt ", 4) == 0) {
      if (chunk_size < 16) {
        fclose(f);
        return 0;
      }

      unsigned char fmt_data[16];
      if (fread(fmt_data, 1, sizeof(fmt_data), f) != sizeof(fmt_data)) {
        fclose(f);
        return 0;
      }

      byte_rate = read_u32_le(fmt_data + 8);

      if (chunk_size > 16) {
        long extra = (long)(chunk_size - 16);
        if (fseek(f, extra, SEEK_CUR) != 0) {
          fclose(f);
          return 0;
        }
      }
    } else if (memcmp(chunk_header, "data", 4) == 0) {
      data_size = chunk_size;
      if (fseek(f, (long)chunk_size, SEEK_CUR) != 0) {
        fclose(f);
        return 0;
      }
    } else {
      if (fseek(f, (long)chunk_size, SEEK_CUR) != 0) {
        fclose(f);
        return 0;
      }
    }

    if (chunk_size & 1) {
      if (fseek(f, 1, SEEK_CUR) != 0) {
        fclose(f);
        return 0;
      }
    }
  }

  fclose(f);

  if (byte_rate == 0 || data_size == 0)
    return 0;

  return (uint32_t)(((uint64_t)data_size * 1000ULL) / (uint64_t)byte_rate);
}

uint32_t get_time_ms(void) {
#ifdef _WIN32
  return (uint32_t)GetTickCount();
#else
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    return 0;

  return (uint32_t)((uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL);
#endif
}
