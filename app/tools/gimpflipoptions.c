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

#include "widgets/gimpwidgets-utils.h"

#include "gimpflipoptions.h"

#include "libgimp/gimpintl.h"


static void   gimp_flip_options_init       (GimpFlipOptions      *options);
static void   gimp_flip_options_class_init (GimpFlipOptionsClass *options_class);

static void   gimp_flip_options_reset      (GimpToolOptions      *tool_options);


GType
gimp_flip_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpFlipOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_flip_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpFlipOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_flip_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpFlipOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_flip_options_class_init (GimpFlipOptionsClass *klass)
{
}

static void
gimp_flip_options_init (GimpFlipOptions *options)
{
  options->type = options->type_d = ORIENTATION_HORIZONTAL;
}

void
gimp_flip_options_gui (GimpToolOptions *tool_options)
{
  GimpFlipOptions *options;
  GtkWidget       *vbox;
  GtkWidget       *frame;
  gchar           *str;

  options = GIMP_FLIP_OPTIONS (tool_options);

  ((GimpToolOptions *) options)->reset_func = gimp_flip_options_reset;

  vbox = tool_options->main_vbox;

  /*  tool toggle  */
  str = g_strdup_printf (_("Tool Toggle  %s"), gimp_get_mod_name_control ());

  frame = gimp_radio_group_new2 (TRUE, str,
                                 G_CALLBACK (gimp_radio_button_update),
                                 &options->type,
                                 GINT_TO_POINTER (options->type),

                                 _("Horizontal"),
                                 GINT_TO_POINTER (ORIENTATION_HORIZONTAL),
                                 &options->type_w[0],

                                 _("Vertical"),
                                 GINT_TO_POINTER (ORIENTATION_VERTICAL),
                                 &options->type_w[1],

                                 NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_free (str);
}

static void
gimp_flip_options_reset (GimpToolOptions *tool_options)
{
  GimpFlipOptions *options;

  options = GIMP_FLIP_OPTIONS (tool_options);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w[0]),
                               GINT_TO_POINTER (options->type_d));
}
