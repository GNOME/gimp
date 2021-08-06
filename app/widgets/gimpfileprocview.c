/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfileprocview.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>
#include <gegl.h>

#include "widgets-types.h"

#include "core/gimp.h"

#include "plug-in/gimppluginprocedure.h"

#include "gimpfileprocview.h"

#include "gimp-intl.h"

/*  an arbitrary limit to keep the file dialog from becoming too wide  */
#define MAX_EXTENSIONS  4

enum
{
  COLUMN_PROC,
  COLUMN_LABEL,
  COLUMN_EXTENSIONS,
  COLUMN_HELP_ID,
  COLUMN_FILTER,
  N_COLUMNS
};

enum
{
  CHANGED,
  LAST_SIGNAL
};


static void            gimp_file_proc_view_finalize               (GObject             *object);

static void            gimp_file_proc_view_selection_changed      (GtkTreeSelection    *selection,
                                                                   GimpFileProcView    *view);

static GtkFileFilter * gimp_file_proc_view_process_procedure      (GimpPlugInProcedure *file_proc,
                                                                   GtkFileFilter       *all);
static gchar         * gimp_file_proc_view_pattern_from_extension (const gchar         *extension);


G_DEFINE_TYPE (GimpFileProcView, gimp_file_proc_view, GTK_TYPE_TREE_VIEW)

#define parent_class gimp_file_proc_view_parent_class

static guint view_signals[LAST_SIGNAL] = { 0 };


static void
gimp_file_proc_view_class_init (GimpFileProcViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_file_proc_view_finalize;

  klass->changed         = NULL;

  view_signals[CHANGED] = g_signal_new ("changed",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET (GimpFileProcViewClass,
                                                         changed),
                                        NULL, NULL, NULL,
                                        G_TYPE_NONE, 0);
}

static void
gimp_file_proc_view_init (GimpFileProcView *view)
{
}

static void
gimp_file_proc_view_finalize (GObject *object)
{
  GimpFileProcView *view = GIMP_FILE_PROC_VIEW (object);

  if (view->meta_extensions)
    {
      g_list_free_full (view->meta_extensions, (GDestroyNotify) g_free);
      view->meta_extensions = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
gimp_file_proc_view_new (Gimp        *gimp,
                         GSList      *procedures,
                         const gchar *automatic,
                         const gchar *automatic_help_id)
{
  GtkFileFilter     *all_filter;
  GtkTreeView       *view;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;
  GtkListStore      *store;
  GSList            *list;
  GtkTreeIter        iter;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  store = gtk_list_store_new (N_COLUMNS,
                              GIMP_TYPE_PLUG_IN_PROCEDURE, /*  COLUMN_PROC       */
                              G_TYPE_STRING,               /*  COLUMN_LABEL      */
                              G_TYPE_STRING,               /*  COLUMN_EXTENSIONS */
                              G_TYPE_STRING,               /*  COLUMN_HELP_ID    */
                              GTK_TYPE_FILE_FILTER);       /*  COLUMN_FILTER     */

  view = g_object_new (GIMP_TYPE_FILE_PROC_VIEW,
                       "model",      store,
                       "rules-hint", TRUE,
                       NULL);

  g_object_unref (store);

  all_filter = gtk_file_filter_new ();

  for (list = procedures; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;

      if (! proc->prefixes_list) /*  skip URL loaders  */
        {
          const gchar *label   = gimp_procedure_get_label (GIMP_PROCEDURE (proc));
          const gchar *help_id = gimp_procedure_get_help_id (GIMP_PROCEDURE (proc));
          GSList      *list2;

          if (label)
            {
              GtkFileFilter *filter;

              filter = gimp_file_proc_view_process_procedure (proc, all_filter);
              gtk_list_store_append (store, &iter);
              gtk_list_store_set (store, &iter,
                                  COLUMN_PROC,       proc,
                                  COLUMN_LABEL,      label,
                                  COLUMN_EXTENSIONS, proc->extensions,
                                  COLUMN_HELP_ID,    help_id,
                                  COLUMN_FILTER,     filter,
                                  -1);
              g_object_unref (filter);
            }

          for (list2 = proc->extensions_list;
               list2;
               list2 = g_slist_next (list2))
            {
              GimpFileProcView *proc_view = GIMP_FILE_PROC_VIEW (view);
              const gchar      *ext       = list2->data;
              const gchar      *dot       = strchr (ext, '.');

              if (dot && dot != ext)
                proc_view->meta_extensions =
                  g_list_append (proc_view->meta_extensions,
                                 g_strdup (dot + 1));
            }
        }
    }

  if (automatic)
    {
      gtk_list_store_prepend (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLUMN_PROC,    NULL,
                          COLUMN_LABEL,   automatic,
                          COLUMN_HELP_ID, automatic_help_id,
                          COLUMN_FILTER,  all_filter,
                          -1);
    }

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("File Type"));
  gtk_tree_view_column_set_expand (column, TRUE);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text", COLUMN_LABEL,
                                       NULL);

  gtk_tree_view_append_column (view, column);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Extensions"));

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text", COLUMN_EXTENSIONS,
                                       NULL);

  gtk_tree_view_append_column (view, column);

  g_signal_connect (gtk_tree_view_get_selection (view), "changed",
                    G_CALLBACK (gimp_file_proc_view_selection_changed),
                    view);

  return GTK_WIDGET (view);
}

GimpPlugInProcedure *
gimp_file_proc_view_get_proc (GimpFileProcView  *view,
                              gchar            **label,
                              GtkFileFilter    **filter)
{
  GtkTreeModel        *model;
  GtkTreeSelection    *selection;
  GimpPlugInProcedure *proc;
  GtkTreeIter          iter;
  gboolean             has_selection;

  g_return_val_if_fail (GIMP_IS_FILE_PROC_VIEW (view), NULL);

  if (label)  *label  = NULL;
  if (filter) *filter = NULL;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  has_selection = gtk_tree_selection_get_selected (selection, &model, &iter);

  /* if there's no selected item, we return the "automatic" procedure, which,
   * if exists, is the first item.
   */
  if (! has_selection)
    {
      model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));

      if (! gtk_tree_model_get_iter_first (model, &iter))
        return NULL;
    }

  gtk_tree_model_get (model, &iter,
                      COLUMN_PROC, &proc,
                      -1);

  if (proc)
    {
      g_object_unref (proc);

      /* there's no selected item, and no "automatic" procedure.  return NULL.
       */
      if (! has_selection)
        return NULL;
    }

  if (label)
    {
      gtk_tree_model_get (model, &iter,
                          COLUMN_LABEL, label,
                          -1);
    }

  if (filter)
    {
      gtk_tree_model_get (model, &iter,
                          COLUMN_FILTER, filter,
                          -1);
    }

  return proc;
}

gboolean
gimp_file_proc_view_set_proc (GimpFileProcView    *view,
                              GimpPlugInProcedure *proc)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  g_return_val_if_fail (GIMP_IS_FILE_PROC_VIEW (view), FALSE);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      GimpPlugInProcedure *this;

      gtk_tree_model_get (model, &iter,
                          COLUMN_PROC, &this,
                          -1);

      if (this)
        g_object_unref (this);

      if (this == proc)
        break;
    }

  if (iter_valid)
    {
      GtkTreeSelection *selection;

      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

      gtk_tree_selection_select_iter (selection, &iter);
    }

  return iter_valid;
}

gchar *
gimp_file_proc_view_get_help_id (GimpFileProcView *view)
{
  GtkTreeModel     *model;
  GtkTreeSelection *selection;
  GtkTreeIter       iter;

  g_return_val_if_fail (GIMP_IS_FILE_PROC_VIEW (view), NULL);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gchar *help_id;

      gtk_tree_model_get (model, &iter,
                          COLUMN_HELP_ID, &help_id,
                          -1);

      return help_id;
    }

  return NULL;
}

static void
gimp_file_proc_view_selection_changed (GtkTreeSelection *selection,
                                       GimpFileProcView *view)
{
  g_signal_emit (view, view_signals[CHANGED], 0);
}

/**
 * gimp_file_proc_view_process_procedure:
 * @file_proc:
 * @all:
 *
 * Creates a #GtkFileFilter of @file_proc and adds the extensions to
 * the @all filter.
 * The returned #GtkFileFilter has a normal ref and must be unreffed
 * when used.
 **/
static GtkFileFilter *
gimp_file_proc_view_process_procedure (GimpPlugInProcedure *file_proc,
                                       GtkFileFilter       *all)
{
  GtkFileFilter *filter;
  GString       *str;
  GSList        *list;
  gint           i;

  if (! file_proc->extensions_list)
    return NULL;

  filter = gtk_file_filter_new ();
  str    = g_string_new (gimp_procedure_get_label (GIMP_PROCEDURE (file_proc)));

  /* Take ownership directly so we don't have to mess with a floating
   * ref
   */
  g_object_ref_sink (filter);

  for (list = file_proc->mime_types_list; list; list = g_slist_next (list))
    {
      const gchar *mime_type = list->data;

      gtk_file_filter_add_mime_type (filter, mime_type);
      gtk_file_filter_add_mime_type (all, mime_type);
    }

  for (list = file_proc->extensions_list, i = 0;
       list;
       list = g_slist_next (list), i++)
    {
      const gchar *extension = list->data;
      gchar       *pattern;

      pattern = gimp_file_proc_view_pattern_from_extension (extension);
      gtk_file_filter_add_pattern (filter, pattern);
      gtk_file_filter_add_pattern (all, pattern);
      g_free (pattern);

      if (i == 0)
        {
          g_string_append (str, " (");
        }
      else if (i <= MAX_EXTENSIONS)
        {
          g_string_append (str, ", ");
        }

      if (i < MAX_EXTENSIONS)
        {
          g_string_append (str, "*.");
          g_string_append (str, extension);
        }
      else if (i == MAX_EXTENSIONS)
        {
          g_string_append (str, "...");
        }

      if (! list->next)
        {
          g_string_append (str, ")");
        }
    }

  gtk_file_filter_set_name (filter, str->str);
  g_string_free (str, TRUE);

  return filter;
}

static gchar *
gimp_file_proc_view_pattern_from_extension (const gchar *extension)
{
  gchar *pattern;
  gchar *p;
  gint   len, i;

  g_return_val_if_fail (extension != NULL, NULL);

  /* This function assumes that file extensions are 7bit ASCII.  It
   * could certainly be rewritten to handle UTF-8 if this assumption
   * turns out to be incorrect.
   */

  len = strlen (extension);

  pattern = g_new (gchar, 4 + 4 * len);

  strcpy (pattern, "*.");

  for (i = 0, p = pattern + 2; i < len; i++, p+= 4)
    {
      p[0] = '[';
      p[1] = g_ascii_tolower (extension[i]);
      p[2] = g_ascii_toupper (extension[i]);
      p[3] = ']';
    }

  *p = '\0';

  return pattern;
}
