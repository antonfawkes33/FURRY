#ifndef FURRY_UI_H
#define FURRY_UI_H

#include <stddef.h>

#define FURRY_UI_MAX_BUTTONS 8
#define FURRY_UI_MAX_TEXT 128

typedef struct FurryColor {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} FurryColor;

typedef struct FurryMenuButton {
    char id[FURRY_UI_MAX_TEXT];
    char label[FURRY_UI_MAX_TEXT];
} FurryMenuButton;

typedef struct FurryMainMenuTheme {
    char title[FURRY_UI_MAX_TEXT];
    char subtitle[FURRY_UI_MAX_TEXT];
    FurryColor gradient_top;
    FurryColor gradient_bottom;
    FurryColor accent;
    char font_primary[FURRY_UI_MAX_TEXT];
    char font_mono[FURRY_UI_MAX_TEXT];
    FurryMenuButton buttons[FURRY_UI_MAX_BUTTONS];
    size_t button_count;
} FurryMainMenuTheme;

void furry_ui_default_main_menu(FurryMainMenuTheme *theme);
void furry_ui_dump_main_menu(const FurryMainMenuTheme *theme);

#endif
