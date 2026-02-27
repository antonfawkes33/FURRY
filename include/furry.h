#ifndef FURRY_H
#define FURRY_H

#include <stddef.h>

#define FURRY_MAX_NAME 64
#define FURRY_MAX_TEXT 512
#define FURRY_MAX_LABEL 64
#define FURRY_MAX_KEY 64
#define FURRY_MAX_VALUE 128
#define FURRY_MAX_ASSET 260
#define FURRY_MAX_CHOICES 8
#define FURRY_MAX_VARS 128
#define FURRY_MAX_CALLSTACK 128
#define FURRY_MAX_ERROR_TEXT 256

typedef enum FurryOpCode {
    FURRY_OP_LABEL = 0,
    FURRY_OP_SAY,
    FURRY_OP_GOTO,
    FURRY_OP_CALL,
    FURRY_OP_RETURN,
    FURRY_OP_SET,
    FURRY_OP_ADD,
    FURRY_OP_IF_EQ,
    FURRY_OP_BG,
    FURRY_OP_FG,
    FURRY_OP_BUTTON,
    FURRY_OP_UI_BEGIN,
    FURRY_OP_UI_END,
    FURRY_OP_UI_PANEL,
    FURRY_OP_UI_TEXT,
    FURRY_OP_UI_IMAGE,
    FURRY_OP_UI_ANIM,
    FURRY_OP_UI_VIDEO,
    FURRY_OP_UI_BIND,
    FURRY_OP_MUSIC,
    FURRY_OP_SFX,
    FURRY_OP_SAVE,
    FURRY_OP_LOAD,
    FURRY_OP_CHOICE,
    FURRY_OP_END
} FurryOpCode;

typedef struct FurryChoice {
    char text[FURRY_MAX_TEXT];
    char target[FURRY_MAX_LABEL];
} FurryChoice;

typedef struct FurryInstruction {
    FurryOpCode op;
    char a[FURRY_MAX_TEXT];
    char b[FURRY_MAX_TEXT];
    char c[FURRY_MAX_TEXT];
    int i;
    FurryChoice choices[FURRY_MAX_CHOICES];
    size_t choice_count;
} FurryInstruction;

typedef struct FurryProgram {
    FurryInstruction *code;
    size_t count;
} FurryProgram;

typedef struct FurryVar {
    char key[FURRY_MAX_KEY];
    char value[FURRY_MAX_VALUE];
} FurryVar;

typedef struct FurryRuntimeSnapshot {
    size_t ip;
    size_t callstack_depth;
    size_t callstack[FURRY_MAX_CALLSTACK];
    size_t var_count;
    FurryVar vars[FURRY_MAX_VARS];
} FurryRuntimeSnapshot;

typedef struct FurryRuntimeConfig {
    int max_steps;
    int (*choose_option)(const char *prompt, const FurryChoice *choices, size_t count, void *user_data);
    int (*on_host_command)(FurryOpCode op, const FurryInstruction *ins, const FurryRuntimeSnapshot *snapshot, void *user_data);
    int (*save_slot)(const char *slot, const FurryRuntimeSnapshot *snapshot, void *user_data);
    int (*load_slot)(const char *slot, FurryRuntimeSnapshot *snapshot, void *user_data);
    void *user_data;
} FurryRuntimeConfig;

typedef struct FurryCompileError {
    int line;
    char message[FURRY_MAX_ERROR_TEXT];
} FurryCompileError;

const char *furry_version(void);

int furry_compile_script(const char *script, FurryProgram *out_program);
int furry_compile_script_ex(const char *script, FurryProgram *out_program, FurryCompileError *out_error);
int furry_run_program(const FurryProgram *program, const FurryRuntimeConfig *config);
void furry_free_program(FurryProgram *program);

int furry_snapshot_save(const FurryRuntimeSnapshot *snapshot, char *out_text, size_t out_size);
int furry_snapshot_load(const char *text, FurryRuntimeSnapshot *out_snapshot);
int furry_media_is_supported(const char *asset_path);
const char *furry_audio_backend_name(void);

#endif
