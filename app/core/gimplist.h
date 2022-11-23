/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmalist.h
 * Copyright (C) 2001-2016 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_LIST_H__
#define __LIGMA_LIST_H__


#include "ligmacontainer.h"


#define LIGMA_TYPE_LIST            (ligma_list_get_type ())
#define LIGMA_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LIST, LigmaList))
#define LIGMA_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LIST, LigmaListClass))
#define LIGMA_IS_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LIST))
#define LIGMA_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LIST))
#define LIGMA_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LIST, LigmaListClass))


typedef struct _LigmaListClass LigmaListClass;

struct _LigmaList
{
  LigmaContainer  parent_instance;

  GQueue        *queue;
  gboolean       unique_names;
  GCompareFunc   sort_func;
  gboolean       append;
};

struct _LigmaListClass
{
  LigmaContainerClass  parent_class;
};


GType           ligma_list_get_type      (void) G_GNUC_CONST;

LigmaContainer * ligma_list_new           (GType         children_type,
                                         gboolean      unique_names);
LigmaContainer * ligma_list_new_weak      (GType         children_type,
                                         gboolean      unique_names);

void            ligma_list_reverse       (LigmaList     *list);
void            ligma_list_set_sort_func (LigmaList     *list,
                                         GCompareFunc  sort_func);
GCompareFunc    ligma_list_get_sort_func (LigmaList     *list);
void            ligma_list_sort          (LigmaList     *list,
                                         GCompareFunc  sort_func);
void            ligma_list_sort_by_name  (LigmaList     *list);


#endif  /* __LIGMA_LIST_H__ */
