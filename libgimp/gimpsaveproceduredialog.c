/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsaveproceduredialog.c
 * Copyright (C) 2020 Jehan
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"
#include "gimpui.h"

#include "libgimp-intl.h"


struct _GimpSaveProcedureDialogPrivate
{
  GList     *additional_metadata;
  GimpImage *image;

  GThread   *metadata_thread;
  GMutex     metadata_thread_mutex;
};


static void   gimp_save_procedure_dialog_finalize  (GObject             *object);

static void   gimp_save_procedure_dialog_fill_list (GimpProcedureDialog *dialog,
                                                    GimpProcedure       *procedure,
                                                    GimpProcedureConfig *config,
                                                    GList               *properties);

static gpointer gimp_save_procedure_dialog_edit_metadata_thread   (gpointer                 data);
static gboolean gimp_save_procedure_dialog_activate_edit_metadata (GtkLinkButton           *link,
                                                                   GimpSaveProcedureDialog *dialog);


G_DEFINE_TYPE_WITH_PRIVATE (GimpSaveProcedureDialog, gimp_save_procedure_dialog, GIMP_TYPE_PROCEDURE_DIALOG)

#define parent_class gimp_save_procedure_dialog_parent_class

static void
gimp_save_procedure_dialog_class_init (GimpSaveProcedureDialogClass *klass)
{
  GObjectClass             *object_class;
  GimpProcedureDialogClass *proc_dialog_class;

  object_class      = G_OBJECT_CLASS (klass);
  proc_dialog_class = GIMP_PROCEDURE_DIALOG_CLASS (klass);

  object_class->finalize       = gimp_save_procedure_dialog_finalize;
  proc_dialog_class->fill_list = gimp_save_procedure_dialog_fill_list;
}

static void
gimp_save_procedure_dialog_init (GimpSaveProcedureDialog *dialog)
{
  dialog->priv = gimp_save_procedure_dialog_get_instance_private (dialog);

  dialog->priv->additional_metadata = NULL;
  dialog->priv->image               = NULL;
  dialog->priv->metadata_thread     = NULL;
  g_mutex_init (&dialog->priv->metadata_thread_mutex);
}

static void
gimp_save_procedure_dialog_finalize (GObject *object)
{
  GimpSaveProcedureDialog *dialog = GIMP_SAVE_PROCEDURE_DIALOG (object);

  g_list_free_full (dialog->priv->additional_metadata, g_free);
  g_clear_pointer (&dialog->priv->metadata_thread, g_thread_unref);
  g_mutex_clear (&dialog->priv->metadata_thread_mutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_save_procedure_dialog_fill_list (GimpProcedureDialog *dialog,
                                      GimpProcedure       *procedure,
                                      GimpProcedureConfig *config,
                                      GList               *properties)
{
  GimpSaveProcedureDialog *save_dialog;
  GimpSaveProcedure       *save_procedure;
  GtkWidget               *content_area;
  GList                   *properties2 = NULL;
  GList                   *iter;

  save_dialog    = GIMP_SAVE_PROCEDURE_DIALOG (dialog);
  save_procedure = GIMP_SAVE_PROCEDURE (procedure);
  content_area   = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  for (iter = properties; iter; iter = iter->next)
    {
      gchar *propname = iter->data;

      if ((gimp_save_procedure_get_support_exif (save_procedure) &&
           g_strcmp0 (propname, "save-exif") == 0)                 ||
          (gimp_save_procedure_get_support_iptc (save_procedure) &&
           g_strcmp0 (propname, "save-iptc") == 0)                 ||
          (gimp_save_procedure_get_support_xmp (save_procedure) &&
           g_strcmp0 (propname, "save-xmp") == 0)                  ||
          (gimp_save_procedure_get_support_profile (save_procedure) &&
           g_strcmp0 (propname, "save-color-profile") == 0)        ||
          (gimp_save_procedure_get_support_thumbnail (save_procedure) &&
           g_strcmp0 (propname, "save-thumbnail") == 0)            ||
          (gimp_save_procedure_get_support_comment (save_procedure) &&
           (g_strcmp0 (propname, "save-comment") == 0 ||
            g_strcmp0 (propname, "gimp-comment") == 0))            ||
          g_list_find (save_dialog->priv->additional_metadata, propname))
        /* Ignoring the standards and custom metadata. */
        continue;

      properties2 = g_list_prepend (properties2, propname);
    }
  properties2 = g_list_reverse (properties2);
  GIMP_PROCEDURE_DIALOG_CLASS (parent_class)->fill_list (dialog, procedure, config, properties2);
  g_list_free (properties2);


  if (gimp_save_procedure_get_support_exif      (save_procedure) ||
      gimp_save_procedure_get_support_iptc      (save_procedure) ||
      gimp_save_procedure_get_support_xmp       (save_procedure) ||
      gimp_save_procedure_get_support_profile   (save_procedure) ||
      gimp_save_procedure_get_support_thumbnail (save_procedure) ||
      g_list_length (save_dialog->priv->additional_metadata) > 0 ||
      gimp_save_procedure_get_support_comment   (save_procedure))
    {
      GtkWidget      *frame;
      GtkWidget      *frame_title;
      GtkWidget      *grid;
      GtkWidget      *widget;
      GtkWidget      *label;
      GtkWidget      *link;
      PangoAttrList  *attrs;
      PangoAttribute *attr;
      gint            n_metadata;
      gint            left = 0;
      gint            top  = 0;

      frame = gimp_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

      /* Metadata frame title: a label and an edit link. */
      frame_title = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

      label = gtk_label_new (_("Metadata"));
      attrs = pango_attr_list_new ();
      attr  = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
      pango_attr_list_insert (attrs, attr);
      gtk_label_set_attributes (GTK_LABEL (label), attrs);
      pango_attr_list_unref (attrs);
      gtk_box_pack_start (GTK_BOX (frame_title), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      link = gtk_link_button_new_with_label (_("Edit Metadata"), _("(edit)"));
      gtk_link_button_set_visited (GTK_LINK_BUTTON (link), FALSE);
      g_signal_connect (link, "activate-link",
                        G_CALLBACK (gimp_save_procedure_dialog_activate_edit_metadata),
                        dialog);
      gtk_box_pack_start (GTK_BOX (frame_title), link, FALSE, FALSE, 0);
      gtk_widget_show (link);

      gtk_frame_set_label_widget (GTK_FRAME (frame), frame_title);
      gtk_widget_show (frame_title);

      /* Metadata frame contents in a grid.. */
      grid = gtk_grid_new ();
      gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
      gtk_widget_set_vexpand (grid, TRUE);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);

      /* Line for 3 metadata formats: Exif, IPTC, XMP. */
      n_metadata = gimp_save_procedure_get_support_exif (save_procedure) +
                   gimp_save_procedure_get_support_iptc (save_procedure) +
                   gimp_save_procedure_get_support_xmp  (save_procedure);
      n_metadata = MAX (n_metadata,
                        gimp_save_procedure_get_support_profile (save_procedure) +
                        gimp_save_procedure_get_support_thumbnail (save_procedure));

      if (gimp_save_procedure_get_support_exif (save_procedure))
        {
          widget = gimp_prop_check_button_new (G_OBJECT (config),
                                               "save-exif", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, 0, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          top   = 1;
          gtk_widget_show (widget);
        }
      if (gimp_save_procedure_get_support_iptc (save_procedure))
        {
          widget = gimp_prop_check_button_new (G_OBJECT (config),
                                               "save-iptc", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, 0, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          top   = 1;
          gtk_widget_show (widget);
        }
      if (gimp_save_procedure_get_support_xmp (save_procedure))
        {
          widget = gimp_prop_check_button_new (G_OBJECT (config),
                                               "save-xmp", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, 0, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          top   = 1;
          gtk_widget_show (widget);
        }

      /* Line for specific metadata: profile, thumbnail. */
      left = 0;

      if (gimp_save_procedure_get_support_profile (save_procedure))
        {
          widget = gimp_prop_check_button_new (G_OBJECT (config),
                                               "save-color-profile", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, top, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          gtk_widget_show (widget);
        }
      if (gimp_save_procedure_get_support_thumbnail (save_procedure))
        {
          widget = gimp_prop_check_button_new (G_OBJECT (config),
                                               "save-thumbnail", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, top, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          gtk_widget_show (widget);
        }
      if (n_metadata > 0)
        top++;

      /* Custom metadata: n_metadata items per line. */
      left = 0;
      for (iter = save_dialog->priv->additional_metadata; iter; iter = iter->next)
        {
          widget = gimp_procedure_dialog_get_widget (dialog, iter->data, G_TYPE_NONE);
          gtk_grid_attach (GTK_GRID (grid), widget, left, top, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          if (left >= 6)
            {
              top++;
              left = 0;
            }

          gtk_widget_show (widget);
        }
      top++;

      /* Last line for comment field. */
      if (gimp_save_procedure_get_support_comment (save_procedure))
        {
          GtkTextBuffer *buffer;
          const gchar   *tooltip;
          GtkWidget     *frame2;
          GtkWidget     *title;
          GParamSpec    *pspec;
          GtkWidget     *scrolled_window;

          pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                "gimp-comment");

          frame2 = gimp_frame_new (NULL);
          title  = gimp_prop_check_button_new (G_OBJECT (config),
                                               "save-comment", NULL);
          gtk_frame_set_label_widget (GTK_FRAME (frame2), title);
          gtk_widget_show (title);

          buffer = gimp_prop_text_buffer_new (G_OBJECT (config),
                                              "gimp-comment", -1);
          widget = gtk_text_view_new_with_buffer (buffer);
          gtk_text_view_set_top_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_bottom_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_left_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_right_margin (GTK_TEXT_VIEW (widget), 3);
          g_object_unref (buffer);

          tooltip = g_param_spec_get_blurb (pspec);
          if (tooltip)
            gimp_help_set_help_data (widget, tooltip, NULL);

          scrolled_window = gtk_scrolled_window_new (NULL, NULL);
          gtk_widget_set_size_request (scrolled_window, -1, 100);
          gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_OUT);
          gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                          GTK_POLICY_NEVER,
                                          GTK_POLICY_AUTOMATIC);
          gtk_container_add (GTK_CONTAINER (frame2), scrolled_window);
          gtk_widget_show (scrolled_window);
          gtk_widget_set_hexpand (widget, TRUE);
          gtk_widget_set_vexpand (widget, TRUE);
          gtk_container_add (GTK_CONTAINER (scrolled_window), widget);
          gtk_widget_show (widget);

          gtk_grid_attach (GTK_GRID (grid), frame2, 0, top, 6, 1);
          gtk_widget_show (frame2);
        }

      gtk_box_pack_start (GTK_BOX (content_area), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);
    }
}

static gpointer
gimp_save_procedure_dialog_edit_metadata_thread (gpointer data)
{
  GimpSaveProcedureDialog *dialog = data;

  gimp_pdb_run_procedure (gimp_get_pdb (),    "plug-in-metadata-editor",
                          GIMP_TYPE_RUN_MODE, GIMP_RUN_INTERACTIVE,
                          GIMP_TYPE_IMAGE,    dialog->priv->image,
                          G_TYPE_NONE);

  g_mutex_lock (&dialog->priv->metadata_thread_mutex);
  g_thread_unref (dialog->priv->metadata_thread);
  dialog->priv->metadata_thread = NULL;
  g_mutex_unlock (&dialog->priv->metadata_thread_mutex);

  return NULL;
}

static gboolean
gimp_save_procedure_dialog_activate_edit_metadata (GtkLinkButton           *link,
                                                   GimpSaveProcedureDialog *dialog)
{
  gtk_link_button_set_visited (link, TRUE);

  g_mutex_lock (&dialog->priv->metadata_thread_mutex);

  if (! dialog->priv->metadata_thread)
    /* Only run if not already running. */
    dialog->priv->metadata_thread = g_thread_try_new ("Edit Metadata",
                                                      gimp_save_procedure_dialog_edit_metadata_thread,
                                                      dialog, NULL);

  g_mutex_unlock (&dialog->priv->metadata_thread_mutex);

  /* Stop propagation as the URI is bogus. */
  return TRUE;
}


/* Public Functions */


GtkWidget *
gimp_save_procedure_dialog_new (GimpSaveProcedure   *procedure,
                                GimpProcedureConfig *config,
                                GimpImage           *image)
{
  GtkWidget   *dialog;
  gchar       *title;
  const gchar *format_name;
  const gchar *help_id;
  gboolean     use_header_bar;

  g_return_val_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), NULL);
  g_return_val_if_fail (gimp_procedure_config_get_procedure (config) ==
                        GIMP_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  format_name = gimp_file_procedure_get_format_name (GIMP_FILE_PROCEDURE (procedure));
  if (! format_name)
    {
      g_critical ("%s: no format name set on file procedure '%s'. "
                  "Set one with gimp_file_procedure_set_format_name()",
                  G_STRFUNC,
                  gimp_procedure_get_name (GIMP_PROCEDURE (procedure)));
      return NULL;
    }
  /* TRANSLATORS: %s will be a format name, e.g. "PNG" or "JPEG". */
  title = g_strdup_printf (_("Export Image as %s"), format_name);

  help_id = gimp_procedure_get_help_id (GIMP_PROCEDURE (procedure));

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (GIMP_TYPE_SAVE_PROCEDURE_DIALOG,
                         "procedure",      procedure,
                         "config",         config,
                         "title",          title,
                         "help-func",      gimp_standard_help_func,
                         "help-id",        help_id,
                         "use-header-bar", use_header_bar,
                         NULL);
  GIMP_SAVE_PROCEDURE_DIALOG (dialog)->priv->image = image;
  g_free (title);

  return dialog;
}

void
gimp_save_procedure_dialog_add_metadata (GimpSaveProcedureDialog *dialog,
                                         const gchar             *property)
{
  if (! g_list_find (dialog->priv->additional_metadata, property))
    dialog->priv->additional_metadata = g_list_append (dialog->priv->additional_metadata,
                                                       g_strdup (property));
}
