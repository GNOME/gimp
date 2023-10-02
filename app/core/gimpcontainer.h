/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpcontainer.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_CONTAINER_H__
#define __GIMP_CONTAINER_H__


#include "gimpobject.h"


#define GIMP_TYPE_CONTAINER            (gimp_container_get_type ())
#define GIMP_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER, GimpContainer))
#define GIMP_CONTAINER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER, GimpContainerClass))
#define GIMP_IS_CONTAINER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER))
#define GIMP_IS_CONTAINER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER))
#define GIMP_CONTAINER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER, GimpContainerClass))


typedef gboolean (* GimpContainerSearchFunc) (GimpObject *object,
                                              gpointer    user_data);


typedef struct _GimpContainerClass   GimpContainerClass;
typedef struct _GimpContainerPrivate GimpContainerPrivate;

struct _GimpContainer
{
  GimpObject            parent_instance;

  GimpContainerPrivate *priv;
};

struct _GimpContainerClass
{
  GimpObjectClass  parent_class;

  /*  signals  */
  void         (* add)                (GimpContainer           *container,
                                       GimpObject              *object);
  void         (* remove)             (GimpContainer           *container,
                                       GimpObject              *object);
  void         (* reorder)            (GimpContainer           *container,
                                       GimpObject              *object,
                                       gint                     new_index);
  void         (* freeze)             (GimpContainer           *container);
  void         (* thaw)               (GimpContainer           *container);

  /*  virtual functions  */
  void         (* clear)              (GimpContainer           *container);
  gboolean     (* have)               (GimpContainer           *container,
                                       GimpObject              *object);
  void         (* foreach)            (GimpContainer           *container,
                                       GFunc                    func,
                                       gpointer                 user_data);
  GimpObject * (* search)             (GimpContainer           *container,
                                       GimpContainerSearchFunc  func,
                                       gpointer                 user_data);
  gboolean     (* get_unique_names)   (GimpContainer           *container);
  GimpObject * (* get_child_by_name)  (GimpContainer           *container,
                                       const gchar             *name);
  GList    * (* get_children_by_name) (GimpContainer           *container,
                                       const gchar             *name);
  GimpObject * (* get_child_by_index) (GimpContainer           *container,
                                       gint                     index);
  gint         (* get_child_index)    (GimpContainer           *container,
                                       GimpObject              *object);
};


GType        gimp_container_get_type           (void) G_GNUC_CONST;

GType        gimp_container_get_children_type  (GimpContainer           *container);
GimpContainerPolicy gimp_container_get_policy  (GimpContainer           *container);
gint         gimp_container_get_n_children     (GimpContainer           *container);

gboolean     gimp_container_add                (GimpContainer           *container,
                                                GimpObject              *object);
gboolean     gimp_container_remove             (GimpContainer           *container,
                                                GimpObject              *object);
gboolean     gimp_container_insert             (GimpContainer           *container,
                                                GimpObject              *object,
                                                gint                     new_index);
gboolean     gimp_container_reorder            (GimpContainer           *container,
                                                GimpObject              *object,
                                                gint                     new_index);

void         gimp_container_freeze             (GimpContainer           *container);
void         gimp_container_thaw               (GimpContainer           *container);
gboolean     gimp_container_frozen             (GimpContainer           *container);
gint         gimp_container_freeze_count       (GimpContainer           *container);

void         gimp_container_clear              (GimpContainer           *container);
gboolean     gimp_container_is_empty           (GimpContainer           *container);
gboolean     gimp_container_have               (GimpContainer           *container,
                                                GimpObject              *object);
void         gimp_container_foreach            (GimpContainer           *container,
                                                GFunc                    func,
                                                gpointer                 user_data);
GimpObject * gimp_container_search             (GimpContainer           *container,
                                                GimpContainerSearchFunc  func,
                                                gpointer                 user_data);

gboolean     gimp_container_get_unique_names   (GimpContainer           *container);

GList    * gimp_container_get_children_by_name (GimpContainer           *container,
                                                const gchar             *name);
GimpObject * gimp_container_get_child_by_name  (GimpContainer           *container,
                                                const gchar             *name);
GimpObject * gimp_container_get_child_by_index (GimpContainer           *container,
                                                gint                     index);
GimpObject * gimp_container_get_first_child    (GimpContainer           *container);
GimpObject * gimp_container_get_last_child     (GimpContainer           *container);
gint         gimp_container_get_child_index    (GimpContainer           *container,
                                                GimpObject              *object);

GimpObject * gimp_container_get_neighbor_of    (GimpContainer           *container,
                                                GimpObject              *object);

gchar     ** gimp_container_get_name_array     (GimpContainer           *container);

GQuark       gimp_container_add_handler        (GimpContainer           *container,
                                                const gchar             *signame,
                                                GCallback                callback,
                                                gpointer                 callback_data);
void         gimp_container_remove_handler     (GimpContainer           *container,
                                                GQuark                   id);
void         gimp_container_remove_handlers_by_func
                                               (GimpContainer           *container,
                                                GCallback                callback,
                                                gpointer                 callback_data);
void         gimp_container_remove_handlers_by_data
                                               (GimpContainer           *container,
                                                gpointer                 callback_data);


#endif  /* __GIMP_CONTAINER_H__ */
