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

#include "core/core-types.h"
#include "libgimptool/gimptooltypes.h"

#include "core/gimptoolinfo.h"

#include "paint/gimpdodgeburn.h"

#include "widgets/gimpenummenu.h"

#include "gimpdodgeburntool.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


static void   gimp_dodgeburn_tool_class_init (GimpDodgeBurnToolClass *klass);
static void   gimp_dodgeburn_tool_init       (GimpDodgeBurnTool      *dodgeburn);

static void   gimp_dodgeburn_tool_modifier_key  (GimpTool        *tool,
                                                 GdkModifierType  key,
                                                 gboolean         press,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *gdisp);
static void   gimp_dodgeburn_tool_cursor_update (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *gdisp);

static GimpToolOptions * gimp_dodgeburn_tool_options_new   (GimpToolInfo    *tool_info);
static void              gimp_dodgeburn_tool_options_reset (GimpToolOptions *tool_options);


static GimpPaintToolClass *parent_class = NULL;


void
gimp_dodgeburn_tool_register (GimpToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (GIMP_TYPE_DODGEBURN_TOOL,
                gimp_dodgeburn_tool_options_new,
                TRUE,
                "gimp-dodgeburn-tool",
                _("Dodge/Burn"),
                _("Dodge or Burn strokes"),
                N_("/Tools/Paint Tools/DodgeBurn"), "<shift>D",
                NULL, "tools/dodgeburn.html",
                GIMP_STOCK_TOOL_DODGE,
                data);
}

GType
gimp_dodgeburn_tool_get_type (void)
{
  static GType tool_type = 0;

  if (!tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpDodgeBurnToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_dodgeburn_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpDodgeBurnTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_dodgeburn_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_PAINT_TOOL,
					  "GimpDodgeBurnTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_dodgeburn_tool_class_init (GimpDodgeBurnToolClass *klass)
{
  GimpToolClass	*tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->modifier_key  = gimp_dodgeburn_tool_modifier_key;
  tool_class->cursor_update = gimp_dodgeburn_tool_cursor_update;
}

static void
gimp_dodgeburn_tool_init (GimpDodgeBurnTool *dodgeburn)
{
  GimpTool	*tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (dodgeburn);
  paint_tool = GIMP_PAINT_TOOL (dodgeburn);

  gimp_tool_control_set_motion_mode        (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_tool_cursor        (tool->control, GIMP_DODGE_TOOL_CURSOR);
  gimp_tool_control_set_toggle_tool_cursor (tool->control, GIMP_BURN_TOOL_CURSOR);

  paint_tool->core = g_object_new (GIMP_TYPE_DODGEBURN, NULL);
}

static void
gimp_dodgeburn_tool_modifier_key (GimpTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
				  GdkModifierType  state,
				  GimpDisplay     *gdisp)
{
  GimpDodgeBurnOptions *options;

  options = (GimpDodgeBurnOptions *) tool->tool_info->tool_options;

  if (key == GDK_CONTROL_MASK &&
      ! (state & GDK_SHIFT_MASK)) /* leave stuff untouched in line draw mode */
    {
      switch (options->type)
        {
        case GIMP_DODGE:
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                                       GINT_TO_POINTER (GIMP_BURN));
          break;
        case GIMP_BURN:
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                                       GINT_TO_POINTER (GIMP_DODGE));
          break;
        default:
          break;
        }
    }

  gimp_tool_control_set_toggle (tool->control, (options->type == GIMP_BURN));
}

static void
gimp_dodgeburn_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
				   GdkModifierType  state,
				   GimpDisplay     *gdisp)
{
  GimpDodgeBurnOptions *options;

  options = (GimpDodgeBurnOptions *) tool->tool_info->tool_options;


  gimp_tool_control_set_toggle (tool->control, (options->type == GIMP_BURN));

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}


/*  tool options stuff  */

static GimpToolOptions *
gimp_dodgeburn_tool_options_new (GimpToolInfo *tool_info)
{
  GimpDodgeBurnOptions *options;
  GtkWidget            *vbox;
  GtkWidget            *table;
  GtkWidget            *frame;

  options = gimp_dodgeburn_options_new ();

  paint_options_init ((GimpPaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = gimp_dodgeburn_tool_options_reset;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  /* the type (dodge or burn) */
  frame = gimp_enum_radio_frame_new (GIMP_TYPE_DODGE_BURN_TYPE,
                                     gtk_label_new (_("Type (<Ctrl>)")),
                                     2,
                                     G_CALLBACK (gimp_radio_button_update),
                                     &options->type,
                                     &options->type_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                               GINT_TO_POINTER (options->type));

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  mode (highlights, midtones, or shadows)  */
  frame = gimp_enum_radio_frame_new (GIMP_TYPE_TRANSFER_MODE,
                                     gtk_label_new (_("Mode")),
                                     2,
                                     G_CALLBACK (gimp_radio_button_update),
                                     &options->mode,
                                     &options->mode_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->mode_w),
                               GINT_TO_POINTER (options->mode));

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the exposure scale  */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->exposure_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
					      _("Exposure:"), -1, 50,
					      options->exposure,
					      0.0, 100.0, 1.0, 10.0, 1,
					      TRUE, 0.0, 0.0,
					      NULL, NULL);

  g_signal_connect (G_OBJECT (options->exposure_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->exposure);

  return (GimpToolOptions *) options;
}

static void
gimp_dodgeburn_tool_options_reset (GimpToolOptions *tool_options)
{
  GimpDodgeBurnOptions *options;

  options = (GimpDodgeBurnOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->exposure_w),
			    options->exposure_d);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                               GINT_TO_POINTER (options->type_d));

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->mode_w),
                               GINT_TO_POINTER (options->mode_d));
}
