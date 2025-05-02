#include "murmur64a.h"
#include "hll.h"

#include "ap_int.h"

uint8_t count_zll(const ap_uint<64> data){
    uint8_t count = 0;
    for(uint8_t i = 0;i<64;i++){
        if(data[i]){
            count = i;
            break;
        }
    }
    return count;
}

uint8_t hllPatLen(ap_uint<8> *data, const uint32_t len, uint64_t *idx){
    uint64_t hash;

    uint32_t count;

    hash = murmur64a(data, len, 0xadc83b19ULL);

    *idx = hash & HLL_P_MASK;

    hash >>= HLL_P;
    hash |= ((uint64_t)1<<HLL_Q);

    count = count_zll(hash) + 1;

    return count;

}

uint8_t hllSet(uint8_t *registers, uint64_t idx, uint8_t count){
    uint8_t oldcount=0;
    //printf("hls: idx: %lu count: %d\n", idx, count);

    HLL_DENSE_GET_REGISTER(oldcount, registers, idx);
    //printf("hls: count> oldcount(%d)\n", oldcount);

    if (count > oldcount) {
        HLL_DENSE_SET_REGISTER(registers, idx, count);
        return 1;
    } else {
        return 0;
    }

}

uint8_t hllAdd(uint8_t *registers, ap_uint<8> *data, const uint32_t len){
    uint64_t idx = 0;
    uint8_t count = hllPatLen(data, len, &idx);

    return hllSet(registers, idx, count);
}
