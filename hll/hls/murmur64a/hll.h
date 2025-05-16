#ifndef HLL_H_
#define HLL_H_

#include <stdint.h>
#include <ap_int.h>
#include <hls_stream.h>
#include "ap_axi_sdata.h"

#define STREAM_WIDTH 512
#define PAYLOAD_SIZE 64

typedef ap_axiu<STREAM_WIDTH, 1, 1, 1> stream_t;

#define HLL_P 10 /* The greater is P, the smaller the error. */
#define HLL_Q (64-HLL_P) /* The number of bits of the hash value used for determining the number of leading zeros. */
#define HLL_REGISTERS (1<<HLL_P) /* With P=14, 16384 registers. */
#define HLL_P_MASK (HLL_REGISTERS-1) /* Mask to index register. */
#define HLL_BITS 6 /* Enough to count up to 63 leading zeroes. */
#define HLL_REGISTER_MAX ((1<<HLL_BITS)-1)
#define INFINITY ((float)(1e+300 * 1e+300))

#define HLL_ALPHA_INF 0.721347520444481703680 /* constant for 0.5/ln(2) */

void hllPatLen(ap_uint<8> *data, const uint32_t len, uint64_t *idx, uint8_t *ret);
void hllSet(ap_uint<6> *registers, uint64_t idx, uint8_t count, uint8_t *ret); // Updated to ap_uint<6>
void hllAdd(ap_uint<6> *registers, ap_uint<8> *data, const uint32_t len); // Updated to ap_uint<6>
void hllRegHisto(ap_uint<6> *registers, uint32_t *reghisto); // Updated to ap_uint<6>
void hllCount(ap_uint<6> *registers, uint64_t *ret); // Updated to ap_uint<6>
void hllCompute(hls::stream<stream_t> &in, hls::stream<stream_t> &out);

#endif // HLL_H_
