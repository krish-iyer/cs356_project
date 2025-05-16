#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <ap_int.h>
#include <hls_stream.h>
#include "ap_axi_sdata.h"

#define STREAM_WIDTH 512
#define HDR_SIZE 16
#define PAYLOAD_SIZE 64//((STREAM_WIDTH/8) - HDR_SIZE)

typedef ap_axiu<STREAM_WIDTH, 1, 1, 1> stream_t;

void top(volatile uint8_t* req_frm_dram, volatile uint8_t* dram_out);
void fetcher(volatile uint8_t* req_frm_dram, hls::stream<stream_t>& to_sha256, hls::stream<stream_t>& to_hll);
void store(volatile uint8_t* dram_out, hls::stream<stream_t>& sha256_in, hls::stream<stream_t>& hll_in);

#endif // SCHEDULER_H_
