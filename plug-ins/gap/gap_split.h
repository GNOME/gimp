/* gap_lib.h
 * 1997.11.01 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * Split Image to Anim Frames (seperate Images on disk)
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
 * 0.96.00; 1998/07/03   hof: 1.st releas
 */

#ifndef _GAP_SPLIT_H
#define _GAP_SPLIT_H

#include "libgimp/gimp.h"



int gap_split_image(GimpRunModeType run_mode,
                             gint32     image_id,
                             gint32     inverse_order,
                             gint32     no_alpha,
                             char      *extension);


#endif


