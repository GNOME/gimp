/* gap_decode_xanim.h
 * 1999.11.22 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *         Call xanim exporting edition (the loki version)
 *         via shellscript gap_xanim_export.sh
 *         To split any xanim supported video into
 *         anim frames (single images on disk)
 *         Audio Tracks can also be extracted.
 *
 */
/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* revision history:
 * 1.1.11b; 1999/11/22   hof: first release
 */

#ifndef _GAP_XANIM_H
#define _GAP_XANIM_H

#include "libgimp/gimp.h"

/* fileformats supported by gap_decode_xanim */
typedef enum
{ 
   XAENC_XCF     /* no direct support by xanim, have to convert */
 , XAENC_PPMRAW
 , XAENC_JPEG
} t_gap_xa_formats;


int gap_xanim_decode(GimpRunModeType run_mode
                             );

#endif


