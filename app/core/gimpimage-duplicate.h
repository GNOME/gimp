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
#ifndef __CHANNEL_OPS_H__
#define __CHANNEL_OPS_H__

#include "gimpimageF.h"
#include "gimpdrawableF.h"

typedef enum
{
  OFFSET_BACKGROUND,
  OFFSET_TRANSPARENT
} ChannelOffsetType;

/*  channel_ops functions  */
void  channel_ops_offset    (GimpImage *gimage);
void  channel_ops_duplicate (GimpImage *gimage);

void        offset    (GimpImage    *gimage,
		       GimpDrawable *drawable,
		       gboolean      wrap_around,
		       gint          fill_type,
		       gint          offset_x,
		       gint          offset_y);

GimpImage * duplicate (GimpImage    *gimage);

#endif  /*  __CHANNEL_OPS_H__  */
