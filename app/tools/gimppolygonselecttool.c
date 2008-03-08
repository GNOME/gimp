/* GIMP - The GNU Image Manipulation Program
 *
 * A polygonal selection tool for GIMP
 * Copyright (C) 2007 Martin Nordholts
 *
 * Based on gimpfreeselecttool.c which is
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
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimplayer-floating-sel.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimppolygonselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define DEFAULT_MAX_INC         1024

#define HANDLE_SIZE             10
#define POINT_GRAB_THRESHOLD_SQ SQR(HANDLE_SIZE / 2)


struct _GimpPolygonSelectTool
{
  GimpSelectionTool  parent_instance;

  /* Point which is part of he polygon already. */
  GimpVector2       *grabbed_point;

  gboolean           button1_down;

  /* Save the grabbed point state so we can cancel a movement
   * operation.
   */
  GimpVector2        saved_grabbed_point;

  /* Point which is used to draw the polygon but which is not part of
   * it yet.
   */
  GimpVector2        pending_point;
  gboolean           show_pending_point;

  /* The points of the polygon. */
  GimpVector2       *points;

  /* The number of points used for the actual selection. */
  gint               n_points;

  gint               max_segs;
};


static void         gimp_polygon_select_tool_finalize       (GObject               *object);

static void         gimp_polygon_select_tool_control        (GimpTool              *tool,
                                                             GimpToolAction         action,
                                                             GimpDisplay           *display);
static void         gimp_polygon_select_tool_oper_update    (GimpTool              *tool,
                                                             GimpCoords            *coords,
                                                             GdkModifierType        state,
                                                             gboolean               proximity,
                                                             GimpDisplay           *display);
static void         gimp_polygon_select_tool_cursor_update  (GimpTool              *tool,
                                                             GimpCoords            *coords,
                                                             GdkModifierType        state,
                                                             GimpDisplay           *display);
static void         gimp_polygon_select_tool_button_press   (GimpTool              *tool,
                                                             GimpCoords            *coords,
                                                             guint32                time,
                                                             GdkModifierType        state,
                                                             GimpDisplay           *display);
static void         gimp_polygon_select_tool_motion         (GimpTool              *tool,
                                                             GimpCoords            *coords,
                                                             guint32                time,
                                                             GdkModifierType        state,
                                                             GimpDisplay           *display);
static void         gimp_polygon_select_tool_button_release (GimpTool              *tool,
                                                             GimpCoords            *coords,
                                                             guint32                time,
                                                             GdkModifierType        state,
                                                             GimpButtonReleaseType  release_type,
                                                             GimpDisplay           *display);
static gboolean     gimp_polygon_select_tool_key_press      (GimpTool              *tool,
                                                             GdkEventKey           *kevent,
                                                             GimpDisplay           *display);

static void         gimp_polygon_select_tool_start          (GimpPolygonSelectTool *poly_sel_tool,
                                                             GimpDisplay           *display);
static void         gimp_polygon_select_tool_commit         (GimpPolygonSelectTool *poly_sel_tool,
                                                             GimpDisplay           *display);
static void         gimp_polygon_select_tool_halt           (GimpPolygonSelectTool *poly_sel_tool);

static void         gimp_polygon_select_tool_draw           (GimpDrawTool          *draw_tool);

static void         gimp_polygon_select_tool_real_select    (GimpPolygonSelectTool *poly_sel_tool,
                                                             GimpDisplay           *display);

static GimpVector2 *gimp_polygon_select_tool_add_point      (GimpPolygonSelectTool *poly_sel_tool,
                                                             gdouble                x,
                                                             gdouble                y);
static void         gimp_polygon_select_tool_update_button_state
                                                            (GimpPolygonSelectTool *poly_sel_tool,
                                                             GdkModifierType        state);
static void         gimp_polygon_select_tool_remove_last    (GimpPolygonSelectTool *poly_sel_tool);
static void         gimp_polygon_select_tool_select_closest_point
                                                            (GimpPolygonSelectTool *poly_sel_tool,
                                                             GimpDisplay           *display,
                                                             GimpCoords            *coords);
static gboolean     gimp_polygon_select_tool_should_close   (GimpPolygonSelectTool *poly_sel_tool,
                                                             GimpDisplay           *display,
                                                             GimpCoords            *coords);


G_DEFINE_TYPE (GimpPolygonSelectTool, gimp_polygon_select_tool,
               GIMP_TYPE_SELECTION_TOOL);

#define parent_class gimp_polygon_select_tool_parent_class


void
gimp_polygon_select_tool_register (GimpToolRegisterCallback callback,
                                   gpointer                 data)
{
  (* callback) (GIMP_TYPE_POLYGON_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-polygon-select-tool",
                _("Polygon Select"),
                _("Polygon Select Tool: Select a hand-drawn polygon"),
                N_("_Polygon Select"), "G",
                NULL, GIMP_HELP_TOOL_POLYGON_SELECT,
                GIMP_STOCK_TOOL_POLYGON_SELECT,
                data);
}

static void
gimp_polygon_select_tool_class_init (GimpPolygonSelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = gimp_polygon_select_tool_finalize;

  tool_class->control        = gimp_polygon_select_tool_control;
  tool_class->oper_update    = gimp_polygon_select_tool_oper_update;
  tool_class->cursor_update  = gimp_polygon_select_tool_cursor_update;
  tool_class->button_press   = gimp_polygon_select_tool_button_press;
  tool_class->motion         = gimp_polygon_select_tool_motion;
  tool_class->button_release = gimp_polygon_select_tool_button_release;
  tool_class->key_press      = gimp_polygon_select_tool_key_press;

  draw_tool_class->draw      = gimp_polygon_select_tool_draw;

  klass->select              = gimp_polygon_select_tool_real_select;
}

static void
gimp_polygon_select_tool_init (GimpPolygonSelectTool *poly_sel_tool)
{
  GimpTool *tool = GIMP_TOOL (poly_sel_tool);

  gimp_tool_control_set_scroll_lock (tool->control, FALSE);
  gimp_tool_control_set_wants_click (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_POLYGON_SELECT);

  poly_sel_tool->points   = NULL;
  poly_sel_tool->n_points = 0;
  poly_sel_tool->max_segs = 0;
}

static void
gimp_polygon_select_tool_finalize (GObject *object)
{
  GimpPolygonSelectTool *poly_sel_tool = GIMP_POLYGON_SELECT_TOOL (object);

  if (poly_sel_tool->points)
    {
      g_free (poly_sel_tool->points);
      poly_sel_tool->points = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_polygon_select_tool_control (GimpTool       *tool,
                                  GimpToolAction  action,
                                  GimpDisplay    *display)
{
  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_polygon_select_tool_halt (GIMP_POLYGON_SELECT_TOOL (tool));
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_polygon_select_tool_oper_update (GimpTool        *tool,
                                      GimpCoords      *coords,
                                      GdkModifierType  state,
                                      gboolean         proximity,
                                      GimpDisplay     *display)
{
  GimpPolygonSelectTool *poly_sel_tool = GIMP_POLYGON_SELECT_TOOL (tool);

  if (tool->display == display)
    {
      gboolean hovering_first_point;

      gimp_polygon_select_tool_select_closest_point (poly_sel_tool,
                                                     display,
                                                     coords);

      hovering_first_point = gimp_polygon_select_tool_should_close (poly_sel_tool,
                                                                    display,
                                                                    coords);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      if (poly_sel_tool->n_points == 0 ||
          (poly_sel_tool->grabbed_point && ! hovering_first_point))
        {
          poly_sel_tool->show_pending_point = FALSE;
        }
      else
        {
          poly_sel_tool->show_pending_point = TRUE;

          if (hovering_first_point)
            {
              poly_sel_tool->pending_point = poly_sel_tool->points[0];
            }
          else
            {
              poly_sel_tool->pending_point.x = coords->x;
              poly_sel_tool->pending_point.y = coords->y;
            }
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_polygon_select_tool_cursor_update (GimpTool        *tool,
                                        GimpCoords      *coords,
                                        GdkModifierType  state,
                                        GimpDisplay     *display)
{
  GimpPolygonSelectTool *poly_sel_tool = GIMP_POLYGON_SELECT_TOOL (tool);

  if (poly_sel_tool->grabbed_point &&
      ! gimp_polygon_select_tool_should_close (poly_sel_tool, display, coords))
    {
      gimp_tool_set_cursor (tool, display,
                            gimp_tool_control_get_cursor (tool->control),
                            gimp_tool_control_get_tool_cursor (tool->control),
                            GIMP_CURSOR_MODIFIER_MOVE);

      return;
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_polygon_select_tool_button_press (GimpTool        *tool,
                                       GimpCoords      *coords,
                                       guint32          time,
                                       GdkModifierType  state,
                                       GimpDisplay     *display)
{
  GimpDrawTool          *draw_tool     = GIMP_DRAW_TOOL (tool);
  GimpPolygonSelectTool *poly_sel_tool = GIMP_POLYGON_SELECT_TOOL (tool);

  gimp_draw_tool_pause (draw_tool);

  gimp_polygon_select_tool_update_button_state (poly_sel_tool, state);

  if (display != tool->display)
    {
      gimp_polygon_select_tool_start (poly_sel_tool, display);

      gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (poly_sel_tool),
                                      coords);
    }

  if (poly_sel_tool->grabbed_point)
    {
      poly_sel_tool->saved_grabbed_point = *poly_sel_tool->grabbed_point;
    }

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_polygon_select_tool_motion (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 guint32          time,
                                 GdkModifierType  state,
                                 GimpDisplay     *display)
{
  if (tool->display == display)
    {
      GimpPolygonSelectTool *poly_sel_tool = GIMP_POLYGON_SELECT_TOOL (tool);
      GimpDrawTool          *draw_tool     = GIMP_DRAW_TOOL (tool);

      gimp_draw_tool_pause (draw_tool);

      if (poly_sel_tool->grabbed_point)
        {
          poly_sel_tool->grabbed_point->x = coords->x;
          poly_sel_tool->grabbed_point->y = coords->y;

          if (gimp_polygon_select_tool_should_close (poly_sel_tool,
                                                     display,
                                                     coords))
            {
              poly_sel_tool->pending_point.x = coords->x;
              poly_sel_tool->pending_point.y = coords->y;
            }
        }
      else
        {
          poly_sel_tool->pending_point.x = coords->x;
          poly_sel_tool->pending_point.y = coords->y;
        }

      gimp_draw_tool_resume (draw_tool);
    }
}

static void
gimp_polygon_select_tool_button_release (GimpTool              *tool,
                                         GimpCoords            *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         GimpButtonReleaseType  release_type,
                                         GimpDisplay           *display)
{
  GimpPolygonSelectTool *poly_sel_tool = GIMP_POLYGON_SELECT_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (poly_sel_tool));

  gimp_polygon_select_tool_update_button_state (poly_sel_tool, state);

  switch (release_type)
    {
    case GIMP_BUTTON_RELEASE_CLICK:
      if (gimp_polygon_select_tool_should_close (poly_sel_tool,
                                                 display,
                                                 coords))
        {
          gimp_polygon_select_tool_commit (poly_sel_tool, display);
          break;
        }

      /* Fall through */

    case GIMP_BUTTON_RELEASE_NORMAL:
      if (! poly_sel_tool->grabbed_point)
        {
          gimp_polygon_select_tool_add_point (poly_sel_tool,
                                              coords->x, coords->y);
        }
      else
        {
          /* We don't need to do anything since the grabbed point have
           * already been moved in _motion.
           */
        }
      break;

    case GIMP_BUTTON_RELEASE_CANCEL:
      if (poly_sel_tool->grabbed_point)
        {
          *poly_sel_tool->grabbed_point = poly_sel_tool->saved_grabbed_point;
        }
      break;

    case GIMP_BUTTON_RELEASE_NO_MOTION:
      if (gimp_image_floating_sel (display->image))
        {
          /*  If there is a floating selection, anchor it  */
          floating_sel_anchor (gimp_image_floating_sel (display->image));
        }
      else
        {
          /*  Otherwise, clear the selection mask  */
          gimp_channel_clear (gimp_image_get_mask (display->image), NULL, TRUE);
        }
      break;
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (poly_sel_tool));
}

static gboolean
gimp_polygon_select_tool_key_press (GimpTool    *tool,
                                    GdkEventKey *kevent,
                                    GimpDisplay *display)
{
  GimpPolygonSelectTool *poly_sel_tool = GIMP_POLYGON_SELECT_TOOL (tool);

  switch (kevent->keyval)
    {
    case GDK_BackSpace:
      gimp_polygon_select_tool_remove_last (poly_sel_tool);
      return TRUE;

    case GDK_Return:
    case GDK_KP_Enter:
    case GDK_ISO_Enter:
      gimp_polygon_select_tool_commit (poly_sel_tool, display);
      return TRUE;

    case GDK_Escape:
      gimp_polygon_select_tool_halt (poly_sel_tool);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_polygon_select_tool_start (GimpPolygonSelectTool *poly_sel_tool,
                                GimpDisplay           *display)
{
  GimpTool     *tool      = GIMP_TOOL (poly_sel_tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  gimp_polygon_select_tool_halt (poly_sel_tool);

  gimp_tool_control_activate (tool->control);

  tool->display = display;

  poly_sel_tool->n_points           = 0;
  poly_sel_tool->grabbed_point      = NULL;
  poly_sel_tool->button1_down       = FALSE;
  poly_sel_tool->show_pending_point = FALSE;

  gimp_draw_tool_start (draw_tool, display);
}

static void
gimp_polygon_select_tool_commit (GimpPolygonSelectTool *poly_sel_tool,
                                 GimpDisplay           *display)
{
  if (poly_sel_tool->n_points >= 3)
    {
      gimp_polygon_select_tool_select (poly_sel_tool, display);
    }

  gimp_polygon_select_tool_halt (poly_sel_tool);

  gimp_image_flush (display->image);
}

static void
gimp_polygon_select_tool_halt (GimpPolygonSelectTool *poly_sel_tool)
{
  GimpTool     *tool      = GIMP_TOOL (poly_sel_tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (poly_sel_tool);

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);

  poly_sel_tool->grabbed_point      = NULL;
  poly_sel_tool->show_pending_point = FALSE;
  poly_sel_tool->n_points           = 0;

  tool->display = NULL;
}

static void
gimp_polygon_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpPolygonSelectTool *poly_sel_tool = GIMP_POLYGON_SELECT_TOOL (draw_tool);

  gimp_draw_tool_draw_lines (draw_tool,
                             poly_sel_tool->points, poly_sel_tool->n_points,
                             FALSE, FALSE);

  if (poly_sel_tool->grabbed_point &&
      ! poly_sel_tool->button1_down)
    {
      gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_CIRCLE,
                                  poly_sel_tool->grabbed_point->x,
                                  poly_sel_tool->grabbed_point->y,
                                  HANDLE_SIZE, HANDLE_SIZE,
                                  GTK_ANCHOR_CENTER, FALSE);
    }

  if (poly_sel_tool->show_pending_point)
    {
      GimpVector2 last = poly_sel_tool->points[poly_sel_tool->n_points - 1];

      gimp_draw_tool_draw_line (draw_tool,
                                last.x,
                                last.y,
                                poly_sel_tool->pending_point.x,
                                poly_sel_tool->pending_point.y,
                                FALSE);
    }
}

void
gimp_polygon_select_tool_select (GimpPolygonSelectTool *poly_sel_tool,
                                 GimpDisplay           *display)
{
  g_return_if_fail (GIMP_IS_POLYGON_SELECT_TOOL (poly_sel_tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  GIMP_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel_tool)->select (poly_sel_tool,
                                                              display);
}

static void
gimp_polygon_select_tool_real_select (GimpPolygonSelectTool *poly_sel_tool,
                                      GimpDisplay           *display)
{
  GimpSelectionOptions *options = GIMP_SELECTION_TOOL_GET_OPTIONS (poly_sel_tool);

  gimp_channel_select_polygon (gimp_image_get_mask (display->image),
                               Q_("command|Polygon Select"),
                               poly_sel_tool->n_points,
                               poly_sel_tool->points,
                               options->operation,
                               options->antialias,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius,
                               TRUE);
}

static GimpVector2 *
gimp_polygon_select_tool_add_point (GimpPolygonSelectTool *poly_sel_tool,
                                    gdouble                x,
                                    gdouble                y)
{
  if (poly_sel_tool->n_points >= poly_sel_tool->max_segs)
    {
      poly_sel_tool->max_segs += DEFAULT_MAX_INC;

      poly_sel_tool->points = g_realloc (poly_sel_tool->points,
                                         sizeof (GimpVector2) * poly_sel_tool->max_segs);
    }

  poly_sel_tool->points[poly_sel_tool->n_points].x = x;
  poly_sel_tool->points[poly_sel_tool->n_points].y = y;

  return &poly_sel_tool->points[poly_sel_tool->n_points++];
}

static void
gimp_polygon_select_tool_update_button_state (GimpPolygonSelectTool *poly_sel_tool,
                                              GdkModifierType        state)
{
  poly_sel_tool->button1_down = state & GDK_BUTTON1_MASK ? TRUE : FALSE;
}

static void
gimp_polygon_select_tool_remove_last (GimpPolygonSelectTool *poly_sel_tool)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (poly_sel_tool);

  gimp_draw_tool_pause (draw_tool);

  poly_sel_tool->n_points--;

  if (poly_sel_tool->n_points == 0)
    {
      gimp_polygon_select_tool_halt (poly_sel_tool);
    }

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_polygon_select_tool_select_closest_point (GimpPolygonSelectTool *poly_sel_tool,
                                               GimpDisplay           *display,
                                               GimpCoords            *coords)
{
  GimpDrawTool *draw_tool     = GIMP_DRAW_TOOL (poly_sel_tool);
  gdouble       shortest_dist = POINT_GRAB_THRESHOLD_SQ;
  GimpVector2  *grabbed_point = NULL;
  int           i;

  for (i = 0; i < poly_sel_tool->n_points; i++)
    {
      gdouble dist;

      dist = gimp_draw_tool_calc_distance_square (draw_tool,
                                                  display,
                                                  coords->x,
                                                  coords->y,
                                                  poly_sel_tool->points[i].x,
                                                  poly_sel_tool->points[i].y);

      if (dist < shortest_dist)
        {
          grabbed_point = &poly_sel_tool->points[i];
        }
    }

  if (grabbed_point != poly_sel_tool->grabbed_point)
    {
      gimp_draw_tool_pause(draw_tool);

      poly_sel_tool->grabbed_point = grabbed_point;

      gimp_draw_tool_resume(draw_tool);
    }
}

static gboolean
gimp_polygon_select_tool_should_close (GimpPolygonSelectTool *poly_sel_tool,
                                       GimpDisplay           *display,
                                       GimpCoords            *coords)
{
  gboolean should_close = FALSE;

  if (poly_sel_tool->n_points > 0)
    {
      gdouble dist;

      dist = gimp_draw_tool_calc_distance_square (GIMP_DRAW_TOOL (poly_sel_tool),
                                                  display,
                                                  coords->x,
                                                  coords->y,
                                                  poly_sel_tool->points[0].x,
                                                  poly_sel_tool->points[0].y);

      should_close = dist < POINT_GRAB_THRESHOLD_SQ;
    }

  return should_close;
}
