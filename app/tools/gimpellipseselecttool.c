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

#include "config.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include "apptypes.h"

#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimpchannel.h"
#include "gimpimage.h"

#include "gimpellipseselecttool.h"
#include "gimptoolinfo.h"
#include "selection_options.h"
#include "tool_options.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"

#include "pixmaps2.h"


static void   gimp_ellipse_select_tool_class_init (GimpEllipseSelectToolClass *klass);
static void   gimp_ellipse_select_tool_init       (GimpEllipseSelectTool      *ellipse_select);
static void   gimp_ellipse_select_tool_destroy     (GtkObject          *object);

static void   gimp_ellipse_select_tool_draw        (GimpDrawTool       *draw_tool);

static void   gimp_ellipse_select_tool_rect_select (GimpRectSelectTool *rect_tool,
                                                    gint                x,
                                                    gint                y,
                                                    gint                w,
                                                    gint                h);

static void   gimp_ellipse_select_tool_options_reset (void);


static GimpRectSelectToolClass *parent_class = NULL;

static SelectionOptions *ellipse_options = NULL;


/*  public functions  */

void
gimp_ellipse_select_tool_register (void)
{
  tool_manager_register_tool (GIMP_TYPE_ELLIPSE_SELECT_TOOL, FALSE,
                              "gimp:ellipse_select_tool",
                              _("Ellipse Select"),
                              _("Select elliptical regions"),
                              _("/Tools/Selection Tools/Ellipse Select"), "E",
                              NULL, "tools/ellipse_select.html",
                              (const gchar **) circ_bits);
}

GtkType
gimp_ellipse_select_tool_get_type (void)
{
  static GtkType ellipse_select_type = 0;

  if (! ellipse_select_type)
    {
      GtkTypeInfo ellipse_select_info =
      {
        "GimpEllipseSelectTool",
        sizeof (GimpEllipseSelectTool),
        sizeof (GimpEllipseSelectToolClass),
        (GtkClassInitFunc) gimp_ellipse_select_tool_class_init,
        (GtkObjectInitFunc) gimp_ellipse_select_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL
      };

      ellipse_select_type = gtk_type_unique (GIMP_TYPE_RECT_SELECT_TOOL,
                                             &ellipse_select_info);
    }

  return ellipse_select_type;
}

void
ellipse_select (GimpImage *gimage,
		gint       x,
		gint       y,
		gint       w,
		gint       h,
		SelectOps  op,
		gboolean   antialias,
		gboolean   feather,
		gdouble    feather_radius)
{
  GimpChannel *new_mask;

  /*  if applicable, replace the current selection  */
  if (op == SELECTION_REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      new_mask = gimp_channel_new_mask (gimage, gimage->width, gimage->height);
      gimp_channel_combine_ellipse (new_mask, CHANNEL_OP_ADD,
				    x, y, w, h, antialias);
      gimp_channel_feather (new_mask, gimp_image_get_mask (gimage),
			    feather_radius,
			    feather_radius,
			    op, 0, 0);
      gtk_object_unref (GTK_OBJECT (new_mask));
    }
  else if (op == SELECTION_INTERSECT)
    {
      new_mask = gimp_channel_new_mask (gimage, gimage->width, gimage->height);
      gimp_channel_combine_ellipse (new_mask, CHANNEL_OP_ADD,
				    x, y, w, h, antialias);
      gimp_channel_combine_mask (gimp_image_get_mask (gimage), new_mask,
				 op, 0, 0);
      gtk_object_unref (GTK_OBJECT (new_mask));
    }
  else
    {
      gimp_channel_combine_ellipse (gimp_image_get_mask (gimage), op,
				    x, y, w, h, antialias);
    }
}


/*  private functions  */

static void
gimp_ellipse_select_tool_class_init (GimpEllipseSelectToolClass *klass)
{
  GtkObjectClass          *object_class;
  GimpToolClass           *tool_class;
  GimpDrawToolClass       *draw_tool_class;
  GimpRectSelectToolClass *rect_tool_class;

  object_class    = (GtkObjectClass *) klass;
  tool_class      = (GimpToolClass *) klass;
  draw_tool_class = (GimpDrawToolClass *) klass;
  rect_tool_class = (GimpRectSelectToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_RECT_SELECT_TOOL);

  object_class->destroy        = gimp_ellipse_select_tool_destroy;

  draw_tool_class->draw        = gimp_ellipse_select_tool_draw;

  rect_tool_class->rect_select = gimp_ellipse_select_tool_rect_select;
}

static void
gimp_ellipse_select_tool_init (GimpEllipseSelectTool *ellipse_select)
{
  GimpTool          *tool;
  GimpSelectionTool *select_tool;

  tool        = GIMP_TOOL (ellipse_select);
  select_tool = GIMP_SELECTION_TOOL (ellipse_select);

  if (! ellipse_options)
    {
      ellipse_options =
        selection_options_new (GIMP_TYPE_ELLIPSE_SELECT_TOOL,
                               gimp_ellipse_select_tool_options_reset);

      tool_manager_register_tool_options (GIMP_TYPE_ELLIPSE_SELECT_TOOL,
                                          (ToolOptions *) ellipse_options);
    }

  tool->tool_cursor = GIMP_ELLIPSE_SELECT_TOOL_CURSOR;
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */
}

static void
gimp_ellipse_select_tool_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_ellipse_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool           *tool;
  GimpRectSelectTool *rect_sel;
  gint                x1, y1;
  gint                x2, y2;

  tool     = GIMP_TOOL (draw_tool);
  rect_sel = GIMP_RECT_SELECT_TOOL (draw_tool);

  x1 = MIN (rect_sel->x, rect_sel->x + rect_sel->w);
  y1 = MIN (rect_sel->y, rect_sel->y + rect_sel->h);
  x2 = MAX (rect_sel->x, rect_sel->x + rect_sel->w);
  y2 = MAX (rect_sel->y, rect_sel->y + rect_sel->h);

  gdisplay_transform_coords (tool->gdisp, x1, y1, &x1, &y1, 0);
  gdisplay_transform_coords (tool->gdisp, x2, y2, &x2, &y2, 0);

  gdk_draw_arc (draw_tool->win,
		draw_tool->gc,
                0,
		x1, y1, (x2 - x1), (y2 - y1),
                0, 23040);
}

static void
gimp_ellipse_select_tool_rect_select (GimpRectSelectTool *rect_tool,
                                      gint                x,
                                      gint                y,
                                      gint                w,
                                      gint                h)
{
  GimpTool          *tool;
  GimpSelectionTool *sel_tool;
  SelectionOptions  *sel_options;

  tool     = GIMP_TOOL (rect_tool);
  sel_tool = GIMP_SELECTION_TOOL (rect_tool);

  sel_options = (SelectionOptions *)
    tool_manager_get_info_by_tool (tool)->tool_options;

  ellipse_select (tool->gdisp->gimage,
                  x, y, w, h,
                  sel_tool->op,
                  sel_options->antialias,
                  sel_options->feather,
                  sel_options->feather_radius);
}

static void
gimp_ellipse_select_tool_options_reset (void)
{
  selection_options_reset (ellipse_options);
}
