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
#ifndef __CHANNEL_PVT_H__
#define __CHANNEL_PVT_H__

#include "drawable.h"

#include "boundary.h"
#include "temp_buf.h"
#include "tile_manager.h"

#include "drawable_pvt.h"

struct _GimpChannel
{
  GimpDrawable drawable;

  guchar     col[3];            /*  RGB triplet for channel color  */
  gint       opacity;           /*  Channel opacity                */
  gboolean   show_masked;       /*  Show masked areas--as          */
                                /*  opposed to selected areas      */

  /*  Selection mask variables  */
  gboolean   boundary_known;    /*  is the current boundary valid  */
  BoundSeg  *segs_in;           /*  outline of selected region     */
  BoundSeg  *segs_out;          /*  outline of selected region     */
  gint       num_segs_in;       /*  number of lines in boundary    */
  gint       num_segs_out;      /*  number of lines in boundary    */
  gboolean   empty;             /*  is the region empty?           */
  gboolean   bounds_known;      /*  recalculate the bounds?        */
  gint       x1, y1;            /*  coordinates for bounding box   */
  gint       x2, y2;            /*  lower right hand coordinate    */
};

struct _GimpChannelClass
{
  GimpDrawableClass parent_class;
};

#endif /* __CHANNEL_PVT_H__ */
