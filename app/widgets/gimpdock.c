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

#include <gegl.h>
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
#include "gimpdockcolumns.h"
#include "gimpdockcontainer.h"
#include "gimpdockwindow.h"
#include "gimppanedbox.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define DEFAULT_DOCK_FONT_SCALE  PANGO_SCALE_SMALL


enum
{
  BOOK_ADDED,
  BOOK_REMOVED,
  DESCRIPTION_INVALIDATED,
  GEOMETRY_INVALIDATED,
  LAST_SIGNAL
};


struct _GimpDockPrivate
{
  GtkWidget         *temp_vbox;
  GtkWidget         *main_vbox;
  GtkWidget         *paned_vbox;

  GList             *dockbooks;

  gint               ID;
};


static void       gimp_dock_dispose                (GObject      *object);

static void       gimp_dock_style_updated          (GtkWidget    *widget);

static gchar    * gimp_dock_real_get_description   (GimpDock     *dock,
                                                    gboolean      complete);
static void       gimp_dock_real_book_added        (GimpDock     *dock,
                                                    GimpDockbook *dockbook);
static void       gimp_dock_real_book_removed      (GimpDock     *dock,
                                                    GimpDockbook *dockbook);
static void       gimp_dock_invalidate_description (GimpDock     *dock);
static gboolean   gimp_dock_dropped_cb             (GtkWidget    *source,
                                                    gint          insert_index,
                                                    gpointer      data);


G_DEFINE_TYPE (GimpDock, gimp_dock, GTK_TYPE_BOX)

#define parent_class gimp_dock_parent_class

static guint dock_signals[LAST_SIGNAL] = { 0 };


static void
gimp_dock_class_init (GimpDockClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

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

  dock_signals[DESCRIPTION_INVALIDATED] =
    g_signal_new ("description-invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockClass, description_invalidated),
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

  object_class->dispose          = gimp_dock_dispose;

  widget_class->style_updated    = gimp_dock_style_updated;

  klass->get_description         = gimp_dock_real_get_description;
  klass->set_host_geometry_hints = NULL;
  klass->book_added              = gimp_dock_real_book_added;
  klass->book_removed            = gimp_dock_real_book_removed;
  klass->description_invalidated = NULL;
  klass->geometry_invalidated    = NULL;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_double ("font-scale",
                                                                NULL, NULL,
                                                                0.0,
                                                                G_MAXDOUBLE,
                                                                DEFAULT_DOCK_FONT_SCALE,
                                                                GIMP_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (GimpDockPrivate));
}

static void
gimp_dock_init (GimpDock *dock)
{
  static gint  dock_ID = 1;
  gchar       *name    = NULL;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (dock),
                                  GTK_ORIENTATION_VERTICAL);

  dock->p = G_TYPE_INSTANCE_GET_PRIVATE (dock,
                                         GIMP_TYPE_DOCK,
                                         GimpDockPrivate);
  dock->p->ID             = dock_ID++;

  name = g_strdup_printf ("gimp-internal-dock-%d", dock->p->ID);
  gtk_widget_set_name (GTK_WIDGET (dock), name);
  g_free (name);

  dock->p->temp_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (dock), dock->p->temp_vbox, FALSE, FALSE, 0);
  /* Never show it */

  dock->p->main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (dock), dock->p->main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (dock->p->main_vbox);

  dock->p->paned_vbox = gimp_paned_box_new (FALSE, 0, GTK_ORIENTATION_VERTICAL);
  gimp_paned_box_set_dropped_cb (GIMP_PANED_BOX (dock->p->paned_vbox),
                                 gimp_dock_dropped_cb,
                                 dock);
  gtk_box_pack_start (GTK_BOX (dock->p->main_vbox), dock->p->paned_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (dock->p->paned_vbox);
}

static void
gimp_dock_dispose (GObject *object)
{
  GimpDock *dock = GIMP_DOCK (object);

  while (dock->p->dockbooks)
    {
      GimpDockbook *dockbook = dock->p->dockbooks->data;

      g_object_ref (dockbook);
      gimp_dock_remove_book (dock, dockbook);
      gtk_widget_destroy (GTK_WIDGET (dockbook));
      g_object_unref (dockbook);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_dock_style_updated (GtkWidget *widget)
{
  GimpDock *dock       = GIMP_DOCK (widget);
  gdouble   font_scale = 1.0;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
                        "font-scale", &font_scale,
                        NULL);

  if (font_scale != 1.0)
    {
      PangoContext         *context;
      PangoFontDescription *font_desc;
      gint                  font_size;
      gchar                *font_str;
      gchar                *rc_string;

      context = gtk_widget_get_pango_context (widget);
      font_desc = pango_context_get_font_description (context);
      font_desc = pango_font_description_copy (font_desc);

      font_size = pango_font_description_get_size (font_desc);
      font_size = font_scale * font_size;
      pango_font_description_set_size (font_desc, font_size);

      font_str = pango_font_description_to_string (font_desc);
      pango_font_description_free (font_desc);

      rc_string =
        g_strdup_printf ("style \"gimp-dock-style\""
                         "{"
                         "  font_name = \"%s\""
                         "}"
                         "widget \"*.gimp-internal-dock-%d.*\" style \"gimp-dock-style\"",
                         font_str,
                         dock->p->ID);
      g_free (font_str);

      gtk_rc_parse_string (rc_string);
      g_free (rc_string);

      gtk_widget_reset_rc_styles (widget);
    }
}

static gchar *
gimp_dock_real_get_description (GimpDock *dock,
                                gboolean  complete)
{
  GString *desc;
  GList   *list;

  desc = g_string_new (NULL);

  for (list = gimp_dock_get_dockbooks (dock);
       list;
       list = g_list_next (list))
    {
      GimpDockbook *dockbook = list->data;
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
          GimpDockable *dockable = child->data;

          g_string_append (desc, gimp_dockable_get_name (dockable));

          if (g_list_next (child))
            g_string_append (desc, GIMP_DOCK_DOCKABLE_SEPARATOR);
        }

      g_list_free (children);

      if (g_list_next (list))
        g_string_append (desc, GIMP_DOCK_BOOK_SEPARATOR);
    }

  return g_string_free (desc, FALSE);
}

static void
gimp_dock_real_book_added (GimpDock     *dock,
                           GimpDockbook *dockbook)
{
  g_signal_connect_object (dockbook, "switch-page",
                           G_CALLBACK (gimp_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);
}

static void
gimp_dock_real_book_removed (GimpDock     *dock,
                             GimpDockbook *dockbook)
{
  g_signal_handlers_disconnect_by_func (dockbook,
                                        gimp_dock_invalidate_description,
                                        dock);
}

static void
gimp_dock_invalidate_description (GimpDock *dock)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));

  g_signal_emit (dock, dock_signals[DESCRIPTION_INVALIDATED], 0);
}

static gboolean
gimp_dock_dropped_cb (GtkWidget *source,
                      gint       insert_index,
                      gpointer   data)
{
  GimpDock          *dock     = GIMP_DOCK (data);
  GimpDockable      *dockable = gimp_dockbook_drag_source_to_dockable (source);
  GimpDialogFactory *factory;
  GtkWidget         *dockbook = NULL;

  if (!dockable )
    return FALSE;

  /*  if dropping to the same dock, take care that we don't try
   *  to reorder the *only* dockable in the dock
   */
  if (gimp_dockbook_get_dock (gimp_dockable_get_dockbook (dockable)) == dock)
    {
      GList *children;
      gint   n_books;
      gint   n_dockables;

      n_books = g_list_length (gimp_dock_get_dockbooks (dock));

      children = gtk_container_get_children (GTK_CONTAINER (gimp_dockable_get_dockbook (dockable)));
      n_dockables = g_list_length (children);
      g_list_free (children);

      if (n_books == 1 && n_dockables == 1)
        return TRUE; /* successfully do nothing */
    }

  /* Detach the dockable from the old dockbook */
  g_object_ref (dockable);
  gimp_dockbook_remove (gimp_dockable_get_dockbook (dockable), dockable);

  /* Create a new dockbook */
  factory = gimp_dock_get_dialog_factory (dock);
  dockbook = gimp_dockbook_new (gimp_dialog_factory_get_menu_factory (factory));
  gimp_dock_add_book (dock, GIMP_DOCKBOOK (dockbook), insert_index);

  /* Add the dockable to new new dockbook */
  gimp_dockbook_add (GIMP_DOCKBOOK (dockbook), dockable, -1);
  g_object_unref (dockable);

  return TRUE;
}


/*  public functions  */

/**
 * gimp_dock_get_description:
 * @dock:
 * @complete: If %TRUE, only includes the active dockables, i.e. not the
 *            dockables in a non-active GtkNotebook tab
 *
 * Returns: A string describing the contents of the dock.
 **/
gchar *
gimp_dock_get_description (GimpDock *dock,
                           gboolean  complete)
{
  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  if (GIMP_DOCK_GET_CLASS (dock)->get_description)
    return GIMP_DOCK_GET_CLASS (dock)->get_description (dock, complete);

  return NULL;
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

/**
 * gimp_dock_update_with_context:
 * @dock:
 * @context:
 *
 * Set the @context on all dockables in the @dock.
 **/
void
gimp_dock_update_with_context (GimpDock    *dock,
                               GimpContext *context)
{
  GList *iter = NULL;

  for (iter = gimp_dock_get_dockbooks (dock);
       iter;
       iter = g_list_next (iter))
    {
      GimpDockbook *dockbook = GIMP_DOCKBOOK (iter->data);

      gimp_dockbook_update_with_context (dockbook, context);
    }
}

/**
 * gimp_dock_get_context:
 * @dock:
 *
 * Returns: The #GimpContext for the #GimpDockWindow the @dock is in.
 **/
GimpContext *
gimp_dock_get_context (GimpDock *dock)
{
  GimpContext *context = NULL;

  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  /* First try GimpDockColumns */
  if (! context)
    {
      GimpDockColumns *dock_columns;

      dock_columns =
        GIMP_DOCK_COLUMNS (gtk_widget_get_ancestor (GTK_WIDGET (dock),
                                                    GIMP_TYPE_DOCK_COLUMNS));

      if (dock_columns)
        context = gimp_dock_columns_get_context (dock_columns);
    }

  /* Then GimpDockWindow */
  if (! context)
    {
      GimpDockWindow *dock_window = gimp_dock_window_from_dock (dock);

      if (dock_window)
        context = gimp_dock_window_get_context (dock_window);
    }

  return context;
}

/**
 * gimp_dock_get_dialog_factory:
 * @dock:
 *
 * Returns: The #GimpDialogFactory for the #GimpDockWindow the @dock
 *          is in.
 **/
GimpDialogFactory *
gimp_dock_get_dialog_factory (GimpDock *dock)
{
  GimpDialogFactory *dialog_factory = NULL;

  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  /* First try GimpDockColumns */
  if (! dialog_factory)
    {
      GimpDockColumns *dock_columns;

      dock_columns =
        GIMP_DOCK_COLUMNS (gtk_widget_get_ancestor (GTK_WIDGET (dock),
                                                    GIMP_TYPE_DOCK_COLUMNS));

      if (dock_columns)
        dialog_factory = gimp_dock_columns_get_dialog_factory (dock_columns);
    }

  /* Then GimpDockWindow */
  if (! dialog_factory)
    {
      GimpDockWindow *dock_window = gimp_dock_window_from_dock (dock);

      if (dock_window)
        dialog_factory = gimp_dock_container_get_dialog_factory (GIMP_DOCK_CONTAINER (dock_window));
    }

  return dialog_factory;
}

/**
 * gimp_dock_get_ui_manager:
 * @dock:
 *
 * Returns: The #GimpUIManager for the #GimpDockWindow the @dock is
 *          in.
 **/
GimpUIManager *
gimp_dock_get_ui_manager (GimpDock *dock)
{
  GimpUIManager *ui_manager = NULL;

  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  /* First try GimpDockColumns */
  if (! ui_manager)
    {
      GimpDockColumns *dock_columns;

      dock_columns =
        GIMP_DOCK_COLUMNS (gtk_widget_get_ancestor (GTK_WIDGET (dock),
                                                    GIMP_TYPE_DOCK_COLUMNS));

      if (dock_columns)
        ui_manager = gimp_dock_columns_get_ui_manager (dock_columns);
    }

  /* Then GimpDockContainer */
  if (! ui_manager)
    {
      GimpDockWindow *dock_window = gimp_dock_window_from_dock (dock);

      if (dock_window)
        {
          GimpDockContainer *dock_container = GIMP_DOCK_CONTAINER (dock_window);

          ui_manager = gimp_dock_container_get_ui_manager (dock_container);
        }
    }

  return ui_manager;
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

gint
gimp_dock_get_id (GimpDock *dock)
{
  g_return_val_if_fail (GIMP_IS_DOCK (dock), 0);

  return dock->p->ID;
}

void
gimp_dock_set_id (GimpDock *dock,
                  gint      ID)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));

  dock->p->ID = ID;
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
  g_return_if_fail (gimp_dockable_get_dockbook (dockable) == NULL);

  dockbook = GIMP_DOCKBOOK (dock->p->dockbooks->data);

  gimp_dockbook_add (dockbook, dockable, position);
}

void
gimp_dock_remove (GimpDock     *dock,
                  GimpDockable *dockable)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (gimp_dockable_get_dockbook (dockable) != NULL);
  g_return_if_fail (gimp_dockbook_get_dock (gimp_dockable_get_dockbook (dockable)) == dock);

  gimp_dockbook_remove (gimp_dockable_get_dockbook (dockable), dockable);
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

  g_signal_connect_object (dockbook, "dockable-added",
                           G_CALLBACK (gimp_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);
  g_signal_connect_object (dockbook, "dockable-removed",
                           G_CALLBACK (gimp_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);
  g_signal_connect_object (dockbook, "dockable-reordered",
                           G_CALLBACK (gimp_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);

  dock->p->dockbooks = g_list_insert (dock->p->dockbooks, dockbook, index);
  gimp_paned_box_add_widget (GIMP_PANED_BOX (dock->p->paned_vbox),
                             GTK_WIDGET (dockbook),
                             index);
  gtk_widget_show (GTK_WIDGET (dockbook));

  gimp_dock_invalidate_description (dock);

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

  g_signal_handlers_disconnect_by_func (dockbook,
                                        gimp_dock_invalidate_description,
                                        dock);

  /* Ref the dockbook so we can emit the "book-removed" signal and
   * pass it as a parameter before it's destroyed
   */
  g_object_ref (dockbook);

  dock->p->dockbooks = g_list_remove (dock->p->dockbooks, dockbook);
  gimp_paned_box_remove_widget (GIMP_PANED_BOX (dock->p->paned_vbox),
                                GTK_WIDGET (dockbook));

  gimp_dock_invalidate_description (dock);

  g_signal_emit (dock, dock_signals[BOOK_REMOVED], 0, dockbook);

  g_object_unref (dockbook);
}

/**
 * gimp_dock_temp_add:
 * @dock:
 * @widget:
 *
 * Method to temporarily add a widget to the dock, for example to make
 * font-scale style property to be applied temporarily to the
 * child.
 **/
void
gimp_dock_temp_add (GimpDock  *dock,
                    GtkWidget *child)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GTK_IS_WIDGET (child));

  gtk_box_pack_start (GTK_BOX (dock->p->temp_vbox), child, FALSE, FALSE, 0);
}

/**
 * gimp_dock_temp_remove:
 * @dock:
 * @child:
 *
 * Removes a temporarly child added with gimp_dock_temp_add().
 **/
void
gimp_dock_temp_remove (GimpDock  *dock,
                       GtkWidget *child)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GTK_IS_WIDGET (child));

  gtk_container_remove (GTK_CONTAINER (dock->p->temp_vbox), child);
}
