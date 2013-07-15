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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpselectbyshapetool.h"
#include "gimprectangleselectoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_select_by_shape_tool_draw   (GimpDrawTool            *draw_tool);

static void   gimp_select_by_shape_tool_select (GimpRectangleSelectTool *rect_tool,
                                               GimpChannelOps           operation,
                                               gint                     x,
                                               gint                     y,
                                               gint                     w,
                                               gint                     h);


G_DEFINE_TYPE (GimpSelectByShapeTool, gimp_select_by_shape_tool,
               GIMP_TYPE_RECTANGLE_SELECT_TOOL)

#define parent_class gimp_select_by_shape_tool_parent_class


void
gimp_select_by_shape_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data)
{
  (* callback) (GIMP_TYPE_SELECT_BY_SHAPE_TOOL,
                GIMP_TYPE_RECTANGLE_SELECT_OPTIONS,
                gimp_rectangle_select_options_gui,
                0,
                "gimp-select-by-shape-tool",
                _("Select by Shape"),
                _("Select by Shape Tool: Selects a predefined shape"),
                N_("_Select By Shape"), "<shift>S",
                NULL, GIMP_HELP_TOOL_SELECT_BY_SHAPE,
                GIMP_STOCK_TOOL_SELECT_BY_SHAPE,
                data);
}

static void
gimp_select_by_shape_tool_class_init (GimpSelectByShapeToolClass *klass)
{
  GimpDrawToolClass            *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);
  GimpRectangleSelectToolClass *rect_tool_class = GIMP_RECTANGLE_SELECT_TOOL_CLASS (klass);

  draw_tool_class->draw   = gimp_select_by_shape_tool_draw;

  rect_tool_class->select = gimp_select_by_shape_tool_select;
}

static void
gimp_select_by_shape_tool_init (GimpSelectByShapeTool *select_by_shape)
{
  GimpTool *tool = GIMP_TOOL (select_by_shape);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_ELLIPSE_SELECT);
  //Need to add new Cursors :'(
}

static void
gimp_select_by_shape_tool_draw (GimpDrawTool *draw_tool)
{
  GimpCanvasGroup                *stroke_group = NULL;

  /*GimpSelectByShapeTool        *rect_sel_tool;
  GimpSelectByShape            *priv;
  GimpCanvasGroup                *stroke_group = NULL;

  gint x1, y1, x2, y2;

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
  
  shp_sel_tool = GIMP_SELECT_BY_SHAPE_TOOL (draw_tool);
  priv          = GIMP_SELECT_BY_SHAPE_TOOL_GET (shp_sel_tool);
 
  switch(priv->shape_type)
  {
    case GIMP_SHAPE_RECTANGLE : 
    gimp_rectangle_tool_draw (draw_tool, stroke_group);
    break;

    case GIMP_SHAPE_ELLIPSE :
    {
     g_object_get (draw_tool,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

     gimp_draw_tool_add_arc (draw_tool,
                          FALSE,
                          x1, y1,
                          x2 - x1, y2 - y1,
                          0.0, 2 * G_PI);
     break;
    }

    case GIMP_SHAPE_ROUNDED_RECT :
    {
      GimpCanvasItem *item;
      gint            x1, y1, x2, y2;
      gdouble         radius;
      gint            square_size;

      g_object_get (rect_sel_tool,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      radius = MIN (priv->corner_radius,
                    MIN ((x2 - x1) / 2.0, (y2 - y1) / 2.0));

      square_size = (int) (radius * 2);

      stroke_group =
        GIMP_CANVAS_GROUP (gimp_draw_tool_add_stroke_group (draw_tool));

      item = gimp_draw_tool_add_arc (draw_tool, FALSE,
                                     x1, y1,
                                     square_size, square_size,
                                     G_PI / 2.0, G_PI / 2.0);
      gimp_canvas_group_add_item (stroke_group, item);
      gimp_draw_tool_remove_item (draw_tool, item);

      item = gimp_draw_tool_add_arc (draw_tool, FALSE,
                                     x2 - square_size, y1,
                                     square_size, square_size,
                                     0.0, G_PI / 2.0);
      gimp_canvas_group_add_item (stroke_group, item);
      gimp_draw_tool_remove_item (draw_tool, item);

      item = gimp_draw_tool_add_arc (draw_tool, FALSE,
                                     x2 - square_size, y2 - square_size,
                                     square_size, square_size,
                                     G_PI * 1.5, G_PI / 2.0);
      gimp_canvas_group_add_item (stroke_group, item);
      gimp_draw_tool_remove_item (draw_tool, item);

      item = gimp_draw_tool_add_arc (draw_tool, FALSE,
                                     x1, y2 - square_size,
                                     square_size, square_size,
                                     G_PI, G_PI / 2.0);
      gimp_canvas_group_add_item (stroke_group, item);
      gimp_draw_tool_remove_item (draw_tool, item);
      break;
    }
    default
     {
      gimp_rectangle_tool_draw (draw_tool, stroke_group);
      break;
     }
  }*/  
  gimp_rectangle_tool_draw (draw_tool, stroke_group);
}




static void
gimp_select_by_shape_tool_select (GimpRectangleSelectTool *rect_tool,
                                 GimpChannelOps           operation,
                                 gint                     x,
                                 gint                     y,
                                 gint                     w,
                                 gint                     h)
{
  GimpTool                   *tool    = GIMP_TOOL (rect_tool);
  GimpSelectionOptions       *options = GIMP_SELECTION_TOOL_GET_OPTIONS (rect_tool);
  GimpRectangleSelectOptions *sel_options     = GIMP_RECTANGLE_SELECT_TOOL_GET_OPTIONS (tool);
  GimpChannel                *channel;
  GimpImage                *image;
  
  channel = gimp_image_get_mask (gimp_display_get_image (tool->display));
  image = gimp_display_get_image (tool->display);

  switch(sel_options->shape_type)
   {
     case GIMP_SHAPE_RECTANGLE :
      gimp_channel_select_rectangle (channel,
                                     x, y, w, h,
                                     operation,
                                     options->feather,
                                     options->feather_radius,
                                     options->feather_radius,
                                     TRUE);
      break;
    
     case GIMP_SHAPE_ELLIPSE :  
      gimp_channel_select_ellipse (channel,
                                   x, y, w, h,
                                   operation,
                                   options->antialias,
                                   options->feather,
                                   options->feather_radius,
                                   options->feather_radius,
                                   TRUE);
      break;
    
     case GIMP_SHAPE_ROUNDED_RECT :
      {
       gdouble max    = MIN (w / 2.0, h / 2.0);
       //gdouble radius = MIN (sel_options->corner_radius, max);
       gdouble radius = sel_options->corner_radius*max/100;

       gimp_channel_select_round_rect (channel,
                                      x, y, w, h,
                                      radius, 
                                      radius,
                                      operation,
                                      options->antialias,
                                      options->feather,
                                      options->feather_radius,
                                      options->feather_radius,
                                      TRUE);
       break;
     }

     case GIMP_SHAPE_SINGLE_ROW :
      {
        gdouble w1 = gimp_image_get_width (image);
        gdouble h1 = gimp_image_get_height (image);

       switch(sel_options->line_orientation)
        {
          case GIMP_ORIENTATION_HORIZONTAL :
          gimp_channel_select_rectangle (channel,
                                        0, y, h1, 1,
                                        operation,
                                        options->feather,
                                        options->feather_radius,
                                        options->feather_radius,
                                        TRUE);
          break;
    
          case GIMP_ORIENTATION_VERTICAL :  
          gimp_channel_select_rectangle (channel,
                                       x, 0, 1, w1,
                                       operation,
                                       options->feather,
                                       options->feather_radius,
                                       options->feather_radius,
                                       TRUE);
          break;

         case GIMP_ORIENTATION_UNKNOWN :  
         gimp_channel_select_rectangle (channel,
                                        x, y, 1, 1,
                                        operation,
                                        options->feather,
                                        options->feather_radius,
                                        options->feather_radius,
                                        TRUE);
          break; 
       }

       break;
     }
     
     case GIMP_SHAPE_N_POLYGON :
      {
       gimp_channel_select_ellipse (channel,
                                   x, y, w, h,
                                   operation,
                                   options->antialias,
                                   options->feather,
                                   options->feather_radius,
                                   options->feather_radius,
                                   TRUE);
       break;
     }

   } 
}
