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
color_profile_import_dialog_run (GimpImage                 *image,
                                 GimpContext               *context,
                                 GtkWidget                 *parent,
                                 GimpColorProfile         **dest_profile,
                                 GimpColorRenderingIntent  *intent,
                                 gboolean                  *bpc,
                                 gboolean                  *dont_ask)
{
  GtkWidget              *dialog;
  GtkWidget              *main_vbox;
  GtkWidget              *vbox;
  GtkWidget              *frame;
  GtkWidget              *label;
  GtkWidget              *intent_combo;
  GtkWidget              *bpc_toggle;
  GtkWidget              *dont_ask_toggle;
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

                              _("_Keep"),    GTK_RESPONSE_CANCEL,
                              _("C_onvert"), GTK_RESPONSE_OK,

                              NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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

  if (intent && bpc)
    {
      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
      gtk_widget_show (vbox);
    }
  else
    {
      vbox = main_vbox;
    }

  if (intent)
    {
      GtkWidget *hbox;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new_with_mnemonic (_("_Rendering Intent:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      intent_combo = gimp_enum_combo_box_new (GIMP_TYPE_COLOR_RENDERING_INTENT);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (intent_combo),
                                     *intent);
      gtk_box_pack_start (GTK_BOX (hbox), intent_combo, TRUE, TRUE, 0);
      gtk_widget_show (intent_combo);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), intent_combo);
    }

  if (bpc)
    {
      bpc_toggle =
        gtk_check_button_new_with_mnemonic (_("_Black Point Compensation"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bpc_toggle), *bpc);
      gtk_box_pack_start (GTK_BOX (vbox), bpc_toggle, FALSE, FALSE, 0);
      gtk_widget_show (bpc_toggle);
    }

  if (dont_ask)
    {
      dont_ask_toggle =
        gtk_check_button_new_with_mnemonic (_("_Don't ask me again"));
      gtk_box_pack_end (GTK_BOX (main_vbox), dont_ask_toggle, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dont_ask_toggle), FALSE);
      gtk_widget_show (dont_ask_toggle);
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

  if (intent)
    gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (intent_combo),
                                   (gint *) intent);

  if (bpc)
    *bpc = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (bpc_toggle));

  if (dont_ask)
    *dont_ask = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dont_ask_toggle));

  gtk_widget_destroy (dialog);

  return policy;
}
