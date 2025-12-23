/**
 * \file cpxm.c
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

#include "cpxm.h"
#include "libdvdcpxm.h"
#include "ioctl.h"
#include "css.h"
#include "libdvdcss.h"
#include <stddef.h>

/******************************************************************************
 * From dvdcss_disckey, Grab the album ID for cppm
 *****************************************************************************/
int cppm_set_id_album( dvdcss_t dvdcss )
{
    unsigned char p_buffer[ DVD_DISCKEY_SIZE ];

    if( GetBusKey( dvdcss ) < 0 )
    {
        return -1;
    }

    /* Get encrypted disc key */
    if( ioctl_ReadDiscKey( dvdcss->i_fd, &dvdcss->css.i_agid, p_buffer ) < 0 )
    {
        print_error( dvdcss, "ioctl ReadDiscKey failed" );
        return -1;
    }

    /* This should have invalidated the AGID and got us ASF=1. */
    if( GetASF( dvdcss ) != 1 )
    {
        /* Region mismatch (or region not set) is the most likely source. */
        print_error( dvdcss, "authentication success flag (ASF) not 1 after "
                             "reading disc key (region mismatch?)" );
        ioctl_InvalidateAgid( dvdcss->i_fd, &dvdcss->css.i_agid );
        return -1;
    }

    /* Shuffle disc key using bus key */
    for( int i = 0 ; i < DVD_DISCKEY_SIZE ; i++ )
    {
        p_buffer[i] ^= dvdcss->css.p_bus_key[4 - (i % DVD_KEY_SIZE)];
    }

    READ64_BE( dvdcss->cpxm.id_album, &p_buffer[80] );
    return 0;
}

/******************************************************************************
 * From dvdcss_disckey, Grab the album ID for CPRM
 *****************************************************************************/
int cprm_set_id_media( dvdcss_t dvdcss )
{
    uint8_t p_buffer[CPRM_MEDIA_ID_SIZE + 4];

    if( GetBusKey( dvdcss ) < 0 )
    {
        return -1;
    }

    if( ioctl_ReadCPRMMediaId( dvdcss->i_fd, &dvdcss->css.i_agid, p_buffer ) ) 
        return -1;

    READ64_BE( dvdcss->cpxm.id_media,
                   p_buffer + offsetof(cprm_media_id_t, id_media)
                   );

    return 0;
}


