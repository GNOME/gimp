/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapatheditor.c
 * Copyright (C) 1999-2004 Michael Natterer <mitch@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "ligmawidgetstypes.h"

#undef LIGMA_DISABLE_DEPRECATED
#include "ligmafileentry.h"

#include "ligmahelpui.h"
#include "ligmaicons.h"
#include "ligmapatheditor.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmapatheditor
 * @title: LigmaPathEditor
 * @short_description: Widget for editing a file search path.
 * @see_also: #LigmaFileEntry, #G_SEARCHPATH_SEPARATOR
 *
 * This widget is used to edit file search paths.
 *
 * It shows a list of all directories which are in the search
 * path. You can click a directory to select it. The widget provides a
 * #LigmaFileEntry to change the currently selected directory.
 *
 * There are buttons to add or delete directories as well as "up" and
 * "down" buttons to change the order in which the directories will be
 * searched.
 *
 * Whenever the user adds, deletes, changes or reorders a directory of
 * the search path, the "path_changed" signal will be emitted.
 **/


enum
{
  PATH_CHANGED,
  WRITABLE_CHANGED,
  LAST_SIGNAL
};

enum
{
  COLUMN_UTF8,
  COLUMN_DIRECTORY,
  COLUMN_WRITABLE,
  NUM_COLUMNS
};


static void   ligma_path_editor_new_clicked        (GtkWidget           *widget,
                                                   LigmaPathEditor      *editor);
static void   ligma_path_editor_move_clicked       (GtkWidget           *widget,
                                                   LigmaPathEditor      *editor);
static void   ligma_path_editor_delete_clicked     (GtkWidget           *widget,
                                                   LigmaPathEditor      *editor);
static void   ligma_path_editor_file_entry_changed (GtkWidget           *widget,
                                                   LigmaPathEditor      *editor);
static void   ligma_path_editor_selection_changed  (GtkTreeSelection    *sel,
                                                   LigmaPathEditor      *editor);
static void   ligma_path_editor_writable_toggled   (GtkCellRendererToggle *toggle,
                                                   gchar               *path_str,
                                                   LigmaPathEditor      *editor);


G_DEFINE_TYPE (LigmaPathEditor, ligma_path_editor, GTK_TYPE_BOX)

#define parent_class ligma_path_editor_parent_class

static guint ligma_path_editor_signals[LAST_SIGNAL] = { 0 };


static void
ligma_path_editor_class_init (LigmaPathEditorClass *klass)
{
  /**
   * LigmaPathEditor::path-changed:
   *
   * This signal is emitted whenever the user adds, deletes, modifies
   * or reorders an element of the search path.
   **/
  ligma_path_editor_signals[PATH_CHANGED] =
    g_signal_new ("path-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaPathEditorClass, path_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * LigmaPathEditor::writable-changed:
   *
   * This signal is emitted whenever the "writable" column of a directory
   * is changed, either by the user clicking on it or by calling
   * ligma_path_editor_set_dir_writable().
   **/
  ligma_path_editor_signals[WRITABLE_CHANGED] =
    g_signal_new ("writable-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaPathEditorClass, writable_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  klass->path_changed     = NULL;
  klass->writable_changed = NULL;
}

static void
ligma_path_editor_init (LigmaPathEditor *editor)
{
  GtkWidget         *button_box;
  GtkWidget         *button;
  GtkWidget         *image;
  GtkWidget         *scrolled_window;
  GtkWidget         *tv;
  GtkTreeViewColumn *col;
  GtkCellRenderer   *renderer;

  editor->file_entry = NULL;
  editor->sel_path   = NULL;
  editor->num_items  = 0;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  editor->upper_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (editor), editor->upper_hbox, FALSE, TRUE, 0);
  gtk_widget_show (editor->upper_hbox);

  button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (button_box), TRUE);
  gtk_box_pack_start (GTK_BOX (editor->upper_hbox), button_box, FALSE, TRUE, 0);
  gtk_widget_show (button_box);

  editor->new_button = button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_DOCUMENT_NEW,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_path_editor_new_clicked),
                    editor);

  ligma_help_set_help_data (editor->new_button,
                           _("Add a new folder"),
                           NULL);

  editor->up_button = button = gtk_button_new ();
  gtk_widget_set_sensitive (button, FALSE);
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_GO_UP,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_path_editor_move_clicked),
                    editor);

  ligma_help_set_help_data (editor->up_button,
                           _("Move the selected folder up"),
                           NULL);

  editor->down_button = button = gtk_button_new ();
  gtk_widget_set_sensitive (button, FALSE);
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_GO_DOWN,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_path_editor_move_clicked),
                    editor);

  ligma_help_set_help_data (editor->down_button,
                           _("Move the selected folder down"),
                           NULL);

  editor->delete_button = button = gtk_button_new ();
  gtk_widget_set_sensitive (button, FALSE);
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_EDIT_DELETE,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_path_editor_delete_clicked),
                    editor);

  ligma_help_set_help_data (editor->delete_button,
                           _("Remove the selected folder from the list"),
                           NULL);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (editor), scrolled_window, TRUE, TRUE, 2);
  gtk_widget_show (scrolled_window);

  editor->dir_list = gtk_list_store_new (NUM_COLUMNS,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING,
                                         G_TYPE_BOOLEAN);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (editor->dir_list));
  g_object_unref (editor->dir_list);

  renderer = gtk_cell_renderer_toggle_new ();

  g_signal_connect (renderer, "toggled",
                    G_CALLBACK (ligma_path_editor_writable_toggled),
                    editor);

  editor->writable_column = col = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (col, _("Writable"));
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_tree_view_column_add_attribute (col, renderer, "active", COLUMN_WRITABLE);

  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

  gtk_tree_view_column_set_visible (col, FALSE);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv),
                                               -1, _("Folder"),
                                               gtk_cell_renderer_text_new (),
                                               "text", COLUMN_UTF8,
                                               NULL);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), TRUE);

  gtk_container_add (GTK_CONTAINER (scrolled_window), tv);
  gtk_widget_show (tv);

  editor->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  g_signal_connect (editor->sel, "changed",
                    G_CALLBACK (ligma_path_editor_selection_changed),
                    editor);
}

/**
 * ligma_path_editor_new:
 * @title: The title of the #GtkFileChooser dialog which can be popped up.
 * @path: (nullable): The initial search path.
 *
 * Creates a new #LigmaPathEditor widget.
 *
 * The elements of the initial search path must be separated with the
 * #G_SEARCHPATH_SEPARATOR character.
 *
 * Returns: A pointer to the new #LigmaPathEditor widget.
 **/
GtkWidget *
ligma_path_editor_new (const gchar *title,
                      const gchar *path)
{
  LigmaPathEditor *editor;

  g_return_val_if_fail (title != NULL, NULL);

  editor = g_object_new (LIGMA_TYPE_PATH_EDITOR, NULL);

  editor->file_entry = ligma_file_entry_new (title, "", TRUE, TRUE);
  gtk_widget_set_sensitive (editor->file_entry, FALSE);
  gtk_box_pack_start (GTK_BOX (editor->upper_hbox), editor->file_entry,
                      TRUE, TRUE, 0);
  gtk_widget_show (editor->file_entry);

  g_signal_connect (editor->file_entry, "filename-changed",
                    G_CALLBACK (ligma_path_editor_file_entry_changed),
                    editor);

  if (path)
    ligma_path_editor_set_path (editor, path);

  return GTK_WIDGET (editor);
}

/**
 * ligma_path_editor_get_path:
 * @editor: The path editor you want to get the search path from.
 *
 * The elements of the returned search path string are separated with the
 * #G_SEARCHPATH_SEPARATOR character.
 *
 * Note that you have to g_free() the returned string.
 *
 * Returns: The search path the user has selected in the path editor.
 **/
gchar *
ligma_path_editor_get_path (LigmaPathEditor *editor)
{
  GtkTreeModel *model;
  GString      *path;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  g_return_val_if_fail (LIGMA_IS_PATH_EDITOR (editor), g_strdup (""));

  model = GTK_TREE_MODEL (editor->dir_list);

  path = g_string_new ("");

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gchar *dir;

      gtk_tree_model_get (model, &iter,
                          COLUMN_DIRECTORY, &dir,
                          -1);

      if (path->len > 0)
        g_string_append_c (path, G_SEARCHPATH_SEPARATOR);

      g_string_append (path, dir);

      g_free (dir);
    }

  return g_string_free (path, FALSE);
}

/**
 * ligma_path_editor_set_path:
 * @editor: The path editor you want to set the search path from.
 * @path:   The new path to set.
 *
 * The elements of the initial search path must be separated with the
 * %G_SEARCHPATH_SEPARATOR character.
 **/
void
ligma_path_editor_set_path (LigmaPathEditor *editor,
                           const gchar    *path)
{
  gchar *old_path;
  GList *path_list;
  GList *list;

  g_return_if_fail (LIGMA_IS_PATH_EDITOR (editor));

  old_path = ligma_path_editor_get_path (editor);

  if (old_path && path && strcmp (old_path, path) == 0)
    {
      g_free (old_path);
      return;
    }

  g_free (old_path);

  path_list = ligma_path_parse (path, 256, FALSE, NULL);

  gtk_list_store_clear (editor->dir_list);

  for (list = path_list; list; list = g_list_next (list))
    {
      gchar       *directory = list->data;
      gchar       *utf8;
      GtkTreeIter  iter;

      utf8 = g_filename_to_utf8 (directory, -1, NULL, NULL, NULL);

      gtk_list_store_append (editor->dir_list, &iter);
      gtk_list_store_set (editor->dir_list, &iter,
                          COLUMN_UTF8,      utf8,
                          COLUMN_DIRECTORY, directory,
                          COLUMN_WRITABLE,  FALSE,
                          -1);

      g_free (utf8);

      editor->num_items++;
    }

  ligma_path_free (path_list);

  g_signal_emit (editor, ligma_path_editor_signals[PATH_CHANGED], 0);
}

gchar *
ligma_path_editor_get_writable_path (LigmaPathEditor *editor)
{
  GtkTreeModel *model;
  GString      *path;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  g_return_val_if_fail (LIGMA_IS_PATH_EDITOR (editor), g_strdup (""));

  model = GTK_TREE_MODEL (editor->dir_list);

  path = g_string_new ("");

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gchar    *dir;
      gboolean  dir_writable;

      gtk_tree_model_get (model, &iter,
                          COLUMN_DIRECTORY, &dir,
                          COLUMN_WRITABLE,  &dir_writable,
                          -1);

      if (dir_writable)
        {
          if (path->len > 0)
            g_string_append_c (path, G_SEARCHPATH_SEPARATOR);

          g_string_append (path, dir);
        }

      g_free (dir);
    }

  return g_string_free (path, FALSE);
}

void
ligma_path_editor_set_writable_path (LigmaPathEditor *editor,
                                    const gchar    *path)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      iter_valid;
  GList        *path_list;
  gboolean      writable_changed = FALSE;

  g_return_if_fail (LIGMA_IS_PATH_EDITOR (editor));

  gtk_tree_view_column_set_visible (editor->writable_column, TRUE);

  path_list = ligma_path_parse (path, 256, FALSE, NULL);

  model = GTK_TREE_MODEL (editor->dir_list);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gchar    *dir;
      gboolean  dir_writable;
      gboolean  new_writable = FALSE;

      gtk_tree_model_get (model, &iter,
                          COLUMN_DIRECTORY, &dir,
                          COLUMN_WRITABLE,  &dir_writable,
                          -1);

      if (g_list_find_custom (path_list, dir, (GCompareFunc) strcmp))
        new_writable = TRUE;

      g_free (dir);

      if (dir_writable != new_writable)
        {
          gtk_list_store_set (editor->dir_list, &iter,
                              COLUMN_WRITABLE, new_writable,
                              -1);

          writable_changed = TRUE;
        }
    }

  ligma_path_free (path_list);

  if (writable_changed)
    g_signal_emit (editor, ligma_path_editor_signals[WRITABLE_CHANGED], 0);
}

gboolean
ligma_path_editor_get_dir_writable (LigmaPathEditor *editor,
                                   const gchar    *directory)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  g_return_val_if_fail (LIGMA_IS_PATH_EDITOR (editor), FALSE);
  g_return_val_if_fail (directory != NULL, FALSE);

  model = GTK_TREE_MODEL (editor->dir_list);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gchar    *dir;
      gboolean  dir_writable;

      gtk_tree_model_get (model, &iter,
                          COLUMN_DIRECTORY, &dir,
                          COLUMN_WRITABLE,  &dir_writable,
                          -1);

      if (! strcmp (dir, directory))
        {
          g_free (dir);

          return dir_writable;
        }

      g_free (dir);
    }

  return FALSE;
}

void
ligma_path_editor_set_dir_writable (LigmaPathEditor *editor,
                                   const gchar    *directory,
                                   gboolean        writable)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  g_return_if_fail (LIGMA_IS_PATH_EDITOR (editor));
  g_return_if_fail (directory != NULL);

  model = GTK_TREE_MODEL (editor->dir_list);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gchar    *dir;
      gboolean  dir_writable;

      gtk_tree_model_get (model, &iter,
                          COLUMN_DIRECTORY, &dir,
                          COLUMN_WRITABLE,  &dir_writable,
                          -1);

      if (! strcmp (dir, directory) && dir_writable != writable)
        {
          gtk_list_store_set (editor->dir_list, &iter,
                              COLUMN_WRITABLE, writable ? TRUE : FALSE,
                              -1);

          g_signal_emit (editor, ligma_path_editor_signals[WRITABLE_CHANGED], 0);

          g_free (dir);
          break;
        }

      g_free (dir);
    }
}


/*  private functions  */

static void
ligma_path_editor_new_clicked (GtkWidget      *widget,
                              LigmaPathEditor *editor)
{
  if (editor->sel_path)
    {
      g_signal_handlers_block_by_func (editor->sel,
                                       ligma_path_editor_selection_changed,
                                       editor);

      gtk_tree_selection_unselect_path (editor->sel, editor->sel_path);

      g_signal_handlers_unblock_by_func (editor->sel,
                                         ligma_path_editor_selection_changed,
                                         editor);

      gtk_tree_path_free (editor->sel_path);
      editor->sel_path = NULL;
    }

  gtk_widget_set_sensitive (editor->delete_button, FALSE);
  gtk_widget_set_sensitive (editor->up_button, FALSE);
  gtk_widget_set_sensitive (editor->down_button, FALSE);
  gtk_widget_set_sensitive (editor->file_entry, TRUE);

  gtk_editable_set_position
    (GTK_EDITABLE (ligma_file_entry_get_entry (LIGMA_FILE_ENTRY (editor->file_entry))), -1);
  gtk_widget_grab_focus
    (ligma_file_entry_get_entry (LIGMA_FILE_ENTRY (editor->file_entry)));
}

static void
ligma_path_editor_move_clicked (GtkWidget      *widget,
                               LigmaPathEditor *editor)
{
  GtkTreePath  *path;
  GtkTreeModel *model;
  GtkTreeIter   iter1, iter2;
  gchar        *utf81, *utf82;
  gchar        *dir1, *dir2;
  gboolean      writable1, writable2;

  if (editor->sel_path == NULL)
    return;

  path = gtk_tree_path_copy (editor->sel_path);

  if (widget == editor->up_button)
    gtk_tree_path_prev (path);
  else
    gtk_tree_path_next (path);

  model = GTK_TREE_MODEL (editor->dir_list);

  gtk_tree_model_get_iter (model, &iter1, editor->sel_path);
  gtk_tree_model_get_iter (model, &iter2, path);

  gtk_tree_model_get (model, &iter1,
                      COLUMN_UTF8,      &utf81,
                      COLUMN_DIRECTORY, &dir1,
                      COLUMN_WRITABLE,  &writable1,
                      -1);
  gtk_tree_model_get (model, &iter2,
                      COLUMN_UTF8,      &utf82,
                      COLUMN_DIRECTORY, &dir2,
                      COLUMN_WRITABLE,  &writable2,
                      -1);

  gtk_list_store_set (editor->dir_list, &iter1,
                      COLUMN_UTF8,      utf82,
                      COLUMN_DIRECTORY, dir2,
                      COLUMN_WRITABLE,  writable2,
                      -1);
  gtk_list_store_set (editor->dir_list, &iter2,
                      COLUMN_UTF8,      utf81,
                      COLUMN_DIRECTORY, dir1,
                      COLUMN_WRITABLE,  writable1,
                      -1);

  g_free (utf81);
  g_free (utf82);

  g_free (dir2);
  g_free (dir1);

  gtk_tree_selection_select_iter (editor->sel, &iter2);

  g_signal_emit (editor, ligma_path_editor_signals[PATH_CHANGED], 0);
}

static void
ligma_path_editor_delete_clicked (GtkWidget      *widget,
                                 LigmaPathEditor *editor)
{
  GtkTreeIter iter;
  gboolean    dir_writable;

  if (editor->sel_path == NULL)
    return;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (editor->dir_list), &iter,
                           editor->sel_path);

  gtk_tree_model_get (GTK_TREE_MODEL (editor->dir_list), &iter,
                      COLUMN_WRITABLE, &dir_writable,
                      -1);

  gtk_list_store_remove (editor->dir_list, &iter);

  editor->num_items--;

  if (editor->num_items == 0)
    {
      gtk_tree_path_free (editor->sel_path);
      editor->sel_path = NULL;

      g_signal_handlers_block_by_func (editor->file_entry,
                                       ligma_path_editor_file_entry_changed,
                                       editor);

      ligma_file_entry_set_filename (LIGMA_FILE_ENTRY (editor->file_entry), "");

      g_signal_handlers_unblock_by_func (editor->file_entry,
                                         ligma_path_editor_file_entry_changed,
                                         editor);

      gtk_widget_set_sensitive (editor->delete_button, FALSE);
      gtk_widget_set_sensitive (editor->up_button,     FALSE);
      gtk_widget_set_sensitive (editor->down_button,   FALSE);
      gtk_widget_set_sensitive (editor->file_entry,    FALSE);
    }
  else
    {
      gint *indices;

      indices = gtk_tree_path_get_indices (editor->sel_path);
      if ((indices[0] == editor->num_items) && (indices[0] > 0))
        gtk_tree_path_prev (editor->sel_path);

      gtk_tree_selection_select_path (editor->sel, editor->sel_path);
    }

  g_signal_emit (editor, ligma_path_editor_signals[PATH_CHANGED], 0);

  if (dir_writable)
    g_signal_emit (editor, ligma_path_editor_signals[WRITABLE_CHANGED], 0);
}

static void
ligma_path_editor_file_entry_changed (GtkWidget      *widget,
                                     LigmaPathEditor *editor)
{
  gchar       *dir;
  gchar       *utf8;
  GtkTreeIter  iter;

  dir = ligma_file_entry_get_filename (LIGMA_FILE_ENTRY (widget));
  if (strcmp (dir, "") == 0)
    {
      g_free (dir);
      return;
    }

  utf8 = g_filename_display_name (dir);

  if (editor->sel_path == NULL)
    {
      gtk_list_store_append (editor->dir_list, &iter);
      gtk_list_store_set (editor->dir_list, &iter,
                          COLUMN_UTF8,      utf8,
                          COLUMN_DIRECTORY, dir,
                          COLUMN_WRITABLE,  FALSE,
                          -1);
      editor->num_items++;

      gtk_tree_selection_select_iter (editor->sel, &iter);
    }
  else
    {
      gtk_tree_model_get_iter (GTK_TREE_MODEL (editor->dir_list), &iter,
                               editor->sel_path);
      gtk_list_store_set (editor->dir_list, &iter,
                          COLUMN_UTF8,      utf8,
                          COLUMN_DIRECTORY, dir,
                          -1);
    }

  g_free (dir);
  g_free (utf8);

  g_signal_emit (editor, ligma_path_editor_signals[PATH_CHANGED], 0);
}

static void
ligma_path_editor_selection_changed (GtkTreeSelection *sel,
                                    LigmaPathEditor   *editor)
{
  GtkTreeIter  iter;
  gchar       *directory;
  gint        *indices;

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (editor->dir_list), &iter,
                          COLUMN_DIRECTORY, &directory,
                          -1);

      g_signal_handlers_block_by_func (editor->file_entry,
                                       ligma_path_editor_file_entry_changed,
                                       editor);

      ligma_file_entry_set_filename (LIGMA_FILE_ENTRY (editor->file_entry),
                                    directory);

      g_signal_handlers_unblock_by_func (editor->file_entry,
                                         ligma_path_editor_file_entry_changed,
                                         editor);

      g_free (directory);

      if (editor->sel_path)
        gtk_tree_path_free (editor->sel_path);

      editor->sel_path =
        gtk_tree_model_get_path (GTK_TREE_MODEL (editor->dir_list), &iter);

      indices = gtk_tree_path_get_indices (editor->sel_path);

      gtk_widget_set_sensitive (editor->delete_button, TRUE);
      gtk_widget_set_sensitive (editor->up_button, (indices[0] > 0));
      gtk_widget_set_sensitive (editor->down_button,
                                (indices[0] < (editor->num_items - 1)));
      gtk_widget_set_sensitive (editor->file_entry, TRUE);
    }
  else
    {
      g_signal_handlers_block_by_func (sel,
                                       ligma_path_editor_selection_changed,
                                       editor);

      gtk_tree_selection_select_path (editor->sel, editor->sel_path);

      g_signal_handlers_unblock_by_func (sel,
                                         ligma_path_editor_selection_changed,
                                         editor);
    }
}

static void
ligma_path_editor_writable_toggled (GtkCellRendererToggle *toggle,
                                   gchar                 *path_str,
                                   LigmaPathEditor        *editor)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (editor->dir_list), &iter, path))
    {
      gboolean dir_writable;

      gtk_tree_model_get (GTK_TREE_MODEL (editor->dir_list), &iter,
                          COLUMN_WRITABLE,  &dir_writable,
                          -1);

      gtk_list_store_set (editor->dir_list, &iter,
                          COLUMN_WRITABLE, ! dir_writable,
                          -1);

      g_signal_emit (editor, ligma_path_editor_signals[WRITABLE_CHANGED], 0);
    }

  gtk_tree_path_free (path);
}
