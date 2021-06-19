/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerview.h
 * Copyright (C) 2001-2010 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_CONTAINER_VIEW_H__
#define __GIMP_CONTAINER_VIEW_H__


typedef enum
{
  GIMP_CONTAINER_VIEW_PROP_0,
  GIMP_CONTAINER_VIEW_PROP_CONTAINER,
  GIMP_CONTAINER_VIEW_PROP_CONTEXT,
  GIMP_CONTAINER_VIEW_PROP_SELECTION_MODE,
  GIMP_CONTAINER_VIEW_PROP_REORDERABLE,
  GIMP_CONTAINER_VIEW_PROP_VIEW_SIZE,
  GIMP_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH,
  GIMP_CONTAINER_VIEW_PROP_LAST = GIMP_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH
} GimpContainerViewProp;


#define GIMP_TYPE_CONTAINER_VIEW (gimp_container_view_get_type ())
G_DECLARE_INTERFACE (GimpContainerView, gimp_container_view, GIMP, CONTAINER_VIEW, GtkWidget)


struct _GimpContainerViewInterface
{
  GTypeInterface base_iface;

  /*  signals  */
  gboolean (* select_items)       (GimpContainerView *view,
                                   GList             *items,
                                   GList             *paths);

  void     (* activate_item)      (GimpContainerView *view,
                                   GimpViewable      *object,
                                   gpointer           insert_data);

  /*  virtual functions  */
  void     (* set_container)      (GimpContainerView *view,
                                   GimpContainer     *container);
  void     (* set_context)        (GimpContainerView *view,
                                   GimpContext       *context);
  void     (* set_selection_mode) (GimpContainerView *view,
                                   GtkSelectionMode   mode);

  gpointer (* insert_item)        (GimpContainerView *view,
                                   GimpViewable      *object,
                                   gpointer           parent_insert_data,
                                   gint               index);
  void     (* insert_items_after) (GimpContainerView *view);
  void     (* remove_item)        (GimpContainerView *view,
                                   GimpViewable      *object,
                                   gpointer           insert_data);
  void     (* reorder_item)       (GimpContainerView *view,
                                   GimpViewable      *object,
                                   gint               new_index,
                                   gpointer           insert_data);
  void     (* rename_item)        (GimpContainerView *view,
                                   GimpViewable      *object,
                                   gpointer           insert_data);
  void     (* expand_item)        (GimpContainerView *view,
                                   GimpViewable      *object,
                                   gpointer           insert_data);
  void     (* clear_items)        (GimpContainerView *view);
  void     (* set_view_size)      (GimpContainerView *view);
  gint     (* get_selected)       (GimpContainerView  *view,
                                   GList             **items,
                                   GList             **insert_data);


  /*  the destroy notifier for private->hash_table's values  */
  GDestroyNotify  insert_data_free;
  gboolean        model_is_tree;
};


GimpContainer    * gimp_container_view_get_container      (GimpContainerView  *view);
void               gimp_container_view_set_container      (GimpContainerView  *view,
                                                           GimpContainer      *container);

GimpContext      * gimp_container_view_get_context        (GimpContainerView  *view);
void               gimp_container_view_set_context        (GimpContainerView  *view,
                                                           GimpContext        *context);

GtkSelectionMode   gimp_container_view_get_selection_mode (GimpContainerView  *view);
void               gimp_container_view_set_selection_mode (GimpContainerView  *view,
                                                           GtkSelectionMode    mode);

gint               gimp_container_view_get_view_size      (GimpContainerView  *view,
                                                           gint               *view_border_width);
void               gimp_container_view_set_view_size      (GimpContainerView  *view,
                                                           gint                view_size,
                                                           gint                view_border_width);

gboolean           gimp_container_view_get_reorderable    (GimpContainerView  *view);
void               gimp_container_view_set_reorderable    (GimpContainerView  *view,
                                                           gboolean            reorderable);

GtkWidget        * gimp_container_view_get_dnd_widget     (GimpContainerView  *view);
void               gimp_container_view_set_dnd_widget     (GimpContainerView  *view,
                                                           GtkWidget          *dnd_widget);

void               gimp_container_view_enable_dnd         (GimpContainerView  *editor,
                                                           GtkButton          *button,
                                                           GType               children_type);

gboolean           gimp_container_view_select_items       (GimpContainerView  *view,
                                                           GList              *viewables);
gboolean           gimp_container_view_select_item        (GimpContainerView  *view,
                                                           GimpViewable       *viewable);
void               gimp_container_view_activate_item      (GimpContainerView  *view,
                                                           GimpViewable       *viewable);
gint               gimp_container_view_get_selected       (GimpContainerView  *view,
                                                           GList             **items,
                                                           GList             **items_data);
gboolean           gimp_container_view_is_item_selected   (GimpContainerView  *view,
                                                           GimpViewable       *viewable);

/*  protected  */

gpointer           gimp_container_view_lookup             (GimpContainerView  *view,
                                                           GimpViewable       *viewable);
gboolean           gimp_container_view_contains           (GimpContainerView *view,
                                                           GList             *viewables);


gboolean           gimp_container_view_item_selected      (GimpContainerView  *view,
                                                           GimpViewable       *item);
gboolean           gimp_container_view_multi_selected     (GimpContainerView  *view,
                                                           GList              *items,
                                                           GList              *paths);
void               gimp_container_view_item_activated     (GimpContainerView  *view,
                                                           GimpViewable       *item);

/*  convenience functions  */

void               gimp_container_view_install_properties (GObjectClass       *klass);
void               gimp_container_view_set_property       (GObject            *object,
                                                           guint               property_id,
                                                           const GValue       *value,
                                                           GParamSpec         *pspec);
void               gimp_container_view_get_property       (GObject            *object,
                                                           guint               property_id,
                                                           GValue             *value,
                                                           GParamSpec         *pspec);

#endif  /*  __GIMP_CONTAINER_VIEW_H__  */
