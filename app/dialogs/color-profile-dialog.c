/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color-profile-dialog.h
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

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-constructors.h"
#include "widgets/gimpwidgets-utils.h"

#include "color-profile-dialog.h"

#include "gimp-intl.h"


typedef struct
{
  ColorProfileDialogType    dialog_type;
  GimpImage                *image;
  GimpColorProfile         *current_profile;
  GimpColorProfile         *default_profile;
  GimpColorRenderingIntent  intent;
  gboolean                  bpc;
  GimpColorProfileCallback  callback;
  gpointer                  user_data;

  GimpColorConfig          *config;
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
                          GimpImage                *image,
                          GimpContext              *context,
                          GtkWidget                *parent,
                          GimpColorProfile         *current_profile,
                          GimpColorProfile         *default_profile,
                          GimpColorRenderingIntent  intent,
                          gboolean                  bpc,
                          GimpColorProfileCallback  callback,
                          gpointer                  user_data)
{
  ProfileDialog *private;
  GtkWidget     *dialog;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *expander;
  GtkWidget     *label;
  const gchar   *dest_label;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (current_profile == NULL ||
                        GIMP_IS_COLOR_PROFILE (current_profile), NULL);
  g_return_val_if_fail (default_profile == NULL ||
                        GIMP_IS_COLOR_PROFILE (default_profile), NULL);
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
  private->config          = image->gimp->config->color_management;

  switch (dialog_type)
    {
    case COLOR_PROFILE_DIALOG_ASSIGN_PROFILE:
      dialog =
        gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                                  _("Assign ICC Color Profile"),
                                  "gimp-image-color-profile-assign",
                                  NULL,
                                  _("Assign a color profile to the image"),
                                  parent,
                                  gimp_standard_help_func,
                                  GIMP_HELP_IMAGE_COLOR_PROFILE_ASSIGN,

                                  _("_Cancel"), GTK_RESPONSE_CANCEL,
                                  _("_Assign"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Assign");
      break;

    case COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE:
      dialog =
        gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                                  _("Convert to ICC Color Profile"),
                                  "gimp-image-color-profile-convert",
                                  NULL,
                                  _("Convert the image to a color profile"),
                                  parent,
                                  gimp_standard_help_func,
                                  GIMP_HELP_IMAGE_COLOR_PROFILE_CONVERT,

                                  _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                  _("C_onvert"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Convert to");
      break;

    case COLOR_PROFILE_DIALOG_CONVERT_TO_RGB:
      dialog =
        gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                                  _("RGB Conversion"),
                                  "gimp-image-convert-rgb",
                                  GIMP_ICON_CONVERT_RGB,
                                  _("Convert Image to RGB"),
                                  parent,
                                  gimp_standard_help_func,
                                  GIMP_HELP_IMAGE_CONVERT_RGB,

                                  _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                  _("C_onvert"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Convert to");
      break;

    case COLOR_PROFILE_DIALOG_CONVERT_TO_GRAY:
      dialog =
        gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                                  _("Grayscale Conversion"),
                                  "gimp-image-convert-gray",
                                  GIMP_ICON_CONVERT_GRAYSCALE,
                                  _("Convert Image to Grayscale"),
                                  parent,
                                  gimp_standard_help_func,
                                  GIMP_HELP_IMAGE_CONVERT_GRAYSCALE,

                                  _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                  _("C_onvert"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Convert to");
      break;

    case COLOR_PROFILE_DIALOG_SELECT_SOFTPROOF_PROFILE:
      dialog =
        gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                                  _("Soft-Proof Profile"),
                                  "gimp-select-softproof-profile",
                                  GIMP_ICON_DOCUMENT_PRINT,
                                  _("Select Soft-Proof Profile"),
                                  parent,
                                  gimp_standard_help_func,
                                  GIMP_HELP_VIEW_COLOR_MANAGEMENT,

                                  _("_Cancel"), GTK_RESPONSE_CANCEL,
                                  _("_Select"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("New Color Profile");
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  private->dialog = dialog;

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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

  frame = gimp_frame_new (_("Current Color Profile"));
  gtk_box_pack_start (GTK_BOX (private->main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  label = gimp_color_profile_label_new (private->current_profile);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);

  frame = gimp_frame_new (dest_label);
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

  private->dest_view = gimp_color_profile_view_new ();
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

      combo = gimp_enum_combo_box_new (GIMP_TYPE_COLOR_RENDERING_INTENT);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  private->intent,
                                  G_CALLBACK (gimp_int_combo_box_get_active),
                                  &private->intent);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      toggle =
        gtk_check_button_new_with_mnemonic (_("_Black Point Compensation"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), private->bpc);
      gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
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
  gchar        *history;

  history = gimp_personal_rc_file ("profilerc");
  store = gimp_color_profile_store_new (history);
  g_free (history);

  if (private->default_profile)
    {
      GimpImageBaseType  base_type;
      GimpPrecision      precision;
      GError            *error = NULL;

      switch (private->dialog_type)
        {
        case COLOR_PROFILE_DIALOG_ASSIGN_PROFILE:
        case COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE:
          base_type = gimp_image_get_base_type (private->image);
          break;

        case COLOR_PROFILE_DIALOG_CONVERT_TO_RGB:
          base_type = GIMP_RGB;
          break;

        case COLOR_PROFILE_DIALOG_CONVERT_TO_GRAY:
          base_type = GIMP_GRAY;
          break;

        default:
          g_return_val_if_reached (NULL);
        }

      precision = gimp_image_get_precision (private->image);

      if (! gimp_color_profile_store_add_defaults (GIMP_COLOR_PROFILE_STORE (store),
                                                   private->config,
                                                   base_type,
                                                   precision,
                                                   &error))
        {
          gimp_message (private->image->gimp, G_OBJECT (private->dialog),
                        GIMP_MESSAGE_ERROR,
                        "%s", error->message);
          g_clear_error (&error);
        }
    }
  else
    {
      gimp_color_profile_store_add_file (GIMP_COLOR_PROFILE_STORE (store),
                                         NULL, NULL);
    }

  chooser =
    gimp_color_profile_chooser_dialog_new (_("Select Destination Profile"),
                                           NULL,
                                           GTK_FILE_CHOOSER_ACTION_OPEN);

  gimp_color_profile_chooser_dialog_connect_path (chooser,
                                                  G_OBJECT (private->image->gimp->config),
                                                  "color-profile-path");

  combo = gimp_color_profile_combo_box_new_with_model (chooser,
                                                       GTK_TREE_MODEL (store));
  g_object_unref (store);

  gimp_color_profile_combo_box_set_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
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
      GimpColorProfile *profile = NULL;
      GFile            *file;

      file = gimp_color_profile_combo_box_get_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (private->combo));

      if (file)
        {
          GError *error = NULL;

          profile = gimp_color_profile_new_from_file (file, &error);
          g_object_unref (file);

          if (! profile)
            {
              gimp_message (private->image->gimp, G_OBJECT (dialog),
                            GIMP_MESSAGE_ERROR,
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
  GimpColorProfile *dest_profile = NULL;
  GFile            *file;

  file = gimp_color_profile_combo_box_get_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo));

  if (file)
    {
      GError *error = NULL;

      dest_profile = gimp_color_profile_new_from_file (file, &error);
      g_object_unref (file);

      if (! dest_profile)
        {
          gimp_color_profile_view_set_error (GIMP_COLOR_PROFILE_VIEW (private->dest_view),
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
      gimp_color_profile_view_set_error (GIMP_COLOR_PROFILE_VIEW (private->dest_view),
                                         C_("profile", "None"));
    }

  if (dest_profile)
    {
      gimp_color_profile_view_set_profile (GIMP_COLOR_PROFILE_VIEW (private->dest_view),
                                           dest_profile);
      g_object_unref (dest_profile);
    }
}
