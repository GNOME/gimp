/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptagcache.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdata.h"
#include "gimptagcache.h"
#include "gimplist.h"

#include "gimp-intl.h"


#define WRITABLE_PATH_KEY "gimp-data-cache-writable-path"


static void    gimp_tag_cache_finalize     (GObject              *object);

static gint64  gimp_tag_cache_get_memsize  (GimpObject           *object,
                                               gint64               *gui_size);

G_DEFINE_TYPE (GimpTagCache, gimp_tag_cache, GIMP_TYPE_OBJECT)

#define parent_class gimp_tag_cache_parent_class


static void
gimp_tag_cache_class_init (GimpTagCacheClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize         = gimp_tag_cache_finalize;

  gimp_object_class->get_memsize = gimp_tag_cache_get_memsize;
}

static void
gimp_tag_cache_init (GimpTagCache *cache)
{
  cache->gimp                   = NULL;
}

static void
gimp_tag_cache_finalize (GObject *object)
{
  GimpTagCache *cache = GIMP_TAG_CACHE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_tag_cache_get_memsize (GimpObject *object,
                               gint64     *gui_size)
{
  GimpTagCache *cache = GIMP_TAG_CACHE (object);
  gint64           memsize = 0;

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

GimpTagCache *
gimp_tag_cache_new (Gimp       *gimp)
{
  GimpTagCache *cache;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  cache = g_object_new (GIMP_TYPE_TAG_CACHE, NULL);

  cache->gimp                   = gimp;

  return cache;
}

void
gimp_tag_cache_update (GimpTaggedInterface *tagged,
                       GimpTagCache        *cache)
{
    printf("resource received: %s\n", gimp_tagged_get_identifier (tagged)); 
}
