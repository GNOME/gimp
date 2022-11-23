/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color-profile-import-dialog.h
 * Copyright (C) 2015 Michael Natterer <mitch@ligma.org>
 *
 * Partly based on the lcms plug-in
 * Copyright (C) 2006, 2007  Sven Neumann <sven@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-color-profile.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaviewabledialog.h"
#include "widgets/ligmawidgets-constructors.h"

#include "color-profile-import-dialog.h"

#include "ligma-intl.h"


/*  public functions  */

LigmaColorProfilePolicy
color_profile_import_dialog_run (LigmaImage                 *image,
                                 LigmaContext               *context,
                                 GtkWidget                 *parent,
                                 LigmaColorProfile         **dest_profile,
                                 LigmaColorRenderingIntent  *intent,
                                 gboolean                  *bpc,
                                 gboolean                  *dont_ask)
{
  GtkWidget              *dialog;
  GtkWidget              *main_vbox;
  GtkWidget              *vbox;
  GtkWidget              *stack;
  GtkWidget              *switcher;
  GtkWidget              *frame;
  GtkWidget              *label;
  GtkWidget              *intent_combo;
  GtkWidget              *bpc_toggle;
  GtkWidget              *dont_ask_toggle;
  LigmaColorProfile       *src_profile;
  LigmaColorProfile       *pref_profile = NULL;
  LigmaColorProfilePolicy  policy;
  const gchar            *frame_title;
  gchar                  *text;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), LIGMA_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), LIGMA_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent),
                        LIGMA_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (dest_profile != NULL, LIGMA_COLOR_PROFILE_POLICY_KEEP);

  src_profile   = ligma_image_get_color_profile (image);
  *dest_profile = ligma_image_get_builtin_color_profile (image);

  if (ligma_image_get_base_type (image) == LIGMA_GRAY)
    {
      frame_title = _("Convert the image to the built-in grayscale color profile?");

      pref_profile = ligma_color_config_get_gray_color_profile (image->ligma->config->color_management, NULL);
      if (pref_profile && ligma_color_profile_is_equal (pref_profile, *dest_profile))
        g_clear_object (&pref_profile);
    }
  else
    {
      frame_title = _("Convert the image to the built-in sRGB color profile?");

      pref_profile = ligma_color_config_get_rgb_color_profile (image->ligma->config->color_management, NULL);
      if (pref_profile && ligma_color_profile_is_equal (pref_profile, *dest_profile))
        g_clear_object (&pref_profile);
    }

  dialog =
    ligma_viewable_dialog_new (g_list_prepend (NULL, image), context,
                              _("Keep the Embedded Working Space?"),
                              "ligma-image-color-profile-import",
                              NULL,
                              _("Keep the image's color profile"),
                              parent,
                              ligma_standard_help_func,
                              LIGMA_HELP_IMAGE_COLOR_PROFILE_IMPORT,

                              _("_Keep"),    GTK_RESPONSE_YES,
                              _("_Convert"), GTK_RESPONSE_NO,

                              NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  text = g_strdup_printf (_("The image '%s' has an embedded color profile"),
                          ligma_image_get_display_name (image));
  frame = ligma_frame_new (text);
  g_free (text);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  label = ligma_color_profile_label_new (src_profile);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);

  switcher = gtk_stack_switcher_new ();

  stack = gtk_stack_new ();
  gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (switcher), GTK_STACK (stack));
  gtk_box_pack_start (GTK_BOX (main_vbox), stack, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), switcher, FALSE, FALSE, 0);
  gtk_widget_show (stack);

  frame = ligma_frame_new (frame_title);
  gtk_stack_add_titled (GTK_STACK (stack), frame, "builtin", "Built-in Profile");
  gtk_widget_show (frame);

  label = ligma_color_profile_label_new (*dest_profile);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);

  if (pref_profile)
    {
      if (ligma_image_get_base_type (image) == LIGMA_GRAY)
        frame_title  = _("Convert the image to the preferred grayscale color profile?");
      else
        frame_title = _("Convert the image to the preferred RGB color profile?");

      frame = ligma_frame_new (frame_title);
      gtk_stack_add_titled (GTK_STACK (stack), frame, "preferred", "Preferred Profile");
      gtk_widget_show (frame);

      label = ligma_color_profile_label_new (pref_profile);
      gtk_container_add (GTK_CONTAINER (frame), label);
      gtk_widget_show (label);

      gtk_widget_show (switcher);
      gtk_stack_set_visible_child_name (GTK_STACK (stack), "preferred");
    }

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

      intent_combo = ligma_enum_combo_box_new (LIGMA_TYPE_COLOR_RENDERING_INTENT);
      ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (intent_combo),
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
      gtk_widget_set_tooltip_text (dont_ask_toggle,
                                   _("Your choice can later be edited in Preferences > Color Management"));
      gtk_box_pack_end (GTK_BOX (main_vbox), dont_ask_toggle, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dont_ask_toggle), FALSE);
      gtk_widget_show (dont_ask_toggle);
    }

  switch (gtk_dialog_run (GTK_DIALOG (dialog)))
    {
    case GTK_RESPONSE_NO:
      if (g_strcmp0 (gtk_stack_get_visible_child_name (GTK_STACK (stack)),
                     "builtin") == 0)
        {
          policy = LIGMA_COLOR_PROFILE_POLICY_CONVERT_BUILTIN;
          g_object_ref (*dest_profile);
        }
      else
        {
          policy = LIGMA_COLOR_PROFILE_POLICY_CONVERT_PREFERRED;
          *dest_profile = g_object_ref (pref_profile);
        }
      break;

    default:
      policy = LIGMA_COLOR_PROFILE_POLICY_KEEP;
      break;
    }

  if (intent)
    ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (intent_combo),
                                   (gint *) intent);

  if (bpc)
    *bpc = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (bpc_toggle));

  if (dont_ask)
    *dont_ask = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dont_ask_toggle));

  gtk_widget_destroy (dialog);
  g_clear_object (&pref_profile);

  return policy;
}
