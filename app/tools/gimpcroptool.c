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

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimpimage-mask.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "gui/info-dialog.h"

#include "widgets/gimpenummenu.h"
#include "widgets/gimpviewabledialog.h"

#include "gimpcroptool.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "app_procs.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


/*  speed of key movement  */
#define ARROW_VELOCITY 25

/*  possible crop functions  */
enum
{
  CREATING,
  MOVING,
  RESIZING_LEFT,
  RESIZING_RIGHT,
  CROPPING
};


typedef struct _CropOptions CropOptions;

struct _CropOptions
{
  GimpToolOptions  tool_options;

  gboolean      layer_only;
  gboolean      layer_only_d;
  GtkWidget    *layer_only_w;

  gboolean      allow_enlarge;
  gboolean      allow_enlarge_d;
  GtkWidget    *allow_enlarge_w;

  GimpCropType  type;
  GimpCropType  type_d;
  GtkWidget    *type_w;
};


static void   gimp_crop_tool_class_init     (GimpCropToolClass *klass);
static void   gimp_crop_tool_init           (GimpCropTool      *crop_tool);

static void   gimp_crop_tool_finalize       (GObject         *object);

static void   gimp_crop_tool_control        (GimpTool        *tool,
					     GimpToolAction   action,
					     GimpDisplay     *gdisp);
static void   gimp_crop_tool_button_press   (GimpTool        *tool,
                                             GimpCoords      *coords,
                                             guint32          time,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);
static void   gimp_crop_tool_button_release (GimpTool        *tool,
                                             GimpCoords      *coords,
                                             guint32          time,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);
static void   gimp_crop_tool_motion         (GimpTool        *tool,
                                             GimpCoords      *coords,
                                             guint32          time,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);
static void   gimp_crop_tool_arrow_key      (GimpTool        *tool,
					     GdkEventKey     *kevent,
					     GimpDisplay     *gdisp);
static void   gimp_crop_tool_modifier_key   (GimpTool        *tool,
                                             GdkModifierType  key,
                                             gboolean         press,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);
static void   gimp_crop_tool_cursor_update  (GimpTool        *tool,
                                             GimpCoords      *coords,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);

static void   gimp_crop_tool_draw           (GimpDrawTool    *draw_tool);

/*  Crop helper functions  */
static void   crop_tool_crop_image          (GimpImage       *gimage,
					     gint             x1,
					     gint             y1,
					     gint             x2,
					     gint             y2,
					     gboolean         layer_only,
					     gboolean         crop_layers);

static void   crop_recalc                   (GimpCropTool    *crop);
static void   crop_start                    (GimpCropTool    *crop);

/*  Crop dialog functions  */
static void   crop_info_update              (GimpCropTool    *crop);
static void   crop_info_create              (GimpCropTool    *crop);
static void   crop_crop_callback            (GtkWidget       *widget,
					     GimpCropTool    *crop);
static void   crop_resize_callback          (GtkWidget       *widget,
					     GimpCropTool    *crop);
static void   crop_close_callback           (GtkWidget       *widget,
					     GimpCropTool    *crop);

static void   crop_selection_callback       (GtkWidget       *widget,
					     GimpCropTool    *crop);
static void   crop_automatic_callback       (GtkWidget       *widget,
					     GimpCropTool    *crop);

static void   crop_origin_changed           (GtkWidget       *widget,
					     GimpCropTool    *crop);
static void   crop_size_changed             (GtkWidget       *widget,
					     GimpCropTool    *crop);

static GimpToolOptions * crop_options_new   (GimpToolInfo    *tool_info);
static void              crop_options_reset (GimpToolOptions *tool_options);


static GimpDrawToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_crop_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_CROP_TOOL,
                crop_options_new,
                FALSE,
                "gimp-crop-tool",
                _("Crop & Resize"),
                _("Crop or Resize an image"),
                N_("/Tools/Transform Tools/Crop & Resize"), "<shift>C",
                NULL, "tools/crop_tool.html",
                GIMP_STOCK_TOOL_CROP,
                data);
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
  tool_class->arrow_key      = gimp_crop_tool_arrow_key;
  tool_class->modifier_key   = gimp_crop_tool_modifier_key;
  tool_class->cursor_update  = gimp_crop_tool_cursor_update;

  draw_tool_class->draw      = gimp_crop_tool_draw;
}

static void
gimp_crop_tool_init (GimpCropTool *crop_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (crop_tool);

  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_CROP_TOOL_CURSOR);
}

static void
gimp_crop_tool_finalize (GObject *object)
{
  GimpCropTool *crop;

  crop = GIMP_CROP_TOOL (object);

  if (crop->crop_info)
    {
      info_dialog_free (crop->crop_info);
      crop->crop_info = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_crop_tool_control (GimpTool       *tool,
			GimpToolAction  action,
			GimpDisplay    *gdisp)
{
  GimpCropTool *crop;

  crop = GIMP_CROP_TOOL (tool);
  
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      crop_recalc (crop);
      break;

    case HALT:
      crop_close_callback (NULL, crop);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_crop_tool_button_press (GimpTool        *tool,
                             GimpCoords      *coords,
                             guint32          time,
			     GdkModifierType  state,
			     GimpDisplay     *gdisp)
{
  GimpCropTool *crop;
  GimpDrawTool *draw_tool;
  
  crop      = GIMP_CROP_TOOL (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);

  if (!gimp_tool_control_is_active (tool->control) || gdisp != tool->gdisp)
    {
      crop->function = CREATING;
    }
  else
    {
      /*  If the cursor is in either the upper left or lower right boxes,
       *  The new function will be to resize the current crop area
       */
      if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                    coords->x, coords->y,
                                    GIMP_HANDLE_SQUARE,
                                    crop->x1, crop->y1,
                                    crop->dcw, crop->dch,
                                    GTK_ANCHOR_NORTH_WEST,
                                    FALSE))
        {
          crop->function = RESIZING_LEFT;
        }
      else if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                         coords->x, coords->y,
                                         GIMP_HANDLE_SQUARE,
                                         crop->x2, crop->y2,
                                         crop->dcw, crop->dch,
                                         GTK_ANCHOR_SOUTH_EAST,
                                         FALSE))
        {
          crop->function = RESIZING_RIGHT;
        }
      /*  If the cursor is in either the upper right or lower left boxes,
       *  The new function will be to translate the current crop area
       */
      else if  (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                          coords->x, coords->y,
                                          GIMP_HANDLE_SQUARE,
                                          crop->x2, crop->y1,
                                          crop->dcw, crop->dch,
                                          GTK_ANCHOR_NORTH_EAST,
                                          FALSE) ||
                gimp_draw_tool_on_handle (draw_tool, gdisp,
                                          coords->x, coords->y,
                                          GIMP_HANDLE_SQUARE,
                                          crop->x1, crop->y2,
                                          crop->dcw, crop->dch,
                                          GTK_ANCHOR_SOUTH_WEST,
                                          FALSE))
        {
          crop->function = MOVING;
        }
      /*  If the pointer is in the rectangular region, crop or resize it!
       */
      else if (coords->x > crop->x1 &&
               coords->x < crop->x2 &&
	       coords->y > crop->y1 &&
               coords->y < crop->y2)
        {
          crop->function = CROPPING;
        }
      /*  otherwise, the new function will be creating, since we want
       *  to start a new
       */
      else
        {
          crop->function = CREATING;
        }
    }

  if (crop->function == CREATING)
    {
      if (gimp_tool_control_is_active (tool->control))
	gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

      tool->gdisp = gdisp;

      crop->x2 = crop->x1 = ROUND (coords->x);
      crop->y2 = crop->y1 = ROUND (coords->y);

      crop_start (crop);
    }

  crop->lastx = crop->startx = ROUND (coords->x);
  crop->lasty = crop->starty = ROUND (coords->y);

  gimp_tool_control_activate (tool->control);
}

static void
gimp_crop_tool_button_release (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
			       GdkModifierType  state,
			       GimpDisplay     *gdisp)
{
  GimpCropTool *crop;
  CropOptions  *options;

  crop = GIMP_CROP_TOOL (tool);

  options = (CropOptions *) tool->tool_info->tool_options;

  gimp_tool_pop_status (tool);

  if (! (state & GDK_BUTTON3_MASK))
    {
      if (crop->function == CROPPING)
	{
	  if (options->type == GIMP_CROP)
	    crop_tool_crop_image (gdisp->gimage,
				  crop->x1, crop->y1,
                                  crop->x2, crop->y2, 
				  options->layer_only,
                                  TRUE);
	  else
	    crop_tool_crop_image (gdisp->gimage,
				  crop->x1, crop->y1,
                                  crop->x2, crop->y2, 
				  options->layer_only,
                                  FALSE);

	  /*  Finish the tool  */
	  crop_close_callback (NULL, crop);
	}
      else
        {
          crop_info_update (crop);
        }
    }
}

static void
gimp_crop_tool_motion (GimpTool        *tool,
                       GimpCoords      *coords,
                       guint32          time,
		       GdkModifierType  state,
		       GimpDisplay     *gdisp)
{
  GimpCropTool     *crop;
  CropOptions      *options;
  GimpLayer        *layer;
  gint              x1, y1, x2, y2;
  gint              curx, cury;
  gint              inc_x, inc_y;
  gint              min_x, min_y, max_x, max_y; 

  crop = GIMP_CROP_TOOL (tool);

  options = (CropOptions *) tool->tool_info->tool_options;

  /*  This is the only case when the motion events should be ignored--
      we're just waiting for the button release event to crop the image  */
  if (crop->function == CROPPING)
    return;

  curx = ROUND (coords->x);
  cury = ROUND (coords->y);

  x1 = crop->startx;
  y1 = crop->starty;
  x2 = curx;
  y2 = cury;

  inc_x = (x2 - x1);
  inc_y = (y2 - y1);

  /*  If there have been no changes... return  */
  if (crop->lastx == x2 && crop->lasty == y2)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (options->layer_only)
    {
      layer = gimp_image_get_active_layer (gdisp->gimage);
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
    case CREATING:
      if (! options->allow_enlarge)
	{
	  x1 = CLAMP (x1, min_x, max_x);
	  y1 = CLAMP (y1, min_y, max_y);
	  x2 = CLAMP (x2, min_x, max_x);
	  y2 = CLAMP (y2, min_y, max_y);
	}
      break;

    case RESIZING_LEFT:
      x1 = crop->x1 + inc_x;
      y1 = crop->y1 + inc_y;
      if (! options->allow_enlarge)
	{
	  x1 = CLAMP (x1, min_x, max_x);
	  y1 = CLAMP (y1, min_y, max_y);
	}
      x2 = MAX (x1, crop->x2);
      y2 = MAX (y1, crop->y2);
      crop->startx = curx;
      crop->starty = cury;
      break;

    case RESIZING_RIGHT:
      x2 = crop->x2 + inc_x;
      y2 = crop->y2 + inc_y;
      if (! options->allow_enlarge)
	{
	  x2 = CLAMP (x2, min_x, max_x);
	  y2 = CLAMP (y2, min_y, max_y);
	}
      x1 = MIN (crop->x1, x2);
      y1 = MIN (crop->y1, y2);
      crop->startx = curx;
      crop->starty = cury;
      break;

    case MOVING:
      if (! options->allow_enlarge)
	{
	  inc_x = CLAMP (inc_x, min_x - crop->x1, max_x - crop->x2);
	  inc_y = CLAMP (inc_y, min_y - crop->y1, max_y - crop->y2);
	}
      x1 = crop->x1 + inc_x;
      x2 = crop->x2 + inc_x;
      y1 = crop->y1 + inc_y;
      y2 = crop->y2 + inc_y;
      crop->startx = curx;
      crop->starty = cury;
      break;
    }

  /*  make sure that the coords are in bounds  */
  crop->x1 = MIN (x1, x2);
  crop->y1 = MIN (y1, y2);
  crop->x2 = MAX (x1, x2);
  crop->y2 = MAX (y1, y2);

  crop->lastx = curx;
  crop->lasty = cury;

  /*  recalculate the coordinates for crop_draw based on the new values  */
  crop_recalc (crop);

  if (crop->function == CREATING      || 
      crop->function == RESIZING_LEFT ||
      crop->function == RESIZING_RIGHT)
    {
      gimp_tool_pop_status (tool);

      gimp_tool_push_status_coords (tool,
                                    _("Crop: "),
                                    crop->x2 - crop->x1,
                                    " x ",
                                    crop->y2 - crop->y1);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_crop_tool_arrow_key (GimpTool    *tool,
			  GdkEventKey *kevent,
			  GimpDisplay *gdisp)
{
  GimpCropTool *crop;
  CropOptions  *options;
  GimpLayer    *layer;
  gint          inc_x, inc_y;
  gint          min_x, min_y;
  gint          max_x, max_y;

  crop = GIMP_CROP_TOOL (tool);

  options = (CropOptions *) tool->tool_info->tool_options;

  if (gimp_tool_control_is_active (tool->control))
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

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      if (options->layer_only)
	{
	  layer = gimp_image_get_active_layer (gdisp->gimage);
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
	  crop->x2 = crop->x2 + inc_x;
	  crop->y2 = crop->y2 + inc_y;
	  if (! options->allow_enlarge)
	    {
	      crop->x2 = CLAMP (crop->x2, min_x, max_x);
	      crop->y2 = CLAMP (crop->y2, min_y, max_y);
	    }
	  crop->x1 = MIN (crop->x1, crop->x2);
	  crop->y1 = MIN (crop->y1, crop->y2);
	}
      else
	{
	  if (! options->allow_enlarge)
	    {	  
	      inc_x = CLAMP (inc_x,
			     -crop->x1, gdisp->gimage->width - crop->x2);
	      inc_y = CLAMP (inc_y,
			     -crop->y1, gdisp->gimage->height - crop->y2);
	    }
	  crop->x1 += inc_x;
	  crop->x2 += inc_x;
	  crop->y1 += inc_y;
	  crop->y2 += inc_y;
	}

      crop_recalc (crop);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_crop_tool_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
			     GdkModifierType  state,
			     GimpDisplay     *gdisp)
{
  CropOptions *options;

  options = (CropOptions *) tool->tool_info->tool_options;

  if (state & GDK_MOD1_MASK)
    {
      if (! options->allow_enlarge)
        {
          gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON (options->allow_enlarge_w), TRUE);
        }
    }
  else
    {
      if (options->allow_enlarge)
        {
          gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON (options->allow_enlarge_w), FALSE);
        }
    }

  if (key == GDK_CONTROL_MASK)
    {
      switch (options->type)
        {
        case GIMP_CROP:
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                                       GINT_TO_POINTER (GIMP_RESIZE));
          break;
        case GIMP_RESIZE:
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                                       GINT_TO_POINTER (GIMP_CROP));
          break;
        default:
          break;
        }
    }
}

static void
gimp_crop_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
			      GdkModifierType  state,
			      GimpDisplay     *gdisp)
{
  GimpCropTool      *crop;
  GimpDrawTool      *draw_tool;
  CropOptions       *options;
  GdkCursorType      ctype     = GIMP_MOUSE_CURSOR;
  GimpCursorModifier cmodifier = GIMP_CURSOR_MODIFIER_NONE;

  crop      = GIMP_CROP_TOOL (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);

  options = (CropOptions *) tool->tool_info->tool_options;

  if (!gimp_tool_control_is_active (tool->control) ||
      (gimp_tool_control_is_active (tool->control) && tool->gdisp != gdisp))
      /* this expression can be simplified to !..._is_active () || t->g != g */
    {
      ctype = GIMP_CROSSHAIR_SMALL_CURSOR;
    }
  else if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     crop->x1, crop->y1,
                                     crop->dcw, crop->dch,
                                     GTK_ANCHOR_NORTH_WEST,
                                     FALSE) ||
           gimp_draw_tool_on_handle (draw_tool, gdisp,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     crop->x2, crop->y2,
                                     crop->dcw, crop->dch,
                                     GTK_ANCHOR_SOUTH_EAST,
                                     FALSE))
    {
      cmodifier = GIMP_CURSOR_MODIFIER_RESIZE;
    }
  else if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     crop->x2, crop->y1,
                                     crop->dcw, crop->dch,
                                     GTK_ANCHOR_NORTH_EAST,
                                     FALSE) ||
           gimp_draw_tool_on_handle (draw_tool, gdisp,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     crop->x1, crop->y2,
                                     crop->dcw, crop->dch,
                                     GTK_ANCHOR_SOUTH_WEST,
                                     FALSE))
    {
      cmodifier = GIMP_CURSOR_MODIFIER_MOVE;
    }
  else if (! (coords->x > crop->x1 && coords->x < crop->x2 &&
	      coords->y > crop->y1 && coords->y < crop->y2))
    {
      ctype = GIMP_CROSSHAIR_SMALL_CURSOR;
    }

  gimp_tool_control_set_cursor (tool->control, ctype);
  gimp_tool_control_set_tool_cursor (tool->control, (options->type == GIMP_CROP ? 
                                                     GIMP_CROP_TOOL_CURSOR : 
                                                     GIMP_RESIZE_TOOL_CURSOR));
  gimp_tool_control_set_cursor_modifier (tool->control, cmodifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

void
gimp_crop_tool_draw (GimpDrawTool *draw)
{
  GimpCropTool     *crop;
  GimpTool         *tool;
  GimpDisplayShell *shell;

  crop = GIMP_CROP_TOOL (draw);
  tool = GIMP_TOOL (draw);

  shell = GIMP_DISPLAY_SHELL (tool->gdisp->shell);

  gdk_draw_line (draw->win, draw->gc,
                 crop->dx1, crop->dy1, 
                 shell->disp_width, crop->dy1);
  gdk_draw_line (draw->win, draw->gc,
		 crop->dx1, crop->dy1,
                 crop->dx1, shell->disp_height);
  gdk_draw_line (draw->win, draw->gc,
		 crop->dx2, crop->dy2,
                 0, crop->dy2);
  gdk_draw_line (draw->win, draw->gc,
		 crop->dx2, crop->dy2,
                 crop->dx2, 0);

  gimp_draw_tool_draw_handle (draw,
                              GIMP_HANDLE_FILLED_SQUARE,
                              crop->x1, crop->y1,
                              crop->dcw, crop->dch,
                              GTK_ANCHOR_NORTH_WEST,
                              FALSE);
  gimp_draw_tool_draw_handle (draw,
                              GIMP_HANDLE_FILLED_SQUARE,
                              crop->x2, crop->y1,
                              crop->dcw, crop->dch,
                              GTK_ANCHOR_NORTH_EAST,
                              FALSE);
  gimp_draw_tool_draw_handle (draw,
                              GIMP_HANDLE_FILLED_SQUARE,
                              crop->x1, crop->y2,
                              crop->dcw, crop->dch,
                              GTK_ANCHOR_SOUTH_WEST,
                              FALSE);
  gimp_draw_tool_draw_handle (draw,
                              GIMP_HANDLE_FILLED_SQUARE,
                              crop->x2, crop->y2,
                              crop->dcw, crop->dch,
                              GTK_ANCHOR_SOUTH_EAST,
                              FALSE);

  crop_info_update (crop);
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
}

static void
crop_recalc (GimpCropTool *crop)
{
  GimpTool *tool;

  tool = GIMP_TOOL (crop);

  gimp_display_shell_transform_xy (GIMP_DISPLAY_SHELL (tool->gdisp->shell),
                                   crop->x1, crop->y1,
                                   &crop->dx1, &crop->dy1,
                                   FALSE);
  gimp_display_shell_transform_xy (GIMP_DISPLAY_SHELL (tool->gdisp->shell),
                                   crop->x2, crop->y2,
                                   &crop->dx2, &crop->dy2,
                                   FALSE);

#define SRW 10
#define SRH 10

  crop->dcw = ((crop->dx2 - crop->dx1) < SRW) ? (crop->dx2 - crop->dx1) : SRW;
  crop->dch = ((crop->dy2 - crop->dy1) < SRH) ? (crop->dy2 - crop->dy1) : SRH;

#undef SRW
#undef SRH
}

static void
crop_start (GimpCropTool *crop)
{
  static GimpDisplay *old_gdisp = NULL;

  GimpTool *tool;

  tool = GIMP_TOOL (crop);

  crop_recalc (crop);

  if (! crop->crop_info)
    crop_info_create (crop);

  gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (crop->crop_info->shell),
                                     GIMP_VIEWABLE (tool->gdisp->gimage));

  g_signal_handlers_block_by_func (G_OBJECT (crop->origin_sizeentry), 
                                   crop_origin_changed,
                                   crop);
  g_signal_handlers_block_by_func (G_OBJECT (crop->size_sizeentry), 
                                   crop_size_changed,
                                   crop);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (crop->origin_sizeentry), 0,
				  tool->gdisp->gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (crop->origin_sizeentry), 1,
				  tool->gdisp->gimage->yresolution, FALSE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (crop->origin_sizeentry), 0,
			    0, tool->gdisp->gimage->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (crop->origin_sizeentry), 1,
			    0, tool->gdisp->gimage->height);
      
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (crop->size_sizeentry), 0,
				  tool->gdisp->gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (crop->size_sizeentry), 1,
				  tool->gdisp->gimage->yresolution, FALSE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (crop->size_sizeentry), 0,
			    0, tool->gdisp->gimage->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (crop->size_sizeentry), 1,
			    0, tool->gdisp->gimage->height);

  if (old_gdisp != tool->gdisp)
    {
      GimpDisplayShell *shell;

      shell = GIMP_DISPLAY_SHELL (tool->gdisp->shell);

      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (crop->origin_sizeentry),
				tool->gdisp->gimage->unit) ;
      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (crop->size_sizeentry),
				tool->gdisp->gimage->unit);

      if (shell->dot_for_dot)
	{
	  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (crop->origin_sizeentry),
				    GIMP_UNIT_PIXEL);
	  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (crop->size_sizeentry),
				    GIMP_UNIT_PIXEL);
	}
    }

  g_signal_handlers_unblock_by_func (G_OBJECT (crop->origin_sizeentry), 
                                     crop_origin_changed,
                                     crop);
  g_signal_handlers_unblock_by_func (G_OBJECT (crop->size_sizeentry), 
                                     crop_size_changed,
                                     crop);

  old_gdisp = tool->gdisp;

  /* initialize the statusbar display */
  gimp_tool_push_status (tool, _("Crop: 0 x 0"));

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), tool->gdisp);
}


/***************************/
/*  Crop dialog functions  */
/***************************/

static void
crop_info_create (GimpCropTool *crop)
{
  GimpTool         *tool;
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  GtkWidget        *spinbutton;
  GtkWidget        *bbox;
  GtkWidget        *button;

  tool = GIMP_TOOL (crop);

  gdisp = tool->gdisp;
  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  create the info dialog  */
  crop->crop_info = info_dialog_new (NULL,
                                     tool->tool_info->blurb,
                                     GIMP_OBJECT (tool->tool_info)->name,
                                     tool->tool_info->stock_id,
                                     _("Crop & Resize Information"),
                                     tool_manager_help_func, NULL);

  /*  create the action area  */
  gimp_dialog_create_action_area (GIMP_DIALOG (crop->crop_info->shell),

				  GTK_STOCK_CLOSE, crop_close_callback,
				  crop, NULL, NULL, FALSE, FALSE,

				  GIMP_STOCK_RESIZE, crop_resize_callback,
				  crop, NULL, NULL, FALSE, FALSE,

				  GIMP_STOCK_TOOL_CROP, crop_crop_callback,
				  crop, NULL, NULL, TRUE, FALSE,

				  NULL);

  /*  add the information fields  */
  spinbutton = info_dialog_add_spinbutton (crop->crop_info, _("Origin X:"), NULL,
					    -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  crop->origin_sizeentry =
    info_dialog_add_sizeentry (crop->crop_info, _("Y:"), crop->orig_vals, 1,
			       shell->dot_for_dot ? 
			       GIMP_UNIT_PIXEL : gdisp->gimage->unit, "%a",
			       TRUE, TRUE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
			       G_CALLBACK (crop_origin_changed),
			       crop);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (crop->origin_sizeentry),
			     GTK_SPIN_BUTTON (spinbutton), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (crop->origin_sizeentry),
                                         0, -65536, 65536);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (crop->origin_sizeentry),
                                         1, -65536, 65536);

  spinbutton = info_dialog_add_spinbutton (crop->crop_info, _("Width:"), NULL,
					    -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  crop->size_sizeentry =
    info_dialog_add_sizeentry (crop->crop_info, _("Height:"), crop->size_vals, 1,
			       shell->dot_for_dot ? 
			       GIMP_UNIT_PIXEL : gdisp->gimage->unit, "%a",
			       TRUE, TRUE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
			       G_CALLBACK (crop_size_changed),
			       crop);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (crop->size_sizeentry),
			     GTK_SPIN_BUTTON (spinbutton), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (crop->size_sizeentry),
                                         0, -65536, 65536);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (crop->size_sizeentry),
                                         1, -65536, 65536);

  gtk_table_set_row_spacing (GTK_TABLE (crop->crop_info->info_table), 0, 0);
  gtk_table_set_row_spacing (GTK_TABLE (crop->crop_info->info_table), 1, 6);
  gtk_table_set_row_spacing (GTK_TABLE (crop->crop_info->info_table), 2, 0);

  /* Create the area selection buttons */
  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
  gtk_box_set_spacing (GTK_BOX (bbox), 4);

  button = gtk_button_new_with_label (_("From Selection"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button) , "clicked",
                    G_CALLBACK (crop_selection_callback), 
                    crop);

  button = gtk_button_new_with_label (_("Auto Shrink"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button) , "clicked",
                    G_CALLBACK (crop_automatic_callback),
                    crop);

  gtk_box_pack_start (GTK_BOX (crop->crop_info->vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);
}

static void
crop_info_update (GimpCropTool *crop)
{
  crop->orig_vals[0] = crop->x1;
  crop->orig_vals[1] = crop->y1;
  crop->size_vals[0] = crop->x2 - crop->x1;
  crop->size_vals[1] = crop->y2 - crop->y1;

  info_dialog_update (crop->crop_info);
  info_dialog_popup (crop->crop_info);
}

static void
crop_crop_callback (GtkWidget    *widget,
		    GimpCropTool *crop)
{
  GimpTool    *tool;
  CropOptions *options;

  tool = GIMP_TOOL (crop);

  options = (CropOptions *) tool->tool_info->tool_options;

  crop_tool_crop_image (tool->gdisp->gimage,
                        crop->x1, crop->y1,
                        crop->x2, crop->y2,
                        options->layer_only,
                        TRUE);

  crop_close_callback (NULL, crop);
}

static void
crop_resize_callback (GtkWidget    *widget,
		      GimpCropTool *crop)
{
  GimpTool    *tool;
  CropOptions *options;

  tool = GIMP_TOOL (crop);

  options = (CropOptions *) tool->tool_info->tool_options;

  crop_tool_crop_image (tool->gdisp->gimage,
                        crop->x1, crop->y1,
                        crop->x2, crop->y2, 
                        options->layer_only,
                        FALSE);

  crop_close_callback (NULL, crop);
}

static void
crop_close_callback (GtkWidget    *widget,
		     GimpCropTool *crop)
{
  if (gimp_tool_control_is_active (GIMP_TOOL (crop)->control))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (crop));

  gimp_tool_control_halt (GIMP_TOOL (crop)->control);    /* sets paused_count to 0 -- is this ok? */

  if (crop->crop_info)
    info_dialog_popdown (crop->crop_info);
}

static void
crop_selection_callback (GtkWidget    *widget,
			 GimpCropTool *crop)
{
  CropOptions *options;
  GimpLayer   *layer;
  GimpDisplay *gdisp;

  options = (CropOptions *) GIMP_TOOL (crop)->tool_info->tool_options;
 
  gdisp = GIMP_TOOL (crop)->gdisp;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (crop));

  if (! gimp_image_mask_bounds (gdisp->gimage,
                                &crop->x1, &crop->y1,
                                &crop->x2, &crop->y2))
    {
      if (options->layer_only)
        {
          layer = gimp_image_get_active_layer (gdisp->gimage);
          gimp_drawable_offsets (GIMP_DRAWABLE (layer),
                                 &crop->x1, &crop->y1);
          crop->x2 = gimp_drawable_width  (GIMP_DRAWABLE (layer)) + crop->x1;
          crop->y2 = gimp_drawable_height (GIMP_DRAWABLE (layer)) + crop->y1;
        }
      else
        {
          crop->x1 = crop->y1 = 0;
          crop->x2 = gdisp->gimage->width;
          crop->y2 = gdisp->gimage->height;
        }
    }

  crop_recalc (crop);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (crop));
}

static void
crop_automatic_callback (GtkWidget    *widget,
			 GimpCropTool *crop)
{
  CropOptions  *options;
  GimpDisplay  *gdisp;
  GimpDrawable *active_drawable;
  gint          offset_x, offset_y;
  gint          width, height;
  gint          x1, y1, x2, y2;
  gint          shrunk_x1;
  gint          shrunk_y1;
  gint          shrunk_x2;
  gint          shrunk_y2;

  options = (CropOptions *) GIMP_TOOL (crop)->tool_info->tool_options;

  gdisp = GIMP_TOOL (crop)->gdisp;

  if (options->layer_only)
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

  x1 = crop->x1 - offset_x  > 0      ? crop->x1 - offset_x : 0;
  x2 = crop->x2 - offset_x  < width  ? crop->x2 - offset_x : width;
  y1 = crop->y1 - offset_y  > 0      ? crop->y1 - offset_y : 0;
  y2 = crop->y2 - offset_y  < height ? crop->y2 - offset_y : height;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (crop));

  if (gimp_image_crop_auto_shrink (gdisp->gimage,
                                   x1, y1, x2, y2,
                                   options->layer_only,
                                   &shrunk_x1,
                                   &shrunk_y1,
                                   &shrunk_x2,
                                   &shrunk_y2))
    {
      crop->x1 = offset_x + shrunk_x1;
      crop->x2 = offset_x + shrunk_x2;
      crop->y1 = offset_y + shrunk_y1;
      crop->y2 = offset_y + shrunk_y2;

      crop_recalc (crop);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (crop));
}

static void
crop_origin_changed (GtkWidget    *widget,
                     GimpCropTool *crop)
{
  gint ox;
  gint oy;

  ox = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0));
  oy = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1));

  if ((ox != crop->x1) ||
      (oy != crop->y1))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (crop));

      crop->x2 = crop->x2 + (ox - crop->x1);
      crop->x1 = ox;
      crop->y2 = crop->y2 + (oy - crop->y1);
      crop->y1 = oy;

      crop_recalc (crop);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (crop));
    }
}

static void
crop_size_changed (GtkWidget    *widget,
		   GimpCropTool *crop)
{
  gint sx;
  gint sy;

  sx = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  sy = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if ((sx != (crop->x2 - crop->x1)) ||
      (sy != (crop->y2 - crop->y1)))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (crop));

      crop->x2 = sx + crop->x1;
      crop->y2 = sy + crop->y1;

      crop_recalc (crop);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (crop));
    }
}


/*  tool options stuff  */

static GimpToolOptions *
crop_options_new (GimpToolInfo *tool_info)
{
  CropOptions *options;
  GtkWidget   *vbox;
  GtkWidget   *frame;

  options = g_new0 (CropOptions, 1);

  tool_options_init ((GimpToolOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = crop_options_reset;

  options->layer_only    = options->layer_only_d    = FALSE;
  options->allow_enlarge = options->allow_enlarge_d = FALSE;
  options->type          = options->type_d          = GIMP_CROP;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  tool toggle  */
  frame = gimp_enum_radio_frame_new (GIMP_TYPE_CROP_TYPE,
                                     gtk_label_new (_("Tool Toggle (<Ctrl>)")),
                                     2,
                                     G_CALLBACK (gimp_radio_button_update),
                                     &options->type,
                                     &options->type_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                               GINT_TO_POINTER (options->type));

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  layer toggle  */
  options->layer_only_w =
    gtk_check_button_new_with_label (_("Current Layer only"));
  gtk_box_pack_start (GTK_BOX (vbox), options->layer_only_w,
		      FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (options->layer_only_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->layer_only);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->layer_only_w),
				options->layer_only);
  gtk_widget_show (options->layer_only_w);

  /*  enlarge toggle  */
  options->allow_enlarge_w =
    gtk_check_button_new_with_label (_("Allow Enlarging (<Alt>)"));
  gtk_box_pack_start (GTK_BOX (vbox), options->allow_enlarge_w,
		      FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (options->allow_enlarge_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->allow_enlarge);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_enlarge_w),
				options->allow_enlarge);
  gtk_widget_show (options->allow_enlarge_w);

  return (GimpToolOptions *) options;
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

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                               GINT_TO_POINTER (options->type_d));
}
