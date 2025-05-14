#include "scheduler.h"
#include "hll.h"
#include "sha256.h"

#include <ap_int.h>
#include <hls_stream.h>

#define STREAM_WIDTH 512
#define HDR_SIZE 16
#define PAYLOAD_SIZE 48//((STREAM_WIDTH/8) - HDR_SIZE)

typedef ap_uint<STREAM_WIDTH> stream_t;

void top_module(
    volatile uint8_t* req_frm_dram,    // AXI Master for request input
    hls::stream<stream_t>& sha256_out, // AXI-Stream for SHA256 hash
    hls::stream<stream_t>& hll_out     // AXI-Stream for HLL cardinality
) {
#pragma HLS INTERFACE m_axi port=req_frm_dram offset=slave bundle=DRAM depth=1024
#pragma HLS INTERFACE axis port=sha256_out
#pragma HLS INTERFACE axis port=hll_out
#pragma HLS INTERFACE s_axilite port=return

    hls::stream<stream_t> to_sha256;
    hls::stream<stream_t> to_hll;
#pragma HLS STREAM variable=to_sha256 depth=16
#pragma HLS STREAM variable=to_hll depth=16

#pragma HLS DATAFLOW
    fetcher(req_frm_dram, to_sha256, to_hll);
    sha256(to_sha256, sha256_out);
    hllCompute(to_hll, hll_out);
}

void fetcher(uint8_t* req_frm_dram, hls::stream<stream_t>& to_sha256, hls::stream<stream_t>& to_hll){
#pragma HLS INTERFACE m_axi port=dram offset=slave bundle=DRAM
#pragma HLS INTERFACE axis port=to_sha256
#pragma HLS INTERFACE axis port=to_hll
#pragma HLS INTERFACE s_axilite port=return

    uint8_t func_id = dram[0];
    uint16_t req_len = *(uint16_t*)&dram[1];
    uint64_t req_ptr = *(uint64_t*)&dram[3];

    if (req_len > 0){

        stream_t transfer = 0;
        transfer(31, 0) = func_param;

        uint16_t bytes_remaining = req_len;
        uint16_t offset = 0;

        uint16_t payload_bytes = (bytes_remaining > PAYLOAD_SIZE) ? PAYLOAD_SIZE : bytes_remaining;

        while (bytes_remaining > 0) {
#pragma HLS PIPELINE
            transfer = 0;
            payload_bytes = (bytes_remaining > STREAM_WIDTH/8) ? STREAM_WIDTH/8 : bytes_remaining;
            if (func_id == 0) { // SHA256
                for (int i = 0; i < payload_bytes; i++) {
#pragma HLS UNROLL
                    transfer = req_frm_dram[req_ptr + offset + i];
                }
                to_sha256.write(transfer);
            } else if (func_id == 1) { // HLL
                for (int i = 0; i < payload_bytes; i++) {
#pragma HLS UNROLL
                    transfer = req_frm_dram[req_ptr + offset + i];
                }
                to_hll.write(transfer);
            }
            bytes_remaining -= payload_bytes;
            offset += payload_bytes;
        }

    }
}
