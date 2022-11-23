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

#include "paint/ligmasourceoptions.h"

#include "widgets/ligmahelp-ids.h"

#include "ligmahealtool.h"
#include "ligmapaintoptions-gui.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static GtkWidget * ligma_heal_options_gui (LigmaToolOptions *tool_options);


G_DEFINE_TYPE (LigmaHealTool, ligma_heal_tool, LIGMA_TYPE_SOURCE_TOOL)


void
ligma_heal_tool_register (LigmaToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (LIGMA_TYPE_HEAL_TOOL,
                LIGMA_TYPE_SOURCE_OPTIONS,
                ligma_heal_options_gui,
                LIGMA_PAINT_OPTIONS_CONTEXT_MASK,
                "ligma-heal-tool",
                _("Healing"),
                _("Healing Tool: Heal image irregularities"),
                N_("_Heal"),
                "H",
                NULL,
                LIGMA_HELP_TOOL_HEAL,
                LIGMA_ICON_TOOL_HEAL,
                data);
}

static void
ligma_heal_tool_class_init (LigmaHealToolClass *klass)
{
}

static void
ligma_heal_tool_init (LigmaHealTool *heal)
{
  LigmaTool       *tool        = LIGMA_TOOL (heal);
  LigmaPaintTool  *paint_tool  = LIGMA_PAINT_TOOL (tool);
  LigmaSourceTool *source_tool = LIGMA_SOURCE_TOOL (tool);

  ligma_tool_control_set_tool_cursor (tool->control, LIGMA_TOOL_CURSOR_HEAL);

  paint_tool->status      = _("Click to heal");
  paint_tool->status_ctrl = _("%s to set a new heal source");

  source_tool->status_paint           = _("Click to heal");
  /* Translators: the translation of "Click" must be the first word */
  source_tool->status_set_source      = _("Click to set a new heal source");
  source_tool->status_set_source_ctrl = _("%s to set a new heal source");
}


/*  tool options stuff  */

static GtkWidget *
ligma_heal_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = ligma_paint_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *combo;

  /* the sample merged checkbox */
  button = ligma_prop_check_button_new (config, "sample-merged",
                                       _("Sample merged"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /* the alignment combo */
  combo = ligma_prop_enum_combo_box_new (config, "align-mode", 0, 0);
  ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Alignment"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);

  return vbox;
}
