#include<stdio.h>
#include<bitset>
#include<intrin.h>

#define NUM  4294967296;
#define MAX_LEN 2<<12
#define rol(x,j) ((x<<j)|(unsigned int(x)>>(32-j)))
#define P0(x) ((x) ^ rol((x), 9) ^ rol((x), 17))
#define P1(x) ((x) ^ rol((x), 15) ^ rol((x), 23))
#define FF0(x, y, z) ((x) ^ (y) ^ (z))
#define FF1(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define GG0(x, y, z) ((x) ^ (y) ^ (z))
#define GG1(x, y, z) (((x) & (y)) | ((~(x)) & (z)))
#define _m128i_left(a, k) _mm_or_si128(_mm_slli_epi32(a, k&31), _mm_srli_epi32(a, 32 - k&31))
#define _m128i_P1_simd(X) _mm_xor_si128(_mm_xor_si128(X, _m128i_left(X, 15)), _m128i_left(X, 23))
#define byte_swap32(x) ((x & 0xff000000) >> 24) |((x & 0x00ff0000) >> 8) |((x & 0x0000ff00) << 8) |((x & 0x000000ff) << 24)

uint32_t IV[8] = { 0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600, 0xa96f30bc, 0x163138aa, 0xe38dee4d ,0xb0fb0e4e };
char* plaintext_after_stuffing;



static void dump_buf(char* hash, size_t lenth)
{
    for (size_t i = 0; i < lenth; i++) {
        printf("%02x", (unsigned char)hash[i]);
    }
}


uint32_t bit_stuffing(uint8_t* plaintext, size_t len) {
    uint64_t bit_len = len * uint64_t(8);
    uint64_t the_num_of_fin_group = (bit_len >> 3);
    uint32_t the_mod_of_fin_froup = bit_len % 512;
    uint32_t lenth_for_p_after_stuffing;
    size_t i, j, k = the_mod_of_fin_froup < 448 ? 1 : 2;
    lenth_for_p_after_stuffing = (((len >> 6) + k) << 6);
    plaintext_after_stuffing = new char[lenth_for_p_after_stuffing];

    __m128i* src = (__m128i*)plaintext;
    __m128i* dst = (__m128i*)plaintext_after_stuffing;
    for (i = 0; i < len; i += 16) {
        __m128i temp = _mm_loadu_si128(src++);
        _mm_storeu_si128(dst++, temp);
    }
    plaintext_after_stuffing[len] = static_cast <char>(0x80);
    for (i = len + 1; i < lenth_for_p_after_stuffing - 8; i++) {
        plaintext_after_stuffing[i] = 0;
    }

    for (i = lenth_for_p_after_stuffing - 8, j = 0; i < lenth_for_p_after_stuffing; i++, j++) {
        plaintext_after_stuffing[i] = ((char*)&bit_len)[7 - j];
    }
    return lenth_for_p_after_stuffing;
}


void CF_for_simd(uint32_t* V, int* BB) {
    uint32_t W[68];
    uint32_t W_t[64];
    __m128i w16, w9, w13, w3, w6, w16_or_w9, rsl_w3, rsl_w13, w16_or_w9_or_rsl_w3, rsl_w13_or_w6, P, re;
    uint32_t temp, SS1, SS2, TT1, TT2;

    for (int i = 0; i < 16; i++)
    {
        W[i] = byte_swap32(BB[i]);
    }

    for (int j = 4; j < 17; j++)
    {
        W[(j << 2)] = P1(W[(j << 2) - 16] ^ W[(j << 2) - 9] ^ (rol(W[(j << 2) - 3], 15))) ^ rol(W[(j << 2) - 13], 7) ^ W[(j << 2) - 6];
        w16 = _mm_setr_epi32(W[(j << 2) - 16], W[(j << 2) - 15], W[(j << 2) - 14], W[(j << 2) - 13]);;
        w9 = _mm_setr_epi32(W[(j << 2) - 9], W[(j << 2) - 8], W[(j << 2) - 7], W[(j << 2) - 6]);
        w13 = _mm_setr_epi32(W[(j << 2) - 13], W[(j << 2) - 12], W[(j << 2) - 11], W[(j << 2) - 10]);
        w3 = _mm_setr_epi32(W[(j << 2) - 3], W[(j << 2) - 2], W[(j << 2) - 1], W[(j << 2)]);
        w6 = _mm_setr_epi32(W[(j << 2) - 6], W[(j << 2) - 5], W[(j << 2) - 4], W[(j << 2) - 3]);
        w16_or_w9 = _mm_xor_si128(w16, w9);
        rsl_w3 = _m128i_left(w3, 15);
        rsl_w13 = _m128i_left(w13, 7);
        w16_or_w9_or_rsl_w3 = _mm_xor_si128(w16_or_w9, rsl_w3);
        rsl_w13_or_w6 = _mm_xor_si128(rsl_w13, w6);
        P = _m128i_P1_simd(w16_or_w9_or_rsl_w3);
        re = _mm_xor_si128(P, rsl_w13_or_w6);
        memcpy(&W[(j << 2)], (int*)&re, 16);
    }
    for (int i = 0; i < 64; i += 4) {
        __m128i w1 = _mm_loadu_si128((__m128i*)(W + i));
        __m128i w2 = _mm_loadu_si128((__m128i*)(W + i + 4));
        __m128i w_t = _mm_xor_si128(w1, w2);
        _mm_storeu_si128((__m128i*)(W_t + i), w_t);
    }
    int A = V[0], B = V[1], C = V[2], D = V[3], E = V[4], F = V[5], G = V[6], H = V[7];

    for (size_t i = 0; i < 16; i++) {
        temp = rol(A, 12) + E + rol(0x79cc4519, i);
        SS1 = rol(temp, 7);
        SS2 = SS1 ^ rol(A, 12);
        TT1 = FF0(A, B, C) + D + SS2 + W_t[i];
        TT2 = GG0(E, F, G) + H + SS1 + W[i];
        D = C;
        C = rol(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = rol(F, 19);
        F = E;
        E = P0(TT2);

    }
    for (size_t i = 16; i < 64; i++) {
        temp = rol(A, 12) + E + rol(0x7a879d8a, i % 32);
        SS1 = rol(temp, 7);
        SS2 = SS1 ^ rol(A, 12);
        TT1 = FF1(A, B, C) + D + SS2 + W_t[i];
        TT2 = GG1(E, F, G) + H + SS1 + W[i];
        D = C;
        C = rol(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = rol(F, 19);
        F = E;
        E = P0(TT2);
    }
    V[0] = A ^ V[0], V[1] = B ^ V[1], V[2] = C ^ V[2], V[3] = D ^ V[3], V[4] = E ^ V[4], V[5] = F ^ V[5], V[6] = G ^ V[6], V[7] = H ^ V[7];

}


void sm3_simd(uint8_t* plaintext, uint32_t* hash_val, size_t len) {
    size_t i;
    uint32_t n = bit_stuffing(plaintext, len) / 64;
    for (i = 0; i < n; i++) {
        CF_for_simd(IV, (int*)&plaintext_after_stuffing[i * 64]);
    }
    for (i = 0; i < 8; i++) {
        hash_val[i] = byte_swap32(IV[i]);
    }
}



int main() {
    uint8_t plaintext[MAX_LEN] = "sdusdusdu\0";
    uint32_t hash_val[8];
    size_t len = 0;
    for (size_t i = 0; i < MAX_LEN; i++) {
        if (plaintext[i] == '\0')break;
        len++;
    }

    sm3_simd(plaintext, hash_val, len);
    dump_buf((char*)hash_val, 32);

    return 0;
}