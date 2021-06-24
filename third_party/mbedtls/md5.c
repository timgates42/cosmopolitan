#include "libc/bits/bits.h"
#include "third_party/mbedtls/common.h"
#include "third_party/mbedtls/error.h"
#include "third_party/mbedtls/md5.h"
#include "third_party/mbedtls/platform.h"

asm(".ident\t\"\\n\\n\
Mbed TLS (Apache 2.0)\\n\
Copyright ARM Limited\\n\
Copyright Mbed TLS Contributors\"");
asm(".include \"libc/disclaimer.inc\"");

/* clang-format off */
/*
 *  RFC 1321 compliant MD5 implementation
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
 *  The MD5 algorithm was designed by Ron Rivest in 1991.
 *
 *  http://www.ietf.org/rfc/rfc1321.txt
 */

#if !defined(MBEDTLS_MD5_ALT)

#define GET_UINT32_LE(n,b,i) (n) = READ32LE((b) + (i))
#define PUT_UINT32_LE(n,b,i) WRITE32LE((b) + (i), n)

/**
 * \brief          Initialize MD5 context
 *
 * \param ctx      MD5 context to be initialized
 *
 * \warning        MD5 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 */
void mbedtls_md5_init( mbedtls_md5_context *ctx )
{
    memset( ctx, 0, sizeof( mbedtls_md5_context ) );
}

/**
 * \brief          Clear MD5 context
 *
 * \param ctx      MD5 context to be cleared
 *
 * \warning        MD5 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 */
void mbedtls_md5_free( mbedtls_md5_context *ctx )
{
    if( !ctx ) return;
    mbedtls_platform_zeroize( ctx, sizeof( mbedtls_md5_context ) );
}

/**
 * \brief          Clone (the state of) an MD5 context
 *
 * \param dst      The destination context
 * \param src      The context to be cloned
 *
 * \warning        MD5 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 */
void mbedtls_md5_clone( mbedtls_md5_context *dst,
                        const mbedtls_md5_context *src )
{
    *dst = *src;
}

/**
 * \brief          MD5 context setup
 *
 * \param ctx      context to be initialized
 *
 * \return         0 if successful
 *
 * \warning        MD5 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 */
int mbedtls_md5_starts_ret( mbedtls_md5_context *ctx )
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    return( 0 );
}

#if !defined(MBEDTLS_MD5_PROCESS_ALT)

/**
 * \brief          MD5 process data block (internal use only)
 *
 * \param ctx      MD5 context
 * \param data     buffer holding one block of data
 *
 * \return         0 if successful
 *
 * \warning        MD5 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 */
int mbedtls_internal_md5_process( mbedtls_md5_context *ctx,
                                  const unsigned char data[64] )
{
    struct
    {
        uint32_t X[16], A, B, C, D;
    } local;

    GET_UINT32_LE( local.X[ 0], data,  0 );
    GET_UINT32_LE( local.X[ 1], data,  4 );
    GET_UINT32_LE( local.X[ 2], data,  8 );
    GET_UINT32_LE( local.X[ 3], data, 12 );
    GET_UINT32_LE( local.X[ 4], data, 16 );
    GET_UINT32_LE( local.X[ 5], data, 20 );
    GET_UINT32_LE( local.X[ 6], data, 24 );
    GET_UINT32_LE( local.X[ 7], data, 28 );
    GET_UINT32_LE( local.X[ 8], data, 32 );
    GET_UINT32_LE( local.X[ 9], data, 36 );
    GET_UINT32_LE( local.X[10], data, 40 );
    GET_UINT32_LE( local.X[11], data, 44 );
    GET_UINT32_LE( local.X[12], data, 48 );
    GET_UINT32_LE( local.X[13], data, 52 );
    GET_UINT32_LE( local.X[14], data, 56 );
    GET_UINT32_LE( local.X[15], data, 60 );

#define S(x,n)                                                          \
    ( ( (x) << (n) ) | ( ( (x) & 0xFFFFFFFF) >> ( 32 - (n) ) ) )

#define P(a,b,c,d,k,s,t)                                                \
    do                                                                  \
    {                                                                   \
        (a) += F((b),(c),(d)) + local.X[(k)] + (t);                     \
        (a) = S((a),(s)) + (b);                                         \
    } while( 0 )

    local.A = ctx->state[0];
    local.B = ctx->state[1];
    local.C = ctx->state[2];
    local.D = ctx->state[3];

#define F(x,y,z) ((z) ^ ((x) & ((y) ^ (z))))

    P( local.A, local.B, local.C, local.D,  0,  7, 0xD76AA478 );
    P( local.D, local.A, local.B, local.C,  1, 12, 0xE8C7B756 );
    P( local.C, local.D, local.A, local.B,  2, 17, 0x242070DB );
    P( local.B, local.C, local.D, local.A,  3, 22, 0xC1BDCEEE );
    P( local.A, local.B, local.C, local.D,  4,  7, 0xF57C0FAF );
    P( local.D, local.A, local.B, local.C,  5, 12, 0x4787C62A );
    P( local.C, local.D, local.A, local.B,  6, 17, 0xA8304613 );
    P( local.B, local.C, local.D, local.A,  7, 22, 0xFD469501 );
    P( local.A, local.B, local.C, local.D,  8,  7, 0x698098D8 );
    P( local.D, local.A, local.B, local.C,  9, 12, 0x8B44F7AF );
    P( local.C, local.D, local.A, local.B, 10, 17, 0xFFFF5BB1 );
    P( local.B, local.C, local.D, local.A, 11, 22, 0x895CD7BE );
    P( local.A, local.B, local.C, local.D, 12,  7, 0x6B901122 );
    P( local.D, local.A, local.B, local.C, 13, 12, 0xFD987193 );
    P( local.C, local.D, local.A, local.B, 14, 17, 0xA679438E );
    P( local.B, local.C, local.D, local.A, 15, 22, 0x49B40821 );

#undef F

#define F(x,y,z) ((y) ^ ((z) & ((x) ^ (y))))

    P( local.A, local.B, local.C, local.D,  1,  5, 0xF61E2562 );
    P( local.D, local.A, local.B, local.C,  6,  9, 0xC040B340 );
    P( local.C, local.D, local.A, local.B, 11, 14, 0x265E5A51 );
    P( local.B, local.C, local.D, local.A,  0, 20, 0xE9B6C7AA );
    P( local.A, local.B, local.C, local.D,  5,  5, 0xD62F105D );
    P( local.D, local.A, local.B, local.C, 10,  9, 0x02441453 );
    P( local.C, local.D, local.A, local.B, 15, 14, 0xD8A1E681 );
    P( local.B, local.C, local.D, local.A,  4, 20, 0xE7D3FBC8 );
    P( local.A, local.B, local.C, local.D,  9,  5, 0x21E1CDE6 );
    P( local.D, local.A, local.B, local.C, 14,  9, 0xC33707D6 );
    P( local.C, local.D, local.A, local.B,  3, 14, 0xF4D50D87 );
    P( local.B, local.C, local.D, local.A,  8, 20, 0x455A14ED );
    P( local.A, local.B, local.C, local.D, 13,  5, 0xA9E3E905 );
    P( local.D, local.A, local.B, local.C,  2,  9, 0xFCEFA3F8 );
    P( local.C, local.D, local.A, local.B,  7, 14, 0x676F02D9 );
    P( local.B, local.C, local.D, local.A, 12, 20, 0x8D2A4C8A );

#undef F

#define F(x,y,z) ((x) ^ (y) ^ (z))

    P( local.A, local.B, local.C, local.D,  5,  4, 0xFFFA3942 );
    P( local.D, local.A, local.B, local.C,  8, 11, 0x8771F681 );
    P( local.C, local.D, local.A, local.B, 11, 16, 0x6D9D6122 );
    P( local.B, local.C, local.D, local.A, 14, 23, 0xFDE5380C );
    P( local.A, local.B, local.C, local.D,  1,  4, 0xA4BEEA44 );
    P( local.D, local.A, local.B, local.C,  4, 11, 0x4BDECFA9 );
    P( local.C, local.D, local.A, local.B,  7, 16, 0xF6BB4B60 );
    P( local.B, local.C, local.D, local.A, 10, 23, 0xBEBFBC70 );
    P( local.A, local.B, local.C, local.D, 13,  4, 0x289B7EC6 );
    P( local.D, local.A, local.B, local.C,  0, 11, 0xEAA127FA );
    P( local.C, local.D, local.A, local.B,  3, 16, 0xD4EF3085 );
    P( local.B, local.C, local.D, local.A,  6, 23, 0x04881D05 );
    P( local.A, local.B, local.C, local.D,  9,  4, 0xD9D4D039 );
    P( local.D, local.A, local.B, local.C, 12, 11, 0xE6DB99E5 );
    P( local.C, local.D, local.A, local.B, 15, 16, 0x1FA27CF8 );
    P( local.B, local.C, local.D, local.A,  2, 23, 0xC4AC5665 );

#undef F

#define F(x,y,z) ((y) ^ ((x) | ~(z)))

    P( local.A, local.B, local.C, local.D,  0,  6, 0xF4292244 );
    P( local.D, local.A, local.B, local.C,  7, 10, 0x432AFF97 );
    P( local.C, local.D, local.A, local.B, 14, 15, 0xAB9423A7 );
    P( local.B, local.C, local.D, local.A,  5, 21, 0xFC93A039 );
    P( local.A, local.B, local.C, local.D, 12,  6, 0x655B59C3 );
    P( local.D, local.A, local.B, local.C,  3, 10, 0x8F0CCC92 );
    P( local.C, local.D, local.A, local.B, 10, 15, 0xFFEFF47D );
    P( local.B, local.C, local.D, local.A,  1, 21, 0x85845DD1 );
    P( local.A, local.B, local.C, local.D,  8,  6, 0x6FA87E4F );
    P( local.D, local.A, local.B, local.C, 15, 10, 0xFE2CE6E0 );
    P( local.C, local.D, local.A, local.B,  6, 15, 0xA3014314 );
    P( local.B, local.C, local.D, local.A, 13, 21, 0x4E0811A1 );
    P( local.A, local.B, local.C, local.D,  4,  6, 0xF7537E82 );
    P( local.D, local.A, local.B, local.C, 11, 10, 0xBD3AF235 );
    P( local.C, local.D, local.A, local.B,  2, 15, 0x2AD7D2BB );
    P( local.B, local.C, local.D, local.A,  9, 21, 0xEB86D391 );

#undef F

    ctx->state[0] += local.A;
    ctx->state[1] += local.B;
    ctx->state[2] += local.C;
    ctx->state[3] += local.D;

    /* Zeroise variables to clear sensitive data from memory. */
    mbedtls_platform_zeroize( &local, sizeof( local ) );

    return( 0 );
}

#endif /* !MBEDTLS_MD5_PROCESS_ALT */

/**
 * \brief          MD5 process buffer
 *
 * \param ctx      MD5 context
 * \param input    buffer holding the data
 * \param ilen     length of the input data
 *
 * \return         0 if successful
 *
 * \warning        MD5 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 */
int mbedtls_md5_update_ret( mbedtls_md5_context *ctx,
                            const unsigned char *input,
                            size_t ilen )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t fill;
    uint32_t left;

    if( ilen == 0 )
        return( 0 );

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t) ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < (uint32_t) ilen )
        ctx->total[1]++;

    if( left && ilen >= fill )
    {
        memcpy( (void *) (ctx->buffer + left), input, fill );
        if( ( ret = mbedtls_internal_md5_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while( ilen >= 64 )
    {
        if( ( ret = mbedtls_internal_md5_process( ctx, input ) ) != 0 )
            return( ret );

        input += 64;
        ilen  -= 64;
    }

    if( ilen > 0 )
    {
        memcpy( (void *) (ctx->buffer + left), input, ilen );
    }

    return( 0 );
}

/**
 * \brief          MD5 final digest
 *
 * \param ctx      MD5 context
 * \param output   MD5 checksum result
 *
 * \return         0 if successful
 *
 * \warning        MD5 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 */
int mbedtls_md5_finish_ret( mbedtls_md5_context *ctx,
                            unsigned char output[16] )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    uint32_t used;
    uint32_t high, low;

    /*
     * Add padding: 0x80 then 0x00 until 8 bytes remain for the length
     */
    used = ctx->total[0] & 0x3F;

    ctx->buffer[used++] = 0x80;

    if( used <= 56 )
    {
        /* Enough room for padding + length in current block */
        memset( ctx->buffer + used, 0, 56 - used );
    }
    else
    {
        /* We'll need an extra block */
        memset( ctx->buffer + used, 0, 64 - used );

        if( ( ret = mbedtls_internal_md5_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        memset( ctx->buffer, 0, 56 );
    }

    /*
     * Add message length
     */
    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT32_LE( low,  ctx->buffer, 56 );
    PUT_UINT32_LE( high, ctx->buffer, 60 );

    if( ( ret = mbedtls_internal_md5_process( ctx, ctx->buffer ) ) != 0 )
        return( ret );

    /*
     * Output final state
     */
    PUT_UINT32_LE( ctx->state[0], output,  0 );
    PUT_UINT32_LE( ctx->state[1], output,  4 );
    PUT_UINT32_LE( ctx->state[2], output,  8 );
    PUT_UINT32_LE( ctx->state[3], output, 12 );

    return( 0 );
}

#endif /* !MBEDTLS_MD5_ALT */

/**
 * \brief          Output = MD5( input buffer )
 *
 * \param input    buffer holding the data
 * \param ilen     length of the input data
 * \param output   MD5 checksum result
 *
 * \return         0 if successful
 *
 * \warning        MD5 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 */
int mbedtls_md5_ret( const unsigned char *input,
                     size_t ilen,
                     unsigned char output[16] )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_md5_context ctx;

    mbedtls_md5_init( &ctx );

    if( ( ret = mbedtls_md5_starts_ret( &ctx ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_md5_update_ret( &ctx, input, ilen ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_md5_finish_ret( &ctx, output ) ) != 0 )
        goto exit;

exit:
    mbedtls_md5_free( &ctx );

    return( ret );
}

#if defined(MBEDTLS_SELF_TEST)
/*
 * RFC 1321 test vectors
 */
static const unsigned char md5_test_buf[7][81] =
{
    { "" },
    { "a" },
    { "abc" },
    { "message digest" },
    { "abcdefghijklmnopqrstuvwxyz" },
    { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" },
    { "12345678901234567890123456789012345678901234567890123456789012345678901234567890" }
};

static const size_t md5_test_buflen[7] =
{
    0, 1, 3, 14, 26, 62, 80
};

static const unsigned char md5_test_sum[7][16] =
{
    { 0xD4, 0x1D, 0x8C, 0xD9, 0x8F, 0x00, 0xB2, 0x04,
      0xE9, 0x80, 0x09, 0x98, 0xEC, 0xF8, 0x42, 0x7E },
    { 0x0C, 0xC1, 0x75, 0xB9, 0xC0, 0xF1, 0xB6, 0xA8,
      0x31, 0xC3, 0x99, 0xE2, 0x69, 0x77, 0x26, 0x61 },
    { 0x90, 0x01, 0x50, 0x98, 0x3C, 0xD2, 0x4F, 0xB0,
      0xD6, 0x96, 0x3F, 0x7D, 0x28, 0xE1, 0x7F, 0x72 },
    { 0xF9, 0x6B, 0x69, 0x7D, 0x7C, 0xB7, 0x93, 0x8D,
      0x52, 0x5A, 0x2F, 0x31, 0xAA, 0xF1, 0x61, 0xD0 },
    { 0xC3, 0xFC, 0xD3, 0xD7, 0x61, 0x92, 0xE4, 0x00,
      0x7D, 0xFB, 0x49, 0x6C, 0xCA, 0x67, 0xE1, 0x3B },
    { 0xD1, 0x74, 0xAB, 0x98, 0xD2, 0x77, 0xD9, 0xF5,
      0xA5, 0x61, 0x1C, 0x2C, 0x9F, 0x41, 0x9D, 0x9F },
    { 0x57, 0xED, 0xF4, 0xA2, 0x2B, 0xE3, 0xC9, 0x55,
      0xAC, 0x49, 0xDA, 0x2E, 0x21, 0x07, 0xB6, 0x7A }
};

/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 *
 * \warning        MD5 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 */
int mbedtls_md5_self_test( int verbose )
{
    int i, ret = 0;
    unsigned char md5sum[16];

    for( i = 0; i < 7; i++ )
    {
        if( verbose != 0 )
            mbedtls_printf( "  MD5 test #%d: ", i + 1 );

        ret = mbedtls_md5_ret( md5_test_buf[i], md5_test_buflen[i], md5sum );
        if( ret != 0 )
            goto fail;

        if( memcmp( md5sum, md5_test_sum[i], 16 ) != 0 )
        {
            ret = 1;
            goto fail;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

    return( 0 );

fail:
    if( verbose != 0 )
        mbedtls_printf( "failed\n" );

    return( ret );
}

#endif /* MBEDTLS_SELF_TEST */
