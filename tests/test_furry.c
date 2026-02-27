#include <assert.h>
#include <string.h>

#include "furry.h"

static int pick_first(const char *prompt, const FurryChoice *choices, size_t count, void *user_data) {
    (void)prompt;
    (void)choices;
    (void)count;
    (void)user_data;
    return 0;
}

typedef struct TestHostState {
    int host_calls;
    int save_calls;
    int load_calls;
} TestHostState;

static int on_host(FurryOpCode op, const FurryInstruction *ins, const FurryRuntimeSnapshot *snapshot, void *user_data) {
    TestHostState *state = (TestHostState *)user_data;
    (void)snapshot;
    if (op == FURRY_OP_FG) {
        assert(strcmp(ins->a, "hero_idle.png") == 0);
        assert(strcmp(ins->b, "0.5") == 0);
    } else if (op == FURRY_OP_UI_VIDEO) {
        assert(strcmp(ins->b, "scene.mp4") == 0);
    }
    state->host_calls++;
    return 0;
}

static int save_slot(const char *slot, const FurryRuntimeSnapshot *snapshot, void *user_data) {
    TestHostState *state = (TestHostState *)user_data;
    assert(strcmp(slot, "autosave") == 0);
    assert(snapshot != NULL);
    state->save_calls++;
    return 0;
}

static int load_slot(const char *slot, FurryRuntimeSnapshot *snapshot, void *user_data) {
    TestHostState *state = (TestHostState *)user_data;
    assert(strcmp(slot, "autosave") == 0);
    assert(snapshot != NULL);
    snapshot->ip += 1;
    state->load_calls++;
    return 0;
}

int main(void) {
    assert(strcmp(furry_version(), "0.4.0") == 0);
    assert(strcmp(furry_audio_backend_name(), "miniaudio") == 0 || strcmp(furry_audio_backend_name(), "none") == 0);

    const char *script =
        "start:\n"
        "fg hero_idle.png|0.5|0.9|0|fade_in\n"
        "ui_begin root\n"
        "ui_panel root_panel|0|0|1|1\n"
        "ui_text caption|Hello\n"
        "ui_image portrait|portrait.webp\n"
        "ui_anim pulse|pulse.anim|loop\n"
        "ui_video cinematic|scene.mp4|1\n"
        "ui_bind caption|route\n"
        "button continue_btn|Continue|ok\n"
        "ui_end\n"
        "set score=10\n"
        "add score=5\n"
        "if_eq score|15|ok\n"
        "say Narrator|bad\n"
        "ok:\n"
        "save autosave\n"
        "load autosave\n"
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

    TestHostState host_state = {0};
    FurryRuntimeConfig config = {
        .max_steps = 4000,
        .choose_option = pick_first,
        .on_host_command = on_host,
        .save_slot = save_slot,
        .load_slot = load_slot,
        .user_data = &host_state
    };
    assert(furry_run_program(&program, &config) == 0);
    furry_free_program(&program);
    assert(host_state.host_calls >= 2);
    assert(host_state.save_calls == 1);
    assert(host_state.load_calls == 1);

    assert(furry_compile_script("if_eq bad\n", &program) != 0);
    assert(furry_compile_script("choice broken|NoTarget\n", &program) != 0);

    FurryCompileError compile_error;
    assert(furry_compile_script_ex("ui_video clip|broken.xyz|1\n", &program, &compile_error) != 0);
    assert(compile_error.line > 0);
    assert(strstr(compile_error.message, "invalid syntax") != NULL);

    assert(furry_compile_script_ex("start:\ngoto missing\nend\n", &program, &compile_error) != 0);
    assert(strstr(compile_error.message, "unknown label target") != NULL);

    assert(furry_media_is_supported("video.mp4") == 1);
    assert(furry_media_is_supported("anim.gif") == 1);
    assert(furry_media_is_supported("archive.zip") == 0);

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

    return 0;
}
