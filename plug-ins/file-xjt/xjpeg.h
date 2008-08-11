/*  xjpeg.h
 *
 * This Module contains:
 *   jpeg load and save procedures for the use in XJT fileformat save
 *   (based on libjpeg)
 *
 */
/* GIMP - The GNU Image Manipulation Program
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
 * version 1.00.00; 1998/10/26  hof: 1.st (pre) release
 */

#ifndef _XJPEG_H
#define _XJPEG_H

#include "libgimp/gimp.h"

typedef enum {
JSVM_DRAWABLE,     /* save the drawable as it is (ignore alpha channel) */
JSVM_ALPHA         /* save the alpha channel */

} t_save_mode;

typedef struct
{
  gdouble quality;
  gdouble smoothing;
  gint optimize;
  gint clr_transparent; /* clear pixels where alpha is full transparent */
} t_JpegSaveVals;


gint
xjpg_save_drawable (const char *filename,
                    gint32      image_ID,
                    gint32      drawable_ID,
                    gint        save_mode,
                    t_JpegSaveVals *jsvals);

gint32
xjpg_load_layer (const char   *filename,
                 gint32        image_id,
                 int           image_type,
                 char         *layer_name,
                 gdouble       layer_opacity,
                 GimpLayerModeEffects     layer_mode);

gint
xjpg_load_layer_alpha (const char *filename,
                       gint32      image_id,
                       gint32      layer_id);

gint32
xjpg_load_channel (const char   *filename,
                   gint32        image_id,
                   gint32        drawable_id,
                   char         *channel_name,
                   gdouble       channel_opacity,
                   guchar red, guchar  green, guchar blue);

#endif
