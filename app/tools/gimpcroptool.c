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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"
#include "gui/gui-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimpimage-mask.h"

#include "gui/info-dialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "gimpcroptool.h"
#include "gimpdrawtool.h"
#include "gimptool.h"
#include "tool_options.h"
#include "tool_manager.h"

#include "app_procs.h"

#include "libgimp/gimpintl.h"


#define STATUSBAR_SIZE 128

/*  possible crop functions  */
#define CREATING        0
#define MOVING          1
#define RESIZING_LEFT   2
#define RESIZING_RIGHT  3
#define CROPPING        4

/*  speed of key movement  */
#define ARROW_VELOCITY 25

/*  the crop structures  */

typedef struct _CropOptions CropOptions;

struct _CropOptions
{
  GimpToolOptions  tool_options;

  gboolean     layer_only;
  gboolean     layer_only_d;
  GtkWidget   *layer_only_w;

  gboolean     allow_enlarge;
  gboolean     allow_enlarge_d;
  GtkWidget   *allow_enlarge_w;

  CropType     type;
  CropType     type_d;
  GtkWidget   *type_w[2];
};


/*  the crop tool options  */
static CropOptions *crop_options = NULL;

/*  the crop tool info dialog  */
static InfoDialog  *crop_info = NULL;

static gdouble      orig_vals[2];
static gdouble      size_vals[2];

static GtkWidget   *origin_sizeentry;
static GtkWidget   *size_sizeentry;


static void   gimp_crop_tool_class_init     (GimpCropToolClass *klass);
static void   gimp_crop_tool_init           (GimpCropTool      *crop_tool);

static void   gimp_crop_tool_finalize       (GObject         *object);

static void   gimp_crop_tool_button_press   (GimpTool        *tool,
					     GdkEventButton  *bevent,
					     GDisplay        *gdisp);
static void   gimp_crop_tool_button_release (GimpTool        *tool,
					     GdkEventButton  *bevent,
					     GDisplay        *gdisp);
static void   gimp_crop_tool_motion         (GimpTool        *tool,
					     GdkEventMotion  *mevent,
					     GDisplay        *gdisp);
static void   gimp_crop_tool_cursor_update  (GimpTool        *tool,
					     GdkEventMotion  *mevent,
					     GDisplay        *gdisp);
static void   gimp_crop_tool_control        (GimpTool        *tool,
					     ToolAction       action,
					     GDisplay        *gdisp);
static void   gimp_crop_tool_arrow_key      (GimpTool        *tool,
					     GdkEventKey     *kevent,
					     GDisplay        *gdisp);
static void   gimp_crop_tool_modifier_key   (GimpTool        *tool,
					     GdkEventKey     *kevent,
					     GDisplay        *gdisp);

static void   gimp_crop_tool_draw           (GimpDrawTool    *draw_tool);

/*  Crop helper functions  */
static void   crop_tool_crop_image          (GimpImage       *gimage,
					     gint             x1,
					     gint             y1,
					     gint             x2,
					     gint             y2,
					     gboolean         layer_only,
					     gboolean         crop_layers);

static void   crop_recalc                   (GimpTool        *tool,
					     GimpCropTool    *crop);
static void   crop_start                    (GimpTool        *tool,
					     GimpCropTool    *crop);

/*  Crop dialog functions  */
static void   crop_info_update              (GimpTool        *tool);
static void   crop_info_create              (GimpTool        *tool);
static void   crop_crop_callback            (GtkWidget       *widget,
					     gpointer         data);
static void   crop_resize_callback          (GtkWidget       *widget,
					     gpointer         data);
static void   crop_close_callback           (GtkWidget       *widget,
					     gpointer         data);

static void   crop_selection_callback       (GtkWidget       *widget,
					     gpointer         data);
static void   crop_automatic_callback       (GtkWidget       *widget,
					     gpointer         data);

static void   crop_origin_changed           (GtkWidget       *widget,
					     gpointer         data);
static void   crop_size_changed             (GtkWidget       *widget,
					     gpointer         data);

static CropOptions * crop_options_new       (void);
static void          crop_options_reset     (GimpToolOptions *tool_options);


static GimpDrawToolClass *parent_class = NULL;


/*  Functions  */

void
gimp_crop_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_CROP_TOOL,
                              FALSE,
                              "gimp:crop_tool",
                              _("Crop Tool"),
                              _("Crop or Resize an image"),
                              N_("/Tools/Transform Tools/Crop Tool"), "<shift>C",
                              NULL, "tools/crop_tool.html",
                              GIMP_STOCK_TOOL_CROP);
}

GType
gimp_crop_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpCropToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_crop_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpCropTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_crop_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpCropTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_crop_tool_class_init (GimpCropToolClass *klass)
{
  GObjectClass      *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = G_OBJECT_CLASS (klass);
  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_crop_tool_finalize;

  tool_class->control        = gimp_crop_tool_control;
  tool_class->button_press   = gimp_crop_tool_button_press;
  tool_class->button_release = gimp_crop_tool_button_release;
  tool_class->motion         = gimp_crop_tool_motion;
  tool_class->cursor_update  = gimp_crop_tool_cursor_update;
  tool_class->arrow_key      = gimp_crop_tool_arrow_key;
  tool_class->modifier_key   = gimp_crop_tool_modifier_key;

  draw_tool_class->draw      = gimp_crop_tool_draw;
}

static void
gimp_crop_tool_init (GimpCropTool *crop_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (crop_tool);

  if (! crop_options)
    {
      crop_options = crop_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_CROP_TOOL,
                                         (GimpToolOptions *) crop_options);
    }

  tool->preserve = FALSE;  /*  Don't preserve on drawable change  */
}

static void
gimp_crop_tool_finalize (GObject *object)
{
  if (crop_info)
    {
      info_dialog_free (crop_info);
      crop_info = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_crop_tool_control (GimpTool   *tool,
			ToolAction  action,
			GDisplay   *gdisp)
{
  GimpCropTool *crop_tool;

  crop_tool = GIMP_CROP_TOOL (tool);
  
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      crop_recalc (tool, crop_tool);
      break;

    case HALT:
      crop_close_callback (NULL, NULL);
      break;

    default:
      break;
    }

  if (GIMP_TOOL_CLASS (parent_class)->control)
    GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_crop_tool_button_press (GimpTool       *tool,
			     GdkEventButton *bevent,
			     GDisplay       *gdisp)
{
  GimpCropTool *crop;
  GimpDrawTool *draw;
  
  crop = GIMP_CROP_TOOL (tool);
  draw = GIMP_DRAW_TOOL (tool);
  
  if (tool->state == INACTIVE ||
      gdisp != tool->gdisp)
    {
      crop->function = CREATING;
    }
  else
    {
      /*  If the cursor is in either the upper left or lower right boxes,
	  The new function will be to resize the current crop area        */
      if (bevent->x == CLAMP (bevent->x, crop->x1, crop->x1 + crop->srw) &&
	  bevent->y == CLAMP (bevent->y, crop->y1, crop->y1 + crop->srh))
	crop->function = RESIZING_LEFT;
      else if (bevent->x == CLAMP (bevent->x, crop->x2 - crop->srw, crop->x2) &&
	       bevent->y == CLAMP (bevent->y, crop->y2 - crop->srh, crop->y2))
	crop->function = RESIZING_RIGHT;

      /*  If the cursor is in either the upper right or lower left boxes,
	  The new function will be to translate the current crop area     */
      else if  ((bevent->x == CLAMP (bevent->x,
				     crop->x1, crop->x1 + crop->srw) &&
		 bevent->y == CLAMP (bevent->y,
				     crop->y2 - crop->srh, crop->y2)) ||
		(bevent->x == CLAMP (bevent->x,
				     crop->x2 - crop->srw, crop->x2) &&
		 bevent->y == CLAMP (bevent->y,
				     crop->y1, crop->y1 + crop->srh)))
	crop->function = MOVING;

      /*  If the pointer is in the rectangular region, crop or resize it!  */
      else if (bevent->x > crop->x1 && bevent->x < crop->x2 &&
	       bevent->y > crop->y1 && bevent->y < crop->y2)
	crop->function = CROPPING;
      /*  otherwise, the new function will be creating, since we want to start anew  */
      else
	crop->function = CREATING;
    }

  if (crop->function == CREATING)
    {
      if (tool->state == ACTIVE)
	gimp_draw_tool_stop(draw);

      tool->gdisp    = gdisp;
      tool->drawable = gimp_image_active_drawable (gdisp->gimage);

      gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
				   &crop->tx1, &crop->ty1, TRUE, FALSE);
      crop->tx2 = crop->tx1;
      crop->ty2 = crop->ty1;

      crop_start (tool, crop);
    }

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &crop->startx, &crop->starty, TRUE, FALSE);
  crop->lastx = crop->startx;
  crop->lasty = crop->starty;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
}

static void
gimp_crop_tool_button_release (GimpTool       *tool,
			       GdkEventButton *bevent,
			       GDisplay       *gdisp)
{
  GimpCropTool *crop;

  crop = GIMP_CROP_TOOL (tool);
  
  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), crop->context_id);

  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      if (crop->function == CROPPING)
	{
	  if (crop_options->type == CROP_CROP)
	    crop_tool_crop_image (gdisp->gimage,
				  crop->tx1, crop->ty1, crop->tx2, crop->ty2, 
				  crop_options->layer_only, TRUE);
	  else
	    crop_tool_crop_image (gdisp->gimage,
				  crop->tx1, crop->ty1, crop->tx2, crop->ty2, 
				  crop_options->layer_only, FALSE);

	  /*  Finish the tool  */
	  crop_close_callback (NULL, NULL);
	}
      else
	crop_info_update (tool);
    }
}

static void
gimp_crop_tool_motion (GimpTool       *tool,
		       GdkEventMotion *mevent,
		       GDisplay       *gdisp)
{
  GimpCropTool *crop;
  GimpDrawTool *draw;
  GimpLayer    *layer;
  gint          x1, y1, x2, y2;
  gint          curx, cury;
  gint          inc_x, inc_y;
  gchar         size[STATUSBAR_SIZE];
  gint          min_x, min_y, max_x, max_y; 

  crop = GIMP_CROP_TOOL (tool);
  draw = GIMP_DRAW_TOOL (tool);
  
  /*  This is the only case when the motion events should be ignored--
      we're just waiting for the button release event to crop the image  */
  if (crop->function == CROPPING)
    return;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &curx, &cury,
			       TRUE, FALSE);
  x1 = crop->startx;
  y1 = crop->starty;
  x2 = curx;
  y2 = cury;

  inc_x = (x2 - x1);
  inc_y = (y2 - y1);

  /*  If there have been no changes... return  */
  if (crop->lastx == curx && crop->lasty == cury)
    return;

  gimp_draw_tool_pause (draw);

  if (crop_options->layer_only)
    {
      layer = gimp_image_get_active_layer(gdisp->gimage);
      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &min_x, &min_y);
      max_x = gimp_drawable_width (GIMP_DRAWABLE (layer)) + min_x;
      max_y = gimp_drawable_height (GIMP_DRAWABLE (layer)) + min_y;
    }
  else
    {
      min_x = min_y = 0;
      max_x = gdisp->gimage->width;
      max_y = gdisp->gimage->height;
    }

  switch (crop->function)
    {
    case CREATING :
      if (!crop_options->allow_enlarge)
	{
	  x1 = CLAMP (x1, min_x, max_x);
	  y1 = CLAMP (y1, min_y, max_y);
	  x2 = CLAMP (x2, min_x, max_x);
	  y2 = CLAMP (y2, min_y, max_y);
	}
      break;

    case RESIZING_LEFT :
      x1 = crop->tx1 + inc_x;
      y1 = crop->ty1 + inc_y;
      if (!crop_options->allow_enlarge)
	{
	  x1 = CLAMP (x1, min_x, max_x);
	  y1 = CLAMP (y1, min_y, max_y);
	}
      x2 = MAX (x1, crop->tx2);
      y2 = MAX (y1, crop->ty2);
      crop->startx = curx;
      crop->starty = cury;
      break;

    case RESIZING_RIGHT :
      x2 = crop->tx2 + inc_x;
      y2 = crop->ty2 + inc_y;
      if (!crop_options->allow_enlarge)
	{
	  x2 = CLAMP (x2, min_x, max_x);
	  y2 = CLAMP (y2, min_y, max_y);
	}
      x1 = MIN (crop->tx1, x2);
      y1 = MIN (crop->ty1, y2);
      crop->startx = curx;
      crop->starty = cury;
      break;

    case MOVING :
      if (!crop_options->allow_enlarge)
	{
	  inc_x = CLAMP (inc_x, min_x - crop->tx1, max_x - crop->tx2);
	  inc_y = CLAMP (inc_y, min_y - crop->ty1, max_y - crop->ty2);
	}
      x1 = crop->tx1 + inc_x;
      x2 = crop->tx2 + inc_x;
      y1 = crop->ty1 + inc_y;
      y2 = crop->ty2 + inc_y;
      crop->startx = curx;
      crop->starty = cury;
      break;
    }

  /*  make sure that the coords are in bounds  */
  crop->tx1 = MIN (x1, x2);
  crop->ty1 = MIN (y1, y2);
  crop->tx2 = MAX (x1, x2);
  crop->ty2 = MAX (y1, y2);

  crop->lastx = curx;
  crop->lasty = cury;

  /*  recalculate the coordinates for crop_draw based on the new values  */
  crop_recalc (tool, crop);

  if (crop->function == CREATING || 
      crop->function == RESIZING_LEFT || crop->function == RESIZING_RIGHT)
    {
      gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), crop->context_id);
      if (gdisp->dot_for_dot)
	{
	  g_snprintf (size, STATUSBAR_SIZE, gdisp->cursor_format_str,
		      _("Crop: "), 
		      (crop->tx2 - crop->tx1), " x ", (crop->ty2 - crop->ty1));
	}
      else /* show real world units */
	{
	  gdouble unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);
	  
	  g_snprintf (size, STATUSBAR_SIZE, gdisp->cursor_format_str,
		      _("Crop: "), 
		      (gdouble) (crop->tx2 - crop->tx1) * unit_factor /
		      gdisp->gimage->xresolution,
		      " x ",
		      (gdouble) (crop->ty2 - crop->ty1) * unit_factor /
		      gdisp->gimage->yresolution);
	}
      gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), crop->context_id,
			  size);
    }

  gimp_draw_tool_resume (draw);
}

static void
gimp_crop_tool_cursor_update (GimpTool       *tool,
			      GdkEventMotion *mevent,
			      GDisplay       *gdisp)
{
  GimpCropTool      *crop;
  GdkCursorType      ctype     = GIMP_MOUSE_CURSOR;
  GimpCursorModifier cmodifier = GIMP_CURSOR_MODIFIER_NONE;

  crop = GIMP_CROP_TOOL (tool);

  if (tool->state == INACTIVE ||
      (tool->state == ACTIVE && tool->gdisp != gdisp))
    {
      ctype = GIMP_CROSSHAIR_SMALL_CURSOR;
    }
  else if (mevent->x == CLAMP (mevent->x, crop->x1, crop->x1 + crop->srw) &&
	   mevent->y == CLAMP (mevent->y, crop->y1, crop->y1 + crop->srh))
    {
      cmodifier = GIMP_CURSOR_MODIFIER_RESIZE;
    }
  else if (mevent->x == CLAMP (mevent->x, crop->x2 - crop->srw, crop->x2) &&
	   mevent->y == CLAMP (mevent->y, crop->y2 - crop->srh, crop->y2))
    {
      cmodifier = GIMP_CURSOR_MODIFIER_RESIZE;
    }
  else if  (mevent->x == CLAMP (mevent->x, crop->x1, crop->x1 + crop->srw) &&
	    mevent->y == CLAMP (mevent->y, crop->y2 - crop->srh, crop->y2))
    {
      cmodifier = GIMP_CURSOR_MODIFIER_MOVE;
    }
  else if  (mevent->x == CLAMP (mevent->x, crop->x2 - crop->srw, crop->x2) &&
	    mevent->y == CLAMP (mevent->y, crop->y1, crop->y1 + crop->srh))
    {
      cmodifier = GIMP_CURSOR_MODIFIER_MOVE;
    }
  else if (! (mevent->x > crop->x1 && mevent->x < crop->x2 &&
	      mevent->y > crop->y1 && mevent->y < crop->y2))
    {
      ctype = GIMP_CROSSHAIR_SMALL_CURSOR;
    }

  gdisplay_install_tool_cursor (gdisp,
				ctype,
				crop_options->type == CROP_CROP ?
				GIMP_CROP_TOOL_CURSOR : GIMP_RESIZE_TOOL_CURSOR,
				cmodifier);
}

static void
gimp_crop_tool_arrow_key (GimpTool    *tool,
			  GdkEventKey *kevent,
			  GDisplay    *gdisp)
{
  GimpLayer    *layer;
  GimpDrawTool *draw;
  GimpCropTool *crop;
  gint          inc_x, inc_y;
  gint          min_x, min_y;
  gint          max_x, max_y;

  crop = GIMP_CROP_TOOL (tool);
  draw = GIMP_DRAW_TOOL (tool);

  if (tool->state == ACTIVE)
    {
      inc_x = inc_y = 0;

      switch (kevent->keyval)
	{
	case GDK_Up    : inc_y = -1; break;
	case GDK_Left  : inc_x = -1; break;
	case GDK_Right : inc_x =  1; break;
	case GDK_Down  : inc_y =  1; break;
	}

      /*  If the shift key is down, move by an accelerated increment  */
      if (kevent->state & GDK_SHIFT_MASK)
	{
	  inc_y *= ARROW_VELOCITY;
	  inc_x *= ARROW_VELOCITY;
	}

      gimp_draw_tool_pause (draw);

      if (crop_options->layer_only)
	{
	  layer = gimp_image_get_active_layer(gdisp->gimage);
	  gimp_drawable_offsets (GIMP_DRAWABLE (layer), &min_x, &min_y);
	  max_x = gimp_drawable_width (GIMP_DRAWABLE (layer)) + min_x;
	  max_y = gimp_drawable_height (GIMP_DRAWABLE (layer)) + min_y;
	}
      else
	{
	  min_x = min_y = 0;
	  max_x = gdisp->gimage->width;
	  max_y = gdisp->gimage->height;
	}

      if (kevent->state & GDK_CONTROL_MASK)  /* RESIZING */
	{
	  crop->tx2 = crop->tx2 + inc_x;
	  crop->ty2 = crop->ty2 + inc_y;
	  if (!crop_options->allow_enlarge)
	    {
	      crop->tx2 = CLAMP (crop->tx2, min_x, max_x);
	      crop->ty2 = CLAMP (crop->ty2, min_y, max_y);
	    }
	  crop->tx1 = MIN (crop->tx1, crop->tx2);
	  crop->ty1 = MIN (crop->ty1, crop->ty2);
	}
      else
	{
	  if (!crop_options->allow_enlarge)
	    {	  
	      inc_x = CLAMP (inc_x,
			     -crop->tx1, gdisp->gimage->width - crop->tx2);
	      inc_y = CLAMP (inc_y,
			     -crop->ty1, gdisp->gimage->height - crop->ty2);
	    }
	  crop->tx1 += inc_x;
	  crop->tx2 += inc_x;
	  crop->ty1 += inc_y;
	  crop->ty2 += inc_y;
	}

      crop_recalc (tool, crop);

      gimp_draw_tool_resume (draw);
    }
}

static void
gimp_crop_tool_modifier_key (GimpTool    *tool,
			     GdkEventKey *kevent,
			     GDisplay    *gdisp)
{
  switch (kevent->keyval)
    {
    case GDK_Alt_L: case GDK_Alt_R:
      gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON (crop_options->allow_enlarge_w),
	 !crop_options->allow_enlarge);
      break;
    case GDK_Shift_L: case GDK_Shift_R:
      break;
    case GDK_Control_L: case GDK_Control_R:
      if (crop_options->type == CROP_CROP)
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (crop_options->type_w[RESIZE_CROP]), TRUE);
      else
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (crop_options->type_w[CROP_CROP]), TRUE);
      break;
    }
}

void
gimp_crop_tool_draw (GimpDrawTool *draw)
{
  GimpCropTool *crop;
  GimpTool     *tool;

  crop = GIMP_CROP_TOOL (draw);
  tool = GIMP_TOOL (draw);
  
#define SRW 10
#define SRH 10

  gdk_draw_line (draw->win, draw->gc,crop->x1, crop->y1, 
                 tool->gdisp->disp_width, crop->y1);
  gdk_draw_line (draw->win, draw->gc,
		 crop->x1, crop->y1, crop->x1, tool->gdisp->disp_height);
  gdk_draw_line (draw->win, draw->gc,
		 crop->x2, crop->y2, 0, crop->y2);
  gdk_draw_line (draw->win, draw->gc,
		 crop->x2, crop->y2, crop->x2, 0);

  crop->srw = ((crop->x2 - crop->x1) < SRW) ? (crop->x2 - crop->x1) : SRW;
  crop->srh = ((crop->y2 - crop->y1) < SRH) ? (crop->y2 - crop->y1) : SRH;

  gdk_draw_rectangle (draw->win, draw->gc, 1,
		      crop->x1, crop->y1, crop->srw, crop->srh);
  gdk_draw_rectangle (draw->win, draw->gc, 1,
		      crop->x2 - crop->srw, crop->y2-crop->srh, crop->srw, crop->srh);
  gdk_draw_rectangle (draw->win, draw->gc, 1,
		      crop->x2 - crop->srw, crop->y1, crop->srw, crop->srh);
  gdk_draw_rectangle (draw->win, draw->gc, 1,
		      crop->x1, crop->y2-crop->srh, crop->srw, crop->srh);

#undef SRW
#undef SRH

  crop_info_update (tool);
}

static void
crop_tool_crop_image (GimpImage *gimage,
		      gint       x1,
		      gint       y1,
		      gint       x2,
		      gint       y2,
		      gboolean   layer_only,
		      gboolean   crop_layers)
{
  if (!(x2 - x1) || !(y2 - y1))
    return;

  gimp_image_crop (gimage,
		   x1, y1, x2, y2,
		   layer_only,
		   crop_layers);

  gdisplays_update_full (gimage);
  gdisplays_shrink_wrap (gimage);

  gdisplays_flush ();
}

static void
crop_recalc (GimpTool     *tool,
	     GimpCropTool *crop)
{
  gdisplay_transform_coords (tool->gdisp, crop->tx1, crop->ty1,
			     &crop->x1, &crop->y1, FALSE);
  gdisplay_transform_coords (tool->gdisp, crop->tx2, crop->ty2,
			     &crop->x2, &crop->y2, FALSE);
}

static void
crop_start (GimpTool     *tool,
	    GimpCropTool *crop)
{
  static GDisplay *old_gdisp = NULL;
  GimpDrawTool *draw;

  draw = GIMP_DRAW_TOOL (tool);
  crop_recalc (tool, crop);

  if (! crop_info)
    crop_info_create (tool);

  g_signal_handlers_block_by_func (G_OBJECT (origin_sizeentry), 
                                   crop_origin_changed,
                                   crop_info);
  g_signal_handlers_block_by_func (G_OBJECT (size_sizeentry), 
                                   crop_size_changed,
                                   crop_info);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (origin_sizeentry), 0,
				  tool->gdisp->gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (origin_sizeentry), 1,
				  tool->gdisp->gimage->yresolution, FALSE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (origin_sizeentry), 0,
			    0, tool->gdisp->gimage->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (origin_sizeentry), 1,
			    0, tool->gdisp->gimage->height);
      
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size_sizeentry), 0,
				  tool->gdisp->gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size_sizeentry), 1,
				  tool->gdisp->gimage->yresolution, FALSE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (size_sizeentry), 0,
			    0, tool->gdisp->gimage->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (size_sizeentry), 1,
			    0, tool->gdisp->gimage->height);

  if (old_gdisp != tool->gdisp)
    {
      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (origin_sizeentry),
				tool->gdisp->gimage->unit) ;
      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (size_sizeentry),
				tool->gdisp->gimage->unit);

      if (tool->gdisp->dot_for_dot)
	{
	  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (origin_sizeentry),
				    GIMP_UNIT_PIXEL);
	  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (size_sizeentry),
				    GIMP_UNIT_PIXEL);
	}
    }

  g_signal_handlers_unblock_by_func (G_OBJECT (origin_sizeentry), 
                                     crop_origin_changed,
                                     crop_info);
  g_signal_handlers_unblock_by_func (G_OBJECT (size_sizeentry), 
                                     crop_size_changed,
                                     crop_info);

  old_gdisp = tool->gdisp;

  /* initialize the statusbar display */
  crop->context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (tool->gdisp->statusbar),
				  "crop");
  gtk_statusbar_push (GTK_STATUSBAR (tool->gdisp->statusbar),
		      crop->context_id, _("Crop: 0 x 0"));

  gimp_draw_tool_start(draw, tool->gdisp->canvas->window);
}


/***************************/
/*  Crop dialog functions  */
/***************************/

static void
crop_info_create (GimpTool *tool)
{
  GDisplay  *gdisp;
  GtkWidget *spinbutton;
  GtkWidget *bbox;
  GtkWidget *button;

  gdisp = (GDisplay *) tool->gdisp;

  /*  create the info dialog  */
  crop_info = info_dialog_new (_("Crop & Resize Information"),
			       tool_manager_help_func, NULL);

  /*  create the action area  */
  gimp_dialog_create_action_area (GIMP_DIALOG (crop_info->shell),

				  _("Crop"), crop_crop_callback,
				  NULL, NULL, NULL, TRUE, FALSE,
				  _("Resize"), crop_resize_callback,
				  NULL, NULL, NULL, FALSE, FALSE,
				  GTK_STOCK_CLOSE, crop_close_callback,
				  NULL, NULL, NULL, FALSE, FALSE,

				  NULL);

  /*  add the information fields  */
  spinbutton = info_dialog_add_spinbutton (crop_info, _("Origin X:"), NULL,
					    -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  origin_sizeentry =
    info_dialog_add_sizeentry (crop_info, _("Y:"), orig_vals, 1,
			       gdisp->dot_for_dot ? 
			       GIMP_UNIT_PIXEL : gdisp->gimage->unit, "%a",
			       TRUE, TRUE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
			       G_CALLBACK (crop_origin_changed),
			       crop_info);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (origin_sizeentry),
			     GTK_SPIN_BUTTON (spinbutton), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (origin_sizeentry), 0,
					 -65536, 65536);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (origin_sizeentry), 1,
					 -65536, 65536);

  spinbutton = info_dialog_add_spinbutton (crop_info, _("Width:"), NULL,
					    -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  size_sizeentry =
    info_dialog_add_sizeentry (crop_info, _("Height:"), size_vals, 1,
			       gdisp->dot_for_dot ? 
			       GIMP_UNIT_PIXEL : gdisp->gimage->unit, "%a",
			       TRUE, TRUE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
			       G_CALLBACK (crop_size_changed),
			       crop_info);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (size_sizeentry),
			     GTK_SPIN_BUTTON (spinbutton), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (size_sizeentry), 0,
					 -65536, 65536);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (size_sizeentry), 1,
					 -65536, 65536);

  gtk_table_set_row_spacing (GTK_TABLE (crop_info->info_table), 0, 0);
  gtk_table_set_row_spacing (GTK_TABLE (crop_info->info_table), 1, 6);
  gtk_table_set_row_spacing (GTK_TABLE (crop_info->info_table), 2, 0);

  /* Create the area selection buttons */
  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 4);

  button = gtk_button_new_with_label (_("From Selection"));
  gtk_container_add (GTK_CONTAINER(bbox), button);
  g_signal_connect (G_OBJECT (button) , "clicked",
                    G_CALLBACK (crop_selection_callback), 
                    NULL);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Auto Shrink"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  g_signal_connect (G_OBJECT (button) , "clicked",
                    G_CALLBACK (crop_automatic_callback),
                    NULL);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (crop_info->vbox), bbox, FALSE, FALSE, 2);
  gtk_widget_show (bbox);
}

static void
crop_info_update (GimpTool *tool)
{
  GimpCropTool *crop;

  crop = GIMP_CROP_TOOL(tool);

  orig_vals[0] = crop->tx1;
  orig_vals[1] = crop->ty1;
  size_vals[0] = crop->tx2 - crop->tx1;
  size_vals[1] = crop->ty2 - crop->ty1;

  info_dialog_update (crop_info);
  info_dialog_popup (crop_info);
}

static void
crop_crop_callback (GtkWidget *widget,
		    gpointer   data)
{
  GimpTool     *tool;
  GimpCropTool *crop;

  /* XXX active_tool is bad */
  tool = tool_manager_get_active (the_gimp);

  crop = GIMP_CROP_TOOL (tool);

  crop_tool_crop_image (tool->gdisp->gimage,
			crop->tx1, crop->ty1,
			crop->tx2, crop->ty2,
			crop_options->layer_only,
			TRUE);

  /*  Finish the tool  */
  crop_close_callback (NULL, NULL);
}

static void
crop_resize_callback (GtkWidget *widget,
		      gpointer   data)
{
  GimpTool     *tool;
  GimpCropTool *crop;
  
  /* XXX active_tool is bad */
  tool = tool_manager_get_active (the_gimp);

  crop = GIMP_CROP_TOOL (tool);

  crop_tool_crop_image (tool->gdisp->gimage,
			crop->tx1, crop->ty1,
			crop->tx2, crop->ty2, 
			crop_options->layer_only,
			FALSE);

  /*  Finish the tool  */
  crop_close_callback (NULL, NULL);
}

static void
crop_close_callback (GtkWidget *widget,
		     gpointer   data)
{
  GimpTool     *tool;
  GimpCropTool *crop;
  GimpDrawTool *draw;

  /* XXX active_tool is bad */
  tool = tool_manager_get_active (the_gimp);

  crop = GIMP_CROP_TOOL (tool);
  draw = GIMP_DRAW_TOOL (tool);
  
  if (tool->state == ACTIVE)
    gimp_draw_tool_stop (draw);

  info_dialog_popdown (crop_info);

  tool->gdisp    = NULL;
  tool->drawable = NULL;
  tool->state    = INACTIVE;
}

static void
crop_selection_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpTool     *tool;
  GimpCropTool *crop;
  GimpDrawTool *draw;
  GimpLayer    *layer;
  GDisplay     *gdisp;

  /* XXX active_tool is bad */
  tool = tool_manager_get_active (the_gimp);

  crop = GIMP_CROP_TOOL (tool);
  draw = GIMP_DRAW_TOOL (tool);
  
  gdisp = tool->gdisp;

  gimp_draw_tool_pause(draw);
  if (! gimage_mask_bounds (gdisp->gimage,
			    &crop->tx1, &crop->ty1, &crop->tx2, &crop->ty2))
    {
      if (crop_options->layer_only)
	{
	  layer = gimp_image_get_active_layer(gdisp->gimage);
	  gimp_drawable_offsets (GIMP_DRAWABLE (layer), &crop->tx1, &crop->ty1);
	  crop->tx2 = gimp_drawable_width  (GIMP_DRAWABLE (layer)) + crop->tx1;
	  crop->ty2 = gimp_drawable_height (GIMP_DRAWABLE (layer)) + crop->ty1;
	}
      else
	{
	  crop->tx1 = crop->ty1 = 0;
	  crop->tx2 = gdisp->gimage->width;
	  crop->ty2 = gdisp->gimage->height;
	}
    }
  crop_recalc (tool, crop);
  gimp_draw_tool_resume(draw);
}

static void
crop_automatic_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpTool        *tool;
  GimpCropTool    *crop;
  GimpDrawTool    *draw;
  GDisplay        *gdisp;
  GimpDrawable    *active_drawable;
  gint             offset_x, offset_y;
  gint             width, height;
  gint             x1, y1, x2, y2;
  gint             shrunk_x1;
  gint             shrunk_y1;
  gint             shrunk_x2;
  gint             shrunk_y2;
  
  /* FIXME: active_tool is bad */
  tool = tool_manager_get_active (the_gimp);

  crop  = GIMP_CROP_TOOL (tool);
  draw  = GIMP_DRAW_TOOL (tool);
  gdisp = tool->gdisp;

  if (crop_options->layer_only)
    {
      active_drawable = gimp_image_active_drawable (gdisp->gimage);

      if (! active_drawable)
	return;

      width  = gimp_drawable_width  (GIMP_DRAWABLE (active_drawable)); 
      height = gimp_drawable_height (GIMP_DRAWABLE (active_drawable));
      gimp_drawable_offsets (GIMP_DRAWABLE (active_drawable),
			     &offset_x, &offset_y);
    }
  else
    {
      width    = gdisp->gimage->width;
      height   = gdisp->gimage->height;
      offset_x = 0;
      offset_y = 0;
   }

  x1 = crop->tx1 - offset_x  > 0      ? crop->tx1 - offset_x : 0;
  x2 = crop->tx2 - offset_x  < width  ? crop->tx2 - offset_x : width;
  y1 = crop->ty1 - offset_y  > 0      ? crop->ty1 - offset_y : 0;
  y2 = crop->ty2 - offset_y  < height ? crop->ty2 - offset_y : height;

  gimp_draw_tool_pause (draw);

  if (gimp_image_crop_auto_shrink (gdisp->gimage,
				   x1, y1, x2, y2,
				   crop_options->layer_only,
				   &shrunk_x1,
				   &shrunk_y1,
				   &shrunk_x2,
				   &shrunk_y2))
    {
      crop->tx1 = offset_x + shrunk_x1;
      crop->tx2 = offset_x + shrunk_x2;
      crop->ty1 = offset_y + shrunk_y1;
      crop->ty2 = offset_y + shrunk_y2;

      crop_recalc (tool, crop);
    }

  gimp_draw_tool_resume (draw);
}

static void
crop_origin_changed (GtkWidget *widget,
                     gpointer   data)
{
  GimpTool     *tool;
  GimpCropTool *crop;
  GimpDrawTool *draw;
  gint          ox;
  gint          oy;

  /* XXX active_tool is bad */
  tool = tool_manager_get_active (the_gimp);

  if (tool && GIMP_IS_CROP_TOOL(tool))
    {
      crop = GIMP_CROP_TOOL(tool);
      draw = GIMP_DRAW_TOOL(tool);
      
      ox = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0));
      oy = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1));

      if ((ox != crop->tx1) ||
	  (oy != crop->ty1))
	{
	  gimp_draw_tool_pause(draw);
	  crop->tx2 = crop->tx2 + (ox - crop->tx1);
	  crop->tx1 = ox;
	  crop->ty2 = crop->ty2 + (oy - crop->ty1);
	  crop->ty1 = oy;
	  crop_recalc (tool, crop);
	  gimp_draw_tool_resume(draw);
	}
    }
}

static void
crop_size_changed (GtkWidget *widget,
		   gpointer   data)
{
  GimpTool     *tool;
  GimpCropTool *crop;
  GimpDrawTool *draw;

  gint          sx;
  gint          sy;

  /* XXX active_tool is bad */
  tool = tool_manager_get_active (the_gimp);

  if (tool && GIMP_IS_CROP_TOOL (tool))
    {
      crop = GIMP_CROP_TOOL (tool);
      draw = GIMP_DRAW_TOOL (tool);

      sx = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
      sy = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

      if ((sx != (crop->tx2 - crop->tx1)) ||
	  (sy != (crop->ty2 - crop->ty1)))
	{
	  gimp_draw_tool_pause (draw);
	  crop->tx2 = sx + crop->tx1;
	  crop->ty2 = sy + crop->ty1;
	  crop_recalc (tool, crop);
	  gimp_draw_tool_resume (draw);
	}
    }
}


/*  tool options stuff  */

static CropOptions *
crop_options_new (void)
{
  CropOptions *options;
  GtkWidget   *vbox;
  GtkWidget   *frame;

  options = g_new0 (CropOptions, 1);
  tool_options_init ((GimpToolOptions *) options,
		     crop_options_reset);

  options->layer_only    = options->layer_only_d    = FALSE;
  options->allow_enlarge = options->allow_enlarge_d = FALSE;
  options->type          = options->type_d          = CROP_CROP;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  layer toggle  */
  options->layer_only_w =
    gtk_check_button_new_with_label(_("Current Layer only"));
  gtk_box_pack_start (GTK_BOX (vbox), options->layer_only_w,
		      FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (options->layer_only_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->layer_only);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->layer_only_w),
				options->layer_only_d);
  gtk_widget_show (options->layer_only_w);

  /*  enlarge toggle  */
  options->allow_enlarge_w = gtk_check_button_new_with_label (_("Allow Enlarging"));
  gtk_box_pack_start (GTK_BOX (vbox), options->allow_enlarge_w,
		      FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (options->allow_enlarge_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->allow_enlarge);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_enlarge_w),
				options->allow_enlarge_d);
  gtk_widget_show (options->allow_enlarge_w);

  /*  tool toggle  */
  frame = gimp_radio_group_new2 (TRUE, _("Tool Toggle"),
				 G_CALLBACK (gimp_radio_button_update),
				 &options->type, (gpointer) options->type,

				 _("Crop"), (gpointer) CROP_CROP,
				 &options->type_w[0],
				 _("Resize"), (gpointer) RESIZE_CROP,
				 &options->type_w[1],

				 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return options;
}

static void
crop_options_reset (GimpToolOptions *tool_options)
{
  CropOptions *options;

  options = (CropOptions *) tool_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->layer_only_w),
				options->layer_only_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(options->allow_enlarge_w),
				options->allow_enlarge_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE); 
}
