/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimplayer-floating-selection.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolpolygon.h"

#include "gimpfreeselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


struct _GimpFreeSelectToolPrivate
{
  gboolean        started;

  /* The selection operation active when the tool was started */
  GimpChannelOps  operation_at_start;
};


static void   gimp_free_select_tool_control      (GimpTool              *tool,
                                                  GimpToolAction         action,
                                                  GimpDisplay           *display);
static void   gimp_free_select_tool_button_press (GimpTool              *tool,
                                                  const GimpCoords      *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpButtonPressType    press_type,
                                                  GimpDisplay           *display);

static void   gimp_free_select_tool_commit       (GimpFreeSelectTool    *free_sel,
                                                  GimpDisplay           *display);
static void   gimp_free_select_tool_halt         (GimpFreeSelectTool    *free_sel);


G_DEFINE_TYPE_WITH_PRIVATE (GimpFreeSelectTool, gimp_free_select_tool,
                            GIMP_TYPE_POLYGON_SELECT_TOOL)

#define parent_class gimp_free_select_tool_parent_class


void
gimp_free_select_tool_register (GimpToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (GIMP_TYPE_FREE_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-free-select-tool",
                _("Free Select"),
                _("Free Select Tool: Select a hand-drawn region with free "
                  "and polygonal segments"),
                N_("_Free Select"), "F",
                NULL, GIMP_HELP_TOOL_FREE_SELECT,
                GIMP_ICON_TOOL_FREE_SELECT,
                data);
}

static void
gimp_free_select_tool_class_init (GimpFreeSelectToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  tool_class->control      = gimp_free_select_tool_control;
  tool_class->button_press = gimp_free_select_tool_button_press;
}

static void
gimp_free_select_tool_init (GimpFreeSelectTool *free_sel)
{
  GimpTool *tool = GIMP_TOOL (free_sel);

  free_sel->priv = gimp_free_select_tool_get_instance_private (free_sel);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_FREE_SELECT);
}

static void
gimp_free_select_tool_control (GimpTool       *tool,
                               GimpToolAction  action,
                               GimpDisplay    *display)
{
  GimpFreeSelectTool *free_sel = GIMP_FREE_SELECT_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_free_select_tool_halt (free_sel);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_free_select_tool_commit (free_sel, display);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_free_select_tool_button_press (GimpTool            *tool,
                                    const GimpCoords    *coords,
                                    guint32              time,
                                    GdkModifierType      state,
                                    GimpButtonPressType  press_type,
                                    GimpDisplay         *display)
{
  GimpSelectionOptions      *options  = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpFreeSelectTool        *free_sel = GIMP_FREE_SELECT_TOOL (tool);
  GimpPolygonSelectTool     *poly_sel = GIMP_POLYGON_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv     = free_sel->priv;

  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);

  if (press_type == GIMP_BUTTON_PRESS_NORMAL &&
      gimp_polygon_select_tool_is_grabbed (poly_sel))
    {
      if (! priv->started)
        {
          priv->started            = TRUE;
          priv->operation_at_start = options->operation;
        }
    }
}

static void
gimp_free_select_tool_halt (GimpFreeSelectTool *free_sel)
{
  GimpFreeSelectToolPrivate *priv = free_sel->priv;

  priv->started = FALSE;
}

static void
gimp_free_select_tool_commit (GimpFreeSelectTool *free_sel,
                              GimpDisplay        *display)
{
  GimpSelectionOptions      *options = GIMP_SELECTION_TOOL_GET_OPTIONS (free_sel);
  GimpFreeSelectToolPrivate *priv    = free_sel->priv;
  GimpImage                 *image   = gimp_display_get_image (display);
  const GimpVector2         *points;
  gint                       n_points;

  gimp_polygon_select_tool_get_points (GIMP_POLYGON_SELECT_TOOL (free_sel),
                                       &points, &n_points);

  if (n_points > 2)
    {
      gimp_channel_select_polygon (gimp_image_get_mask (image),
                                   C_("command", "Free Select"),
                                   n_points,
                                   points,
                                   priv->operation_at_start,
                                   options->antialias,
                                   options->feather,
                                   options->feather_radius,
                                   options->feather_radius,
                                   TRUE);

      gimp_image_flush (image);
    }
}
