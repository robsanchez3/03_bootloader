#ifndef SW_SHA256_H
#define SW_SHA256_H

#include <stdint.h>
#include <stddef.h>

#define SW_SHA256_DIGEST_SIZE  32u
#define SW_SHA256_BLOCK_SIZE   64u

typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t  buf[SW_SHA256_BLOCK_SIZE];
    uint32_t buf_len;
} sw_sha256_ctx_t;

/* Streaming SHA-256 (FIPS 180-4) — call update() as many times as needed
   (e.g. once per FatFS f_read chunk), then final() once. */
void sw_sha256_init(sw_sha256_ctx_t *ctx);
void sw_sha256_update(sw_sha256_ctx_t *ctx, const uint8_t *data, size_t len);
void sw_sha256_final(sw_sha256_ctx_t *ctx, uint8_t out[SW_SHA256_DIGEST_SIZE]);

/* Convenience: hash a single in-memory buffer in one call. */
void sw_sha256(const uint8_t *data, size_t len, uint8_t out[SW_SHA256_DIGEST_SIZE]);

#endif /* SW_SHA256_H */
