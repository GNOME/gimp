/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "ligmaressionist.h"
#include "color.h"

#include "libligma/stdplugins-intl.h"


#define NUMCOLORRADIO 2

static GtkWidget *colorradio[NUMCOLORRADIO];
static GtkWidget *colornoiseadjust = NULL;

void
color_restore (void)
{
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (colorradio[pcvals.color_type]), TRUE);

  ligma_label_spin_set_value (LIGMA_LABEL_SPIN (colornoiseadjust),
                             pcvals.color_noise);
}

int
color_type_input (int in)
{
  return CLAMP_UP_TO (in, NUMCOLORRADIO);
}

void
create_colorpage (GtkNotebook *notebook)
{
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *frame;

  label = gtk_label_new_with_mnemonic (_("Co_lor"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_widget_show (vbox);

  frame = ligma_int_radio_group_new (TRUE, _("Color"),
                                    G_CALLBACK (ligma_radio_button_update),
                                    &pcvals.color_type, NULL, 0,

                                    _("A_verage under brush"),
                                    COLOR_TYPE_AVERAGE, &colorradio[COLOR_TYPE_AVERAGE],
                                    _("C_enter of brush"),
                                    COLOR_TYPE_CENTER, &colorradio[COLOR_TYPE_CENTER],

                                    NULL);

  ligma_help_set_help_data
    (colorradio[COLOR_TYPE_AVERAGE],
     _("Color is computed from the average of all pixels under the brush"),
     NULL);
  ligma_help_set_help_data
    (colorradio[COLOR_TYPE_CENTER],
     _("Samples the color from the pixel in the center of the brush"), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  colornoiseadjust =
    ligma_scale_entry_new (_("Color _noise:"), pcvals.color_noise, 0.0, 100.0, 0);
  ligma_help_set_help_data (colornoiseadjust,
                           _("Adds random noise to the color"),
                           NULL);
  g_signal_connect (colornoiseadjust, "value-changed",
                    G_CALLBACK (ligmaressionist_scale_entry_update_double),
                    &pcvals.color_noise);
  gtk_box_pack_start (GTK_BOX (vbox), colornoiseadjust, FALSE, FALSE, 6);
  gtk_widget_show (colornoiseadjust);


  color_restore ();

  gtk_notebook_append_page_menu (notebook, vbox, label, NULL);
}
