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

#include "paint/gimpairbrush.h"

#include "gimpairbrushtool.h"
#include "paint_options.h"
#include "gimptool.h"

#include "libgimp/gimpintl.h"


#define MAX_PRESSURE 0.075


static void   gimp_airbrush_tool_class_init (GimpAirbrushToolClass *klass);
static void   gimp_airbrush_tool_init       (GimpAirbrushTool      *airbrush);

static GimpToolOptions * airbrush_options_new   (GimpToolInfo    *tool_info);
static void              airbrush_options_reset (GimpToolOptions *tool_options);


static GimpPaintToolClass *parent_class = NULL;


/*  functions  */

void
gimp_airbrush_tool_register (Gimp                     *gimp,
                             GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_AIRBRUSH_TOOL,
                airbrush_options_new,
                TRUE,
                "gimp-airbrush-tool",
                _("Airbrush"),
                _("Airbrush with variable pressure"),
                N_("/Tools/Paint Tools/Airbrush"), "A",
                NULL, "tools/airbrush.html",
                GIMP_STOCK_TOOL_AIRBRUSH);
}

GType
gimp_airbrush_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpAirbrushToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_airbrush_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpAirbrushTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_airbrush_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_PAINT_TOOL,
					  "GimpAirbrushTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void 
gimp_airbrush_tool_class_init (GimpAirbrushToolClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
}

static void
gimp_airbrush_tool_init (GimpAirbrushTool *airbrush)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (airbrush);
  paint_tool = GIMP_PAINT_TOOL (airbrush);

  tool->tool_cursor       = GIMP_AIRBRUSH_TOOL_CURSOR;

  paint_tool->pick_colors = TRUE;
  paint_tool->core        = g_object_new (GIMP_TYPE_AIRBRUSH, NULL);
}


/*  tool options stuff  */

static GimpToolOptions *
airbrush_options_new (GimpToolInfo *tool_info)
{
  GimpAirbrushOptions *options;
  GtkWidget           *vbox;
  GtkWidget           *table;

  options = gimp_airbrush_options_new ();

  paint_options_init ((GimpPaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = airbrush_options_reset;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  /*  the rate scale  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->rate_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
					  _("Rate:"), -1, 50,
					  options->rate_d,
					  0.0, 150.0, 1.0, 1.0, 1,
					  TRUE, 0.0, 0.0,
					  NULL, NULL);

  g_signal_connect (G_OBJECT (options->rate_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->rate);

  /*  the pressure scale  */
  options->pressure_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
					      _("Pressure:"), -1, 50,
					      options->pressure_d,
					      0.0, 100.0, 1.0, 1.0, 1,
					      TRUE, 0.0, 0.0,
					      NULL, NULL);

  g_signal_connect (G_OBJECT (options->pressure_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->pressure);

  return (GimpToolOptions *) options;
}

static void
airbrush_options_reset (GimpToolOptions *tool_options)
{
  GimpAirbrushOptions *options;

  options = (GimpAirbrushOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->rate_w),
			    options->rate_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->pressure_w),
			    options->pressure_d);
}
