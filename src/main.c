#include <stdio.h>

#include "furry.h"
#include "furry_ui.h"

static int pick_second_choice(const char *prompt, const FurryChoice *choices, size_t count, void *user_data) {
    (void)choices;
    (void)user_data;
    printf("[CHOOSER] %s (%zu options)\n", prompt, count);
    return count > 1 ? 1 : 0;
}

int main(void) {
    FurryMainMenuTheme menu;
    furry_ui_default_main_menu(&menu);

    printf("FURRY version: %s\n", furry_version());
    furry_ui_dump_main_menu(&menu);

    const char *script =
        "boot:\n"
        "bg city_night\n"
        "music ui_theme.ogg\n"
        "say Narrator|Welcome to FURRY, a modern VN runtime.\n"
        "choice Pick your path|Investigate->investigate|Socialize->social\n"
        "investigate:\n"
        "call tech_scene\n"
        "goto end_scene\n"
        "social:\n"
        "say Guide|You chose the social branch.\n"
        "goto end_scene\n"
        "tech_scene:\n"
        "sfx select.wav\n"
        "set route=tech\n"
        "add affinity=5\n"
        "if_eq route|tech|fast_lane\n"
        "say Guide|This should not appear when route=tech\n"
        "fast_lane:\n"
        "say System|Affinity raised by 5.\n"
        "return\n"
        "end_scene:\n"
        "end\n";

    FurryProgram program;
    if (furry_compile_script(script, &program) != 0) {
        fprintf(stderr, "compile failed\n");
        return 1;
    }

    FurryRuntimeConfig config = { .max_steps = 20000, .choose_option = pick_second_choice, .user_data = NULL };
    int rc = furry_run_program(&program, &config);
    furry_free_program(&program);

    return rc == 0 ? 0 : 1;
}
