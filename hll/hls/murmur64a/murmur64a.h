#ifndef MURMUR64A_H_
#define MURMUR64A_H_

#include <stdint.h>
#include <ap_int.h>

void murmur64a (const ap_uint<8> *data, uint32_t len, uint32_t seed, uint64_t *ret);

#endif
