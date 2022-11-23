/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatagcache.h
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

#ifndef __LIGMA_TAG_CACHE_H__
#define __LIGMA_TAG_CACHE_H__


#include "ligmaobject.h"


#define LIGMA_TYPE_TAG_CACHE            (ligma_tag_cache_get_type ())
#define LIGMA_TAG_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TAG_CACHE, LigmaTagCache))
#define LIGMA_TAG_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TAG_CACHE, LigmaTagCacheClass))
#define LIGMA_IS_TAG_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TAG_CACHE))
#define LIGMA_IS_TAG_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TAG_CACHE))
#define LIGMA_TAG_CACHE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TAG_CACHE, LigmaTagCacheClass))


typedef struct _LigmaTagCacheClass   LigmaTagCacheClass;
typedef struct _LigmaTagCachePrivate LigmaTagCachePrivate;

struct _LigmaTagCache
{
  LigmaObject           parent_instance;

  LigmaTagCachePrivate *priv;
};

struct _LigmaTagCacheClass
{
  LigmaObjectClass  parent_class;
};


GType           ligma_tag_cache_get_type      (void) G_GNUC_CONST;

LigmaTagCache *  ligma_tag_cache_new           (void);

void            ligma_tag_cache_save          (LigmaTagCache  *cache);
void            ligma_tag_cache_load          (LigmaTagCache  *cache);

void            ligma_tag_cache_add_container (LigmaTagCache  *cache,
                                              LigmaContainer *container);


#endif  /*  __LIGMA_TAG_CACHE_H__  */
