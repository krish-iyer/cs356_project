#include "scheduler.h"
#include "../sha256/hls/sha256.h"
#include "../hll/hls/murmur64a/hll.h"
#include <ap_int.h>
#include <hls_stream.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <thread>

#define STREAM_WIDTH 512
#define HDR_SIZE 16
#define PAYLOAD_SIZE 48
#define DRAM_SIZE 256

typedef ap_uint<STREAM_WIDTH> stream_t;

void test_fetch(){
		uint8_t *dram = (uint8_t*) malloc(DRAM_SIZE*sizeof(uint8_t));
	    uint64_t dram_addr = 0;

	    hls::stream<stream_t> sha256_out;
	    hls::stream<stream_t> hll_out;

	    std::cout << "Starting SHA256 test..." << std::endl;

	    dram[0] = 0;
	    *(uint16_t*)&dram[1] = 64;
	    uint8_t* req =  (uint8_t*)&dram[3];
	    for(int i=0;i<64;i++){
	        req[i] = 0xff;
	    }

	    fetcher(dram, sha256_out, hll_out);

	    if (!sha256_out.empty()) {
	        stream_t out = sha256_out.read();

	        ap_uint<512> hash_bits = out(511, 0); // Extract 256-bit hash
	        uint8_t hash[64];

	        for (int i = 0; i < 64; i++) {
	            hash[i] = hash_bits(i*8+8-1, i*8); // MSB first
	        }

	        std::printf("SHA256 hash: 0x");
	        for (int i = 0; i < 32; i++) {
	            std::printf("%02x", hash[i]); // Two-digit hex
	        }
	        std::printf("\n");
	        if (hash == 0) {
	            std::cout << "SHA256 test PASSED" << std::endl;
	        } else {
	            std::cout << "SHA256 test FAILED" << std::endl;
	        }
	    } else {
	        std::cout << "SHA256 test FAILED: No output" << std::endl;
	    }

	    free(dram);
}

void test_store() {

    uint8_t* dram = (uint8_t*)malloc(DRAM_SIZE * sizeof(uint8_t));
    if (!dram) {
        std::cerr << "Failed to allocate DRAM" << std::endl;
        return;
    }
    std::memset(dram, 0, DRAM_SIZE);

    hls::stream<stream_t> sha256_in;
    hls::stream<stream_t> hll_in;

    std::thread store_thread([&]() {
    	store(dram, sha256_in, hll_in);
    });

    stream_t sha256_data = 0;
    ap_uint<256> test_hash = ap_uint<256>("0x3f79bb7b435b05321651daefd374cdc681dc06faa65e374e38337b88ca046dea");
    ap_uint<256> ex_test_hash = ap_uint<256>("0x79bb7b435b05321651daefd374cdc681dc06faa65e374e38337b88ca046dea00");

    sha256_data(255, 0) = test_hash;
    sha256_in.write(sha256_data);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ap_uint<256> hash_bits = 0;
    for (int i = 0; i < 32; i++) {
        hash_bits(i*8 + 7, i*8) = dram[i];
    }
    std::printf("SHA256 hash: 0x");
    for (int i = 0; i < 32; i++) {
        std::printf("%02x", dram[i]);
    }
    std::printf("\n");
    if (hash_bits == ex_test_hash) {
        std::cout << "SHA256 store test PASSED" << std::endl;
    } else {
        std::cout << "SHA256 store test FAILED" << std::endl;
    }

    free(dram);
}



int main() {

	test_fetch();
	test_store();

    std::cout << "Simulation complete" << std::endl;
    return 0;
}
