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


#define SMUDGE_DEFAULT_RATE  50.0


static void   gimp_smudge_tool_class_init     (GimpSmudgeToolClass *klass);
static void   gimp_smudge_tool_init           (GimpSmudgeTool      *tool);

static GimpToolOptions * smudge_options_new   (GimpToolInfo        *tool_info);
static void              smudge_options_reset (GimpToolOptions     *tool_options);


static GimpPaintToolClass *parent_class = NULL;


/* global functions  */

void
gimp_smudge_tool_register (Gimp                     *gimp,
                           GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_SMUDGE_TOOL,
                smudge_options_new,
                TRUE,
                "gimp:smudge_tool",
                _("Smudge"),
                _("Smudge image"),
                N_("/Tools/Paint Tools/Smudge"), "S",
                NULL, "tools/smudge.html",
                GIMP_STOCK_TOOL_SMUDGE);
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

  tool->tool_cursor       = GIMP_SMUDGE_TOOL_CURSOR;

  paint_tool->pick_colors = TRUE;
  paint_tool->core        = g_object_new (GIMP_TYPE_SMUDGE, NULL);
}


#if 0

static GimpSmudgeTool *non_gui_smudge = NULL;

gboolean
gimp_smudge_tool_non_gui_default (GimpDrawable *drawable,
				  gint          num_strokes,
				  gdouble      *stroke_array)
{
  GimpToolInfo  *tool_info;
  SmudgeOptions *options;
  gdouble        rate = SMUDGE_DEFAULT_RATE;

  tool_info = tool_manager_get_info_by_type (drawable->gimage->gimp,
                                             GIMP_TYPE_SMUDGE_TOOL);

  options = (SmudgeOptions *) tool_info->tool_options;

  if (options)
    rate = options->rate;

  return gimp_smudge_tool_non_gui (drawable, rate, num_strokes, stroke_array);
}

gboolean
gimp_smudge_tool_non_gui (GimpDrawable *drawable,
			  gdouble       rate,
			  gint          num_strokes,
			  gdouble      *stroke_array)
{
  GimpPaintTool *paint_tool;
  gint           i;

  if (! non_gui_smudge)
    {
      non_gui_smudge = g_object_new (GIMP_TYPE_SMUDGE_TOOL, NULL);
    }

  paint_tool = GIMP_PAINT_TOOL (non_gui_smudge);

  if (gimp_paint_tool_start (paint_tool, drawable,
			    stroke_array[0], stroke_array[1]))
    {
      gimp_smudge_tool_start (paint_tool, drawable);

      non_gui_rate = rate;

      paint_tool->cur_coords.x = paint_tool->start_coords.x = 
	paint_tool->last_coords.x = stroke_array[0];
      paint_tool->cur_coords.y = paint_tool->start_coords.y = 
	paint_tool->last_coords.y = stroke_array[1];

      gimp_smudge_tool_paint (paint_tool, drawable, 0); 

      for (i = 1; i < num_strokes; i++)
	{
	  paint_tool->cur_coords.x = stroke_array[i * 2 + 0];
	  paint_tool->cur_coords.y = stroke_array[i * 2 + 1];

	  gimp_paint_tool_interpolate (paint_tool, drawable);

	  paint_tool->last_coords.x = paint_tool->cur_coords.x;
	  paint_tool->last_coords.y = paint_tool->cur_coords.y;
	}

      gimp_paint_tool_finish (paint_tool, drawable);

      gimp_paint_tool_cleanup (paint_tool);

      gimp_smudge_tool_finish (paint_tool, drawable);

      return TRUE;
    }

  return FALSE;
}

#endif


/*  tool options stuff  */

static GimpToolOptions *
smudge_options_new (GimpToolInfo *tool_info)
{
  SmudgeOptions *options;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkWidget     *scale;

  options = g_new0 (SmudgeOptions, 1);

  paint_options_init ((PaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = smudge_options_reset;

  options->rate = options->rate_d = SMUDGE_DEFAULT_RATE;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  /*  the rate scale  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Rate:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->rate_w =
    gtk_adjustment_new (options->rate_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->rate_w));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  g_signal_connect (G_OBJECT (options->rate_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->rate);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);

  return (GimpToolOptions *) options;
}

static void
smudge_options_reset (GimpToolOptions *tool_options)
{
  SmudgeOptions *options;

  options = (SmudgeOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->rate_w), options->rate_d);
}
