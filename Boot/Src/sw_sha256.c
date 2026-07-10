/* sw_sha256.c — Software SHA-256 (FIPS 180-4).
   Used to fingerprint firmware files for the .sig authorization scheme
   (see Plan_Cifrado_Bootloader.txt). Streaming API so large files can be
   hashed directly from FatFS reads without loading them fully into RAM. */

#include "sw_sha256.h"
#include <string.h>

static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32u - n)); }

static void sha256_transform(sw_sha256_ctx_t *ctx, const uint8_t block[SW_SHA256_BLOCK_SIZE])
{
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    int i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[4*i]   << 24) | ((uint32_t)block[4*i+1] << 16)
             | ((uint32_t)block[4*i+2] <<  8) |  (uint32_t)block[4*i+3];
    }
    for (i = 16; i < 64; i++) {
        uint32_t s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
        uint32_t s1 = rotr(w[i-2], 17) ^ rotr(w[i-2], 19)  ^ (w[i-2]  >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];

    for (i = 0; i < 64; i++) {
        uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + S1 + ch + K[i] + w[i];
        uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = S0 + maj;

        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

void sw_sha256_init(sw_sha256_ctx_t *ctx)
{
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
    ctx->bitlen  = 0;
    ctx->buf_len = 0;
}

void sw_sha256_update(sw_sha256_ctx_t *ctx, const uint8_t *data, size_t len)
{
    size_t i = 0;

    while (i < len) {
        size_t take = SW_SHA256_BLOCK_SIZE - ctx->buf_len;
        if (take > len - i) take = len - i;

        memcpy(ctx->buf + ctx->buf_len, data + i, take);
        ctx->buf_len += (uint32_t)take;
        i            += take;

        if (ctx->buf_len == SW_SHA256_BLOCK_SIZE) {
            sha256_transform(ctx, ctx->buf);
            ctx->bitlen += (uint64_t)SW_SHA256_BLOCK_SIZE * 8u;
            ctx->buf_len = 0;
        }
    }
}

void sw_sha256_final(sw_sha256_ctx_t *ctx, uint8_t out[SW_SHA256_DIGEST_SIZE])
{
    uint64_t total_bits = ctx->bitlen + (uint64_t)ctx->buf_len * 8u;
    uint8_t  pad = 0x80u;
    int i;

    sw_sha256_update(ctx, &pad, 1u);

    pad = 0x00u;
    while (ctx->buf_len != 56u)
        sw_sha256_update(ctx, &pad, 1u);

    for (i = 7; i >= 0; i--) {
        uint8_t b = (uint8_t)(total_bits >> (8u * (uint32_t)i));
        sw_sha256_update(ctx, &b, 1u);
    }

    for (i = 0; i < 8; i++) {
        out[4*i]   = (uint8_t)(ctx->state[i] >> 24);
        out[4*i+1] = (uint8_t)(ctx->state[i] >> 16);
        out[4*i+2] = (uint8_t)(ctx->state[i] >>  8);
        out[4*i+3] = (uint8_t) ctx->state[i];
    }
}

void sw_sha256(const uint8_t *data, size_t len, uint8_t out[SW_SHA256_DIGEST_SIZE])
{
    sw_sha256_ctx_t ctx;
    sw_sha256_init(&ctx);
    sw_sha256_update(&ctx, data, len);
    sw_sha256_final(&ctx, out);
}
