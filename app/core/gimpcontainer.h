/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmacontainer.h
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTAINER_H__
#define __LIGMA_CONTAINER_H__


#include "ligmaobject.h"


#define LIGMA_TYPE_CONTAINER            (ligma_container_get_type ())
#define LIGMA_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTAINER, LigmaContainer))
#define LIGMA_CONTAINER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTAINER, LigmaContainerClass))
#define LIGMA_IS_CONTAINER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTAINER))
#define LIGMA_IS_CONTAINER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTAINER))
#define LIGMA_CONTAINER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTAINER, LigmaContainerClass))


typedef gboolean (* LigmaContainerSearchFunc) (LigmaObject *object,
                                              gpointer    user_data);


typedef struct _LigmaContainerClass   LigmaContainerClass;
typedef struct _LigmaContainerPrivate LigmaContainerPrivate;

struct _LigmaContainer
{
  LigmaObject            parent_instance;

  LigmaContainerPrivate *priv;
};

struct _LigmaContainerClass
{
  LigmaObjectClass  parent_class;

  /*  signals  */
  void         (* add)                (LigmaContainer           *container,
                                       LigmaObject              *object);
  void         (* remove)             (LigmaContainer           *container,
                                       LigmaObject              *object);
  void         (* reorder)            (LigmaContainer           *container,
                                       LigmaObject              *object,
                                       gint                     new_index);
  void         (* freeze)             (LigmaContainer           *container);
  void         (* thaw)               (LigmaContainer           *container);

  /*  virtual functions  */
  void         (* clear)              (LigmaContainer           *container);
  gboolean     (* have)               (LigmaContainer           *container,
                                       LigmaObject              *object);
  void         (* foreach)            (LigmaContainer           *container,
                                       GFunc                    func,
                                       gpointer                 user_data);
  LigmaObject * (* search)             (LigmaContainer           *container,
                                       LigmaContainerSearchFunc  func,
                                       gpointer                 user_data);
  gboolean     (* get_unique_names)   (LigmaContainer           *container);
  LigmaObject * (* get_child_by_name)  (LigmaContainer           *container,
                                       const gchar             *name);
  LigmaObject * (* get_child_by_index) (LigmaContainer           *container,
                                       gint                     index);
  gint         (* get_child_index)    (LigmaContainer           *container,
                                       LigmaObject              *object);
};


GType        ligma_container_get_type           (void) G_GNUC_CONST;

GType        ligma_container_get_children_type  (LigmaContainer           *container);
LigmaContainerPolicy ligma_container_get_policy  (LigmaContainer           *container);
gint         ligma_container_get_n_children     (LigmaContainer           *container);

gboolean     ligma_container_add                (LigmaContainer           *container,
                                                LigmaObject              *object);
gboolean     ligma_container_remove             (LigmaContainer           *container,
                                                LigmaObject              *object);
gboolean     ligma_container_insert             (LigmaContainer           *container,
                                                LigmaObject              *object,
                                                gint                     new_index);
gboolean     ligma_container_reorder            (LigmaContainer           *container,
                                                LigmaObject              *object,
                                                gint                     new_index);

void         ligma_container_freeze             (LigmaContainer           *container);
void         ligma_container_thaw               (LigmaContainer           *container);
gboolean     ligma_container_frozen             (LigmaContainer           *container);
gint         ligma_container_freeze_count       (LigmaContainer           *container);

void         ligma_container_clear              (LigmaContainer           *container);
gboolean     ligma_container_is_empty           (LigmaContainer           *container);
gboolean     ligma_container_have               (LigmaContainer           *container,
                                                LigmaObject              *object);
void         ligma_container_foreach            (LigmaContainer           *container,
                                                GFunc                    func,
                                                gpointer                 user_data);
LigmaObject * ligma_container_search             (LigmaContainer           *container,
                                                LigmaContainerSearchFunc  func,
                                                gpointer                 user_data);

gboolean     ligma_container_get_unique_names   (LigmaContainer           *container);

LigmaObject * ligma_container_get_child_by_name  (LigmaContainer           *container,
                                                const gchar             *name);
LigmaObject * ligma_container_get_child_by_index (LigmaContainer           *container,
                                                gint                     index);
LigmaObject * ligma_container_get_first_child    (LigmaContainer           *container);
LigmaObject * ligma_container_get_last_child     (LigmaContainer           *container);
gint         ligma_container_get_child_index    (LigmaContainer           *container,
                                                LigmaObject              *object);

LigmaObject * ligma_container_get_neighbor_of    (LigmaContainer           *container,
                                                LigmaObject              *object);

gchar     ** ligma_container_get_name_array     (LigmaContainer           *container);

GQuark       ligma_container_add_handler        (LigmaContainer           *container,
                                                const gchar             *signame,
                                                GCallback                callback,
                                                gpointer                 callback_data);
void         ligma_container_remove_handler     (LigmaContainer           *container,
                                                GQuark                   id);
void         ligma_container_remove_handlers_by_func
                                               (LigmaContainer           *container,
                                                GCallback                callback,
                                                gpointer                 callback_data);
void         ligma_container_remove_handlers_by_data
                                               (LigmaContainer           *container,
                                                gpointer                 callback_data);


#endif  /* __LIGMA_CONTAINER_H__ */
