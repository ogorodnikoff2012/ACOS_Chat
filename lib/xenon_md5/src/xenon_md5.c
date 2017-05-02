#include "xenon_md5.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define DIGITS "0123456789ABCDEF"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;
typedef u32(* func)(u32, u32, u32);

static u32 T[64], S[64];

__attribute__((constructor))
static void init_xenonmd5() {
    for (int i = 0; i < 64; ++i) {
        T[i] = (u32)((1ull << 32) * fabs(sin((double)(i + 1))));
        S[i] = 0;
    }
    S[0] = 7;
    S[1] = 12;
    S[2] = 17;
    S[3] = 22;
    S[16] = 5;
    S[17] = 9;
    S[18] = 14;
    S[19] = 20;
    S[32] = 4;
    S[33] = 11;
    S[34] = 16;
    S[35] = 23;
    S[48] = 6;
    S[49] = 10;
    S[50] = 15;
    S[51] = 21;
    for (int i = 0; i < 64; ++i) {
        if (S[i] == 0) {
            S[i] = S[i - 4];
        }
    }
}

static u32 funcF(u32 x, u32 y, u32 z) {
    return (x & y) | (~x & z);
}

static u32 funcG(u32 x, u32 y, u32 z) {
    return (x & z) | (~z & y);
}

static u32 funcH(u32 x, u32 y, u32 z) {
    return x ^ y ^ z;
}

static u32 funcI(u32 x, u32 y, u32 z) {
    return y ^ (~z | x);
}

static func funcs[] = {funcF, funcG, funcH, funcI};

static u32 rotateLeft(u32 n, int shift) {
    u32 ans = (n << (shift & 0x1f)) | (n >> (-shift & 0x1f));
    return ans;
}

static u32 fromLE(u8 *arr, int offset) {
    u32 ans = 0;
    for (int i = 3; i >= 0; i--) {
        ans = (ans << 8) | arr[offset + i];
    }
    return ans;
}

static void md5round(u32 *a, u32 b, u32 c, u32 d, int k, int s, int i, u8 *block) {
    *a = b + rotateLeft(*a + funcs[i >> 4](b, c, d) + fromLE(block, 4 * k) + T[i], s);
}

static int getKFromI(int i) {
    if (i < 16) {
        return i & 0xf;
    } else if (i < 32) {
        return (5 * i + 1) & 0xf;
    } else if (i < 48) {
        return (3 * i + 5) & 0xf;
    } else {
        return (7 * i) & 0xf;
    }
}

u8 *md5sum(const char *str) {
    int length = strlen(str);
    int messageSize = (((length + 8) >> 6) + 1) << 6; // 64 * n >= length + 9
    u8 *message = malloc(messageSize);
    for (int i = 0; i < length; ++i) {
        message[i] = str[i];
    }

    message[length] = 0x80;
    for (int i = length + 1; i < messageSize; ++i) {
        message[i] = 0;
    }

    u64 msgBitLen = (u64) length << 3;
    for (int i = 0; i < 8; ++i) {
        message[messageSize - 8 + i] = (msgBitLen >> (8 * i)) & 0xff;
    }

    u32 A = 0x67452301;
    u32 B = 0xEFCDAB89;
    u32 C = 0x98BADCFE;
    u32 D = 0x10325476;

    for (int off = 0; off < messageSize; off += 64) {
        u32 AA = A, BB = B, CC = C, DD = D;
        for (int i = 0; i < 64; ++i) {
            md5round(&A, B, C, D, getKFromI(i), S[i], i, message + off);
            u32 t = D;
            D = C;
            C = B;
            B = A;
            A = t;
        }
        A += AA;
        B += BB;
        C += CC;
        D += DD;
    }

    free(message);
    void *ans = malloc(16);
    ((u32 *) ans)[0] = A;
    ((u32 *) ans)[1] = B;
    ((u32 *) ans)[2] = C;
    ((u32 *) ans)[3] = D;
    return (u8 *) ans;
}

char *md5hex(const char *str) {
    char *msg = malloc(33);
    msg[32] = '\0';

    u8 *sum = md5sum(str);
    for (int i = 0; i < 16; ++i) {
        msg[(i << 1) + 1] = DIGITS[sum[i] & 0x0f];
        msg[(i << 1) + 0] = DIGITS[(sum[i] >> 4) & 0x0f];
    }
    free(sum);
    return msg;
}
