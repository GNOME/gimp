/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmafilteredcontainer.h
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
 *               2011 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_FILTERED_CONTAINER_H__
#define __LIGMA_FILTERED_CONTAINER_H__


#include "ligmalist.h"


#define LIGMA_TYPE_FILTERED_CONTAINER            (ligma_filtered_container_get_type ())
#define LIGMA_FILTERED_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FILTERED_CONTAINER, LigmaFilteredContainer))
#define LIGMA_FILTERED_CONTAINER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FILTERED_CONTAINER, LigmaFilteredContainerClass))
#define LIGMA_IS_FILTERED_CONTAINER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FILTERED_CONTAINER))
#define LIGMA_IS_FILTERED_CONTAINER_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), LIGMA_TYPE_FILTERED_CONTAINER))
#define LIGMA_FILTERED_CONTAINER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FILTERED_CONTAINER, LigmaFilteredContainerClass))


typedef struct _LigmaFilteredContainerClass LigmaFilteredContainerClass;

struct _LigmaFilteredContainer
{
  LigmaList              parent_instance;

  LigmaContainer        *src_container;
  LigmaObjectFilterFunc  filter_func;
  gpointer              filter_data;
};

struct _LigmaFilteredContainerClass
{
  LigmaContainerClass  parent_class;

  void (* src_add)    (LigmaFilteredContainer *filtered_container,
                       LigmaObject            *object);
  void (* src_remove) (LigmaFilteredContainer *filtered_container,
                       LigmaObject            *object);
  void (* src_freeze) (LigmaFilteredContainer *filtered_container);
  void (* src_thaw)   (LigmaFilteredContainer *filtered_container);
};


GType           ligma_filtered_container_get_type (void) G_GNUC_CONST;

LigmaContainer * ligma_filtered_container_new      (LigmaContainer        *src_container,
                                                  LigmaObjectFilterFunc  filter_func,
                                                  gpointer              filter_data);


#endif  /* __LIGMA_FILTERED_CONTAINER_H__ */
