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
#include <openssl/sha.h>

#define STREAM_WIDTH 512
#define HDR_SIZE 16
#define PAYLOAD_SIZE 48
#define DRAM_SIZE 256

typedef struct {
    uint8_t func_id;
    uint16_t req_len;
    uint8_t *req;
}__attribute__((packed, aligned(1))) req_ctx;

typedef ap_axiu<STREAM_WIDTH, 1, 1, 1> stream_t;

void ref_sha256(const char *input, uint32_t len, unsigned char* hash) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input, len);
    SHA256_Final(hash, &sha256);
}

void test_top(){
    uint8_t *dram_in = (uint8_t*) malloc(DRAM_SIZE*sizeof(uint8_t));
    uint8_t *dram_out = (uint8_t*) malloc(DRAM_SIZE*sizeof(uint8_t));

    char data_ref[13] = "hello world!";

    req_ctx sha256_req;
    sha256_req.func_id = 0;
    sha256_req.req_len = 12;
    sha256_req.req = (uint8_t*) malloc(13*sizeof(uint8_t));

    char hash_str[13] = "hello world!";
    printf("Hash in str: 0x");
    for (int i = 0; i < sha256_req.req_len; i++) {
        printf("%02x", hash_str[i]); // Two-digit hex
    }
    for (int i=0;i<12;i++){
        sha256_req.req[i] =  hash_str[i];
    }
    dram_in[0] = sha256_req.func_id;
    dram_in[1] = sha256_req.req_len;

    uint8_t *dram_req_ptr = &dram_in[3];

    printf("sizeof req: %d\n",sizeof(sha256_req));
    memcpy(dram_req_ptr, sha256_req.req, sha256_req.req_len);
    //std::thread store_thread([&]() {
    printf("DRAM IN: 0x");
    for (int i = 0; i < 16; i++) {
        printf("%02x", dram_in[i]); // Two-digit hex
    }

    top(dram_in, dram_out);
    //});
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::printf("hls_hash: 0x");
    for (int i = 0; i < 33; i++) {
        std::printf("%02x", dram_out[i]);
    }
    std::printf("\n");

    uint8_t ref_hash[32];
    ref_sha256(data_ref, 12, ref_hash);
    printf("ref_hash: 0x");
    for (int i = 0; i < 32; i++) {
        printf("%x", ref_hash[i]);
    }
    printf("\n");

    for (int i = 0; i < 32; i++) {
        if (dram_out[i+1] != ref_hash[i]) {
            printf("ref_hash: 0x");
            for (int j = 0; j < 32; j++) {
                printf("%x", ref_hash[j]);
            }
            printf("\n");
            printf("hls_hash: 0x");
            for (int j = 0; j < 32; j++) {
                printf("%x", dram_out[j+1]);
            }
            printf("\n");
            goto cleanup;
        }
    }

    printf("Test passed!\n");

cleanup:
    printf("");

    free(dram_in);
    free(dram_out);



}

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

        ap_uint<512> hash_bits = out.data(511, 0); // Extract 256-bit hash
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

    stream_t sha256_data;
    ap_uint<256> test_hash = ap_uint<256>("0x3f79bb7b435b05321651daefd374cdc681dc06faa65e374e38337b88ca046dea");
    ap_uint<256> ex_test_hash = ap_uint<256>("0x79bb7b435b05321651daefd374cdc681dc06faa65e374e38337b88ca046dea00");

    sha256_data.data(255, 0) = test_hash;
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

    //test_fetch();
    //test_store();
    test_top();

    std::cout << "Simulation complete" << std::endl;
    return 0;
}
