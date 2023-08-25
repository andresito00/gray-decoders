#ifndef DATA_RASTER_H_
#define DATA_RASTER_H_
#include <array>
#include <vector>
#include <string>
#include <climits>
#include <cstdint>
#include <iterator>
#include <string.h>

constexpr static std::array<unsigned char, 4> kDelimiter{ 0xEF, 0xBE, 0xAD, 0xDE };
constexpr static size_t kDefaultSpikeTrainSize{64};

namespace raster {

template<typename T>
struct SpikeRaster {
  T id;
  std::vector<T> raster;  // 1-D array of event times
  SpikeRaster() noexcept: id{-1LU}, raster{{}} {}
  SpikeRaster(size_t sz) noexcept: id{-1LU}, raster{std::move(std::vector<T>(sz, 0))} {}
  SpikeRaster(T id, size_t sz) noexcept: id{id}, raster{std::move(std::vector<T>(sz, 0))} {}
  SpikeRaster(T id, std::vector<T>&& raster) noexcept: id{id}, raster{std::move(raster)} {}
  SpikeRaster(SpikeRaster& other) noexcept: id{other.id}, raster{other.raster} {}
  SpikeRaster(SpikeRaster&& other) noexcept: id{other.id}, raster{std::move(other.raster)} {
    other.id = -1LU;
    other.raster = {};
  }

  SpikeRaster& operator=(SpikeRaster& other) noexcept {
    if (this == &other) {
      return *this;
    }
    id = other.id;
    raster = other.raster;
    return *this;
  }

  SpikeRaster& operator=(SpikeRaster&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    id = other.id;
    raster = std::move(other.raster);
    other.id = -1LU;
    other.raster = {};
    return *this;
  }

  bool operator==(const SpikeRaster& other) {
    return id == other.id;
  }

  T get_id() const noexcept {
    return id;
  }

  size_t raster_size() const noexcept {
    return raster.size();
  }

  void clear() {
    id = -1LU;
    raster.clear();
  }

  static std::vector<SpikeRaster<T>> deserialize(std::vector<unsigned char>& buff) {
    // todo: read up on better c++ deserialization patterns... consider moving out of struct.
    // This is also assuming well-behaved input from the network. Needs more sanitizing.
    std::vector<SpikeRaster<T>> result{};
    auto delim_start = kDelimiter.begin();
    auto delim_end = kDelimiter.end();
    auto range_start = buff.begin();
    auto range_end = buff.end();
    auto found = range_end;
    while ((found = std::search(range_start, range_end, delim_start, delim_end)) != range_end) {

      auto id = *(T *) buff.data();
      range_start += sizeof(id);
      auto raster_bytes = static_cast<size_t>(found - range_start);

      std::vector<T> current(raster_bytes / sizeof(T), 0);
      const unsigned char *raster_start = buff.data() + sizeof(id);
      memcpy(current.data(), raster_start, raster_bytes);
      result.emplace_back(SpikeRaster<T>(id, std::move(current)));

      buff.erase(buff.begin(), found + kDelimiter.size());
      range_start = buff.begin();
      range_end = buff.end();
    }

    return result;
  }
};

using SpikeRaster64 = struct SpikeRaster<uint64_t> ;

};

#endif  // UTIL_UTIL_H_
