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

#ifndef __TRANSFORM_OPTIONS_H__
#define __TRANSFORM_OPTIONS_H_

#include "tool_options.h"

typedef struct _TransformOptions TransformOptions;

struct _TransformOptions
{
  ToolOptions  tool_options;

  gboolean     smoothing;
  gboolean     smoothing_d;
  GtkWidget   *smoothing_w;

  gint	       direction;
  gint         direction_d;
  GtkWidget   *direction_w[2];  /* 2 radio buttons */

  gboolean     show_grid;
  gboolean     show_grid_d;
  GtkWidget   *show_grid_w;

  gint	       grid_size;
  gint         grid_size_d;
  GtkObject   *grid_size_w;

  gboolean     clip;
  gboolean     clip_d;
  GtkWidget   *clip_w;

  gboolean     showpath;
  gboolean     showpath_d;
  GtkWidget   *showpath_w;
};


TransformOptions * transform_options_new   (GtkType               tool_type,
					    ToolOptionsResetFunc  reset_func);

void               transform_options_init  (TransformOptions     *options,
					    GtkType               tool_type,
					    ToolOptionsResetFunc  reset_func);
void               transform_options_reset (ToolOptions          *tool_options);

gboolean           gimp_transform_tool_smoothing (void);
gboolean           gimp_transform_tool_showpath  (void);
gboolean           gimp_transform_tool_clip      (void);
gint               gimp_transform_tool_direction (void);
gint               gimp_transform_tool_grid_size (void);
gboolean           gimp_transform_tool_show_grid (void);


#endif /* __TRANSFORM_OPTIONS_H__ */
