#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_

#include <vector>
#include <string>
#include <stdint.h>
#include <assert.h>

#ifndef PERF_BUILD
#define ASSERT(x) assert(x)
#else  // PERF_BUILD
#define ASSERT(x)
#endif  // PERF_BUILD

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct __attribute__((packed)) SpikeRaster {
  uint64_t id;
  std::vector<uint64_t> raster;  // 1-D array of event times
} SpikeRaster_t;

static inline uint8_t *util_find_packet_end(uint8_t *buffer, uint8_t *end)
{
  static const std::string needle("\xEF\xBE\xAD\xDE", 4);
  std::string sv(reinterpret_cast<char const *>(buffer), (end - buffer));
  if (std::size_t n = sv.find(needle); n != sv.npos) {
    return reinterpret_cast<uint8_t *>(buffer + n);
  }
  return nullptr;
}

#endif  // UTIL_UTIL_H_
