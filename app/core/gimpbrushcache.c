/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrushcache.c
 * Copyright (C) 2011 Michael Natterer <mitch@ligma.org>
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

#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligmabrushcache.h"

#include "ligma-log.h"
#include "ligma-intl.h"


#define MAX_CACHED_DATA 20


enum
{
  PROP_0,
  PROP_DATA_DESTROY
};


typedef struct _LigmaBrushCacheUnit LigmaBrushCacheUnit;

struct _LigmaBrushCacheUnit
{
  gpointer data;

  gint     width;
  gint     height;
  gdouble  scale;
  gdouble  aspect_ratio;
  gdouble  angle;
  gboolean reflect;
  gdouble  hardness;
};


static void   ligma_brush_cache_constructed  (GObject      *object);
static void   ligma_brush_cache_finalize     (GObject      *object);
static void   ligma_brush_cache_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   ligma_brush_cache_get_property (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaBrushCache, ligma_brush_cache, LIGMA_TYPE_OBJECT)

#define parent_class ligma_brush_cache_parent_class


static void
ligma_brush_cache_class_init (LigmaBrushCacheClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_brush_cache_constructed;
  object_class->finalize     = ligma_brush_cache_finalize;
  object_class->set_property = ligma_brush_cache_set_property;
  object_class->get_property = ligma_brush_cache_get_property;

  g_object_class_install_property (object_class, PROP_DATA_DESTROY,
                                   g_param_spec_pointer ("data-destroy",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_brush_cache_init (LigmaBrushCache *brush)
{
}

static void
ligma_brush_cache_constructed (GObject *object)
{
  LigmaBrushCache *cache = LIGMA_BRUSH_CACHE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (cache->data_destroy != NULL);
}

static void
ligma_brush_cache_finalize (GObject *object)
{
  LigmaBrushCache *cache = LIGMA_BRUSH_CACHE (object);

  ligma_brush_cache_clear (cache);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_brush_cache_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaBrushCache *cache = LIGMA_BRUSH_CACHE (object);

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
ligma_brush_cache_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaBrushCache *cache = LIGMA_BRUSH_CACHE (object);

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

LigmaBrushCache *
ligma_brush_cache_new (GDestroyNotify  data_destroy,
                      gchar           debug_hit,
                      gchar           debug_miss)
{
  LigmaBrushCache *cache;

  g_return_val_if_fail (data_destroy != NULL, NULL);

  cache =  g_object_new (LIGMA_TYPE_BRUSH_CACHE,
                         "data-destroy", data_destroy,
                         NULL);

  cache->debug_hit  = debug_hit;
  cache->debug_miss = debug_miss;

  return cache;
}

void
ligma_brush_cache_clear (LigmaBrushCache *cache)
{
  g_return_if_fail (LIGMA_IS_BRUSH_CACHE (cache));

  if (cache->cached_units)
    {
      GList *iter;

      for (iter = cache->cached_units; iter; iter = g_list_next (iter))
        {
          LigmaBrushCacheUnit *unit = iter->data;

          cache->data_destroy (unit->data);
        }

      g_list_free_full (cache->cached_units, g_free);
      cache->cached_units = NULL;
    }
}

gconstpointer
ligma_brush_cache_get (LigmaBrushCache *cache,
                      gint            width,
                      gint            height,
                      gdouble         scale,
                      gdouble         aspect_ratio,
                      gdouble         angle,
                      gboolean        reflect,
                      gdouble         hardness)
{
  GList *iter;

  g_return_val_if_fail (LIGMA_IS_BRUSH_CACHE (cache), NULL);

  for (iter = cache->cached_units; iter; iter = g_list_next (iter))
    {
      LigmaBrushCacheUnit *unit = iter->data;

      if (unit->data                         &&
          unit->width        == width        &&
          unit->height       == height       &&
          unit->scale        == scale        &&
          unit->aspect_ratio == aspect_ratio &&
          unit->angle        == angle        &&
          unit->reflect      == reflect      &&
          unit->hardness     == hardness)
        {
          if (ligma_log_flags & LIGMA_LOG_BRUSH_CACHE)
            g_printerr ("%c", cache->debug_hit);

          /* Make the returned cached brush first in the list. */
          cache->cached_units = g_list_remove_link (cache->cached_units, iter);
          iter->next = cache->cached_units;
          if (cache->cached_units)
            cache->cached_units->prev = iter;
          cache->cached_units = iter;

          return (gconstpointer) unit->data;
        }
    }

  if (ligma_log_flags & LIGMA_LOG_BRUSH_CACHE)
    g_printerr ("%c", cache->debug_miss);

  return NULL;
}

void
ligma_brush_cache_add (LigmaBrushCache *cache,
                      gpointer        data,
                      gint            width,
                      gint            height,
                      gdouble         scale,
                      gdouble         aspect_ratio,
                      gdouble         angle,
                      gboolean        reflect,
                      gdouble         hardness)
{
  GList              *iter;
  LigmaBrushCacheUnit *unit;
  GList              *last   = NULL;
  gint                length = 0;

  g_return_if_fail (LIGMA_IS_BRUSH_CACHE (cache));
  g_return_if_fail (data != NULL);

  for (iter = cache->cached_units; iter; iter = g_list_next (iter))
    {
      unit = iter->data;

      if (data == unit->data)
        return;

      length++;
      last = iter;
    }

  if (length > MAX_CACHED_DATA)
    {
      unit = last->data;

      cache->data_destroy (unit->data);
      cache->cached_units = g_list_delete_link (cache->cached_units, last);
      g_free (unit);
    }

  unit = g_new0 (LigmaBrushCacheUnit, 1);

  unit->data         = data;
  unit->width        = width;
  unit->height       = height;
  unit->scale        = scale;
  unit->aspect_ratio = aspect_ratio;
  unit->angle        = angle;
  unit->reflect      = reflect;
  unit->hardness     = hardness;

  cache->cached_units = g_list_prepend (cache->cached_units, unit);
}
