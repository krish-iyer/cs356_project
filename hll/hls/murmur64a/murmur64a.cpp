#include "murmur64a.h"

#include "stdint.h"
#include <ap_int.h>

uint64_t murmur64a (const ap_uint<8> *data, const uint32_t len, const uint32_t seed){

    const uint64_t m = 0xc6a4a7935bd1e995;
    const uint8_t r = 47;
    const uint32_t end = (len - (len & 7));
    uint64_t h = seed ^ (len * m);
    ap_uint<8> tmp_cpy[8];

    for (int j=0 ; j< 8 ;j++){
#pragma HLS UNROLL FACTOR=8
        tmp_cpy[j] = data[j];

    }

LOOP_MURMUR_1:
    for (uint32_t i = 0; i < end; i=i+8) {
#pragma HLS PIPELINE II=8
        uint64_t tmp = *(ap_uint<64>*)(&tmp_cpy);

        tmp *= m;
        tmp ^= ( tmp >> r);
        tmp *= m;
        h ^= tmp;
        h *= m;

        data += 8;

    LOOP_MURMUR_1_CPY:
        for (int j=0 ; j< 8 ;j++){
#pragma HLS UNROLL FACTOR=8
            tmp_cpy[j] = data[j];
        }
    }

    uint64_t tmp_r[8];

    tmp_r[7] = (uint64_t)tmp_cpy[6] << 48;
    tmp_r[6] = (uint64_t)tmp_cpy[5] << 40;
    tmp_r[5] = (uint64_t)tmp_cpy[4] << 32;
    tmp_r[4] = (uint64_t)tmp_cpy[3] << 24;
    tmp_r[3] = (uint64_t)tmp_cpy[2] << 16;
    tmp_r[2] = (uint64_t)tmp_cpy[1] << 8;
    tmp_r[1] = (uint64_t)tmp_cpy[0];

LOOP_MURMUR_2: // need to optimize this, data dependency might be bottleneck
    for (uint8_t i=(len&7) ; i >= 1; i--){
        h ^= tmp_r[i];
    }

    if (len & 7){
        h *= m;
    }

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}
