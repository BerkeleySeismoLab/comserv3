



/*
    Q660 SHA256 Routines
    Copyright 2016 by
    Kinemetrics, Inc.
    Pasadena, CA 91107 USA.

    This file is part of Lib660

    Lib660 is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Lib660 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2016-09-05 rdr Created.
    2 2019-09027 jms pure C version from https://github.com/amosnier/sha-2/blob/master/sha-256.c
    3 2020-12-24 rdr Add stdio.h header due to sprintf.
    4 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
                     remove alternate version
*/


#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef char *pchar ;



#define CHUNK_SIZE 64
#define TOTAL_LEN_LEN 8

/*
 * Comments from pseudo-code at https://en.wikipedia.org/wiki/SHA-2 are reproduced here.
 * When useful for clarification, portions of the pseudo-code are reproduced here too.
 */

/*
 * Initialize array of round constants:
 * (first 32 bits of the fractional parts of the cube roots of the first 64 primes 2..311):
 */
static const uint32_t k[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

struct buffer_state
{
    const uint8_t * p;
    size_t len;
    size_t total_len;
    int single_one_delivered;
    int total_len_delivered;
};

static inline uint32_t right_rot(uint32_t value, unsigned int count)
{
    /*
     * Defined behaviour in standard C for all count where 0 < count < 32,
     * which is what we need here.
     */
    return value >> count | value << (32 - count);
}

static void init_buf_state(struct buffer_state * state, const void * input, size_t len)
{
    state->p = input;
    state->len = len;
    state->total_len = len;
    state->single_one_delivered = 0;
    state->total_len_delivered = 0;
}

static int calc_chunk(uint8_t chunk[CHUNK_SIZE], struct buffer_state * state)
{
    size_t space_in_chunk;

    if (state->total_len_delivered) {
        return 0;
    }

    if (state->len >= CHUNK_SIZE) {
        memcpy(chunk, state->p, CHUNK_SIZE);
        state->p += CHUNK_SIZE;
        state->len -= CHUNK_SIZE;
        return 1;

    }

    memcpy(chunk, state->p, state->len);
    chunk += state->len;
    space_in_chunk = CHUNK_SIZE - state->len;
    state->p += state->len;
    state->len = 0;

    /* If we are here, space_in_chunk is one at minimum. */
    if (!state->single_one_delivered) {
        *chunk++ = 0x80;
        space_in_chunk -= 1;
        state->single_one_delivered = 1;
    }

    /*
     * Now:
     * - either there is enough space left for the total length, and we can conclude,
     * - or there is too little space left, and we have to pad the rest of this chunk with zeroes.
     * In the latter case, we will conclude at the next invokation of this function.
     */
    if (space_in_chunk >= TOTAL_LEN_LEN) {
        const size_t left = space_in_chunk - TOTAL_LEN_LEN;
        const size_t len = state->total_len * 8;
        memset(chunk, 0x00, left);
        chunk += left;

#if SIZE_MAX > UINT32_MAX
        chunk[0] = (uint8_t) (len >> 56);
        chunk[1] = (uint8_t) (len >> 48);
        chunk[2] = (uint8_t) (len >> 40);
        chunk[3] = (uint8_t) (len >> 32);
#else
        chunk[0] = 0;
        chunk[1] = 0;
        chunk[2] = 0;
        chunk[3] = 0;
#endif

#if SIZE_MAX > UINT16_MAX
        chunk[4] = (uint8_t) (len >> 24);
        chunk[5] = (uint8_t) (len >> 16);
#else
        chunk[4] = 0;
        chunk[5] = 0;
#endif

#if SIZE_MAX > UINT8_MAX
        chunk[6] = (uint8_t) (len >> 8);
#else
        chunk[6] = 0;
#endif

        chunk[7] = (uint8_t) len;

        state->total_len_delivered = 1;
    } else {
        memset(chunk, 0x00, space_in_chunk);
    }

    return 1;
}

/*
 * Limitations:
 * - len must be small enough for (8 * len) to fit in len. Otherwise, the results are unpredictable.
 * - sizeof size_t is assumed to be either 8, 16, 32 or 64. Otherwise, the results are unpredictable.
 * - Since input is a pointer in RAM, the data to hash should be in RAM, which could be a problem
 *   for large data sizes.
 * - SHA algorithms theoretically operate on bit strings. However, this implementation has no support
 *   for bit string lengths that are not multiples of eight, and it really operates on arrays of bytes.
 *   the len parameter is a number of bytes.
 */
void calc_sha_256(uint8_t hash[32], const void * input, size_t len)
{
    /*
     * Note 1: All variables are 32 bit unsigned integers and addition is calculated modulo 232
     * Note 2: For each round, there is one round constant k[i] and one entry in the message schedule array w[i], 0 = i = 63
     * Note 3: The compression function uses 8 working variables, a through h
     * Note 4: Big-endian convention is used when expressing the constants in this pseudocode,
     *     and when parsing message block data from bytes to words, for example,
     *     the first word of the input message "abc" after padding is 0x61626380
     */

    /*
     * Initialize hash values:
     * (first 32 bits of the fractional parts of the square roots of the first 8 primes 2..19):
     */

    uint32_t h0 = 0x6a09e667;
    uint32_t h1 = 0xbb67ae85;
    uint32_t h2 = 0x3c6ef372;
    uint32_t h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f;
    uint32_t h5 = 0x9b05688c;
    uint32_t h6 = 0x1f83d9ab;
    uint32_t h7 = 0x5be0cd19;

    /* 512-bit chunks is what we will operate on. */

    uint8_t chunk[64];

    struct buffer_state state;

    init_buf_state(&state, input, len);

    while (calc_chunk(chunk, &state)) {
        uint32_t a, b, c, d, e, f, g, h;

        /*
         * create a 64-entry message schedule array w[0..63] of 32-bit words
         * (The initial values in w[0..63] don't matter, so many implementations zero them here)
         * copy chunk into first 16 words w[0..15] of the message schedule array
         */
        uint32_t w[64];
        const uint8_t *p = chunk;
        int i;

        memset(w, 0x00, sizeof w);

        for (i = 0; i < 16; i++) {
            w[i] = (uint32_t) p[0] << 24 | (uint32_t) p[1] << 16 |
                   (uint32_t) p[2] << 8 | (uint32_t) p[3];
            p += 4;
        }

        /* Extend the first 16 words into the remaining 48 words w[16..63] of the message schedule array: */
        for (i = 16; i < 64; i++) {
            const uint32_t s0 = right_rot(w[i - 15], 7) ^ right_rot(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const uint32_t s1 = right_rot(w[i - 2], 17) ^ right_rot(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        /* Initialize working variables to current hash value: */
        a = h0;
        b = h1;
        c = h2;
        d = h3;
        e = h4;
        f = h5;
        g = h6;
        h = h7;

        /* Compression function main loop: */
        for (i = 0; i < 64; i++) {
            const uint32_t s1 = right_rot(e, 6) ^ right_rot(e, 11) ^ right_rot(e, 25);
            const uint32_t ch = (e & f) ^ (~e & g);
            const uint32_t temp1 = h + s1 + ch + k[i] + w[i];
            const uint32_t s0 = right_rot(a, 2) ^ right_rot(a, 13) ^ right_rot(a, 22);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t temp2 = s0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        /* Add the compressed chunk to the current hash value: */
        h0 = h0 + a;
        h1 = h1 + b;
        h2 = h2 + c;
        h3 = h3 + d;
        h4 = h4 + e;
        h5 = h5 + f;
        h6 = h6 + g;
        h7 = h7 + h;
    }

    /* Produce the final hash value (big-endian): */
    hash[0] = (uint8_t) (h0 >> 24);
    hash[1] = (uint8_t) (h0 >> 16);
    hash[2] = (uint8_t) (h0 >> 8);
    hash[3] = (uint8_t) h0;
    hash[4] = (uint8_t) (h1 >> 24);
    hash[5] = (uint8_t) (h1 >> 16);
    hash[6] = (uint8_t) (h1 >> 8);
    hash[7] = (uint8_t) h1;
    hash[8] = (uint8_t) (h2 >> 24);
    hash[9] = (uint8_t) (h2 >> 16);
    hash[10] = (uint8_t) (h2 >> 8);
    hash[11] = (uint8_t) h2;
    hash[12] = (uint8_t) (h3 >> 24);
    hash[13] = (uint8_t) (h3 >> 16);
    hash[14] = (uint8_t) (h3 >> 8);
    hash[15] = (uint8_t) h3;
    hash[16] = (uint8_t) (h4 >> 24);
    hash[17] = (uint8_t) (h4 >> 16);
    hash[18] = (uint8_t) (h4 >> 8);
    hash[19] = (uint8_t) h4;
    hash[20] = (uint8_t) (h5 >> 24);
    hash[21] = (uint8_t) (h5 >> 16);
    hash[22] = (uint8_t) (h5 >> 8);
    hash[23] = (uint8_t) h5;
    hash[24] = (uint8_t) (h6 >> 24);
    hash[25] = (uint8_t) (h6 >> 16);
    hash[26] = (uint8_t) (h6 >> 8);
    hash[27] = (uint8_t) h6;
    hash[28] = (uint8_t) (h7 >> 24);
    hash[29] = (uint8_t) (h7 >> 16);
    hash[30] = (uint8_t) (h7 >> 8);
    hash[31] = (uint8_t) h7;
}


static void hash_to_string(char s[65], const uint8_t hash[32])
{
    size_t i;

    for (i = 0; i < 32; i++) {
        s+= sprintf(s, "%02x", hash[i]);
    }
}

void sha256 (pchar inpmsg, pchar outhash)
{

    uint8_t hash[32];
    calc_sha_256(hash, inpmsg, strlen(inpmsg));
    hash_to_string(outhash, hash);

}



