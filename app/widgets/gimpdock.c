/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdock.c
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"

#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimpdockseparator.h"
#include "gimppanedbox.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_DIALOG_FACTORY,
  PROP_UI_MANAGER
};

enum
{
  BOOK_ADDED,
  BOOK_REMOVED,
  TITLE_INVALIDATED,
  GEOMETRY_INVALIDATED,
  LAST_SIGNAL
};


struct _GimpDockPrivate
{
  GimpDialogFactory *dialog_factory;
  GimpContext       *context;
  GimpUIManager     *ui_manager;

  GtkWidget         *main_vbox;
  GtkWidget         *paned_vbox;

  GList             *dockbooks;
};


static void      gimp_dock_set_property      (GObject               *object,
                                              guint                  property_id,
                                              const GValue          *value,
                                              GParamSpec            *pspec);
static void      gimp_dock_get_property      (GObject               *object,
                                              guint                  property_id,
                                              GValue                *value,
                                              GParamSpec            *pspec);

static void      gimp_dock_destroy           (GtkObject             *object);

static void      gimp_dock_real_book_added   (GimpDock              *dock,
                                              GimpDockbook          *dockbook);
static void      gimp_dock_real_book_removed (GimpDock              *dock,
                                              GimpDockbook          *dockbook);
static gboolean  gimp_dock_dropped_cb        (GimpDockSeparator     *separator,
                                              GtkWidget             *source,
                                              gpointer               data);


G_DEFINE_TYPE (GimpDock, gimp_dock, GTK_TYPE_VBOX)

#define parent_class gimp_dock_parent_class

static guint dock_signals[LAST_SIGNAL] = { 0 };


static void
gimp_dock_class_init (GimpDockClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);

  dock_signals[BOOK_ADDED] =
    g_signal_new ("book-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockClass, book_added),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCKBOOK);

  dock_signals[BOOK_REMOVED] =
    g_signal_new ("book-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockClass, book_removed),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCKBOOK);

  dock_signals[TITLE_INVALIDATED] =
    g_signal_new ("title-invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockClass, title_invalidated),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  dock_signals[GEOMETRY_INVALIDATED] =
    g_signal_new ("geometry-invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockClass, geometry_invalidated),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->set_property     = gimp_dock_set_property;
  object_class->get_property     = gimp_dock_get_property;

  gtk_object_class->destroy      = gimp_dock_destroy;

  klass->setup                   = NULL;
  klass->set_host_geometry_hints = NULL;
  klass->book_added              = gimp_dock_real_book_added;
  klass->book_removed            = gimp_dock_real_book_removed;
  klass->title_invalidated       = NULL;
  klass->geometry_invalidated    = NULL;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DIALOG_FACTORY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_MANAGER,
                                   g_param_spec_object ("ui-manager",
                                                        NULL, NULL,
                                                        GIMP_TYPE_UI_MANAGER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpDockPrivate));
}

static void
gimp_dock_init (GimpDock *dock)
{
  dock->p = G_TYPE_INSTANCE_GET_PRIVATE (dock,
                                         GIMP_TYPE_DOCK,
                                         GimpDockPrivate);
  dock->p->context        = NULL;
  dock->p->dialog_factory = NULL;

  dock->p->main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dock), dock->p->main_vbox);
  gtk_widget_show (dock->p->main_vbox);

  dock->p->paned_vbox = gimp_paned_box_new (FALSE, 0, GTK_ORIENTATION_VERTICAL);
  gimp_paned_box_set_dropped_cb (GIMP_PANED_BOX (dock->p->paned_vbox),
                                 gimp_dock_dropped_cb,
                                 dock);
  gtk_container_add (GTK_CONTAINER (dock->p->main_vbox), dock->p->paned_vbox);
  gtk_widget_show (dock->p->paned_vbox);
}

static void
gimp_dock_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpDock *dock = GIMP_DOCK (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      dock->p->context = g_value_dup_object (value);
      break;

    case PROP_DIALOG_FACTORY:
      dock->p->dialog_factory = g_value_get_object (value);
      break;

    case PROP_UI_MANAGER:
      dock->p->ui_manager = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dock_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpDock *dock = GIMP_DOCK (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, dock->p->context);
      break;

    case PROP_DIALOG_FACTORY:
      g_value_set_object (value, dock->p->dialog_factory);
      break;

    case PROP_UI_MANAGER:
      g_value_set_object (value, dock->p->ui_manager);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dock_destroy (GtkObject *object)
{
  GimpDock *dock = GIMP_DOCK (object);

  while (dock->p->dockbooks)
    gimp_dock_remove_book (dock, GIMP_DOCKBOOK (dock->p->dockbooks->data));

  if (dock->p->ui_manager)
    {
      g_object_unref (dock->p->ui_manager);
      dock->p->ui_manager = NULL;
    }

  if (dock->p->context)
    {
      g_object_unref (dock->p->context);
      dock->p->context = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_dock_real_book_added (GimpDock     *dock,
                           GimpDockbook *dockbook)
{
}

static void
gimp_dock_real_book_removed (GimpDock     *dock,
                             GimpDockbook *dockbook)
{
}

static gboolean
gimp_dock_dropped_cb (GimpDockSeparator *separator,
                      GtkWidget         *source,
                      gpointer           data)
{
  GimpDock     *dock     = GIMP_DOCK (data);
  GimpDockable *dockable = NULL;
  GtkWidget    *dockbook = NULL;
  gint          index    = gimp_dock_separator_get_insert_pos (separator);

  if (GIMP_IS_DOCKABLE (source))
    dockable = GIMP_DOCKABLE (source);
  else
    dockable = g_object_get_data (G_OBJECT (source), "gimp-dockable");

  if (!dockable )
    return FALSE;

  g_object_set_data (G_OBJECT (dockable),
                     "gimp-dock-drag-widget", NULL);

  /*  if dropping to the same dock, take care that we don't try
   *  to reorder the *only* dockable in the dock
   */
  if (gimp_dockbook_get_dock (dockable->dockbook) == dock)
    {
      GList *children;
      gint   n_books;
      gint   n_dockables;

      n_books = g_list_length (gimp_dock_get_dockbooks (dock));

      children = gtk_container_get_children (GTK_CONTAINER (dockable->dockbook));
      n_dockables = g_list_length (children);
      g_list_free (children);

      if (n_books == 1 && n_dockables == 1)
        return TRUE; /* successfully do nothing */
    }

  /* Detach the dockable from the old dockbook */
  g_object_ref (dockable);
  gimp_dockbook_remove (dockable->dockbook, dockable);

  /* Create a new dockbook */
  dockbook = gimp_dockbook_new (gimp_dock_get_dialog_factory (dock)->menu_factory);
  gimp_dock_add_book (dock, GIMP_DOCKBOOK (dockbook), index);

  /* Add the dockable to new new dockbook */
  gimp_dockbook_add (GIMP_DOCKBOOK (dockbook), dockable, -1);
  g_object_unref (dockable);

  return TRUE;
}

/*  public functions  */

void
gimp_dock_setup (GimpDock       *dock,
                 const GimpDock *template)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GIMP_IS_DOCK (template));

  if (GIMP_DOCK_GET_CLASS (dock)->setup)
    GIMP_DOCK_GET_CLASS (dock)->setup (dock, template);
}

void
gimp_dock_set_aux_info (GimpDock *dock,
                        GList    *aux_info)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));

  if (GIMP_DOCK_GET_CLASS (dock)->set_aux_info)
    GIMP_DOCK_GET_CLASS (dock)->set_aux_info (dock, aux_info);
}

GList *
gimp_dock_get_aux_info (GimpDock *dock)
{
  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  if (GIMP_DOCK_GET_CLASS (dock)->get_aux_info)
    return GIMP_DOCK_GET_CLASS (dock)->get_aux_info (dock);

  return NULL;
}

gchar *
gimp_dock_get_title (GimpDock *dock)
{
  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  if (GIMP_DOCK_GET_CLASS (dock)->get_title)
    return GIMP_DOCK_GET_CLASS (dock)->get_title (dock);

  return NULL;
}

void
gimp_dock_invalidate_title (GimpDock *dock)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));

  g_signal_emit (dock, dock_signals[TITLE_INVALIDATED], 0);
}

/**
 * gimp_dock_set_host_geometry_hints:
 * @dock:   The dock
 * @window: The #GtkWindow to adapt to hosting the dock
 *
 * Some docks have some specific needs on the #GtkWindow they are
 * in. This function allows such docks to perform any such setup on
 * the #GtkWindow they are in/will be put in.
 **/
void
gimp_dock_set_host_geometry_hints (GimpDock  *dock,
                                   GtkWindow *window)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GTK_IS_WINDOW (window));

  if (GIMP_DOCK_GET_CLASS (dock)->set_host_geometry_hints)
    GIMP_DOCK_GET_CLASS (dock)->set_host_geometry_hints (dock, window);
}

/**
 * gimp_dock_invalidate_geometry:
 * @dock:
 *
 * Call when the dock needs to setup its host #GtkWindow with
 * GtkDock::set_host_geometry_hints().
 **/
void
gimp_dock_invalidate_geometry (GimpDock *dock)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));

  g_signal_emit (dock, dock_signals[GEOMETRY_INVALIDATED], 0);
}

GimpContext *
gimp_dock_get_context (GimpDock *dock)
{
  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  return dock->p->context;
}

GimpDialogFactory *
gimp_dock_get_dialog_factory (GimpDock *dock)
{
  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  return dock->p->dialog_factory;
}

GimpUIManager *
gimp_dock_get_ui_manager (GimpDock *dock)
{
  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  return dock->p->ui_manager;
}

GList *
gimp_dock_get_dockbooks (GimpDock *dock)
{
  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  return dock->p->dockbooks;
}

gint
gimp_dock_get_n_dockables (GimpDock *dock)
{
  GList *list = NULL;
  gint   n    = 0;

  g_return_val_if_fail (GIMP_IS_DOCK (dock), 0);

  for (list = dock->p->dockbooks; list; list = list->next)
    n += gtk_notebook_get_n_pages (GTK_NOTEBOOK (list->data));

  return n;
}

GtkWidget *
gimp_dock_get_main_vbox (GimpDock *dock)
{
  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  return dock->p->main_vbox;
}

GtkWidget *
gimp_dock_get_vbox (GimpDock *dock)
{
  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  return dock->p->paned_vbox;
}

void
gimp_dock_add (GimpDock     *dock,
               GimpDockable *dockable,
               gint          section,
               gint          position)
{
  GimpDockbook *dockbook;

  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (dockable->dockbook == NULL);

  dockbook = GIMP_DOCKBOOK (dock->p->dockbooks->data);

  gimp_dockbook_add (dockbook, dockable, position);
}

void
gimp_dock_remove (GimpDock     *dock,
                  GimpDockable *dockable)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (dockable->dockbook != NULL);
  g_return_if_fail (gimp_dockbook_get_dock (dockable->dockbook) == dock);

  gimp_dockbook_remove (dockable->dockbook, dockable);
}

void
gimp_dock_add_book (GimpDock     *dock,
                    GimpDockbook *dockbook,
                    gint          index)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));
  g_return_if_fail (gimp_dockbook_get_dock (dockbook) == NULL);

  gimp_dockbook_set_dock (dockbook, dock);

  dock->p->dockbooks = g_list_insert (dock->p->dockbooks, dockbook, index);
  gimp_paned_box_add_widget (GIMP_PANED_BOX (dock->p->paned_vbox),
                             GTK_WIDGET (dockbook),
                             index);
  gtk_widget_show (GTK_WIDGET (dockbook));

  g_signal_emit (dock, dock_signals[BOOK_ADDED], 0, dockbook);
}

void
gimp_dock_remove_book (GimpDock     *dock,
                       GimpDockbook *dockbook)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));
  g_return_if_fail (gimp_dockbook_get_dock (dockbook) == dock);

  gimp_dockbook_set_dock (dockbook, NULL);

  /* Ref the dockbook so we can emit the "book-removed" signal and
   * pass it as a parameter before it's destroyed
   */
  g_object_ref (dockbook);

  dock->p->dockbooks = g_list_remove (dock->p->dockbooks, dockbook);
  gimp_paned_box_remove_widget (GIMP_PANED_BOX (dock->p->paned_vbox),
                                GTK_WIDGET (dockbook));

  g_signal_emit (dock, dock_signals[BOOK_REMOVED], 0, dockbook);

  g_object_unref (dockbook);
}

