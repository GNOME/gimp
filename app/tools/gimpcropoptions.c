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

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpenummenu.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpcropoptions.h"

#include "libgimp/gimpintl.h"


static void   gimp_crop_options_init       (GimpCropOptions      *options);
static void   gimp_crop_options_class_init (GimpCropOptionsClass *options_class);

static void   gimp_crop_options_reset      (GimpToolOptions *tool_options);


GType
gimp_crop_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpCropOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_crop_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpCropOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_crop_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpCropOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_crop_options_class_init (GimpCropOptionsClass *klass)
{
}

static void
gimp_crop_options_init (GimpCropOptions *options)
{
  options->layer_only    = options->layer_only_d    = FALSE;
  options->allow_enlarge = options->allow_enlarge_d = FALSE;
  options->type          = options->type_d          = GIMP_CROP;
}

void
gimp_crop_options_gui (GimpToolOptions *tool_options)
{
  GimpCropOptions *options;
  GtkWidget       *vbox;
  GtkWidget       *frame;
  gchar           *str;

  options = GIMP_CROP_OPTIONS (tool_options);

  ((GimpToolOptions *) options)->reset_func = gimp_crop_options_reset;

  /*  the main vbox  */
  vbox = GIMP_TOOL_OPTIONS (options)->main_vbox;

  /*  tool toggle  */
  str = g_strdup_printf (_("Tool Toggle  %s"), gimp_get_mod_name_control ());

  frame = gimp_enum_radio_frame_new (GIMP_TYPE_CROP_TYPE,
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

  /*  layer toggle  */
  options->layer_only_w =
    gtk_check_button_new_with_label (_("Current Layer only"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->layer_only_w),
				options->layer_only);
  gtk_box_pack_start (GTK_BOX (vbox), options->layer_only_w,
		      FALSE, FALSE, 0);
  gtk_widget_show (options->layer_only_w);

  g_signal_connect (options->layer_only_w, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->layer_only);

  /*  enlarge toggle  */
  str = g_strdup_printf (_("Allow Enlarging  %s"), gimp_get_mod_name_alt ());

  options->allow_enlarge_w = gtk_check_button_new_with_label (str);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_enlarge_w),
				options->allow_enlarge);
  gtk_box_pack_start (GTK_BOX (vbox), options->allow_enlarge_w,
		      FALSE, FALSE, 0);
  gtk_widget_show (options->allow_enlarge_w);

  g_free (str);

  g_signal_connect (options->allow_enlarge_w, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->allow_enlarge);
}

static void
gimp_crop_options_reset (GimpToolOptions *tool_options)
{
  GimpCropOptions *options;

  options = GIMP_CROP_OPTIONS (tool_options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->layer_only_w),
				options->layer_only_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(options->allow_enlarge_w),
				options->allow_enlarge_d);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                               GINT_TO_POINTER (options->type_d));
}
