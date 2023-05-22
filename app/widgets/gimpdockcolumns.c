/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockcolumns.c
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

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "menus/menus.h"

#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimpdockcolumns.h"
#include "gimpmenudock.h"
#include "gimppanedbox.h"
#include "gimptoolbox.h"
#include "gimpuimanager.h"

#include "gimp-log.h"


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


struct _GimpDockColumnsPrivate
{
  GimpContext       *context;
  GimpDialogFactory *dialog_factory;
  GimpUIManager     *ui_manager;

  GList             *docks;

  GtkWidget         *paned_hbox;
};


static void      gimp_dock_columns_dispose           (GObject         *object);
static void      gimp_dock_columns_set_property      (GObject         *object,
                                                      guint            property_id,
                                                      const GValue    *value,
                                                      GParamSpec      *pspec);
static void      gimp_dock_columns_get_property      (GObject         *object,
                                                      guint            property_id,
                                                      GValue          *value,
                                                      GParamSpec      *pspec);
static gboolean  gimp_dock_columns_dropped_cb        (GtkWidget       *notebook,
                                                      GtkWidget       *child,
                                                      gint             insert_index,
                                                      gpointer         data);
static void      gimp_dock_columns_real_dock_added   (GimpDockColumns *dock_columns,
                                                      GimpDock        *dock);
static void      gimp_dock_columns_real_dock_removed (GimpDockColumns *dock_columns,
                                                      GimpDock        *dock);
static void      gimp_dock_columns_dock_book_removed (GimpDockColumns *dock_columns,
                                                      GimpDockbook    *dockbook,
                                                      GimpDock        *dock);


G_DEFINE_TYPE_WITH_PRIVATE (GimpDockColumns, gimp_dock_columns, GTK_TYPE_BOX)

#define parent_class gimp_dock_columns_parent_class

static guint dock_columns_signals[LAST_SIGNAL] = { 0 };


static void
gimp_dock_columns_class_init (GimpDockColumnsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gimp_dock_columns_dispose;
  object_class->set_property = gimp_dock_columns_set_property;
  object_class->get_property = gimp_dock_columns_get_property;

  klass->dock_added   = gimp_dock_columns_real_dock_added;
  klass->dock_removed = gimp_dock_columns_real_dock_removed;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DIALOG_FACTORY,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_MANAGER,
                                   g_param_spec_object ("ui-manager",
                                                        NULL, NULL,
                                                        GIMP_TYPE_UI_MANAGER,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  dock_columns_signals[DOCK_ADDED] =
    g_signal_new ("dock-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockColumnsClass, dock_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCK);

  dock_columns_signals[DOCK_REMOVED] =
    g_signal_new ("dock-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockColumnsClass, dock_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCK);
}

static void
gimp_dock_columns_init (GimpDockColumns *dock_columns)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (dock_columns),
                                  GTK_ORIENTATION_HORIZONTAL);

  dock_columns->p = gimp_dock_columns_get_instance_private (dock_columns);

  dock_columns->p->paned_hbox = gimp_paned_box_new (FALSE, 0,
                                                    GTK_ORIENTATION_HORIZONTAL);
  gimp_paned_box_set_dropped_cb (GIMP_PANED_BOX (dock_columns->p->paned_hbox),
                                 gimp_dock_columns_dropped_cb,
                                 dock_columns);
  gtk_box_pack_start (GTK_BOX (dock_columns), dock_columns->p->paned_hbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (dock_columns->p->paned_hbox);
}

static void
gimp_dock_columns_dispose (GObject *object)
{
  GimpDockColumns *dock_columns = GIMP_DOCK_COLUMNS (object);

  while (dock_columns->p->docks)
    {
      GimpDock *dock = dock_columns->p->docks->data;

      g_object_ref (dock);
      gimp_dock_columns_remove_dock (dock_columns, dock);
      gtk_widget_destroy (GTK_WIDGET (dock));
      g_object_unref (dock);
    }

  g_clear_weak_pointer (&dock_columns->p->context);
  g_clear_weak_pointer (&dock_columns->p->dialog_factory);
  g_clear_weak_pointer (&dock_columns->p->ui_manager);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_dock_columns_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpDockColumns *dock_columns = GIMP_DOCK_COLUMNS (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_set_weak_pointer (&dock_columns->p->context,
                          g_value_get_object (value));
      break;

    case PROP_DIALOG_FACTORY:
      g_set_weak_pointer (&dock_columns->p->dialog_factory,
                          g_value_get_object (value));
      break;

    case PROP_UI_MANAGER:
      g_set_weak_pointer (&dock_columns->p->ui_manager,
                          g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dock_columns_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpDockColumns *dock_columns = GIMP_DOCK_COLUMNS (object);

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
gimp_dock_columns_dropped_cb (GtkWidget *notebook,
                              GtkWidget *child,
                              gint       insert_index,
                              gpointer   data)
{
  GimpDockColumns *dock_columns = GIMP_DOCK_COLUMNS (data);
  GimpDockable    *dockable     = GIMP_DOCKABLE (child);
  GtkWidget       *new_dockbook = NULL;

  /* Create a new dock (including a new dockbook) */
  gimp_dock_columns_prepare_dockbook (dock_columns,
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
gimp_dock_columns_real_dock_added (GimpDockColumns *dock_columns,
                                   GimpDock        *dock)
{
}

static void
gimp_dock_columns_real_dock_removed (GimpDockColumns *dock_columns,
                                     GimpDock        *dock)
{
}

static void
gimp_dock_columns_dock_book_removed (GimpDockColumns *dock_columns,
                                     GimpDockbook    *dockbook,
                                     GimpDock        *dock)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));

  if (gimp_dock_get_dockbooks (dock) == NULL &&
      ! GIMP_IS_TOOLBOX (dock) &&
      gtk_widget_get_parent (GTK_WIDGET (dock)) != NULL)
    gimp_dock_columns_remove_dock (dock_columns, dock);
}


/**
 * gimp_dock_columns_new:
 * @context:
 *
 * Returns: A new #GimpDockColumns.
 **/
GtkWidget *
gimp_dock_columns_new (GimpContext       *context,
                       GimpDialogFactory *dialog_factory,
                       GimpUIManager     *ui_manager)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (ui_manager), NULL);

  return g_object_new (GIMP_TYPE_DOCK_COLUMNS,
                       "context",        context,
                       "dialog-factory", dialog_factory,
                       "ui-manager",     ui_manager,
                       NULL);
}

/**
 * gimp_dock_columns_add_dock:
 * @dock_columns:
 * @dock:
 *
 * Add a dock, added to a horizontal GimpPanedBox.
 **/
void
gimp_dock_columns_add_dock (GimpDockColumns *dock_columns,
                            GimpDock        *dock,
                            gint             index)
{
  g_return_if_fail (GIMP_IS_DOCK_COLUMNS (dock_columns));
  g_return_if_fail (GIMP_IS_DOCK (dock));

  GIMP_LOG (DND, "Adding GimpDock %p to GimpDockColumns %p", dock, dock_columns);

  dock_columns->p->docks = g_list_insert (dock_columns->p->docks, dock, index);

  gimp_dock_update_with_context (dock, dock_columns->p->context);

  gimp_paned_box_add_widget (GIMP_PANED_BOX (dock_columns->p->paned_hbox),
                             GTK_WIDGET (dock),
                             index);

  g_signal_connect_object (dock, "book-removed",
                           G_CALLBACK (gimp_dock_columns_dock_book_removed),
                           dock_columns,
                           G_CONNECT_SWAPPED);

  g_signal_emit (dock_columns, dock_columns_signals[DOCK_ADDED], 0, dock);
}

/**
 * gimp_dock_columns_prepare_dockbook:
 * @dock_columns:
 * @dock_index:
 * @dockbook_p:
 *
 * Create a new dock and add it to the dock columns with the given
 * dock_index insert index, then create and add a dockbook and put it
 * in the dock.
 **/
void
gimp_dock_columns_prepare_dockbook (GimpDockColumns  *dock_columns,
                                    gint              dock_index,
                                    GtkWidget       **dockbook_p)
{
  GimpDialogFactory *dialog_factory;
  GimpMenuFactory   *menu_factory;
  GtkWidget         *dock;
  GtkWidget         *dockbook;

  dock = gimp_menu_dock_new ();
  gimp_dock_columns_add_dock (dock_columns, GIMP_DOCK (dock), dock_index);

  dialog_factory = dock_columns->p->dialog_factory;
  menu_factory   = menus_get_global_menu_factory (gimp_dialog_factory_get_context (dialog_factory)->gimp);
  dockbook = gimp_dockbook_new (menu_factory);
  gimp_dock_add_book (GIMP_DOCK (dock), GIMP_DOCKBOOK (dockbook), -1);

  gtk_widget_show (GTK_WIDGET (dock));

  if (dockbook_p)
    *dockbook_p = dockbook;
}


void
gimp_dock_columns_remove_dock (GimpDockColumns *dock_columns,
                               GimpDock        *dock)
{
  g_return_if_fail (GIMP_IS_DOCK_COLUMNS (dock_columns));
  g_return_if_fail (GIMP_IS_DOCK (dock));

  GIMP_LOG (DND, "Removing GimpDock %p from GimpDockColumns %p", dock, dock_columns);

  dock_columns->p->docks = g_list_remove (dock_columns->p->docks, dock);

  gimp_dock_update_with_context (dock, NULL);

  g_signal_handlers_disconnect_by_func (dock,
                                        gimp_dock_columns_dock_book_removed,
                                        dock_columns);

  g_object_ref (dock);
  gimp_paned_box_remove_widget (GIMP_PANED_BOX (dock_columns->p->paned_hbox),
                                GTK_WIDGET (dock));

  g_signal_emit (dock_columns, dock_columns_signals[DOCK_REMOVED], 0, dock);
  g_object_unref (dock);
}

GList *
gimp_dock_columns_get_docks (GimpDockColumns *dock_columns)
{
  g_return_val_if_fail (GIMP_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->docks;
}

GimpContext *
gimp_dock_columns_get_context (GimpDockColumns *dock_columns)
{
  g_return_val_if_fail (GIMP_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->context;
}

void
gimp_dock_columns_set_context (GimpDockColumns *dock_columns,
                               GimpContext     *context)
{
  g_return_if_fail (GIMP_IS_DOCK_COLUMNS (dock_columns));

  dock_columns->p->context = context;
}

GimpDialogFactory *
gimp_dock_columns_get_dialog_factory (GimpDockColumns *dock_columns)
{
  g_return_val_if_fail (GIMP_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->dialog_factory;
}

GimpUIManager *
gimp_dock_columns_get_ui_manager (GimpDockColumns *dock_columns)
{
  g_return_val_if_fail (GIMP_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->ui_manager;
}
