/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Measure tool
 * Copyright (C) 1999 Sven Neumann <sven@gimp.org>
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

#include "gimpmeasureoptions.h"

#include "libgimp/gimpintl.h"


static void   gimp_measure_options_init       (GimpMeasureOptions      *options);
static void   gimp_measure_options_class_init (GimpMeasureOptionsClass *options_class);

static void   gimp_measure_options_reset      (GimpToolOptions *tool_options);


GType
gimp_measure_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpMeasureOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_measure_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpMeasureOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_measure_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpMeasureOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_measure_options_class_init (GimpMeasureOptionsClass *klass)
{
}

static void
gimp_measure_options_init (GimpMeasureOptions *options)
{
  options->use_info_window = options->use_info_window_d  = FALSE;
}

void
gimp_measure_options_gui (GimpToolOptions *tool_options)
{
  GimpMeasureOptions *options;
  GtkWidget          *vbox;

  options = GIMP_MEASURE_OPTIONS (tool_options);

  tool_options->reset_func = gimp_measure_options_reset;

  vbox = tool_options->main_vbox;

  /*  the use_info_window toggle button  */
  options->use_info_window_w =
    gtk_check_button_new_with_label (_("Use Info Window"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_info_window_w),
				options->use_info_window);
  gtk_box_pack_start (GTK_BOX (vbox), options->use_info_window_w,
		      FALSE, FALSE, 0);
  gtk_widget_show (options->use_info_window_w);

  g_signal_connect (options->use_info_window_w, "toggled",
		    G_CALLBACK (gimp_toggle_button_update),
		    &options->use_info_window);
}

static void
gimp_measure_options_reset (GimpToolOptions *tool_options)
{
  GimpMeasureOptions *options;

  options = GIMP_MEASURE_OPTIONS (tool_options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_info_window_w),
				options->use_info_window_d);
}
