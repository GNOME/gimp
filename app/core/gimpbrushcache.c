/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushcache.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "core-types.h"

#include "gimpbrushcache.h"

#include "gimp-log.h"
#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_DATA_DESTROY
};


static void   gimp_brush_cache_constructed  (GObject      *object);
static void   gimp_brush_cache_finalize     (GObject      *object);
static void   gimp_brush_cache_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   gimp_brush_cache_get_property (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);


G_DEFINE_TYPE (GimpBrushCache, gimp_brush_cache, GIMP_TYPE_OBJECT)

#define parent_class gimp_brush_cache_parent_class


static void
gimp_brush_cache_class_init (GimpBrushCacheClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_brush_cache_constructed;
  object_class->finalize     = gimp_brush_cache_finalize;
  object_class->set_property = gimp_brush_cache_set_property;
  object_class->get_property = gimp_brush_cache_get_property;

  g_object_class_install_property (object_class, PROP_DATA_DESTROY,
                                   g_param_spec_pointer ("data-destroy",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_brush_cache_init (GimpBrushCache *brush)
{
}

static void
gimp_brush_cache_constructed (GObject *object)
{
  GimpBrushCache *cache = GIMP_BRUSH_CACHE (object);

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (cache->data_destroy != NULL);
}

static void
gimp_brush_cache_finalize (GObject *object)
{
  GimpBrushCache *cache = GIMP_BRUSH_CACHE (object);

  if (cache->last_data)
    {
      cache->data_destroy (cache->last_data);
      cache->last_data = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_brush_cache_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpBrushCache *cache = GIMP_BRUSH_CACHE (object);

  switch (property_id)
    {
    case PROP_DATA_DESTROY:
      cache->data_destroy = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_brush_cache_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpBrushCache *cache = GIMP_BRUSH_CACHE (object);

  switch (property_id)
    {
    case PROP_DATA_DESTROY:
      g_value_set_pointer (value, cache->data_destroy);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GimpBrushCache *
gimp_brush_cache_new (GDestroyNotify  data_destroy,
                      gchar           debug_hit,
                      gchar           debug_miss)
{
  GimpBrushCache *cache;

  g_return_val_if_fail (data_destroy != NULL, NULL);

  cache =  g_object_new (GIMP_TYPE_BRUSH_CACHE,
                         "data-destroy", data_destroy,
                         NULL);

  cache->debug_hit  = debug_hit;
  cache->debug_miss = debug_miss;

  return cache;
}

void
gimp_brush_cache_clear (GimpBrushCache *cache)
{
  g_return_if_fail (GIMP_IS_BRUSH_CACHE (cache));

  if (cache->last_data)
    {
      cache->data_destroy (cache->last_data);
      cache->last_data = NULL;
    }
}

gconstpointer
gimp_brush_cache_get (GimpBrushCache *cache,
                      gint            width,
                      gint            height,
                      gdouble         scale,
                      gdouble         aspect_ratio,
                      gdouble         angle,
                      gdouble         hardness)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_CACHE (cache), NULL);

  if (cache->last_data                         &&
      cache->last_width        == width        &&
      cache->last_height       == height       &&
      cache->last_scale        == scale        &&
      cache->last_aspect_ratio == aspect_ratio &&
      cache->last_angle        == angle        &&
      cache->last_hardness     == hardness)
    {
      if (gimp_log_flags & GIMP_LOG_BRUSH_CACHE)
        g_printerr ("%c", cache->debug_hit);

      return (gconstpointer) cache->last_data;
    }

  if (gimp_log_flags & GIMP_LOG_BRUSH_CACHE)
    g_printerr ("%c", cache->debug_miss);

  return NULL;
}

void
gimp_brush_cache_add (GimpBrushCache *cache,
                      gpointer        data,
                      gint            width,
                      gint            height,
                      gdouble         scale,
                      gdouble         aspect_ratio,
                      gdouble         angle,
                      gdouble         hardness)
{
  g_return_if_fail (GIMP_IS_BRUSH_CACHE (cache));
  g_return_if_fail (data != NULL);

  if (data == cache->last_data)
    return;

  if (cache->last_data)
    cache->data_destroy (cache->last_data);

  cache->last_data         = data;
  cache->last_width        = width;
  cache->last_height       = height;
  cache->last_scale        = scale;
  cache->last_aspect_ratio = aspect_ratio;
  cache->last_angle        = angle;
  cache->last_hardness     = hardness;
}
