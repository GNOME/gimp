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


#include "tool_options.h"


/*  the selection options structures  */

struct _SelectionOptions
{
  GimpToolOptions  tool_options;

  /*  options used by all selection tools  */
  SelectOps           op;
  SelectOps           op_d;
  GtkWidget          *op_w[4]; /*  4 radio buttons  */

  gboolean            antialias;
  gboolean            antialias_d;
  GtkWidget          *antialias_w;

  gboolean            feather;
  gboolean            feather_d;
  GtkWidget          *feather_w;

  gdouble             feather_radius;
  gdouble             feather_radius_d;
  GtkObject          *feather_radius_w;

  /*  used by fuzzy, by-color selection  */
  gboolean            select_transparent;
  gboolean            select_transparent_d;
  GtkWidget          *select_transparent_w;

  gboolean            sample_merged;
  gboolean            sample_merged_d;
  GtkWidget          *sample_merged_w;

  gdouble             threshold;
  /* gdouble          threshold_d; (from gimprc) */
  GtkObject          *threshold_w;

  /*  used by rect., ellipse selection  */
  gboolean            auto_shrink;
  gboolean            auto_shrink_d;
  GtkWidget          *auto_shrink_w;

  gboolean            shrink_merged;
  gboolean            shrink_merged_d;
  GtkWidget          *shrink_merged_w;

  GimpRectSelectMode  fixed_mode;
  gboolean            fixed_mode_d;
  GtkWidget          *fixed_mode_w;

  gdouble             fixed_width;
  gdouble             fixed_width_d;
  GtkObject          *fixed_width_w;

  gdouble             fixed_height;
  gdouble             fixed_height_d;
  GtkObject          *fixed_height_w;

  GimpUnit            fixed_unit;
  GimpUnit            fixed_unit_d;
  GtkWidget          *fixed_unit_w;

  /*  used by iscissors */
  gboolean            interactive;
  gboolean            interactive_d;
  GtkWidget          *interactive_w;
};


/*  selection tool options functions
 */
GimpToolOptions * selection_options_new   (GimpToolInfo     *tool_info);

void              selection_options_reset (GimpToolOptions  *tool_options);


/*  to be used by "derived" selection options only
 */
void              selection_options_init  (SelectionOptions *options,
                                           GimpToolInfo     *tool_info);


#endif  /*  __SELCTION_OPTIONS_H__  */
