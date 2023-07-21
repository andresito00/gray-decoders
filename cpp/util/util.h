#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_

#include <vector>
#include <string>
#include <climits>
#include <stdint.h>
#include <assert.h>

#ifndef PERF_BUILD
#define ASSERT(x) assert(x)
#else  // PERF_BUILD
#define ASSERT(x)
#endif  // PERF_BUILD

typedef struct SpikeRaster {
  uint64_t id;
  std::vector<uint64_t> raster;  // 1-D array of event times
} __attribute__((packed)) SpikeRaster_t;

#endif  // UTIL_UTIL_H_
