#ifndef SW_AES_H
#define SW_AES_H

#include <stdint.h>

/* AES-256-ECB: encrypt one 16-byte block.
   key: 32 bytes,  in/out: 16 bytes each. */
void sw_aes256_ecb_encrypt(const uint8_t key[32],
                           const uint8_t in[16],
                           uint8_t       out[16]);

/* AES-256-CBC: decrypt a buffer.
   key     : 32 bytes
   iv      : 16 bytes (prepended IV, NOT modified)
   in      : ciphertext
   out     : plaintext (may alias in for in-place)
   len_bytes: must be a multiple of 16 */
void sw_aes256_cbc_decrypt(const uint8_t  key[32],
                           const uint8_t  iv[16],
                           const uint8_t *in,
                           uint8_t       *out,
                           uint32_t       len_bytes);

/* AES-256-CBC: decrypt buf in-place, chunk by chunk (streaming decryption).
   key      : 32 bytes
   iv_inout : 16 bytes — on input: current IV / previous ciphertext block;
              on output: last ciphertext block (enables chaining successive calls)
   buf      : ciphertext overwritten with plaintext in-place
   len_bytes: must be a multiple of 16 */
void sw_aes256_cbc_decrypt_inplace(const uint8_t key[32],
                                    uint8_t       iv_inout[16],
                                    uint8_t      *buf,
                                    uint32_t      len_bytes);

#endif /* SW_AES_H */
