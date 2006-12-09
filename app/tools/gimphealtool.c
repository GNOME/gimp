/* GIMP - The GNU Image Manipulation Program
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

#include "paint/gimpheal.h"
#include "paint/gimpsourceoptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimphealtool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static GtkWidget * gimp_heal_options_gui (GimpToolOptions *tool_options);


G_DEFINE_TYPE (GimpHealTool, gimp_heal_tool, GIMP_TYPE_SOURCE_TOOL)


void
gimp_heal_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_HEAL_TOOL,
                GIMP_TYPE_SOURCE_OPTIONS,
                gimp_heal_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK,
                "gimp-heal-tool",
                _("Heal"),
                _("Healing Tool: Heal image irregularities"),
                N_("_Heal"),
                "H",
                NULL,
                GIMP_HELP_TOOL_HEAL,
                GIMP_STOCK_TOOL_HEAL,
                data);
}

static void
gimp_heal_tool_class_init (GimpHealToolClass *klass)
{
}

static void
gimp_heal_tool_init (GimpHealTool *heal)
{
  GimpTool       *tool        = GIMP_TOOL (heal);
  GimpPaintTool  *paint_tool  = GIMP_PAINT_TOOL (tool);
  GimpSourceTool *source_tool = GIMP_SOURCE_TOOL (tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_HEAL);

  paint_tool->status      = _("Click to heal");
  paint_tool->status_ctrl = _("%s to set a new heal source");

  source_tool->status_paint           = _("Click to heal");
  source_tool->status_set_source      = _("Click to set a new heal source");
  source_tool->status_set_source_ctrl = _("%s to set a new heal source");
}


/*  tool options stuff  */

static GtkWidget *
gimp_heal_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *combo;

  /* create and attach the sample merged checkbox */
  button = gimp_prop_check_button_new (config, "sample-merged",
                                       _("Sample merged"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* create and attach the alignment options to a table */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  combo = gimp_prop_enum_combo_box_new (config, "align-mode", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Alignment:"), 0.0, 0.5,
                             combo, 1, FALSE);

  return vbox;
}
