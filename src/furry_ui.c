#include "furry_ui.h"

#include <stdio.h>
#include <string.h>

static void add_button(FurryMainMenuTheme *theme, const char *id, const char *label) {
    if (theme->button_count >= FURRY_UI_MAX_BUTTONS) {
        return;
    }
    FurryMenuButton *button = &theme->buttons[theme->button_count++];
    snprintf(button->id, sizeof(button->id), "%s", id);
    snprintf(button->label, sizeof(button->label), "%s", label);
}

void furry_ui_default_main_menu(FurryMainMenuTheme *theme) {
    if (theme == NULL) {
        return;
    }
    memset(theme, 0, sizeof(*theme));

    snprintf(theme->title, sizeof(theme->title), "%s", "FURRY");
    snprintf(theme->subtitle, sizeof(theme->subtitle), "%s", "C + Vulkan + SDL3 Visual Novel Engine");

    theme->gradient_top = (FurryColor){20, 26, 42, 255};
    theme->gradient_bottom = (FurryColor){8, 10, 18, 255};
    theme->accent = (FurryColor){83, 196, 255, 255};

    snprintf(theme->font_primary, sizeof(theme->font_primary), "%s", "Inter");
    snprintf(theme->font_mono, sizeof(theme->font_mono), "%s", "JetBrains Mono");

    add_button(theme, "new_game", "New Story");
    add_button(theme, "continue", "Continue");
    add_button(theme, "scenes", "Scene Browser");
    add_button(theme, "mods", "Mod Loader");
    add_button(theme, "settings", "Settings");
    add_button(theme, "quit", "Quit");
}

void furry_ui_dump_main_menu(const FurryMainMenuTheme *theme) {
    if (theme == NULL) {
        return;
    }

    printf("== Modern Main Menu ==\n");
    printf("Title: %s\n", theme->title);
    printf("Subtitle: %s\n", theme->subtitle);
    printf("Gradient: rgba(%u,%u,%u,%u) -> rgba(%u,%u,%u,%u)\n",
           theme->gradient_top.r, theme->gradient_top.g, theme->gradient_top.b, theme->gradient_top.a,
           theme->gradient_bottom.r, theme->gradient_bottom.g, theme->gradient_bottom.b, theme->gradient_bottom.a);
    printf("Accent: rgba(%u,%u,%u,%u)\n", theme->accent.r, theme->accent.g, theme->accent.b, theme->accent.a);
    printf("Fonts: %s / %s\n", theme->font_primary, theme->font_mono);

    for (size_t i = 0; i < theme->button_count; ++i) {
        printf("[%zu] %s (%s)\n", i + 1, theme->buttons[i].label, theme->buttons[i].id);
    }
}
