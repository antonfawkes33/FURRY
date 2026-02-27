#include <stdio.h>

#include "furry.h"

static int pick_second_choice(const char *prompt, const FurryChoice *choices, size_t count, void *user_data) {
    (void)choices;
    (void)user_data;
    printf("[CHOOSER] %s (%zu options)\n", prompt, count);
    return count > 1 ? 1 : 0;
}

static int host_dispatch(FurryOpCode op, const FurryInstruction *ins, const FurryRuntimeSnapshot *snapshot, void *user_data) {
    (void)user_data;
    (void)snapshot;
    if (op == FURRY_OP_FG) {
        printf("[FG] asset=%s x=%s y=%s rot=%s anim=%s\n", ins->a, ins->b, ins->c, ins->choices[0].text, ins->choices[0].target);
    } else if (op == FURRY_OP_BUTTON) {
        printf("[BUTTON] id=%s label=%s action=%s\n", ins->a, ins->b, ins->c);
    } else if (op == FURRY_OP_BG) {
        printf("[BG] %s\n", ins->a);
    } else if (op == FURRY_OP_MUSIC) {
        printf("[MUSIC:miniaudio] %s\n", ins->a);
    } else if (op == FURRY_OP_SFX) {
        printf("[SFX:miniaudio] %s\n", ins->a);
    } else if (op >= FURRY_OP_UI_BEGIN && op <= FURRY_OP_UI_BIND) {
        printf("[UI] op=%d a=%s b=%s c=%s\n", (int)op, ins->a, ins->b, ins->c);
    }
    return 0;
}

int main(void) {
    printf("FURRY version: %s\n", furry_version());
    printf("Renderer target: Vulkan (frontend-owned implementation)\n");
    printf("Audio backend target: %s\n", furry_audio_backend_name());

    const char *script =
        "boot:\n"
        "bg city_night\n"
        "fg guide_idle.png|0.50|0.90|0|idle\n"
        "ui_begin main_hud\n"
        "ui_panel root|0.0|0.0|1.0|1.0\n"
        "ui_text title|FURRY VN Runtime\n"
        "ui_image logo|logo.webp\n"
        "ui_anim pulse|pulse.anim|loop\n"
        "ui_video intro|intro.mp4|1\n"
        "ui_bind title|route\n"
        "button start_btn|Start Story|investigate\n"
        "ui_end\n"
        "music ui_theme.ogg\n"
        "say Narrator|Welcome to FURRY, bring your own UI frontend.\n"
        "save quick\n"
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
    FurryCompileError compile_error;
    if (furry_compile_script_ex(script, &program, &compile_error) != 0) {
        fprintf(stderr, "compile failed at line %d: %s\n", compile_error.line, compile_error.message);
        return 1;
    }

    FurryRuntimeConfig config = {
        .max_steps = 20000,
        .choose_option = pick_second_choice,
        .on_host_command = host_dispatch,
        .save_slot = NULL,
        .load_slot = NULL,
        .user_data = NULL
    };
    int rc = furry_run_program(&program, &config);
    furry_free_program(&program);

    return rc == 0 ? 0 : 1;
}
