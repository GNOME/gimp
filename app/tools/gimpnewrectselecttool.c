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
#include "libgimpconfig/gimpconfig.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimptoolinfo.h"
#include "core/gimp-utils.h"

#include "widgets/gimpdialogfactory.h"
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

#include "gimpselectiontool.h"
#include "gimpselectionoptions.h"
#include "gimpnewrectselecttool.h"
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


static void     gimp_new_rect_select_tool_class_init     (GimpNewRectSelectToolClass *klass);
static void     gimp_new_rect_select_tool_init           (GimpNewRectSelectTool      *new_rect_select_tool);

static void     gimp_new_rect_select_tool_finalize       (GObject         *object);

static void     gimp_new_rect_select_tool_control        (GimpTool        *tool,
                                                          GimpToolAction   action,
                                                          GimpDisplay     *gdisp);
static void     gimp_new_rect_select_tool_button_press   (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);
static void     gimp_new_rect_select_tool_button_release (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);
static void     gimp_new_rect_select_tool_motion         (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);
static gboolean gimp_new_rect_select_tool_key_press      (GimpTool        *tool,
                                                          GdkEventKey     *kevent,
                                                          GimpDisplay     *gdisp);
static void     gimp_new_rect_select_tool_modifier_key   (GimpTool        *tool,
                                                          GdkModifierType  key,
                                                          gboolean         press,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);
static void     gimp_new_rect_select_tool_oper_update    (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);
static void     gimp_new_rect_select_tool_cursor_update  (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);

static void     gimp_new_rect_select_tool_draw           (GimpDrawTool    *draw_tool);

/*  Rect_Select helper functions  */
static void     rect_select_recalc                       (GimpNewRectSelectTool    *rect_select,
                                                          gboolean         recalc_highlight);
static void     rect_select_start                        (GimpNewRectSelectTool    *rect_select,
                                                          gboolean         dialog);

/*  Rect_Select dialog functions  */
static void     rect_select_info_update                  (GimpNewRectSelectTool    *rect_select);
static void     rect_select_info_create                  (GimpNewRectSelectTool    *rect_select);
static void     rect_select_response                     (GtkWidget       *widget,
                                                          gint             response_id,
                                                          GimpNewRectSelectTool    *rect_select);

static void     rect_select_selection_callback           (GtkWidget       *widget,
                                                          GimpNewRectSelectTool    *rect_select);
static void     rect_select_automatic_callback           (GtkWidget       *widget,
                                                          GimpNewRectSelectTool    *rect_select);

static void     rect_select_origin_changed               (GtkWidget       *widget,
                                                          GimpNewRectSelectTool    *rect_select);
static void     rect_select_size_changed                 (GtkWidget       *widget,
                                                          GimpNewRectSelectTool    *rect_select);
static void     rect_select_aspect_changed               (GtkWidget       *widget,
                                                          GimpNewRectSelectTool    *rect_select);

static void  new_rect_select_tool_rect_select            (GimpNewRectSelectTool *rect_tool,
                                                          gint                x,
                                                          gint                y,
                                                          gint                w,
                                                          gint                h);

static void   gimp_new_rect_select_tool_real_rect_select (GimpNewRectSelectTool *rect_tool,
                                                          gint                x,
                                                          gint                y,
                                                          gint                w,
                                                          gint                h);

static GimpDrawToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_new_rect_select_tool_register (GimpToolRegisterCallback  callback,
                                    gpointer                  data)
{
  (* callback) (GIMP_TYPE_NEW_RECT_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-new-rect-select-tool",
                _("New Rect Select"),
                _("Select a Rectangular part of an image"),
                N_("_New Rect Select"), "<shift>C",
                NULL, GIMP_HELP_TOOL_RECT_SELECT,
                GIMP_STOCK_TOOL_NEW_RECT_SELECT,
                data);
}

GType
gimp_new_rect_select_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpNewRectSelectToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_new_rect_select_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpNewRectSelectTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_new_rect_select_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_SELECTION_TOOL,
                                          "GimpNewRectSelectTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_new_rect_select_tool_class_init (GimpNewRectSelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_new_rect_select_tool_finalize;

  tool_class->control        = gimp_new_rect_select_tool_control;
  tool_class->button_press   = gimp_new_rect_select_tool_button_press;
  tool_class->button_release = gimp_new_rect_select_tool_button_release;
  tool_class->motion         = gimp_new_rect_select_tool_motion;
  tool_class->key_press      = gimp_new_rect_select_tool_key_press;
  tool_class->modifier_key   = gimp_new_rect_select_tool_modifier_key;
  tool_class->oper_update    = gimp_new_rect_select_tool_oper_update;
  tool_class->cursor_update  = gimp_new_rect_select_tool_cursor_update;

  draw_tool_class->draw      = gimp_new_rect_select_tool_draw;
  klass->rect_select         = gimp_new_rect_select_tool_real_rect_select;
}

static void
gimp_new_rect_select_tool_init (GimpNewRectSelectTool *new_rect_select_tool)
{
  GimpTool *tool = GIMP_TOOL (new_rect_select_tool);

  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control, GIMP_DIRTY_IMAGE_SIZE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_RECT_SELECT);
}

static void
gimp_new_rect_select_tool_finalize (GObject *object)
{
  GimpNewRectSelectTool *rect_select = GIMP_NEW_RECT_SELECT_TOOL (object);

  if (rect_select->rect_select_info)
    {
      info_dialog_free (rect_select->rect_select_info);
      rect_select->rect_select_info = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_new_rect_select_tool_control (GimpTool       *tool,
                                   GimpToolAction  action,
                                   GimpDisplay    *gdisp)
{
  GimpNewRectSelectTool *rect_select = GIMP_NEW_RECT_SELECT_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      rect_select_recalc (rect_select, FALSE);
      break;

    case HALT:
      rect_select_response (NULL, GTK_RESPONSE_CANCEL, rect_select);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_new_rect_select_tool_button_press (GimpTool        *tool,
                                        GimpCoords      *coords,
                                        guint32          time,
                                        GdkModifierType  state,
                                        GimpDisplay     *gdisp)
{
  GimpNewRectSelectTool *rect_select  = GIMP_NEW_RECT_SELECT_TOOL (tool);
  GimpDrawTool          *draw_tool    = GIMP_DRAW_TOOL (tool);
  GimpSelectionOptions  *options;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  if (gdisp != tool->gdisp)
    {
      if (gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_stop (draw_tool);

      rect_select->function = CREATING;
      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);

      tool->gdisp = gdisp;

      rect_select->x2 = rect_select->x1 = ROUND (coords->x);
      rect_select->y2 = rect_select->y1 = ROUND (coords->y);

      rect_select_start (rect_select, options->show_dialog);
    }

  rect_select->lastx = rect_select->startx = ROUND (coords->x);
  rect_select->lasty = rect_select->starty = ROUND (coords->y);

  gimp_tool_control_activate (tool->control);
}

static void
gimp_new_rect_select_tool_button_release (GimpTool        *tool,
                                          GimpCoords      *coords,
                                          guint32          time,
                                          GdkModifierType  state,
                                          GimpDisplay     *gdisp)
{
  GimpNewRectSelectTool  *rect_select    = GIMP_NEW_RECT_SELECT_TOOL (tool);
  GimpSelectionOptions  *options;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimp_tool_control_halt (tool->control);
  gimp_tool_pop_status (tool);

  if (! (state & GDK_BUTTON3_MASK))
    {
      if (rect_select->function == CROPPING || !options->adjustable)
        rect_select_response (NULL, GIMP_CROP_MODE_CROP, rect_select);
      else if (rect_select->function == CREATING)
        {
          if ( (rect_select->lastx == rect_select->startx) &&
               (rect_select->lasty == rect_select->starty))
            {
              rect_select_response (NULL, GTK_RESPONSE_CANCEL, rect_select);
            }
        }
      else if (rect_select->rect_select_info)
        rect_select_info_update (rect_select);
    }
}

static void
gimp_new_rect_select_tool_motion (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *gdisp)
{
  GimpNewRectSelectTool    *rect_select    = GIMP_NEW_RECT_SELECT_TOOL (tool);
  gint             x1, y1, x2, y2;
  gint             curx, cury;
  gint             inc_x, inc_y;
  gint             min_x, min_y, max_x, max_y;

  /*  This is the only case when the motion events should be ignored--
      we're just waiting for the button release event to rect_select the image  */
  if (rect_select->function == CROPPING)
    return;

  curx = ROUND (coords->x);
  cury = ROUND (coords->y);

  x1 = rect_select->startx;
  y1 = rect_select->starty;
  x2 = curx;
  y2 = cury;

  inc_x = (x2 - x1);
  inc_y = (y2 - y1);

  /*  If there have been no changes... return  */
  if (rect_select->lastx == x2 && rect_select->lasty == y2)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  min_x = min_y = 0;
  max_x = gdisp->gimage->width;
  max_y = gdisp->gimage->height;

  switch (rect_select->function)
    {
    case CREATING:
      rect_select->change_aspect_ratio = TRUE;
      break;

    case RESIZING_LEFT:
      x1 = rect_select->x1 + inc_x;
      y1 = rect_select->y1 + inc_y;
      x2 = MAX (x1, rect_select->x2);
      y2 = MAX (y1, rect_select->y2);
      rect_select->startx = curx;
      rect_select->starty = cury;
      break;

    case RESIZING_RIGHT:
      x2 = rect_select->x2 + inc_x;
      y2 = rect_select->y2 + inc_y;
      x1 = MIN (rect_select->x1, x2);
      y1 = MIN (rect_select->y1, y2);
      rect_select->startx = curx;
      rect_select->starty = cury;
      break;

    case MOVING:
      x1 = rect_select->x1 + inc_x;
      x2 = rect_select->x2 + inc_x;
      y1 = rect_select->y1 + inc_y;
      y2 = rect_select->y2 + inc_y;
      rect_select->startx = curx;
      rect_select->starty = cury;
      break;
    }

  /*  make sure that the coords are in bounds  */
  rect_select->x1 = MIN (x1, x2);
  rect_select->y1 = MIN (y1, y2);
  rect_select->x2 = MAX (x1, x2);
  rect_select->y2 = MAX (y1, y2);

  rect_select->lastx = curx;
  rect_select->lasty = cury;

  /*  recalculate the coordinates for rect_select_draw based on the new values  */
  rect_select_recalc (rect_select, TRUE);

  switch (rect_select->function)
    {
    case RESIZING_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rect_select->x1 - coords->x,
                                          rect_select->y1 - coords->y,
                                          0, 0);
      break;

    case RESIZING_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rect_select->x2 - coords->x,
                                          rect_select->y2 - coords->y,
                                          0, 0);
      break;

    case MOVING:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rect_select->x1 - coords->x,
                                          rect_select->y1 - coords->y,
                                          rect_select->x2 - rect_select->x1,
                                          rect_select->y2 - rect_select->y1);
      break;

    default:
      break;
    }

  if (rect_select->function == CREATING      ||
      rect_select->function == RESIZING_LEFT ||
      rect_select->function == RESIZING_RIGHT)
    {
      gimp_tool_pop_status (tool);

      gimp_tool_push_status_coords (tool,
                                    _("Rect_Select: "),
                                    rect_select->x2 - rect_select->x1,
                                    " x ",
                                    rect_select->y2 - rect_select->y1);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static gboolean
gimp_new_rect_select_tool_key_press (GimpTool    *tool,
                                     GdkEventKey *kevent,
                                     GimpDisplay *gdisp)
{
  GimpNewRectSelectTool    *rect_select    = GIMP_NEW_RECT_SELECT_TOOL (tool);
  gint                      inc_x, inc_y;
  gint                      min_x, min_y;
  gint                      max_x, max_y;

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
      rect_select_response (NULL, GIMP_CROP_MODE_CROP, rect_select);
      return TRUE;

    case GDK_Escape:
      rect_select_response (NULL, GTK_RESPONSE_CANCEL, rect_select);
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

  min_x = min_y = 0;
  max_x = gdisp->gimage->width;
  max_y = gdisp->gimage->height;

  rect_select->x1 += inc_x;
  rect_select->x2 += inc_x;
  rect_select->y1 += inc_y;
  rect_select->y2 += inc_y;

  rect_select_recalc (rect_select, TRUE);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  return TRUE;
}

static void
gimp_new_rect_select_tool_modifier_key (GimpTool        *tool,
                                        GdkModifierType  key,
                                        gboolean         press,
                                        GdkModifierType  state,
                                        GimpDisplay     *gdisp)
{
}

static void
gimp_new_rect_select_tool_oper_update (GimpTool        *tool,
                                       GimpCoords      *coords,
                                       GdkModifierType  state,
                                       GimpDisplay     *gdisp)
{
  GimpNewRectSelectTool *rect_select  = GIMP_NEW_RECT_SELECT_TOOL (tool);
  GimpDrawTool          *draw_tool    = GIMP_DRAW_TOOL (tool);

  if (tool->gdisp != gdisp)
    return;

  /*  If the cursor is in either the upper left or lower right boxes,
   *  The new function will be to resize the current rect_select area
   */
  if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                coords->x, coords->y,
                                GIMP_HANDLE_SQUARE,
                                rect_select->x1, rect_select->y1,
                                rect_select->dcw, rect_select->dch,
                                GTK_ANCHOR_NORTH_WEST,
                                FALSE))
    {
      rect_select->function = RESIZING_LEFT;

      gimp_tool_control_set_snap_offsets (tool->control,
                                          rect_select->x1 - coords->x,
                                          rect_select->y1 - coords->y,
                                          0, 0);
    }
  else if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     rect_select->x2, rect_select->y2,
                                     rect_select->dcw, rect_select->dch,
                                     GTK_ANCHOR_SOUTH_EAST,
                                     FALSE))
    {
      rect_select->function = RESIZING_RIGHT;

      gimp_tool_control_set_snap_offsets (tool->control,
                                          rect_select->x2 - coords->x,
                                          rect_select->y2 - coords->y,
                                          0, 0);
    }
  /*  If the cursor is in either the upper right or lower left boxes,
   *  The new function will be to translate the current rect_select area
   */
  else if  (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                      coords->x, coords->y,
                                      GIMP_HANDLE_SQUARE,
                                      rect_select->x2, rect_select->y1,
                                      rect_select->dcw, rect_select->dch,
                                      GTK_ANCHOR_NORTH_EAST,
                                      FALSE) ||
            gimp_draw_tool_on_handle (draw_tool, gdisp,
                                      coords->x, coords->y,
                                      GIMP_HANDLE_SQUARE,
                                      rect_select->x1, rect_select->y2,
                                      rect_select->dcw, rect_select->dch,
                                      GTK_ANCHOR_SOUTH_WEST,
                                      FALSE))
    {
      rect_select->function = MOVING;

      gimp_tool_control_set_snap_offsets (tool->control,
                                          rect_select->x1 - coords->x,
                                          rect_select->y1 - coords->y,
                                          rect_select->x2 - rect_select->x1,
                                          rect_select->y2 - rect_select->y1);
    }
  /*  If the pointer is in the rectangular region, rect_select or resize it!
   */
  else if (coords->x > rect_select->x1 &&
           coords->x < rect_select->x2 &&
           coords->y > rect_select->y1 &&
           coords->y < rect_select->y2)
    {
      rect_select->function = CROPPING;
    }
  /*  otherwise, the new function will be creating, since we want
   *  to start a new
   */
  else
    {
      rect_select->function = CREATING;

      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);
    }
}

static void
gimp_new_rect_select_tool_cursor_update (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         GdkModifierType  state,
                                         GimpDisplay     *gdisp)
{
  GimpNewRectSelectTool       *rect_select  = GIMP_NEW_RECT_SELECT_TOOL (tool);
  GimpSelectionOptions        *options;
  GimpCursorType               cursor       = GIMP_CURSOR_CROSSHAIR_SMALL;
  GimpCursorModifier           modifier     = GIMP_CURSOR_MODIFIER_NONE;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  if (tool->gdisp == gdisp)
    {
      switch (rect_select->function)
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
                                     GIMP_TOOL_CURSOR_CROP);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_new_rect_select_tool_draw (GimpDrawTool *draw)
{
  GimpNewRectSelectTool     *rect_select  = GIMP_NEW_RECT_SELECT_TOOL (draw);
  GimpTool                  *tool         = GIMP_TOOL (draw);
  GimpDisplayShell          *shell        = GIMP_DISPLAY_SHELL (tool->gdisp->shell);
  GimpCanvas                *canvas       = GIMP_CANVAS (shell->canvas);

  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         rect_select->dx1, rect_select->dy1,
                         shell->disp_width, rect_select->dy1);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         rect_select->dx1, rect_select->dy1,
                         rect_select->dx1, shell->disp_height);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         rect_select->dx2, rect_select->dy2,
                         0, rect_select->dy2);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         rect_select->dx2, rect_select->dy2,
                         rect_select->dx2, 0);

  gimp_draw_tool_draw_handle (draw,
                              GIMP_HANDLE_FILLED_SQUARE,
                              rect_select->x1, rect_select->y1,
                              rect_select->dcw, rect_select->dch,
                              GTK_ANCHOR_NORTH_WEST,
                              FALSE);
  gimp_draw_tool_draw_handle (draw,
                              GIMP_HANDLE_FILLED_SQUARE,
                              rect_select->x2, rect_select->y1,
                              rect_select->dcw, rect_select->dch,
                              GTK_ANCHOR_NORTH_EAST,
                              FALSE);
  gimp_draw_tool_draw_handle (draw,
                              GIMP_HANDLE_FILLED_SQUARE,
                              rect_select->x1, rect_select->y2,
                              rect_select->dcw, rect_select->dch,
                              GTK_ANCHOR_SOUTH_WEST,
                              FALSE);
  gimp_draw_tool_draw_handle (draw,
                              GIMP_HANDLE_FILLED_SQUARE,
                              rect_select->x2, rect_select->y2,
                              rect_select->dcw, rect_select->dch,
                              GTK_ANCHOR_SOUTH_EAST,
                              FALSE);
}

static void
rect_select_recalc (GimpNewRectSelectTool *rect_select,
                    gboolean      recalc_highlight)
{
  GimpTool         *tool    = GIMP_TOOL (rect_select);
  GimpDisplayShell *shell   = GIMP_DISPLAY_SHELL (tool->gdisp->shell);

  if (! tool->gdisp)
    return;

  if (recalc_highlight)
    {
      GdkRectangle rect;

      rect.x      = rect_select->x1;
      rect.y      = rect_select->y1;
      rect.width  = rect_select->x2 - rect_select->x1;
      rect.height = rect_select->y2 - rect_select->y1;

      gimp_display_shell_set_highlight (shell, &rect);
    }

  gimp_display_shell_transform_xy (shell,
                                   rect_select->x1, rect_select->y1,
                                   &rect_select->dx1, &rect_select->dy1,
                                   FALSE);
  gimp_display_shell_transform_xy (shell,
                                   rect_select->x2, rect_select->y2,
                                   &rect_select->dx2, &rect_select->dy2,
                                   FALSE);

#define SRW 10
#define SRH 10

  rect_select->dcw = ((rect_select->dx2 - rect_select->dx1) < SRW) ? (rect_select->dx2 - rect_select->dx1) : SRW;
  rect_select->dch = ((rect_select->dy2 - rect_select->dy1) < SRH) ? (rect_select->dy2 - rect_select->dy1) : SRH;

#undef SRW
#undef SRH

  if ((rect_select->y2 - rect_select->y1) != 0)
    {
      if (rect_select->change_aspect_ratio)
        rect_select->aspect_ratio = ((gdouble) (rect_select->x2 - rect_select->x1) /
                              (gdouble) (rect_select->y2 - rect_select->y1));
    }
  else
    {
      rect_select->aspect_ratio = 0.0;
    }

  if (rect_select->rect_select_info)
    rect_select_info_update (rect_select);
}

static void
rect_select_start (GimpNewRectSelectTool *rect_select,
                   gboolean               dialog)
{
  GimpTool         *tool  = GIMP_TOOL (rect_select);
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (tool->gdisp->shell);

  rect_select_recalc (rect_select, TRUE);

  if (dialog && ! rect_select->rect_select_info)
    rect_select_info_create (rect_select);

  if (rect_select->rect_select_info)
    {
      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (rect_select->rect_select_info->shell),
                                         GIMP_VIEWABLE (tool->gdisp->gimage));

      g_signal_handlers_block_by_func (rect_select->origin_sizeentry,
                                       rect_select_origin_changed,
                                       rect_select);
      g_signal_handlers_block_by_func (rect_select->size_sizeentry,
                                       rect_select_size_changed,
                                       rect_select);

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (rect_select->origin_sizeentry), 0,
                                      tool->gdisp->gimage->xresolution, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (rect_select->origin_sizeentry), 1,
                                      tool->gdisp->gimage->yresolution, FALSE);

      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (rect_select->origin_sizeentry), 0,
                                0, tool->gdisp->gimage->width);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (rect_select->origin_sizeentry), 1,
                                0, tool->gdisp->gimage->height);

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (rect_select->size_sizeentry), 0,
                                      tool->gdisp->gimage->xresolution, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (rect_select->size_sizeentry), 1,
                                      tool->gdisp->gimage->yresolution, FALSE);

      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (rect_select->size_sizeentry), 0,
                                0, tool->gdisp->gimage->width);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (rect_select->size_sizeentry), 1,
                                0, tool->gdisp->gimage->height);

      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (rect_select->origin_sizeentry),
                                shell->unit);
      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (rect_select->size_sizeentry),
                                shell->unit);

      g_signal_handlers_unblock_by_func (rect_select->origin_sizeentry,
                                         rect_select_origin_changed,
                                         rect_select);
      g_signal_handlers_unblock_by_func (rect_select->size_sizeentry,
                                         rect_select_size_changed,
                                         rect_select);

      /* restore sensitivity of buttons */
      gtk_dialog_set_response_sensitive (GTK_DIALOG (rect_select->rect_select_info->shell),
                                         GIMP_CROP_MODE_CROP,   TRUE);
    }

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, _("Rect_Select: "), 0, " x ", 0);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), tool->gdisp);
}


/***************************/
/*  Rect_Select dialog functions  */
/***************************/

static void
rect_select_info_create (GimpNewRectSelectTool *rect_select)
{
  GimpTool         *tool  = GIMP_TOOL (rect_select);
  GimpDisplay      *gdisp = tool->gdisp;
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (gdisp->shell);
  GtkWidget        *spinbutton;
  GtkWidget        *bbox;
  GtkWidget        *button;
  const gchar      *stock_id;

  stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool->tool_info));

  rect_select->rect_select_info = info_dialog_new (NULL,
                                     tool->tool_info->blurb,
                                     GIMP_OBJECT (tool->tool_info)->name,
                                     stock_id,
                                     _("Rectangle Select Information"),
                                     NULL /* gdisp->shell */,
                                     gimp_standard_help_func,
                                     tool->tool_info->help_id);

  gtk_dialog_add_buttons (GTK_DIALOG (rect_select->rect_select_info->shell),
                          GTK_STOCK_CANCEL,     GTK_RESPONSE_CANCEL,
                          GIMP_STOCK_SELECTION, GIMP_CROP_MODE_CROP,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (rect_select->rect_select_info->shell),
                                   GIMP_CROP_MODE_CROP);

  g_signal_connect (rect_select->rect_select_info->shell, "response",
                    G_CALLBACK (rect_select_response),
                    rect_select);

  /*  add the information fields  */
  spinbutton = info_dialog_add_spinbutton (rect_select->rect_select_info,
                                           _("Origin X:"), NULL,
                                           -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  rect_select->origin_sizeentry =
    info_dialog_add_sizeentry (rect_select->rect_select_info,
                               _("Origin Y:"), rect_select->orig_vals, 1,
                               shell->unit, "%a",
                               TRUE, TRUE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
                               G_CALLBACK (rect_select_origin_changed),
                               rect_select);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (rect_select->origin_sizeentry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (rect_select->origin_sizeentry),
                                         0, -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (rect_select->origin_sizeentry),
                                         1, -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);

  spinbutton = info_dialog_add_spinbutton (rect_select->rect_select_info, _("Width:"), NULL,
                                            -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
  rect_select->size_sizeentry =
    info_dialog_add_sizeentry (rect_select->rect_select_info,
                               _("Height:"), rect_select->size_vals, 1,
                               shell->unit, "%a",
                               TRUE, TRUE, FALSE, GIMP_SIZE_ENTRY_UPDATE_SIZE,
                               G_CALLBACK (rect_select_size_changed),
                               rect_select);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (rect_select->size_sizeentry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (rect_select->size_sizeentry),
                                         0,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (rect_select->size_sizeentry),
                                         1,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gtk_table_set_row_spacing (GTK_TABLE (rect_select->rect_select_info->info_table), 0, 0);
  gtk_table_set_row_spacing (GTK_TABLE (rect_select->rect_select_info->info_table), 1, 6);
  gtk_table_set_row_spacing (GTK_TABLE (rect_select->rect_select_info->info_table), 2, 0);

  spinbutton = info_dialog_add_spinbutton (rect_select->rect_select_info, _("Aspect ratio:"),
                                           &rect_select->aspect_ratio,
                                           0, 65536, 0.01, 0.1, 1, 0.5, 2,
                                           G_CALLBACK (rect_select_aspect_changed),
                                           rect_select);

  /* Create the area selection buttons */
  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
  gtk_box_set_spacing (GTK_BOX (bbox), 6);

  button = gtk_button_new_with_label (_("From selection"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button , "clicked",
                    G_CALLBACK (rect_select_selection_callback),
                    rect_select);

  button = gtk_button_new_with_label (_("Auto shrink"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button , "clicked",
                    G_CALLBACK (rect_select_automatic_callback),
                    rect_select);

  gtk_box_pack_start (GTK_BOX (rect_select->rect_select_info->vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);

  gimp_dialog_factory_add_foreign (gimp_dialog_factory_from_name ("toplevel"),
                                   "gimp-crop-tool-dialog",
                                   rect_select->rect_select_info->shell);
}

static void
rect_select_info_update (GimpNewRectSelectTool *rect_select)
{
  rect_select->orig_vals[0] = rect_select->x1;
  rect_select->orig_vals[1] = rect_select->y1;
  rect_select->size_vals[0] = rect_select->x2 - rect_select->x1;
  rect_select->size_vals[1] = rect_select->y2 - rect_select->y1;

  info_dialog_update (rect_select->rect_select_info);
  info_dialog_show (rect_select->rect_select_info);
}

static void
rect_select_response (GtkWidget             *widget,
                      gint                   response_id,
                      GimpNewRectSelectTool *rect_select)
{
  GimpTool        *tool    = GIMP_TOOL (rect_select);

  switch (response_id)
    {
    case GIMP_CROP_MODE_CROP:
      if (rect_select->rect_select_info)
        {
          /* set these buttons to be insensitive so that you cannot
           * accidentially trigger a rect_select while one is ongoing */

          gtk_dialog_set_response_sensitive (GTK_DIALOG (rect_select->rect_select_info->shell),
                                             GIMP_CROP_MODE_CROP,   FALSE);
        }

      gimp_display_shell_set_highlight (GIMP_DISPLAY_SHELL (tool->gdisp->shell),
                                        NULL);

      new_rect_select_tool_rect_select (GIMP_NEW_RECT_SELECT_TOOL (tool),
                                        rect_select->x1, rect_select->y1,
                                        rect_select->x2 - rect_select->x1,
                                        rect_select->y2 - rect_select->y1);

      gimp_image_flush (tool->gdisp->gimage);
      break;

    default:
      break;
    }

  if (tool->gdisp)
    gimp_display_shell_set_highlight (GIMP_DISPLAY_SHELL (tool->gdisp->shell),
                                      NULL);

  if (rect_select->rect_select_info)
    {
      info_dialog_free (rect_select->rect_select_info);
      rect_select->rect_select_info = NULL;
    }

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (rect_select)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (rect_select));

  if (gimp_tool_control_is_active (GIMP_TOOL (rect_select)->control))
    gimp_tool_control_halt (GIMP_TOOL (rect_select)->control);

  tool->gdisp    = NULL;
  tool->drawable = NULL;
}

static void
rect_select_selection_callback (GtkWidget             *widget,
                                GimpNewRectSelectTool *rect_select)
{
  GimpSelectionOptions *options;
  GimpDisplay          *gdisp;

  options = GIMP_SELECTION_OPTIONS (GIMP_TOOL (rect_select)->tool_info->tool_options);
  gdisp   = GIMP_TOOL (rect_select)->gdisp;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (rect_select));

  if (! gimp_channel_bounds (gimp_image_get_mask (gdisp->gimage),
                             &rect_select->x1, &rect_select->y1,
                             &rect_select->x2, &rect_select->y2))
    {
      rect_select->x1 = rect_select->y1 = 0;
      rect_select->x2 = gdisp->gimage->width;
      rect_select->y2 = gdisp->gimage->height;
    }

  /* force change of aspect ratio */
  if ((rect_select->y2 - rect_select->y1) != 0)
    rect_select->aspect_ratio = ((gdouble) (rect_select->x2 - rect_select->x1) /
                          (gdouble) (rect_select->y2 - rect_select->y1));

  rect_select_recalc (rect_select, TRUE);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (rect_select));
}

static void
rect_select_automatic_callback (GtkWidget             *widget,
                                GimpNewRectSelectTool *rect_select)
{
  GimpSelectionOptions *options;
  GimpDisplay     *gdisp;
  gint             offset_x, offset_y;
  gint             width, height;
  gint             x1, y1, x2, y2;
  gint             shrunk_x1;
  gint             shrunk_y1;
  gint             shrunk_x2;
  gint             shrunk_y2;

  options = GIMP_SELECTION_OPTIONS (GIMP_TOOL (rect_select)->tool_info->tool_options);
  gdisp   = GIMP_TOOL (rect_select)->gdisp;

  width    = gdisp->gimage->width;
  height   = gdisp->gimage->height;
  offset_x = 0;
  offset_y = 0;

  x1 = rect_select->x1 - offset_x  > 0      ? rect_select->x1 - offset_x : 0;
  x2 = rect_select->x2 - offset_x  < width  ? rect_select->x2 - offset_x : width;
  y1 = rect_select->y1 - offset_y  > 0      ? rect_select->y1 - offset_y : 0;
  y2 = rect_select->y2 - offset_y  < height ? rect_select->y2 - offset_y : height;

  if (gimp_image_crop_auto_shrink (gdisp->gimage,
                                   x1, y1, x2, y2,
                                   FALSE,
                                   &shrunk_x1,
                                   &shrunk_y1,
                                   &shrunk_x2,
                                   &shrunk_y2))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rect_select));

      rect_select->x1 = offset_x + shrunk_x1;
      rect_select->x2 = offset_x + shrunk_x2;
      rect_select->y1 = offset_y + shrunk_y1;
      rect_select->y2 = offset_y + shrunk_y2;

      rect_select_recalc (rect_select, TRUE);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rect_select));
    }
}

static void
rect_select_origin_changed (GtkWidget             *widget,
                            GimpNewRectSelectTool *rect_select)
{
  gint origin_x;
  gint origin_y;

  origin_x = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0));
  origin_y = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1));

  if ((origin_x != rect_select->x1) ||
      (origin_y != rect_select->y1))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rect_select));

      rect_select->x2 = rect_select->x2 + (origin_x - rect_select->x1);
      rect_select->x1 = origin_x;
      rect_select->y2 = rect_select->y2 + (origin_y - rect_select->y1);
      rect_select->y1 = origin_y;

      rect_select_recalc (rect_select, TRUE);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rect_select));
    }
}

static void
rect_select_size_changed (GtkWidget             *widget,
                          GimpNewRectSelectTool *rect_select)
{
  gint size_x = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  gint size_y = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if ((size_x != (rect_select->x2 - rect_select->x1)) ||
      (size_y != (rect_select->y2 - rect_select->y1)))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rect_select));

      rect_select->x2 = size_x + rect_select->x1;
      rect_select->y2 = size_y + rect_select->y1;

      rect_select_recalc (rect_select, TRUE);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rect_select));
    }
}

static void
rect_select_aspect_changed (GtkWidget             *widget,
                            GimpNewRectSelectTool *rect_select)
{
  rect_select->aspect_ratio = GTK_ADJUSTMENT (widget)->value;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (rect_select));

  rect_select->y2 = rect_select->y1 + ((gdouble) (rect_select->x2 - rect_select->x1) / rect_select->aspect_ratio);

  rect_select->change_aspect_ratio = FALSE;
  rect_select_recalc (rect_select, TRUE);
  rect_select->change_aspect_ratio = TRUE;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (rect_select));
}


static void
new_rect_select_tool_rect_select (GimpNewRectSelectTool *rect_tool,
                                  gint                x,
                                  gint                y,
                                  gint                w,
                                  gint                h)
{
  GimpTool             *tool;
  GimpSelectionOptions *options;

  g_return_if_fail (GIMP_IS_NEW_RECT_SELECT_TOOL (rect_tool));

  tool    = GIMP_TOOL (rect_tool);
  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  if (options->auto_shrink)
    {
      gint off_x = 0;
      gint off_y = 0;
      gint x2, y2;

      if (! gimp_rectangle_intersect (x, y, w, h,
                                      0, 0,
                                      tool->gdisp->gimage->width,
                                      tool->gdisp->gimage->height,
                                      &x, &y, &w, &h))
        {
          return;
        }

      if (! options->shrink_merged)
        {
          GimpItem *item;
          gint      width, height;

          item = GIMP_ITEM (gimp_image_active_drawable (tool->gdisp->gimage));

          gimp_item_offsets (item, &off_x, &off_y);
          width  = gimp_item_width  (item);
          height = gimp_item_height (item);

          if (! gimp_rectangle_intersect (x, y, w, h,
                                          off_x, off_y, width, height,
                                          &x, &y, &w, &h))
            {
              return;
            }

          x -= off_x;
          y -= off_y;
        }

      if (gimp_image_crop_auto_shrink (tool->gdisp->gimage,
                                       x, y,
                                       x + w, y + h,
                                       ! options->shrink_merged,
                                       &x, &y,
                                       &x2, &y2))
        {
          w = x2 - x;
          h = y2 - y;
        }

      x += off_x;
      y += off_y;
    }

  GIMP_NEW_RECT_SELECT_TOOL_GET_CLASS (rect_tool)->rect_select (rect_tool,
                                                            x, y, w, h);
}

static void
gimp_new_rect_select_tool_real_rect_select (GimpNewRectSelectTool *rect_tool,
                                            gint                x,
                                            gint                y,
                                            gint                w,
                                            gint                h)
{
  GimpTool             *tool     = GIMP_TOOL (rect_tool);
  GimpSelectionTool    *sel_tool = GIMP_SELECTION_TOOL (rect_tool);
  GimpSelectionOptions *options;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimp_channel_select_rectangle (gimp_image_get_mask (tool->gdisp->gimage),
                                 x, y, w, h,
                                 options->operation,
                                 options->feather,
                                 options->feather_radius,
                                 options->feather_radius);
}

