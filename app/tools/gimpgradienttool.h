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

#ifndef  __GIMP_GRADIENT_TOOL_H__
#define  __GIMP_GRADIENT_TOOL_H__


#include "gimpdrawtool.h"


#define GIMP_TYPE_GRADIENT_TOOL            (gimp_gradient_tool_get_type ())
#define GIMP_GRADIENT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GRADIENT_TOOL, GimpGradientTool))
#define GIMP_GRADIENT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GRADIENT_TOOL, GimpGradientToolClass))
#define GIMP_IS_GRADIENT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GRADIENT_TOOL))
#define GIMP_IS_GRADIENT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GRADIENT_TOOL))
#define GIMP_GRADIENT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_GRADIENT_TOOL, GimpGradientToolClass))

#define GIMP_GRADIENT_TOOL_GET_OPTIONS(t)  (GIMP_GRADIENT_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpGradientTool      GimpGradientTool;
typedef struct _GimpGradientToolClass GimpGradientToolClass;

struct _GimpGradientTool
{
  GimpDrawTool        parent_instance;

  GimpGradient       *gradient;
  GimpGradient       *tentative_gradient;

  gdouble             start_x;    /*  starting x coord  */
  gdouble             start_y;    /*  starting y coord  */
  gdouble             end_x;      /*  ending x coord    */
  gdouble             end_y;      /*  ending y coord    */

  GimpToolWidget     *widget;
  GimpToolWidget     *grab_widget;

  GeglNode           *graph;
  GeglNode           *render_node;
#if 0
  GeglNode           *subtract_node;
  GeglNode           *divide_node;
#endif
  GeglNode           *dist_node;
  GeglBuffer         *dist_buffer;
  GimpDrawableFilter *filter;

  /*  editor  */

  gint                block_handlers_count;

  gint                edit_count;
  GSList             *undo_stack;
  GSList             *redo_stack;

  guint               flush_idle_id;

  GimpToolGui        *gui;
  GtkWidget          *endpoint_editor;
  GtkWidget          *endpoint_se;
  GtkWidget          *endpoint_color_panel;
  GtkWidget          *endpoint_type_combo;
  GtkWidget          *stop_editor;
  GtkWidget          *stop_se;
  GtkWidget          *stop_left_color_panel;
  GtkWidget          *stop_left_type_combo;
  GtkWidget          *stop_right_color_panel;
  GtkWidget          *stop_right_type_combo;
  GtkWidget          *stop_chain_button;
  GtkWidget          *midpoint_editor;
  GtkWidget          *midpoint_se;
  GtkWidget          *midpoint_type_combo;
  GtkWidget          *midpoint_color_combo;
  GtkWidget          *midpoint_new_stop_button;
  GtkWidget          *midpoint_center_button;
};

struct _GimpGradientToolClass
{
  GimpDrawToolClass  parent_class;
};


void    gimp_gradient_tool_register               (GimpToolRegisterCallback  callback,
                                                   gpointer                  data);

GType   gimp_gradient_tool_get_type               (void) G_GNUC_CONST;


/*  protected functions  */

void    gimp_gradient_tool_set_tentative_gradient (GimpGradientTool         *gradient_tool,
                                                   GimpGradient             *gradient);


#endif  /*  __GIMP_GRADIENT_TOOL_H__  */
