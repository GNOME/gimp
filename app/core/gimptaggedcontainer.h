/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimptaggedcontainer.h
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

#ifndef __GIMP_TAGGED_CONTAINER_H__
#define __GIMP_TAGGED_CONTAINER_H__


#include "gimpfilteredcontainer.h"


#define GIMP_TYPE_TAGGED_CONTAINER            (gimp_tagged_container_get_type ())
#define GIMP_TAGGED_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TAGGED_CONTAINER, GimpTaggedContainer))
#define GIMP_TAGGED_CONTAINER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TAGGED_CONTAINER, GimpTaggedContainerClass))
#define GIMP_IS_TAGGED_CONTAINER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TAGGED_CONTAINER))
#define GIMP_IS_TAGGED_CONTAINER_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GIMP_TYPE_TAGGED_CONTAINER))
#define GIMP_TAGGED_CONTAINER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TAGGED_CONTAINER, GimpTaggedContainerClass))


typedef struct _GimpTaggedContainerClass GimpTaggedContainerClass;

struct _GimpTaggedContainer
{
  GimpFilteredContainer  parent_instance;

  GList                 *filter;
  GHashTable            *tag_ref_counts;
  gint                   tag_count;
};

struct _GimpTaggedContainerClass
{
  GimpFilteredContainerClass  parent_class;

  void (* tag_count_changed) (GimpTaggedContainer *container,
                              gint                 count);
};


GType           gimp_tagged_container_get_type      (void) G_GNUC_CONST;

GimpContainer * gimp_tagged_container_new           (GimpContainer       *src_container);

void            gimp_tagged_container_set_filter    (GimpTaggedContainer *tagged_container,
                                                     GList               *tags);
const GList   * gimp_tagged_container_get_filter    (GimpTaggedContainer *tagged_container);

gint            gimp_tagged_container_get_tag_count (GimpTaggedContainer *container);


#endif  /* __GIMP_TAGGED_CONTAINER_H__ */
