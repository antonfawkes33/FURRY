#include <assert.h>
#include <string.h>

#include "furry.h"
#include "furry_ui.h"

static int pick_first(const char *prompt, const FurryChoice *choices, size_t count, void *user_data) {
    (void)prompt;
    (void)choices;
    (void)count;
    (void)user_data;
    return 0;
}

int main(void) {
    assert(strcmp(furry_version(), "0.4.0") == 0);

    const char *script =
        "start:\n"
        "set score=10\n"
        "add score=5\n"
        "if_eq score|15|ok\n"
        "say Narrator|bad\n"
        "ok:\n"
        "choice Next step|Go subroutine->sub|Go end->done\n"
        "sub:\n"
        "call inc\n"
        "goto done\n"
        "inc:\n"
        "add score=10\n"
        "return\n"
        "done:\n"
        "end\n";

    FurryProgram program;
    assert(furry_compile_script(script, &program) == 0);
    assert(program.count > 0);

    FurryRuntimeConfig config = { .max_steps = 4000, .choose_option = pick_first, .user_data = NULL };
    assert(furry_run_program(&program, &config) == 0);
    furry_free_program(&program);

    assert(furry_compile_script("if_eq bad\n", &program) != 0);
    assert(furry_compile_script("choice broken|NoTarget\n", &program) != 0);

    FurryRuntimeSnapshot snap;
    memset(&snap, 0, sizeof(snap));
    snap.ip = 12;
    snap.callstack_depth = 2;
    snap.var_count = 1;
    strcpy(snap.vars[0].key, "route");
    strcpy(snap.vars[0].value, "tech");

    char encoded[128];
    assert(furry_snapshot_save(&snap, encoded, sizeof(encoded)) == 0);

    FurryRuntimeSnapshot decoded;
    assert(furry_snapshot_load(encoded, &decoded) == 0);
    assert(decoded.ip == 12);
    assert(decoded.callstack_depth == 2);
    assert(decoded.var_count == 1);

    FurryMainMenuTheme theme;
    furry_ui_default_main_menu(&theme);
    assert(theme.button_count >= 5);

    return 0;
}
