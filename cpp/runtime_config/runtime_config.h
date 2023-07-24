#include <string>
#include <vector>

struct RuntimeConfig {
  std::string optional_ip;
  int optional_port;
  std::string raster_file;
  std::string kinematic_data_file;
  size_t bin_size;
};
