/* gap_mpege.h
 * 1998.07.04 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
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
 * 0.96.00; 1998/07/02   hof: first release
 */

#ifndef _GAP_MPEGE_H
#define _GAP_MPEGE_H

#include "libgimp/gimp.h"

/* Animation sizechange modes */
typedef enum
{
   MPEG_ENCODE  
 , MPEG2ENCODE 
} t_gap_mpeg_encoder;


int gap_mpeg_encode(GimpRunModeType run_mode,
                             gint32 image_id,
                             t_gap_mpeg_encoder encoder
                          /* ,
                             char   *output,
                             gint    bitrate,
                             gdouble framerate
                           */
                             );

#endif


