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
#ifndef  __BUCKET_FILL_H__
#define  __BUCKET_FILL_H__

#include "gimpimageF.h"
#include "gimpdrawableF.h"
#include "tools.h"

typedef enum
{
  FG_BUCKET_FILL,
  BG_BUCKET_FILL,
  PATTERN_BUCKET_FILL
} BucketFillMode;

void        bucket_fill        (GimpImage      *gimage,
				GimpDrawable   *drawable,
				BucketFillMode  fill_mode,
				gint            paint_mode,
				gdouble         opacity,
				gdouble         threshold,
				gboolean        sample_merged,
				gdouble         x,
				gdouble         y);

void        bucket_fill_region (BucketFillMode  fill_mode,
				PixelRegion    *bufPR,
				PixelRegion    *maskPR,
				guchar         *col,
				TempBuf        *pattern,
				gint            off_x,
				gint            off_y,
				gboolean        has_alpha);

Tool *      tools_new_bucket_fill   (void);
void        tools_free_bucket_fill  (Tool *tool);

#endif  /*  __BUCKET_FILL_H__  */
