#ifndef FR_PLATFORM_H
#define FR_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct FrWindow FrWindow;
struct SDL_Window;

struct FrPlatformInfo {
  const char *title;
  uint32_t width;
  uint32_t height;
  bool fullscreen;
};

bool fr_platform_init(void);
void fr_platform_shutdown(void);

struct FrWindow *fr_window_create(const struct FrPlatformInfo *info);
void fr_window_destroy(struct FrWindow *window);
bool fr_window_should_close(struct FrWindow *window);
void fr_window_poll_events(struct FrWindow *window);

// New API for renderer sync
void fr_window_get_size(struct FrWindow *window, int *width, int *height);
bool fr_window_is_minimized(struct FrWindow *window);

// Getters for internal/renderer use
struct SDL_Window *fr_window_get_handle(struct FrWindow *window);

double fr_platform_get_time(void);

#endif // FR_PLATFORM_H
