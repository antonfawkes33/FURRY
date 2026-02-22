#include "fr_time.h"
#include "../platform/fr_platform.h"

void fr_time_init(FrTime *time) {
  if (!time)
    return;
  time->current_time = fr_platform_get_time();
  time->last_time = time->current_time;
  time->delta_time = 0.0f;
  time->frame_count = 0;
}

void fr_time_update(FrTime *time) {
  if (!time)
    return;
  time->last_time = time->current_time;
  time->current_time = fr_platform_get_time();
  time->delta_time = (float)(time->current_time - time->last_time);
  time->frame_count++;
}
