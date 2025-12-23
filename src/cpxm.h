/**
 * \file cpxm.h
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
#ifndef CPXM_H
#define CPXM_H

#include "dvdcss/dvdcss.h"
#include "bswap.h"
#include <string.h>

#define READ64_BE(dest, src) \
    do { \
        uint64_t __tmp; \
        memcpy(&__tmp, (src), sizeof(uint64_t)); \
        B2N_64(__tmp); \
        (dest) = __tmp; \
    } while(0)

typedef struct cpxm
{
   uint64_t media_key;
   uint64_t id_album;
   uint64_t id_media;
   uint64_t vr_k_te;
} cpxm;

/* cpxm uses the same css authentification method when using a usb dvd drive */
int cppm_set_id_album( dvdcss_t dvdcss );
int cprm_set_id_media( dvdcss_t dvdcss );

#endif // CPXM_H
