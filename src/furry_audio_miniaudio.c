#include "furry.h"

const char *furry_audio_backend_name(void) {
#if defined(FURRY_ENABLE_MINIAUDIO)
    return "miniaudio";
#else
    return "none";
#endif
}
