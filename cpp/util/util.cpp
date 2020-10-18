#include <stdint.h>
#include "util.h"
#include <iostream>

static const tail_marker_t TAIL_MARKER = 0xDEADBEEF;

uint8_t *util_find_packet_end(uint8_t *buffer, uint8_t *end)
{
  uint32_t *current = reinterpret_cast<uint32_t *>(buffer);
  uint32_t *end_alias = reinterpret_cast<uint32_t *>(end);
  while(current < end_alias) {
    if(*current == TAIL_MARKER) {
      return reinterpret_cast<uint8_t *>(current);
    }
    ++current;
  }
  return nullptr;
}
