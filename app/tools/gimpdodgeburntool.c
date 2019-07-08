/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "paint/gimpdodgeburnoptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpdodgeburntool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_dodge_burn_tool_modifier_key  (GimpTool          *tool,
                                                  GdkModifierType    key,
                                                  gboolean           press,
                                                  GdkModifierType    state,
                                                  GimpDisplay       *display);
static void   gimp_dodge_burn_tool_cursor_update (GimpTool          *tool,
                                                  const GimpCoords  *coords,
                                                  GdkModifierType    state,
                                                  GimpDisplay       *display);
static void   gimp_dodge_burn_tool_oper_update   (GimpTool          *tool,
                                                  const GimpCoords  *coords,
                                                  GdkModifierType    state,
                                                  gboolean           proximity,
                                                  GimpDisplay       *display);
static void   gimp_dodge_burn_tool_status_update (GimpTool          *tool,
                                                  GimpDodgeBurnType  type);

static GtkWidget * gimp_dodge_burn_options_gui   (GimpToolOptions   *tool_options);


G_DEFINE_TYPE (GimpDodgeBurnTool, gimp_dodge_burn_tool, GIMP_TYPE_BRUSH_TOOL)

#define parent_class gimp_dodge_burn_tool_parent_class


void
gimp_dodge_burn_tool_register (GimpToolRegisterCallback  callback,
                               gpointer                  data)
{
  (* callback) (GIMP_TYPE_DODGE_BURN_TOOL,
                GIMP_TYPE_DODGE_BURN_OPTIONS,
                gimp_dodge_burn_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK,
                "gimp-dodge-burn-tool",
                _("Dodge / Burn"),
                _("Dodge / Burn Tool: Selectively lighten or darken using a brush"),
                N_("Dod_ge / Burn"), "<shift>D",
                NULL, GIMP_HELP_TOOL_DODGE_BURN,
                GIMP_ICON_TOOL_DODGE,
                data);
}

static void
gimp_dodge_burn_tool_class_init (GimpDodgeBurnToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  tool_class->modifier_key  = gimp_dodge_burn_tool_modifier_key;
  tool_class->cursor_update = gimp_dodge_burn_tool_cursor_update;
  tool_class->oper_update   = gimp_dodge_burn_tool_oper_update;
}

static void
gimp_dodge_burn_tool_init (GimpDodgeBurnTool *dodgeburn)
{
  GimpTool *tool = GIMP_TOOL (dodgeburn);

  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_DODGE);
  gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                            GIMP_TOOL_CURSOR_BURN);

  gimp_dodge_burn_tool_status_update (tool, GIMP_DODGE_BURN_TYPE_BURN);
}

static void
gimp_dodge_burn_tool_modifier_key (GimpTool        *tool,
                                   GdkModifierType  key,
                                   gboolean         press,
                                   GdkModifierType  state,
                                   GimpDisplay     *display)
{
  GimpDodgeBurnTool    *dodgeburn   = GIMP_DODGE_BURN_TOOL (tool);
  GimpDodgeBurnOptions *options     = GIMP_DODGE_BURN_TOOL_GET_OPTIONS (tool);
  GdkModifierType       line_mask   = GIMP_PAINT_TOOL_LINE_MASK;
  GdkModifierType       toggle_mask = gimp_get_toggle_behavior_mask ();

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
        case GIMP_DODGE_BURN_TYPE_DODGE:
          g_object_set (options, "type", GIMP_DODGE_BURN_TYPE_BURN, NULL);
          break;

        case GIMP_DODGE_BURN_TYPE_BURN:
          g_object_set (options, "type", GIMP_DODGE_BURN_TYPE_DODGE, NULL);
          break;

        default:
          break;
        }
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
gimp_dodge_burn_tool_cursor_update (GimpTool         *tool,
                                    const GimpCoords *coords,
                                    GdkModifierType   state,
                                    GimpDisplay      *display)
{
  GimpDodgeBurnOptions *options = GIMP_DODGE_BURN_TOOL_GET_OPTIONS (tool);

  gimp_tool_control_set_toggled (tool->control,
                                 options->type == GIMP_DODGE_BURN_TYPE_BURN);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);
}

static void
gimp_dodge_burn_tool_oper_update (GimpTool         *tool,
                                  const GimpCoords *coords,
                                  GdkModifierType   state,
                                  gboolean          proximity,
                                  GimpDisplay      *display)
{
  GimpDodgeBurnOptions *options = GIMP_DODGE_BURN_TOOL_GET_OPTIONS (tool);

  gimp_dodge_burn_tool_status_update (tool, options->type);

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
gimp_dodge_burn_tool_status_update (GimpTool          *tool,
                                    GimpDodgeBurnType  type)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);

  switch (type)
    {
    case GIMP_DODGE_BURN_TYPE_DODGE:
      paint_tool->status      = _("Click to dodge");
      paint_tool->status_line = _("Click to dodge the line");
      paint_tool->status_ctrl = _("%s to burn");
      break;

    case GIMP_DODGE_BURN_TYPE_BURN:
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
gimp_dodge_burn_options_gui (GimpToolOptions *tool_options)
{
  GObject         *config = G_OBJECT (tool_options);
  GtkWidget       *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget       *frame;
  GtkWidget       *scale;
  gchar           *str;
  GdkModifierType  toggle_mask;

  toggle_mask = gimp_get_toggle_behavior_mask ();

  /* the type (dodge or burn) */
  str = g_strdup_printf (_("Type  (%s)"),
                         gimp_get_mod_string (toggle_mask));

  frame = gimp_prop_enum_radio_frame_new (config, "type",
                                          str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_free (str);

  /*  mode (highlights, midtones, or shadows)  */
  frame = gimp_prop_enum_radio_frame_new (config, "mode", NULL,
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the exposure scale  */
  scale = gimp_prop_spin_scale_new (config, "exposure", NULL,
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  return vbox;
}
