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
  // sb->scroll_offset = 0.0f; // Keep scroll position
  sb->hovered_tile = -1;
  sb->playing_tile = -1;
  sb->play_start_time = 0;
  sb->sound_duration = 0;

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
      sb->sounds[sb->count].marquee_offset = 0.0f;
      sb->count++;
    }
  }
  closedir(dir);
}

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
            FALSE,  // Don't watch subdirectories
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
      // File change detected
      sb->needs_refresh = 1;
      ResetEvent(overlapped.hEvent);
    } else if (wait_status == WAIT_OBJECT_0 + 1) {
      // Stop event signaled
      break;
    } else {
      // Error
      fprintf(stderr, "WaitForMultipleObjects failed\n");
      break;
    }
  }

  CloseHandle(hDir);
  CloseHandle(overlapped.hEvent);
  return 0;
}

void play_sound(const char* path, Soundboard* sb, int tile_index) {
  PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC);
  sb->playing_tile = tile_index;
  sb->play_start_time = GetTickCount();
  sb->sound_duration = get_sound_duration(path);
}

DWORD get_sound_duration(const char* path) {
  // Open the WAV file to read its header and calculate duration
  HMMIO hmmio;
  MMCKINFO mmckinfoParent;
  MMCKINFO mmckinfoSubchunk;
  WAVEFORMATEX* pwfxInfo;
  DWORD dwFmtSize;
  DWORD dwDataSize = 0;

  hmmio = mmioOpenA((LPSTR)path, NULL, MMIO_READ | MMIO_ALLOCBUF);
  if (hmmio == NULL) {
    return 0;
  }

  // Locate the 'RIFF' chunk with a 'WAVE' form type
  mmckinfoParent.fccType = mmioFOURCC('W', 'A', 'V', 'E');
  if (mmioDescend(hmmio, (LPMMCKINFO)&mmckinfoParent, NULL, MMIO_FINDRIFF)) {
    mmioClose(hmmio, 0);
    return 0;
  }

  // Find the format chunk
  mmckinfoSubchunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
  if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK)) {
    mmioClose(hmmio, 0);
    return 0;
  }

  dwFmtSize = mmckinfoSubchunk.cksize;
  pwfxInfo = (WAVEFORMATEX*)malloc(dwFmtSize);
  if (mmioRead(hmmio, (HPSTR)pwfxInfo, dwFmtSize) != (LONG)dwFmtSize) {
    free(pwfxInfo);
    mmioClose(hmmio, 0);
    return 0;
  }

  mmioAscend(hmmio, &mmckinfoSubchunk, 0);

  // Find the data chunk
  mmckinfoSubchunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
  if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK)) {
    free(pwfxInfo);
    mmioClose(hmmio, 0);
    return 0;
  }

  dwDataSize = mmckinfoSubchunk.cksize;

  // Calculate duration in milliseconds
  DWORD duration = 0;
  if (pwfxInfo->nAvgBytesPerSec > 0) {
    duration = (dwDataSize * 1000) / pwfxInfo->nAvgBytesPerSec;
  }

  free(pwfxInfo);
  mmioClose(hmmio, 0);
  return duration;
}

int is_sound_playing(void) {
  // Check if PlaySound is still playing
  return !PlaySoundA(NULL, NULL, SND_NOSTOP);
}
