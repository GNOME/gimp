/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Major improvement to support polygonal segments
 * Copyright (C) 2008 Martin Nordholts
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligmachannel.h"
#include "core/ligmachannel-select.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer-floating-selection.h"

#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"
#include "display/ligmatoolpolygon.h"

#include "ligmafreeselecttool.h"
#include "ligmaselectionoptions.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


struct _LigmaFreeSelectToolPrivate
{
  gboolean        started;
  gboolean        changed;

  /* The selection operation active when the tool was started */
  LigmaChannelOps  operation_at_start;
};


static void       ligma_free_select_tool_control         (LigmaTool              *tool,
                                                         LigmaToolAction         action,
                                                         LigmaDisplay           *display);
static void       ligma_free_select_tool_button_press    (LigmaTool              *tool,
                                                         const LigmaCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         LigmaButtonPressType    press_type,
                                                         LigmaDisplay           *display);
static void       ligma_free_select_tool_button_release  (LigmaTool              *tool,
                                                         const LigmaCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         LigmaButtonReleaseType  release_type,
                                                         LigmaDisplay           *display);
static void       ligma_free_select_tool_options_notify  (LigmaTool              *tool,
                                                         LigmaToolOptions       *options,
                                                         const GParamSpec      *pspec);

static gboolean   ligma_free_select_tool_have_selection  (LigmaSelectionTool     *sel_tool,
                                                         LigmaDisplay           *display);

static void       ligma_free_select_tool_change_complete (LigmaPolygonSelectTool *poly_sel,
                                                         LigmaDisplay           *display);

static void       ligma_free_select_tool_commit          (LigmaFreeSelectTool    *free_sel,
                                                         LigmaDisplay           *display);
static void       ligma_free_select_tool_halt            (LigmaFreeSelectTool    *free_sel);

static gboolean   ligma_free_select_tool_select          (LigmaFreeSelectTool    *free_sel,
                                                         LigmaDisplay           *display);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaFreeSelectTool, ligma_free_select_tool,
                            LIGMA_TYPE_POLYGON_SELECT_TOOL)

#define parent_class ligma_free_select_tool_parent_class


void
ligma_free_select_tool_register (LigmaToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (LIGMA_TYPE_FREE_SELECT_TOOL,
                LIGMA_TYPE_SELECTION_OPTIONS,
                ligma_selection_options_gui,
                0,
                "ligma-free-select-tool",
                _("Free Select"),
                _("Free Select Tool: Select a hand-drawn region with free "
                  "and polygonal segments"),
                N_("_Free Select"), "F",
                NULL, LIGMA_HELP_TOOL_FREE_SELECT,
                LIGMA_ICON_TOOL_FREE_SELECT,
                data);
}

static void
ligma_free_select_tool_class_init (LigmaFreeSelectToolClass *klass)
{
  LigmaToolClass              *tool_class     = LIGMA_TOOL_CLASS (klass);
  LigmaSelectionToolClass     *sel_class      = LIGMA_SELECTION_TOOL_CLASS (klass);
  LigmaPolygonSelectToolClass *poly_sel_class = LIGMA_POLYGON_SELECT_TOOL_CLASS (klass);

  tool_class->control             = ligma_free_select_tool_control;
  tool_class->button_press        = ligma_free_select_tool_button_press;
  tool_class->button_release      = ligma_free_select_tool_button_release;
  tool_class->options_notify      = ligma_free_select_tool_options_notify;

  sel_class->have_selection       = ligma_free_select_tool_have_selection;

  poly_sel_class->change_complete = ligma_free_select_tool_change_complete;
}

static void
ligma_free_select_tool_init (LigmaFreeSelectTool *free_sel)
{
  LigmaTool          *tool     = LIGMA_TOOL (free_sel);
  LigmaSelectionTool *sel_tool = LIGMA_SELECTION_TOOL (tool);

  free_sel->priv = ligma_free_select_tool_get_instance_private (free_sel);

  ligma_tool_control_set_preserve     (tool->control, FALSE);
  ligma_tool_control_set_dirty_mask   (tool->control,
                                      LIGMA_DIRTY_SELECTION);
  ligma_tool_control_set_dirty_action (tool->control,
                                      LIGMA_TOOL_ACTION_COMMIT);
  ligma_tool_control_set_tool_cursor  (tool->control,
                                      LIGMA_TOOL_CURSOR_FREE_SELECT);

  sel_tool->allow_move = TRUE;
}

static void
ligma_free_select_tool_control (LigmaTool       *tool,
                               LigmaToolAction  action,
                               LigmaDisplay    *display)
{
  LigmaFreeSelectTool *free_sel = LIGMA_FREE_SELECT_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_free_select_tool_halt (free_sel);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_free_select_tool_commit (free_sel, display);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_free_select_tool_button_press (LigmaTool            *tool,
                                    const LigmaCoords    *coords,
                                    guint32              time,
                                    GdkModifierType      state,
                                    LigmaButtonPressType  press_type,
                                    LigmaDisplay         *display)
{
  LigmaSelectionOptions      *options  = LIGMA_SELECTION_TOOL_GET_OPTIONS (tool);
  LigmaFreeSelectTool        *free_sel = LIGMA_FREE_SELECT_TOOL (tool);
  LigmaPolygonSelectTool     *poly_sel = LIGMA_POLYGON_SELECT_TOOL (tool);
  LigmaFreeSelectToolPrivate *priv     = free_sel->priv;

  if (press_type == LIGMA_BUTTON_PRESS_NORMAL &&
      ligma_selection_tool_start_edit (LIGMA_SELECTION_TOOL (poly_sel),
                                      display, coords))
    return;

  LIGMA_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);

  if (press_type == LIGMA_BUTTON_PRESS_NORMAL &&
      ligma_polygon_select_tool_is_grabbed (poly_sel))
    {
      if (! priv->started)
        {
          priv->started            = TRUE;
          priv->operation_at_start = options->operation;
        }

      ligma_selection_tool_start_change (
        LIGMA_SELECTION_TOOL (tool),
        ! ligma_polygon_select_tool_is_closed (poly_sel),
        priv->operation_at_start);

      priv->changed = FALSE;
    }
}

static void
ligma_free_select_tool_button_release (LigmaTool              *tool,
                                      const LigmaCoords      *coords,
                                      guint32                time,
                                      GdkModifierType        state,
                                      LigmaButtonReleaseType  release_type,
                                      LigmaDisplay           *display)
{
  LigmaFreeSelectTool        *free_sel = LIGMA_FREE_SELECT_TOOL (tool);
  LigmaPolygonSelectTool     *poly_sel = LIGMA_POLYGON_SELECT_TOOL (tool);
  LigmaFreeSelectToolPrivate *priv     = free_sel->priv;

  if (ligma_polygon_select_tool_is_grabbed (poly_sel))
    {
      ligma_selection_tool_end_change (LIGMA_SELECTION_TOOL (tool),
                                      ! priv->changed);

      priv->changed = FALSE;
    }

  LIGMA_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);
}

static void
ligma_free_select_tool_options_notify (LigmaTool         *tool,
                                      LigmaToolOptions  *options,
                                      const GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "antialias") ||
      ! strcmp (pspec->name, "feather")   ||
      ! strcmp (pspec->name, "feather-radius"))
    {
      if (tool->display)
        {
          ligma_free_select_tool_change_complete (
            LIGMA_POLYGON_SELECT_TOOL (tool), tool->display);
        }
    }

  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);
}

static gboolean
ligma_free_select_tool_have_selection (LigmaSelectionTool *sel_tool,
                                      LigmaDisplay       *display)
{
  LigmaPolygonSelectTool *poly_sel = LIGMA_POLYGON_SELECT_TOOL (sel_tool);
  LigmaTool              *tool     = LIGMA_TOOL (sel_tool);

  if (display == tool->display)
    {
      gint n_points;

      ligma_polygon_select_tool_get_points (poly_sel, NULL, &n_points);

      if (n_points > 2)
        return TRUE;
    }

  return LIGMA_SELECTION_TOOL_CLASS (parent_class)->have_selection (sel_tool,
                                                                   display);
}

static void
ligma_free_select_tool_change_complete (LigmaPolygonSelectTool *poly_sel,
                                       LigmaDisplay           *display)
{
  LigmaFreeSelectTool        *free_sel = LIGMA_FREE_SELECT_TOOL (poly_sel);
  LigmaFreeSelectToolPrivate *priv     = free_sel->priv;

  priv->changed = TRUE;

  ligma_selection_tool_start_change (LIGMA_SELECTION_TOOL (free_sel),
                                    FALSE,
                                    priv->operation_at_start);

  if (ligma_polygon_select_tool_is_closed (poly_sel))
    ligma_free_select_tool_select (free_sel, display);

  ligma_selection_tool_end_change (LIGMA_SELECTION_TOOL (free_sel),
                                  FALSE);
}

static void
ligma_free_select_tool_halt (LigmaFreeSelectTool *free_sel)
{
  LigmaFreeSelectToolPrivate *priv = free_sel->priv;

  priv->started = FALSE;
}

static void
ligma_free_select_tool_commit (LigmaFreeSelectTool *free_sel,
                              LigmaDisplay        *display)
{
  LigmaPolygonSelectTool *poly_sel = LIGMA_POLYGON_SELECT_TOOL (free_sel);

  if (! ligma_polygon_select_tool_is_closed (poly_sel))
    {
      if (ligma_free_select_tool_select (free_sel, display))
        ligma_image_flush (ligma_display_get_image (display));
    }
}

static gboolean
ligma_free_select_tool_select (LigmaFreeSelectTool *free_sel,
                              LigmaDisplay        *display)
{
  LigmaSelectionOptions      *options = LIGMA_SELECTION_TOOL_GET_OPTIONS (free_sel);
  LigmaTool                  *tool    = LIGMA_TOOL (free_sel);
  LigmaFreeSelectToolPrivate *priv    = free_sel->priv;
  LigmaImage                 *image   = ligma_display_get_image (display);
  const LigmaVector2         *points;
  gint                       n_points;

  ligma_polygon_select_tool_get_points (LIGMA_POLYGON_SELECT_TOOL (free_sel),
                                       &points, &n_points);

  if (n_points > 2)
    {
      /* prevent this change from halting the tool */
      ligma_tool_control_push_preserve (tool->control, TRUE);

      ligma_channel_select_polygon (ligma_image_get_mask (image),
                                   C_("command", "Free Select"),
                                   n_points,
                                   points,
                                   priv->operation_at_start,
                                   options->antialias,
                                   options->feather,
                                   options->feather_radius,
                                   options->feather_radius,
                                   TRUE);

      ligma_tool_control_pop_preserve (tool->control);

      return TRUE;
    }

  return FALSE;
}
