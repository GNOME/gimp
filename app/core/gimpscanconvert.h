/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef SCAN_CONVERT_H
#define SCAN_CONVERT_H

#include <glib.h>
#include "channel.h"

typedef struct {
    gdouble	x;
    gdouble	y;
} ScanConvertPoint;


typedef struct ScanConverterPrivate ScanConverter;


/* Create a new scan conversion context.  Set "antialias" to 1 for no
 * supersampling, or the amount to supersample by otherwise.  */
ScanConverter * scan_converter_new (guint width, guint height,
				    guint antialias);


/* Add "npoints" from "pointlist" to the polygon currently being
 * described by "scan_converter".  */
void scan_converter_add_points (ScanConverter *scan_converter,
				guint npoints,
				ScanConvertPoint *pointlist);


/* Scan convert the polygon described by the list of points passed to
 * scan_convert_add_points, and return a channel with a bits set if
 * they fall within the polygon defined.  The polygon is filled
 * according to the even-odd rule.  The polygon is closed by
 * joining the final point to the initial point. */
Channel * scan_converter_to_channel (ScanConverter *scan_converter,
				     GimpImage *gimage);


void scan_converter_free (ScanConverter *scan_converter);


#endif /* SCAN_CONVERT_H */
