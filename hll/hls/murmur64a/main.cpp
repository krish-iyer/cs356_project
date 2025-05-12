#include "murmur64a.h"
#include "hll.h"
#include <hls_math.h>
#include "ap_int.h"

uint8_t count_zll(const ap_uint<64> data) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < 64; i++) {
#pragma HLS UNROLL
        if (data[i]) {
            count = i;
            break;
        }
    }
    return count;
}

void hllPatLen(ap_uint<8> *data, const uint32_t len, uint64_t *idx, uint8_t *ret) {
    uint64_t hash;
    uint32_t count;

    murmur64a(data, len, 0xadc83b19ULL, &hash);

    *idx = hash & HLL_P_MASK;

    hash >>= HLL_P;
    hash |= ((uint64_t)1 << HLL_Q);

    count = count_zll(hash) + 1;

    *ret = count;
}

void hllSet(ap_uint<6> *registers, uint64_t idx, uint8_t count, uint8_t *ret) {
#pragma HLS INLINE
    ap_uint<6> oldcount = registers[idx];

    if (count > oldcount) {
        registers[idx] = count;
        *ret = 1;
    } else {
        *ret = 0;
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
        x *= x;
        zPrime = z;
        z += x * y;
        y += y;
    } while (zPrime != z);
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
        double tmp_x = x;
        x = sqrt(x);
        zPrime = z;
        double tmp_sq = (1 + tmp_x - 2 * x);
        y *= 0.5;
        z -= tmp_sq * y;
    } while (zPrime != z);
    return z / 3;
}

void hllCount(ap_uint<6> *registers, uint64_t *ret) {
#pragma HLS INLINE OFF
    double m = HLL_REGISTERS;
    double E;

    uint32_t reghisto[64] = {0};
#pragma HLS ARRAY_PARTITION variable=reghisto type=complete

    uint8_t localhisto_0[64] = {0};
    uint8_t localhisto_1[64] = {0};
    uint8_t localhisto_2[64] = {0};
    uint8_t localhisto_3[64] = {0};
    uint8_t localhisto_4[64] = {0};
    uint8_t localhisto_5[64] = {0};
    uint8_t localhisto_6[64] = {0};
    uint8_t localhisto_7[64] = {0};
    uint8_t localhisto_8[64] = {0};
    uint8_t localhisto_9[64] = {0};
    uint8_t localhisto_10[64] = {0};
    uint8_t localhisto_11[64] = {0};
    uint8_t localhisto_12[64] = {0};
    uint8_t localhisto_13[64] = {0};
    uint8_t localhisto_14[64] = {0};
    uint8_t localhisto_15[64] = {0};
#pragma HLS ARRAY_PARTITION variable=localhisto_0 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_1 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_2 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_3 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_4 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_5 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_6 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_7 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_8 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_9 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_10 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_11 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_12 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_13 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_14 type=complete
#pragma HLS ARRAY_PARTITION variable=localhisto_15 type=complete

LOOP_REG_HISTO:
    for (uint16_t j = 0; j < HLL_REGISTERS; j++) {
#pragma HLS PIPELINE II=1
#pragma HLS UNROLL FACTOR=16
        uint16_t idx = j % 16;
        if (idx == 0) localhisto_0[registers[j]]++;
        else if (idx == 1) localhisto_1[registers[j]]++;
        else if (idx == 2) localhisto_2[registers[j]]++;
        else if (idx == 3) localhisto_3[registers[j]]++;
        else if (idx == 4) localhisto_4[registers[j]]++;
        else if (idx == 5) localhisto_5[registers[j]]++;
        else if (idx == 6) localhisto_6[registers[j]]++;
        else if (idx == 7) localhisto_7[registers[j]]++;
        else if (idx == 8) localhisto_8[registers[j]]++;
        else if (idx == 9) localhisto_9[registers[j]]++;
        else if (idx == 10) localhisto_10[registers[j]]++;
        else if (idx == 11) localhisto_11[registers[j]]++;
        else if (idx == 12) localhisto_12[registers[j]]++;
        else if (idx == 13) localhisto_13[registers[j]]++;
        else if (idx == 14) localhisto_14[registers[j]]++;
        else localhisto_15[registers[j]]++;
    }
    for (uint8_t j = 0; j < 64; j++) {
#pragma HLS PIPELINE II=1
#pragma HLS UNROLL factor=64
        reghisto[j] = localhisto_0[j] + localhisto_1[j] + localhisto_2[j] + localhisto_3[j] + localhisto_4[j] + \
            localhisto_5[j] + localhisto_6[j] + localhisto_7[j] + localhisto_8[j] + localhisto_9[j] + localhisto_10[j] \
            + localhisto_11[j] + localhisto_12[j] + localhisto_13[j] + localhisto_14[j] + localhisto_15[j];
    }

    double z = m * hllTau((m - reghisto[HLL_Q + 1]) / (double)m);

    uint64_t tmp_z = 0;
LOOP_COUNT:
    for (uint16_t j = HLL_Q; j >= 1; --j) {
#pragma HLS UNROLL
        tmp_z += reghisto[j];
    }
    z += tmp_z;
    z *= 0.5;
    z += m * hllSigma(reghisto[0] / (double)m);
    E = llround(HLL_ALPHA_INF * m * m / z);

    *ret = (uint64_t)E;
}

void hllAdd(ap_uint<6> *registers, ap_uint<8> *data, const uint32_t len) {
#pragma HLS DATAFLOW
    uint64_t idx = 0;
    uint8_t count = 0;
    uint8_t ret = 0;

    hllPatLen(data, len, &idx, &count);
    hllSet(registers, idx, count, &ret);
}

void hllCompute(ap_uint<8> *data, const uint32_t *len, const uint32_t num_ele, uint64_t *count) {
#pragma HLS INTERFACE m_axi port=data depth=256
#pragma HLS INTERFACE m_axi port=len depth=256
#pragma HLS INTERFACE s_axilite port=count
#pragma HLS INTERFACE s_axilite port=num_ele
#pragma HLS INTERFACE s_axilite port=return

    static ap_uint<6> registers[HLL_REGISTERS];
#pragma HLS ARRAY_PARTITION variable=registers type=cyclic factor=32 dim=1

LOOP_ELE:
    for (uint32_t i = 0; i < num_ele; i++) {
        hllAdd(registers, data, len[i]);
        data += len[i];
    }

    hllCount(registers, count);
}
