/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmataggedcontainer.h
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#ifndef __LIGMA_TAGGED_CONTAINER_H__
#define __LIGMA_TAGGED_CONTAINER_H__


#include "ligmafilteredcontainer.h"


#define LIGMA_TYPE_TAGGED_CONTAINER            (ligma_tagged_container_get_type ())
#define LIGMA_TAGGED_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TAGGED_CONTAINER, LigmaTaggedContainer))
#define LIGMA_TAGGED_CONTAINER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TAGGED_CONTAINER, LigmaTaggedContainerClass))
#define LIGMA_IS_TAGGED_CONTAINER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TAGGED_CONTAINER))
#define LIGMA_IS_TAGGED_CONTAINER_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), LIGMA_TYPE_TAGGED_CONTAINER))
#define LIGMA_TAGGED_CONTAINER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TAGGED_CONTAINER, LigmaTaggedContainerClass))


typedef struct _LigmaTaggedContainerClass LigmaTaggedContainerClass;

struct _LigmaTaggedContainer
{
  LigmaFilteredContainer  parent_instance;

  GList                 *filter;
  GHashTable            *tag_ref_counts;
  gint                   tag_count;
};

struct _LigmaTaggedContainerClass
{
  LigmaFilteredContainerClass  parent_class;

  void (* tag_count_changed) (LigmaTaggedContainer *container,
                              gint                 count);
};


GType           ligma_tagged_container_get_type      (void) G_GNUC_CONST;

LigmaContainer * ligma_tagged_container_new           (LigmaContainer       *src_container);

void            ligma_tagged_container_set_filter    (LigmaTaggedContainer *tagged_container,
                                                     GList               *tags);
const GList   * ligma_tagged_container_get_filter    (LigmaTaggedContainer *tagged_container);

gint            ligma_tagged_container_get_tag_count (LigmaTaggedContainer *container);


#endif  /* __LIGMA_TAGGED_CONTAINER_H__ */
