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
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "apptypes.h"

#include "errors.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimpchannel.h"
#include "gimpimage.h"
#include "scan_convert.h"

#include "gimpeditselectiontool.h"
#include "gimpfreeselecttool.h"
#include "selection_options.h"
#include "tool_options.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"

#include "pixmaps2.h"


#define DEFAULT_MAX_INC  1024
#define SUPERSAMPLE      3
#define SUPERSAMPLE2     9


static void   gimp_free_select_tool_class_init (GimpFreeSelectToolClass *klass);
static void   gimp_free_select_tool_init       (GimpFreeSelectTool      *free_select);
static void   gimp_free_select_tool_destroy        (GtkObject      *object);

static void   gimp_free_select_tool_button_press   (GimpTool       *tool,
                                                    GdkEventButton *bevent,
                                                    GDisplay       *gdisp);
static void   gimp_free_select_tool_button_release (GimpTool       *tool,
                                                    GdkEventButton *bevent,
                                                    GDisplay       *gdisp);
static void   gimp_free_select_tool_motion         (GimpTool       *tool,
                                                    GdkEventMotion *mevent,
                                                    GDisplay       *gdisp);

static void   gimp_free_select_tool_draw           (GimpDrawTool   *draw_tool);

static void   gimp_free_select_tool_options_reset  (void);

static void   gimp_free_select_tool_add_point      (GimpFreeSelectTool *free_sel,
                                                    gint                x,
                                                    gint                y);

static GimpChannel * scan_convert (GimpImage        *gimage,
                                   gint              num_pts,
                                   ScanConvertPoint *pts,
                                   gint              width,
                                   gint              height,
                                   gboolean          antialias);


static GimpSelectionToolClass *parent_class = NULL;

static SelectionOptions * free_options = NULL;


/*  public functions  */

void
gimp_free_select_tool_register (void)
{
  tool_manager_register_tool (GIMP_TYPE_FREE_SELECT_TOOL, FALSE,
                              "gimp:free_select_tool",
                              _("Free Select"),
                              _("Select hand-drawn regions"),
                              _("/Tools/Selection Tools/Free Select"), "F",
                              NULL, "tools/free_select.html",
                              (const gchar **) free_bits);
}

GtkType
gimp_free_select_tool_get_type (void)
{
  static GtkType free_select_type = 0;

  if (! free_select_type)
    {
      GtkTypeInfo free_select_info =
      {
        "GimpFreeSelectTool",
        sizeof (GimpFreeSelectTool),
        sizeof (GimpFreeSelectToolClass),
        (GtkClassInitFunc) gimp_free_select_tool_class_init,
        (GtkObjectInitFunc) gimp_free_select_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL
      };

      free_select_type = gtk_type_unique (GIMP_TYPE_SELECTION_TOOL,
                                          &free_select_info);
    }

  return free_select_type;
}

void
free_select (GimpImage        *gimage,
	     gint              num_pts,
	     ScanConvertPoint *pts,
	     SelectOps         op,
	     gboolean          antialias,
	     gboolean          feather,
	     gdouble           feather_radius)
{
  GimpChannel *mask;

  /*  if applicable, replace the current selection  */
  /*  or insure that a floating selection is anchored down...  */
  if (op == SELECTION_REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  mask = scan_convert (gimage, num_pts, pts,
		       gimage->width, gimage->height, antialias);

  if (mask)
    {
      if (feather)
	gimp_channel_feather (mask, gimp_image_get_mask (gimage),
			      feather_radius,
			      feather_radius,
			      op, 0, 0);
      else
	gimp_channel_combine_mask (gimp_image_get_mask (gimage),
				   mask, op, 0, 0);

      gtk_object_unref (GTK_OBJECT (mask));
    }
}


/*  private functions  */

static void
gimp_free_select_tool_class_init (GimpFreeSelectToolClass *klass)
{
  GtkObjectClass    *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = (GtkObjectClass *) klass;
  tool_class      = (GimpToolClass *) klass;
  draw_tool_class = (GimpDrawToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_SELECTION_TOOL);

  object_class->destroy      = gimp_free_select_tool_destroy;

  tool_class->button_press   = gimp_free_select_tool_button_press;
  tool_class->button_release = gimp_free_select_tool_button_release;
  tool_class->motion         = gimp_free_select_tool_motion;

  draw_tool_class->draw      = gimp_free_select_tool_draw;
}

static void
gimp_free_select_tool_init (GimpFreeSelectTool *free_select)
{
  GimpTool          *tool;
  GimpSelectionTool *select_tool;

  tool        = GIMP_TOOL (free_select);
  select_tool = GIMP_SELECTION_TOOL (free_select);

  if (! free_options)
    {
      free_options = selection_options_new (GIMP_TYPE_FREE_SELECT_TOOL,
                                            gimp_free_select_tool_options_reset);

      tool_manager_register_tool_options (GIMP_TYPE_FREE_SELECT_TOOL,
                                          (ToolOptions *) free_options);
    }

  tool->tool_cursor = GIMP_FREE_SELECT_TOOL_CURSOR;
  tool->scroll_lock = TRUE;   /*  Do not allow scrolling  */

  free_select->points     = NULL;
  free_select->num_points = 0;
  free_select->max_segs   = 0;
}

static void
gimp_free_select_tool_destroy (GtkObject *object)
{
  GimpFreeSelectTool *free_sel;

  free_sel = GIMP_FREE_SELECT_TOOL (object);

  g_free (free_sel->points);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_free_select_tool_button_press (GimpTool       *tool,
                                    GdkEventButton *bevent,
                                    GDisplay       *gdisp)
{
  GimpFreeSelectTool *free_sel;

  free_sel = GIMP_FREE_SELECT_TOOL (tool);

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
  tool->gdisp = gdisp;

  switch (GIMP_SELECTION_TOOL (tool)->op)
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

  free_sel->num_points = 0;

  gimp_free_select_tool_add_point (free_sel, bevent->x, bevent->y);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool),
                        gdisp->canvas->window);
}

static void
gimp_free_select_tool_button_release (GimpTool       *tool,
                                      GdkEventButton *bevent,
                                      GDisplay       *gdisp)
{
  GimpFreeSelectTool *free_sel;
  ScanConvertPoint   *pts;
  gint                i;

  free_sel = GIMP_FREE_SELECT_TOOL (tool);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      if (free_sel->num_points == 1)
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

      pts = g_new (ScanConvertPoint, free_sel->num_points);

      for (i = 0; i < free_sel->num_points; i++)
	{
	  gdisplay_untransform_coords_f (gdisp,
                                         free_sel->points[i].x,
                                         free_sel->points[i].y,
					 &pts[i].x,
                                         &pts[i].y,
                                         FALSE);
	}

      free_select (gdisp->gimage,
                   free_sel->num_points, pts,
                   GIMP_SELECTION_TOOL (tool)->op,
		   free_options->antialias,
                   free_options->feather,
		   free_options->feather_radius);

      g_free (pts);

      gdisplays_flush ();
    }
}

static void
gimp_free_select_tool_motion (GimpTool       *tool,
                              GdkEventMotion *mevent,
                              GDisplay       *gdisp)
{
  GimpFreeSelectTool *free_sel;
  GimpSelectionTool  *sel_tool;
  GimpDrawTool       *draw_tool;

  free_sel  = GIMP_FREE_SELECT_TOOL (tool);
  sel_tool  = GIMP_SELECTION_TOOL (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);

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

  gimp_free_select_tool_add_point (free_sel, mevent->x, mevent->y);

  gdk_draw_line (draw_tool->win,
                 draw_tool->gc,
                 free_sel->points[free_sel->num_points - 2].x,
                 free_sel->points[free_sel->num_points - 2].y,
                 free_sel->points[free_sel->num_points - 1].x,
                 free_sel->points[free_sel->num_points - 1].y);
}

static void
gimp_free_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpFreeSelectTool *free_sel;
  gint                i;

  free_sel  = GIMP_FREE_SELECT_TOOL (draw_tool);

  for (i = 1; i < free_sel->num_points; i++)
    {
      gdk_draw_line (draw_tool->win,
                     draw_tool->gc,
                     free_sel->points[i - 1].x,
                     free_sel->points[i - 1].y,
                     free_sel->points[i].x,
                     free_sel->points[i].y);
    }
}

static void
gimp_free_select_tool_options_reset (void)
{
  selection_options_reset (free_options);
}

static void
gimp_free_select_tool_add_point (GimpFreeSelectTool *free_sel,
                                 gint                x,
                                 gint                y)
{
  if (free_sel->num_points >= free_sel->max_segs)
    {
      free_sel->max_segs += DEFAULT_MAX_INC;

      free_sel->points = (GdkPoint *)
        g_realloc ((void *) free_sel->points,
                   sizeof (GdkPoint) * free_sel->max_segs);

      if (! free_sel->points)
	gimp_fatal_error ("%s(): Unable to reallocate points array",
                          G_GNUC_FUNCTION);
    }

  free_sel->points[free_sel->num_points].x = x;
  free_sel->points[free_sel->num_points].y = y;

  free_sel->num_points++;
}

static GimpChannel *
scan_convert (GimpImage        *gimage,
	      gint              num_pts,
	      ScanConvertPoint *pts,
	      gint              width,
	      gint              height,
	      gboolean          antialias)
{
  GimpChannel   *mask;
  ScanConverter *sc;

  sc = scan_converter_new (width, height, antialias ? SUPERSAMPLE : 1);
  scan_converter_add_points (sc, num_pts, pts);

  mask = scan_converter_to_channel (sc, gimage);
  scan_converter_free (sc);

  return mask;
}
