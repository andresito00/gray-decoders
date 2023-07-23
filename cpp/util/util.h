#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_
#include <array>
#include <vector>
#include <string>
#include <climits>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <iterator>
#include <iostream>

#ifndef PERF_BUILD
#define ASSERT(x) assert(x)
#else  // PERF_BUILD
#define ASSERT(x)
#endif  // PERF_BUILD

constexpr std::array<unsigned char, 4> kDelimiter{ 0xEF, 0xBE, 0xAD, 0xDE };

typedef struct SpikeRaster {
  uint64_t id;
  std::vector<uint64_t> raster;  // 1-D array of event times

  size_t size(void) const {
    return sizeof(struct SpikeRaster);
  }

  void clear(void) {
    id = INT_MAX;
    raster.clear();
  }


  static std::vector<struct SpikeRaster> deserialize(std::vector<unsigned char>& buff) {
    // todo: read up on better c++ deserialization patterns...
    std::vector<struct SpikeRaster> result{};
    result.reserve(5);
    auto delim_start = kDelimiter.begin();
    auto delim_end = kDelimiter.end();
    auto range_start = buff.begin();
    auto range_end = buff.end();
    auto found = std::search(range_start, range_end, delim_start, delim_end);

    std::cout << "found? " << (found != range_end) << std::endl;

    while (found != range_end) {
      struct SpikeRaster current;
      size_t raster_bytes = found - (range_start + sizeof(current.id));
    // // todo: Templatize SpikeRaster on integer type and make this generic...
      current.id = *(reinterpret_cast<const uint64_t *>(&(*range_start))); // ugly as sin, must fix
      current.raster.resize(raster_bytes/sizeof(uint64_t));
      const unsigned char *raster_start = &(*range_start) + sizeof(current.id);
      memcpy(reinterpret_cast<unsigned char *>(current.raster.data()),
        raster_start,
        raster_bytes);
      std::cout << "pushing back " << std::dec << current.size() << " bytes" << std::endl;
      result.push_back(current);
      /////////////////////////
      buff.erase(range_start, range_start + sizeof(current.id) + raster_bytes + kDelimiter.size());
      range_start = buff.begin();
      range_end = buff.end();
      if (buff.size() > 0) {
        found = std::search(range_start, range_end, delim_start, delim_end);
      } else {
        break;
      }
    }
    return result;
  }
} __attribute__((packed)) SpikeRaster_t;

#endif  // UTIL_UTIL_H_
