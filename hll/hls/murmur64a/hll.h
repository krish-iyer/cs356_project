#ifndef HLL_H_
#define HLL_H_

#include <stdint.h>
#include <ap_int.h>

#define HLL_P 14 /* The greater is P, the smaller the error. */
#define HLL_Q (64-HLL_P) /* The number of bits of the hash value used for
                            determining the number of leading zeros. */
#define HLL_REGISTERS (1<<HLL_P) /* With P=14, 16384 registers. */
#define HLL_P_MASK (HLL_REGISTERS-1) /* Mask to index register. */
#define HLL_BITS 6 /* Enough to count up to 63 leading zeroes. */
#define HLL_REGISTER_MAX ((1<<HLL_BITS)-1)
#define INFINITY ((float)(1e+300 * 1e+300))


#define HLL_ALPHA_INF 0.721347520444481703680 /* constant for 0.5/ln(2) */

/* Store the value of the register at position 'regnum' into variable 'target'.
 * 'p' is an array of unsigned bytes. */
#define HLL_DENSE_GET_REGISTER(target,p,regnum) do { \
    uint8_t *_p = (uint8_t*) p; \
    uint64_t _byte = regnum*HLL_BITS/8; \
    uint64_t _fb = regnum*HLL_BITS&7; \
    uint64_t _fb8 = 8 - _fb; \
    uint64_t b0 = _p[_byte]; \
    uint64_t b1 = _p[_byte+1]; \
    target = ((b0 >> _fb) | (b1 << _fb8)) & HLL_REGISTER_MAX; \
} while(0)

/* Set the value of the register at position 'regnum' to 'val'.
 * 'p' is an array of unsigned bytes. */
#define HLL_DENSE_SET_REGISTER(p,regnum,val) do { \
    uint8_t *_p = (uint8_t*) p; \
    uint64_t _byte = (regnum)*HLL_BITS/8; \
    uint64_t _fb = (regnum)*HLL_BITS&7; \
    uint64_t _fb8 = 8 - _fb; \
    uint64_t _v = (val); \
    _p[_byte] &= ~(HLL_REGISTER_MAX << _fb); \
    _p[_byte] |= _v << _fb; \
    _p[_byte+1] &= ~(HLL_REGISTER_MAX >> _fb8); \
    _p[_byte+1] |= _v >> _fb8; \
} while(0)


uint8_t hllPatLen(ap_uint<8> *data, const uint32_t len, uint64_t *idx);
uint8_t hllSet(uint8_t *registers, uint64_t idx, uint8_t count);
uint64_t hllAdd(uint8_t *registers, ap_uint<8> *data, const uint32_t len);
void hllRegHisto(uint8_t *registers, uint32_t *reghisto);
uint64_t hllCount(uint8_t* registers);
uint64_t hllCompute(ap_uint<8> *data, const uint32_t *len, const uint32_t num_ele);

#endif // HLL_H_
