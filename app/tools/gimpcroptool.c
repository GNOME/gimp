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

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpenumwidgets.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvas.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#ifdef __GNUC__
#warning FIXME #include "dialogs/dialogs-types.h"
#endif
#include "dialogs/dialogs-types.h"
#include "dialogs/info-dialog.h"

#include "gimpcropoptions.h"
#include "gimpcroptool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


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


static void     gimp_crop_tool_class_init     (GimpCropToolClass *klass);
static void     gimp_crop_tool_init           (GimpCropTool      *crop_tool);

static void     gimp_crop_tool_finalize       (GObject         *object);

static void     gimp_crop_tool_control        (GimpTool        *tool,
                                               GimpToolAction   action,
                                               GimpDisplay     *gdisp);
static void     gimp_crop_tool_button_press   (GimpTool        *tool,
                                               GimpCoords      *coords,
                                               guint32          time,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);
static void     gimp_crop_tool_button_release (GimpTool        *tool,
                                               GimpCoords      *coords,
                                               guint32          time,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);
static void     gimp_crop_tool_motion         (GimpTool        *tool,
                                               GimpCoords      *coords,
                                               guint32          time,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);
static gboolean gimp_crop_tool_key_press      (GimpTool        *tool,
                                               GdkEventKey     *kevent,
                                               GimpDisplay     *gdisp);
static void     gimp_crop_tool_modifier_key   (GimpTool        *tool,
                                               GdkModifierType  key,
                                               gboolean         press,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);
static void     gimp_crop_tool_oper_update    (GimpTool        *tool,
                                               GimpCoords      *coords,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);
static void     gimp_crop_tool_cursor_update  (GimpTool        *tool,
                                               GimpCoords      *coords,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);

static void     gimp_crop_tool_draw           (GimpDrawTool    *draw_tool);

/*  Crop helper functions  */
static void     crop_tool_crop_image          (GimpImage       *gimage,
                                               GimpContext     *context,
                                               gint             x1,
                                               gint             y1,
                                               gint             x2,
                                               gint             y2,
                                               gboolean         layer_only,
                                               GimpCropMode     crop_mode);

static void     crop_recalc                   (GimpCropTool    *crop,
                                               gboolean         recalc_highlight);
static void     crop_start                    (GimpCropTool    *crop,
                                               gboolean         dialog);

/*  Crop dialog functions  */
static void     crop_info_update              (GimpCropTool    *crop);
static void     crop_info_create              (GimpCropTool    *crop);
static void     crop_response                 (GtkWidget       *widget,
                                               gint             response_id,
                                               GimpCropTool    *crop);

static void     crop_selection_callback       (GtkWidget       *widget,
                                               GimpCropTool    *crop);
static void     crop_automatic_callback       (GtkWidget       *widget,
                                               GimpCropTool    *crop);

static void     crop_origin_changed           (GtkWidget       *widget,
                                               GimpCropTool    *crop);
static void     crop_size_changed             (GtkWidget       *widget,
                                               GimpCropTool    *crop);
static void     crop_aspect_changed           (GtkWidget       *widget,
                                               GimpCropTool    *crop);


static GimpDrawToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_crop_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_CROP_TOOL,
                GIMP_TYPE_CROP_OPTIONS,
                gimp_crop_options_gui,
                0,
                "gimp-crop-tool",
                _("Crop & Resize"),
                _("Crop or Resize an image"),
                N_("_Crop & Resize"), "<shift>C",
                NULL, GIMP_HELP_TOOL_CROP,
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
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_crop_tool_finalize;

  tool_class->control        = gimp_crop_tool_control;
  tool_class->button_press   = gimp_crop_tool_button_press;
  tool_class->button_release = gimp_crop_tool_button_release;
  tool_class->motion         = gimp_crop_tool_motion;
  tool_class->key_press      = gimp_crop_tool_key_press;
  tool_class->modifier_key   = gimp_crop_tool_modifier_key;
  tool_class->oper_update    = gimp_crop_tool_oper_update;
  tool_class->cursor_update  = gimp_crop_tool_cursor_update;

  draw_tool_class->draw      = gimp_crop_tool_draw;
}

static void
gimp_crop_tool_init (GimpCropTool *crop_tool)
{
  GimpTool *tool = GIMP_TOOL (crop_tool);

  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control, GIMP_DIRTY_IMAGE_SIZE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_CROP);
}

static void
gimp_crop_tool_finalize (GObject *object)
{
  GimpCropTool *crop = GIMP_CROP_TOOL (object);

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
  GimpCropTool *crop = GIMP_CROP_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      crop_recalc (crop, FALSE);
      break;

    case HALT:
      crop_response (NULL, GTK_RESPONSE_CANCEL, crop);
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
  GimpCropTool *crop      = GIMP_CROP_TOOL (tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (gdisp != tool->gdisp)
    {
      if (gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_stop (draw_tool);

      crop->function = CREATING;
      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);

      tool->gdisp = gdisp;

      crop->x2 = crop->x1 = ROUND (coords->x);
      crop->y2 = crop->y1 = ROUND (coords->y);

      crop_start (crop, (state & GDK_SHIFT_MASK) == 0);
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
  GimpCropTool    *crop    = GIMP_CROP_TOOL (tool);
  GimpCropOptions *options = GIMP_CROP_OPTIONS (tool->tool_info->tool_options);

  gimp_tool_control_halt (tool->control);
  gimp_tool_pop_status (tool);

  if (! (state & GDK_BUTTON3_MASK))
    {
      if (crop->function == CROPPING)
        crop_response (NULL, options->crop_mode, crop);
      else if (crop->crop_info)
        crop_info_update (crop);
    }
}

static void
gimp_crop_tool_motion (GimpTool        *tool,
                       GimpCoords      *coords,
                       guint32          time,
                       GdkModifierType  state,
                       GimpDisplay     *gdisp)
{
  GimpCropTool    *crop    = GIMP_CROP_TOOL (tool);
  GimpCropOptions *options = GIMP_CROP_OPTIONS (tool->tool_info->tool_options);
  GimpLayer       *layer;
  gint             x1, y1, x2, y2;
  gint             curx, cury;
  gint             inc_x, inc_y;
  gint             min_x, min_y, max_x, max_y;

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
      gimp_item_offsets (GIMP_ITEM (layer), &min_x, &min_y);
      max_x = gimp_item_width  (GIMP_ITEM (layer)) + min_x;
      max_y = gimp_item_height (GIMP_ITEM (layer)) + min_y;
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
      g_object_set (options, "keep-aspect", FALSE, NULL);
      crop->change_aspect_ratio = TRUE;
      break;

    case RESIZING_LEFT:
      x1 = crop->x1 + inc_x;
      y1 = crop->y1 + inc_y;
     if (options->keep_aspect && crop->aspect_ratio != 0)
       {
         y1 = crop->y2 + (gint)((gdouble)(x1 - crop->x2) / crop->aspect_ratio);
       }
      if (! options->allow_enlarge)
        {
          x1 = CLAMP (x1, min_x, max_x);
          if ((y1 < CLAMP (y1, min_y, max_y)) && (options->keep_aspect))
            {
              x1 = crop->x1;
            }
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
      if (options->keep_aspect && crop->aspect_ratio != 0)
        {
          y2 = crop->y1 + (gint)((gdouble)(x2 - crop->x1) / crop->aspect_ratio);
        }
      if (! options->allow_enlarge)
        {
          x2 = CLAMP (x2, min_x, max_x);
          if ((y2 > CLAMP (y2, min_y, max_y)) && (options->keep_aspect))
            {
              x2 = crop->x2;
            }
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
  crop_recalc (crop, TRUE);

  switch (crop->function)
    {
    case RESIZING_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          crop->x1 - coords->x,
                                          crop->y1 - coords->y,
                                          0, 0);
      break;

    case RESIZING_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          crop->x2 - coords->x,
                                          crop->y2 - coords->y,
                                          0, 0);
      break;

    case MOVING:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          crop->x1 - coords->x,
                                          crop->y1 - coords->y,
                                          crop->x2 - crop->x1,
                                          crop->y2 - crop->y1);
      break;

    default:
      break;
    }

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

static gboolean
gimp_crop_tool_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *gdisp)
{
  GimpCropTool    *crop    = GIMP_CROP_TOOL (tool);
  GimpCropOptions *options = GIMP_CROP_OPTIONS (tool->tool_info->tool_options);
  GimpLayer       *layer;
  gint             inc_x, inc_y;
  gint             min_x, min_y;
  gint             max_x, max_y;

  if (gdisp != tool->gdisp)
    return FALSE;

  inc_x = inc_y = 0;

  switch (kevent->keyval)
    {
    case GDK_Up:
      inc_y = -1;
      break;
    case GDK_Left:
      inc_x = -1;
      break;
    case GDK_Right:
      inc_x = 1;
      break;
    case GDK_Down:
      inc_y = 1;
      break;

    case GDK_KP_Enter:
    case GDK_Return:
      crop_response (NULL, options->crop_mode, crop);
      return TRUE;

    case GDK_Escape:
      crop_response (NULL, GTK_RESPONSE_CANCEL, crop);
      return TRUE;

    default:
      return FALSE;
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
      gimp_item_offsets (GIMP_ITEM (layer), &min_x, &min_y);
      max_x = gimp_item_width (GIMP_ITEM (layer)) + min_x;
      max_y = gimp_item_height (GIMP_ITEM (layer)) + min_y;
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

  crop_recalc (crop, TRUE);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  return TRUE;
}

static void
gimp_crop_tool_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             GimpDisplay     *gdisp)
{
  GimpCropOptions *options = GIMP_CROP_OPTIONS (tool->tool_info->tool_options);

  if (state & GDK_MOD1_MASK)
    {
      if (! options->allow_enlarge)
        g_object_set (options, "allow-enlarge", TRUE, NULL);
    }
  else
    {
      if (options->allow_enlarge)
        g_object_set (options, "allow-enlarge", FALSE, NULL);
    }

  if (state & GDK_SHIFT_MASK)
    {
      if (! options->keep_aspect)
        g_object_set (options, "keep-aspect", TRUE, NULL);
    }
  else
    {
      if (options->keep_aspect)
        g_object_set (options, "keep-aspect", FALSE, NULL);
    }

  if (key == GDK_CONTROL_MASK)
    {
      switch (options->crop_mode)
        {
        case GIMP_CROP_MODE_CROP:
          g_object_set (options, "crop-mode", GIMP_CROP_MODE_RESIZE, NULL);
          break;

        case GIMP_CROP_MODE_RESIZE:
          g_object_set (options, "crop-mode", GIMP_CROP_MODE_CROP, NULL);
          break;

        default:
          break;
        }
    }
}

static void
gimp_crop_tool_oper_update (GimpTool        *tool,
                            GimpCoords      *coords,
                            GdkModifierType  state,
                            GimpDisplay     *gdisp)
{
  GimpCropTool *crop      = GIMP_CROP_TOOL (tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (tool->gdisp != gdisp)
    return;

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

      gimp_tool_control_set_snap_offsets (tool->control,
                                          crop->x1 - coords->x,
                                          crop->y1 - coords->y,
                                          0, 0);
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

      gimp_tool_control_set_snap_offsets (tool->control,
                                          crop->x2 - coords->x,
                                          crop->y2 - coords->y,
                                          0, 0);
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

      gimp_tool_control_set_snap_offsets (tool->control,
                                          crop->x1 - coords->x,
                                          crop->y1 - coords->y,
                                          crop->x2 - crop->x1,
                                          crop->y2 - crop->y1);
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

      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);
    }
}

static void
gimp_crop_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpCropTool       *crop     = GIMP_CROP_TOOL (tool);
  GimpCropOptions    *options;
  GimpCursorType      cursor   = GIMP_CURSOR_CROSSHAIR_SMALL;
  GimpCursorModifier  modifier = GIMP_CURSOR_MODIFIER_NONE;

  options = GIMP_CROP_OPTIONS (tool->tool_info->tool_options);

  if (tool->gdisp == gdisp)
    {
      switch (crop->function)
        {
        case MOVING:
          modifier = GIMP_CURSOR_MODIFIER_MOVE;
          break;

        case RESIZING_LEFT:
        case RESIZING_RIGHT:
          modifier = GIMP_CURSOR_MODIFIER_RESIZE;
          break;

        default:
          break;
        }
    }

  gimp_tool_control_set_cursor (tool->control, cursor);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     options->crop_mode ==
                                     GIMP_CROP_MODE_CROP ?
                                     GIMP_TOOL_CURSOR_CROP :
                                     GIMP_TOOL_CURSOR_RESIZE);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_crop_tool_draw (GimpDrawTool *draw)
{
  GimpCropTool     *crop   = GIMP_CROP_TOOL (draw);
  GimpTool         *tool   = GIMP_TOOL (draw);
  GimpDisplayShell *shell  = GIMP_DISPLAY_SHELL (tool->gdisp->shell);
  GimpCanvas       *canvas = GIMP_CANVAS (shell->canvas);

  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         crop->dx1, crop->dy1,
                         shell->disp_width, crop->dy1);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         crop->dx1, crop->dy1,
                         crop->dx1, shell->disp_height);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         crop->dx2, crop->dy2,
                         0, crop->dy2);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
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
}

static void
crop_tool_crop_image (GimpImage    *gimage,
                      GimpContext  *context,
                      gint          x1,
                      gint          y1,
                      gint          x2,
                      gint          y2,
                      gboolean      layer_only,
                      GimpCropMode  crop_mode)
{
  if ((x1 == x2) || (y1 == y2))
    return;

  gimp_image_crop (gimage, context,
                   x1, y1, x2, y2,
                   layer_only,
                   crop_mode == GIMP_CROP_MODE_CROP);

  gimp_image_flush (gimage);
}

static void
crop_recalc (GimpCropTool *crop,
             gboolean      recalc_highlight)
{
  GimpTool         *tool    = GIMP_TOOL (crop);
  GimpDisplayShell *shell   = GIMP_DISPLAY_SHELL (tool->gdisp->shell);
  GimpCropOptions  *options = GIMP_CROP_OPTIONS (tool->tool_info->tool_options);

  if (! tool->gdisp)
    return;

  if (recalc_highlight)
    {
      GdkRectangle rect;

      rect.x      = crop->x1;
      rect.y      = crop->y1;
      rect.width  = crop->x2 - crop->x1;
      rect.height = crop->y2 - crop->y1;

      gimp_display_shell_set_highlight (shell, &rect);
    }

  gimp_display_shell_transform_xy (shell,
                                   crop->x1, crop->y1,
                                   &crop->dx1, &crop->dy1,
                                   FALSE);
  gimp_display_shell_transform_xy (shell,
                                   crop->x2, crop->y2,
                                   &crop->dx2, &crop->dy2,
                                   FALSE);

#define SRW 10
#define SRH 10

  crop->dcw = ((crop->dx2 - crop->dx1) < SRW) ? (crop->dx2 - crop->dx1) : SRW;
  crop->dch = ((crop->dy2 - crop->dy1) < SRH) ? (crop->dy2 - crop->dy1) : SRH;

#undef SRW
#undef SRH

  if (! options->keep_aspect)
    {
      if ((crop->y2 - crop->y1) != 0)
        {
          if (crop->change_aspect_ratio)
            crop->aspect_ratio = ((gdouble) (crop->x2 - crop->x1) /
                                  (gdouble) (crop->y2 - crop->y1));
        }
      else
        {
          crop->aspect_ratio = 0.0;
        }
    }

  if (crop->crop_info)
    crop_info_update (crop);
}

static void
crop_start (GimpCropTool *crop,
            gboolean      dialog)
{
  GimpTool         *tool  = GIMP_TOOL (crop);
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (tool->gdisp->shell);

  crop_recalc (crop, TRUE);

  if (dialog && ! crop->crop_info)
    crop_info_create (crop);

  if (crop->crop_info)
    {
      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (crop->crop_info->shell),
                                         GIMP_VIEWABLE (tool->gdisp->gimage));

      g_signal_handlers_block_by_func (crop->origin_sizeentry,
                                       crop_origin_changed,
                                       crop);
      g_signal_handlers_block_by_func (crop->size_sizeentry,
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

      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (crop->origin_sizeentry),
                                shell->unit);
      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (crop->size_sizeentry),
                                shell->unit);

      g_signal_handlers_unblock_by_func (crop->origin_sizeentry,
                                         crop_origin_changed,
                                         crop);
      g_signal_handlers_unblock_by_func (crop->size_sizeentry,
                                         crop_size_changed,
                                         crop);

      /* restore sensitivity of buttons */
      gtk_dialog_set_response_sensitive (GTK_DIALOG (crop->crop_info->shell),
                                         GIMP_CROP_MODE_CROP,   TRUE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (crop->crop_info->shell),
                                         GIMP_CROP_MODE_RESIZE, TRUE);
    }

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, _("Crop: "), 0, " x ", 0);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), tool->gdisp);
}


/***************************/
/*  Crop dialog functions  */
/***************************/

static void
crop_info_create (GimpCropTool *crop)
{
  GimpTool         *tool  = GIMP_TOOL (crop);
  GimpDisplay      *gdisp = tool->gdisp;
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (gdisp->shell);
  GtkWidget        *spinbutton;
  GtkWidget        *bbox;
  GtkWidget        *button;
  const gchar      *stock_id;

  stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool->tool_info));

  crop->crop_info = info_dialog_new (NULL,
                                     tool->tool_info->blurb,
                                     GIMP_OBJECT (tool->tool_info)->name,
                                     stock_id,
                                     _("Crop & Resize Information"),
                                     NULL /* gdisp->shell */,
                                     gimp_standard_help_func,
                                     tool->tool_info->help_id);

  gtk_dialog_add_buttons (GTK_DIALOG (crop->crop_info->shell),
                          GTK_STOCK_CANCEL,     GTK_RESPONSE_CANCEL,
                          GIMP_STOCK_RESIZE,    GIMP_CROP_MODE_RESIZE,
                          GIMP_STOCK_TOOL_CROP, GIMP_CROP_MODE_CROP,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (crop->crop_info->shell),
                                   GIMP_CROP_MODE_CROP);

  g_signal_connect (crop->crop_info->shell, "response",
                    G_CALLBACK (crop_response),
                    crop);

  /*  add the information fields  */
  spinbutton = info_dialog_add_spinbutton (crop->crop_info,
                                           _("Origin X:"), NULL,
                                           -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  crop->origin_sizeentry =
    info_dialog_add_sizeentry (crop->crop_info,
                               _("Origin Y:"), crop->orig_vals, 1,
                               shell->unit, "%a",
                               TRUE, TRUE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
                               G_CALLBACK (crop_origin_changed),
                               crop);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (crop->origin_sizeentry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (crop->origin_sizeentry),
                                         0, -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (crop->origin_sizeentry),
                                         1, -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);

  spinbutton = info_dialog_add_spinbutton (crop->crop_info, _("Width:"), NULL,
                                            -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  crop->size_sizeentry =
    info_dialog_add_sizeentry (crop->crop_info,
                               _("Height:"), crop->size_vals, 1,
                               shell->unit, "%a",
                               TRUE, TRUE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
                               G_CALLBACK (crop_size_changed),
                               crop);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (crop->size_sizeentry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (crop->size_sizeentry),
                                         0,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (crop->size_sizeentry),
                                         1,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gtk_table_set_row_spacing (GTK_TABLE (crop->crop_info->info_table), 0, 0);
  gtk_table_set_row_spacing (GTK_TABLE (crop->crop_info->info_table), 1, 6);
  gtk_table_set_row_spacing (GTK_TABLE (crop->crop_info->info_table), 2, 0);

  spinbutton = info_dialog_add_spinbutton (crop->crop_info, _("Aspect ratio:"),
                                           &crop->aspect_ratio,
                                           0, 65536, 0.01, 0.1, 1, 0.5, 2,
                                           G_CALLBACK (crop_aspect_changed),
                                           crop);

  /* Create the area selection buttons */
  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
  gtk_box_set_spacing (GTK_BOX (bbox), 6);

  button = gtk_button_new_with_label (_("From selection"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button , "clicked",
                    G_CALLBACK (crop_selection_callback),
                    crop);

  button = gtk_button_new_with_label (_("Auto shrink"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button , "clicked",
                    G_CALLBACK (crop_automatic_callback),
                    crop);

  gtk_box_pack_start (GTK_BOX (crop->crop_info->vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);

  gimp_dialog_factory_add_foreign (gimp_dialog_factory_from_name ("toplevel"),
                                   "gimp-crop-tool-dialog",
                                   crop->crop_info->shell);
}

static void
crop_info_update (GimpCropTool *crop)
{
  crop->orig_vals[0] = crop->x1;
  crop->orig_vals[1] = crop->y1;
  crop->size_vals[0] = crop->x2 - crop->x1;
  crop->size_vals[1] = crop->y2 - crop->y1;

  info_dialog_update (crop->crop_info);
  info_dialog_show (crop->crop_info);
}

static void
crop_response (GtkWidget    *widget,
               gint          response_id,
               GimpCropTool *crop)
{
  GimpTool        *tool    = GIMP_TOOL (crop);
  GimpCropOptions *options = GIMP_CROP_OPTIONS (tool->tool_info->tool_options);

  switch (response_id)
    {
    case GIMP_CROP_MODE_CROP:
    case GIMP_CROP_MODE_RESIZE:
      if (crop->crop_info)
        {
          /* set these buttons to be insensitive so that you cannot
           * accidentially trigger a crop while one is ongoing */

          gtk_dialog_set_response_sensitive (GTK_DIALOG (crop->crop_info->shell),
                                             GIMP_CROP_MODE_CROP,   FALSE);
          gtk_dialog_set_response_sensitive (GTK_DIALOG (crop->crop_info->shell),
                                             GIMP_CROP_MODE_RESIZE, FALSE);
        }

      gimp_display_shell_set_highlight (GIMP_DISPLAY_SHELL (tool->gdisp->shell),
                                        NULL);

      crop_tool_crop_image (tool->gdisp->gimage,
                            GIMP_CONTEXT (options),
                            crop->x1, crop->y1,
                            crop->x2, crop->y2,
                            options->layer_only,
                            (GimpCropMode) response_id);

      break;

    default:
      break;
    }

  if (tool->gdisp)
    gimp_display_shell_set_highlight (GIMP_DISPLAY_SHELL (tool->gdisp->shell),
                                      NULL);

  if (crop->crop_info)
    {
      info_dialog_free (crop->crop_info);
      crop->crop_info = NULL;
    }

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (crop)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (crop));

  if (gimp_tool_control_is_active (GIMP_TOOL (crop)->control))
    gimp_tool_control_halt (GIMP_TOOL (crop)->control);

  tool->gdisp    = NULL;
  tool->drawable = NULL;
}

static void
crop_selection_callback (GtkWidget    *widget,
                         GimpCropTool *crop)
{
  GimpCropOptions *options;
  GimpDisplay     *gdisp;

  options = GIMP_CROP_OPTIONS (GIMP_TOOL (crop)->tool_info->tool_options);
  gdisp   = GIMP_TOOL (crop)->gdisp;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (crop));

  if (! gimp_channel_bounds (gimp_image_get_mask (gdisp->gimage),
                             &crop->x1, &crop->y1,
                             &crop->x2, &crop->y2))
    {
      if (options->layer_only)
        {
          GimpLayer *layer;

          layer = gimp_image_get_active_layer (gdisp->gimage);

          gimp_item_offsets (GIMP_ITEM (layer), &crop->x1, &crop->y1);
          crop->x2 = crop->x1 + gimp_item_width  (GIMP_ITEM (layer));
          crop->y2 = crop->y1 + gimp_item_height (GIMP_ITEM (layer));
        }
      else
        {
          crop->x1 = crop->y1 = 0;
          crop->x2 = gdisp->gimage->width;
          crop->y2 = gdisp->gimage->height;
        }
    }

  /* force change of aspect ratio */
  if ((crop->y2 - crop->y1) != 0)
    crop->aspect_ratio = ((gdouble) (crop->x2 - crop->x1) /
                          (gdouble) (crop->y2 - crop->y1));

  crop_recalc (crop, TRUE);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (crop));
}

static void
crop_automatic_callback (GtkWidget    *widget,
                         GimpCropTool *crop)
{
  GimpCropOptions *options;
  GimpDisplay     *gdisp;
  gint             offset_x, offset_y;
  gint             width, height;
  gint             x1, y1, x2, y2;
  gint             shrunk_x1;
  gint             shrunk_y1;
  gint             shrunk_x2;
  gint             shrunk_y2;

  options = GIMP_CROP_OPTIONS (GIMP_TOOL (crop)->tool_info->tool_options);
  gdisp   = GIMP_TOOL (crop)->gdisp;

  if (options->layer_only)
    {
      GimpDrawable *active_drawable;

      active_drawable = gimp_image_active_drawable (gdisp->gimage);

      if (! active_drawable)
        return;

      gimp_item_offsets (GIMP_ITEM (active_drawable), &offset_x, &offset_y);
      width  = gimp_item_width  (GIMP_ITEM (active_drawable));
      height = gimp_item_height (GIMP_ITEM (active_drawable));
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

  if (gimp_image_crop_auto_shrink (gdisp->gimage,
                                   x1, y1, x2, y2,
                                   options->layer_only,
                                   &shrunk_x1,
                                   &shrunk_y1,
                                   &shrunk_x2,
                                   &shrunk_y2))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (crop));

      crop->x1 = offset_x + shrunk_x1;
      crop->x2 = offset_x + shrunk_x2;
      crop->y1 = offset_y + shrunk_y1;
      crop->y2 = offset_y + shrunk_y2;

      crop_recalc (crop, TRUE);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (crop));
    }
}

static void
crop_origin_changed (GtkWidget    *widget,
                     GimpCropTool *crop)
{
  gint origin_x;
  gint origin_y;

  origin_x = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0));
  origin_y = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1));

  if ((origin_x != crop->x1) ||
      (origin_y != crop->y1))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (crop));

      crop->x2 = crop->x2 + (origin_x - crop->x1);
      crop->x1 = origin_x;
      crop->y2 = crop->y2 + (origin_y - crop->y1);
      crop->y1 = origin_y;

      crop_recalc (crop, TRUE);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (crop));
    }
}

static void
crop_size_changed (GtkWidget    *widget,
                   GimpCropTool *crop)
{
  gint size_x = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  gint size_y = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if ((size_x != (crop->x2 - crop->x1)) ||
      (size_y != (crop->y2 - crop->y1)))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (crop));

      crop->x2 = size_x + crop->x1;
      crop->y2 = size_y + crop->y1;

      crop_recalc (crop, TRUE);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (crop));
    }
}

static void
crop_aspect_changed (GtkWidget    *widget,
                     GimpCropTool *crop)
{
  crop->aspect_ratio = GTK_ADJUSTMENT (widget)->value;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (crop));

  crop->y2 = crop->y1 + ((gdouble) (crop->x2 - crop->x1) / crop->aspect_ratio);

  if (crop->y2 >= crop->y1 + 1)
    crop->change_aspect_ratio = FALSE;
  else
    {
      crop->change_aspect_ratio = TRUE;
      crop->y2 = crop->y1 + 1;
    }

  crop_recalc (crop, TRUE);
  crop->change_aspect_ratio = TRUE;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (crop));
}

