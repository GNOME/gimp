/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpColorProfileChooserDialog
 * Copyright (C) 2006-2014 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorprofilechooserdialog.h"
#include "gimpcolorprofileview.h"

#include "libgimp/libgimp-intl.h"


struct _GimpColorProfileChooserDialogPrivate
{
  GimpColorProfileView *profile_view;
};


static void   gimp_color_profile_chooser_dialog_constructed    (GObject                       *object);
static void   gimp_color_profile_chooser_dialog_add_shortcut   (GimpColorProfileChooserDialog *dialog);
static void   gimp_color_profile_chooser_dialog_update_preview (GimpColorProfileChooserDialog *dialog);


G_DEFINE_TYPE (GimpColorProfileChooserDialog, gimp_color_profile_chooser_dialog,
               GTK_TYPE_FILE_CHOOSER_DIALOG);

#define parent_class gimp_color_profile_chooser_dialog_parent_class


static void
gimp_color_profile_chooser_dialog_class_init (GimpColorProfileChooserDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = gimp_color_profile_chooser_dialog_constructed;

  g_type_class_add_private (klass, sizeof (GimpColorProfileChooserDialogPrivate));
}

static void
gimp_color_profile_chooser_dialog_init (GimpColorProfileChooserDialog *dialog)
{
  dialog->priv =
    G_TYPE_INSTANCE_GET_PRIVATE (dialog,
                                 GIMP_TYPE_COLOR_PROFILE_CHOOSER_DIALOG,
                                 GimpColorProfileChooserDialogPrivate);
}

static void
gimp_color_profile_chooser_dialog_constructed (GObject *object)
{
  GimpColorProfileChooserDialog *dialog;
  GtkFileFilter                 *filter;
  GtkWidget                     *scrolled_window;
  GtkWidget                     *profile_view;

  dialog  = GIMP_COLOR_PROFILE_CHOOSER_DIALOG (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_window_set_role (GTK_WINDOW (dialog), "gimp-profile-chooser-dialog");

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT,
                          NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  gimp_color_profile_chooser_dialog_add_shortcut (dialog);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All files (*.*)"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("ICC color profile (*.icc, *.icm)"));
  gtk_file_filter_add_pattern (filter, "*.[Ii][Cc][Cc]");
  gtk_file_filter_add_pattern (filter, "*.[Ii][Cc][Mm]");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  /*  the preview widget  */

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_window, 300, -1);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  profile_view = gimp_color_profile_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolled_window), profile_view);
  gtk_widget_show (profile_view);

  dialog->priv->profile_view = GIMP_COLOR_PROFILE_VIEW (profile_view);

  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (dialog),
                                       scrolled_window);

  g_signal_connect (dialog, "update-preview",
                    G_CALLBACK (gimp_color_profile_chooser_dialog_update_preview),
                    NULL);
}

/* Add shortcuts for default ICC profile locations */
static gboolean
add_shortcut (GimpColorProfileChooserDialog *dialog,
              const gchar                   *folder)
{
  return (g_file_test (folder, G_FILE_TEST_IS_DIR) &&
          gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                                folder, NULL));
}

static void
gimp_color_profile_chooser_dialog_add_shortcut (GimpColorProfileChooserDialog *dialog)
{
#ifdef G_OS_WIN32
  {
    const gchar *prefix = g_getenv ("SystemRoot");
    gchar       *folder;

    if (! prefix)
      prefix = "c:\\windows";

    folder = g_strconcat (prefix, "\\system32\\spool\\drivers\\color", NULL);

    add_shortcut (dialog, folder);

    g_free (folder);
  }
#elif defined(PLATFORM_OSX)
  {
    add_shortcut (dialog, "/Library/ColorSync/Profiles");
  }
#else
  {
    add_shortcut (dialog, "/usr/share/color/icc");
  }
#endif
}

static void
gimp_color_profile_chooser_dialog_update_preview (GimpColorProfileChooserDialog *dialog)
{
  GimpColorProfile *profile;
  GFile            *file;
  GError           *error = NULL;

  file = gtk_file_chooser_get_preview_file (GTK_FILE_CHOOSER (dialog));

  if (! file)
    {
      gimp_color_profile_view_set_profile (dialog->priv->profile_view, NULL);
      return;
    }

  switch (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL))
    {
    case G_FILE_TYPE_REGULAR:
      profile = gimp_color_profile_new_from_file (file, &error);

      if (! profile)
        {
          gimp_color_profile_view_set_error (dialog->priv->profile_view,
                                             error->message);
          g_clear_error (&error);
        }
      else
        {
          gimp_color_profile_view_set_profile (dialog->priv->profile_view,
                                               profile);
          g_object_unref (profile);
        }
      break;

    case G_FILE_TYPE_DIRECTORY:
      gimp_color_profile_view_set_error (dialog->priv->profile_view,
                                         _("Folder"));
      break;

    default:
      gimp_color_profile_view_set_error (dialog->priv->profile_view,
                                         _("Not a regular file."));
      break;
    }

  g_object_unref (file);
}
