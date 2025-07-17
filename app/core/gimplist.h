/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimplist.h
 * Copyright (C) 2001-2016 Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "gimpcontainer.h"


#define GIMP_TYPE_LIST            (gimp_list_get_type ())
#define GIMP_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LIST, GimpList))
#define GIMP_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LIST, GimpListClass))
#define GIMP_IS_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LIST))
#define GIMP_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LIST))
#define GIMP_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LIST, GimpListClass))


typedef struct _GimpListClass GimpListClass;

struct _GimpList
{
  GimpContainer  parent_instance;

  GQueue        *queue;
  gboolean       unique_names;
  GCompareFunc   sort_func;
  gboolean       append;
};

struct _GimpListClass
{
  GimpContainerClass  parent_class;
};


GType           gimp_list_get_type      (void) G_GNUC_CONST;

GimpContainer * gimp_list_new           (GType         child_type,
                                         gboolean      unique_names);
GimpContainer * gimp_list_new_weak      (GType         child_type,
                                         gboolean      unique_names);

void            gimp_list_reverse       (GimpList     *list);
void            gimp_list_set_sort_func (GimpList     *list,
                                         GCompareFunc  sort_func);
GCompareFunc    gimp_list_get_sort_func (GimpList     *list);
void            gimp_list_sort          (GimpList     *list,
                                         GCompareFunc  sort_func);
void            gimp_list_sort_by_name  (GimpList     *list);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GimpList, g_object_unref);
