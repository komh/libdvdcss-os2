/**
 * \file libdvdcpxm.c
 * \author Maxim V.Anisiutkin <Maxim.Anisiutkin@gmail.com>
 * \author Saifelden Ismail <saifeldenmi@gmail.com>
 *
 * \brief Integration of libdvdcpxm functionality into libdvdcss.
 *
 * This file adapts core logic from libdvdcpxm for use in libdvdcss, allowing
 * improved support for CPPM-protected DVD-Audio discs.
 */

/*
 * Copyright (C) 1999-2025 VideoLAN
 * Copyright (C) Maxim V.Anisiutkin <Maxim.Anisiutkin@gmail.com>
 * Copyright (C) 2025 Saifelden Ismail <saifeldenmi@gmail.com>
 *
 * This file is part of libdvdcss.
 *
 * libdvdcss is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdcss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdcss; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpxm.h"
#include "dvdcss/dvdcpxm.h"
#include "ioctl.h"
#include "device.h"
#include "bswap.h"
#include "libdvdcpxm.h"
#include "libdvdcss.h"

#define IS_SYNC_CODE(word) \
        ( (word)[0] == 0x00 && (word)[1] == 0x00 && (word)[2] == 0x01 && (word)[3] == 0xBA )

/* these values are used by libdvdcpxm to process the Media Key Block */
/* They are present inside DVD-Audio players and are used in conjunction with keys taken from the disc in order to generate the final media_key used by the C2 Cypher */
uint32_t sbox_f[256];

static uint8_t sbox[256] = {
    0x3a, 0xd0, 0x9a, 0xb6, 0xf5, 0xc1, 0x16, 0xb7, 
    0x58, 0xf6, 0xed, 0xe6, 0xd9, 0x8c, 0x57, 0xfc, 
    0xfd, 0x4b, 0x9b, 0x47, 0x0e, 0x8e, 0xff, 0xf3, 
    0xbb, 0xba, 0x0a, 0x80, 0x15, 0xd7, 0x2b, 0x36, 
    0x6a, 0x43, 0x5a, 0x89, 0xb4, 0x5d, 0x71, 0x19, 
    0x8f, 0xa0, 0x88, 0xb8, 0xe8, 0x8a, 0xc3, 0xae, 
    0x7c, 0x4e, 0x3d, 0xb5, 0x96, 0xcc, 0x21, 0x00, 
    0x1a, 0x6b, 0x12, 0xdb, 0x1f, 0xe4, 0x11, 0x9d, 
    0xd3, 0x93, 0x68, 0xb0, 0x7f, 0x3b, 0x52, 0xb9, 
    0x94, 0xdd, 0xa5, 0x1b, 0x46, 0x60, 0x31, 0xec, 
    0xc9, 0xf8, 0xe9, 0x5e, 0x13, 0x98, 0xbf, 0x27, 
    0x56, 0x08, 0x91, 0xe3, 0x6f, 0x20, 0x40, 0xb2, 
    0x2c, 0xce, 0x02, 0x10, 0xe0, 0x18, 0xd5, 0x6c, 
    0xde, 0xcd, 0x87, 0x79, 0xaf, 0xa9, 0x26, 0x50, 
    0xf2, 0x33, 0x92, 0x6e, 0xc0, 0x3f, 0x39, 0x41, 
    0xaa, 0x5b, 0x7d, 0x24, 0x03, 0xd6, 0x2f, 0xeb, 
    0x0b, 0x99, 0x86, 0x4c, 0x51, 0x45, 0x8d, 0x2e, 
    0xef, 0x07, 0x7b, 0xe2, 0x4d, 0x7a, 0xfe, 0x25, 
    0x5c, 0x29, 0xa2, 0xa8, 0xb1, 0xf0, 0xb3, 0xc4, 
    0x30, 0x7e, 0x63, 0x38, 0xcb, 0xf4, 0x4f, 0xd1, 
    0xdf, 0x44, 0x32, 0xdc, 0x17, 0x5f, 0x66, 0x2a, 
    0x81, 0x9e, 0x77, 0x4a, 0x65, 0x67, 0x34, 0xfa, 
    0x54, 0x1e, 0x14, 0xbe, 0x04, 0xf1, 0xa7, 0x9c, 
    0x8b, 0x37, 0xee, 0x85, 0xab, 0x22, 0x0f, 0x69, 
    0xc5, 0xd4, 0x05, 0x84, 0xa4, 0x73, 0x42, 0xa1, 
    0x64, 0xe1, 0x70, 0x83, 0x90, 0xc2, 0x48, 0x0d, 
    0x61, 0x1c, 0xc6, 0x72, 0xfb, 0x76, 0x74, 0xe7, 
    0x01, 0xd8, 0xc8, 0xd2, 0x75, 0xa3, 0xcf, 0x28, 
    0x82, 0x1d, 0x49, 0x35, 0xc7, 0xbd, 0xca, 0xa6, 
    0xac, 0x0c, 0x62, 0xad, 0xf9, 0x3c, 0xea, 0x2d, 
    0x59, 0xda, 0x3e, 0x97, 0x6d, 0x09, 0xf7, 0x55, 
    0xe5, 0x23, 0x53, 0x9f, 0x06, 0xbc, 0x95, 0x78, 
};

static device_key_t cppm_device_keys[] =
{
    { 0x00, 0x4821, 0x6d05086b755c81 },
    { 0x01, 0x091c, 0x97ace18dd26973 },
    { 0x02, 0x012a, 0xfefc0a25a38d42 },
    { 0x03, 0x469b, 0x0780491970db2c },
    { 0x04, 0x0f9b, 0x0bedd116d43484 },
    { 0x05, 0x59b2, 0x566936bcebe294 },
    { 0x06, 0x5fc8, 0xdc610f649b1fc0 },
    { 0x07, 0x11de, 0x6ee01d3872c2d9 },
    { 0x08, 0x52b6, 0xd0132c376e439b },
    { 0x09, 0x135f, 0x800faa66206922 },
    { 0x0a, 0x3806, 0x9d1aa1460885c2 },
    { 0x0b, 0x2da2, 0x9833f21818ba33 },
    { 0x0c, 0x113f, 0xd50aa7d022045a },
    { 0x0d, 0x11ec, 0x88abee7bb83a32 },
    { 0x0e, 0x071b, 0x9b45eea4e7d140 },
    { 0x0f, 0x5c55, 0x5a49f860cca5cf },

    { 0x00, 0x0375, 0x1a12793404c279 },
    { 0x01, 0x4307, 0x61418b44cea550 },
    { 0x02, 0x1f70, 0x52bde5b73adcda },
    { 0x03, 0x1bbc, 0x70a031ae493159 },
    { 0x04, 0x1f9d, 0x0a570636aedb61 },
    { 0x05, 0x4e7b, 0xc313563e7883e9 },
    { 0x06, 0x07c4, 0x32c55f7bc42d45 },
    { 0x07, 0x4216, 0x4f854df6c1d721 },
    { 0x08, 0x11c5, 0xc0e3f0f3df33cc },
    { 0x09, 0x0486, 0xbfca7754db5de6 },
    { 0x0a, 0x2f82, 0xa964fc061af87c },
    { 0x0b, 0x236a, 0xb96d68856c45d5 },
    { 0x0c, 0x5beb, 0xd2ca3cbb7d13cc },
    { 0x0d, 0x3db6, 0x58cf827ff3c540 },
    { 0x0e, 0x4b22, 0xbb4037442a869c },
    { 0x0f, 0x59b5, 0x3a83e0ddf37a6e },
};

static device_key_t cprm_device_keys[] =
{
    { 0x00, 0x0809, 0xd50fe4150d32d2 },
    { 0x01, 0x0719, 0x3131c69e825462 },
    { 0x02, 0x0408, 0x7c2e6878b3a494 },
    { 0x03, 0x040a, 0x3c9f93ec5848a2 },
    { 0x04, 0x03bc, 0x614f4bda9876a5 },
    { 0x05, 0x0812, 0x2e901a9227fc47 },
    { 0x06, 0x090d, 0xeebf4957d53d62 },
    { 0x07, 0x0322, 0x6314ec2ca6b32b },
    { 0x08, 0x0035, 0x14f6d08c096483 },
    { 0x09, 0x07c5, 0x8f7eff1d689a81 },
    { 0x0a, 0x069a, 0xff4b538492c611 },
    { 0x0b, 0x0bd8, 0x909300c14c1467 },
    { 0x0c, 0x01d1, 0xba5826ef832e2b },
    { 0x0d, 0x0583, 0xa92e636998767d },
    { 0x0e, 0x02e8, 0x313f0a51478df8 },
    { 0x0f, 0x08fc, 0xd28ce525a2be4b },
};

/* Functions Used by the C2 Cypher */
static inline uint32_t rol32( uint32_t code, int n )
{
    return (code << n) | (code >> (32 - n));
}

static inline uint8_t rol8( uint8_t code, int n )
{
    return (code << n) | (code >> (8 - n));
}

static  inline uint32_t F( uint32_t code, uint32_t key )
{
    uint32_t work;

    work = code + key;
    work ^= sbox_f[work & 0xff];
    work ^= rol32( work, 9 ) ^ rol32( work, 22 );
    return work;
}

void c2_init()
{
    int i;
    unsigned c0, c1, c2, c3;

    for ( i = 0; i < 256; i++ )
    {
        c0 = sbox[i];
        c1 = rol8( (c0 ^ 0x65), 1 );
        c2 = rol8( (c0 ^ 0x2b), 5 );
        c3 = rol8( (c0 ^ 0xc9), 2 );
        c0 ^= i;
        sbox_f[i] = (c3 << 24) + (c2 << 16) + (c1 << 8) + c0;
    }
}

uint64_t c2_enc( uint64_t code, uint64_t key )
{
    uint32_t L, R, t;
    uint32_t ktmpa, ktmpb, ktmpc, ktmpd;
    uint32_t sk[10];
    int      round;

    L     = (uint32_t) ((code >> 32) & 0xffffffff);
    R     = (uint32_t) ((code      ) & 0xffffffff);
    ktmpa = (uint32_t) ((key  >> 32) & 0x00ffffff);
    ktmpb = (uint32_t) ((key       ) & 0xffffffff);

    for (round = 0; round < 10; round++)
    {
        ktmpa &= 0x00ffffff;
        sk[round] = ktmpb + ((uint32_t) sbox[(ktmpa & 0xff) ^ round] << 4);
        ktmpc = (ktmpb >> (32 - 17));
        ktmpd = (ktmpa >> (24 - 17));
        ktmpa = (ktmpa << 17) | ktmpc;
        ktmpb = (ktmpb << 17) | ktmpd;
    }

    for ( round = 0; round < 10; round++ )
    {
        L += F( R, sk[round] );
        t = L; L = R; R = t;
    }
    t = L; L = R; R = t;
    return (((uint64_t) L) << 32) | R;
}

uint64_t c2_dec( uint64_t code, uint64_t key )
{
    uint32_t L, R, t;
    uint32_t ktmpa, ktmpb, ktmpc, ktmpd;
    uint32_t sk[10];
    int      round;

    L  =    (uint32_t) ((code >> 32) & 0xffffffff);
    R  =    (uint32_t) ((code      ) & 0xffffffff);
    ktmpa = (uint32_t) ((key  >> 32) & 0x00ffffff);
    ktmpb = (uint32_t) ((key       ) & 0xffffffff);

    for ( round = 0; round < 10; round++ )
    {
        ktmpa &= 0x00ffffff;
        sk[round] = ktmpb + ((uint32_t) sbox[(ktmpa & 0xff) ^ round] << 4);
        ktmpc = (ktmpb >> (32 - 17));
        ktmpd = (ktmpa >> (24 - 17));
        ktmpa = (ktmpa << 17) | ktmpc;
        ktmpb = (ktmpb << 17) | ktmpd;
    }

    for ( round = 9; round >= 0; round-- )
    {
        L -= F(R, sk[round]);
        t = L; L = R; R = t;
    }

    t = L; L = R; R = t;
    return (((uint64_t) L) << 32) | R;
}

uint64_t c2_g( uint64_t code, uint64_t key )
{
    return c2_enc( code, key ) ^ code;
}

void c2_ecbc( void *p_buffer, uint64_t key, int length )
{
    uint32_t L, R, t;
    uint32_t ktmpa, ktmpb, ktmpc, ktmpd;
    uint32_t sk[10];
    uint64_t inout, inkey;
    int      round, key_round, i;

    inkey = key;
    key_round = 10;

    for ( i = 0; i < length; i += 8 )
    {
        READ64_BE( inout, p_buffer );
        L  =    (uint32_t) ((inout >> 32) & 0xffffffff);
        R  =    (uint32_t) ((inout      ) & 0xffffffff);
        ktmpa = (uint32_t) ((inkey >> 32) & 0x00ffffff);
        ktmpb = (uint32_t) ((inkey      ) & 0xffffffff);

        for ( round = 0; round < key_round; round++ )
        {
            ktmpa &= 0x00ffffff;
            sk[round] = ktmpb + ((uint32_t) sbox[(ktmpa & 0xff) ^ round] << 4);
            ktmpc = (ktmpb >> (32 - 17));
            ktmpd = (ktmpa >> (24 - 17));
            ktmpa = (ktmpa << 17) | ktmpc;
            ktmpb = (ktmpb << 17) | ktmpd;
        }

        for ( round = 0; round < 10; round++ )
        {
            L += F(R, sk[round % key_round]);

            if ( round == 4 )
            {
                inkey = key ^ (((uint64_t) (R & 0x00ffffff) << 32) | L);
            }
            t = L; L = R; R = t;
        }

        t = L; L = R; R = t;
        inout = (((uint64_t) L) << 32) | R;
        B2N_64( inout );
        memcpy( p_buffer, &inout, sizeof( inout ) );
        p_buffer = (uint8_t *) p_buffer + 8;
        key_round = 2;
    }
}

void c2_dcbc( void *p_buffer, uint64_t key, int length )
{
    uint32_t L, R, t;
    uint32_t ktmpa, ktmpb, ktmpc, ktmpd;
    uint32_t sk[10];
    uint64_t inout, inkey;
    int      round, key_round, i;
    uint8_t *buf = (uint8_t *) p_buffer;

    inkey = key;
    key_round = 10;

    for ( i = 0; i < length; i += 8 )
    {
        READ64_BE( inout, buf );

        L  = (uint32_t) ((inout >> 32) & 0xffffffff);
        R  = (uint32_t) ((inout      ) & 0xffffffff);
        ktmpa = (uint32_t) ((inkey >> 32) & 0x00ffffff);
        ktmpb = (uint32_t) ((inkey      ) & 0xffffffff);

        for ( round = 0; round < key_round; round++ )
        {
            ktmpa &= 0x00ffffff;
            sk[round] = ktmpb + ((uint32_t) sbox[(ktmpa & 0xff) ^ round] << 4);
            ktmpc = (ktmpb >> (32 - 17));
            ktmpd = (ktmpa >> (24 - 17));
            ktmpa = (ktmpa << 17) | ktmpc;
            ktmpb = (ktmpb << 17) | ktmpd;
        }

        for ( round = 9; round >= 0; round-- )
        {
            L -= F(R, sk[round % key_round]);
            t = L; L = R; R = t;

            if ( round == 5 )
            {
                inkey = key ^ (((uint64_t) (R & 0x00ffffff) << 32) | L);
            }
        }

        t = L; L = R; R = t;
        inout = (((uint64_t) L) << 32) | R;
        B2N_64( inout );
        memcpy( buf, &inout, sizeof( uint64_t ) );

        buf += 8;
        key_round = 2;
    }
}

/* for CPPM, libdvdread is responsible for retrieving the Media Key Block */
uint8_t *cprm_get_mkb( dvdcss_t dvdcss )
{
    uint8_t mkb_pack[CPRM_MKB_PACK_SIZE];
    uint8_t *p_mkb = NULL;
    int mkb_packs, i;
    mkb_packs = 16;

    if ( ioctl_ReadCPRMMKBPack( dvdcss->i_fd, &dvdcss->css.i_agid, 0,
                                (uint8_t *) mkb_pack, &mkb_packs ) )
        return NULL;

    p_mkb = malloc( mkb_packs * CPRM_MKB_PACK_SIZE - 16 );

    if (!p_mkb)
        return NULL;

    memcpy( p_mkb, &mkb_pack[16], CPRM_MKB_PACK_SIZE - 16 );

    for ( i = 1; i < mkb_packs; i++ )
    {
        if ( ioctl_ReadCPRMMKBPack( dvdcss->i_fd,&dvdcss->css.i_agid, i,
                    (uint8_t *) p_mkb + i * CPRM_MKB_PACK_SIZE - 16, &mkb_packs ) )
        {
            free( p_mkb );
            p_mkb = NULL;
            break;
        }
    }

    return p_mkb;
}

#define f( c, r ) (((uint64_t) c << 32) | (uint64_t) r)

/* This function retrieves the main key used to decryption; this key is derived
 * from applying the C2 cypher to the MKB and the DVD-Audio player device keys,
 * as well as a unique album_id and media_id */
int process_mkb( uint8_t *p_mkb, device_key_t *p_dev_keys, int nr_dev_keys, uint64_t *p_media_key )
{
    int mkb_pos, length, i, i_dev_key, no_more_keys, no_more_records;
    uint8_t record_type, column;
    uint64_t buffer, media_key, verification_data;

    /* Init everything */
    i_dev_key = no_more_keys = 0;
    buffer = media_key = verification_data = 0;

    while ( !no_more_keys )
    {
        /* skip the file identifier and the length */
        mkb_pos = 16;
        no_more_records = 0;
        while (!no_more_records)
        {
            record_type = *(uint8_t *) &p_mkb[mkb_pos];
            memcpy( &length, &p_mkb[mkb_pos], sizeof( length ) );
            length &= 0xffffff00;
            B2N_32( length );

            if (length >= 12)
            {
                memcpy( &buffer, &p_mkb[mkb_pos + 4], sizeof( buffer ) );
            }
            else
            {
                if (length < 4)
                    length = 4;
            }

            switch ( record_type )
            {
                case 0x82: /* Conditionally calculate media key record */
                    B2N_64( buffer );
                    buffer = c2_dec( buffer, media_key );

                    if ( (buffer & 0xffffffff00000000) != 0xdeadbeef00000000 )
                        break;

                    B2N_64( buffer );
                    /* intentional fallthrough */
                case 0x01: /* Calculate media key record */
                    column = ((uint8_t *) &buffer)[4];
                    /*
                    if (column >= 16 || ((uint8_t*)&buffer)[5] != 0 || ((uint8_t*)&buffer)[6] != 0 || ((uint8_t*)&buffer)[7] != 1)
                        break;
                    */
                    /* Get appropriate device key for column */
                    no_more_keys = 1;
                    for ( i = i_dev_key; i < nr_dev_keys; i++ )
                    {
                        if ( p_dev_keys[i].col == column )
                        {
                            no_more_keys = 0;
                            i_dev_key = i;
                            break;
                        }
                    }
                    if ( no_more_keys )
                        break;
                    if ( 12 + p_dev_keys[i_dev_key].row * 8 + 8 > length )
                        break;
                    memcpy( &buffer, &p_mkb[mkb_pos + 12 + p_dev_keys[i_dev_key].row * 8], sizeof( buffer ) );
                    B2N_64( buffer );

                    if ( record_type == 0x82 )
                        buffer = c2_dec(buffer, media_key);

                    media_key = (c2_dec( buffer, p_dev_keys[i_dev_key].key ) & 0x00ffffffffffffff) ^ f( column, p_dev_keys[i_dev_key].row );
                    buffer = c2_dec( verification_data, media_key );

                    if ( (buffer & 0xffffffff00000000) == 0xdeadbeef00000000 )
                    {
                        *p_media_key = media_key;
                        return 0;
                    }

                    break;
                case 0x02: /* End of media key record */
                    no_more_records = 1;
                    break;
                case 0x81: /* Verify media key record */
                    B2N_64( buffer );
                    verification_data = buffer;
                    break;
                default:
                    break;
            }
            mkb_pos += length;
        }
        i_dev_key++;
    }
    return -1;
}

int vr_get_k_te( cpxm cpxm,char *psz_vr_mangr )
{
    FILE    *f_vr_m;
    uint8_t vmgi_mat[512];
    int ret = -1;

    f_vr_m = fopen( psz_vr_mangr, "rb" );

    if ( !f_vr_m )
        return -1;

    if ( fread(vmgi_mat, 1, 512, f_vr_m) == 512 )
    {
        if ( memcmp( vmgi_mat, "DVD_RTR_VMG0", 12 ) == 0 )
        {
            cpxm.vr_k_te = 0;
            if ( vmgi_mat[267] & 1 ) /* Check encrypted title key status bit */
            {
                memcpy( &cpxm.vr_k_te, &vmgi_mat[268], sizeof( cpxm.vr_k_te ) );
                B2N_64( cpxm.vr_k_te );
            }
            ret = 0;
        }
    }

    fclose( f_vr_m );
    return ret;
}

/* Function should be called on a dvdcss var to set cppm struct which needs
 * to persist in order to decrypt the media */
LIBDVDCSS_EXPORT int dvdcpxm_init( dvdcss_t dvdcss, uint8_t *p_mkb )
{
    /* In the case that p_mkb is received as null, then either you were unable
     * to read the mkb or the encryption type is cprm */
    char psz_file[PATH_MAX];
    int ret = -1;

    c2_init();
    switch ( dvdcss->media_type )
    {
        case COPYRIGHT_PROTECTION_NONE:
            ret = 0;
            break;
        case COPYRIGHT_PROTECTION_CPPM:
            if ( cppm_set_id_album( dvdcss ) == 0 )
            {
                if ( p_mkb )
                {
                    ret = process_mkb( p_mkb, cppm_device_keys,
                            sizeof(cppm_device_keys) / sizeof(*cppm_device_keys),
                            &dvdcss->cpxm.media_key );
                    free(p_mkb);
                    if (ret) break;
                }
            }
            break;
        case COPYRIGHT_PROTECTION_CPRM:
            if ( cprm_set_id_media( dvdcss ) == 0 )
            {
                p_mkb = cprm_get_mkb( dvdcss );
                if ( p_mkb )
                {
                    ret = process_mkb( p_mkb, cprm_device_keys,
                            sizeof( cprm_device_keys ) / sizeof( *cprm_device_keys ),
                            &dvdcss->cpxm.media_key );
                    free( p_mkb );
                    if (ret) break;
                }
                snprintf( psz_file, PATH_MAX, "%s/DVD_RTAV/VR_MANGR.IFO", dvdcss->psz_device );
                ret = vr_get_k_te( dvdcss->cpxm,psz_file );

                if (ret)
                {
                    snprintf( psz_file, PATH_MAX, "%s/DVD_RTAV/VR_MANGR.BUP", dvdcss->psz_device );
                    ret = vr_get_k_te( dvdcss->cpxm,psz_file );
                    if (ret) break;
                }
            }
            break;
    }
    return dvdcss->media_type;
}

/* Ensures that the block is encrypted */
int mpeg2_check_pes_scrambling_control( uint8_t *p_block )
{
    int pes_scrambling_control;

    pes_scrambling_control = 0;
    if ( IS_SYNC_CODE(p_block) )
    {
        pes_scrambling_control = (p_block[20] & 0x30) >> 4;
    }
    return pes_scrambling_control;
}

void mpeg2_reset_pes_scrambling_control( uint8_t *p_block )
{
    if ( IS_SYNC_CODE(p_block) )
    {
        p_block[20] &= 0xCD; // reset pes_scrambling_control and copyright flags;
    }
}

void mpeg2_reset_cci( uint8_t *p_block )
{
    uint8_t *p_mlp_pcm, *p_curr;
    int pes_sid;
    int pes_len;

    p_curr = p_block;
    if ( IS_SYNC_CODE(p_block) )
    {
        p_curr += 14 + (p_curr[13] & 0x07);

        while ( p_curr < p_block + DVDCPXM_BLOCK_SIZE )
        {
            pes_len = (p_curr[4] << 8) + p_curr[5];

            if (p_curr[0] == 0x00 &&
                p_curr[1] == 0x00 &&
                p_curr[2] == 0x01)
            {
                pes_sid = p_curr[3];
                if ( pes_sid == 0xbd ) // private stream 1
                {
                    p_mlp_pcm = p_curr + 9 + p_curr[8];
                    switch ( p_mlp_pcm[0] )  // stream id
                    {
                    case 0xa0: // PCM stream id
                        if ( p_mlp_pcm[3] > 8 ) p_mlp_pcm[12] = CCI_BYTE; // reset CCI
                        break;
                    case 0xa1: // MLP stream di
                        if ( p_mlp_pcm[3] > 4 ) p_mlp_pcm[8] = CCI_BYTE; // reset CCI
                        break;
                    }
                }
                p_curr += 6 + pes_len;
            }
            else break;
        }
    }
}

/* Inside this header are fields relating to the type of packet and DVD-Audio specifications. */
/* There are also a set of keys at different addresses that are used by CPPM to decrypt the block */
/* only the last 1920 bytes contain protected content, the first 180 bytes are left untouched. */
int cppm_decrypt_block( uint8_t *p_buffer, int flags, uint64_t id_album, uint64_t media_key )
{
    uint64_t d_kc_i, k_au, k_i, k_c;
    int encrypted;

    encrypted = 0;
    if ( mpeg2_check_pes_scrambling_control( p_buffer ) )
    {
        k_au = c2_g( id_album, media_key ) & 0x00ffffffffffffff;

        READ64_BE( d_kc_i, &p_buffer[24] );
        k_i = c2_g( d_kc_i, k_au ) & 0x00ffffffffffffff;

        READ64_BE( d_kc_i, &p_buffer[32] );
        k_i = c2_g( d_kc_i, k_i ) & 0x00ffffffffffffff;

        READ64_BE( d_kc_i, &p_buffer[40] );
        k_i = c2_g( d_kc_i, k_i ) & 0x00ffffffffffffff;

        READ64_BE( d_kc_i, &p_buffer[48] );
        k_i = c2_g( d_kc_i, k_i ) & 0x00ffffffffffffff;

        READ64_BE( d_kc_i, &p_buffer[84] );
        k_c = c2_g( d_kc_i, k_i ) & 0x00ffffffffffffff;

        c2_dcbc( &p_buffer[DVDCPXM_BLOCK_SIZE - DVDCPXM_ENCRYPTED_SIZE], k_c, DVDCPXM_ENCRYPTED_SIZE );
        mpeg2_reset_pes_scrambling_control( p_buffer );
        encrypted = 1;
    }

    if ( ( flags & DVDCPXM_PRESERVE_CCI ) != DVDCPXM_PRESERVE_CCI )
        mpeg2_reset_cci( p_buffer );

    return encrypted;
}

int cprm_decrypt_block( uint8_t *p_buffer, int flags, uint64_t id_album, uint64_t media_key )
{
    uint64_t d_kc_i, k_au, k_i, k_c;
    int encrypted;

    encrypted = 0;
    if ( mpeg2_check_pes_scrambling_control( p_buffer ) )
    {
        k_au = c2_g( id_album, media_key ) & 0x00ffffffffffffff;

        READ64_BE( d_kc_i, &p_buffer[24] );
        k_i = c2_g( d_kc_i, k_au ) & 0x00ffffffffffffff;

        READ64_BE( d_kc_i, &p_buffer[32] );
        k_i = c2_g( d_kc_i, k_i ) & 0x00ffffffffffffff;

        READ64_BE( d_kc_i, &p_buffer[40] );
        k_i = c2_g( d_kc_i, k_i ) & 0x00ffffffffffffff;

        READ64_BE( d_kc_i, &p_buffer[48] );
        k_i = c2_g( d_kc_i, k_i ) & 0x00ffffffffffffff;

        READ64_BE( d_kc_i, &p_buffer[84] );
        k_c = c2_g( d_kc_i, k_i ) & 0x00ffffffffffffff;

        c2_dcbc( &p_buffer[DVDCPXM_BLOCK_SIZE - DVDCPXM_ENCRYPTED_SIZE], k_c, DVDCPXM_ENCRYPTED_SIZE );
        mpeg2_reset_pes_scrambling_control( p_buffer );
        encrypted = 1;
    }

    if ( ( flags & DVDCPXM_PRESERVE_CCI ) != DVDCPXM_PRESERVE_CCI )
        mpeg2_reset_cci( p_buffer );

    return encrypted;
}

int dvdcpxm_decrypt( cpxm cpxm, int media_type,void *p_buffer, int flags )
{
    switch ( media_type )
    {
        case COPYRIGHT_PROTECTION_CPPM:
            return cppm_decrypt_block( (uint8_t *) p_buffer, flags, cpxm.id_album, cpxm.media_key );
        case COPYRIGHT_PROTECTION_CPRM:
            return cprm_decrypt_block( (uint8_t* ) p_buffer, flags, cpxm.id_album ,cpxm.media_key );
    }

    return 0;
}

/* CPXM exported prototype definitions */
/* these methods should behave similarily but use dvdcpxm_decrypt instead of unscramble, and remove any unnecessary code */
int dvdcpxm_close ( dvdcss_t dvdcss )
{
    int i_ret;
    i_ret = dvdcss_close_device( dvdcss );

    free( dvdcss->psz_device );
    free( dvdcss );

    return i_ret;
}

int dvdcpxm_read ( dvdcss_t dvdcss, void *p_buffer,
                                          int i_blocks,
                                          int i_flags ){
    uint8_t *_p_buffer = p_buffer;
    int i_ret, i_index;

    i_ret = dvdcss->pf_read( dvdcss, _p_buffer, i_blocks );

    if( i_ret <= 0
         || !(i_flags & DVDCSS_READ_DECRYPT) )
    {
        return i_ret;
    }

    /* Decrypt the blocks we managed to read */
    for( i_index = i_ret; i_index; i_index-- )
    {
        dvdcpxm_decrypt( dvdcss->cpxm, dvdcss->media_type, _p_buffer, DVDCPXM_RESET_CCI );
        _p_buffer = _p_buffer + DVDCSS_BLOCK_SIZE;
    }

    return i_ret;
}

int dvdcpxm_seek ( dvdcss_t dvdcss, int i_blocks, int i_flags )
{
    (void)i_flags;
    return dvdcss->pf_seek( dvdcss, i_blocks );
}

int dvdcpxm_readv ( dvdcss_t dvdcss, void *p_iovec,
                                           int i_blocks,
                                           int i_flags )
{
    struct iovec *_p_iovec = p_iovec;
    int i_ret, i_index;
    void *iov_base;
    size_t iov_len;

    i_ret = dvdcss->pf_readv( dvdcss, _p_iovec, i_blocks );

    if( i_ret <= 0
         || !(i_flags & DVDCSS_READ_DECRYPT) )
    {
        return i_ret;
    }

    /* Initialize loop for decryption */
    iov_base = _p_iovec->iov_base;
    iov_len = _p_iovec->iov_len;

    /* Decrypt the blocks we managed to read */
    for( i_index = i_ret; i_index; i_index-- )
    {
        /* Check that iov_len is a multiple of 2048 */
        if( iov_len & 0x7ff )
        {
            return -1;
        }

        while( iov_len == 0 )
        {
            _p_iovec++;
            iov_base = _p_iovec->iov_base;
            iov_len = _p_iovec->iov_len;
        }
        /* reseting CCI handled by decrypt */
        dvdcpxm_decrypt( dvdcss->cpxm, dvdcss->media_type, iov_base, DVDCPXM_RESET_CCI );

        iov_base = (uint8_t *) iov_base + DVDCSS_BLOCK_SIZE;
        iov_len -= DVDCSS_BLOCK_SIZE;
    }

    return i_ret;
}
