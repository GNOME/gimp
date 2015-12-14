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
#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-color-profile.h"
#include "core/gimpimage-undo.h"
#include "core/gimpprogress.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-constructors.h"

#include "color-profile-dialog.h"

#include "gimp-intl.h"


typedef struct
{
  GtkWidget                *dialog;
  GtkWidget                *main_vbox;
  GtkWidget                *combo;
  GtkWidget                *dest_view;

  GimpImage                *image;
  GimpProgress             *progress;
  GimpColorConfig          *config;
  gboolean                  convert;

  GimpColorRenderingIntent  intent;
  gboolean                  bpc;
} ProfileDialog;


static ProfileDialog * color_profile_dialog_new      (GimpImage     *image,
                                                      GimpContext   *context,
                                                      GtkWidget     *parent,
                                                      GimpProgress  *progress,
                                                      gboolean       convert);
static GtkWidget     * color_profile_combo_box_new   (ProfileDialog *dialog);
static void            color_profile_dialog_response (GtkWidget     *widget,
                                                      gint           response_id,
                                                      ProfileDialog *dialog);
static void            color_profile_dest_changed    (GtkWidget     *combo,
                                                      ProfileDialog *dialog);
static void            color_profile_dialog_free     (ProfileDialog *dialog);


/*  defaults  */

static GimpColorRenderingIntent saved_intent = -1;
static gboolean                 saved_bpc    = FALSE;


/*  public functions  */

GtkWidget *
color_profile_assign_dialog_new (GimpImage    *image,
                                 GimpContext  *context,
                                 GtkWidget    *parent,
                                 GimpProgress *progress)
{
  ProfileDialog *dialog;

  dialog = color_profile_dialog_new (image, context, parent, progress, FALSE);

  return dialog ? dialog->dialog : NULL;
}

GtkWidget *
color_profile_convert_dialog_new (GimpImage    *image,
                                  GimpContext  *context,
                                  GtkWidget    *parent,
                                  GimpProgress *progress)
{
  ProfileDialog *dialog;

  dialog = color_profile_dialog_new (image, context, parent, progress, TRUE);

  return dialog ? dialog->dialog : NULL;
}


/*  private functions  */

static ProfileDialog *
color_profile_dialog_new (GimpImage    *image,
                          GimpContext  *context,
                          GtkWidget    *parent,
                          GimpProgress *progress,
                          gboolean      convert)
{
  ProfileDialog    *dialog;
  GtkWidget        *frame;
  GtkWidget        *vbox;
  GtkWidget        *expander;
  GtkWidget        *label;
  GimpColorProfile *src_profile;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  dialog = g_slice_new0 (ProfileDialog);

  dialog->image    = image;
  dialog->progress = progress;
  dialog->config   = image->gimp->config->color_management;
  dialog->convert  = convert;

  if (saved_intent == -1)
    {
      dialog->intent = dialog->config->display_intent;
      dialog->bpc    = (dialog->intent == GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC);
    }
  else
    {
      dialog->intent = saved_intent;
      dialog->bpc    = saved_bpc;
    }

  if (convert)
    {
      dialog->dialog =
        gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                                  _("Convert to ICC Color Profile"),
                                  "gimp-image-color-profile-convert",
                                  NULL,
                                  _("Convert the image to a color profile"),
                                  parent,
                                  gimp_standard_help_func,
                                  GIMP_HELP_IMAGE_COLOR_PROFILE_CONVERT,

                                  GTK_STOCK_CANCEL,  GTK_RESPONSE_CANCEL,
                                  GTK_STOCK_CONVERT, GTK_RESPONSE_OK,

                                  NULL);
    }
  else
    {
      dialog->dialog =
        gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                                  _("Assign ICC Color Profile"),
                                  "gimp-image-color-profile-assign",
                                  NULL,
                                  _("Assign a color profile to the image"),
                                  parent,
                                  gimp_standard_help_func,
                                  GIMP_HELP_IMAGE_COLOR_PROFILE_ASSIGN,

                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                  _("_Assign"),     GTK_RESPONSE_OK,

                                  NULL);
    }

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog->dialog),
                     (GWeakNotify) color_profile_dialog_free, dialog);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (color_profile_dialog_response),
                    dialog);

  dialog->main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (dialog->main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog->dialog))),
                      dialog->main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (dialog->main_vbox);

  frame = gimp_frame_new (_("Current Color Profile"));
  gtk_box_pack_start (GTK_BOX (dialog->main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  src_profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));

  label = gimp_color_profile_label_new (src_profile);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);

  frame = gimp_frame_new (convert ? _("Convert to") : _("Assign"));
  gtk_box_pack_start (GTK_BOX (dialog->main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  dialog->combo = color_profile_combo_box_new (dialog);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->combo, FALSE, FALSE, 0);
  gtk_widget_show (dialog->combo);

  expander = gtk_expander_new_with_mnemonic (_("Profile _details"));
  gtk_box_pack_start (GTK_BOX (vbox), expander, FALSE, FALSE, 0);
  gtk_widget_show (expander);

  dialog->dest_view = gimp_color_profile_view_new ();
  gtk_container_add (GTK_CONTAINER (expander), dialog->dest_view);
  gtk_widget_show (dialog->dest_view);

  g_signal_connect (dialog->combo, "changed",
                    G_CALLBACK (color_profile_dest_changed),
                    dialog);

  color_profile_dest_changed (dialog->combo, dialog);

  if (convert)
    {
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *combo;
      GtkWidget *toggle;

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_box_pack_start (GTK_BOX (dialog->main_vbox), vbox, FALSE, FALSE, 0);
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
                                  dialog->intent,
                                  G_CALLBACK (gimp_int_combo_box_get_active),
                                  &dialog->intent);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      toggle =
        gtk_check_button_new_with_mnemonic (_("_Black Point Compensation"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), dialog->bpc);
      gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &dialog->bpc);
    }

  return dialog;
}

static GtkWidget *
color_profile_combo_box_new (ProfileDialog *dialog)
{
  GtkWidget        *combo;
  GtkWidget        *chooser;
  gchar            *history;
  GimpColorProfile *profile;
  gchar            *label;
  GError           *error = NULL;

  chooser = gimp_color_profile_chooser_dialog_new (_("Select destination profile"));

  history = gimp_personal_rc_file ("profilerc");
  combo = gimp_color_profile_combo_box_new (chooser, history);
  g_free (history);

  profile = gimp_image_get_builtin_color_profile (dialog->image);

  if (gimp_image_get_base_type (dialog->image) == GIMP_GRAY)
    {
      label = g_strdup_printf (_("Built-in GRAY (%s)"),
                               gimp_color_profile_get_label (profile));

      profile = gimp_color_config_get_gray_color_profile (dialog->config, &error);
    }
  else
    {
      label = g_strdup_printf (_("Built-in RGB (%s)"),
                               gimp_color_profile_get_label (profile));

      profile = gimp_color_config_get_rgb_color_profile (dialog->config, &error);
    }

  gimp_color_profile_combo_box_add_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                         NULL, label);
  g_free (label);

  if (profile)
    {
      GFile *file = g_file_new_for_path (dialog->config->rgb_profile);

      if (gimp_image_get_base_type (dialog->image) == GIMP_GRAY)
        {
          label = g_strdup_printf (_("Preferred GRAY (%s)"),
                                   gimp_color_profile_get_label (profile));
        }
      else
        {
          label = g_strdup_printf (_("Preferred RGB (%s)"),
                                   gimp_color_profile_get_label (profile));
        }

      g_object_unref (profile);

      gimp_color_profile_combo_box_add_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                             file, label);
      g_object_unref (file);
      g_free (label);
    }
  else if (error)
    {
      gimp_message (dialog->image->gimp, G_OBJECT (dialog->dialog),
                    GIMP_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);
    }

  gimp_color_profile_combo_box_set_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                                NULL, NULL);

  return combo;
}

static void
color_profile_dialog_response (GtkWidget     *widget,
                               gint           response_id,
                               ProfileDialog *dialog)
{
  gboolean  success = TRUE;
  GError   *error   = NULL;

  if (response_id == GTK_RESPONSE_OK)
    {
      GimpColorProfile *dest_profile = NULL;
      GFile            *file;

      file = gimp_color_profile_combo_box_get_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (dialog->combo));

      if (file)
        {
          dest_profile = gimp_color_profile_new_from_file (file, &error);

          if (! dest_profile)
            success = FALSE;

          g_object_unref (file);
        }
      else
        {
          dest_profile = gimp_image_get_builtin_color_profile (dialog->image);
          g_object_ref (dest_profile);
        }

      if (success)
        {
          if (dialog->convert)
            {
              GimpProgress *progress;
              const gchar  *label;

              label = gimp_color_profile_get_label (dest_profile);

              progress = gimp_progress_start (dialog->progress, FALSE,
                                              _("Converting to '%s'"), label);

              success = gimp_image_convert_color_profile (dialog->image,
                                                          dest_profile,
                                                          dialog->intent,
                                                          dialog->bpc,
                                                          progress,
                                                          &error);

              if (progress)
                gimp_progress_end (progress);

              if (success)
                {
                  saved_intent = dialog->intent;
                  saved_bpc    = dialog->bpc;
                }
            }
          else
            {
              gimp_image_undo_group_start (dialog->image,
                                           GIMP_UNDO_GROUP_PARASITE_ATTACH,
                                           _("Assign color profile"));

              success = gimp_image_set_color_profile (dialog->image,
                                                      dest_profile,
                                                      &error);

              /*  omg...  */
              if (success)
                gimp_image_parasite_detach (dialog->image, "icc-profile-name");

              gimp_image_undo_group_end (dialog->image);

              if (! success)
                gimp_image_undo (dialog->image);
            }

          if (success)
            gimp_image_flush (dialog->image);

          g_object_unref (dest_profile);
        }
    }

  if (success)
    {
      gtk_widget_destroy (dialog->dialog);
    }
  else
    {
      gimp_message (dialog->image->gimp, G_OBJECT (dialog->dialog),
                    GIMP_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);
    }
}

static void
color_profile_dest_changed (GtkWidget     *combo,
                            ProfileDialog *dialog)
{
  GimpColorProfile *dest_profile = NULL;
  GFile            *file;
  GError           *error        = NULL;

  file = gimp_color_profile_combo_box_get_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo));

  if (file)
    {
      dest_profile = gimp_color_profile_new_from_file (file, &error);
      g_object_unref (file);
    }
  else
    {
      dest_profile = gimp_image_get_builtin_color_profile (dialog->image);
      g_object_ref (dest_profile);
    }

  if (! dest_profile)
    {
      gimp_color_profile_view_set_error (GIMP_COLOR_PROFILE_VIEW (dialog->dest_view),
                                         error->message);
      g_clear_error (&error);
    }
  else
    {
      gimp_color_profile_view_set_profile (GIMP_COLOR_PROFILE_VIEW (dialog->dest_view),
                                           dest_profile);
      g_object_unref (dest_profile);
    }
}

static void
color_profile_dialog_free (ProfileDialog *dialog)
{
  g_slice_free (ProfileDialog, dialog);
}
