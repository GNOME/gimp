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
#ifndef __SELECTION_OPTIONS_H__
#define __SELECTION_OPTIONS_H__

#include "tools.h"
#include "tool_options.h"

#include "libgimp/gimpunit.h"

/*  the selection options structures  */

typedef struct _SelectionOptions SelectionOptions;
struct _SelectionOptions
{
  ToolOptions  tool_options;

  /*  options used by all selection tools  */
  gboolean     feather;
  gboolean     feather_d;
  GtkWidget   *feather_w;

  gdouble      feather_radius;
  gdouble      feather_radius_d;
  GtkObject   *feather_radius_w;

  /*  used by all selection tools except rect. select  */
  gboolean     antialias;
  gboolean     antialias_d;
  GtkWidget   *antialias_w;

  /*  used by fuzzy, by-color selection  */
  gboolean     sample_merged;
  gboolean     sample_merged_d;
  GtkWidget   *sample_merged_w;

  /*  used by rect., ellipse selection  */
  gboolean     fixed_size;
  gboolean     fixed_size_d;
  GtkWidget   *fixed_size_w;

  gdouble      fixed_width;
  gdouble      fixed_width_d;
  GtkObject   *fixed_width_w;

  gdouble      fixed_height;
  gdouble      fixed_height_d;
  GtkObject   *fixed_height_w;

  GUnit        fixed_unit;
  GUnit        fixed_unit_d;
  GtkWidget   *fixed_unit_w;

  /*  used by bezier selection  */
  gint         extend;
};

/*  selection tool options functions
 */
SelectionOptions * selection_options_new   (ToolType              tool_type,
					    ToolOptionsResetFunc  reset_func);

void               selection_options_reset (SelectionOptions     *options);

/*  to be used by "derived selection options only
 */
void               selection_options_init  (SelectionOptions     *options,
					    ToolType              tool_type,
					    ToolOptionsResetFunc  reset_func);

#endif  /*  __SELCTION_OPTIONS_H__  */
