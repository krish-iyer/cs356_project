#include "sha256.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <openssl/sha.h>
#include <stdlib.h>

void ref_sha256(const char *input, uint32_t len, unsigned char* hash) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input, len);
    SHA256_Final(hash, &sha256);
}

int main (){
    char data[13] = "hello world!";
    //char data[12] = "dlrow olleh";
    int len = strlen(data);
    uint8_t hash[32] = {0} ; // (uint8_t*)malloc(32);
    sha256(data, 12, hash);
    printf("hash: 0x");
    for(int i=0;i<32;i++)
        printf("%x",hash[i]);
    printf("\n");

    uint8_t ref_hash[32];

    ref_sha256(data, 12, ref_hash);
    printf("ref_hash: 0x");
    for(int i=0;i<32;i++)
        printf("%x",ref_hash[i]);
    printf("\n");

    for(int i=0;i<32;i++){
        if(hash[i] != ref_hash[i]){
            printf("ref_hash: 0x");
            for(int j=0;j<32;j++)
                printf("%x",ref_hash[j]);
            printf("\n");
            printf("hls_hash: 0x");
            for(int j=0;j<32;j++)
                printf("%x",hash[j]);
            printf("\n");
            goto cleanup;
        }
    }

    printf("Test passed!\n");

cleanup:
    printf("");
    return 0;
}
