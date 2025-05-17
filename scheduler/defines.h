#ifndef DEFINES_H_
#define DEFINES_H_

#include <ap_int.h>

#define STREAM_WIDTH 512
#define HDR_SIZE 16
#define PAYLOAD_SIZE 64//((STREAM_WIDTH/8) - HDR_SIZE)
#define BYTE_WIDTH STREAM_WIDTH/8

//typedef ap_axiu<STREAM_WIDTH, 1, 1, 1> stream_t;
typedef struct {
    ap_uint<STREAM_WIDTH> data; // 512-bit data
    ap_uint<BYTE_WIDTH> keep;   // tkeep: 1 bit per byte (64 bits)
    ap_uint<1> last;            // tlast: 1 bit to indicate last packet
    // Optional: add tstrb, tuser, tid, tdest if needed
    // ap_uint<BYTE_WIDTH> strb; // tstrb: byte strobe
    // ap_uint<8> user;         // tuser: user-defined signals
    // ap_uint<8> id;           // tid: stream identifier
    // ap_uint<8> dest;         // tdest: destination identifier
}stream_t;



#endif // DEFINES_H_
