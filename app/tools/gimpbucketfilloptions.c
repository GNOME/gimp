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

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"

#include "widgets/gimpenummenu.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpbucketfilloptions.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


static void   gimp_bucket_fill_options_init       (GimpBucketFillOptions      *options);
static void   gimp_bucket_fill_options_class_init (GimpBucketFillOptionsClass *options_class);

static void   gimp_bucket_fill_options_reset      (GimpToolOptions *tool_options);


GType
gimp_bucket_fill_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpBucketFillOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_bucket_fill_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpBucketFillOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_bucket_fill_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_OPTIONS,
                                     "GimpBucketFillOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_bucket_fill_options_class_init (GimpBucketFillOptionsClass *klass)
{
}

static void
gimp_bucket_fill_options_init (GimpBucketFillOptions *options)
{
  options->fill_transparent = options->fill_transparent_d = TRUE;
  options->sample_merged    = options->sample_merged_d    = FALSE;
  options->fill_mode        = options->fill_mode_d = GIMP_FG_BUCKET_FILL;
}

void
gimp_bucket_fill_options_gui (GimpToolOptions *tool_options)
{
  GimpBucketFillOptions *options;
  GtkWidget             *vbox;
  GtkWidget             *vbox2;
  GtkWidget             *table;
  GtkWidget             *frame;
  gchar                 *str;

  options = GIMP_BUCKET_FILL_OPTIONS (tool_options);

  options->threshold =
    GIMP_GUI_CONFIG (tool_options->tool_info->gimp->config)->default_threshold;

  gimp_paint_options_gui (tool_options);

  ((GimpToolOptions *) options)->reset_func = gimp_bucket_fill_options_reset;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  str = g_strdup_printf (_("Fill Type  %s"), gimp_get_mod_name_control ());

  /*  fill type  */
  frame = gimp_enum_radio_frame_new (GIMP_TYPE_BUCKET_FILL_MODE,
                                     gtk_label_new (str),
                                     2,
                                     G_CALLBACK (gimp_radio_button_update),
                                     &options->fill_mode,
                                     &options->fill_mode_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->fill_mode_w),
                               GINT_TO_POINTER (options->fill_mode));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_free (str);

  frame = gtk_frame_new (_("Finding Similar Colors"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /*  the fill transparent areas toggle  */
  options->fill_transparent_w =
    gtk_check_button_new_with_label (_("Fill Transparent Areas"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->fill_transparent_w),
                                options->fill_transparent);
  gtk_box_pack_start (GTK_BOX (vbox2), options->fill_transparent_w,
                      FALSE, FALSE, 0);
  gtk_widget_show (options->fill_transparent_w);

  gimp_help_set_help_data (options->fill_transparent_w,
                           _("Allow completely transparent regions "
                             "to be filled"), NULL);

  g_signal_connect (options->fill_transparent_w, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->fill_transparent);

  /*  the sample merged toggle  */
  options->sample_merged_w =
    gtk_check_button_new_with_label (_("Sample Merged"));
  gtk_box_pack_start (GTK_BOX (vbox2), options->sample_merged_w,
                      FALSE, FALSE, 0);
  gtk_widget_show (options->sample_merged_w);

  gimp_help_set_help_data (options->sample_merged_w,
			   _("Base filled area on all visible layers"), NULL);

  g_signal_connect (options->sample_merged_w, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->sample_merged);

  /*  the threshold scale  */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->threshold_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
					       _("Threshold:"), -1, -1,
					       options->threshold,
					       0.0, 255.0, 1.0, 16.0, 1,
					       TRUE, 0.0, 0.0,
					       _("Maximum color difference"),
					       NULL);

  g_signal_connect (options->threshold_w, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->threshold);

  gimp_bucket_fill_options_reset (tool_options);
}

static void
gimp_bucket_fill_options_reset (GimpToolOptions *tool_options)
{
  GimpBucketFillOptions *options;

  options = GIMP_BUCKET_FILL_OPTIONS (tool_options);

  gimp_paint_options_reset (tool_options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->fill_transparent_w),
				options->fill_transparent_d);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged_d);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
			    GIMP_GUI_CONFIG (tool_options->tool_info->gimp->config)->default_threshold);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->fill_mode_w),
                               GINT_TO_POINTER (options->fill_mode_d));
}
