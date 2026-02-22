#include "fr_input.h"
#include <string.h>

static bool keys[FR_KEY_MAX];
static bool keys_last[FR_KEY_MAX];
static bool mouse_buttons[FR_MOUSE_MAX];
static float mouse_x, mouse_y;

void fr_input_init(void) {
  memset(keys, 0, sizeof(keys));
  memset(keys_last, 0, sizeof(keys_last));
  memset(mouse_buttons, 0, sizeof(mouse_buttons));
  mouse_x = 0;
  mouse_y = 0;
}

void fr_input_update_early(void) { memcpy(keys_last, keys, sizeof(keys)); }

void fr_input_set_key(FrKey key, bool down) {
  if (key < FR_KEY_MAX) {
    keys[key] = down;
  }
}

void fr_input_set_mouse_button(FrMouseButton button, bool down) {
  if (button < FR_MOUSE_MAX) {
    mouse_buttons[button] = down;
  }
}

void fr_input_set_mouse_pos(float x, float y) {
  mouse_x = x;
  mouse_y = y;
}

bool fr_key_down(FrKey key) { return key < FR_KEY_MAX ? keys[key] : false; }

bool fr_key_pressed(FrKey key) {
  return (key < FR_KEY_MAX) ? (keys[key] && !keys_last[key]) : false;
}

bool fr_mouse_down(FrMouseButton button) {
  return button < FR_MOUSE_MAX ? mouse_buttons[button] : false;
}

void fr_mouse_pos(float *x, float *y) {
  if (x)
    *x = mouse_x;
  if (y)
    *y = mouse_y;
}
