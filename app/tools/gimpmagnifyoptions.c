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

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpenummenu.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpmagnifyoptions.h"

#include "libgimp/gimpintl.h"


static void   gimp_magnify_options_init       (GimpMagnifyOptions      *options);
static void   gimp_magnify_options_class_init (GimpMagnifyOptionsClass *options_class);

static void   gimp_magnify_options_reset      (GimpToolOptions *tool_options);


GType
gimp_magnify_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpMagnifyOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_magnify_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpMagnifyOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_magnify_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpMagnifyOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_magnify_options_class_init (GimpMagnifyOptionsClass *klass)
{
}

static void
gimp_magnify_options_init (GimpMagnifyOptions *options)
{
  options->type_d       = options->type      = GIMP_ZOOM_IN;
  options->threshold_d  = options->threshold = 5;
}

void
gimp_magnify_options_gui (GimpToolOptions *tool_options)
{
  GimpMagnifyOptions *options;
  GtkWidget          *vbox;
  GtkWidget          *frame;
  GtkWidget          *table;
  gchar              *str;

  options = GIMP_MAGNIFY_OPTIONS (tool_options);

  tool_options->reset_func = gimp_magnify_options_reset;

  options->allow_resize =
    GIMP_DISPLAY_CONFIG (tool_options->tool_info->gimp->config)->resize_windows_on_zoom;
  options->type_d       = options->type      = GIMP_ZOOM_IN;
  options->threshold_d  = options->threshold = 5;

  vbox = tool_options->main_vbox;

  /*  the allow_resize toggle button  */
  options->allow_resize_w =
    gtk_check_button_new_with_label (_("Allow Window Resizing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_resize_w),
				options->allow_resize);
  gtk_box_pack_start (GTK_BOX (vbox),  options->allow_resize_w,
                      FALSE, FALSE, 0);
  gtk_widget_show (options->allow_resize_w);

  g_signal_connect (options->allow_resize_w, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->allow_resize);

  /*  tool toggle  */
  str = g_strdup_printf (_("Tool Toggle  %s"), gimp_get_mod_name_control ());

  frame = gimp_enum_radio_frame_new (GIMP_TYPE_ZOOM_TYPE,
                                     gtk_label_new (str),
                                     2,
                                     G_CALLBACK (gimp_radio_button_update),
                                     &options->type,
                                     &options->type_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                               GINT_TO_POINTER (options->type));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_free (str);

  /*  window threshold */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->threshold_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
					       _("Threshold:"), -1, -1,
					       options->threshold,
					       1.0, 15.0, 1.0, 3.0, 1,
					       TRUE, 0.0, 0.0,
					       NULL, NULL);

  g_signal_connect (options->threshold_w, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->threshold);
}

static void
gimp_magnify_options_reset (GimpToolOptions *tool_options)
{
  GimpMagnifyOptions *options;

  options = GIMP_MAGNIFY_OPTIONS (tool_options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_resize_w),
				GIMP_DISPLAY_CONFIG (tool_options->tool_info->gimp->config)->resize_windows_on_zoom);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                               GINT_TO_POINTER (options->type_d));

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
			    options->threshold_d);
}
