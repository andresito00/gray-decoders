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

namespace raster {

template<typename T>
struct SpikeRaster {
  T id;
  std::vector<T> raster;  // 1-D array of event times
  SpikeRaster() noexcept: id{-1LU}, raster{{}} {}
  SpikeRaster(SpikeRaster& other) noexcept: id{other.id}, raster{other.raster} {}
  SpikeRaster(SpikeRaster&& other) noexcept: id{-1LU}, raster{{}} {
    id = other.id;
    raster = std::move(other.raster);
    other.id = -1LU;
    other.raster = {};
  }

  SpikeRaster& operator=(SpikeRaster& other) noexcept {
    if (this == &other) {
      return *this;
    }
    id = other.id;
    raster = other.raster;
    other.id = -1LU;
    other.raster = {};
    return *this;
  }

  SpikeRaster& operator=(SpikeRaster&& other) noexcept {
    if (*this != other) {
      id = other.id;
      raster = std::move(other.raster);
      other.id = -1LU;
      other.raster = {};
    }
    return *this;
  }

  bool operator==(const SpikeRaster& other) {
    return id == other.id;
  }

  T get_id() const {
    return id;
  }

  size_t size() const {
    return sizeof(SpikeRaster);
  }

  void clear() {
    id = -1LU;
    raster.clear();
  }

  static std::vector<SpikeRaster<T>> deserialize(std::vector<unsigned char>& buff) {
    // todo: read up on better c++ deserialization patterns...
    // This is also assuming well-behaved input from the network. Needs more sanitizing.
    std::vector<SpikeRaster<T>> result{};
    auto delim_start = kDelimiter.begin();
    auto delim_end = kDelimiter.end();
    auto range_start = buff.begin();
    auto range_end = buff.end();
    auto found = range_end;

    while ((found = std::search(range_start, range_end, delim_start, delim_end)) != range_end) {
      SpikeRaster<T> current;

      // todo: Templatize SpikeRaster on integer type and make this generic...
      current.id = *(reinterpret_cast<const T*>(&(*range_start))); // ugly as sin, must fix
      const unsigned char *raster_start = &(*range_start) + sizeof(current.id);

      size_t raster_bytes = found - (range_start + offsetof(SpikeRaster<T>, raster));
      current.raster.resize(raster_bytes / sizeof(T));
      memcpy(current.raster.data(), raster_start, raster_bytes);

      result.emplace_back(std::move(current));

      // update buff start pointer
      buff.erase(range_start, found + kDelimiter.size());

      range_start = buff.begin();
      range_end = buff.end();
    }
    return result;
  }
};


using SpikeRaster64 = struct SpikeRaster<uint64_t> ;

};



#endif  // UTIL_UTIL_H_
