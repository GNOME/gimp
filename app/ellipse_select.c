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
#include <stdlib.h>

#include "appenv.h"
#include "edit_selection.h"
#include "ellipse_select.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "rect_select.h"
/*  private header file for rect_select data structure  */
#include "rect_selectP.h"
#include "selection_options.h"


/*  the ellipse selection tool options  */
SelectionOptions * ellipse_options = NULL;


/*************************************/
/*  Ellipsoidal selection apparatus  */

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
  Channel *new_mask;

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
      new_mask = channel_new_mask (gimage, gimage->width, gimage->height);
      channel_combine_ellipse (new_mask, ADD, x, y, w, h, antialias);
      channel_feather (new_mask, gimage_get_mask (gimage),
		       feather_radius,
		       feather_radius,
		       op, 0, 0);
      channel_delete (new_mask);
    }
  else if (op == SELECTION_INTERSECT)
    {
      new_mask = channel_new_mask (gimage, gimage->width, gimage->height);
      channel_combine_ellipse (new_mask, ADD, x, y, w, h, antialias);
      channel_combine_mask (gimage_get_mask (gimage), new_mask, op, 0, 0);
      channel_delete (new_mask);
    }
  else
    channel_combine_ellipse (gimage_get_mask (gimage), op,
			     x, y, w, h, antialias);
}

void
ellipse_select_draw (Tool *tool)
{
  GDisplay      *gdisp;
  EllipseSelect *ellipse_sel;
  gint x1, y1;
  gint x2, y2;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  ellipse_sel = (EllipseSelect *) tool->private;

  x1 = MIN (ellipse_sel->x, ellipse_sel->x + ellipse_sel->w);
  y1 = MIN (ellipse_sel->y, ellipse_sel->y + ellipse_sel->h);
  x2 = MAX (ellipse_sel->x, ellipse_sel->x + ellipse_sel->w);
  y2 = MAX (ellipse_sel->y, ellipse_sel->y + ellipse_sel->h);

  gdisplay_transform_coords (gdisp, x1, y1, &x1, &y1, 0);
  gdisplay_transform_coords (gdisp, x2, y2, &x2, &y2, 0);

  gdk_draw_arc (ellipse_sel->core->win,
		ellipse_sel->core->gc, 0,
		x1, y1, (x2 - x1), (y2 - y1), 0, 23040);
}

static void
ellipse_select_options_reset (void)
{
  selection_options_reset (ellipse_options);
}

Tool *
tools_new_ellipse_select  (void)
{
  Tool          *tool;
  EllipseSelect *private;

  /*  The tool options  */
  if (!ellipse_options)
    {
      ellipse_options =
	selection_options_new (ELLIPSE_SELECT, ellipse_select_options_reset);
      tools_register (ELLIPSE_SELECT, (ToolOptions *) ellipse_options);
    }

  tool = tools_new_tool (ELLIPSE_SELECT);
  private = g_new0 (EllipseSelect, 1);

  private->core = draw_core_new (ellipse_select_draw);
  /*  Make the selection static, not blinking  */
  private->x = private->y = 0;
  private->w = private->h = 0;

  tool->private = (void *) private;

  tool->button_press_func   = rect_select_button_press;
  tool->button_release_func = rect_select_button_release;
  tool->motion_func         = rect_select_motion;
  tool->modifier_key_func   = rect_select_modifier_update;
  tool->cursor_update_func  = rect_select_cursor_update;
  tool->oper_update_func    = rect_select_oper_update;
  tool->control_func        = rect_select_control;

  return tool;
}

void
tools_free_ellipse_select (Tool *tool)
{
  EllipseSelect *ellipse_sel;

  ellipse_sel = (EllipseSelect *) tool->private;

  draw_core_free (ellipse_sel->core);
  g_free (ellipse_sel);
}
