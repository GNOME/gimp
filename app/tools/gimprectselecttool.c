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

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimpmarshal.h"

#include "cursorutil.h"
#include "gdisplay.h"
#include "floating_sel.h"

#include "gimpeditselectiontool.h"
#include "gimprectselecttool.h"
#include "gimptoolinfo.h"
#include "selection_options.h"
#include "tool_options.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"

#include "pixmaps2.h"


#define STATUSBAR_SIZE 128


enum
{
  RECT_SELECT,
  LAST_SIGNAL
};


static void   gimp_rect_select_tool_class_init (GimpRectSelectToolClass *klass);
static void   gimp_rect_select_tool_init       (GimpRectSelectTool      *rect_select);
static void   gimp_rect_select_tool_destroy          (GtkObject      *object);

static void   gimp_rect_select_tool_button_press     (GimpTool       *tool,
                                                      GdkEventButton *bevent,
                                                      GDisplay       *gdisp);
static void   gimp_rect_select_tool_button_release   (GimpTool       *tool,
                                                      GdkEventButton *bevent,
                                                      GDisplay       *gdisp);
static void   gimp_rect_select_tool_motion           (GimpTool       *tool,
                                                      GdkEventMotion *mevent,
                                                      GDisplay       *gdisp);

static void   gimp_rect_select_tool_draw             (GimpDrawTool   *draw_tool);

static void   gimp_rect_select_tool_real_rect_select (GimpRectSelectTool *rect_tool,
                                                      gint                x,
                                                      gint                y,
                                                      gint                w,
                                                      gint                h);


static guint rect_select_signals[LAST_SIGNAL] = { 0 };

static GimpSelectionToolClass *parent_class = NULL;

static SelectionOptions *rect_options = NULL;


/*  public functions  */

void
gimp_rect_select_tool_register (void)
{
  tool_manager_register_tool (GIMP_TYPE_RECT_SELECT_TOOL, FALSE,
                              "gimp:rect_select_tool",
                              _("Rect Select"),
                              _("Select rectangular regions"),
                              _("/Tools/Selection Tools/Rect Select"), "R",
                              NULL, "tools/rect_select.html",
                              (const gchar **) rect_bits);
}

GtkType
gimp_rect_select_tool_get_type (void)
{
  static GtkType rect_select_type = 0;

  if (! rect_select_type)
    {
      GtkTypeInfo rect_select_info =
      {
        "GimpRectSelectTool",
        sizeof (GimpRectSelectTool),
        sizeof (GimpRectSelectToolClass),
        (GtkClassInitFunc) gimp_rect_select_tool_class_init,
        (GtkObjectInitFunc) gimp_rect_select_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL
      };

      rect_select_type = gtk_type_unique (GIMP_TYPE_SELECTION_TOOL,
                                          &rect_select_info);
    }

  return rect_select_type;
}

void
rect_select (GimpImage *gimage,
	     gint       x,
	     gint       y,
	     gint       w,
	     gint       h,
	     SelectOps  op,
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
      gimp_channel_combine_rect (new_mask, CHANNEL_OP_ADD, x, y, w, h);
      gimp_channel_feather (new_mask, gimp_image_get_mask (gimage),
			    feather_radius,
			    feather_radius,
			    op, 0, 0);
      gtk_object_unref (GTK_OBJECT (new_mask));
    }
  else if (op == SELECTION_INTERSECT)
    {
      new_mask = gimp_channel_new_mask (gimage, gimage->width, gimage->height);
      gimp_channel_combine_rect (new_mask, CHANNEL_OP_ADD, x, y, w, h);
      gimp_channel_combine_mask (gimp_image_get_mask (gimage), new_mask,
				 op, 0, 0);
      gtk_object_unref (GTK_OBJECT (new_mask));
    }
  else
    {
      gimp_channel_combine_rect (gimp_image_get_mask (gimage), op, x, y, w, h);
    }
}


/*  private funuctions  */

static void
gimp_rect_select_tool_class_init (GimpRectSelectToolClass *klass)
{
  GtkObjectClass    *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = (GtkObjectClass *) klass;
  tool_class      = (GimpToolClass *) klass;
  draw_tool_class = (GimpDrawToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_SELECTION_TOOL);

  rect_select_signals[RECT_SELECT] =
    gtk_signal_new ("rect_select",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpRectSelectToolClass,
                                       rect_select),
                    gimp_marshal_NONE__INT_INT_INT_INT,
                    GTK_TYPE_NONE, 4,
                    GTK_TYPE_INT,
                    GTK_TYPE_INT,
                    GTK_TYPE_INT,
                    GTK_TYPE_INT);

  gtk_object_class_add_signals (object_class, rect_select_signals, LAST_SIGNAL);

  object_class->destroy      = gimp_rect_select_tool_destroy;

  tool_class->button_press   = gimp_rect_select_tool_button_press;
  tool_class->button_release = gimp_rect_select_tool_button_release;
  tool_class->motion         = gimp_rect_select_tool_motion;

  draw_tool_class->draw      = gimp_rect_select_tool_draw;

  klass->rect_select         = gimp_rect_select_tool_real_rect_select;
}

static void
gimp_rect_select_tool_init (GimpRectSelectTool *rect_select)
{
  GimpTool          *tool;
  GimpSelectionTool *select_tool;

  tool        = GIMP_TOOL (rect_select);
  select_tool = GIMP_SELECTION_TOOL (rect_select);

  if (! rect_options)
    {
      rect_options = selection_options_new (GIMP_TYPE_RECT_SELECT_TOOL,
                                            selection_options_reset);

      tool_manager_register_tool_options (GIMP_TYPE_RECT_SELECT_TOOL,
                                          (ToolOptions *) rect_options);
    }

  tool->tool_cursor = GIMP_RECT_SELECT_TOOL_CURSOR;
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */

  rect_select->x = rect_select->y = 0;
  rect_select->w = rect_select->h = 0;
}

static void
gimp_rect_select_tool_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_rect_select_tool_button_press (GimpTool       *tool,
                                    GdkEventButton *bevent,
                                    GDisplay       *gdisp)
{
  GimpRectSelectTool *rect_sel;
  GimpSelectionTool  *sel_tool;
  SelectionOptions   *sel_options;
  gchar               select_mode[STATUSBAR_SIZE];
  gint                x, y;
  GimpUnit            unit = GIMP_UNIT_PIXEL;
  gdouble             unit_factor;

  rect_sel = GIMP_RECT_SELECT_TOOL (tool);
  sel_tool = GIMP_SELECTION_TOOL (tool);

  sel_options = (SelectionOptions *)
    tool_manager_get_info_by_tool (tool)->tool_options;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, TRUE, 0);

  rect_sel->x = x;
  rect_sel->y = y;

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

  rect_sel->w = 0;
  rect_sel->h = 0;

  rect_sel->center = FALSE;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
  tool->gdisp = gdisp;

  switch (sel_tool->op)
    {
    case SELECTION_MOVE_MASK:
      init_edit_selection (tool, gdisp, bevent, EDIT_MASK_TRANSLATE);
      return;
    case SELECTION_MOVE:
      init_edit_selection (tool, gdisp, bevent, EDIT_MASK_TO_LAYER_TRANSLATE);
      return;
    default:
      break;
    }

  /* initialize the statusbar display */
  rect_sel->context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "selection");

  switch (sel_tool->op)
    {
    case SELECTION_ADD:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: ADD"));
      break;
    case SELECTION_SUB:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: SUBTRACT"));
      break;
    case SELECTION_INTERSECT:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: INTERSECT"));
      break;
    case SELECTION_REPLACE:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: REPLACE"));
      break;
    default:
      break;
    }

  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar),
		      rect_sel->context_id, select_mode);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool),
                        gdisp->canvas->window);
}

static void
gimp_rect_select_tool_button_release (GimpTool       *tool,
                                      GdkEventButton *bevent,
                                      GDisplay       *gdisp)
{
  GimpRectSelectTool *rect_sel;
  GimpSelectionTool  *sel_tool;
  gint                x1, y1;
  gint                x2, y2;
  gint                w, h;

  rect_sel = GIMP_RECT_SELECT_TOOL (tool);
  sel_tool = GIMP_SELECTION_TOOL (tool);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), rect_sel->context_id);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      x1 = (rect_sel->w < 0) ? rect_sel->x + rect_sel->w : rect_sel->x;
      y1 = (rect_sel->h < 0) ? rect_sel->y + rect_sel->h : rect_sel->y;

      w = (rect_sel->w < 0) ? -rect_sel->w : rect_sel->w;
      h = (rect_sel->h < 0) ? -rect_sel->h : rect_sel->h;

      if ((!w || !h) && !rect_sel->fixed_size)
        {
          /*  If there is a floating selection, anchor it  */
          if (gimp_image_floating_sel (gdisp->gimage))
            floating_sel_anchor (gimp_image_floating_sel (gdisp->gimage));
          /*  Otherwise, clear the selection mask  */
          else
            gimage_mask_clear (gdisp->gimage);

          gdisplays_flush ();
          return;
        }

      x2 = x1 + w;
      y2 = y1 + h;

      gimp_rect_select_tool_rect_select (rect_sel,
                                         x1, y1, (x2 - x1), (y2 - y1));

      /*  show selection on all views  */
      gdisplays_flush ();
    }
}

static void
gimp_rect_select_tool_motion (GimpTool       *tool,
                              GdkEventMotion *mevent,
                              GDisplay       *gdisp)
{
  GimpRectSelectTool *rect_sel;
  GimpSelectionTool  *sel_tool;
  gchar               size[STATUSBAR_SIZE];
  gint                ox, oy;
  gint                x, y;
  gint                w, h, s;
  gint                tw, th;
  gdouble             ratio;

  rect_sel = GIMP_RECT_SELECT_TOOL (tool);
  sel_tool = GIMP_SELECTION_TOOL (tool);

  /*  needed for immediate cursor update on modifier event  */
  sel_tool->current_x = mevent->x;
  sel_tool->current_y = mevent->y;

  if (tool->state != ACTIVE)
    return;

  if (sel_tool->op == SELECTION_ANCHOR)
    {
      sel_tool->op = SELECTION_REPLACE;

      gimp_tool_cursor_update (tool, mevent, gdisp);
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

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, TRUE, 0);

  if (rect_sel->fixed_size)
    {
      if (mevent->state & GDK_SHIFT_MASK)
	{
	  ratio = (double)(rect_sel->fixed_height /
			   (double)rect_sel->fixed_width);
	  tw = x - ox;
	  th = y - oy;
	  /* 
	   * This is probably an inefficient way to do it, but it gives
	   * nicer, more predictable results than the original agorithm
	   */
 
	  if ((abs(th) < (ratio * abs(tw))) && (abs(tw) > (abs(th) / ratio)))
	    {
	      w = tw;
	      h = (int)(tw * ratio);
	      /* h should have the sign of th */
	      if ((th < 0 && h > 0) || (th > 0 && h < 0))
		h = -h;
	    } 
	  else 
	    {
	      h = th;
	      w = (int)(th / ratio);
	      /* w should have the sign of tw */
	      if ((tw < 0 && w > 0) || (tw > 0 && w < 0))
		w = -w;
	    }
	}
      else
	{
	  w = (x - ox > 0 ? rect_sel->fixed_width  : -rect_sel->fixed_width);
	  h = (y - oy > 0 ? rect_sel->fixed_height : -rect_sel->fixed_height);
	}
    }
  else
    {
      w = (x - ox);
      h = (y - oy);
    }

  /* If the shift key is down, then make the rectangle square (or
      ellipse circular) */
  if ((mevent->state & GDK_SHIFT_MASK) && !rect_sel->fixed_size)
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

  /*  If the control key is down, create the selection from the center out */
  if (mevent->state & GDK_CONTROL_MASK)
    {
      if (rect_sel->fixed_size) 
	{
          if (mevent->state & GDK_SHIFT_MASK) 
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
	  w = abs(w);
	  h = abs(h);
	  
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

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), rect_sel->context_id);

  if (gdisp->dot_for_dot)
    {
      g_snprintf (size, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Selection: "), abs(rect_sel->w), " x ", abs(rect_sel->h));
    }
  else /* show real world units */
    {
      gdouble unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);

      g_snprintf (size, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Selection: "),
		  (gdouble) abs(rect_sel->w) * unit_factor /
		  gdisp->gimage->xresolution,
		  " x ",
		  (gdouble) abs(rect_sel->h) * unit_factor /
		  gdisp->gimage->yresolution);
    }

  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), rect_sel->context_id,
		      size);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_rect_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpRectSelectTool *rect_sel;
  GimpTool           *tool;
  gint                x1, y1;
  gint                x2, y2;

  rect_sel = GIMP_RECT_SELECT_TOOL (draw_tool);
  tool     = GIMP_TOOL (draw_tool);

  x1 = MIN (rect_sel->x, rect_sel->x + rect_sel->w);
  y1 = MIN (rect_sel->y, rect_sel->y + rect_sel->h);
  x2 = MAX (rect_sel->x, rect_sel->x + rect_sel->w);
  y2 = MAX (rect_sel->y, rect_sel->y + rect_sel->h);

  gdisplay_transform_coords (tool->gdisp, x1, y1, &x1, &y1, 0);
  gdisplay_transform_coords (tool->gdisp, x2, y2, &x2, &y2, 0);

  gdk_draw_rectangle (draw_tool->win,
		      draw_tool->gc, 0,
		      x1, y1, (x2 - x1), (y2 - y1));
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

  sel_options = (SelectionOptions *)
    tool_manager_get_info_by_tool (tool)->tool_options;

  rect_select (tool->gdisp->gimage,
               x, y, w, h,
               sel_tool->op,
               sel_options->feather,
               sel_options->feather_radius);
}

void
gimp_rect_select_tool_rect_select (GimpRectSelectTool *rect_tool,
                                   gint                x,
                                   gint                y,
                                   gint                w,
                                   gint                h)
{
  g_return_if_fail (rect_tool != NULL);
  g_return_if_fail (GIMP_IS_RECT_SELECT_TOOL (rect_tool));

  gtk_signal_emit (GTK_OBJECT (rect_tool), rect_select_signals[RECT_SELECT],
                   x, y, w, h);
}
