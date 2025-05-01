#include "murmur64a.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

uint64_t ref_func (const void * key, const uint32_t len, const uint32_t seed) {
    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;
    uint64_t h = seed ^ (len * m);
    const uint8_t *data = (const uint8_t *)key;
    const uint8_t *end = data + (len-(len&7));

    while(data != end) {
        uint64_t k;

        k = *((uint64_t*)data);

        k *= m;
        k ^= k >> r;
        k *= m;
        h ^= k;
        h *= m;

        data += 8;
    }

    printf("\n");

    switch(len & 7) {
    case 7: h ^= (uint64_t)data[6] << 48; /* fall-thru */
    case 6: h ^= (uint64_t)data[5] << 40; /* fall-thru */
    case 5: h ^= (uint64_t)data[4] << 32; /* fall-thru */
    case 4: h ^= (uint64_t)data[3] << 24; /* fall-thru */
    case 3: h ^= (uint64_t)data[2] << 16; /* fall-thru */
    case 2: h ^= (uint64_t)data[1] << 8; /* fall-thru */
    case 1: h ^= (uint64_t)data[0];
            h *= m; /* fall-thru */
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    return h;
}

uint8_t* create_rand_arr(uint32_t length, int min, int max) {
    // Seed the random number generator (call once, typically in main)
    srand(time(NULL));

    // Allocate memory for the array
    uint8_t* arr = (uint8_t*)malloc(length * sizeof(uint8_t));
    if (arr == NULL){
        printf("Error in allocation\n");
        return NULL; // Check for allocation failure
    }
    // Fill array with random numbers in range [min, max]
    for (int i = 0; i < length; i++) {
        arr[i] = min + rand() % (max - min + 1);
    }

    return arr;
}

int main() {

    uint8_t arr_len = 200;

    uint8_t *data;

    for (uint32_t i=8; i< arr_len ; i++){

        data = create_rand_arr(i, 1, 255);
        uint32_t seed = 0xadc83b19ULL;

        uint64_t hls_ret = murmur64a((uint64_t*)data, 8, seed);
        uint64_t ref_ret = ref_func(data, 8, seed);

        if (hls_ret != ref_ret){
            printf("hls hash doesn't match ref hash\n");
            printf("[ERROR]hls_ret: %lu ref_ret: %lu itr: %d\n", \
                   hls_ret,                                      \
                   ref_ret,                                      \
                   i );

            printf("Array\n");
            for(int j=0;j <= i;j++)
                printf("%d ",data[j]);

            printf("\n");
            free(data);
            break;
        }
        free(data);
        printf("[INFO] hls_ret: %lu ref_ret: %lu\n",hls_ret, ref_ret);
    }

    return 0;
}
