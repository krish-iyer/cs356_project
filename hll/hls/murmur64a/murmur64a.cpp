#include "murmur64a.h"

#include "stdint.h"

uint64_t murmur64a (const uint8_t *data, const uint32_t len, const uint32_t seed){

    const uint64_t m = 0xc6a4a7935bd1e995;
    const uint8_t r = 47;
    const uint32_t end = (len - (len & 7));
    uint64_t h = seed ^ (len * m);

    for (uint32_t i = 0; i < end; i++) {
        uint64_t tmp = (uint64_t) data[i];
        tmp *= m;
        tmp ^= ( tmp >> r);
        tmp *= m;
        h ^= tmp;
        h *= m;
    }

    uint64_t tmp_r[8];

    tmp_r[7] = (uint64_t)data[6] << 48;
    tmp_r[6] = (uint64_t)data[5] << 40;
    tmp_r[5] = (uint64_t)data[4] << 32;
    tmp_r[4] = (uint64_t)data[3] << 24;
    tmp_r[3] = (uint64_t)data[2] << 16;
    tmp_r[2] = (uint64_t)data[1] << 8;
    tmp_r[1] = (uint64_t)data[0];

    for (uint8_t i=1 ; i <= (len&7); i++){
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
