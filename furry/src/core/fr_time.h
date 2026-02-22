#ifndef FR_TIME_H
#define FR_TIME_H

#include <stdint.h>

typedef struct FrTime {
  double current_time;
  double last_time;
  float delta_time;
  uint32_t frame_count;
} FrTime;

void fr_time_init(FrTime *time);
void fr_time_update(FrTime *time);

#endif // FR_TIME_H
