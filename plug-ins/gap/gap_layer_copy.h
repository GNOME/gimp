/* my_layer_copy.h
 * 1997.11.06 hof (Wolfgang Hofer)
 *
 *
 *  this procedure works similar to gimp_layer_copy(src_id);
 *  ==> gimp_layer_copy works only for layers within the same image !!
 *  ==> Workaround:
 *      p_my_layer_copy is my 'private' version of layercopy
 *      that can copy layers from another image of the same type.
 *
 * returns the id of the new layer
 *      and the offests of the original within the source image
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
 * version 0.98.00 1998.11.26   hof: added channel copy
 * version 0.90.00;             hof: 1.st (pre) release
 */

#ifndef _GAP_LAYER_COPY_H
#define _GAP_LAYER_COPY_H

/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

gint32 p_my_layer_copy (gint32 dst_image_id,
                        gint32 src_layer_id,
                        gdouble    opacity, /* 0.0 upto 100.0 */
                        GimpLayerModeEffects mode,
                        gint *src_offset_x,
                        gint *src_offset_y );

gint32 p_my_channel_copy (gint32 dst_image_id,
                        gint32 src_channel_id);

#endif
