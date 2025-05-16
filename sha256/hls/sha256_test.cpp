#include "sha256.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <thread>

void ref_sha256(const char *input, uint32_t len, unsigned char* hash) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input, len);
    SHA256_Final(hash, &sha256);
}


int main() {
    char data[13] = "hello world!";
    char data_ref[13] = "hello world!";

    uint32_t len = 12;

    hls::stream<stream_t> in_stream;
    hls::stream<stream_t> out_stream;

    std::thread store_thread([&]() {
        sha256(in_stream, out_stream);
    });

    stream_t transfer;
    transfer.data = 0;
    transfer.data(7, 0) = 0;
    for (int i = 0; i < len; i++) {
        transfer.data(i*8 + 7, i*8) = (uint8_t)data[i];
    }
    transfer.last = 1;
    transfer.keep = len;
    in_stream.write(transfer);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    uint8_t hash[32];
    if (!out_stream.empty()) {
        stream_t hash_str = out_stream.read();
        ap_uint<256> hash_bits = hash_str.data(255, 0);
        for (int i = 0; i < 32; i++) {
            hash[i] = hash_bits(i*8 + 7, i*8);
        }
        printf("hash: 0x");
        for (int i = 0; i < 32; i++) {
            printf("%x", hash[i]);
        }
        printf("\n");
    } else {
        printf("No output from HLS SHA256\n");
        goto cleanup;
    }

    uint8_t ref_hash[32];
    ref_sha256(data_ref, len, ref_hash);
    printf("ref_hash: 0x");
    for (int i = 0; i < 32; i++) {
        printf("%x", ref_hash[i]);
    }
    printf("\n");

    for (int i = 0; i < 32; i++) {
        if (hash[i] != ref_hash[i]) {
            printf("ref_hash: 0x");
            for (int j = 0; j < 32; j++) {
                printf("%x", ref_hash[j]);
            }
            printf("\n");
            printf("hls_hash: 0x");
            for (int j = 0; j < 32; j++) {
                printf("%x", hash[j]);
            }
            printf("\n");
            goto cleanup;
        }
    }

    printf("Test passed!\n");

cleanup:
    printf("");
    return 0;
}
