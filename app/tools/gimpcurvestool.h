/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_CURVES_TOOL_H__
#define __LIGMA_CURVES_TOOL_H__


#include "ligmafiltertool.h"


#define LIGMA_TYPE_CURVES_TOOL            (ligma_curves_tool_get_type ())
#define LIGMA_CURVES_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CURVES_TOOL, LigmaCurvesTool))
#define LIGMA_IS_CURVES_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CURVES_TOOL))
#define LIGMA_CURVES_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CURVES_TOOL, LigmaCurvesToolClass))
#define LIGMA_IS_CURVES_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CURVES_TOOL))


typedef struct _LigmaCurvesTool      LigmaCurvesTool;
typedef struct _LigmaCurvesToolClass LigmaCurvesToolClass;

struct _LigmaCurvesTool
{
  LigmaFilterTool  parent_instance;

  /* dialog */
  gdouble           scale;
  gdouble           picked_color[5];

  GtkWidget        *channel_menu;
  GtkWidget        *xrange;
  GtkWidget        *yrange;
  GtkWidget        *graph;
  GtkWidget        *point_box;
  GtkWidget        *point_input;
  GtkWidget        *point_output;
  GtkWidget        *point_type;
  GtkWidget        *curve_type;

  /* export dialog */
  gboolean          export_old_format;
};

struct _LigmaCurvesToolClass
{
  LigmaFilterToolClass  parent_class;
};


void    ligma_curves_tool_register (LigmaToolRegisterCallback  callback,
                                   gpointer                  data);

GType   ligma_curves_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_CURVES_TOOL_H__  */
