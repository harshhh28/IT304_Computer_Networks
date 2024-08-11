#include "checksum.h"

// Compute a simple 32-bit checksum
uint32_t compute_checksum(const char *data, size_t length) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < length; ++i) {
        checksum += (uint8_t)data[i];
    }
    return checksum;
}