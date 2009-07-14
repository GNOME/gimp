/* interface.c - user interface for the metadata editor
 *
 * Copyright (C) 2004-2005, Raphaël Quinet <raphael@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <glib.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "interface.h"
#include "metadata.h"
#include "xmp-schemas.h"
#include "xmp-encode.h"


#define RESPONSE_IMPORT   1
#define RESPONSE_EXPORT   2


typedef struct
{
  GtkWidget    *dlg;
  XMPModel     *xmp_model;
  GdkPixbuf    *edit_icon;
  GdkPixbuf    *auto_icon;
  gboolean      run_ok;
} MetadataGui;

static void
value_edited (GtkCellRendererText *cell,
        const gchar         *path_string,
        const gchar         *new_text,
        gpointer             data)
{
  GtkTreeModel *model = data;
  GtkTreePath  *path  = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter   iter;
  gchar        *old_text;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, COL_XMP_VALUE, &old_text, -1);
  g_free (old_text);

  /* FIXME: update value[] array */
  /* FIXME: check widget xref and update other widget if not NULL */
  gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
          COL_XMP_VALUE, new_text,
          -1);

  gtk_tree_path_free (path);
}

static void
add_view_columns (GtkTreeView *treeview)
{
  gint               col_offset;
  GtkCellRenderer   *renderer;
  GtkTreeViewColumn *column;
  GtkTreeModel      *model = gtk_tree_view_get_model (treeview);

  /* Property Name */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset =
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                                                 -1, _("Property"),
                                                 renderer,
                                                 "text",
                                                 COL_XMP_NAME,
                                                 "weight",
                                                 COL_XMP_WEIGHT,
                                                 "weight-set",
                                                 COL_XMP_WEIGHT_SET,
                                                 NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
  gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);

  /* Icon */
  renderer = gtk_cell_renderer_pixbuf_new ();
  col_offset =
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                                                 -1, "",
                                                 renderer,
                                                 "pixbuf",
                                                 COL_XMP_EDIT_ICON,
                                                 "visible",
                                                 COL_XMP_VISIBLE,
                                                 NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
  gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);

  /* Value */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);

  g_signal_connect (renderer, "edited",
        G_CALLBACK (value_edited), model);
  col_offset =
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                                                 -1, _("Value"),
                                                 renderer,
                                                 "text",
                                                 COL_XMP_VALUE,
                                                 "editable",
                                                 COL_XMP_EDITABLE,
                                                 "visible",
                                                 COL_XMP_VISIBLE,
                                                 NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
  gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);
}

static gboolean
icon_foreach_func (GtkTreeModel *model,
                   GtkTreePath  *path,
                   GtkTreeIter  *iter,
                   gpointer      user_data)
{
  gboolean     editable;
  MetadataGui *mgui = user_data;

  gtk_tree_model_get (model, iter,
                      COL_XMP_EDITABLE, &editable,
                      -1);
  if (editable == XMP_AUTO_UPDATE)
    gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                        COL_XMP_EDIT_ICON, mgui->auto_icon,
                        -1);
  else if (editable == TRUE)
    gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                        COL_XMP_EDIT_ICON, mgui->edit_icon,
                        -1);
  else
    gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                        COL_XMP_EDIT_ICON, NULL,
                        -1);
  return FALSE;
}

static void
update_icons (MetadataGui *mgui)
{
  GtkTreeModel *model;

  /* add the edit icon to the rows that are editable */
  model = xmp_model_get_tree_model (mgui->xmp_model);
  gtk_tree_model_foreach (model, icon_foreach_func, mgui);
}

/* FIXME: temporary data structure for testing */
typedef struct
{
  const gchar *schema;
  const gchar *property_name;
  GSList      *widget_list;
} WidgetXRef;

static void
entry_changed (GtkEntry *entry,
               gpointer  user_data)
{
  WidgetXRef *xref = user_data;

  g_print ("XMP: %s %p %p %s\n", xref->property_name, entry, user_data, gtk_entry_get_text (entry)); /* FIXME */
}

static void
register_entry_xref (GtkWidget   *entry,
                     const gchar *schema,
                     const gchar *property_name)
{
  WidgetXRef *xref;

  xref = g_new (WidgetXRef, 1);
  xref->schema = schema;
  xref->property_name = property_name;
  xref->widget_list = g_slist_prepend (NULL, entry);
  g_signal_connect (GTK_ENTRY (entry), "changed",
        G_CALLBACK (entry_changed), xref);
}

static void
text_changed (GtkTextBuffer *text_buffer,
             gpointer       user_data)
{
  WidgetXRef  *xref = user_data;
  GtkTextIter  start;
  GtkTextIter  end;

  gtk_text_buffer_get_bounds (text_buffer, &start, &end);
  g_print ("XMP: %s %p %p %s\n", xref->property_name, text_buffer, user_data, gtk_text_buffer_get_text (text_buffer, &start, &end, FALSE)); /* FIXME */
}

static void
register_text_xref (GtkTextBuffer *text_buffer,
                    const gchar   *schema,
                    const gchar   *property_name)
{
  WidgetXRef *xref;

  xref = g_new (WidgetXRef, 1);
  xref->schema = schema;
  xref->property_name = property_name;
  xref->widget_list = g_slist_prepend (NULL, text_buffer);
  g_signal_connect (GTK_TEXT_BUFFER (text_buffer), "changed",
        G_CALLBACK (text_changed), xref);
}

static void
add_description_tab (GtkWidget *notebook)
{
  GtkWidget     *frame;
  GtkWidget     *table;
  GtkWidget     *entry;
  GtkWidget     *scrolled_window;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer;

  frame = gimp_frame_new (_("Description"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame,
          gtk_label_new (_("Description")));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
  /* gtk_widget_show (frame); */

  table = gtk_table_new (5, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  /* gtk_widget_show (table); */

  entry = gtk_entry_new ();
  register_entry_xref (entry, XMP_SCHEMA_DUBLIN_CORE, "title");
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
           _("Image _title:"), 0.0, 0.5,
           entry, 1, FALSE);

  entry = gtk_entry_new ();
  register_entry_xref (entry, XMP_SCHEMA_DUBLIN_CORE, "creator");
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
           _("_Author:"), 0.0, 0.5,
           entry, 1, FALSE);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
          GTK_POLICY_AUTOMATIC,
          GTK_POLICY_AUTOMATIC);
  text_view = gtk_text_view_new ();
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  gtk_text_buffer_set_text (text_buffer,
                            "FIXME:\n"
                            "These widgets are currently disconnected from the XMP model.\n"
                            "Please use the Advanced tab.",
                            -1);
  register_text_xref (text_buffer, XMP_SCHEMA_DUBLIN_CORE, "description");
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
           _("_Description:"), 0.0, 0.5,
           scrolled_window, 1, FALSE);

  entry = gtk_entry_new ();
  register_entry_xref (entry, XMP_SCHEMA_PHOTOSHOP, "CaptionWriter");
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
           _("Description _writer:"), 0.0, 0.5,
           entry, 1, FALSE);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
          GTK_POLICY_AUTOMATIC,
          GTK_POLICY_AUTOMATIC);
  text_view = gtk_text_view_new ();
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  register_text_xref (text_buffer, XMP_SCHEMA_PDF, "Keywords");
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
           _("_Keywords:"), 0.0, 0.5,
           scrolled_window, 1, FALSE);

  gtk_widget_show_all (frame);
}

static void
add_copyright_tab (GtkWidget *notebook)
{
  GtkWidget *label;

  /* FIXME: add entries, cross-link with XMP model */
  label = gtk_label_new (_("Empty"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), label,
          gtk_label_new (_("Copyright")));
  gtk_widget_show (label);
}

static void
add_origin_tab (GtkWidget *notebook)
{
  GtkWidget *label;

  /* FIXME: add entries, cross-link with XMP model */
  label = gtk_label_new (_("Empty"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), label,
          gtk_label_new (_("Origin")));
  gtk_widget_show (label);
}

static void
add_camera1_tab (GtkWidget *notebook)
{
  GtkWidget *label;

  /* FIXME: add entries, cross-link with XMP model */
  label = gtk_label_new (_("Empty"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), label,
          gtk_label_new (_("Camera 1")));
  gtk_widget_show (label);
}

static void
add_camera2_tab (GtkWidget *notebook)
{
  GtkWidget *label;

  /* FIXME: add entries, cross-link with XMP model */
  label = gtk_label_new (_("Empty"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), label,
          gtk_label_new (_("Camera 2")));
  gtk_widget_show (label);
}

static void
add_thumbnail_tab (GtkWidget *notebook)
{
  GdkPixbuf *default_thumb;
  GtkWidget *image;

  /* FIXME: link thumbnail with XMP model */
  default_thumb = gtk_widget_render_icon (notebook, GIMP_STOCK_QUESTION,
                                          (GtkIconSize) -1, "thumbnail");
  image = gtk_image_new_from_pixbuf (default_thumb);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), image,
          gtk_label_new (_("Thumbnail")));
  gtk_widget_show (image);
}

static void
add_advanced_tab (GtkWidget    *notebook,
      GtkTreeModel *model)
{
  GtkWidget *sw;
  GtkWidget *treeview;

  /* Advanced tab */
  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
               GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
          GTK_POLICY_AUTOMATIC,
          GTK_POLICY_AUTOMATIC);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sw,
          gtk_label_new (_("Advanced")));

  /* create tree view - model will be unref'ed in xmp_model_free() */
  treeview = gtk_tree_view_new_with_model (model);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);

  add_view_columns (GTK_TREE_VIEW (treeview));

  gtk_container_add (GTK_CONTAINER (sw), treeview);

  /* expand all rows after the treeview widget has been realized */
  g_signal_connect (treeview, "realize",
        G_CALLBACK (gtk_tree_view_expand_all), NULL);
  gtk_widget_show (treeview);
  gtk_widget_show (sw);
}

/* show a transient message dialog */
static void
metadata_message_dialog (GtkMessageType  type,
                         GtkWindow      *parent,
                         const gchar    *title,
                         const gchar    *message)
{
  GtkWidget *dlg;

  dlg = gtk_message_dialog_new (parent, 0, type, GTK_BUTTONS_OK, "%s", message);

  if (title)
    gtk_window_set_title (GTK_WINDOW (dlg), title);

  gtk_window_set_role (GTK_WINDOW (dlg), "metadata-message");
  gtk_dialog_run (GTK_DIALOG (dlg));
  gtk_widget_destroy (dlg);
}

/* load XMP metadata from a file (the file may contain other data) */
static void
import_dialog_response (GtkWidget *dlg,
                        gint       response_id,
                        gpointer   data)
{
  MetadataGui *mgui = data;

  g_return_if_fail (mgui != NULL);
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar  *filename;
      gchar  *buffer;
      gsize   buffer_length;
      GError *error = NULL;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));

      if (! g_file_get_contents (filename, &buffer, &buffer_length, &error))
        {
          metadata_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dlg),
                                   _("Open failed"), error->message);
          g_error_free (error);
          g_free (filename);
          return;
        }

      if (! xmp_model_parse_buffer (mgui->xmp_model, buffer, buffer_length,
                                    TRUE, &error))
        {
          metadata_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dlg),
                                   _("Open failed"), error->message);
          g_error_free (error);
          g_free (filename);
          return;
        }

      update_icons (mgui);

      g_free (buffer);
      g_free (filename);
    }

  gtk_widget_destroy (dlg); /* FIXME: destroy or unmap? */
}

/* select file to import */
static void
file_import_dialog (GtkWidget   *parent,
                    MetadataGui *mgui)
{
  static GtkWidget *dlg = NULL;

  if (! dlg)
    {
      dlg = gtk_file_chooser_dialog_new (_("Import XMP from File"),
                                         GTK_WINDOW (parent),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,

                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

                                         NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
      gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

      g_signal_connect (dlg, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dlg);
      g_signal_connect (dlg, "response",
                        G_CALLBACK (import_dialog_response),
                        mgui);
    }

  gtk_window_present (GTK_WINDOW (dlg));
}

/* save XMP metadata to a file (only XMP, nothing else) */
static void
export_dialog_response (GtkWidget *dlg,
                        gint       response_id,
                        gpointer   data)
{
  MetadataGui *mgui = data;

  g_return_if_fail (mgui != NULL);

  if (response_id == GTK_RESPONSE_OK)
    {
      GString *buffer;
      gchar   *filename;
      int      fd;

      buffer = g_string_new (NULL);
      xmp_generate_packet (mgui->xmp_model, buffer);

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
      fd = g_open (filename, O_CREAT | O_TRUNC | O_WRONLY | _O_BINARY, 0666);
      if (fd < 0)
        {
          metadata_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dlg),
                                   _("Open failed"),
                                   _("Cannot create file"));
          g_string_free (buffer, TRUE);
          g_free (filename);
          return;
        }

      if (write (fd, buffer->str, buffer->len) < 0)
        {
          metadata_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dlg),
                                   _("Save failed"),
                                   _("Some error occurred while saving"));
          g_string_free (buffer, TRUE);
          g_free (filename);
          return;
        }

      if (close (fd) < 0)
        {
          metadata_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dlg),
                                   _("Save failed"),
                                   _("Could not close the file"));
          g_string_free (buffer, TRUE);
          g_free (filename);
          return;
        }

      g_string_free (buffer, TRUE);
      g_free (filename);
    }

  gtk_widget_destroy (dlg); /* FIXME: destroy or unmap? */
}

/* select file to export */
static void
file_export_dialog (GtkWidget   *parent,
                    MetadataGui *mgui)
{
  static GtkWidget *dlg = NULL;

  if (! dlg)
    {
      dlg = gtk_file_chooser_dialog_new (_("Export XMP to File"),
                                         GTK_WINDOW (parent),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,

                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                         NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
      gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dlg),
                                                      TRUE);

      g_signal_connect (dlg, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dlg);
      g_signal_connect (dlg, "response",
                        G_CALLBACK (export_dialog_response),
                        mgui);
    }

  gtk_window_present (GTK_WINDOW (dlg));
}

static void
metadata_dialog_response (GtkWidget *widget,
                          gint       response_id,
                          gpointer   data)
{
  MetadataGui *mgui = data;

  g_return_if_fail (mgui != NULL);

  switch (response_id)
    {
    case RESPONSE_IMPORT:
      file_import_dialog (widget, mgui);
      break;

    case RESPONSE_EXPORT:
      file_export_dialog (widget, mgui);
      break;

    case GTK_RESPONSE_OK:
      mgui->run_ok = TRUE;
      /*fallthrough*/

    default:
      gtk_widget_destroy (widget);
      break;
    }
}

gboolean
metadata_dialog (gint32    image_ID,
                 XMPModel *xmp_model)
{
  MetadataGui  mgui;
  GtkWidget   *notebook;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  mgui.dlg = gimp_dialog_new (_("Image Properties"), PLUG_IN_BINARY,
                              NULL, 0,
                              gimp_standard_help_func, EDITOR_PROC,

                              _("_Import XMP..."), RESPONSE_IMPORT,
                              _("_Export XMP..."), RESPONSE_EXPORT,
                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (mgui.dlg),
                                           RESPONSE_IMPORT,
                                           RESPONSE_EXPORT,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (mgui.dlg));

  g_signal_connect (mgui.dlg, "response",
                    G_CALLBACK (metadata_dialog_response),
                    &mgui);
  g_signal_connect (mgui.dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (mgui.dlg)->vbox), notebook,
          TRUE, TRUE, 0);
  gtk_widget_show (notebook);

  mgui.xmp_model = xmp_model;
  mgui.edit_icon = gtk_widget_render_icon (mgui.dlg, GTK_STOCK_EDIT,
                                           GTK_ICON_SIZE_MENU, NULL);
  mgui.auto_icon = gtk_widget_render_icon (mgui.dlg, GIMP_STOCK_WILBER,
                                           GTK_ICON_SIZE_MENU, NULL);
  update_icons (&mgui);

  mgui.run_ok = FALSE;

  /* add the tabs to the notebook */
  add_description_tab (notebook);
  add_copyright_tab (notebook);
  add_origin_tab (notebook);
  add_camera1_tab (notebook);
  add_camera2_tab (notebook);
  add_thumbnail_tab (notebook);
  add_advanced_tab (notebook, xmp_model_get_tree_model (mgui.xmp_model));

  gtk_window_set_default_size (GTK_WINDOW (mgui.dlg), 400, 500);
  gtk_widget_show (mgui.dlg);

  /* run, baby, run! */
  gtk_main ();

  /* clean up and return */
  g_object_unref (mgui.auto_icon);
  g_object_unref (mgui.edit_icon);

  return mgui.run_ok;
}
