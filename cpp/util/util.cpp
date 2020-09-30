#include <stdint.h>
#include <cstddef>
#include "util.h"

#define MARKER 0xDEADBEEF

uint32_t *util_find_raster_end(uint32_t *buffer, uint32_t *end)
{
  uint32_t *current = buffer;
  while(current != end) {
    if(*current == MARKER) {
      return current;
    }
  }

  return nullptr;
}
