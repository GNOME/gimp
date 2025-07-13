/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "gimpfiltertool.h"


#define GIMP_TYPE_LEVELS_TOOL            (gimp_levels_tool_get_type ())
#define GIMP_LEVELS_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LEVELS_TOOL, GimpLevelsTool))
#define GIMP_LEVELS_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LEVELS_TOOL, GimpLevelsToolClass))
#define GIMP_IS_LEVELS_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LEVELS_TOOL))
#define GIMP_IS_LEVELS_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LEVELS_TOOL))
#define GIMP_LEVELS_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LEVELS_TOOL, GimpLevelsToolClass))


typedef struct _GimpLevelsTool      GimpLevelsTool;
typedef struct _GimpLevelsToolClass GimpLevelsToolClass;

struct _GimpLevelsTool
{
  GimpFilterTool  parent_instance;

  /* dialog */
  GimpHistogram  *histogram;
  GimpAsync      *histogram_async;

  GtkWidget      *channel_menu;

  GtkWidget      *histogram_view;

  GtkWidget      *input_bar;
  GtkWidget      *low_input_spinbutton;
  GtkWidget      *high_input_spinbutton;
  GtkWidget      *low_output_spinbutton;
  GtkWidget      *high_output_spinbutton;
  GtkAdjustment  *low_input;
  GtkAdjustment  *gamma;
  GtkAdjustment  *gamma_linear;
  GtkAdjustment  *high_input;

  GtkWidget      *output_bar;

  /* export dialog */
  gboolean        export_old_format;
};

struct _GimpLevelsToolClass
{
  GimpFilterToolClass  parent_class;
};


void    gimp_levels_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data);

GType   gimp_levels_tool_get_type (void) G_GNUC_CONST;
