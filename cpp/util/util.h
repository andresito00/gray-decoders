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

// TODO: Move to its own raster directory/file
// Would like to keep this P.O.D...
typedef struct __attribute__((packed)) SpikeRaster {
  uint64_t id;
  std::vector<uint64_t> raster; // 1-D array of event times
} SpikeRaster_t;

typedef uint32_t tail_marker_t;

uint8_t *util_find_packet_end(uint8_t *buffer, uint8_t *end);

#endif // UTIL_UTIL_H_
