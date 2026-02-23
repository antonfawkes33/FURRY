#include "furry.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FURRY_OK 0
#define FURRY_ERR 1

typedef struct RuntimeState {
    FurryRuntimeSnapshot snap;
} RuntimeState;

static void trim(char *line) {
    size_t len = strlen(line);
    while (len > 0 && isspace((unsigned char)line[len - 1])) {
        line[--len] = '\0';
    }

    size_t offset = 0;
    while (line[offset] != '\0' && isspace((unsigned char)line[offset])) {
        offset++;
    }

    if (offset > 0) {
        memmove(line, line + offset, strlen(line + offset) + 1);
    }
}

static int safe_copy(char *dst, size_t dst_size, const char *src) {
    size_t n = strlen(src);
    if (n >= dst_size) {
        return FURRY_ERR;
    }
    memcpy(dst, src, n + 1);
    return FURRY_OK;
}

static int append_instruction(FurryProgram *program, const FurryInstruction *ins) {
    size_t next = program->count + 1;
    FurryInstruction *resized = realloc(program->code, next * sizeof(FurryInstruction));
    if (resized == NULL) {
        return FURRY_ERR;
    }
    program->code = resized;
    program->code[program->count] = *ins;
    program->count = next;
    return FURRY_OK;
}

static int parse_choice_option(const char *token, FurryChoice *out) {
    char temp[FURRY_MAX_TEXT * 2];
    if (safe_copy(temp, sizeof(temp), token) != FURRY_OK) {
        return FURRY_ERR;
    }
    char *arrow = strstr(temp, "->");
    if (arrow == NULL) {
        return FURRY_ERR;
    }
    *arrow = '\0';
    char *text = temp;
    char *target = arrow + 2;
    trim(text);
    trim(target);
    if (safe_copy(out->text, sizeof(out->text), text) != FURRY_OK ||
        safe_copy(out->target, sizeof(out->target), target) != FURRY_OK) {
        return FURRY_ERR;
    }
    return FURRY_OK;
}

const char *furry_version(void) {
    return "0.4.0";
}

int furry_compile_script(const char *script, FurryProgram *out_program) {
    if (script == NULL || out_program == NULL) {
        return FURRY_ERR;
    }

    out_program->code = NULL;
    out_program->count = 0;

    char *buffer = malloc(strlen(script) + 1);
    if (buffer == NULL) {
        return FURRY_ERR;
    }
    strcpy(buffer, script);

    char *line = strtok(buffer, "\n");
    while (line != NULL) {
        trim(line);
        if (line[0] == '\0' || line[0] == '#') {
            line = strtok(NULL, "\n");
            continue;
        }

        FurryInstruction ins;
        memset(&ins, 0, sizeof(ins));

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == ':') {
            line[len - 1] = '\0';
            trim(line);
            ins.op = FURRY_OP_LABEL;
            if (safe_copy(ins.a, sizeof(ins.a), line) != FURRY_OK || append_instruction(out_program, &ins) != FURRY_OK) {
                furry_free_program(out_program);
                free(buffer);
                return FURRY_ERR;
            }
            line = strtok(NULL, "\n");
            continue;
        }

        if (strncmp(line, "say ", 4) == 0) {
            char *payload = line + 4;
            char *sep = strchr(payload, '|');
            if (sep == NULL) {
                goto compile_error;
            }
            *sep = '\0';
            trim(payload);
            trim(sep + 1);
            ins.op = FURRY_OP_SAY;
            if (safe_copy(ins.a, sizeof(ins.a), payload) != FURRY_OK ||
                safe_copy(ins.b, sizeof(ins.b), sep + 1) != FURRY_OK) {
                goto compile_error;
            }
        } else if (strncmp(line, "goto ", 5) == 0) {
            ins.op = FURRY_OP_GOTO;
            trim(line + 5);
            if (safe_copy(ins.a, sizeof(ins.a), line + 5) != FURRY_OK) {
                goto compile_error;
            }
        } else if (strncmp(line, "call ", 5) == 0) {
            ins.op = FURRY_OP_CALL;
            trim(line + 5);
            if (safe_copy(ins.a, sizeof(ins.a), line + 5) != FURRY_OK) {
                goto compile_error;
            }
        } else if (strcmp(line, "return") == 0) {
            ins.op = FURRY_OP_RETURN;
        } else if (strncmp(line, "set ", 4) == 0) {
            ins.op = FURRY_OP_SET;
            char *key = line + 4;
            char *sep = strchr(key, '=');
            if (sep == NULL) {
                goto compile_error;
            }
            *sep = '\0';
            trim(key);
            trim(sep + 1);
            if (safe_copy(ins.a, sizeof(ins.a), key) != FURRY_OK ||
                safe_copy(ins.b, sizeof(ins.b), sep + 1) != FURRY_OK) {
                goto compile_error;
            }
        } else if (strncmp(line, "add ", 4) == 0) {
            ins.op = FURRY_OP_ADD;
            char *key = line + 4;
            char *sep = strchr(key, '=');
            if (sep == NULL) {
                goto compile_error;
            }
            *sep = '\0';
            trim(key);
            trim(sep + 1);
            ins.i = atoi(sep + 1);
            if (safe_copy(ins.a, sizeof(ins.a), key) != FURRY_OK) {
                goto compile_error;
            }
        } else if (strncmp(line, "if_eq ", 6) == 0) {
            ins.op = FURRY_OP_IF_EQ;
            char *payload = line + 6;
            char *p1 = strchr(payload, '|');
            if (p1 == NULL) {
                goto compile_error;
            }
            *p1 = '\0';
            char *p2 = strchr(p1 + 1, '|');
            if (p2 == NULL) {
                goto compile_error;
            }
            *p2 = '\0';
            trim(payload);
            trim(p1 + 1);
            trim(p2 + 1);
            if (safe_copy(ins.a, sizeof(ins.a), payload) != FURRY_OK ||
                safe_copy(ins.b, sizeof(ins.b), p1 + 1) != FURRY_OK ||
                safe_copy(ins.c, sizeof(ins.c), p2 + 1) != FURRY_OK) {
                goto compile_error;
            }
        } else if (strncmp(line, "bg ", 3) == 0) {
            ins.op = FURRY_OP_BG;
            trim(line + 3);
            if (safe_copy(ins.a, sizeof(ins.a), line + 3) != FURRY_OK) {
                goto compile_error;
            }
        } else if (strncmp(line, "music ", 6) == 0) {
            ins.op = FURRY_OP_MUSIC;
            trim(line + 6);
            if (safe_copy(ins.a, sizeof(ins.a), line + 6) != FURRY_OK) {
                goto compile_error;
            }
        } else if (strncmp(line, "sfx ", 4) == 0) {
            ins.op = FURRY_OP_SFX;
            trim(line + 4);
            if (safe_copy(ins.a, sizeof(ins.a), line + 4) != FURRY_OK) {
                goto compile_error;
            }
        } else if (strncmp(line, "choice ", 7) == 0) {
            ins.op = FURRY_OP_CHOICE;
            char choice_buf[FURRY_MAX_TEXT * 4];
            if (safe_copy(choice_buf, sizeof(choice_buf), line + 7) != FURRY_OK) {
                goto compile_error;
            }

            char *cursor = choice_buf;
            char *sep = strchr(cursor, '|');
            if (sep == NULL) {
                goto compile_error;
            }
            *sep = '\0';
            trim(cursor);
            if (safe_copy(ins.a, sizeof(ins.a), cursor) != FURRY_OK) {
                goto compile_error;
            }

            cursor = sep + 1;
            while (*cursor != '\0') {
                char *next = strchr(cursor, '|');
                if (next != NULL) {
                    *next = '\0';
                }
                trim(cursor);
                if (ins.choice_count >= FURRY_MAX_CHOICES) {
                    goto compile_error;
                }
                if (parse_choice_option(cursor, &ins.choices[ins.choice_count]) != FURRY_OK) {
                    goto compile_error;
                }
                ins.choice_count++;
                if (next == NULL) {
                    break;
                }
                cursor = next + 1;
            }

            if (ins.choice_count == 0) {
                goto compile_error;
            }
        } else if (strcmp(line, "end") == 0) {
            ins.op = FURRY_OP_END;
        } else {
            goto compile_error;
        }

        if (append_instruction(out_program, &ins) != FURRY_OK) {
            goto compile_error;
        }

        line = strtok(NULL, "\n");
        continue;

compile_error:
        furry_free_program(out_program);
        free(buffer);
        return FURRY_ERR;
    }

    free(buffer);
    return FURRY_OK;
}

static int find_label(const FurryProgram *program, const char *label) {
    for (size_t i = 0; i < program->count; ++i) {
        if (program->code[i].op == FURRY_OP_LABEL && strcmp(program->code[i].a, label) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static const char *get_var(const RuntimeState *state, const char *key) {
    for (size_t i = 0; i < state->snap.var_count; ++i) {
        if (strcmp(state->snap.vars[i].key, key) == 0) {
            return state->snap.vars[i].value;
        }
    }
    return "";
}

static int set_var(RuntimeState *state, const char *key, const char *value) {
    for (size_t i = 0; i < state->snap.var_count; ++i) {
        if (strcmp(state->snap.vars[i].key, key) == 0) {
            return safe_copy(state->snap.vars[i].value, sizeof(state->snap.vars[i].value), value);
        }
    }

    if (state->snap.var_count >= FURRY_MAX_VARS) {
        return FURRY_ERR;
    }

    if (safe_copy(state->snap.vars[state->snap.var_count].key, sizeof(state->snap.vars[state->snap.var_count].key), key) != FURRY_OK ||
        safe_copy(state->snap.vars[state->snap.var_count].value, sizeof(state->snap.vars[state->snap.var_count].value), value) != FURRY_OK) {
        return FURRY_ERR;
    }

    state->snap.var_count++;
    return FURRY_OK;
}

static int choose_default(const char *prompt, const FurryChoice *choices, size_t count, void *user_data) {
    (void)user_data;
    printf("[CHOICE] %s\n", prompt);
    for (size_t i = 0; i < count; ++i) {
        printf("  %zu) %s\n", i + 1, choices[i].text);
    }
    return 0;
}

static int jump_to_label(const FurryProgram *program, RuntimeState *state, const char *label) {
    int idx = find_label(program, label);
    if (idx < 0) {
        return FURRY_ERR;
    }
    state->snap.ip = (size_t)idx + 1;
    return FURRY_OK;
}

int furry_run_program(const FurryProgram *program, const FurryRuntimeConfig *config) {
    if (program == NULL || program->code == NULL || program->count == 0) {
        return FURRY_ERR;
    }

    int max_steps = 50000;
    RuntimeState state;
    memset(&state, 0, sizeof(state));

    int (*choose_fn)(const char *, const FurryChoice *, size_t, void *) = choose_default;
    void *choice_user_data = NULL;
    if (config != NULL) {
        if (config->max_steps > 0) {
            max_steps = config->max_steps;
        }
        if (config->choose_option != NULL) {
            choose_fn = config->choose_option;
        }
        choice_user_data = config->user_data;
    }

    int steps = 0;
    while (state.snap.ip < program->count && steps < max_steps) {
        const FurryInstruction *ins = &program->code[state.snap.ip];

        switch (ins->op) {
            case FURRY_OP_LABEL:
                state.snap.ip++;
                break;
            case FURRY_OP_SAY:
                printf("%s: %s\n", ins->a, ins->b);
                state.snap.ip++;
                break;
            case FURRY_OP_GOTO:
                if (jump_to_label(program, &state, ins->a) != FURRY_OK) {
                    return FURRY_ERR;
                }
                break;
            case FURRY_OP_CALL:
                if (state.snap.callstack_depth >= FURRY_MAX_CALLSTACK) {
                    return FURRY_ERR;
                }
                state.snap.callstack[state.snap.callstack_depth++] = state.snap.ip + 1;
                if (jump_to_label(program, &state, ins->a) != FURRY_OK) {
                    return FURRY_ERR;
                }
                break;
            case FURRY_OP_RETURN:
                if (state.snap.callstack_depth == 0) {
                    return FURRY_ERR;
                }
                state.snap.ip = state.snap.callstack[--state.snap.callstack_depth];
                break;
            case FURRY_OP_SET:
                if (set_var(&state, ins->a, ins->b) != FURRY_OK) {
                    return FURRY_ERR;
                }
                state.snap.ip++;
                break;
            case FURRY_OP_ADD: {
                int next = atoi(get_var(&state, ins->a)) + ins->i;
                char out[FURRY_MAX_VALUE];
                snprintf(out, sizeof(out), "%d", next);
                if (set_var(&state, ins->a, out) != FURRY_OK) {
                    return FURRY_ERR;
                }
                state.snap.ip++;
                break;
            }
            case FURRY_OP_IF_EQ:
                if (strcmp(get_var(&state, ins->a), ins->b) == 0) {
                    if (jump_to_label(program, &state, ins->c) != FURRY_OK) {
                        return FURRY_ERR;
                    }
                } else {
                    state.snap.ip++;
                }
                break;
            case FURRY_OP_BG:
                printf("[BG] %s\n", ins->a);
                state.snap.ip++;
                break;
            case FURRY_OP_MUSIC:
                printf("[MUSIC:miniaudio] %s\n", ins->a);
                state.snap.ip++;
                break;
            case FURRY_OP_SFX:
                printf("[SFX:miniaudio] %s\n", ins->a);
                state.snap.ip++;
                break;
            case FURRY_OP_CHOICE: {
                int selected = choose_fn(ins->a, ins->choices, ins->choice_count, choice_user_data);
                if (selected < 0 || (size_t)selected >= ins->choice_count) {
                    return FURRY_ERR;
                }
                if (jump_to_label(program, &state, ins->choices[selected].target) != FURRY_OK) {
                    return FURRY_ERR;
                }
                break;
            }
            case FURRY_OP_END:
                return FURRY_OK;
            default:
                return FURRY_ERR;
        }

        steps++;
    }

    return FURRY_ERR;
}

int furry_snapshot_save(const FurryRuntimeSnapshot *snapshot, char *out_text, size_t out_size) {
    if (snapshot == NULL || out_text == NULL || out_size == 0) {
        return FURRY_ERR;
    }

    int written = snprintf(out_text, out_size, "ip=%zu;depth=%zu;vars=%zu", snapshot->ip, snapshot->callstack_depth, snapshot->var_count);
    if (written < 0 || (size_t)written >= out_size) {
        return FURRY_ERR;
    }
    return FURRY_OK;
}

int furry_snapshot_load(const char *text, FurryRuntimeSnapshot *out_snapshot) {
    if (text == NULL || out_snapshot == NULL) {
        return FURRY_ERR;
    }

    memset(out_snapshot, 0, sizeof(*out_snapshot));
    size_t ip = 0;
    size_t depth = 0;
    size_t vars = 0;
    if (sscanf(text, "ip=%zu;depth=%zu;vars=%zu", &ip, &depth, &vars) != 3) {
        return FURRY_ERR;
    }
    if (depth > FURRY_MAX_CALLSTACK || vars > FURRY_MAX_VARS) {
        return FURRY_ERR;
    }
    out_snapshot->ip = ip;
    out_snapshot->callstack_depth = depth;
    out_snapshot->var_count = vars;
    return FURRY_OK;
}

void furry_free_program(FurryProgram *program) {
    if (program == NULL) {
        return;
    }
    free(program->code);
    program->code = NULL;
    program->count = 0;
}
