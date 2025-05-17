#include "scheduler.h"
#include "../sha256/hls/sha256.h"
#include "../hll/hls/murmur64a/hll.h"
#include <ap_int.h>
#include <hls_stream.h>

void fetcher(volatile uint8_t* req_frm_dram, hls::stream<stream_t> &to_sha256, hls::stream<stream_t> &to_hll) {
#pragma HLS INTERFACE m_axi port=req_frm_dram depth=256
#pragma HLS INTERFACE axis port=to_sha256
#pragma HLS INTERFACE axis port=to_hll
#pragma HLS INTERFACE s_axilite port=return

    uint8_t func_id = req_frm_dram[0];
    uint16_t req_len = (uint16_t)req_frm_dram[1];
    volatile uint8_t *req_ptr = (uint8_t*)&req_frm_dram[3];

    if (req_len > 0) {
        stream_t transfer;

        uint16_t bytes_remaining = req_len;
        uint16_t offset = 0;

        //while (bytes_remaining > 0) {
            transfer.data = 0;
            transfer.last = 0;
            uint16_t payload_bytes = (bytes_remaining > PAYLOAD_SIZE) ? PAYLOAD_SIZE : bytes_remaining;
            if (func_id == 0) { // SHA256
                for (int i = 0; i < payload_bytes; i++) {
                    transfer.data(i*8 + 7, i*8) = *(req_ptr + offset + i);
                }
                transfer.last = (bytes_remaining <= PAYLOAD_SIZE) ? 1 : 0;
                transfer.keep = payload_bytes;
                to_sha256.write(transfer);
            } else if (func_id == 1) { // HLL
                for (int i = 0; i < payload_bytes; i++) {
                    transfer.data(i*8 + 7, i*8) = *(req_ptr + offset + i);
                }
                transfer.last = (bytes_remaining <= PAYLOAD_SIZE) ? 1 : 0;
                to_hll.write(transfer);
            }
            bytes_remaining -= payload_bytes;
            offset += payload_bytes;
            //}
    }
}

void store(volatile uint8_t* dram_out, hls::stream<stream_t>& sha256_in, hls::stream<stream_t>& hll_in) {
#pragma HLS INTERFACE m_axi port=dram_out depth=256
#pragma HLS INTERFACE axis port=sha256_in
#pragma HLS INTERFACE axis port=hll_in
#pragma HLS INTERFACE s_axilite port=return

    volatile uint8_t *out_ptr = dram_out;
    if (!sha256_in.empty()) { // SHA256
        stream_t in = sha256_in.read();
        ap_uint<256> hash = in.data(255, 0);
        *out_ptr = 0; // function id
        out_ptr++;
        for (int i = 0; i < 32; i++) {
#pragma HLS UNROLL
            *(out_ptr + i) = hash(i*8 + 7, i*8);
        }
        out_ptr += 32;
    }
    if (!hll_in.empty()) { // HLL
        stream_t in = hll_in.read();
        ap_uint<32> cardinality = in.data(31, 0);
        *out_ptr = 1;
        out_ptr++;
        for (int i = 0; i < 4; i++) {
#pragma HLS UNROLL
            *(out_ptr + i) = cardinality(i*8 + 7, i*8);
        }
        out_ptr += 4;
    }
}

void top(volatile uint8_t* req_frm_dram, volatile uint8_t* dram_out) {
#pragma HLS INTERFACE m_axi port=req_frm_dram depth=256
#pragma HLS INTERFACE m_axi port=dram_out depth=256
#pragma HLS INTERFACE s_axilite port=return

    hls::stream<stream_t> to_sha256;
    hls::stream<stream_t> to_hll;
    hls::stream<stream_t> sha256_out;
    hls::stream<stream_t> hll_out;

#pragma HLS STREAM variable=to_sha256 depth=8
#pragma HLS STREAM variable=to_hll depth=8
#pragma HLS STREAM variable=sha256_out depth=8
#pragma HLS STREAM variable=hll_out depth=8

    fetcher(req_frm_dram, to_sha256, to_hll);
    sha256(to_sha256, sha256_out);
    hllCompute(to_hll, hll_out);
    store(dram_out, sha256_out, hll_out);
}
