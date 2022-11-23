/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainerview.h
 * Copyright (C) 2001-2010 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTAINER_VIEW_H__
#define __LIGMA_CONTAINER_VIEW_H__


typedef enum
{
  LIGMA_CONTAINER_VIEW_PROP_0,
  LIGMA_CONTAINER_VIEW_PROP_CONTAINER,
  LIGMA_CONTAINER_VIEW_PROP_CONTEXT,
  LIGMA_CONTAINER_VIEW_PROP_SELECTION_MODE,
  LIGMA_CONTAINER_VIEW_PROP_REORDERABLE,
  LIGMA_CONTAINER_VIEW_PROP_VIEW_SIZE,
  LIGMA_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH,
  LIGMA_CONTAINER_VIEW_PROP_LAST = LIGMA_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH
} LigmaContainerViewProp;


#define LIGMA_TYPE_CONTAINER_VIEW (ligma_container_view_get_type ())
G_DECLARE_INTERFACE (LigmaContainerView, ligma_container_view, LIGMA, CONTAINER_VIEW, GtkWidget)


struct _LigmaContainerViewInterface
{
  GTypeInterface base_iface;

  /*  signals  */
  gboolean (* select_items)       (LigmaContainerView *view,
                                   GList             *items,
                                   GList             *paths);

  void     (* activate_item)      (LigmaContainerView *view,
                                   LigmaViewable      *object,
                                   gpointer           insert_data);

  /*  virtual functions  */
  void     (* set_container)      (LigmaContainerView *view,
                                   LigmaContainer     *container);
  void     (* set_context)        (LigmaContainerView *view,
                                   LigmaContext       *context);
  void     (* set_selection_mode) (LigmaContainerView *view,
                                   GtkSelectionMode   mode);

  gpointer (* insert_item)        (LigmaContainerView *view,
                                   LigmaViewable      *object,
                                   gpointer           parent_insert_data,
                                   gint               index);
  void     (* insert_items_after) (LigmaContainerView *view);
  void     (* remove_item)        (LigmaContainerView *view,
                                   LigmaViewable      *object,
                                   gpointer           insert_data);
  void     (* reorder_item)       (LigmaContainerView *view,
                                   LigmaViewable      *object,
                                   gint               new_index,
                                   gpointer           insert_data);
  void     (* rename_item)        (LigmaContainerView *view,
                                   LigmaViewable      *object,
                                   gpointer           insert_data);
  void     (* expand_item)        (LigmaContainerView *view,
                                   LigmaViewable      *object,
                                   gpointer           insert_data);
  void     (* clear_items)        (LigmaContainerView *view);
  void     (* set_view_size)      (LigmaContainerView *view);
  gint     (* get_selected)       (LigmaContainerView  *view,
                                   GList             **items,
                                   GList             **insert_data);


  /*  the destroy notifier for private->hash_table's values  */
  GDestroyNotify  insert_data_free;
  gboolean        model_is_tree;
};


LigmaContainer    * ligma_container_view_get_container      (LigmaContainerView  *view);
void               ligma_container_view_set_container      (LigmaContainerView  *view,
                                                           LigmaContainer      *container);

LigmaContext      * ligma_container_view_get_context        (LigmaContainerView  *view);
void               ligma_container_view_set_context        (LigmaContainerView  *view,
                                                           LigmaContext        *context);

GtkSelectionMode   ligma_container_view_get_selection_mode (LigmaContainerView  *view);
void               ligma_container_view_set_selection_mode (LigmaContainerView  *view,
                                                           GtkSelectionMode    mode);

gint               ligma_container_view_get_view_size      (LigmaContainerView  *view,
                                                           gint               *view_border_width);
void               ligma_container_view_set_view_size      (LigmaContainerView  *view,
                                                           gint                view_size,
                                                           gint                view_border_width);

gboolean           ligma_container_view_get_reorderable    (LigmaContainerView  *view);
void               ligma_container_view_set_reorderable    (LigmaContainerView  *view,
                                                           gboolean            reorderable);

GtkWidget        * ligma_container_view_get_dnd_widget     (LigmaContainerView  *view);
void               ligma_container_view_set_dnd_widget     (LigmaContainerView  *view,
                                                           GtkWidget          *dnd_widget);

void               ligma_container_view_enable_dnd         (LigmaContainerView  *editor,
                                                           GtkButton          *button,
                                                           GType               children_type);

gboolean           ligma_container_view_select_items       (LigmaContainerView  *view,
                                                           GList              *viewables);
gboolean           ligma_container_view_select_item        (LigmaContainerView  *view,
                                                           LigmaViewable       *viewable);
void               ligma_container_view_activate_item      (LigmaContainerView  *view,
                                                           LigmaViewable       *viewable);
gint               ligma_container_view_get_selected       (LigmaContainerView  *view,
                                                           GList             **items,
                                                           GList             **items_data);
gboolean           ligma_container_view_is_item_selected   (LigmaContainerView  *view,
                                                           LigmaViewable       *viewable);

/*  protected  */

gpointer           ligma_container_view_lookup             (LigmaContainerView  *view,
                                                           LigmaViewable       *viewable);
gboolean           ligma_container_view_contains           (LigmaContainerView *view,
                                                           GList             *viewables);


gboolean           ligma_container_view_item_selected      (LigmaContainerView  *view,
                                                           LigmaViewable       *item);
gboolean           ligma_container_view_multi_selected     (LigmaContainerView  *view,
                                                           GList              *items,
                                                           GList              *paths);
void               ligma_container_view_item_activated     (LigmaContainerView  *view,
                                                           LigmaViewable       *item);

/*  convenience functions  */

void               ligma_container_view_install_properties (GObjectClass       *klass);
void               ligma_container_view_set_property       (GObject            *object,
                                                           guint               property_id,
                                                           const GValue       *value,
                                                           GParamSpec         *pspec);
void               ligma_container_view_get_property       (GObject            *object,
                                                           guint               property_id,
                                                           GValue             *value,
                                                           GParamSpec         *pspec);

#endif  /*  __LIGMA_CONTAINER_VIEW_H__  */
