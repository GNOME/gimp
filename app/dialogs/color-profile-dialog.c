/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color-profile-dialog.h
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
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaviewabledialog.h"
#include "widgets/ligmawidgets-constructors.h"
#include "widgets/ligmawidgets-utils.h"

#include "color-profile-dialog.h"

#include "ligma-intl.h"


typedef struct
{
  ColorProfileDialogType    dialog_type;
  LigmaImage                *image;
  LigmaColorProfile         *current_profile;
  LigmaColorProfile         *default_profile;
  LigmaColorRenderingIntent  intent;
  gboolean                  bpc;
  LigmaColorProfileCallback  callback;
  gpointer                  user_data;

  LigmaColorConfig          *config;
  GtkWidget                *dialog;
  GtkWidget                *main_vbox;
  GtkWidget                *combo;
  GtkWidget                *dest_view;

} ProfileDialog;


static void        color_profile_dialog_free     (ProfileDialog *private);
static GtkWidget * color_profile_combo_box_new   (ProfileDialog *private);
static void        color_profile_dialog_response (GtkWidget     *dialog,
                                                  gint           response_id,
                                                  ProfileDialog *private);
static void        color_profile_dest_changed    (GtkWidget     *combo,
                                                  ProfileDialog *private);


/*  public functions  */

GtkWidget *
color_profile_dialog_new (ColorProfileDialogType    dialog_type,
                          LigmaImage                *image,
                          LigmaContext              *context,
                          GtkWidget                *parent,
                          LigmaColorProfile         *current_profile,
                          LigmaColorProfile         *default_profile,
                          LigmaColorRenderingIntent  intent,
                          gboolean                  bpc,
                          LigmaColorProfileCallback  callback,
                          gpointer                  user_data)
{
  ProfileDialog *private;
  GtkWidget     *dialog;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *expander;
  GtkWidget     *label;
  const gchar   *dest_label;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (current_profile == NULL ||
                        LIGMA_IS_COLOR_PROFILE (current_profile), NULL);
  g_return_val_if_fail (default_profile == NULL ||
                        LIGMA_IS_COLOR_PROFILE (default_profile), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (ProfileDialog);

  private->dialog_type     = dialog_type;
  private->image           = image;
  private->current_profile = current_profile;
  private->default_profile = default_profile;
  private->intent          = intent;
  private->bpc             = bpc;
  private->callback        = callback;
  private->user_data       = user_data;
  private->config          = image->ligma->config->color_management;

  switch (dialog_type)
    {
    case COLOR_PROFILE_DIALOG_ASSIGN_PROFILE:
      dialog =
        ligma_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                  _("Assign ICC Color Profile"),
                                  "ligma-image-color-profile-assign",
                                  NULL,
                                  _("Assign a color profile to the image"),
                                  parent,
                                  ligma_standard_help_func,
                                  LIGMA_HELP_IMAGE_COLOR_PROFILE_ASSIGN,

                                  _("_Cancel"), GTK_RESPONSE_CANCEL,
                                  _("_Assign"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Assign");
      break;

    case COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE:
      dialog =
        ligma_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                  _("Convert to ICC Color Profile"),
                                  "ligma-image-color-profile-convert",
                                  NULL,
                                  _("Convert the image to a color profile"),
                                  parent,
                                  ligma_standard_help_func,
                                  LIGMA_HELP_IMAGE_COLOR_PROFILE_CONVERT,

                                  _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                  _("C_onvert"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Convert to");
      break;

    case COLOR_PROFILE_DIALOG_CONVERT_TO_RGB:
      dialog =
        ligma_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                  _("RGB Conversion"),
                                  "ligma-image-convert-rgb",
                                  LIGMA_ICON_CONVERT_RGB,
                                  _("Convert Image to RGB"),
                                  parent,
                                  ligma_standard_help_func,
                                  LIGMA_HELP_IMAGE_CONVERT_RGB,

                                  _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                  _("C_onvert"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Convert to");
      break;

    case COLOR_PROFILE_DIALOG_CONVERT_TO_GRAY:
      dialog =
        ligma_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                  _("Grayscale Conversion"),
                                  "ligma-image-convert-gray",
                                  LIGMA_ICON_CONVERT_GRAYSCALE,
                                  _("Convert Image to Grayscale"),
                                  parent,
                                  ligma_standard_help_func,
                                  LIGMA_HELP_IMAGE_CONVERT_GRAYSCALE,

                                  _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                  _("C_onvert"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Convert to");
      break;

    case COLOR_PROFILE_DIALOG_SELECT_SOFTPROOF_PROFILE:
      dialog =
        ligma_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                  _("Soft-Proof Profile"),
                                  "ligma-select-softproof-profile",
                                  LIGMA_ICON_DOCUMENT_PRINT,
                                  _("Select Soft-Proof Profile"),
                                  parent,
                                  ligma_standard_help_func,
                                  LIGMA_HELP_VIEW_COLOR_MANAGEMENT,

                                  _("_Cancel"), GTK_RESPONSE_CANCEL,
                                  _("_Select"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("New Color Profile");
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  private->dialog = dialog;

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) color_profile_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (color_profile_dialog_response),
                    private);

  private->main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (private->main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      private->main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (private->main_vbox);

  frame = ligma_frame_new (_("Current Color Profile"));
  gtk_box_pack_start (GTK_BOX (private->main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  label = ligma_color_profile_label_new (private->current_profile);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);

  frame = ligma_frame_new (dest_label);
  gtk_box_pack_start (GTK_BOX (private->main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  private->combo = color_profile_combo_box_new (private);
  gtk_box_pack_start (GTK_BOX (vbox), private->combo, FALSE, FALSE, 0);
  gtk_widget_show (private->combo);

  expander = gtk_expander_new_with_mnemonic (_("Profile _details"));
  gtk_box_pack_start (GTK_BOX (vbox), expander, FALSE, FALSE, 0);
  gtk_widget_show (expander);

  private->dest_view = ligma_color_profile_view_new ();
  gtk_container_add (GTK_CONTAINER (expander), private->dest_view);
  gtk_widget_show (private->dest_view);

  g_signal_connect (private->combo, "changed",
                    G_CALLBACK (color_profile_dest_changed),
                    private);

  color_profile_dest_changed (private->combo, private);

  if (dialog_type == COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE)
    {
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *combo;
      GtkWidget *toggle;

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_box_pack_start (GTK_BOX (private->main_vbox), vbox, FALSE, FALSE, 0);
      gtk_widget_show (vbox);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new_with_mnemonic (_("_Rendering Intent:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      combo = ligma_enum_combo_box_new (LIGMA_TYPE_COLOR_RENDERING_INTENT);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                                  private->intent,
                                  G_CALLBACK (ligma_int_combo_box_get_active),
                                  &private->intent, NULL);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      toggle =
        gtk_check_button_new_with_mnemonic (_("_Black Point Compensation"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), private->bpc);
      gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (ligma_toggle_button_update),
                        &private->bpc);
    }

  return dialog;
}


/*  private functions  */

static void
color_profile_dialog_free (ProfileDialog *private)
{
  g_slice_free (ProfileDialog, private);
}

static GtkWidget *
color_profile_combo_box_new (ProfileDialog *private)
{
  GtkListStore *store;
  GtkWidget    *combo;
  GtkWidget    *chooser;
  GFile        *history;

  history = ligma_directory_file ("profilerc", NULL);
  store = ligma_color_profile_store_new (history);
  g_object_unref (history);

  if (private->default_profile)
    {
      LigmaImageBaseType  base_type;
      LigmaPrecision      precision;
      GError            *error = NULL;

      switch (private->dialog_type)
        {
        case COLOR_PROFILE_DIALOG_ASSIGN_PROFILE:
        case COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE:
          base_type = ligma_image_get_base_type (private->image);
          break;

        case COLOR_PROFILE_DIALOG_CONVERT_TO_RGB:
          base_type = LIGMA_RGB;
          break;

        case COLOR_PROFILE_DIALOG_CONVERT_TO_GRAY:
          base_type = LIGMA_GRAY;
          break;

        default:
          g_return_val_if_reached (NULL);
        }

      precision = ligma_image_get_precision (private->image);

      if (! ligma_color_profile_store_add_defaults (LIGMA_COLOR_PROFILE_STORE (store),
                                                   private->config,
                                                   base_type,
                                                   precision,
                                                   &error))
        {
          ligma_message (private->image->ligma, G_OBJECT (private->dialog),
                        LIGMA_MESSAGE_ERROR,
                        "%s", error->message);
          g_clear_error (&error);
        }
    }
  else
    {
      ligma_color_profile_store_add_file (LIGMA_COLOR_PROFILE_STORE (store),
                                         NULL, NULL);
    }

  chooser =
    ligma_color_profile_chooser_dialog_new (_("Select Destination Profile"),
                                           NULL,
                                           GTK_FILE_CHOOSER_ACTION_OPEN);

  ligma_color_profile_chooser_dialog_connect_path (chooser,
                                                  G_OBJECT (private->image->ligma->config),
                                                  "color-profile-path");

  combo = ligma_color_profile_combo_box_new_with_model (chooser,
                                                       GTK_TREE_MODEL (store));
  g_object_unref (store);

  ligma_color_profile_combo_box_set_active_file (LIGMA_COLOR_PROFILE_COMBO_BOX (combo),
                                                NULL, NULL);

  return combo;
}

static void
color_profile_dialog_response (GtkWidget     *dialog,
                               gint           response_id,
                               ProfileDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      LigmaColorProfile *profile = NULL;
      GFile            *file;

      file = ligma_color_profile_combo_box_get_active_file (LIGMA_COLOR_PROFILE_COMBO_BOX (private->combo));

      if (file)
        {
          GError *error = NULL;

          profile = ligma_color_profile_new_from_file (file, &error);
          g_object_unref (file);

          if (! profile)
            {
              ligma_message (private->image->ligma, G_OBJECT (dialog),
                            LIGMA_MESSAGE_ERROR,
                            "%s", error->message);
              g_clear_error (&error);

              return;
            }
        }
      else if (private->default_profile)
        {
          profile = g_object_ref (private->default_profile);
        }

      private->callback (dialog,
                         private->image,
                         profile,
                         file,
                         private->intent,
                         private->bpc,
                         private->user_data);

      if (profile)
        g_object_unref (profile);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

static void
color_profile_dest_changed (GtkWidget     *combo,
                            ProfileDialog *private)
{
  LigmaColorProfile *dest_profile = NULL;
  GFile            *file;

  file = ligma_color_profile_combo_box_get_active_file (LIGMA_COLOR_PROFILE_COMBO_BOX (combo));

  if (file)
    {
      GError *error = NULL;

      dest_profile = ligma_color_profile_new_from_file (file, &error);
      g_object_unref (file);

      if (! dest_profile)
        {
          ligma_color_profile_view_set_error (LIGMA_COLOR_PROFILE_VIEW (private->dest_view),
                                             error->message);
          g_clear_error (&error);
        }
    }
  else if (private->default_profile)
    {
      dest_profile = g_object_ref (private->default_profile);
    }
  else
    {
      ligma_color_profile_view_set_error (LIGMA_COLOR_PROFILE_VIEW (private->dest_view),
                                         C_("profile", "None"));
    }

  if (dest_profile)
    {
      ligma_color_profile_view_set_profile (LIGMA_COLOR_PROFILE_VIEW (private->dest_view),
                                           dest_profile);
      g_object_unref (dest_profile);
    }
}
