/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpwidgets-constructors.h"

#include "gimp-intl.h"


/*  public functions  */

GtkWidget *
gimp_icon_button_new (const gchar *icon_name,
                      const gchar *label)
{
  GtkWidget *button;
  GtkWidget *image;

  button = gtk_button_new ();

  if (label)
    {
      GtkWidget *hbox;
      GtkWidget *lab;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_container_add (GTK_CONTAINER (button), hbox);
      gtk_widget_show (hbox);

      image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);

      lab = gtk_label_new_with_mnemonic (label);
      gtk_label_set_mnemonic_widget (GTK_LABEL (lab), button);
      gtk_box_pack_start (GTK_BOX (hbox), lab, TRUE, TRUE, 0);
      gtk_widget_show (lab);
    }
  else
    {
      image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);
    }

  return button;
}

GtkWidget *
gimp_color_profile_label_new (GimpColorProfile *profile)
{
  GtkWidget   *expander;
  GtkWidget   *view;
  const gchar *label;

  g_return_val_if_fail (profile == NULL ||
                        GIMP_IS_COLOR_PROFILE (profile), NULL);

  if (profile)
    label = gimp_color_profile_get_label (profile);
  else
    label = C_("profile", "None");

  expander = gtk_expander_new (label);

  view = gimp_color_profile_view_new ();

  if (profile)
    gimp_color_profile_view_set_profile (GIMP_COLOR_PROFILE_VIEW (view),
                                         profile);
  else
    gimp_color_profile_view_set_error (GIMP_COLOR_PROFILE_VIEW (view),
                                       C_("profile", "None"));

  gtk_container_add (GTK_CONTAINER (expander), view);
  gtk_widget_show (view);

  return expander;
}
