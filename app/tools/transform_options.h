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
  GimpToolOptions         tool_options;

  GimpTransformDirection  direction;
  GimpTransformDirection  direction_d;
  GtkWidget              *direction_w[2];  /* 2 radio buttons */

  gboolean                smoothing;
  gboolean                smoothing_d;
  GtkWidget              *smoothing_w;

  gboolean                clip;
  gboolean                clip_d;
  GtkWidget              *clip_w;

  gboolean                show_grid;
  gboolean                show_grid_d;
  GtkWidget              *show_grid_w;

  gint	                  grid_size;
  gint                    grid_size_d;
  GtkObject              *grid_size_w;

  gboolean                show_path;
  gboolean                show_path_d;
  GtkWidget              *show_path_w;

  gboolean                constrain_1;
  gboolean                constrain_1_d;
  GtkWidget              *constrain_1_w;

  gboolean                constrain_2;
  gboolean                constrain_2_d;
  GtkWidget              *constrain_2_w;
};


GimpToolOptions * transform_options_new   (GimpToolInfo     *tool_info);

void              transform_options_init  (TransformOptions *options,
                                           GimpToolInfo     *tool_info);
void              transform_options_reset (GimpToolOptions  *tool_options);


#endif /* __TRANSFORM_OPTIONS_H__ */
