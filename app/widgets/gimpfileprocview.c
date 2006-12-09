/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfileprocview.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpmarshal.h"

#include "plug-in/gimppluginmanager-help-domain.h"
#include "plug-in/gimppluginmanager-locale-domain.h"
#include "plug-in/gimppluginprocedure.h"

#include "gimpfileprocview.h"

#include "gimp-intl.h"


enum
{
  COLUMN_PROC,
  COLUMN_LABEL,
  COLUMN_EXTENSIONS,
  COLUMN_HELP_ID,
  NUM_COLUMNS
};

enum
{
  CHANGED,
  LAST_SIGNAL
};


static void  gimp_file_proc_view_finalize          (GObject          *object);

static void  gimp_file_proc_view_selection_changed (GtkTreeSelection *selection,
                                                    GimpFileProcView *view);


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
                                        NULL, NULL,
                                        gimp_marshal_VOID__VOID,
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
      g_list_foreach (view->meta_extensions, (GFunc) g_free, NULL);
      g_list_free (view->meta_extensions);
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
  GtkTreeView       *view;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;
  GtkListStore      *store;
  GSList            *list;
  GtkTreeIter        iter;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  store = gtk_list_store_new (NUM_COLUMNS,
                              GIMP_TYPE_PLUG_IN_PROCEDURE, /*  COLUMN_PROC   */
                              G_TYPE_STRING,          /*  COLUMN_LABEL       */
                              G_TYPE_STRING,          /*  COLUMN_EXTENSIONS  */
                              G_TYPE_STRING);         /*  COLUMN_HELP_ID     */

  view = g_object_new (GIMP_TYPE_FILE_PROC_VIEW,
                       "model",      store,
                       "rules-hint", TRUE,
                       NULL);

  g_object_unref (store);

  for (list = procedures; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;

      if (! proc->prefixes_list) /*  skip URL loaders  */
        {
          const gchar *locale_domain;
          const gchar *help_domain;
          gchar       *label;
          gchar       *help_id;
          GSList      *list2;

          locale_domain = gimp_plug_in_manager_get_locale_domain (gimp->plug_in_manager,
                                                                  proc->prog, NULL);
          help_domain   = gimp_plug_in_manager_get_help_domain (gimp->plug_in_manager,
                                                                proc->prog, NULL);
          label         = gimp_plug_in_procedure_get_label (proc, locale_domain);
          help_id       = gimp_plug_in_procedure_get_help_id (proc, help_domain);

          if (label)
            {
              gtk_list_store_append (store, &iter);
              gtk_list_store_set (store, &iter,
                                  COLUMN_PROC,       proc,
                                  COLUMN_LABEL,      label,
                                  COLUMN_EXTENSIONS, proc->extensions,
                                  COLUMN_HELP_ID,    help_id,
                                  -1);

              g_free (label);
            }

          g_free (help_id);

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
                              gchar            **label)
{
  GtkTreeModel     *model;
  GtkTreeSelection *selection;
  GtkTreeIter       iter;

  g_return_val_if_fail (GIMP_IS_FILE_PROC_VIEW (view), NULL);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      GimpPlugInProcedure *proc;

      if (label)
        gtk_tree_model_get (model, &iter,
                            COLUMN_PROC,  &proc,
                            COLUMN_LABEL, label,
                            -1);
      else
        gtk_tree_model_get (model, &iter,
                            COLUMN_PROC,  &proc,
                            -1);

      if (proc)
        g_object_unref (proc);

      return proc;
    }

  if (label)
    *label = NULL;

  return NULL;
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
