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
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimpimage-mask.h"
#include "core/gimpimage-mask-select.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimpmarshal.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "gimpeditselectiontool.h"
#include "gimprectselecttool.h"
#include "selection_options.h"

#include "libgimp/gimpintl.h"




static void   gimp_rect_select_tool_class_init (GimpRectSelectToolClass *klass);
static void   gimp_rect_select_tool_init       (GimpRectSelectTool      *rect_select);

static void   gimp_rect_select_tool_button_press     (GimpTool        *tool,
                                                      GimpCoords      *coords,
                                                      guint32          time,
                                                      GdkModifierType  state,
                                                      GimpDisplay     *gdisp);
static void   gimp_rect_select_tool_button_release   (GimpTool        *tool,
                                                      GimpCoords      *coords,
                                                      guint32          time,
                                                      GdkModifierType  state,
                                                      GimpDisplay     *gdisp);
static void   gimp_rect_select_tool_motion           (GimpTool        *tool,
                                                      GimpCoords      *coords,
                                                      guint32          time,
                                                      GdkModifierType  state,
                                                      GimpDisplay     *gdisp);

static void   gimp_rect_select_tool_draw             (GimpDrawTool    *draw_tool);

static void   gimp_rect_select_tool_real_rect_select (GimpRectSelectTool *rect_tool,
                                                      gint                x,
                                                      gint                y,
                                                      gint                w,
                                                      gint                h);


static GimpSelectionToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_rect_select_tool_register (GimpToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (GIMP_TYPE_RECT_SELECT_TOOL,
                selection_options_new,
                FALSE,
                "gimp-rect-select-tool",
                _("Rect Select"),
                _("Select rectangular regions"),
                _("/Tools/Selection Tools/Rect Select"), "R",
                NULL, "tools/rect_select.html",
                GIMP_STOCK_TOOL_RECT_SELECT,
                data);
}

GType
gimp_rect_select_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpRectSelectToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_rect_select_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpRectSelectTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_rect_select_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_SELECTION_TOOL,
					  "GimpRectSelectTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_rect_select_tool_class_init (GimpRectSelectToolClass *klass)
{
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->button_press   = gimp_rect_select_tool_button_press;
  tool_class->button_release = gimp_rect_select_tool_button_release;
  tool_class->motion         = gimp_rect_select_tool_motion;

  draw_tool_class->draw      = gimp_rect_select_tool_draw;

  klass->rect_select         = gimp_rect_select_tool_real_rect_select;
}

static void
gimp_rect_select_tool_init (GimpRectSelectTool *rect_select)
{
  GimpTool *tool;

  tool = GIMP_TOOL (rect_select);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_RECT_SELECT_TOOL_CURSOR);

  rect_select->x = rect_select->y = 0;
  rect_select->w = rect_select->h = 0;
}

static void
gimp_rect_select_tool_button_press (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *gdisp)
{
  GimpRectSelectTool *rect_sel;
  GimpSelectionTool  *sel_tool;
  SelectionOptions   *sel_options;
  GimpUnit            unit = GIMP_UNIT_PIXEL;
  gdouble             unit_factor;

  rect_sel = GIMP_RECT_SELECT_TOOL (tool);
  sel_tool = GIMP_SELECTION_TOOL (tool);

  sel_options = (SelectionOptions *) tool->tool_info->tool_options;

  rect_sel->x = RINT (coords->x);
  rect_sel->y = RINT (coords->y);
  rect_sel->w = 0;
  rect_sel->h = 0;

  rect_sel->fixed_size   = sel_options->fixed_size;
  rect_sel->fixed_width  = sel_options->fixed_width;
  rect_sel->fixed_height = sel_options->fixed_height;
  unit = sel_options->fixed_unit;

  switch (unit)
    {
    case GIMP_UNIT_PIXEL:
      break;
    case GIMP_UNIT_PERCENT:
      rect_sel->fixed_width =
	gdisp->gimage->width * rect_sel->fixed_width / 100;
      rect_sel->fixed_height =
	gdisp->gimage->height * rect_sel->fixed_height / 100;
      break;
    default:
      unit_factor = gimp_unit_get_factor (unit);
      rect_sel->fixed_width =
	 rect_sel->fixed_width * gdisp->gimage->xresolution / unit_factor;
      rect_sel->fixed_height =
	rect_sel->fixed_height * gdisp->gimage->yresolution / unit_factor;
      break;
    }

  rect_sel->fixed_width  = MAX (1, rect_sel->fixed_width);
  rect_sel->fixed_height = MAX (1, rect_sel->fixed_height);

  rect_sel->center = FALSE;

  gimp_tool_control_activate (tool->control);
  tool->gdisp = gdisp;

  switch (sel_tool->op)
    {
    case SELECTION_MOVE_MASK:
      init_edit_selection (tool, gdisp, coords, EDIT_MASK_TRANSLATE);
      return;
    case SELECTION_MOVE:
      init_edit_selection (tool, gdisp, coords, EDIT_MASK_TO_LAYER_TRANSLATE);
      return;
    default:
      break;
    }

  switch (sel_tool->op)
    {
    case SELECTION_ADD:
      gimp_tool_push_status (tool, _("Selection: ADD"));
      break;
    case SELECTION_SUBTRACT:
      gimp_tool_push_status (tool, _("Selection: SUBTRACT"));
      break;
    case SELECTION_INTERSECT:
      gimp_tool_push_status (tool, _("Selection: INTERSECT"));
      break;
    case SELECTION_REPLACE:
      gimp_tool_push_status (tool, _("Selection: REPLACE"));
      break;
    default:
      break;
    }

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
}

static void
gimp_rect_select_tool_button_release (GimpTool        *tool,
                                      GimpCoords      *coords,
                                      guint32          time,
                                      GdkModifierType  state,
                                      GimpDisplay     *gdisp)
{
  GimpRectSelectTool *rect_sel;
  GimpSelectionTool  *sel_tool;
  gint                x1, y1;
  gint                x2, y2;
  gint                w, h;

  rect_sel = GIMP_RECT_SELECT_TOOL (tool);
  sel_tool = GIMP_SELECTION_TOOL (tool);

  gimp_tool_pop_status (tool);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control); /* sets paused_count to 0 -- is this ok? */

  /*  First take care of the case where the user "cancels" the action  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      x1 = (rect_sel->w < 0) ? rect_sel->x + rect_sel->w : rect_sel->x;
      y1 = (rect_sel->h < 0) ? rect_sel->y + rect_sel->h : rect_sel->y;

      w = (rect_sel->w < 0) ? -rect_sel->w : rect_sel->w;
      h = (rect_sel->h < 0) ? -rect_sel->h : rect_sel->h;

      if ((! w || ! h) && ! rect_sel->fixed_size)
        {
          /*  If there is a floating selection, anchor it  */
          if (gimp_image_floating_sel (gdisp->gimage))
            floating_sel_anchor (gimp_image_floating_sel (gdisp->gimage));
          /*  Otherwise, clear the selection mask  */
          else
            gimp_image_mask_clear (gdisp->gimage);

          gdisplays_flush ();
          return;
        }

      x2 = x1 + w;
      y2 = y1 + h;

      g_print ("rect_select: %d x %d\n", w, h);

      gimp_rect_select_tool_rect_select (rect_sel,
                                         x1, y1, (x2 - x1), (y2 - y1));

      /*  show selection on all views  */
      gdisplays_flush ();
    }
}

static void
gimp_rect_select_tool_motion (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpRectSelectTool *rect_sel;
  GimpSelectionTool  *sel_tool;
  gint                ox, oy;
  gint                w, h, s;
  gint                tw, th;
  gdouble             ratio;

  rect_sel = GIMP_RECT_SELECT_TOOL (tool);
  sel_tool = GIMP_SELECTION_TOOL (tool);

  if (!gimp_tool_control_is_active (tool->control))
    return;

  if (sel_tool->op == SELECTION_ANCHOR)
    {
      sel_tool->op = SELECTION_REPLACE;

      gimp_tool_cursor_update (tool, coords, state, gdisp);
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* Calculate starting point */

  if (rect_sel->center)
    {
      ox = rect_sel->x + rect_sel->w / 2;
      oy = rect_sel->y + rect_sel->h / 2;
    }
  else
    {
      ox = rect_sel->x;
      oy = rect_sel->y;
    }

  if (rect_sel->fixed_size)
    {
      if (state & GDK_SHIFT_MASK)
	{
	  ratio = ((gdouble) rect_sel->fixed_height /
                   (gdouble) rect_sel->fixed_width);
	  tw = RINT (coords->x) - ox;
	  th = RINT (coords->y) - oy;

          /* This is probably an inefficient way to do it, but it gives
	   * nicer, more predictable results than the original agorithm
	   */
	  if ((abs (th) < (ratio * abs (tw))) &&
              (abs (tw) > (abs (th) / ratio)))
	    {
	      w = tw;
	      h = (gint) (tw * ratio);
	      /* h should have the sign of th */
	      if ((th < 0 && h > 0) || (th > 0 && h < 0))
		h = -h;
	    }
	  else 
	    {
	      h = th;
	      w = (gint) (th / ratio);
	      /* w should have the sign of tw */
	      if ((tw < 0 && w > 0) || (tw > 0 && w < 0))
		w = -w;
	    }
	}
      else
	{
	  w = (RINT (coords->x) - ox > 0 ?
               rect_sel->fixed_width  : -rect_sel->fixed_width);

	  h = (RINT (coords->y) - oy > 0 ?
               rect_sel->fixed_height : -rect_sel->fixed_height);
	}
    }
  else
    {
      w = (RINT (coords->x) - ox);
      h = (RINT (coords->y) - oy);
    }

  /* If the shift key is down, then make the rectangle square (or
   * ellipse circular)
   */
  if ((state & GDK_SHIFT_MASK) && ! rect_sel->fixed_size)
    {
      s = MAX (abs (w), abs (h));

      if (w < 0)
	w = -s;
      else
	w = s;

      if (h < 0)
	h = -s;
      else
	h = s;
    }

  /*  If the control key is down, create the selection from the center out
   */
  if (state & GDK_CONTROL_MASK)
    {
      if (rect_sel->fixed_size) 
	{
          if (state & GDK_SHIFT_MASK) 
            {
              rect_sel->x = ox - w;
              rect_sel->y = oy - h;
              rect_sel->w = w * 2;
              rect_sel->h = h * 2;
            }
          else
            {
              rect_sel->x = ox - w / 2;
              rect_sel->y = oy - h / 2;
              rect_sel->w = w;
              rect_sel->h = h;
            }
	}
      else
	{
	  w = abs (w);
	  h = abs (h);

	  rect_sel->x = ox - w;
	  rect_sel->y = oy - h;
	  rect_sel->w = 2 * w + 1;
	  rect_sel->h = 2 * h + 1;
	}
      rect_sel->center = TRUE;
    }
  else
    {
      rect_sel->x = ox;
      rect_sel->y = oy;
      rect_sel->w = w;
      rect_sel->h = h;

      rect_sel->center = FALSE;
    }

  gimp_tool_pop_status (tool);

  gimp_tool_push_status_coords (tool,
                                _("Selection: "),
                                abs (rect_sel->w),
                                " x ",
                                abs (rect_sel->h));

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_rect_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpRectSelectTool *rect_sel;

  rect_sel = GIMP_RECT_SELECT_TOOL (draw_tool);

  gimp_draw_tool_draw_rectangle (draw_tool,
                                 FALSE,
                                 rect_sel->x,
                                 rect_sel->y,
                                 rect_sel->w,
                                 rect_sel->h,
                                 FALSE);
}

static void
gimp_rect_select_tool_real_rect_select (GimpRectSelectTool *rect_tool,
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

  sel_options = (SelectionOptions *) tool->tool_info->tool_options;

  gimp_image_mask_select_rectangle (tool->gdisp->gimage,
                                    x, y, w, h,
                                    sel_tool->op,
                                    sel_options->feather,
                                    sel_options->feather_radius,
                                    sel_options->feather_radius);
}

void
gimp_rect_select_tool_rect_select (GimpRectSelectTool *rect_tool,
                                   gint                x,
                                   gint                y,
                                   gint                w,
                                   gint                h)
{
  GimpTool         *tool;
  SelectionOptions *sel_options;

  g_return_if_fail (GIMP_IS_RECT_SELECT_TOOL (rect_tool));

  tool = GIMP_TOOL (rect_tool);

  sel_options = (SelectionOptions *) tool->tool_info->tool_options;

  if (sel_options->auto_shrink)
    {
      gint x2, y2;

      x = CLAMP (x, 0, tool->gdisp->gimage->width);
      y = CLAMP (y, 0, tool->gdisp->gimage->height);
      w = CLAMP (w, 0, tool->gdisp->gimage->width  - x);
      h = CLAMP (h, 0, tool->gdisp->gimage->height - y);

      if (w < 1 || h < 1)
        return;

      if (! sel_options->shrink_merged)
        {
          GimpDrawable *drawable;
          gint          off_x, off_y;
          gint          width, height;

          drawable = gimp_image_active_drawable (tool->gdisp->gimage);
          gimp_drawable_offsets (drawable, &off_x, &off_y);
          width  = gimp_drawable_width (drawable);
          height = gimp_drawable_height (drawable);

          x = CLAMP (x, off_x, off_x + width);
          y = CLAMP (y, off_y, off_y + height);
          w = CLAMP (w, 0, off_x + width  - x);
          h = CLAMP (h, 0, off_y + height - y);
        }

      if (gimp_image_crop_auto_shrink (tool->gdisp->gimage,
                                       x, y,
                                       x + w, y + h,
                                       ! sel_options->shrink_merged,
                                       &x, &y,
                                       &x2, &y2))
        {
          w = x2 - x;
          h = y2 - y;
        }
    }

  GIMP_RECT_SELECT_TOOL_GET_CLASS (rect_tool)->rect_select (rect_tool,
                                                            x, y, w, h);
}
