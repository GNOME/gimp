/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "paint/ligmadodgeburnoptions.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmadodgeburntool.h"
#include "ligmapaintoptions-gui.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static void   ligma_dodge_burn_tool_modifier_key  (LigmaTool          *tool,
                                                  GdkModifierType    key,
                                                  gboolean           press,
                                                  GdkModifierType    state,
                                                  LigmaDisplay       *display);
static void   ligma_dodge_burn_tool_cursor_update (LigmaTool          *tool,
                                                  const LigmaCoords  *coords,
                                                  GdkModifierType    state,
                                                  LigmaDisplay       *display);
static void   ligma_dodge_burn_tool_oper_update   (LigmaTool          *tool,
                                                  const LigmaCoords  *coords,
                                                  GdkModifierType    state,
                                                  gboolean           proximity,
                                                  LigmaDisplay       *display);
static void   ligma_dodge_burn_tool_status_update (LigmaTool          *tool,
                                                  LigmaDodgeBurnType  type);

static GtkWidget * ligma_dodge_burn_options_gui   (LigmaToolOptions   *tool_options);


G_DEFINE_TYPE (LigmaDodgeBurnTool, ligma_dodge_burn_tool, LIGMA_TYPE_BRUSH_TOOL)

#define parent_class ligma_dodge_burn_tool_parent_class


void
ligma_dodge_burn_tool_register (LigmaToolRegisterCallback  callback,
                               gpointer                  data)
{
  (* callback) (LIGMA_TYPE_DODGE_BURN_TOOL,
                LIGMA_TYPE_DODGE_BURN_OPTIONS,
                ligma_dodge_burn_options_gui,
                LIGMA_PAINT_OPTIONS_CONTEXT_MASK,
                "ligma-dodge-burn-tool",
                _("Dodge / Burn"),
                _("Dodge / Burn Tool: Selectively lighten or darken using a brush"),
                N_("Dod_ge / Burn"), "<shift>D",
                NULL, LIGMA_HELP_TOOL_DODGE_BURN,
                LIGMA_ICON_TOOL_DODGE,
                data);
}

static void
ligma_dodge_burn_tool_class_init (LigmaDodgeBurnToolClass *klass)
{
  LigmaToolClass *tool_class = LIGMA_TOOL_CLASS (klass);

  tool_class->modifier_key  = ligma_dodge_burn_tool_modifier_key;
  tool_class->cursor_update = ligma_dodge_burn_tool_cursor_update;
  tool_class->oper_update   = ligma_dodge_burn_tool_oper_update;
}

static void
ligma_dodge_burn_tool_init (LigmaDodgeBurnTool *dodgeburn)
{
  LigmaTool *tool = LIGMA_TOOL (dodgeburn);

  ligma_tool_control_set_tool_cursor        (tool->control,
                                            LIGMA_TOOL_CURSOR_DODGE);
  ligma_tool_control_set_toggle_tool_cursor (tool->control,
                                            LIGMA_TOOL_CURSOR_BURN);

  ligma_dodge_burn_tool_status_update (tool, LIGMA_DODGE_BURN_TYPE_BURN);
}

static void
ligma_dodge_burn_tool_modifier_key (LigmaTool        *tool,
                                   GdkModifierType  key,
                                   gboolean         press,
                                   GdkModifierType  state,
                                   LigmaDisplay     *display)
{
  LigmaDodgeBurnTool    *dodgeburn   = LIGMA_DODGE_BURN_TOOL (tool);
  LigmaDodgeBurnOptions *options     = LIGMA_DODGE_BURN_TOOL_GET_OPTIONS (tool);
  GdkModifierType       line_mask   = LIGMA_PAINT_TOOL_LINE_MASK;
  GdkModifierType       toggle_mask = ligma_get_toggle_behavior_mask ();

  if ((key == toggle_mask   &&
      ! (state & line_mask) && /* leave stuff untouched in line draw mode */
       press != dodgeburn->toggled)

      ||

      (key == line_mask   && /* toggle back after keypresses CTRL(hold)->  */
       ! press            && /* SHIFT(hold)->CTRL(release)->SHIFT(release) */
       dodgeburn->toggled &&
       ! (state & toggle_mask)))
    {
      dodgeburn->toggled = press;

      switch (options->type)
        {
        case LIGMA_DODGE_BURN_TYPE_DODGE:
          g_object_set (options, "type", LIGMA_DODGE_BURN_TYPE_BURN, NULL);
          break;

        case LIGMA_DODGE_BURN_TYPE_BURN:
          g_object_set (options, "type", LIGMA_DODGE_BURN_TYPE_DODGE, NULL);
          break;

        default:
          break;
        }
    }

  LIGMA_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
ligma_dodge_burn_tool_cursor_update (LigmaTool         *tool,
                                    const LigmaCoords *coords,
                                    GdkModifierType   state,
                                    LigmaDisplay      *display)
{
  LigmaDodgeBurnOptions *options = LIGMA_DODGE_BURN_TOOL_GET_OPTIONS (tool);

  ligma_tool_control_set_toggled (tool->control,
                                 options->type == LIGMA_DODGE_BURN_TYPE_BURN);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);
}

static void
ligma_dodge_burn_tool_oper_update (LigmaTool         *tool,
                                  const LigmaCoords *coords,
                                  GdkModifierType   state,
                                  gboolean          proximity,
                                  LigmaDisplay      *display)
{
  LigmaDodgeBurnOptions *options = LIGMA_DODGE_BURN_TOOL_GET_OPTIONS (tool);

  ligma_dodge_burn_tool_status_update (tool, options->type);

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
ligma_dodge_burn_tool_status_update (LigmaTool          *tool,
                                    LigmaDodgeBurnType  type)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (tool);

  switch (type)
    {
    case LIGMA_DODGE_BURN_TYPE_DODGE:
      paint_tool->status      = _("Click to dodge");
      paint_tool->status_line = _("Click to dodge the line");
      paint_tool->status_ctrl = _("%s to burn");
      break;

    case LIGMA_DODGE_BURN_TYPE_BURN:
      paint_tool->status      = _("Click to burn");
      paint_tool->status_line = _("Click to burn the line");
      paint_tool->status_ctrl = _("%s to dodge");
      break;

    default:
      break;
    }
}


/*  tool options stuff  */

static GtkWidget *
ligma_dodge_burn_options_gui (LigmaToolOptions *tool_options)
{
  GObject         *config = G_OBJECT (tool_options);
  GtkWidget       *vbox   = ligma_paint_options_gui (tool_options);
  GtkWidget       *frame;
  GtkWidget       *scale;
  gchar           *str;
  GdkModifierType  toggle_mask;

  toggle_mask = ligma_get_toggle_behavior_mask ();

  /* the type (dodge or burn) */
  str = g_strdup_printf (_("Type  (%s)"),
                         ligma_get_mod_string (toggle_mask));
  frame = ligma_prop_enum_radio_frame_new (config, "type",
                                          str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  g_free (str);

  /*  mode (highlights, midtones, or shadows)  */
  frame = ligma_prop_enum_radio_frame_new (config, "mode", NULL,
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  /*  the exposure scale  */
  scale = ligma_prop_spin_scale_new (config, "exposure",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return vbox;
}
