#ifndef FR_INPUT_H
#define FR_INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  FR_KEY_UNKNOWN = 0,
  FR_KEY_A = 4,
  FR_KEY_B,
  FR_KEY_C,
  FR_KEY_D,
  FR_KEY_E,
  FR_KEY_F,
  FR_KEY_G,
  FR_KEY_H,
  FR_KEY_I,
  FR_KEY_J,
  FR_KEY_K,
  FR_KEY_L,
  FR_KEY_M,
  FR_KEY_N,
  FR_KEY_O,
  FR_KEY_P,
  FR_KEY_Q,
  FR_KEY_R,
  FR_KEY_S,
  FR_KEY_T,
  FR_KEY_U,
  FR_KEY_V,
  FR_KEY_W,
  FR_KEY_X,
  FR_KEY_Y,
  FR_KEY_Z,
  FR_KEY_1 = 30,
  FR_KEY_2,
  FR_KEY_3,
  FR_KEY_4,
  FR_KEY_5,
  FR_KEY_6,
  FR_KEY_7,
  FR_KEY_8,
  FR_KEY_9,
  FR_KEY_0,
  FR_KEY_RETURN = 40,
  FR_KEY_ESCAPE = 41,
  FR_KEY_BACKSPACE = 42,
  FR_KEY_TAB = 43,
  FR_KEY_SPACE = 44,
  FR_KEY_UP = 82,
  FR_KEY_DOWN = 81,
  FR_KEY_LEFT = 80,
  FR_KEY_RIGHT = 79,
  FR_KEY_MAX = 512
} FrKey;

typedef enum {
  FR_MOUSE_LEFT = 0,
  FR_MOUSE_MIDDLE,
  FR_MOUSE_RIGHT,
  FR_MOUSE_MAX
} FrMouseButton;

void fr_input_init(void);
void fr_input_update_early(void); // Reset per-frame state

void fr_input_set_key(FrKey key, bool down);
void fr_input_set_mouse_button(FrMouseButton button, bool down);
void fr_input_set_mouse_pos(float x, float y);

bool fr_key_down(FrKey key);
bool fr_key_pressed(FrKey key); // Only true on the frame it was pressed
bool fr_mouse_down(FrMouseButton button);
void fr_mouse_pos(float *x, float *y);

#endif // FR_INPUT_H
