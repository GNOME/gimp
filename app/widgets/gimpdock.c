/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadock.c
 * Copyright (C) 2001-2018 Michael Natterer <mitch@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "ligmadialogfactory.h"
#include "ligmadock.h"
#include "ligmadockable.h"
#include "ligmadockbook.h"
#include "ligmadockcolumns.h"
#include "ligmadockcontainer.h"
#include "ligmadockwindow.h"
#include "ligmapanedbox.h"
#include "ligmauimanager.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  BOOK_ADDED,
  BOOK_REMOVED,
  DESCRIPTION_INVALIDATED,
  GEOMETRY_INVALIDATED,
  LAST_SIGNAL
};


struct _LigmaDockPrivate
{
  GtkWidget *main_vbox;
  GtkWidget *paned_vbox;

  GList     *dockbooks;

  gint       ID;
};


static void       ligma_dock_dispose                (GObject      *object);

static gchar    * ligma_dock_real_get_description   (LigmaDock     *dock,
                                                    gboolean      complete);
static void       ligma_dock_real_book_added        (LigmaDock     *dock,
                                                    LigmaDockbook *dockbook);
static void       ligma_dock_real_book_removed      (LigmaDock     *dock,
                                                    LigmaDockbook *dockbook);
static void       ligma_dock_invalidate_description (LigmaDock     *dock);
static gboolean   ligma_dock_dropped_cb             (GtkWidget    *notebook,
                                                    GtkWidget    *child,
                                                    gint          insert_index,
                                                    gpointer      data);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaDock, ligma_dock, GTK_TYPE_BOX)

#define parent_class ligma_dock_parent_class

static guint dock_signals[LAST_SIGNAL] = { 0 };


static void
ligma_dock_class_init (LigmaDockClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  dock_signals[BOOK_ADDED] =
    g_signal_new ("book-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDockClass, book_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DOCKBOOK);

  dock_signals[BOOK_REMOVED] =
    g_signal_new ("book-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDockClass, book_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DOCKBOOK);

  dock_signals[DESCRIPTION_INVALIDATED] =
    g_signal_new ("description-invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDockClass, description_invalidated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  dock_signals[GEOMETRY_INVALIDATED] =
    g_signal_new ("geometry-invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDockClass, geometry_invalidated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose          = ligma_dock_dispose;

  klass->get_description         = ligma_dock_real_get_description;
  klass->set_host_geometry_hints = NULL;
  klass->book_added              = ligma_dock_real_book_added;
  klass->book_removed            = ligma_dock_real_book_removed;
  klass->description_invalidated = NULL;
  klass->geometry_invalidated    = NULL;

  gtk_widget_class_set_css_name (widget_class, "LigmaDock");
}

static void
ligma_dock_init (LigmaDock *dock)
{
  static gint  dock_ID = 1;
  gchar       *name    = NULL;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (dock),
                                  GTK_ORIENTATION_VERTICAL);

  dock->p = ligma_dock_get_instance_private (dock);
  dock->p->ID = dock_ID++;

  name = g_strdup_printf ("ligma-internal-dock-%d", dock->p->ID);
  gtk_widget_set_name (GTK_WIDGET (dock), name);
  g_free (name);

  dock->p->main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (dock), dock->p->main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (dock->p->main_vbox);

  dock->p->paned_vbox = ligma_paned_box_new (FALSE, 0, GTK_ORIENTATION_VERTICAL);
  ligma_paned_box_set_dropped_cb (LIGMA_PANED_BOX (dock->p->paned_vbox),
                                 ligma_dock_dropped_cb,
                                 dock);
  gtk_box_pack_start (GTK_BOX (dock->p->main_vbox), dock->p->paned_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (dock->p->paned_vbox);
}

static void
ligma_dock_dispose (GObject *object)
{
  LigmaDock *dock = LIGMA_DOCK (object);

  while (dock->p->dockbooks)
    {
      LigmaDockbook *dockbook = dock->p->dockbooks->data;

      g_object_ref (dockbook);
      ligma_dock_remove_book (dock, dockbook);
      gtk_widget_destroy (GTK_WIDGET (dockbook));
      g_object_unref (dockbook);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gchar *
ligma_dock_real_get_description (LigmaDock *dock,
                                gboolean  complete)
{
  GString *desc;
  GList   *list;

  desc = g_string_new (NULL);

  for (list = ligma_dock_get_dockbooks (dock);
       list;
       list = g_list_next (list))
    {
      LigmaDockbook *dockbook = list->data;
      GList        *children;
      GList        *child;

      if (complete)
        {
          /* Include all dockables */
          children = gtk_container_get_children (GTK_CONTAINER (dockbook));
        }
      else
        {
          GtkWidget *dockable = NULL;
          gint       page_num = 0;

          page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));
          dockable = gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

          /* Only include active dockables */
          children = g_list_append (NULL, dockable);
        }

      for (child = children; child; child = g_list_next (child))
        {
          LigmaDockable *dockable = child->data;

          g_string_append (desc, ligma_dockable_get_name (dockable));

          if (g_list_next (child))
            g_string_append (desc, LIGMA_DOCK_DOCKABLE_SEPARATOR);
        }

      g_list_free (children);

      if (g_list_next (list))
        g_string_append (desc, LIGMA_DOCK_BOOK_SEPARATOR);
    }

  return g_string_free (desc, FALSE);
}

static void
ligma_dock_real_book_added (LigmaDock     *dock,
                           LigmaDockbook *dockbook)
{
  g_signal_connect_object (dockbook, "switch-page",
                           G_CALLBACK (ligma_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);
}

static void
ligma_dock_real_book_removed (LigmaDock     *dock,
                             LigmaDockbook *dockbook)
{
  g_signal_handlers_disconnect_by_func (dockbook,
                                        ligma_dock_invalidate_description,
                                        dock);
}

static void
ligma_dock_invalidate_description (LigmaDock *dock)
{
  g_return_if_fail (LIGMA_IS_DOCK (dock));

  g_signal_emit (dock, dock_signals[DESCRIPTION_INVALIDATED], 0);
}

static gboolean
ligma_dock_dropped_cb (GtkWidget *notebook,
                      GtkWidget *child,
                      gint       insert_index,
                      gpointer   data)
{
  LigmaDock          *dock     = LIGMA_DOCK (data);
  LigmaDockbook      *dockbook = LIGMA_DOCKBOOK (notebook);
  LigmaDockable      *dockable = LIGMA_DOCKABLE (child);
  LigmaDialogFactory *factory;
  GtkWidget         *new_dockbook;

  /*  if dropping to the same dock, take care that we don't try
   *  to reorder the *only* dockable in the dock
   */
  if (ligma_dockbook_get_dock (dockbook) == dock)
    {
      GList *children    = gtk_container_get_children (GTK_CONTAINER (dockable));
      gint   n_dockables = g_list_length (children);
      gint   n_books     = g_list_length (ligma_dock_get_dockbooks (dock));

      g_list_free (children);

      if (n_books == 1 && n_dockables == 1)
        return TRUE; /* successfully do nothing */
    }

  /* Detach the dockable from the old dockbook */
  g_object_ref (dockable);
  gtk_notebook_detach_tab (GTK_NOTEBOOK (notebook), child);

  /* Create a new dockbook */
  factory = ligma_dock_get_dialog_factory (dock);
  new_dockbook = ligma_dockbook_new (ligma_dialog_factory_get_menu_factory (factory));
  ligma_dock_add_book (dock, LIGMA_DOCKBOOK (new_dockbook), insert_index);

  /* Add the dockable to new new dockbook */
  gtk_notebook_append_page (GTK_NOTEBOOK (new_dockbook), child, NULL);
  g_object_unref (dockable);

  return TRUE;
}


/*  public functions  */

/**
 * ligma_dock_get_description:
 * @dock:
 * @complete: If %TRUE, only includes the active dockables, i.e. not the
 *            dockables in a non-active GtkNotebook tab
 *
 * Returns: A string describing the contents of the dock.
 **/
gchar *
ligma_dock_get_description (LigmaDock *dock,
                           gboolean  complete)
{
  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);

  if (LIGMA_DOCK_GET_CLASS (dock)->get_description)
    return LIGMA_DOCK_GET_CLASS (dock)->get_description (dock, complete);

  return NULL;
}

/**
 * ligma_dock_set_host_geometry_hints:
 * @dock:   The dock
 * @window: The #GtkWindow to adapt to hosting the dock
 *
 * Some docks have some specific needs on the #GtkWindow they are
 * in. This function allows such docks to perform any such setup on
 * the #GtkWindow they are in/will be put in.
 **/
void
ligma_dock_set_host_geometry_hints (LigmaDock  *dock,
                                   GtkWindow *window)
{
  g_return_if_fail (LIGMA_IS_DOCK (dock));
  g_return_if_fail (GTK_IS_WINDOW (window));

  if (LIGMA_DOCK_GET_CLASS (dock)->set_host_geometry_hints)
    LIGMA_DOCK_GET_CLASS (dock)->set_host_geometry_hints (dock, window);
}

/**
 * ligma_dock_invalidate_geometry:
 * @dock:
 *
 * Call when the dock needs to setup its host #GtkWindow with
 * GtkDock::set_host_geometry_hints().
 **/
void
ligma_dock_invalidate_geometry (LigmaDock *dock)
{
  g_return_if_fail (LIGMA_IS_DOCK (dock));

  g_signal_emit (dock, dock_signals[GEOMETRY_INVALIDATED], 0);
}

/**
 * ligma_dock_update_with_context:
 * @dock:
 * @context:
 *
 * Set the @context on all dockables in the @dock.
 **/
void
ligma_dock_update_with_context (LigmaDock    *dock,
                               LigmaContext *context)
{
  GList *iter = NULL;

  for (iter = ligma_dock_get_dockbooks (dock);
       iter;
       iter = g_list_next (iter))
    {
      LigmaDockbook *dockbook = LIGMA_DOCKBOOK (iter->data);

      ligma_dockbook_update_with_context (dockbook, context);
    }
}

/**
 * ligma_dock_get_context:
 * @dock:
 *
 * Returns: The #LigmaContext for the #LigmaDockWindow the @dock is in.
 **/
LigmaContext *
ligma_dock_get_context (LigmaDock *dock)
{
  LigmaContext *context = NULL;

  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);

  /* First try LigmaDockColumns */
  if (! context)
    {
      LigmaDockColumns *dock_columns;

      dock_columns =
        LIGMA_DOCK_COLUMNS (gtk_widget_get_ancestor (GTK_WIDGET (dock),
                                                    LIGMA_TYPE_DOCK_COLUMNS));

      if (dock_columns)
        context = ligma_dock_columns_get_context (dock_columns);
    }

  /* Then LigmaDockWindow */
  if (! context)
    {
      LigmaDockWindow *dock_window = ligma_dock_window_from_dock (dock);

      if (dock_window)
        context = ligma_dock_window_get_context (dock_window);
    }

  return context;
}

/**
 * ligma_dock_get_dialog_factory:
 * @dock:
 *
 * Returns: The #LigmaDialogFactory for the #LigmaDockWindow the @dock
 *          is in.
 **/
LigmaDialogFactory *
ligma_dock_get_dialog_factory (LigmaDock *dock)
{
  LigmaDialogFactory *dialog_factory = NULL;

  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);

  /* First try LigmaDockColumns */
  if (! dialog_factory)
    {
      LigmaDockColumns *dock_columns;

      dock_columns =
        LIGMA_DOCK_COLUMNS (gtk_widget_get_ancestor (GTK_WIDGET (dock),
                                                    LIGMA_TYPE_DOCK_COLUMNS));

      if (dock_columns)
        dialog_factory = ligma_dock_columns_get_dialog_factory (dock_columns);
    }

  /* Then LigmaDockWindow */
  if (! dialog_factory)
    {
      LigmaDockWindow *dock_window = ligma_dock_window_from_dock (dock);

      if (dock_window)
        dialog_factory = ligma_dock_container_get_dialog_factory (LIGMA_DOCK_CONTAINER (dock_window));
    }

  return dialog_factory;
}

/**
 * ligma_dock_get_ui_manager:
 * @dock:
 *
 * Returns: The #LigmaUIManager for the #LigmaDockWindow the @dock is
 *          in.
 **/
LigmaUIManager *
ligma_dock_get_ui_manager (LigmaDock *dock)
{
  LigmaUIManager *ui_manager = NULL;

  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);

  /* First try LigmaDockColumns */
  if (! ui_manager)
    {
      LigmaDockColumns *dock_columns;

      dock_columns =
        LIGMA_DOCK_COLUMNS (gtk_widget_get_ancestor (GTK_WIDGET (dock),
                                                    LIGMA_TYPE_DOCK_COLUMNS));

      if (dock_columns)
        ui_manager = ligma_dock_columns_get_ui_manager (dock_columns);
    }

  /* Then LigmaDockContainer */
  if (! ui_manager)
    {
      LigmaDockWindow *dock_window = ligma_dock_window_from_dock (dock);

      if (dock_window)
        {
          LigmaDockContainer *dock_container = LIGMA_DOCK_CONTAINER (dock_window);

          ui_manager = ligma_dock_container_get_ui_manager (dock_container);
        }
    }

  return ui_manager;
}

GList *
ligma_dock_get_dockbooks (LigmaDock *dock)
{
  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);

  return dock->p->dockbooks;
}

gint
ligma_dock_get_n_dockables (LigmaDock *dock)
{
  GList *list = NULL;
  gint   n    = 0;

  g_return_val_if_fail (LIGMA_IS_DOCK (dock), 0);

  for (list = dock->p->dockbooks; list; list = list->next)
    n += gtk_notebook_get_n_pages (GTK_NOTEBOOK (list->data));

  return n;
}

GtkWidget *
ligma_dock_get_main_vbox (LigmaDock *dock)
{
  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);

  return dock->p->main_vbox;
}

GtkWidget *
ligma_dock_get_vbox (LigmaDock *dock)
{
  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);

  return dock->p->paned_vbox;
}

void
ligma_dock_set_id (LigmaDock *dock,
                  gint      ID)
{
  g_return_if_fail (LIGMA_IS_DOCK (dock));

  dock->p->ID = ID;
}

gint
ligma_dock_get_id (LigmaDock *dock)
{
  g_return_val_if_fail (LIGMA_IS_DOCK (dock), 0);

  return dock->p->ID;
}

void
ligma_dock_add_book (LigmaDock     *dock,
                    LigmaDockbook *dockbook,
                    gint          index)
{
  g_return_if_fail (LIGMA_IS_DOCK (dock));
  g_return_if_fail (LIGMA_IS_DOCKBOOK (dockbook));
  g_return_if_fail (ligma_dockbook_get_dock (dockbook) == NULL);

  ligma_dockbook_set_dock (dockbook, dock);

  g_signal_connect_object (dockbook, "dockable-added",
                           G_CALLBACK (ligma_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);
  g_signal_connect_object (dockbook, "dockable-removed",
                           G_CALLBACK (ligma_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);
  g_signal_connect_object (dockbook, "dockable-reordered",
                           G_CALLBACK (ligma_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);

  dock->p->dockbooks = g_list_insert (dock->p->dockbooks, dockbook, index);
  ligma_paned_box_add_widget (LIGMA_PANED_BOX (dock->p->paned_vbox),
                             GTK_WIDGET (dockbook),
                             index);
  gtk_widget_show (GTK_WIDGET (dockbook));

  ligma_dock_invalidate_description (dock);

  g_signal_emit (dock, dock_signals[BOOK_ADDED], 0, dockbook);
}

void
ligma_dock_remove_book (LigmaDock     *dock,
                       LigmaDockbook *dockbook)
{
  g_return_if_fail (LIGMA_IS_DOCK (dock));
  g_return_if_fail (LIGMA_IS_DOCKBOOK (dockbook));
  g_return_if_fail (ligma_dockbook_get_dock (dockbook) == dock);

  ligma_dockbook_set_dock (dockbook, NULL);

  g_signal_handlers_disconnect_by_func (dockbook,
                                        ligma_dock_invalidate_description,
                                        dock);

  /* Ref the dockbook so we can emit the "book-removed" signal and
   * pass it as a parameter before it's destroyed
   */
  g_object_ref (dockbook);

  dock->p->dockbooks = g_list_remove (dock->p->dockbooks, dockbook);
  ligma_paned_box_remove_widget (LIGMA_PANED_BOX (dock->p->paned_vbox),
                                GTK_WIDGET (dockbook));

  ligma_dock_invalidate_description (dock);

  g_signal_emit (dock, dock_signals[BOOK_REMOVED], 0, dockbook);

  g_object_unref (dockbook);
}
