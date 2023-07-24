#ifndef DATA_RASTER_H_
#define DATA_RASTER_H_
#include <array>
#include <vector>
#include <string>
#include <climits>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <iterator>
#include <iostream>

constexpr std::array<unsigned char, 4> kDelimiter{ 0xEF, 0xBE, 0xAD, 0xDE };

template<typename T>
struct SpikeRaster {
  uint64_t id;
  std::vector<T> raster;  // 1-D array of event times

  size_t size(void) const {
    return sizeof(struct SpikeRaster);
  }

  void clear(void) {
    id = INT_MAX;
    raster.clear();
  }

  static std::vector<struct SpikeRaster> deserialize(std::vector<unsigned char>& buff) {
    // todo: read up on better c++ deserialization patterns...
    // This is also assuming well-behaved input from the network. Needs more sanitizing.
    std::vector<struct SpikeRaster> result{};
    auto delim_start = kDelimiter.begin();
    auto delim_end = kDelimiter.end();
    auto range_start = buff.begin();
    auto range_end = buff.end();
    auto found = std::search(range_start, range_end, delim_start, delim_end);

    while (found != range_end) {
      struct SpikeRaster current;
      size_t raster_bytes = found - (range_start + sizeof(current.id));

      // todo: Templatize SpikeRaster on integer type and make this generic...
      current.id = *(reinterpret_cast<const uint64_t *>(&(*range_start))); // ugly as sin, must fix
      const unsigned char *raster_start = &(*range_start) + sizeof(current.id);

      current.raster.resize(raster_bytes / sizeof(T));

      memcpy(reinterpret_cast<unsigned char *>(current.raster.data()),
        raster_start,
        raster_bytes);

      result.push_back(current);

      // update buff start pointer
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
};

typedef struct SpikeRaster<uint64_t> SpikeRaster64;

#endif  // UTIL_UTIL_H_
