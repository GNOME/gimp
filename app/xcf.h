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
#ifndef __XCF_H__
#define __XCF_H__


#include <stdio.h>


typedef struct _XcfInfo  XcfInfo;

struct _XcfInfo
{
  FILE         *fp;
  guint         cp;
  gchar        *filename;
  Layer        *active_layer;
  Channel      *active_channel;
  GimpDrawable *floating_sel_drawable;
  Layer        *floating_sel;
  guint         floating_sel_offset;
  gint          swap_num;
  gint         *ref_count;
  gint          compression;
  gint          file_version;
};


void xcf_init (void);


#endif /* __XCF_H__ */
