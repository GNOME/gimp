/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color-profile-import-dialog.h
 * Copyright (C) 2015 Michael Natterer <mitch@gimp.org>
 *
 * Partly based on the lcms plug-in
 * Copyright (C) 2006, 2007  Sven Neumann <sven@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-color-profile.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-constructors.h"

#include "color-profile-import-dialog.h"

#include "gimp-intl.h"


/*  public functions  */

GimpColorProfilePolicy
color_profile_import_dialog_run (GimpImage         *image,
                                 GimpContext       *context,
                                 GtkWidget         *parent,
                                 GimpColorProfile **dest_profile,
                                 gboolean          *dont_ask)
{
  GtkWidget              *dialog;
  GtkWidget              *main_vbox;
  GtkWidget              *frame;
  GtkWidget              *label;
  GtkWidget              *toggle;
  GimpColorProfile       *src_profile;
  GimpColorProfilePolicy  policy;
  const gchar            *title;
  const gchar            *frame_title;
  gchar                  *text;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), GIMP_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), GIMP_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent),
                        GIMP_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (dest_profile != NULL, GIMP_COLOR_PROFILE_POLICY_KEEP);

  src_profile   = gimp_image_get_color_profile (image);
  *dest_profile = gimp_image_get_builtin_color_profile (image);

  if (gimp_image_get_base_type (image) == GIMP_GRAY)
    {
      title       = _("Convert to Grayscale Working Space?");
      frame_title = _("Convert the image to the grayscale working space?");
    }
  else
    {
      title       = _("Convert to RGB Working Space?");
      frame_title = _("Convert the image to the RGB working space?");
    }

  dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                              title,
                              "gimp-image-color-profile-import",
                              NULL,
                              _("Import the image from a color profile"),
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_IMAGE_COLOR_PROFILE_IMPORT,

                              _("Keep"),    GTK_RESPONSE_CANCEL,
                              _("Convert"), GTK_RESPONSE_OK,

                              NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  text = g_strdup_printf (_("The image '%s' has an embedded color profile"),
                          gimp_image_get_display_name (image));
  frame = gimp_frame_new (text);
  g_free (text);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  label = gimp_color_profile_label_new (src_profile);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);

  frame = gimp_frame_new (frame_title);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  label = gimp_color_profile_label_new (*dest_profile);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);

  if (dont_ask)
    {
      toggle = gtk_check_button_new_with_mnemonic (_("_Don't ask me again"));
      gtk_box_pack_end (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), FALSE);
      gtk_widget_show (toggle);
    }

  switch (gtk_dialog_run (GTK_DIALOG (dialog)))
    {
    case GTK_RESPONSE_OK:
      policy = GIMP_COLOR_PROFILE_POLICY_CONVERT;
      g_object_ref (*dest_profile);
      break;

    default:
      policy = GIMP_COLOR_PROFILE_POLICY_KEEP;
      break;
    }

  if (dont_ask)
    {
      *dont_ask = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle));
    }

  gtk_widget_destroy (dialog);

  return policy;
}
