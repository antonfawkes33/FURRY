#ifndef FR_SCRIPT_H
#define FR_SCRIPT_H

#include <stdbool.h>

typedef struct FrScript FrScript;
typedef struct FrWindow FrWindow;
typedef struct FrRenderer FrRenderer;

FrScript *fr_script_init(FrWindow *window, FrRenderer *renderer);
void fr_script_shutdown(FrScript *script);

bool fr_script_execute_file(FrScript *script, const char *path);
bool fr_script_execute_string(FrScript *script, const char *code);

#endif // FR_SCRIPT_H
