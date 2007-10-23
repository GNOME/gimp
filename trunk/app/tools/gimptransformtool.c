/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/boundary.h"
#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpitem-linked.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimppickable.h"
#include "core/gimpprogress.h"
#include "core/gimptoolinfo.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpstroke.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimptooldialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"
#include "gimptransformtool.h"
#include "gimptransformtoolundo.h"

#include "gimp-intl.h"


#define HANDLE_SIZE     25
#define MIN_HANDLE_SIZE  6


/*  local function prototypes  */

static GObject * gimp_transform_tool_constructor (GType                  type,
                                                  guint                  n_params,
                                                  GObjectConstructParam *params);
static void   gimp_transform_tool_finalize       (GObject               *object);

static gboolean gimp_transform_tool_initialize   (GimpTool              *tool,
                                                  GimpDisplay           *display,
                                                  GError               **error);
static void   gimp_transform_tool_control        (GimpTool              *tool,
                                                  GimpToolAction         action,
                                                  GimpDisplay           *display);
static void   gimp_transform_tool_button_press   (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);
static void   gimp_transform_tool_button_release (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpButtonReleaseType  release_type,
                                                  GimpDisplay           *display);
static void   gimp_transform_tool_motion         (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);
static gboolean gimp_transform_tool_key_press    (GimpTool              *tool,
                                                  GdkEventKey           *kevent,
                                                  GimpDisplay           *display);
static void   gimp_transform_tool_modifier_key   (GimpTool              *tool,
                                                  GdkModifierType        key,
                                                  gboolean               press,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);
static void   gimp_transform_tool_oper_update    (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  GdkModifierType        state,
                                                  gboolean               proximity,
                                                  GimpDisplay           *display);
static void   gimp_transform_tool_cursor_update  (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);

static void   gimp_transform_tool_draw           (GimpDrawTool          *draw_tool);

static void   gimp_transform_tool_dialog_update  (GimpTransformTool     *tr_tool);

static TileManager *
              gimp_transform_tool_real_transform (GimpTransformTool     *tr_tool,
                                                  GimpItem              *item,
                                                  gboolean               mask_empty,
                                                  GimpDisplay           *display);

static void   gimp_transform_tool_halt           (GimpTransformTool     *tr_tool);
static void   gimp_transform_tool_bounds         (GimpTransformTool     *tr_tool,
                                                  GimpDisplay           *display);
static void   gimp_transform_tool_dialog         (GimpTransformTool     *tr_tool);
static void   gimp_transform_tool_prepare        (GimpTransformTool     *tr_tool,
                                                  GimpDisplay           *display);
static void   gimp_transform_tool_doit           (GimpTransformTool     *tr_tool,
                                                  GimpDisplay           *display);
static void   gimp_transform_tool_transform_bounding_box (GimpTransformTool *tr_tool);
static void   gimp_transform_tool_grid_recalc    (GimpTransformTool     *tr_tool);

static void   gimp_transform_tool_handles_recalc (GimpTransformTool     *tr_tool,
                                                  GimpDisplay           *display);
static void   gimp_transform_tool_force_expose_preview (GimpTransformTool *tr_tool);

static void   gimp_transform_tool_response       (GtkWidget             *widget,
                                                  gint                   response_id,
                                                  GimpTransformTool     *tr_tool);

static void   gimp_transform_tool_notify_type    (GimpTransformOptions  *options,
                                                  GParamSpec            *pspec,
                                                  GimpTransformTool     *tr_tool);
static void   gimp_transform_tool_notify_preview (GimpTransformOptions  *options,
                                                  GParamSpec            *pspec,
                                                  GimpTransformTool     *tr_tool);


G_DEFINE_TYPE (GimpTransformTool, gimp_transform_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_transform_tool_parent_class


static void
gimp_transform_tool_class_init (GimpTransformToolClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class   = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_class   = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructor       = gimp_transform_tool_constructor;
  object_class->finalize          = gimp_transform_tool_finalize;

  tool_class->initialize          = gimp_transform_tool_initialize;
  tool_class->control             = gimp_transform_tool_control;
  tool_class->button_press        = gimp_transform_tool_button_press;
  tool_class->button_release      = gimp_transform_tool_button_release;
  tool_class->motion              = gimp_transform_tool_motion;
  tool_class->key_press           = gimp_transform_tool_key_press;
  tool_class->modifier_key        = gimp_transform_tool_modifier_key;
  tool_class->active_modifier_key = gimp_transform_tool_modifier_key;
  tool_class->oper_update         = gimp_transform_tool_oper_update;
  tool_class->cursor_update       = gimp_transform_tool_cursor_update;

  draw_class->draw                = gimp_transform_tool_draw;

  klass->dialog                   = NULL;
  klass->dialog_update            = NULL;
  klass->prepare                  = NULL;
  klass->motion                   = NULL;
  klass->recalc                   = NULL;
  klass->transform                = gimp_transform_tool_real_transform;
}

static void
gimp_transform_tool_init (GimpTransformTool *tr_tool)
{
  GimpTool *tool;
  gint      i;

  tool = GIMP_TOOL (tr_tool);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE_SIZE |
                                     GIMP_DIRTY_DRAWABLE   |
                                     GIMP_DIRTY_SELECTION);

  tr_tool->function = TRANSFORM_CREATING;
  tr_tool->original = NULL;

  for (i = 0; i < TRANS_INFO_SIZE; i++)
    {
      tr_tool->trans_info[i]     = 0.0;
      tr_tool->old_trans_info[i] = 0.0;
    }

  gimp_matrix3_identity (&tr_tool->transform);

  tr_tool->use_grid         = FALSE;
  tr_tool->use_handles      = FALSE;
  tr_tool->use_center       = FALSE;
  tr_tool->use_mid_handles  = FALSE;

  tr_tool->handle_w         = HANDLE_SIZE;
  tr_tool->handle_h         = HANDLE_SIZE;

  tr_tool->ngx              = 0;
  tr_tool->ngy              = 0;
  tr_tool->grid_coords      = NULL;
  tr_tool->tgrid_coords     = NULL;

  tr_tool->type             = GIMP_TRANSFORM_TYPE_LAYER;
  tr_tool->direction        = GIMP_TRANSFORM_FORWARD;

  tr_tool->undo_desc        = NULL;

  tr_tool->progress_text    = _("Transforming");
  tr_tool->dialog           = NULL;
}

static GObject *
gimp_transform_tool_constructor (GType                  type,
                                 guint                  n_params,
                                 GObjectConstructParam *params)
{
  GObject              *object;
  GimpTool             *tool;
  GimpTransformTool    *tr_tool;
  GimpTransformOptions *options;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tool    = GIMP_TOOL (object);
  tr_tool = GIMP_TRANSFORM_TOOL (object);
  options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);

  if (tr_tool->use_grid)
    {
      tr_tool->type      = options->type;
      tr_tool->direction = options->direction;

      g_signal_connect_object (options, "notify::type",
                               G_CALLBACK (gimp_transform_tool_notify_type),
                               tr_tool, 0);
      g_signal_connect_object (options, "notify::type",
                               G_CALLBACK (gimp_transform_tool_notify_preview),
                               tr_tool, 0);

      g_signal_connect_object (options, "notify::direction",
                               G_CALLBACK (gimp_transform_tool_notify_type),
                               tr_tool, 0);
      g_signal_connect_object (options, "notify::direction",
                               G_CALLBACK (gimp_transform_tool_notify_preview),
                               tr_tool, 0);

      g_signal_connect_object (options, "notify::preview-type",
                               G_CALLBACK (gimp_transform_tool_notify_preview),
                               tr_tool, 0);
      g_signal_connect_object (options, "notify::grid-type",
                               G_CALLBACK (gimp_transform_tool_notify_preview),
                               tr_tool, 0);
      g_signal_connect_object (options, "notify::grid-size",
                               G_CALLBACK (gimp_transform_tool_notify_preview),
                               tr_tool, 0);
    }

  g_signal_connect_object (options, "notify::constrain",
                           G_CALLBACK (gimp_transform_tool_dialog_update),
                           tr_tool, G_CONNECT_SWAPPED);

  return object;
}

static void
gimp_transform_tool_finalize (GObject *object)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (object);

  if (tr_tool->original)
    {
      tile_manager_unref (tr_tool->original);
      tr_tool->original = NULL;
    }

  if (tr_tool->dialog)
    {
      gtk_widget_destroy (tr_tool->dialog);
      tr_tool->dialog = NULL;
    }

  if (tr_tool->grid_coords)
    {
      g_free (tr_tool->grid_coords);
      tr_tool->grid_coords = NULL;
    }

  if (tr_tool->tgrid_coords)
    {
      g_free (tr_tool->tgrid_coords);
      tr_tool->tgrid_coords = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_transform_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  if (display != tool->display)
    {
      gint i;

      /*  Set the pointer to the active display  */
      tool->display  = display;
      tool->drawable = gimp_image_get_active_drawable (display->image);

      /*  Initialize the transform tool dialog */
      if (! tr_tool->dialog)
        gimp_transform_tool_dialog (tr_tool);

      /*  Find the transform bounds for some tools (like scale,
       *  perspective) that actually need the bounds for
       *  initializing
       */
      gimp_transform_tool_bounds (tr_tool, display);

      gimp_transform_tool_prepare (tr_tool, display);

      /*  Recalculate the transform tool  */
      gimp_transform_tool_recalc (tr_tool, display);

      /*  start drawing the bounding box and handles...  */
      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

      tr_tool->function = TRANSFORM_CREATING;

      /*  Save the current transformation info  */
      for (i = 0; i < TRANS_INFO_SIZE; i++)
        tr_tool->old_trans_info[i] = tr_tool->trans_info[i];
    }

  return TRUE;
}

static void
gimp_transform_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;

    case GIMP_TOOL_ACTION_RESUME:
      gimp_transform_tool_bounds (tr_tool, display);
      gimp_transform_tool_recalc (tr_tool, display);
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_transform_tool_halt (tr_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_transform_tool_button_press (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  if (tr_tool->function == TRANSFORM_CREATING)
    gimp_transform_tool_oper_update (tool, coords, state, TRUE, display);

  tr_tool->lastx = tr_tool->startx = coords->x;
  tr_tool->lasty = tr_tool->starty = coords->y;

  gimp_tool_control_activate (tool->control);
}

static void
gimp_transform_tool_button_release (GimpTool              *tool,
                                    GimpCoords            *coords,
                                    guint32                time,
                                    GdkModifierType        state,
                                    GimpButtonReleaseType  release_type,
                                    GimpDisplay           *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  gint               i;

  /*  if we are creating, there is nothing to be done...exit  */
  if (tr_tool->function == TRANSFORM_CREATING && tr_tool->use_grid)
    return;

  if (release_type != GIMP_BUTTON_RELEASE_CANCEL)
    {
      /* Shift-clicking is another way to approve the transform  */
      if ((state & GDK_SHIFT_MASK) || ! tr_tool->use_grid)
        {
          gimp_transform_tool_response (NULL, GTK_RESPONSE_OK, tr_tool);
        }
    }
  else
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      /* get rid of preview artifacts left outside the drawable's area */
      gimp_transform_tool_expose_preview (tr_tool);

      /*  Restore the previous transformation info  */
      for (i = 0; i < TRANS_INFO_SIZE; i++)
        tr_tool->trans_info[i] = tr_tool->old_trans_info[i];

      /*  reget the selection bounds  */
      gimp_transform_tool_bounds (tr_tool, display);

      /*  recalculate the tool's transformation matrix  */
      gimp_transform_tool_recalc (tr_tool, display);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }

  gimp_tool_control_halt (tool->control);
}

static void
gimp_transform_tool_motion (GimpTool        *tool,
                            GimpCoords      *coords,
                            guint32          time,
                            GdkModifierType  state,
                            GimpDisplay     *display)
{
  GimpTransformTool      *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  GimpTransformToolClass *tr_tool_class;

  /*  if we are creating, there is nothing to be done so exit.  */
  if (tr_tool->function == TRANSFORM_CREATING || ! tr_tool->use_grid)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  tr_tool->curx  = coords->x;
  tr_tool->cury  = coords->y;
  tr_tool->state = state;

  /*  recalculate the tool's transformation matrix  */
  tr_tool_class = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool);

  if (tr_tool_class->motion)
    {
      tr_tool_class->motion (tr_tool, display);

      gimp_transform_tool_expose_preview (tr_tool);

      gimp_transform_tool_recalc (tr_tool, display);
    }

  tr_tool->lastx = tr_tool->curx;
  tr_tool->lasty = tr_tool->cury;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

#define RESPONSE_RESET 1

static gboolean
gimp_transform_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpTransformTool *trans_tool = GIMP_TRANSFORM_TOOL (tool);
  GimpDrawTool      *draw_tool  = GIMP_DRAW_TOOL (tool);

  if (display == draw_tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KP_Enter:
        case GDK_Return:
          gimp_transform_tool_response (NULL, GTK_RESPONSE_OK, trans_tool);
          return TRUE;

        case GDK_Delete:
        case GDK_BackSpace:
          gimp_transform_tool_response (NULL, RESPONSE_RESET, trans_tool);
          return TRUE;

        case GDK_Escape:
          gimp_transform_tool_response (NULL, GTK_RESPONSE_CANCEL, trans_tool);
          return TRUE;
        }
    }

  return FALSE;
}

static void
gimp_transform_tool_modifier_key (GimpTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  GimpDisplay     *display)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);

  if (key == GDK_CONTROL_MASK)
    g_object_set (options,
                  "constrain", ! options->constrain,
                  NULL);
}

static void
gimp_transform_tool_oper_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 gboolean         proximity,
                                 GimpDisplay     *display)
{
  GimpTransformTool *tr_tool   = GIMP_TRANSFORM_TOOL (tool);
  GimpDrawTool      *draw_tool = GIMP_DRAW_TOOL (tool);

  tr_tool->function = TRANSFORM_HANDLE_NONE;

  if (display != tool->display)
    return;

  if (tr_tool->use_handles)
    {
      gdouble closest_dist;
      gdouble dist;

      dist = gimp_draw_tool_calc_distance_square (draw_tool, display,
                                                  coords->x, coords->y,
                                                  tr_tool->tx1, tr_tool->ty1);
      closest_dist = dist;
      tr_tool->function = TRANSFORM_HANDLE_NW;

      dist = gimp_draw_tool_calc_distance_square (draw_tool, display,
                                                  coords->x, coords->y,
                                                  tr_tool->tx2, tr_tool->ty2);
      if (dist < closest_dist)
        {
          closest_dist = dist;
          tr_tool->function = TRANSFORM_HANDLE_NE;
        }

      dist = gimp_draw_tool_calc_distance_square (draw_tool, display,
                                                  coords->x, coords->y,
                                                  tr_tool->tx3, tr_tool->ty3);
      if (dist < closest_dist)
        {
          closest_dist = dist;
          tr_tool->function = TRANSFORM_HANDLE_SW;
        }

      dist = gimp_draw_tool_calc_distance_square (draw_tool, display,
                                                  coords->x, coords->y,
                                                  tr_tool->tx4, tr_tool->ty4);
      if (dist < closest_dist)
        {
          closest_dist = dist;
          tr_tool->function = TRANSFORM_HANDLE_SE;
        }

      if (tr_tool->use_mid_handles)
        {
          gdouble x, y;

          x = (tr_tool->tx1 + tr_tool->tx2) / 2.0;
          y = (tr_tool->ty1 + tr_tool->ty2) / 2.0;

          if (gimp_draw_tool_on_handle (draw_tool, display,
                                        coords->x, coords->y,
                                        GIMP_HANDLE_SQUARE,
                                        x, y,
                                        tr_tool->handle_w, tr_tool->handle_h,
                                        GTK_ANCHOR_CENTER,
                                        FALSE))
            {
              tr_tool->function = TRANSFORM_HANDLE_N;
            }

          x = (tr_tool->tx2 + tr_tool->tx4) / 2.0;
          y = (tr_tool->ty2 + tr_tool->ty4) / 2.0;

          if (gimp_draw_tool_on_handle (draw_tool, display,
                                        coords->x, coords->y,
                                        GIMP_HANDLE_SQUARE,
                                        x, y,
                                        tr_tool->handle_w, tr_tool->handle_h,
                                        GTK_ANCHOR_CENTER,
                                        FALSE))
            {
              tr_tool->function = TRANSFORM_HANDLE_E;
            }

          x = (tr_tool->tx3 + tr_tool->tx4) / 2.0;
          y = (tr_tool->ty3 + tr_tool->ty4) / 2.0;

          if (gimp_draw_tool_on_handle (draw_tool, display,
                                        coords->x, coords->y,
                                        GIMP_HANDLE_SQUARE,
                                        x, y,
                                        tr_tool->handle_w, tr_tool->handle_h,
                                        GTK_ANCHOR_CENTER,
                                        FALSE))
            {
              tr_tool->function = TRANSFORM_HANDLE_S;
            }

          x = (tr_tool->tx3 + tr_tool->tx1) / 2.0;
          y = (tr_tool->ty3 + tr_tool->ty1) / 2.0;

          if (gimp_draw_tool_on_handle (draw_tool, display,
                                        coords->x, coords->y,
                                        GIMP_HANDLE_SQUARE,
                                        x, y,
                                        tr_tool->handle_w, tr_tool->handle_h,
                                        GTK_ANCHOR_CENTER,
                                        FALSE))
            {
              tr_tool->function = TRANSFORM_HANDLE_W;
            }
        }
    }

  if (tr_tool->use_center &&
      gimp_draw_tool_on_handle (draw_tool, display,
                                coords->x, coords->y,
                                GIMP_HANDLE_CIRCLE,
                                tr_tool->tcx, tr_tool->tcy,
                                MIN (tr_tool->handle_w, tr_tool->handle_h),
                                MIN (tr_tool->handle_w, tr_tool->handle_h),
                                GTK_ANCHOR_CENTER,
                                FALSE))
    {
      tr_tool->function = TRANSFORM_HANDLE_CENTER;
    }
}

static void
gimp_transform_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *display)
{
  GimpTransformTool    *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpCursorType        cursor;
  GimpCursorModifier    modifier = GIMP_CURSOR_MODIFIER_NONE;

  cursor = gimp_tool_control_get_cursor (tool->control);

  if (tr_tool->use_handles)
    {
      switch (tr_tool->function)
        {
        case TRANSFORM_HANDLE_NW:
          cursor = GIMP_CURSOR_CORNER_TOP_LEFT;
          break;

        case TRANSFORM_HANDLE_NE:
          cursor = GIMP_CURSOR_CORNER_TOP_RIGHT;
          break;

        case TRANSFORM_HANDLE_SW:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_LEFT;
          break;

        case TRANSFORM_HANDLE_SE:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
          break;

        case TRANSFORM_HANDLE_N:
          cursor = GIMP_CURSOR_SIDE_TOP;
          break;

        case TRANSFORM_HANDLE_E:
          cursor = GIMP_CURSOR_SIDE_RIGHT;
          break;

        case TRANSFORM_HANDLE_S:
          cursor = GIMP_CURSOR_SIDE_BOTTOM;
          break;

        case TRANSFORM_HANDLE_W:
          cursor = GIMP_CURSOR_SIDE_LEFT;
          break;

        default:
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
          break;
        }
    }

  if (tr_tool->use_center && tr_tool->function == TRANSFORM_HANDLE_CENTER)
    {
      modifier = GIMP_CURSOR_MODIFIER_MOVE;
    }

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
    case GIMP_TRANSFORM_TYPE_SELECTION:
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      if (! gimp_image_get_active_vectors (display->image))
        modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_transform_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool          *tool    = GIMP_TOOL (draw_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (draw_tool);
  gdouble            z1, z2, z3, z4;

  if (tr_tool->use_grid)
    {
      /*  draw the bounding box  */
      gimp_draw_tool_draw_line (draw_tool,
                                tr_tool->tx1, tr_tool->ty1,
                                tr_tool->tx2, tr_tool->ty2,
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                tr_tool->tx2, tr_tool->ty2,
                                tr_tool->tx4, tr_tool->ty4,
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                tr_tool->tx3, tr_tool->ty3,
                                tr_tool->tx4, tr_tool->ty4,
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                tr_tool->tx3, tr_tool->ty3,
                                tr_tool->tx1, tr_tool->ty1,
                                FALSE);

      /* We test if the transformed polygon is convex.
       * if z1 and z2 have the same sign as well as z3 and z4
       * the polygon is convex.
       */
      z1 = ((tr_tool->tx2 - tr_tool->tx1) * (tr_tool->ty4 - tr_tool->ty1) -
            (tr_tool->tx4 - tr_tool->tx1) * (tr_tool->ty2 - tr_tool->ty1));
      z2 = ((tr_tool->tx4 - tr_tool->tx1) * (tr_tool->ty3 - tr_tool->ty1) -
            (tr_tool->tx3 - tr_tool->tx1) * (tr_tool->ty4 - tr_tool->ty1));
      z3 = ((tr_tool->tx4 - tr_tool->tx2) * (tr_tool->ty3 - tr_tool->ty2) -
            (tr_tool->tx3 - tr_tool->tx2) * (tr_tool->ty4 - tr_tool->ty2));
      z4 = ((tr_tool->tx3 - tr_tool->tx2) * (tr_tool->ty1 - tr_tool->ty2) -
            (tr_tool->tx1 - tr_tool->tx2) * (tr_tool->ty3 - tr_tool->ty2));

      /*  draw the grid  */
      if (tr_tool->grid_coords  &&
          tr_tool->tgrid_coords &&
          z1 * z2 > 0           &&
          z3 * z4 > 0)
        {
          gint gci, i, k;

          k = tr_tool->ngx + tr_tool->ngy;

          for (i = 0, gci = 0; i < k; i++, gci += 4)
            {
              gimp_draw_tool_draw_line (draw_tool,
                                        tr_tool->tgrid_coords[gci],
                                        tr_tool->tgrid_coords[gci + 1],
                                        tr_tool->tgrid_coords[gci + 2],
                                        tr_tool->tgrid_coords[gci + 3],
                                        FALSE);
            }
        }
    }

  gimp_transform_tool_handles_recalc (tr_tool, tool->display);

  if (tr_tool->use_handles)
    {
      /*  draw the tool handles  */
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_SQUARE,
                                  tr_tool->tx1, tr_tool->ty1,
                                  tr_tool->handle_w, tr_tool->handle_h,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_SQUARE,
                                  tr_tool->tx2, tr_tool->ty2,
                                  tr_tool->handle_w, tr_tool->handle_h,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_SQUARE,
                                  tr_tool->tx3, tr_tool->ty3,
                                  tr_tool->handle_w, tr_tool->handle_h,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_SQUARE,
                                  tr_tool->tx4, tr_tool->ty4,
                                  tr_tool->handle_w, tr_tool->handle_h,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);

      if (tr_tool->use_mid_handles)
        {
          gdouble x, y;

          x = (tr_tool->tx1 + tr_tool->tx2) / 2.0;
          y = (tr_tool->ty1 + tr_tool->ty2) / 2.0;

          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_SQUARE,
                                      x, y,
                                      tr_tool->handle_w, tr_tool->handle_h,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);

          x = (tr_tool->tx2 + tr_tool->tx4) / 2.0;
          y = (tr_tool->ty2 + tr_tool->ty4) / 2.0;

          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_SQUARE,
                                      x, y,
                                      tr_tool->handle_w, tr_tool->handle_h,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);

          x = (tr_tool->tx3 + tr_tool->tx4) / 2.0;
          y = (tr_tool->ty3 + tr_tool->ty4) / 2.0;

          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_SQUARE,
                                      x, y,
                                      tr_tool->handle_w, tr_tool->handle_h,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);

          x = (tr_tool->tx3 + tr_tool->tx1) / 2.0;
          y = (tr_tool->ty3 + tr_tool->ty1) / 2.0;

          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_SQUARE,
                                      x, y,
                                      tr_tool->handle_w, tr_tool->handle_h,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);
        }
    }

  /*  draw the center  */
  if (tr_tool->use_center)
    {
      gint d = MIN (tr_tool->handle_w, tr_tool->handle_h);

      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_CIRCLE,
                                  tr_tool->tcx, tr_tool->tcy,
                                  d, d,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_CROSS,
                                  tr_tool->tcx, tr_tool->tcy,
                                  d, d,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);
    }

  if (tr_tool->type == GIMP_TRANSFORM_TYPE_SELECTION)
    {
      GimpMatrix3     matrix = tr_tool->transform;
      const BoundSeg *orig_in;
      const BoundSeg *orig_out;
      BoundSeg       *segs_in;
      BoundSeg       *segs_out;
      gint            num_segs_in;
      gint            num_segs_out;
      gint            num_groups;
      gint            i;

      gimp_channel_boundary (gimp_image_get_mask (tool->display->image),
                             &orig_in, &orig_out,
                             &num_segs_in, &num_segs_out,
                             0, 0, 0, 0);

      segs_in = boundary_sort (orig_in, num_segs_in, &num_groups);
      num_segs_in += num_groups;

      segs_out = boundary_sort (orig_out, num_segs_out, &num_groups);
      num_segs_out += num_groups;

      if (segs_in)
        {
          for (i = 0; i < num_segs_in; i++)
            {
              gdouble tx, ty;

              if (segs_in[i].x1 != -1 &&
                  segs_in[i].y1 != -1 &&
                  segs_in[i].x2 != -1 &&
                  segs_in[i].y2 != -1)
                {
                  gimp_matrix3_transform_point (&matrix,
                                                segs_in[i].x1, segs_in[i].y1,
                                                &tx, &ty);
                  segs_in[i].x1 = RINT (tx);
                  segs_in[i].y1 = RINT (ty);

                  gimp_matrix3_transform_point (&matrix,
                                                segs_in[i].x2, segs_in[i].y2,
                                                &tx, &ty);
                  segs_in[i].x2 = RINT (tx);
                  segs_in[i].y2 = RINT (ty);
                }
            }

          gimp_draw_tool_draw_boundary (draw_tool,
                                        segs_in, num_segs_in,
                                        0, 0,
                                        FALSE);
          g_free (segs_in);
        }

      if (segs_out)
        {
          for (i = 0; i < num_segs_out; i++)
            {
              gdouble tx, ty;

              if (segs_out[i].x1 != -1 &&
                  segs_out[i].y1 != -1 &&
                  segs_out[i].x2 != -1 &&
                  segs_out[i].y2 != -1)
                {
                  gimp_matrix3_transform_point (&matrix,
                                                segs_out[i].x1, segs_out[i].y1,
                                                &tx, &ty);
                  segs_out[i].x1 = RINT (tx);
                  segs_out[i].y1 = RINT (ty);

                  gimp_matrix3_transform_point (&matrix,
                                                segs_out[i].x2, segs_out[i].y2,
                                                &tx, &ty);
                  segs_out[i].x2 = RINT (tx);
                  segs_out[i].y2 = RINT (ty);
                }
            }

          gimp_draw_tool_draw_boundary (draw_tool,
                                        segs_out, num_segs_out,
                                        0, 0,
                                        FALSE);
          g_free (segs_out);
        }
    }
  else if (tr_tool->type == GIMP_TRANSFORM_TYPE_PATH)
    {
      GimpVectors *vectors;
      GimpStroke  *stroke = NULL;
      GimpMatrix3  matrix = tr_tool->transform;

      vectors = gimp_image_get_active_vectors (tool->display->image);

      if (vectors)
        {
          if (tr_tool->direction == GIMP_TRANSFORM_BACKWARD)
            gimp_matrix3_invert (&matrix);

          while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
            {
              GArray   *coords;
              gboolean  closed;

              coords = gimp_stroke_interpolate (stroke, 1.0, &closed);

              if (coords && coords->len)
                {
                  gint i;

                  for (i = 0; i < coords->len; i++)
                    {
                      GimpCoords *curr = &g_array_index (coords, GimpCoords, i);

                      gimp_matrix3_transform_point (&matrix,
                                                    curr->x, curr->y,
                                                    &curr->x, &curr->y);
                    }

                  gimp_draw_tool_draw_strokes (draw_tool,
                                               &g_array_index (coords,
                                                               GimpCoords, 0),
                                               coords->len, FALSE, FALSE);
                }

              if (coords)
                g_array_free (coords, TRUE);
            }
        }
    }
}

static void
gimp_transform_tool_dialog_update (GimpTransformTool *tr_tool)
{
  if (tr_tool->dialog &&
      GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog_update)
    {
      GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog_update (tr_tool);
    }
}

static TileManager *
gimp_transform_tool_real_transform (GimpTransformTool *tr_tool,
                                    GimpItem          *active_item,
                                    gboolean           mask_empty,
                                    GimpDisplay       *display)
{
  GimpTool             *tool    = GIMP_TOOL (tr_tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpContext          *context = GIMP_CONTEXT (options);
  GimpProgress         *progress;
  TileManager          *ret  = NULL;

  if (tr_tool->dialog)
    gtk_widget_set_sensitive (tr_tool->dialog, FALSE);

  progress = gimp_progress_start (GIMP_PROGRESS (display),
                                  tr_tool->progress_text, FALSE);

  if (gimp_item_get_linked (active_item))
    gimp_item_linked_transform (active_item, context,
                                &tr_tool->transform,
                                options->direction,
                                options->interpolation,
                                options->recursion_level,
                                options->clip,
                                progress);

  if (GIMP_IS_LAYER (active_item) &&
      gimp_layer_get_mask (GIMP_LAYER (active_item)) &&
      mask_empty)
    {
      GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (active_item));

      gimp_item_transform (GIMP_ITEM (mask), context,
                           &tr_tool->transform,
                           options->direction,
                           options->interpolation,
                           options->recursion_level,
                           options->clip,
                           progress);
    }

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
    case GIMP_TRANSFORM_TYPE_SELECTION:
      {
        GimpTransformResize clip_result = options->clip;

        /*  always clip the selction and unfloated channels
         *  so they keep their size
         */
        if (tr_tool->original)
          {
            if (GIMP_IS_CHANNEL (active_item) &&
                tile_manager_bpp (tr_tool->original) == 1)
              clip_result = GIMP_TRANSFORM_RESIZE_CLIP;

            ret =
              gimp_drawable_transform_tiles_affine (GIMP_DRAWABLE (active_item),
                                                    context,
                                                    tr_tool->original,
                                                    &tr_tool->transform,
                                                    options->direction,
                                                    options->interpolation,
                                                    options->recursion_level,
                                                    clip_result,
                                                    progress);
          }
      }
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      gimp_item_transform (active_item, context,
                           &tr_tool->transform,
                           options->direction,
                           options->interpolation,
                           options->recursion_level,
                           options->clip,
                           progress);
      break;
    }

  if (progress)
    gimp_progress_end (progress);

  return ret;
}

static void
gimp_transform_tool_doit (GimpTransformTool *tr_tool,
                          GimpDisplay       *display)
{
  GimpTool             *tool        = GIMP_TOOL (tr_tool);
  GimpTransformOptions *options     = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpContext          *context     = GIMP_CONTEXT (options);
  GimpDisplayShell     *shell       = GIMP_DISPLAY_SHELL (display->shell);
  GimpItem             *active_item = NULL;
  TileManager          *new_tiles;
  const gchar          *message     = NULL;
  gboolean              new_layer;
  gboolean              mask_empty;

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      active_item = GIMP_ITEM (gimp_image_get_active_drawable (display->image));
      message = _("There is no layer to transform.");
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      active_item = GIMP_ITEM (gimp_image_get_mask (display->image));
      /* cannot happen, so don't translate this message */
      message = "There is no selection to transform.";
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      active_item = GIMP_ITEM (gimp_image_get_active_vectors (display->image));
      message = _("There is no path to transform.");
      break;
    }

  if (! active_item)
    {
      gimp_tool_message (tool, display, message);
      return;
    }

  mask_empty = gimp_channel_is_empty (gimp_image_get_mask (display->image));

  if (gimp_display_shell_get_show_transform (shell))
    {
      gimp_display_shell_set_show_transform (shell, FALSE);

      /* get rid of preview artifacts left outside the drawable's area */
      gtk_widget_queue_draw (shell->canvas);
    }

  gimp_set_busy (display->image->gimp);

  /* undraw the tool before we muck around with the transform matrix */
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tr_tool));

  /*  We're going to dirty this image, but we want to keep the tool around  */
  gimp_tool_control_set_preserve (tool->control, TRUE);

  /*  Start a transform undo group  */
  gimp_image_undo_group_start (display->image, GIMP_UNDO_GROUP_TRANSFORM,
                               tr_tool->undo_desc);

  /* With the old UI, if original is NULL, then this is the
   * first transformation. In the new UI, it is always so, right?
   */
  g_assert (tr_tool->original == NULL);

  /*  Copy the current selection to the transform tool's private
   *  selection pointer, so that the original source can be repeatedly
   *  modified.
   */
  tool->drawable = gimp_image_get_active_drawable (display->image);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      tr_tool->original = gimp_drawable_transform_cut (tool->drawable,
                                                       context,
                                                       &new_layer);
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      tr_tool->original = tile_manager_ref (GIMP_DRAWABLE (active_item)->tiles);
      tile_manager_set_offsets (tr_tool->original, 0, 0);
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      tr_tool->original = NULL;
      break;
    }

  /*  Send the request for the transformation to the tool...
   */
  new_tiles = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->transform (tr_tool,
                                                                  active_item,
                                                                  mask_empty,
                                                                  display);

  gimp_transform_tool_bounds (tr_tool, display);
  gimp_transform_tool_prepare (tr_tool, display);
  gimp_transform_tool_recalc (tr_tool, display);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      if (new_tiles)
        {
          /*  paste the new transformed image to the image...also implement
           *  undo...
           */
          gimp_drawable_transform_paste (tool->drawable,
                                         new_tiles,
                                         new_layer);
          tile_manager_unref (new_tiles);
        }
      break;

     case GIMP_TRANSFORM_TYPE_SELECTION:
      if (new_tiles)
        {
          gimp_channel_push_undo (GIMP_CHANNEL (active_item), NULL);

          gimp_drawable_set_tiles (GIMP_DRAWABLE (active_item),
                                   FALSE, NULL, new_tiles);
          tile_manager_unref (new_tiles);
        }

      tile_manager_unref (tr_tool->original);
      tr_tool->original = NULL;
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      /*  Nothing to be done  */
      break;
    }

  /*  Make a note of the new current drawable (since we may have
   *  a floating selection, etc now.
   */
  tool->drawable = gimp_image_get_active_drawable (display->image);

  gimp_image_undo_push (display->image, GIMP_TYPE_TRANSFORM_TOOL_UNDO,
                        GIMP_UNDO_TRANSFORM, NULL,
                        0,
                        "transform-tool", tr_tool,
                        NULL);

  /*  push the undo group end  */
  gimp_image_undo_group_end (display->image);

  /*  We're done dirtying the image, and would like to be restarted
   *  if the image gets dirty while the tool exists
   */
  gimp_tool_control_set_preserve (tool->control, FALSE);

  gimp_unset_busy (display->image->gimp);

  gimp_image_flush (display->image);

  gimp_transform_tool_halt (tr_tool);
}

static void
gimp_transform_tool_transform_bounding_box (GimpTransformTool *tr_tool)
{
  g_return_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool));

  gimp_matrix3_transform_point (&tr_tool->transform,
                                tr_tool->x1, tr_tool->y1,
                                &tr_tool->tx1, &tr_tool->ty1);
  gimp_matrix3_transform_point (&tr_tool->transform,
                                tr_tool->x2, tr_tool->y1,
                                &tr_tool->tx2, &tr_tool->ty2);
  gimp_matrix3_transform_point (&tr_tool->transform,
                                tr_tool->x1, tr_tool->y2,
                                &tr_tool->tx3, &tr_tool->ty3);
  gimp_matrix3_transform_point (&tr_tool->transform,
                                tr_tool->x2, tr_tool->y2,
                                &tr_tool->tx4, &tr_tool->ty4);

  gimp_matrix3_transform_point (&tr_tool->transform,
                                tr_tool->cx, tr_tool->cy,
                                &tr_tool->tcx, &tr_tool->tcy);

  if (tr_tool->grid_coords && tr_tool->tgrid_coords)
    {
      gint i, k;
      gint gci;

      gci = 0;
      k   = (tr_tool->ngx + tr_tool->ngy) * 2;

      for (i = 0; i < k; i++)
        {
          gimp_matrix3_transform_point (&tr_tool->transform,
                                        tr_tool->grid_coords[gci],
                                        tr_tool->grid_coords[gci + 1],
                                        &tr_tool->tgrid_coords[gci],
                                        &tr_tool->tgrid_coords[gci + 1]);
          gci += 2;
        }
    }
}

void
gimp_transform_tool_expose_preview (GimpTransformTool *tr_tool)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  if ((options->preview_type == GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE ||
       options->preview_type == GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE_GRID) &&
      options->type         == GIMP_TRANSFORM_TYPE_LAYER &&
      options->direction    == GIMP_TRANSFORM_FORWARD)
    gimp_transform_tool_force_expose_preview (tr_tool);
}

static void
gimp_transform_tool_force_expose_preview (GimpTransformTool *tr_tool)
{
  static gint       last_x = 0;
  static gint       last_y = 0;
  static gint       last_w = 0;
  static gint       last_h = 0;

  GimpDisplayShell *shell;
  gdouble           dx [4], dy [4];
  gint              area_x, area_y, area_w, area_h;
  gint              i;

  g_return_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool));

  if (! tr_tool->use_grid)
    return;

  /*  we might be called as the result of cancelling the transform
   *  tool dialog, return silently because the draw tool may have
   *  already been stopped by gimp_transform_tool_halt()
   */
  if (! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tr_tool)))
    return;

  shell = GIMP_DISPLAY_SHELL (GIMP_DRAW_TOOL (tr_tool)->display->shell);

  gimp_display_shell_transform_xy_f (shell, tr_tool->tx1, tr_tool->ty1,
                                     dx + 0, dy + 0, FALSE);
  gimp_display_shell_transform_xy_f (shell, tr_tool->tx2, tr_tool->ty2,
                                     dx + 1, dy + 1, FALSE);
  gimp_display_shell_transform_xy_f (shell, tr_tool->tx3, tr_tool->ty3,
                                     dx + 2, dy + 2, FALSE);
  gimp_display_shell_transform_xy_f (shell, tr_tool->tx4, tr_tool->ty4,
                                     dx + 3, dy + 3, FALSE);

  /* find bounding box around preview */
  area_x = area_w = (gint) dx [0];
  area_y = area_h = (gint) dy [0];

  for (i = 1; i < 4; i++)
    {
      if (dx [i] < area_x)
        area_x = (gint) dx [i];
      else if (dx [i] > area_w)
        area_w = (gint) dx [i];

      if (dy [i] < area_y)
        area_y = (gint) dy [i];
      else if (dy [i] > area_h)
        area_h = (gint) dy [i];
    }

  area_w -= area_x;
  area_h -= area_y;

  gimp_display_shell_expose_area (shell,
                                  MIN (area_x, last_x),
                                  MIN (area_y, last_y),
                                  MAX (area_w, last_w) + ABS (last_x - area_x),
                                  MAX (area_h, last_h) + ABS (last_y - area_y));

  /* area of last preview must be re-exposed to avoid leaving artifacts */
  last_x = area_x;
  last_y = area_y;
  last_w = area_w;
  last_h = area_h;
}

static void
gimp_transform_tool_halt (GimpTransformTool *tr_tool)
{
  GimpTool *tool = GIMP_TOOL (tr_tool);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tr_tool)))
    {
      GimpDisplayShell *shell;

      shell = GIMP_DISPLAY_SHELL (GIMP_DRAW_TOOL (tr_tool)->display->shell);

      if (gimp_display_shell_get_show_transform (shell))
        {
          gimp_display_shell_set_show_transform (shell, FALSE);

          /* get rid of preview artifacts left outside the drawable's area */
          gimp_transform_tool_expose_preview (tr_tool);
        }
    }

  if (tr_tool->original)
    {
      tile_manager_unref (tr_tool->original);
      tr_tool->original = NULL;
    }

  /*  inactivate the tool  */
  tr_tool->function = TRANSFORM_CREATING;

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tr_tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tr_tool));

  if (tr_tool->dialog)
    gimp_dialog_factory_hide_dialog (tr_tool->dialog);

  tool->display  = NULL;
  tool->drawable = NULL;
}

static void
gimp_transform_tool_bounds (GimpTransformTool *tr_tool,
                            GimpDisplay       *display)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  g_return_if_fail (GIMP_IS_DISPLAY (display));

  /*  find the boundaries  */
  if (tr_tool->original)
    {
      tile_manager_get_offsets (tr_tool->original, &tr_tool->x1, &tr_tool->y1);

      tr_tool->x2 = tr_tool->x1 + tile_manager_width (tr_tool->original);
      tr_tool->y2 = tr_tool->y1 + tile_manager_height (tr_tool->original);
    }
  else
    {
      switch (options->type)
        {
        case GIMP_TRANSFORM_TYPE_LAYER:
          {
            GimpDrawable *drawable;
            gint          offset_x;
            gint          offset_y;

            drawable = gimp_image_get_active_drawable (display->image);

            gimp_item_offsets (GIMP_ITEM (drawable), &offset_x, &offset_y);

            gimp_drawable_mask_bounds (drawable,
                                       &tr_tool->x1, &tr_tool->y1,
                                       &tr_tool->x2, &tr_tool->y2);
            tr_tool->x1 += offset_x;
            tr_tool->y1 += offset_y;
            tr_tool->x2 += offset_x;
            tr_tool->y2 += offset_y;
          }
          break;

        case GIMP_TRANSFORM_TYPE_SELECTION:
        case GIMP_TRANSFORM_TYPE_PATH:
          gimp_channel_bounds (gimp_image_get_mask (display->image),
                               &tr_tool->x1, &tr_tool->y1,
                               &tr_tool->x2, &tr_tool->y2);
          break;
        }
    }

  tr_tool->cx = (gdouble) (tr_tool->x1 + tr_tool->x2) / 2.0;
  tr_tool->cy = (gdouble) (tr_tool->y1 + tr_tool->y2) / 2.0;

  tr_tool->aspect = ((gdouble) (tr_tool->x2 - tr_tool->x1) /
                     (gdouble) (tr_tool->y2 - tr_tool->y1));

  /*  changing the bounds invalidates any grid we may have  */
  if (tr_tool->use_grid)
    gimp_transform_tool_grid_recalc (tr_tool);
}

static void
gimp_transform_tool_grid_recalc (GimpTransformTool *tr_tool)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  if (tr_tool->grid_coords != NULL)
    {
      g_free (tr_tool->grid_coords);
      tr_tool->grid_coords = NULL;
    }

  if (tr_tool->tgrid_coords != NULL)
    {
      g_free (tr_tool->tgrid_coords);
      tr_tool->tgrid_coords = NULL;
    }

  if (options->preview_type != GIMP_TRANSFORM_PREVIEW_TYPE_GRID &&
      options->preview_type != GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE_GRID)
    return;

  switch (options->grid_type)
    {
    case GIMP_TRANSFORM_GRID_TYPE_N_LINES:
    case GIMP_TRANSFORM_GRID_TYPE_SPACING:
      {
        GimpTool *tool;
        gint      i, gci;
        gdouble  *coords;
        gint      width, height;

        width  = MAX (1, tr_tool->x2 - tr_tool->x1);
        height = MAX (1, tr_tool->y2 - tr_tool->y1);

        tool = GIMP_TOOL (tr_tool);

        if (options->grid_type == GIMP_TRANSFORM_GRID_TYPE_N_LINES)
          {
            if (width <= height)
              {
                tr_tool->ngx = options->grid_size;
                tr_tool->ngy = tr_tool->ngx * MAX (1, height / width);
              }
            else
              {
                tr_tool->ngy = options->grid_size;
                tr_tool->ngx = tr_tool->ngy * MAX (1, width / height);
              }
          }
        else /* GIMP_TRANSFORM_GRID_TYPE_SPACING */
          {
            gint grid_size = MAX (2, options->grid_size);

            tr_tool->ngx = width  / grid_size;
            tr_tool->ngy = height / grid_size;
          }

        tr_tool->grid_coords = coords =
          g_new (gdouble, (tr_tool->ngx + tr_tool->ngy) * 4);

        tr_tool->tgrid_coords =
          g_new (gdouble, (tr_tool->ngx + tr_tool->ngy) * 4);

        gci = 0;

        for (i = 1; i <= tr_tool->ngx; i++)
          {
            coords[gci]     = tr_tool->x1 + (((gdouble) i) / (tr_tool->ngx + 1) *
                                             (tr_tool->x2 - tr_tool->x1));
            coords[gci + 1] = tr_tool->y1;
            coords[gci + 2] = coords[gci];
            coords[gci + 3] = tr_tool->y2;

            gci += 4;
          }

        for (i = 1; i <= tr_tool->ngy; i++)
          {
            coords[gci]     = tr_tool->x1;
            coords[gci + 1] = tr_tool->y1 + (((gdouble) i) / (tr_tool->ngy + 1) *
                                             (tr_tool->y2 - tr_tool->y1));
            coords[gci + 2] = tr_tool->x2;
            coords[gci + 3] = coords[gci + 1];

            gci += 4;
          }
      }

    default:
      break;
    }
}

static void
gimp_transform_tool_handles_recalc (GimpTransformTool *tr_tool,
                                    GimpDisplay       *display)
{
  gint dx1, dy1;
  gint dx2, dy2;
  gint dx3, dy3;
  gint dx4, dy4;
  gint x1, y1;
  gint x2, y2;

  gimp_display_shell_transform_xy (GIMP_DISPLAY_SHELL (display->shell),
                                   tr_tool->tx1, tr_tool->ty1,
                                   &dx1, &dy1,
                                   FALSE);
  gimp_display_shell_transform_xy (GIMP_DISPLAY_SHELL (display->shell),
                                   tr_tool->tx2, tr_tool->ty2,
                                   &dx2, &dy2,
                                   FALSE);
  gimp_display_shell_transform_xy (GIMP_DISPLAY_SHELL (display->shell),
                                   tr_tool->tx3, tr_tool->ty3,
                                   &dx3, &dy3,
                                   FALSE);
  gimp_display_shell_transform_xy (GIMP_DISPLAY_SHELL (display->shell),
                                   tr_tool->tx4, tr_tool->ty4,
                                   &dx4, &dy4,
                                   FALSE);

  x1 = MIN (MIN (dx1, dx2), MIN (dx3, dx4));
  y1 = MIN (MIN (dy1, dy2), MIN (dy3, dy4));
  x2 = MAX (MAX (dx1, dx2), MAX (dx3, dx4));
  y2 = MAX (MAX (dy1, dy2), MAX (dy3, dy4));

  tr_tool->handle_w = CLAMP ((x2 - x1) / 3, MIN_HANDLE_SIZE, HANDLE_SIZE);
  tr_tool->handle_h = CLAMP ((y2 - y1) / 3, MIN_HANDLE_SIZE, HANDLE_SIZE);
}

static void
gimp_transform_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpTool     *tool      = GIMP_TOOL (tr_tool);
  GimpToolInfo *tool_info = tool->tool_info;
  const gchar  *stock_id;

  if (! GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog)
    return;

  stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));

  tr_tool->dialog = gimp_tool_dialog_new (tool_info,
                                          NULL /* tool->display->shell */,
                                          tool_info->blurb,
                                          GIMP_STOCK_RESET, RESPONSE_RESET,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          stock_id,         GTK_RESPONSE_OK,
                                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (tr_tool->dialog),
                                   GTK_RESPONSE_OK);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (tr_tool->dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (tr_tool->dialog, "response",
                    G_CALLBACK (gimp_transform_tool_response),
                    tr_tool);

  GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog (tr_tool);
}

static void
gimp_transform_tool_prepare (GimpTransformTool *tr_tool,
                             GimpDisplay       *display)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  if ((options->preview_type == GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE ||
       options->preview_type == GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE_GRID) &&
      options->type         == GIMP_TRANSFORM_TYPE_LAYER &&
      options->direction    == GIMP_TRANSFORM_FORWARD)
    gimp_display_shell_set_show_transform (GIMP_DISPLAY_SHELL (display->shell),
                                           TRUE);
  else
    gimp_display_shell_set_show_transform (GIMP_DISPLAY_SHELL (display->shell),
                                           FALSE);

  if (tr_tool->dialog)
    {
      GimpDrawable *drawable = gimp_image_get_active_drawable (display->image);

      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (tr_tool->dialog),
                                         GIMP_VIEWABLE (drawable),
                                         GIMP_CONTEXT (options));

      gtk_widget_set_sensitive (tr_tool->dialog, TRUE);
    }

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->prepare)
    GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->prepare (tr_tool, display);
}

void
gimp_transform_tool_recalc (GimpTransformTool *tr_tool,
                            GimpDisplay       *display)
{
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc)
    GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc (tr_tool, display);

  gimp_transform_tool_transform_bounding_box (tr_tool);

  gimp_transform_tool_dialog_update (tr_tool);

  if (tr_tool->dialog)
    gtk_widget_show (tr_tool->dialog);
}

static void
gimp_transform_tool_response (GtkWidget         *widget,
                              gint               response_id,
                              GimpTransformTool *tr_tool)
{
  GimpTool *tool = GIMP_TOOL (tr_tool);

  switch (response_id)
    {
    case RESPONSE_RESET:
      {
        gint i;

        gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

        /*  Restore the previous transformation info  */
        for (i = 0; i < TRANS_INFO_SIZE; i++)
          tr_tool->trans_info[i] = tr_tool->old_trans_info[i];

        /*  reget the selection bounds  */
        gimp_transform_tool_bounds (tr_tool, tool->display);

        /*  recalculate the tool's transformation matrix  */
        gimp_transform_tool_recalc (tr_tool, tool->display);

        /* update preview */
        gimp_transform_tool_expose_preview (tr_tool);

        gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
      }
      break;

    case GTK_RESPONSE_OK:
      g_return_if_fail (tool->display != NULL);
      gimp_transform_tool_doit (tr_tool, tool->display);
      break;

    default:
      gimp_transform_tool_halt (tr_tool);
      break;
    }
}

static void
gimp_transform_tool_notify_type (GimpTransformOptions *options,
                                 GParamSpec           *pspec,
                                 GimpTransformTool    *tr_tool)
{
  GimpDisplay *display = GIMP_TOOL (tr_tool)->display;

  if (tr_tool->function != TRANSFORM_CREATING)
    gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

  tr_tool->type      = options->type;
  tr_tool->direction = options->direction;

  if (tr_tool->function != TRANSFORM_CREATING)
    {
      if (display)
        {
          /*  reget the selection bounds  */
          gimp_transform_tool_bounds (tr_tool, display);

          /*  recalculate the tool's transformation matrix  */
          gimp_transform_tool_recalc (tr_tool, display);
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
    }
}

static void
gimp_transform_tool_notify_preview (GimpTransformOptions *options,
                                    GParamSpec           *pspec,
                                    GimpTransformTool    *tr_tool)
{
  GimpDisplayShell *shell = NULL;

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tr_tool)))
    shell = GIMP_DISPLAY_SHELL (GIMP_DRAW_TOOL (tr_tool)->display->shell);

  switch (options->preview_type)
    {
    default:
    case GIMP_TRANSFORM_PREVIEW_TYPE_OUTLINE:
      if (shell)
        {
          gimp_display_shell_set_show_transform (shell, FALSE);
          gimp_transform_tool_force_expose_preview (tr_tool);
        }
      break;

    case GIMP_TRANSFORM_PREVIEW_TYPE_GRID:
      if (shell)
        {
          gimp_display_shell_set_show_transform (shell, FALSE);
          gimp_transform_tool_force_expose_preview (tr_tool);
        }

      if (tr_tool->function != TRANSFORM_CREATING)
        {
          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

          gimp_transform_tool_grid_recalc (tr_tool);
          gimp_transform_tool_transform_bounding_box (tr_tool);

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
        }
      break;

    case GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE:
      if (shell)
        {
          if (options->type      == GIMP_TRANSFORM_TYPE_LAYER &&
              options->direction == GIMP_TRANSFORM_FORWARD)
            gimp_display_shell_set_show_transform (shell, TRUE);
          else
            gimp_display_shell_set_show_transform (shell, FALSE);

          gimp_transform_tool_force_expose_preview (tr_tool);
        }
      break;

    case GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE_GRID:
      if (shell)
        {
          if (options->type      == GIMP_TRANSFORM_TYPE_LAYER &&
              options->direction == GIMP_TRANSFORM_FORWARD)
            gimp_display_shell_set_show_transform (shell, TRUE);
          else
            gimp_display_shell_set_show_transform (shell, FALSE);

          gimp_transform_tool_force_expose_preview (tr_tool);
        }

      if (tr_tool->function != TRANSFORM_CREATING)
        {
          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

          gimp_transform_tool_grid_recalc (tr_tool);
          gimp_transform_tool_transform_bounding_box (tr_tool);

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
        }
      break;
    }
}
