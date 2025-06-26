/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpexportproceduredialog.c
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


struct _GimpExportProcedureDialog
{
  GimpProcedureDialog  parent_instance;

  GList               *additional_metadata;
  GimpImage           *image;

  GThread             *metadata_thread;
  gboolean             metadata_thread_running;
  GMutex               metadata_thread_mutex;
};


static void   gimp_export_procedure_dialog_finalize  (GObject             *object);

static void   gimp_export_procedure_dialog_fill_end  (GimpProcedureDialog *dialog,
                                                      GimpProcedure       *procedure,
                                                      GimpProcedureConfig *config);
static void   gimp_export_procedure_dialog_fill_list (GimpProcedureDialog *dialog,
                                                      GimpProcedure       *procedure,
                                                      GimpProcedureConfig *config,
                                                      GList               *properties);

static void   gimp_export_procedure_dialog_notify_comment
                                                     (GimpProcedureConfig *config,
                                                      GParamSpec          *pspec,
                                                      GtkWidget           *widget);

static gpointer gimp_export_procedure_dialog_edit_metadata_thread   (gpointer                 data);
static gboolean gimp_export_procedure_dialog_activate_edit_metadata (GtkLinkButton           *link,
                                                                     GimpExportProcedureDialog *dialog);


G_DEFINE_TYPE (GimpExportProcedureDialog, gimp_export_procedure_dialog, GIMP_TYPE_PROCEDURE_DIALOG)

#define parent_class gimp_export_procedure_dialog_parent_class

static void
gimp_export_procedure_dialog_class_init (GimpExportProcedureDialogClass *klass)
{
  GObjectClass             *object_class;
  GimpProcedureDialogClass *proc_dialog_class;

  object_class      = G_OBJECT_CLASS (klass);
  proc_dialog_class = GIMP_PROCEDURE_DIALOG_CLASS (klass);

  object_class->finalize       = gimp_export_procedure_dialog_finalize;
  proc_dialog_class->fill_end  = gimp_export_procedure_dialog_fill_end;
  proc_dialog_class->fill_list = gimp_export_procedure_dialog_fill_list;
}

static void
gimp_export_procedure_dialog_init (GimpExportProcedureDialog *dialog)
{
  dialog->additional_metadata     = NULL;
  dialog->image                   = NULL;
  dialog->metadata_thread         = NULL;
  dialog->metadata_thread_running = FALSE;
  g_mutex_init (&dialog->metadata_thread_mutex);
}

static void
gimp_export_procedure_dialog_finalize (GObject *object)
{
  GimpExportProcedureDialog *dialog = GIMP_EXPORT_PROCEDURE_DIALOG (object);

  g_list_free_full (dialog->additional_metadata, g_free);
  g_mutex_lock (&dialog->metadata_thread_mutex);
  if (dialog->metadata_thread_running)
    {
      g_mutex_unlock (&dialog->metadata_thread_mutex);
      g_thread_join (dialog->metadata_thread);
      g_mutex_lock (&dialog->metadata_thread_mutex);
    }
  g_mutex_unlock (&dialog->metadata_thread_mutex);
  g_mutex_clear (&dialog->metadata_thread_mutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_export_procedure_dialog_fill_end (GimpProcedureDialog *dialog,
                                       GimpProcedure       *procedure,
                                       GimpProcedureConfig *config)
{
  GimpExportProcedureDialog *export_dialog;
  GimpExportProcedure       *export_procedure;
  GtkWidget                 *content_area;

  GIMP_PROCEDURE_DIALOG_CLASS (parent_class)->fill_end (dialog, procedure, config);

  export_dialog    = GIMP_EXPORT_PROCEDURE_DIALOG (dialog);
  export_procedure = GIMP_EXPORT_PROCEDURE (procedure);
  content_area     = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  if (gimp_export_procedure_get_support_exif      (export_procedure) ||
      gimp_export_procedure_get_support_iptc      (export_procedure) ||
      gimp_export_procedure_get_support_xmp       (export_procedure) ||
      gimp_export_procedure_get_support_profile   (export_procedure) ||
      gimp_export_procedure_get_support_thumbnail (export_procedure) ||
      g_list_length (export_dialog->additional_metadata) > 0         ||
      gimp_export_procedure_get_support_comment   (export_procedure))
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
                        G_CALLBACK (gimp_export_procedure_dialog_activate_edit_metadata),
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
      n_metadata = gimp_export_procedure_get_support_exif (export_procedure) +
                   gimp_export_procedure_get_support_iptc (export_procedure) +
                   gimp_export_procedure_get_support_xmp  (export_procedure);
      n_metadata = MAX (n_metadata,
                        gimp_export_procedure_get_support_profile (export_procedure) +
                        gimp_export_procedure_get_support_thumbnail (export_procedure));

      if (gimp_export_procedure_get_support_exif (export_procedure))
        {
          widget = gimp_prop_check_button_new (G_OBJECT (config),
                                               "include-exif", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, 0, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          top   = 1;
          gtk_widget_show (widget);
        }
      if (gimp_export_procedure_get_support_iptc (export_procedure))
        {
          widget = gimp_prop_check_button_new (G_OBJECT (config),
                                               "include-iptc", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, 0, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          top   = 1;
          gtk_widget_show (widget);
        }
      if (gimp_export_procedure_get_support_xmp (export_procedure))
        {
          widget = gimp_prop_check_button_new (G_OBJECT (config),
                                               "include-xmp", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, 0, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          top   = 1;
          gtk_widget_show (widget);
        }

      /* Line for specific metadata: profile, thumbnail. */
      left = 0;

      if (gimp_export_procedure_get_support_profile (export_procedure))
        {
          widget = gimp_prop_check_button_new (G_OBJECT (config),
                                               "include-color-profile", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, top, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          gtk_widget_show (widget);
        }
      if (gimp_export_procedure_get_support_thumbnail (export_procedure))
        {
          widget = gimp_prop_check_button_new (G_OBJECT (config),
                                               "include-thumbnail", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, top, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          gtk_widget_show (widget);
        }
      if (n_metadata > 0)
        top++;

      /* Custom metadata: n_metadata items per line. */
      left = 0;
      for (GList *iter = export_dialog->additional_metadata; iter; iter = iter->next)
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
      if (gimp_export_procedure_get_support_comment (export_procedure))
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
                                               "include-comment", NULL);
          gtk_frame_set_label_widget (GTK_FRAME (frame2), title);
          gtk_widget_show (title);

          buffer = gimp_prop_text_buffer_new (G_OBJECT (config),
                                              "gimp-comment", -1);
          widget = gtk_text_view_new_with_buffer (buffer);
          gtk_text_view_set_top_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_bottom_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_left_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_right_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD);
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

          /* Comment field should only be editable when "Save Comment" is
           * sensitive. */
          g_signal_connect_object (config, "notify::include-comment",
                                   G_CALLBACK (gimp_export_procedure_dialog_notify_comment),
                                   widget, 0);
          gimp_export_procedure_dialog_notify_comment (config, NULL, widget);

          gtk_grid_attach (GTK_GRID (grid), frame2, 0, top, 6, 1);
          gtk_widget_show (frame2);
        }

      gtk_box_pack_start (GTK_BOX (content_area), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);
    }
}

static void
gimp_export_procedure_dialog_fill_list (GimpProcedureDialog *dialog,
                                        GimpProcedure       *procedure,
                                        GimpProcedureConfig *config,
                                        GList               *properties)
{
  GimpExportProcedureDialog *export_dialog;
  GimpExportProcedure       *export_procedure;
  GList                     *properties2 = NULL;
  GList                     *iter;

  export_dialog    = GIMP_EXPORT_PROCEDURE_DIALOG (dialog);
  export_procedure = GIMP_EXPORT_PROCEDURE (procedure);

  for (iter = properties; iter; iter = iter->next)
    {
      gchar *propname = iter->data;

      if ((gimp_export_procedure_get_support_exif (export_procedure) &&
           g_strcmp0 (propname, "include-exif") == 0)                 ||
          (gimp_export_procedure_get_support_iptc (export_procedure) &&
           g_strcmp0 (propname, "include-iptc") == 0)                 ||
          (gimp_export_procedure_get_support_xmp (export_procedure) &&
           g_strcmp0 (propname, "include-xmp") == 0)                  ||
          (gimp_export_procedure_get_support_profile (export_procedure) &&
           g_strcmp0 (propname, "include-color-profile") == 0)        ||
          (gimp_export_procedure_get_support_thumbnail (export_procedure) &&
           g_strcmp0 (propname, "include-thumbnail") == 0)            ||
          (gimp_export_procedure_get_support_comment (export_procedure) &&
           (g_strcmp0 (propname, "include-comment") == 0 ||
            g_strcmp0 (propname, "gimp-comment") == 0))            ||
          g_list_find (export_dialog->additional_metadata, propname))
        /* Ignoring the standards and custom metadata. */
        continue;

      properties2 = g_list_prepend (properties2, propname);
    }
  properties2 = g_list_reverse (properties2);
  GIMP_PROCEDURE_DIALOG_CLASS (parent_class)->fill_list (dialog, procedure, config, properties2);
  g_list_free (properties2);
}

static void
gimp_export_procedure_dialog_notify_comment (GimpProcedureConfig *config,
                                             GParamSpec          *pspec,
                                             GtkWidget           *widget)
{
  gboolean sensitive;

  g_object_get (config, "include-comment", &sensitive, NULL);

  gtk_widget_set_sensitive (widget, sensitive);
}

static gpointer
gimp_export_procedure_dialog_edit_metadata_thread (gpointer data)
{
  GimpExportProcedureDialog *dialog = data;
  GimpProcedure             *procedure;
  GBytes                    *dialog_handle;

  g_mutex_lock (&dialog->metadata_thread_mutex);
  dialog->metadata_thread_running = TRUE;
  g_mutex_unlock (&dialog->metadata_thread_mutex);

  procedure     = gimp_pdb_lookup_procedure (gimp_get_pdb (), "plug-in-metadata-editor");
  dialog_handle = gimp_dialog_get_native_handle (GIMP_DIALOG (dialog));
  gimp_procedure_run (procedure,
                      "run-mode",      GIMP_RUN_INTERACTIVE,
                      "image",         dialog->image,
                      "parent-handle", dialog_handle,
                      NULL);

  g_mutex_lock (&dialog->metadata_thread_mutex);
  dialog->metadata_thread_running = FALSE;
  g_mutex_unlock (&dialog->metadata_thread_mutex);

  return NULL;
}

static gboolean
gimp_export_procedure_dialog_activate_edit_metadata (GtkLinkButton             *link,
                                                     GimpExportProcedureDialog *dialog)
{
  gtk_link_button_set_visited (link, TRUE);

  g_mutex_lock (&dialog->metadata_thread_mutex);

  /* Only run if not already running. */
  if (! dialog->metadata_thread_running)
    {
      if (dialog->metadata_thread)
        g_thread_join (dialog->metadata_thread);

      dialog->metadata_thread = g_thread_try_new ("Edit Metadata",
                                                  gimp_export_procedure_dialog_edit_metadata_thread,
                                                  dialog, NULL);
    }

  g_mutex_unlock (&dialog->metadata_thread_mutex);

  /* Stop propagation as the URI is bogus. */
  return TRUE;
}


/* Public Functions */


GtkWidget *
gimp_export_procedure_dialog_new (GimpExportProcedure *procedure,
                                  GimpProcedureConfig *config,
                                  GimpImage           *image)
{
  GtkWidget   *dialog;
  gchar       *title;
  const gchar *format_name;
  const gchar *help_id;
  gboolean     use_header_bar;

  g_return_val_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure), NULL);
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

  dialog = g_object_new (GIMP_TYPE_EXPORT_PROCEDURE_DIALOG,
                         "procedure",      procedure,
                         "config",         config,
                         "title",          title,
                         "help-func",      gimp_standard_help_func,
                         "help-id",        help_id,
                         "use-header-bar", use_header_bar,
                         NULL);
  GIMP_EXPORT_PROCEDURE_DIALOG (dialog)->image = image;
  g_free (title);

  return dialog;
}

void
gimp_export_procedure_dialog_add_metadata (GimpExportProcedureDialog *dialog,
                                           const gchar               *property)
{
  if (! g_list_find (dialog->additional_metadata, property))
    dialog->additional_metadata = g_list_append (dialog->additional_metadata, g_strdup (property));
}
