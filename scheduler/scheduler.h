#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <ap_int.h>
#include <hls_stream.h>
#include <stdint.h>
#include "defines.h"

void top(volatile uint8_t* req_frm_dram, volatile uint8_t* dram_out);
void fetcher(volatile uint8_t* req_frm_dram, hls::stream<stream_t> &to_sha256, hls::stream<stream_t> &to_hll);
void store(volatile uint8_t* dram_out, hls::stream<stream_t> &sha256_in, hls::stream<stream_t> &hll_in);

#endif // SCHEDULER_H_
