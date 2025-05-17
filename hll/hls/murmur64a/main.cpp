#include "murmur64a.h"
#include "hll.h"
#include <hls_math.h>
#include "ap_int.h"

uint8_t count_zll(const ap_uint<64> data) {
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE ap_none port=data
#pragma HLS INLINE
COUNT_ZLL:
    return data[0] ? 0 :
    data[1] ? 1 :
    data[2] ? 2 :
    data[3] ? 3 :
    data[4] ? 4 :
    data[5] ? 5 :
    data[6] ? 6 :
    data[7] ? 7 :
    data[8] ? 8 :
    data[9] ? 9 :
    data[10] ? 10 :
    data[11] ? 11 :
    data[12] ? 12 :
    data[13] ? 13 :
    data[14] ? 14 :
    data[15] ? 15 :
    data[16] ? 16 :
    data[17] ? 17 :
    data[18] ? 18 :
    data[19] ? 19 :
    data[20] ? 20 :
    data[21] ? 21 :
    data[22] ? 22 :
    data[23] ? 23 :
    data[24] ? 24 :
    data[25] ? 25 :
    data[26] ? 26 :
    data[27] ? 27 :
    data[28] ? 28 :
    data[29] ? 29 :
    data[30] ? 30 :
    data[31] ? 31 :
    data[32] ? 32 :
    data[33] ? 33 :
    data[34] ? 34 :
    data[35] ? 35 :
    data[36] ? 36 :
    data[37] ? 37 :
    data[38] ? 38 :
    data[39] ? 39 :
    data[40] ? 40 :
    data[41] ? 41 :
    data[42] ? 42 :
    data[43] ? 43 :
    data[44] ? 44 :
    data[45] ? 45 :
    data[46] ? 46 :
    data[47] ? 47 :
    data[48] ? 48 :
    data[49] ? 49 :
    data[50] ? 50 :
    data[51] ? 51 :
    data[52] ? 52 :
    data[53] ? 53 :
    data[54] ? 54 :
    data[55] ? 55 :
    data[56] ? 56 :
    data[57] ? 57 :
    data[58] ? 58 :
    data[59] ? 59 :
    data[60] ? 60 :
    data[61] ? 61 :
    data[62] ? 62 :
    data[63] ? 63 : 0;
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

void hllCompute(hls::stream<stream_t> &in, hls::stream<stream_t> &out) {

#pragma HLS INTERFACE axis port=in
#pragma HLS INTERFACE axis port=out
#pragma HLS INTERFACE s_axilite port=return

    uint64_t count = 0;

    static ap_uint<6> registers[HLL_REGISTERS];
#pragma HLS ARRAY_PARTITION variable=registers type=cyclic factor=32 dim=1

    bool done = false;
    //while(!done){
    stream_t transfer;
    if(!in.empty()){
        in.read(transfer);
        uint32_t bytes_processed = 0;
        uint8_t len_mark = transfer.data(7, 0);
        uint8_t num_ele = transfer.data(15, 8);

        uint8_t req[64];
        ap_uint<8> data[32];
        uint8_t len[32];

        for (uint8_t i=0; i < STREAM_WIDTH/8; i++){
#pragma HLS UNROLL
            req[i] = transfer.data(i*8+7, i*8);
        }

        for (uint8_t i=2; i < len_mark;i++){
            data[i-2] = ap_uint<8>(req[i]);
        }

        for (uint8_t i=len_mark; i < STREAM_WIDTH/8;i++){
            len[i-len_mark] = req[i];
        }

        uint8_t idx = 0;
        for (uint32_t i = 0; i < num_ele; i++) {
            hllAdd(registers, &data[idx], len[i]);
            idx += len[i] * 8;
        }

        if (transfer.last == 1) {
            done = true;
        }
//}

        hllCount(registers, &count);

        stream_t output;
        output.data(63, 0) = count;
        output.last = 1;
        out.write(output);
    }
}
