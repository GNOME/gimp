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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimptoolinfo.h"

#include "paint/gimpdodgeburnoptions.h"

#include "widgets/gimphelp-ids.h"
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
                                                  GimpCoords        *coords,
                                                  GdkModifierType    state,
                                                  GimpDisplay       *display);
static void   gimp_dodge_burn_tool_oper_update   (GimpTool          *tool,
                                                  GimpCoords        *coords,
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
                _("Dodge/Burn"),
                _("Dodge or Burn strokes"),
                N_("Dod_geBurn"), "<shift>D",
                NULL, GIMP_HELP_TOOL_DODGE_BURN,
                GIMP_STOCK_TOOL_DODGE,
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

  gimp_dodge_burn_tool_status_update (tool, GIMP_BURN);
}

static void
gimp_dodge_burn_tool_modifier_key (GimpTool        *tool,
                                   GdkModifierType  key,
                                   gboolean         press,
                                   GdkModifierType  state,
                                   GimpDisplay     *display)
{
  GimpDodgeBurnOptions *options;

  options = GIMP_DODGE_BURN_OPTIONS (tool->tool_info->tool_options);

  if (key == GDK_CONTROL_MASK &&
      ! (state & GDK_SHIFT_MASK)) /* leave stuff untouched in line draw mode */
    {
      switch (options->type)
        {
        case GIMP_DODGE:
          g_object_set (options, "type", GIMP_BURN, NULL);
          break;

        case GIMP_BURN:
          g_object_set (options, "type", GIMP_DODGE, NULL);
          break;

        default:
          break;
        }
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
gimp_dodge_burn_tool_cursor_update (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpDodgeBurnOptions *options;

  options = GIMP_DODGE_BURN_OPTIONS (tool->tool_info->tool_options);

  gimp_tool_control_set_toggled (tool->control, (options->type == GIMP_BURN));

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);
}

static void
gimp_dodge_burn_tool_oper_update (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  GdkModifierType  state,
                                  gboolean         proximity,
                                  GimpDisplay     *display)
{
  GimpDodgeBurnOptions *options;

  options = GIMP_DODGE_BURN_OPTIONS (tool->tool_info->tool_options);

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
    case GIMP_DODGE:
      paint_tool->status      = _("Click to dodge.");
      paint_tool->status_line = _("Click to dodge the line.");
      paint_tool->status_ctrl = _("%s to burn");
      break;

    case GIMP_BURN:
      paint_tool->status      = _("Click to burn.");
      paint_tool->status_line = _("Click to burn the line.");
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
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *table;
  GtkWidget *frame;
  gchar     *str;

  /* the type (dodge or burn) */
  str = g_strdup_printf (_("Type  (%s)"),
                         gimp_get_mod_string (GDK_CONTROL_MASK));

  frame = gimp_prop_enum_radio_frame_new (config, "type",
                                          str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_free (str);

  /*  mode (highlights, midtones, or shadows)  */
  frame = gimp_prop_enum_radio_frame_new (config, "mode",
                                          _("Mode"), 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the exposure scale  */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  gimp_prop_scale_entry_new (config, "exposure",
                             GTK_TABLE (table), 0, 0,
                             _("Exposure:"),
                             1.0, 10.0, 1,
                             FALSE, 0.0, 0.0);

  return vbox;
}
