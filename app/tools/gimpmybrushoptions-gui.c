/* GIMP - The GNU Image Manipulation Program
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"

#include "paint/gimpmybrushoptions.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewablebox.h"

#include "gimpmybrushoptions-gui.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"

static void
on_spectral_checkbox_toggled (GtkToggleButton *toggle, gpointer user_data)
{
    GtkWidget *slider = GTK_WIDGET (user_data);
    gboolean   active = gtk_toggle_button_get_active (toggle);
    gtk_widget_set_sensitive (slider, active);
}

GtkWidget *
gimp_mybrush_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *vbox2;
  GtkWidget *button;
  GtkWidget *scale;
  GtkWidget *combo_box;
  GtkWidget *frame;
  GtkWidget *spectral_expander;
  GtkWidget *spectral_check_box;
  GtkWidget *spectral_slider;

  /* Since MyPaint Brushes have their own custom sliders for brush options,
   * we'll need to shift them up to match the layout of other brushes */
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox2, TRUE);
  gtk_box_reorder_child (GTK_BOX (vbox), vbox2, 2);

  /* the brush */
  button = gimp_prop_mybrush_box_new (NULL, GIMP_CONTEXT (tool_options),
                                      _("Brush"), 2,
                                      NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);

  /* radius */
  scale = gimp_prop_spin_scale_new (config, "radius",
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* opaque */
  scale = gimp_prop_spin_scale_new (config, "opaque",
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* hardness */
  scale = gimp_prop_spin_scale_new (config, "hardness",
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* pressure gain */
  scale = gimp_prop_spin_scale_new (config, "gain",
                                    0.1, 0.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* erase mode */
  scale = gimp_prop_check_button_new (config, "eraser", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  /* no erasing */
  scale = gimp_prop_check_button_new (config, "no-erasing", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  /* Spectral blending */

/* Spectral blending */

  /* Create an expander for spectral blending options */
  spectral_expander = gtk_expander_new ("Spectral Blending");
  gtk_expander_set_expanded (GTK_EXPANDER (spectral_expander), FALSE);

  /* Create a vertical box to be the single child of the expander */
  GtkWidget *spectral_expander_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (spectral_expander), spectral_expander_box);

  /* Pack the expander into the main vbox */
  gtk_box_pack_start (GTK_BOX (vbox), spectral_expander, FALSE, FALSE, 0);

  /* Create a prop check box to enable/disable spectral blending */
  spectral_check_box = gimp_prop_check_button_new (config, "spectral-blending",
                                                   "Enable Spectral Blending");
  /* Pack into the expander content box (not directly into the expander) */
  gtk_box_pack_start (GTK_BOX (spectral_expander_box), spectral_check_box, FALSE, FALSE, 0);

  /* Create pigment slider (prop spin scale) and disable initially */
  spectral_slider = gimp_prop_spin_scale_new (config, "pigment", 0.0, 1.0, 2);
  gtk_widget_set_sensitive (spectral_slider, FALSE);
  gtk_box_pack_start (GTK_BOX (spectral_expander_box), spectral_slider, FALSE, FALSE, 0);

  GtkWidget *toggle_child = NULL;
  if (GTK_IS_BIN (spectral_check_box))
    toggle_child = gtk_bin_get_child (GTK_BIN (spectral_check_box));

  if (toggle_child && GTK_IS_TOGGLE_BUTTON (toggle_child))
    g_signal_connect (toggle_child, "toggled",
                      G_CALLBACK (on_spectral_checkbox_toggled), spectral_slider);
  else

    /* Connect directly to the prop widget; many prop wrappers proxy signals 
    g_signal_connect (spectral_check_box, "toggled",
                      G_CALLBACK (on_spectral_checkbox_toggled), spectral_slider);
                      */

  /* Ensure the new widgets are visible */
  gtk_widget_show_all (spectral_expander_box);


  /* Expand layer options */
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  scale = gimp_prop_spin_scale_new (config, "expand-amount",
                                    1, 10, 2);
  gimp_spin_scale_set_constrain_drag (GIMP_SPIN_SCALE (scale), TRUE);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 1000.0);
  gimp_spin_scale_set_gamma (GIMP_SPIN_SCALE (scale), 1.0);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  combo_box = gimp_prop_enum_combo_box_new (config, "expand-fill-type", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), combo_box, FALSE, FALSE, 0);

  frame = gimp_prop_enum_radio_frame_new (config, "expand-mask-fill-type",
                                          _("Fill Layer Mask With"), 0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);

  frame = gimp_prop_expanding_frame_new (config, "expand-use", NULL,
                                         vbox2, NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* ensure all widgets are visible */
  gtk_widget_show_all (vbox);

  return vbox;
}
