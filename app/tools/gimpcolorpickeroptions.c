/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimppaletteeditor.h"
#include "widgets/gimpviewabledialog.h"

#include "gimpcolorpickeroptions.h"

#include "libgimp/gimpintl.h"


static void   gimp_color_picker_options_init       (GimpColorPickerOptions      *options);
static void   gimp_color_picker_options_class_init (GimpColorPickerOptionsClass *options_class);

static void   gimp_color_picker_options_reset      (GimpToolOptions *tool_options);


GType
gimp_color_picker_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpColorPickerOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_picker_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorPickerOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_picker_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpColorPickerOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_color_picker_options_class_init (GimpColorPickerOptionsClass *klass)
{
}

static void
gimp_color_picker_options_init (GimpColorPickerOptions *options)
{
  options->sample_merged  = options->sample_merged_d  = FALSE;
  options->sample_average = options->sample_average_d = FALSE;
  options->average_radius = options->average_radius_d = 1.0;
  options->update_active  = options->update_active_d  = TRUE;
}

void
gimp_color_picker_options_gui (GimpToolOptions *tool_options)
{
  GimpColorPickerOptions *options;
  GtkWidget              *vbox;
  GtkWidget              *frame;
  GtkWidget              *table;

  options = GIMP_COLOR_PICKER_OPTIONS (tool_options);

  ((GimpToolOptions *) options)->reset_func = gimp_color_picker_options_reset;

  /*  the main vbox  */
  vbox = GIMP_TOOL_OPTIONS (options)->main_vbox;

  /*  the sample merged toggle button  */
  options->sample_merged_w =
    gtk_check_button_new_with_label (_("Sample Merged"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged);
  gtk_box_pack_start (GTK_BOX (vbox), options->sample_merged_w, FALSE, FALSE, 0);
  gtk_widget_show (options->sample_merged_w);

  g_signal_connect (options->sample_merged_w, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->sample_merged);

  /*  the sample average options  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  options->sample_average_w =
    gtk_check_button_new_with_label (_("Sample Average"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_average_w),
				options->sample_average);
  gtk_frame_set_label_widget (GTK_FRAME (frame), options->sample_average_w);
  gtk_widget_show (options->sample_average_w);

  g_signal_connect (options->sample_average_w, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->sample_average);

  gtk_widget_set_sensitive (table, options->sample_average);
  g_object_set_data (G_OBJECT (options->sample_average_w), "set_sensitive",
		     table);

  options->average_radius_w =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			  _("Radius:"), -1, -1,
			  options->average_radius,
			  1.0, 15.0, 1.0, 3.0, 0,
			  TRUE, 0.0, 0.0,
			  NULL, NULL);

  g_signal_connect (options->average_radius_w, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->average_radius);

  /*  the update active color toggle button  */
  options->update_active_w =
    gtk_check_button_new_with_label (_("Update Active Color"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->update_active_w),
				options->update_active);
  gtk_box_pack_start (GTK_BOX (vbox), options->update_active_w, FALSE, FALSE, 0);
  gtk_widget_show (options->update_active_w);

  g_signal_connect (options->update_active_w, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->update_active);
}

static void
gimp_color_picker_options_reset (GimpToolOptions *tool_options)
{
  GimpColorPickerOptions *options;

  options = GIMP_COLOR_PICKER_OPTIONS (tool_options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_average_w),
				options->sample_average_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->average_radius_w),
			    options->average_radius_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->update_active_w),
				options->update_active_d);
}
