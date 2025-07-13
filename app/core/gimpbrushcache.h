/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushcache.h
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_BRUSH_CACHE            (gimp_brush_cache_get_type ())
#define GIMP_BRUSH_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BRUSH_CACHE, GimpBrushCache))
#define GIMP_BRUSH_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH_CACHE, GimpBrushCacheClass))
#define GIMP_IS_BRUSH_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BRUSH_CACHE))
#define GIMP_IS_BRUSH_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH_CACHE))
#define GIMP_BRUSH_CACHE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BRUSH_CACHE, GimpBrushCacheClass))


typedef struct _GimpBrushCacheClass GimpBrushCacheClass;

struct _GimpBrushCache
{
  GimpObject      parent_instance;

  GDestroyNotify  data_destroy;

  GList          *cached_units;

  gchar           debug_hit;
  gchar           debug_miss;
};

struct _GimpBrushCacheClass
{
  GimpObjectClass  parent_class;
};


GType            gimp_brush_cache_get_type (void) G_GNUC_CONST;

GimpBrushCache * gimp_brush_cache_new      (GDestroyNotify  data_destory,
                                            gchar           debug_hit,
                                            gchar           debug_miss);

void             gimp_brush_cache_clear    (GimpBrushCache *cache);

gconstpointer    gimp_brush_cache_get      (GimpBrushCache *cache,
                                            gint            width,
                                            gint            height,
                                            gdouble         scale,
                                            gdouble         aspect_ratio,
                                            gdouble         angle,
                                            gboolean        reflect,
                                            gdouble         hardness);
void             gimp_brush_cache_add      (GimpBrushCache *cache,
                                            gpointer        data,
                                            gint            width,
                                            gint            height,
                                            gdouble         scale,
                                            gdouble         aspect_ratio,
                                            gdouble         angle,
                                            gboolean        reflect,
                                            gdouble         hardness);
