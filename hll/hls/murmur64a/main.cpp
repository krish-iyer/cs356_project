#include "murmur64a.h"
#include "hll.h"
#include <hls_math.h>

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

void hllRegHisto(uint8_t *registers, uint32_t *reghisto){

    for(uint16_t j = 0; j < HLL_REGISTERS; j++) {
        uint64_t reg;
        HLL_DENSE_GET_REGISTER(reg, registers, j);
        reghisto[reg]++;
    }
}

double hllSigma(double x) {
    if (x == 1.) return INFINITY;
    double zPrime;
    double y = 1;
    double z = x;
    do {
        x *= x;
        zPrime = z;
        z += x * y;
        y += y;
    } while(zPrime != z);
    return z;
}

double hllTau(double x) {
    if (x == 0. || x == 1.) return 0.;
    double zPrime;
    double y = 1.0;
    double z = 1 - x;
    do {
        x = sqrt(x);
        zPrime = z;
        y *= 0.5;
        z -= pow(1 - x, 2)*y;
    } while(zPrime != z);
    return z / 3;
}


uint64_t hllCount(uint8_t* registers){
    double m = HLL_REGISTERS;
    double E;

    uint32_t reghisto[64] = {0};

    hllRegHisto(registers, reghisto);

    double z = m * hllTau((m-reghisto[HLL_Q+1])/(double)m);
    for (uint16_t j = HLL_Q; j >= 1; --j) {
        z += reghisto[j];
        z *= 0.5;
    }
    z += m * hllSigma(reghisto[0]/(double)m);
    E = llround(HLL_ALPHA_INF*m*m/z);

    return (uint64_t) E;

}

uint64_t hllAdd(uint8_t *registers, ap_uint<8> *data, const uint32_t len){
    uint64_t idx = 0;
    uint8_t count = hllPatLen(data, len, &idx);

    return hllSet(registers, idx, count);

    hllCount(registers);
}

uint64_t hllCompute(ap_uint<8> *data, const uint32_t *len, const uint32_t num_ele){

    static uint8_t registers[16384];

    for (uint32_t i=0 ; i < num_ele ; i++){
        hllAdd(registers, data, len[i]);
        data += len[i];
    }

    return hllCount(registers);
}
