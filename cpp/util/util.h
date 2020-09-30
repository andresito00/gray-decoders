#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_

#include <vector>
#include <stdint.h>
#include <assert.h>

#ifndef PERF_BUILD
#define ASSERT(x) assert(x)
#else // PERF_BUILD
#define ASSERT(x)
#endif // PERF_BUILD

#define MIN(a, b) ((a) < (b) ? (a): (b))
#define MAX(a, b) ((a) > (b) ? (a): (b))

typedef struct SpikeRasterHeader {
  size_t num_events;
  double epoch_ms;
} SpikeRasterHeader_t;

// TODO: Move to its own raster directory/file
typedef struct SpikeRaster {
  SpikeRasterHeader_t header;
  std::vector<double> raster; // 1-D array of event times
} SpikeRaster_t;

uint32_t *util_find_raster_end(uint32_t *buffer, uint32_t *end);

#endif // UTIL_UTIL_H_
