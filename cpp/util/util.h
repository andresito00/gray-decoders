#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_

#include <stdint.h>

#define MIN(a, b) ((a) < (b) ? (a): (b))
#define MAX(a, b) ((a) > (b) ? (a): (b))

typedef struct SpikeRaster {
  size_t num_events;
  double epoch_ms; // ms
  double *raster; // 2-D matrix of (neurons) x (spike event times)
} SpikeRaster_t;


#endif // UTIL_UTIL_H_
