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

#include "paint/gimpsmudge.h"

#include "gimpsmudgetool.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


static void   gimp_smudge_tool_class_init     (GimpSmudgeToolClass *klass);
static void   gimp_smudge_tool_init           (GimpSmudgeTool      *tool);

static GimpToolOptions * smudge_options_new   (GimpToolInfo        *tool_info);
static void              smudge_options_reset (GimpToolOptions     *tool_options);


static GimpPaintToolClass *parent_class = NULL;


/* global functions  */

void
gimp_smudge_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_SMUDGE_TOOL,
                smudge_options_new,
                TRUE,
                "gimp-smudge-tool",
                _("Smudge"),
                _("Smudge image"),
                N_("/Tools/Paint Tools/Smudge"), "S",
                NULL, "tools/smudge.html",
                GIMP_STOCK_TOOL_SMUDGE,
                data);
}

GType
gimp_smudge_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpSmudgeToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_smudge_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpSmudgeTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_smudge_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_PAINT_TOOL,
					  "GimpSmudgeTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_smudge_tool_class_init (GimpSmudgeToolClass *klass)
{
  GimpPaintToolClass *paint_tool_class;

  paint_tool_class = GIMP_PAINT_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
}

static void
gimp_smudge_tool_init (GimpSmudgeTool *smudge)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (smudge);
  paint_tool = GIMP_PAINT_TOOL (smudge);

  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_SMUDGE_TOOL_CURSOR);

  paint_tool->pick_colors = TRUE;
  paint_tool->core        = g_object_new (GIMP_TYPE_SMUDGE, NULL);
}


/*  tool options stuff  */

static GimpToolOptions *
smudge_options_new (GimpToolInfo *tool_info)
{
  GimpSmudgeOptions *options;
  GtkWidget         *vbox;
  GtkWidget         *table;

  options = gimp_smudge_options_new ();

  paint_options_init ((GimpPaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = smudge_options_reset;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  /*  the rate scale  */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->rate_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                          _("Rate:"), -1, 50,
                                          options->rate,
                                          0.0, 100.0, 1.0, 10.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL);

  g_signal_connect (G_OBJECT (options->rate_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->rate);

  return (GimpToolOptions *) options;
}

static void
smudge_options_reset (GimpToolOptions *tool_options)
{
  GimpSmudgeOptions *options;

  options = (GimpSmudgeOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->rate_w), options->rate_d);
}
