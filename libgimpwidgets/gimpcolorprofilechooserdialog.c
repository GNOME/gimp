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

#include <glib.h>  /* lcms.h uses the "inline" keyword */

#include <lcms2.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorprofilechooserdialog.h"

#include "libgimp/libgimp-intl.h"


enum
{
  PROP_0
};


struct _GimpColorProfileChooserDialogPrivate
{
  GtkTextBuffer *buffer;
  gchar         *filename;
  gchar         *desc;
};


static void   gimp_color_profile_chooser_dialog_constructed    (GObject                  *object);
static void   gimp_color_profile_chooser_dialog_finalize       (GObject                  *object);
static void   gimp_color_profile_chooser_dialog_set_property   (GObject                  *object,
                                                                guint                     prop_id,
                                                                const GValue             *value,
                                                                GParamSpec               *pspec);
static void   gimp_color_profile_chooser_dialog_get_property   (GObject                  *object,
                                                                guint                     prop_id,
                                                                GValue                   *value,
                                                                GParamSpec               *pspec);

static void   gimp_color_profile_chooser_dialog_add_shortcut   (GimpColorProfileChooserDialog *dialog);
static void   gimp_color_profile_chooser_dialog_update_preview (GimpColorProfileChooserDialog *dialog);

static GtkWidget * gimp_color_profile_view_new                 (GtkTextBuffer            *buffer);


G_DEFINE_TYPE (GimpColorProfileChooserDialog, gimp_color_profile_chooser_dialog,
               GTK_TYPE_FILE_CHOOSER_DIALOG);

#define parent_class gimp_color_profile_chooser_dialog_parent_class


static void
gimp_color_profile_chooser_dialog_class_init (GimpColorProfileChooserDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_color_profile_chooser_dialog_constructed;
  object_class->finalize     = gimp_color_profile_chooser_dialog_finalize;
  object_class->get_property = gimp_color_profile_chooser_dialog_get_property;
  object_class->set_property = gimp_color_profile_chooser_dialog_set_property;

  g_type_class_add_private (klass, sizeof (GimpColorProfileChooserDialogPrivate));
}

static void
gimp_color_profile_chooser_dialog_init (GimpColorProfileChooserDialog *dialog)
{
  dialog->private =
    G_TYPE_INSTANCE_GET_PRIVATE (dialog,
                                 GIMP_TYPE_COLOR_PROFILE_CHOOSER_DIALOG,
                                 GimpColorProfileChooserDialogPrivate);

  dialog->private->buffer = gtk_text_buffer_new (NULL);

  gtk_text_buffer_create_tag (dialog->private->buffer, "strong",
                              "weight", PANGO_WEIGHT_BOLD,
                              "scale",  PANGO_SCALE_LARGE,
                              NULL);
  gtk_text_buffer_create_tag (dialog->private->buffer, "emphasis",
                              "style",  PANGO_STYLE_OBLIQUE,
                              NULL);
}

static void
gimp_color_profile_chooser_dialog_constructed (GObject *object)
{
  GimpColorProfileChooserDialog *dialog;
  GtkFileFilter                 *filter;

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

  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (dialog),
                                       gimp_color_profile_view_new (dialog->private->buffer));

  g_signal_connect (dialog, "update-preview",
                    G_CALLBACK (gimp_color_profile_chooser_dialog_update_preview),
                    NULL);
}

static void
gimp_color_profile_chooser_dialog_finalize (GObject *object)
{
  GimpColorProfileChooserDialog *dialog;

  dialog = GIMP_COLOR_PROFILE_CHOOSER_DIALOG (object);

  if (dialog->private->buffer)
    {
      g_object_unref (dialog->private->buffer);
      dialog->private->buffer = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_profile_chooser_dialog_set_property (GObject      *object,
                                                guint         prop_id,
                                                const GValue *value,
                                                GParamSpec   *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_color_profile_chooser_dialog_get_property (GObject    *object,
                                                guint       prop_id,
                                                GValue     *value,
                                                GParamSpec *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

GtkWidget *
gimp_color_profile_chooser_dialog_new (const gchar *title)
{

  return g_object_new (GIMP_TYPE_COLOR_PROFILE_CHOOSER_DIALOG,
                       "title", title,
                       NULL);
}

gchar *
gimp_color_profile_chooser_dialog_get_desc (GimpColorProfileChooserDialog *dialog,
                                            const gchar                   *filename)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE_CHOOSER_DIALOG (dialog), NULL);

  if (filename && dialog->private->filename &&
      strcmp (filename, dialog->private->filename) == 0)
    {
      return g_strdup (dialog->private->desc);
    }

  return NULL;
}

/* Add shortcut for default ICC profile location */
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

    if (g_file_test (folder, G_FILE_TEST_IS_DIR))
      gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                            folder, NULL);

    g_free (folder);
  }
#else
  {
    const gchar folder[] = "/usr/share/color/icc";

    if (g_file_test (folder, G_FILE_TEST_IS_DIR))
      gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                            folder, NULL);
  }
#endif
}

static void
gimp_color_profile_chooser_dialog_update_preview (GimpColorProfileChooserDialog *dialog)
{
  cmsHPROFILE  profile;
  gchar       *filename;
  GtkTextIter  iter;
  gchar       *desc;
  gchar       *model;
  gchar       *summary;

  gtk_text_buffer_set_text (dialog->private->buffer, "", 0);

  g_free (dialog->private->filename);
  dialog->private->filename = NULL;

  g_free (dialog->private->desc);
  dialog->private->desc = NULL;

  filename = gtk_file_chooser_get_preview_filename (GTK_FILE_CHOOSER (dialog));

  gtk_text_buffer_get_start_iter (dialog->private->buffer, &iter);

  if (! g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      gtk_text_buffer_insert_with_tags_by_name (dialog->private->buffer,
                                                &iter,
                                                _("Not a regular file"), -1,
                                                "emphasis", NULL);
      g_free (filename);
      return;
    }

  profile = cmsOpenProfileFromFile (filename, "r");

  if (! profile)
    {
      gtk_text_buffer_insert_with_tags_by_name (dialog->private->buffer,
                                                &iter,
                                                _("Cannot open profile"), -1,
                                                "emphasis", NULL);
      g_free (filename);
      return;
    }

  desc    = gimp_lcms_profile_get_description (profile);
  model   = gimp_lcms_profile_get_model (profile);
  summary = gimp_lcms_profile_get_summary (profile);

  cmsCloseProfile (profile);

  if ((desc  && strlen (desc)) ||
      (model && strlen (model)))
    {
      if (desc && strlen (desc))
        {
          dialog->private->desc = desc;
          desc = NULL;
        }
      else
        {
          dialog->private->desc = model;
          model = NULL;
        }

      gtk_text_buffer_insert_with_tags_by_name (dialog->private->buffer,
                                                &iter,
                                                dialog->private->desc, -1,
                                                "strong", NULL);
      gtk_text_buffer_insert (dialog->private->buffer, &iter, "\n", 1);
    }

  if (summary)
    gtk_text_buffer_insert (dialog->private->buffer, &iter, summary, -1);

  dialog->private->filename = filename;
  filename = NULL;

  g_free (desc);
  g_free (model);
  g_free (summary);
  g_free (filename);
}

static GtkWidget *
gimp_color_profile_view_new (GtkTextBuffer *buffer)
{
  GtkWidget *frame;
  GtkWidget *scrolled_window;
  GtkWidget *text_view;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
  gtk_widget_show (scrolled_window);

  text_view = gtk_text_view_new_with_buffer (buffer);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);

  gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (text_view), 6);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (text_view), 2);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (text_view), 2);

  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  gtk_widget_set_size_request (scrolled_window, 300, -1);

  return frame;
}
