#include "murmur64a.h"
#include "hll.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


uint64_t ref_murmur64a (const void * key, const uint32_t len, const uint32_t seed) {
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

int ref_hllpatlen(unsigned char *ele, size_t elesize, uint64_t *idx) {
    uint64_t hash;
    int count;

    /* Count the number of zeroes starting from bit HLL_REGISTERS
     * (that is a power of two corresponding to the first bit we don't use
     * as index). The max run can be 64-P+1 = Q+1 bits.
     *
     * Note that the final "1" ending the sequence of zeroes must be
     * included in the count, so if we find "001" the count is 3, and
     * the smallest count possible is no zeroes at all, just a 1 bit
     * at the first position, that is a count of 1. */
    hash = ref_murmur64a(ele,elesize,0xadc83b19ULL);
    *idx = hash & HLL_P_MASK; /* Register index. */
    hash >>= HLL_P; /* Remove bits used to address the register. */
    hash |= ((uint64_t)1<<HLL_Q); /* Make sure the loop terminates
                                     and count will be <= Q+1. */

    count = __builtin_ctzll(hash) + 1; // I guess count zeros leading left
    return count;
}


int ref_hllDenseSet(uint8_t *registers, long index, uint8_t count) {
    uint8_t oldcount=0;
    //printf("ref: idx: %lu count: %d\n", index, count);

    HLL_DENSE_GET_REGISTER(oldcount,registers,index);
    //printf("ref: count > oldcount(%d)\n", oldcount);

    if (count > oldcount) {
        HLL_DENSE_SET_REGISTER(registers,index,count);
        return 1;
    } else {
        return 0;
    }
}

int ref_hllDenseAdd(uint8_t *registers, unsigned char *ele, size_t elesize) {
    uint64_t  index;
    uint8_t count = ref_hllpatlen(ele,elesize,&index);
    /* Update the register if this element produced a longer run of zeroes. */
    return ref_hllDenseSet(registers,index,count);
}


void ref_hllDenseRegHisto(uint8_t *registers, int* reghisto) {
    int j;

    for(j = 0; j < HLL_REGISTERS; j++) {
        unsigned long reg;
        HLL_DENSE_GET_REGISTER(reg,registers,j);
        reghisto[reg]++;
    }
}

double ref_hllSigma(double x) {
    if (x == 1.) return INFINITY;
    double zPrime;
    double y = 1;
    double z = x;
    do {
        x *= x;
        zPrime = z;
        z += x * y;
        y += y;
    } while(zPrime != z);
    return z;
}

double ref_hllTau(double x) {
    if (x == 0. || x == 1.) return 0.;
    double zPrime;
    double y = 1.0;
    double z = 1 - x;
    do {
        x = sqrt(x);
        zPrime = z;
        y *= 0.5;
        z -= pow(1 - x, 2)*y;
    } while(zPrime != z);
    return z / 3;
}

uint64_t ref_hllCount(uint8_t *registers) {
    double m = HLL_REGISTERS;
    double E;
    int j;

    int reghisto[64] = {0};


    ref_hllDenseRegHisto(registers,reghisto);

    double z = m * ref_hllTau((m-reghisto[HLL_Q+1])/(double)m);
    for (j = HLL_Q; j >= 1; --j) {
        z += reghisto[j];
        z *= 0.5;
    }
    z += m * ref_hllSigma(reghisto[0]/(double)m);
    E = llroundl(HLL_ALPHA_INF*m*m/z);

    return (uint64_t) E;
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

    uint8_t arr_len = 9;

    //uint8_t *data;

    uint64_t hls_idx = 0, ref_idx = 0;
    uint8_t *hls_registers = (uint8_t*)malloc(16384);
    uint8_t *ref_registers = (uint8_t*)malloc(16384);

    for (uint32_t i=8; i< arr_len ; i++){

        //data = create_rand_arr(i, 1, 255);
        uint8_t data[10] = {1,2,3,4,5,6,7,9,7,7};
        uint32_t len[10] = {1,1,1,1,1,1,1,1,1,1};
        uint64_t hls_data[10*8];

        for (int j=0; j<10;j++){
            hls_data[j*8] = data[j];
            for (int k=0;k<7;k++)
                hls_data[j*k] = 0;
        }
        uint32_t seed = 0xadc83b19ULL;

        // uint64_t hls_ret = murmur64a((ap_uint<8> *)data, i, seed);
        // uint64_t ref_ret = ref_murmur64a(data, i, seed);

        //uint32_t hls_ret = hllPatLen((ap_uint<8> *)data, i, &idx);
        //uint32_t ref_ret = ref_hllpatlen(data, i, &idx);
        uint64_t hls_ret;

        hls_ret = hllCompute((ap_uint<8>*)&data, len, 10);

        for(int i=0;i<10;i++){
            ref_hllDenseAdd(ref_registers, (uint8_t*)&data[i], 1);
        }

        //uint64_t hls_ret = hllCount(hls_registers);
        uint64_t ref_ret = ref_hllCount(ref_registers);

        uint8_t data_2[10] = {11,12,13,14,15,16,17,19,10,10};

        for (int j=0; j<10;j++){
            hls_data[j*8] = data_2[j];
            for (int k=0;k<7;k++)
                hls_data[j*k] = 0;
        }
        hls_ret = hllCompute((ap_uint<8>*)&data_2, len, 10);

        for(int i=0;i<10;i++){
            ref_hllDenseAdd(ref_registers, (uint8_t*)&data_2[i], 1);
        }

        //hls_ret = hllCount(hls_registers);
        ref_ret = ref_hllCount(ref_registers);


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
            //free(data);
            break;
        }
        //free(data);
        printf("[INFO] hls_ret: %lu ref_ret: %lu\n",hls_ret, ref_ret);
    }

    return 0;
}
