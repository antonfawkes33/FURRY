#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stdlib.h>

#include "../core/fr_log.h"
#include "fr_input.h"
#include "fr_platform.h"


struct FrWindow {
  SDL_Window *handle;
  bool should_close;
};

bool fr_platform_init(void) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
    FR_ERROR("Failed to initialize SDL: %s", SDL_GetError());
    return false;
  }
  fr_input_init();
  FR_INFO("SDL initialized successfully");
  return true;
}

void fr_platform_shutdown(void) {
  SDL_Quit();
  FR_INFO("SDL shutdown");
}

struct FrWindow *fr_window_create(const struct FrPlatformInfo *info) {
  SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
  if (info->fullscreen)
    flags |= SDL_WINDOW_FULLSCREEN;

  SDL_Window *handle =
      SDL_CreateWindow(info->title, info->width, info->height, flags);
  if (!handle) {
    FR_ERROR("Failed to create SDL window: %s", SDL_GetError());
    return NULL;
  }

  struct FrWindow *window = (struct FrWindow *)malloc(sizeof(struct FrWindow));
  if (!window) {
    SDL_DestroyWindow(handle);
    return NULL;
  }
  window->handle = handle;
  window->should_close = false;

  FR_INFO("Window '%s' (%dx%d) created", info->title, info->width,
          info->height);
  return window;
}

void fr_window_destroy(struct FrWindow *window) {
  if (window) {
    SDL_DestroyWindow(window->handle);
    free(window);
  }
}

bool fr_window_should_close(struct FrWindow *window) {
  return window ? window->should_close : true;
}

void fr_window_poll_events(struct FrWindow *window) {
  if (!window)
    return;

  fr_input_update_early();

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      window->should_close = true;
      break;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      window->should_close = true;
      break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      fr_input_set_key((FrKey)event.key.scancode, event.key.down);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      fr_input_set_mouse_button((FrMouseButton)(event.button.button - 1),
                                event.button.down);
      break;
    case SDL_EVENT_MOUSE_MOTION:
      fr_input_set_mouse_pos(event.motion.x, event.motion.y);
      break;
    }
  }
}

void fr_window_get_size(struct FrWindow *window, int *width, int *height) {
  if (window && window->handle) {
    SDL_GetWindowSizeInPixels(window->handle, width, height);
  }
}

bool fr_window_is_minimized(struct FrWindow *window) {
  if (!window || !window->handle)
    return true;
  SDL_WindowFlags flags = SDL_GetWindowFlags(window->handle);
  return (flags & SDL_WINDOW_MINIMIZED) != 0;
}

struct SDL_Window *fr_window_get_handle(struct FrWindow *window) {
  return window ? window->handle : NULL;
}

double fr_platform_get_time(void) { return (double)SDL_GetTicks() / 1000.0; }
