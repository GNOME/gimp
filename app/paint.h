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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __PAINT_H__
#define __PAINT_H__

#include "tag.h"

/* Forward declarations */
struct _GimpDrawable;


typedef struct _Paint Paint;

Paint *                paint_new            (Tag, struct _GimpDrawable *);
void                   paint_delete         (Paint *);
Paint *                paint_clone          (Paint *);
void                   paint_info           (Paint *);

Tag                    paint_tag            (Paint *);
Precision              paint_precision      (Paint *);
Format                 paint_format         (Paint *);
Alpha                  paint_alpha          (Paint *);

/* WRITEME */
Precision              paint_set_precision  (Paint *, Precision);
/* WRITEME */
Format                 paint_set_format     (Paint *, Format);
/* WRITEME */
Alpha                  paint_set_alpha      (Paint *, Alpha);

/* WRITEME */
void *                 paint_component      (Paint *, guint);
/* WRITEME */
guint                  paint_set_component  (Paint *, guint, void *);
int                    paint_load           (Paint *, Tag, void *);

struct _GimpDrawable * paint_drawable       (Paint *);
struct _GimpDrawable * paint_set_drawable   (Paint  *, struct _GimpDrawable *);

void *                 paint_data           (Paint *);
guint                  paint_bytes          (Paint *);

void                   paint_setup          (void);
void                   paint_free           (void);



/* ===============================================================

  stuff that is here for the moment for want of a better place

*/

/* Opacities */
#define TRANSPARENT        0
#define OPAQUE             255

#define MAX_CHANNELS     4

/*  Color conversion routines  */
void  rgb_to_hsv (int *, int *, int *);
void  hsv_to_rgb (int *, int *, int *);
void  rgb_to_hls (int *, int *, int *);
void  hls_to_rgb (int *, int *, int *);

/*  variable source to RGB color mapping */
void map_to_color (Format, unsigned char *, unsigned char *, unsigned char *);

/*  RGB to index mapping functions */
int  map_rgb_to_indexed (unsigned char *, int, int, int, int, int);

#endif /* __PAINT_H__ */
