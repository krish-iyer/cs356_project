#include "murmur64a.h"
#include "hll.h"
#include <hls_math.h>

#include "ap_int.h"


void hll_get_register(uint16_t *target, uint8_t *p, uint64_t regnum) {
    #pragma HLS INLINE
    uint8_t *_p = (uint8_t*) p;
    uint64_t _byte = ((regnum)*HLL_BITS) >> 3;
    uint64_t _fb = (regnum)*HLL_BITS&7;
    uint64_t _fb8 = 8 - _fb;
    uint64_t b0 = _p[_byte];
    uint64_t b1 = _p[_byte+1];
    *target = ((b0 >> _fb) | (b1 << _fb8)) & HLL_REGISTER_MAX;
}

/* Set the value of the register at position 'regnum' to 'val'.
 * 'p' is an array of unsigned bytes. */
void hll_set_register(uint8_t *p, uint64_t regnum, uint8_t val) {
    #pragma HLS INLINE
    uint8_t *_p = (uint8_t*) p;
    uint64_t _byte = ((regnum)*HLL_BITS) >> 3;
    uint64_t _fb = (regnum)*HLL_BITS&7;
    uint64_t _fb8 = 8 - _fb;
    uint64_t _v =  (val);
    _p[_byte] &= ~(HLL_REGISTER_MAX << _fb);
    _p[_byte] |= _v << _fb;
    _p[_byte+1] &= ~(HLL_REGISTER_MAX >> _fb8);
    _p[_byte+1] |= _v >> _fb8;
}


uint8_t count_zll(const ap_uint<64> data){
    uint8_t count = 0;
    for(uint8_t i = 0;i<64;i++){
#pragma HLS UNROLL
        if(data[i]){
            count = i;
            break;
        }
    }
    return count;
}

void hllPatLen(ap_uint<8> *data, const uint32_t len, uint64_t *idx, uint8_t *ret){
    uint64_t hash;

    uint32_t count;

    murmur64a(data, len, 0xadc83b19ULL, &hash);

    *idx = hash & HLL_P_MASK;

    hash >>= HLL_P;
    hash |= ((uint64_t)1<<HLL_Q);

    count = count_zll(hash) + 1;

    *ret = count;

}

void hllSet(uint8_t *registers, uint64_t idx, uint8_t count, uint8_t *ret){
    uint16_t oldcount=0;
    //printf("hls: idx: %lu count: %d\n", idx, count);
    uint8_t ret_;
    hll_get_register(&oldcount, registers, idx);
    //printf("hls: count> oldcount(%d)\n", oldcount);

    if (count > oldcount) {
        hll_set_register(registers, idx, count);
        ret_ = 1;
    } else {
        ret_ = 0;
    }

    *ret = ret_;

}

void hllRegHisto(uint8_t *registers, uint32_t *reghisto){
//#pragma HLS INLINE
LOOP_REG_HISTO:
    for(uint16_t j = 0; j < HLL_REGISTERS; j++) {
#pragma HLS PIPELINE II=27
#pragma HLS UNROLL FACTOR=64
        uint16_t reg;
        hll_get_register(&reg, registers, (uint64_t)j);
        reghisto[reg]++;
    }
}

double hllSigma(double x) {
#pragma HLS INLINE
    if (x == 1.) return INFINITY;
    double zPrime;
    double y = 1;
    double z = x;
LOOP_SIGMA:
    do {
//#pragma HLS UNROLL FACTOR=4
//#pragma HLS LOOP_TRIPCOUNT max=64
        x *= x;
        zPrime = z;
        z += x * y;
        y += y;
    } while(zPrime != z);
    return z;
}

double hllTau(double x) {
#pragma HLS INLINE
    if (x == 0. || x == 1.) return 0.;
    double zPrime;
    double y = 1.0;
    double z = 1 - x;
LOOP_TAU:
    do {
//#pragma HLS bind_op variable=x impl=dsp
//#pragma HLS bind_op variable=z impl=dsp
//#pragma HLS UNROLL FACTOR=2
//#pragma HLS LOOP_TRIPCOUNT max=128
        double tmp_x = x;
        x = sqrt(x);
        zPrime = z;
        double tmp_sq = (1 + tmp_x - 2*x);
        y *= 0.5;
        z -= tmp_sq * y;
    } while(zPrime != z);
    return z / 3;
}


void hllCount(uint8_t* registers, uint64_t *ret){
#pragma HLS INLINE OFF

    double m = HLL_REGISTERS;
    double E;

    uint32_t reghisto[64] = {0};
//#pragma HLS BIND_STORAGE variable = reghisto type = ram_2p impl=LUTRAM
#pragma HLS ARRAY_RESHAPE variable=reghisto type=cyclic factor=8 dim=1
    uint32_t localhisto[64] = {0};

//#pragma HLS BIND_STORAGE variable = reghisto type = ram_2p impl=LUTRAM
#pragma HLS ARRAY_PARTITION variable=localhisto type=complete
    //hllRegHisto(registers, reghisto);

LOOP_REG_HISTO:
    for(uint16_t j = 0; j < HLL_REGISTERS; j++) {
#pragma HLS PIPELINE II=22
#pragma HLS UNROLL FACTOR=32
        //hll_get_register(&reg, registers, (uint64_t)j);
        ap_uint<6> reg;
        uint8_t *_p = registers;
        ap_uint<16> _byte = ((j)*HLL_BITS) >> 3;
        ap_uint<4> _fb = (j)*HLL_BITS&7;
        ap_uint<4>_fb8 = 8 - _fb;
        ap_uint<8> b0 = _p[_byte];
        ap_uint<8> b1 = _p[_byte+1];
        reg = ((b0 >> _fb) | (b1 << _fb8)) & HLL_REGISTER_MAX;

        uint32_t tmp_reg = localhisto[reg];
        tmp_reg++;
        localhisto[reg] = tmp_reg;
    }

    for (uint8_t j=0;j < 64 ; j++){
#pragma HLS PIPELINE II=1
#pragma HLS UNROLL factor=8
        reghisto[j] = localhisto[j];
    }

    double z = m * hllTau((m-reghisto[HLL_Q+1])/(double)m);

    uint64_t tmp_z = 0;
LOOP_COUNT:
    for (uint16_t j = HLL_Q; j >= 1; --j) {
#pragma HLS UNROLL
        tmp_z += reghisto[j];
    }
    z += tmp_z;
    z *= 0.5;
    z += m * hllSigma(reghisto[0]/(double)m);
    E = llround(HLL_ALPHA_INF*m*m/z);

    *ret = (uint64_t) E;

}

void hllAdd(uint8_t *registers, ap_uint<8> *data, const uint32_t len){
#pragma HLS DATAFLOW
    uint64_t idx = 0;
    uint8_t count = 0;
    uint8_t ret = 0;

    hllPatLen(data, len, &idx, &count);
    hllSet(registers, idx, count, &ret);
}

void hllCompute(ap_uint<8> *data, const uint32_t *len, const uint32_t num_ele, uint64_t *count){

#pragma HLS INTERFACE m_axi port = data depth = 256
#pragma HLS INTERFACE m_axi port = len depth = 256
#pragma HLS INTERFACE s_axilite port = count
#pragma HLS INTERFACE s_axilite port = num_ele
#pragma HLS INTERFACE s_axilite port = return

    static uint8_t registers[HLL_REGISTERS];
#pragma HLS ARRAY_RESHAPE variable=registers type=cyclic factor=128 dim=1

LOOP_ELE:
    for (uint32_t i=0 ; i < num_ele ; i++){
//#pragma HLS UNROLL FACTOR=2
#pragma HLS PIPELINE
        hllAdd(registers, data, len[i]);
        data += len[i];
    }

    hllCount(registers, count);
}
