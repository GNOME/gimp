/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadockcolumns.c
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#include "libligmabase/ligmabase.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "ligmadialogfactory.h"
#include "ligmadock.h"
#include "ligmadockable.h"
#include "ligmadockbook.h"
#include "ligmadockcolumns.h"
#include "ligmamenudock.h"
#include "ligmapanedbox.h"
#include "ligmatoolbox.h"
#include "ligmauimanager.h"

#include "ligma-log.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_DIALOG_FACTORY,
  PROP_UI_MANAGER
};

enum
{
  DOCK_ADDED,
  DOCK_REMOVED,
  LAST_SIGNAL
};


struct _LigmaDockColumnsPrivate
{
  LigmaContext       *context;
  LigmaDialogFactory *dialog_factory;
  LigmaUIManager     *ui_manager;

  GList             *docks;

  GtkWidget         *paned_hbox;
};


static void      ligma_dock_columns_dispose           (GObject         *object);
static void      ligma_dock_columns_set_property      (GObject         *object,
                                                      guint            property_id,
                                                      const GValue    *value,
                                                      GParamSpec      *pspec);
static void      ligma_dock_columns_get_property      (GObject         *object,
                                                      guint            property_id,
                                                      GValue          *value,
                                                      GParamSpec      *pspec);
static gboolean  ligma_dock_columns_dropped_cb        (GtkWidget       *notebook,
                                                      GtkWidget       *child,
                                                      gint             insert_index,
                                                      gpointer         data);
static void      ligma_dock_columns_real_dock_added   (LigmaDockColumns *dock_columns,
                                                      LigmaDock        *dock);
static void      ligma_dock_columns_real_dock_removed (LigmaDockColumns *dock_columns,
                                                      LigmaDock        *dock);
static void      ligma_dock_columns_dock_book_removed (LigmaDockColumns *dock_columns,
                                                      LigmaDockbook    *dockbook,
                                                      LigmaDock        *dock);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaDockColumns, ligma_dock_columns, GTK_TYPE_BOX)

#define parent_class ligma_dock_columns_parent_class

static guint dock_columns_signals[LAST_SIGNAL] = { 0 };


static void
ligma_dock_columns_class_init (LigmaDockColumnsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = ligma_dock_columns_dispose;
  object_class->set_property = ligma_dock_columns_set_property;
  object_class->get_property = ligma_dock_columns_get_property;

  klass->dock_added   = ligma_dock_columns_real_dock_added;
  klass->dock_removed = ligma_dock_columns_real_dock_removed;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_DIALOG_FACTORY,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_MANAGER,
                                   g_param_spec_object ("ui-manager",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_UI_MANAGER,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  dock_columns_signals[DOCK_ADDED] =
    g_signal_new ("dock-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDockColumnsClass, dock_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DOCK);

  dock_columns_signals[DOCK_REMOVED] =
    g_signal_new ("dock-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDockColumnsClass, dock_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DOCK);
}

static void
ligma_dock_columns_init (LigmaDockColumns *dock_columns)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (dock_columns),
                                  GTK_ORIENTATION_HORIZONTAL);

  dock_columns->p = ligma_dock_columns_get_instance_private (dock_columns);

  dock_columns->p->paned_hbox = ligma_paned_box_new (FALSE, 0,
                                                    GTK_ORIENTATION_HORIZONTAL);
  ligma_paned_box_set_dropped_cb (LIGMA_PANED_BOX (dock_columns->p->paned_hbox),
                                 ligma_dock_columns_dropped_cb,
                                 dock_columns);
  gtk_box_pack_start (GTK_BOX (dock_columns), dock_columns->p->paned_hbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (dock_columns->p->paned_hbox);
}

static void
ligma_dock_columns_dispose (GObject *object)
{
  LigmaDockColumns *dock_columns = LIGMA_DOCK_COLUMNS (object);

  while (dock_columns->p->docks)
    {
      LigmaDock *dock = dock_columns->p->docks->data;

      g_object_ref (dock);
      ligma_dock_columns_remove_dock (dock_columns, dock);
      gtk_widget_destroy (GTK_WIDGET (dock));
      g_object_unref (dock);
    }

  if (dock_columns->p->context)
    {
      g_object_remove_weak_pointer (G_OBJECT (dock_columns->p->context),
                                    (gpointer) &dock_columns->p->context);
      dock_columns->p->context = NULL;
    }

  if (dock_columns->p->dialog_factory)
    {
      g_object_remove_weak_pointer (G_OBJECT (dock_columns->p->dialog_factory),
                                    (gpointer) &dock_columns->p->dialog_factory);
      dock_columns->p->dialog_factory = NULL;
    }

  if (dock_columns->p->ui_manager)
    {
      g_object_remove_weak_pointer (G_OBJECT (dock_columns->p->ui_manager),
                                    (gpointer)&dock_columns->p->ui_manager);
      dock_columns->p->ui_manager = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_dock_columns_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaDockColumns *dock_columns = LIGMA_DOCK_COLUMNS (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      if (dock_columns->p->context)
        g_object_remove_weak_pointer (G_OBJECT (dock_columns->p->context),
                                      (gpointer) &dock_columns->p->context);
      dock_columns->p->context = g_value_get_object (value);
      if (dock_columns->p->context)
        g_object_add_weak_pointer (G_OBJECT (dock_columns->p->context),
                                   (gpointer) &dock_columns->p->context);
      break;

    case PROP_DIALOG_FACTORY:
      if (dock_columns->p->dialog_factory)
        g_object_remove_weak_pointer (G_OBJECT (dock_columns->p->dialog_factory),
                                      (gpointer) &dock_columns->p->dialog_factory);
      dock_columns->p->dialog_factory = g_value_get_object (value);
      if (dock_columns->p->dialog_factory)
        g_object_add_weak_pointer (G_OBJECT (dock_columns->p->dialog_factory),
                                   (gpointer) &dock_columns->p->dialog_factory);
      break;

    case PROP_UI_MANAGER:
      if (dock_columns->p->ui_manager)
        g_object_remove_weak_pointer (G_OBJECT (dock_columns->p->ui_manager),
                                      (gpointer) &dock_columns->p->ui_manager);
      dock_columns->p->ui_manager = g_value_get_object (value);
      if (dock_columns->p->ui_manager)
        g_object_add_weak_pointer (G_OBJECT (dock_columns->p->ui_manager),
                                   (gpointer) &dock_columns->p->ui_manager);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_dock_columns_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaDockColumns *dock_columns = LIGMA_DOCK_COLUMNS (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, dock_columns->p->context);
      break;
    case PROP_DIALOG_FACTORY:
      g_value_set_object (value, dock_columns->p->dialog_factory);
      break;
    case PROP_UI_MANAGER:
      g_value_set_object (value, dock_columns->p->ui_manager);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_dock_columns_dropped_cb (GtkWidget *notebook,
                              GtkWidget *child,
                              gint       insert_index,
                              gpointer   data)
{
  LigmaDockColumns *dock_columns = LIGMA_DOCK_COLUMNS (data);
  LigmaDockable    *dockable     = LIGMA_DOCKABLE (child);
  GtkWidget       *new_dockbook = NULL;

  /* Create a new dock (including a new dockbook) */
  ligma_dock_columns_prepare_dockbook (dock_columns,
                                      insert_index,
                                      &new_dockbook);

  /* Move the dockable to the new dockbook */
  g_object_ref (new_dockbook);
  g_object_ref (dockable);

  gtk_notebook_detach_tab (GTK_NOTEBOOK (notebook), child);
  gtk_notebook_append_page (GTK_NOTEBOOK (new_dockbook), child, NULL);

  g_object_unref (dockable);
  g_object_unref (new_dockbook);

  return TRUE;
}

static void
ligma_dock_columns_real_dock_added (LigmaDockColumns *dock_columns,
                                   LigmaDock        *dock)
{
}

static void
ligma_dock_columns_real_dock_removed (LigmaDockColumns *dock_columns,
                                     LigmaDock        *dock)
{
}

static void
ligma_dock_columns_dock_book_removed (LigmaDockColumns *dock_columns,
                                     LigmaDockbook    *dockbook,
                                     LigmaDock        *dock)
{
  g_return_if_fail (LIGMA_IS_DOCK (dock));

  if (ligma_dock_get_dockbooks (dock) == NULL &&
      ! LIGMA_IS_TOOLBOX (dock) &&
      gtk_widget_get_parent (GTK_WIDGET (dock)) != NULL)
    ligma_dock_columns_remove_dock (dock_columns, dock);
}


/**
 * ligma_dock_columns_new:
 * @context:
 *
 * Returns: A new #LigmaDockColumns.
 **/
GtkWidget *
ligma_dock_columns_new (LigmaContext       *context,
                       LigmaDialogFactory *dialog_factory,
                       LigmaUIManager     *ui_manager)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (LIGMA_IS_UI_MANAGER (ui_manager), NULL);

  return g_object_new (LIGMA_TYPE_DOCK_COLUMNS,
                       "context",        context,
                       "dialog-factory", dialog_factory,
                       "ui-manager",     ui_manager,
                       NULL);
}

/**
 * ligma_dock_columns_add_dock:
 * @dock_columns:
 * @dock:
 *
 * Add a dock, added to a horizontal LigmaPanedBox.
 **/
void
ligma_dock_columns_add_dock (LigmaDockColumns *dock_columns,
                            LigmaDock        *dock,
                            gint             index)
{
  g_return_if_fail (LIGMA_IS_DOCK_COLUMNS (dock_columns));
  g_return_if_fail (LIGMA_IS_DOCK (dock));

  LIGMA_LOG (DND, "Adding LigmaDock %p to LigmaDockColumns %p", dock, dock_columns);

  dock_columns->p->docks = g_list_insert (dock_columns->p->docks, dock, index);

  ligma_dock_update_with_context (dock, dock_columns->p->context);

  ligma_paned_box_add_widget (LIGMA_PANED_BOX (dock_columns->p->paned_hbox),
                             GTK_WIDGET (dock),
                             index);

  g_signal_connect_object (dock, "book-removed",
                           G_CALLBACK (ligma_dock_columns_dock_book_removed),
                           dock_columns,
                           G_CONNECT_SWAPPED);

  g_signal_emit (dock_columns, dock_columns_signals[DOCK_ADDED], 0, dock);
}

/**
 * ligma_dock_columns_prepare_dockbook:
 * @dock_columns:
 * @dock_index:
 * @dockbook_p:
 *
 * Create a new dock and add it to the dock columns with the given
 * dock_index insert index, then create and add a dockbook and put it
 * in the dock.
 **/
void
ligma_dock_columns_prepare_dockbook (LigmaDockColumns  *dock_columns,
                                    gint              dock_index,
                                    GtkWidget       **dockbook_p)
{
  LigmaMenuFactory *menu_factory;
  GtkWidget       *dock;
  GtkWidget       *dockbook;

  dock = ligma_menu_dock_new ();
  ligma_dock_columns_add_dock (dock_columns, LIGMA_DOCK (dock), dock_index);

  menu_factory = ligma_dialog_factory_get_menu_factory (dock_columns->p->dialog_factory);
  dockbook = ligma_dockbook_new (menu_factory);
  ligma_dock_add_book (LIGMA_DOCK (dock), LIGMA_DOCKBOOK (dockbook), -1);

  gtk_widget_show (GTK_WIDGET (dock));

  if (dockbook_p)
    *dockbook_p = dockbook;
}


void
ligma_dock_columns_remove_dock (LigmaDockColumns *dock_columns,
                               LigmaDock        *dock)
{
  g_return_if_fail (LIGMA_IS_DOCK_COLUMNS (dock_columns));
  g_return_if_fail (LIGMA_IS_DOCK (dock));

  LIGMA_LOG (DND, "Removing LigmaDock %p from LigmaDockColumns %p", dock, dock_columns);

  dock_columns->p->docks = g_list_remove (dock_columns->p->docks, dock);

  ligma_dock_update_with_context (dock, NULL);

  g_signal_handlers_disconnect_by_func (dock,
                                        ligma_dock_columns_dock_book_removed,
                                        dock_columns);

  g_object_ref (dock);
  ligma_paned_box_remove_widget (LIGMA_PANED_BOX (dock_columns->p->paned_hbox),
                                GTK_WIDGET (dock));

  g_signal_emit (dock_columns, dock_columns_signals[DOCK_REMOVED], 0, dock);
  g_object_unref (dock);
}

GList *
ligma_dock_columns_get_docks (LigmaDockColumns *dock_columns)
{
  g_return_val_if_fail (LIGMA_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->docks;
}

LigmaContext *
ligma_dock_columns_get_context (LigmaDockColumns *dock_columns)
{
  g_return_val_if_fail (LIGMA_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->context;
}

void
ligma_dock_columns_set_context (LigmaDockColumns *dock_columns,
                               LigmaContext     *context)
{
  g_return_if_fail (LIGMA_IS_DOCK_COLUMNS (dock_columns));

  dock_columns->p->context = context;
}

LigmaDialogFactory *
ligma_dock_columns_get_dialog_factory (LigmaDockColumns *dock_columns)
{
  g_return_val_if_fail (LIGMA_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->dialog_factory;
}

LigmaUIManager *
ligma_dock_columns_get_ui_manager (LigmaDockColumns *dock_columns)
{
  g_return_val_if_fail (LIGMA_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->ui_manager;
}
