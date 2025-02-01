#include <NDL.h>
#include <SDL.h>
#include <assert.h>

static void (*audio_callback)(void *userdata, uint8_t *stream, int len) = NULL;
static void *callback_userdata = NULL;

static SDL_AudioSpec audio_spec = { 0 };

static uint64_t last_callback_time   = 0;
static uint64_t callback_interval_ms = 0;

static bool audio_inited = false;
static bool audio_paused = true;

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
  assert(desired != NULL);

  audio_callback = desired->callback;
  callback_userdata = desired->userdata;

  audio_spec = *desired;

  double ms = (double)audio_spec.samples * 1000.0 / (double)audio_spec.freq;  // 计算回调间隔
  callback_interval_ms = (uint64_t)(((uint64_t)ms == 0) ? 1 : ms);

  NDL_OpenAudio(desired->freq, desired->channels, desired->samples);

  audio_inited = true;
  audio_paused = false;

  last_callback_time = NDL_GetTicks();

  if (obtained) {
    *obtained = *desired;
  }

  return 0;
}

void SDL_CloseAudio() {
  SDL_AudioSpec empty = { 0 };

  // Reset all variables
  audio_paused         = true;
  audio_inited         = false;
  audio_callback       = NULL;
  callback_userdata    = NULL;
  last_callback_time   = 0;
  callback_interval_ms = 0;
  audio_spec           = empty;

  NDL_CloseAudio();
}

void SDL_PauseAudio(int pause_on) {
  audio_paused = (pause_on != 0);
}

void SDL_MixAudio(uint8_t *dst, uint8_t *src, uint32_t len, int volume) {
}

SDL_AudioSpec *SDL_LoadWAV(const char *file, SDL_AudioSpec *spec, uint8_t **audio_buf, uint32_t *audio_len) {
  return NULL;
}

void SDL_FreeWAV(uint8_t *audio_buf) {
}

void SDL_LockAudio() {
}

void SDL_UnlockAudio() {
}

#define BUFSIZE 8192
static uint8_t internal_buf[BUFSIZE];

// 回调辅助函数
void CallbackHelper() {
  if (!audio_inited || !audio_callback) {
    printf("[%s] null audio_inited(%d) | audio_callback(%p)", __func__, audio_inited, audio_callback);
    return;
  }

  uint64_t now = NDL_GetTicks();
  if (now - last_callback_time < callback_interval_ms) {
    return;
  }

  last_callback_time = now;

  if (audio_paused) {
    return;
  }

  int free_bytes = NDL_QueryAudio();
  if (free_bytes <= 0) {
    return;
  }

  int bytes_per_sample = audio_spec.format / sizeof(uint8_t);
  int chunk_bytes = audio_spec.samples * audio_spec.channels * bytes_per_sample;

  if (chunk_bytes > free_bytes) {
    chunk_bytes = free_bytes;
  }

  if (chunk_bytes <= 0) {
    return;
  }

  assert(chunk_bytes <= sizeof(internal_buf));

  audio_callback(callback_userdata, internal_buf, chunk_bytes);

  int written = NDL_PlayAudio(internal_buf, chunk_bytes);
  assert(written == chunk_bytes);
}
