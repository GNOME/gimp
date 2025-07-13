/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptagcache.h
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

#pragma once

#include "gimpobject.h"


#define GIMP_TYPE_TAG_CACHE            (gimp_tag_cache_get_type ())
#define GIMP_TAG_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TAG_CACHE, GimpTagCache))
#define GIMP_TAG_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TAG_CACHE, GimpTagCacheClass))
#define GIMP_IS_TAG_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TAG_CACHE))
#define GIMP_IS_TAG_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TAG_CACHE))
#define GIMP_TAG_CACHE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TAG_CACHE, GimpTagCacheClass))


typedef struct _GimpTagCacheClass   GimpTagCacheClass;
typedef struct _GimpTagCachePrivate GimpTagCachePrivate;

struct _GimpTagCache
{
  GimpObject           parent_instance;

  GimpTagCachePrivate *priv;
};

struct _GimpTagCacheClass
{
  GimpObjectClass  parent_class;
};


GType           gimp_tag_cache_get_type      (void) G_GNUC_CONST;

GimpTagCache *  gimp_tag_cache_new           (void);

void            gimp_tag_cache_save          (GimpTagCache  *cache);
void            gimp_tag_cache_load          (GimpTagCache  *cache);

void            gimp_tag_cache_add_container (GimpTagCache  *cache,
                                              GimpContainer *container);
