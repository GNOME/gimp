/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationtilesink.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@gimp.org>
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

#include <string.h>

#undef G_DISABLE_DEPRECATED /* GStaticMutex */
#include <gegl.h>
#include <gegl-buffer.h>

#include "gimp-gegl-types.h"

#include "base/base-types.h"
#include "base/tile-manager.h"
#include "base/pixel-region.h"

#include "gimp-gegl-utils.h"
#include "gimpoperationtilesink.h"


enum
{
  PROP_0,
  PROP_TILE_MANAGER,
  PROP_LINEAR
};

enum
{
  DATA_WRITTEN,
  LAST_SIGNAL
};


static void     gimp_operation_tile_sink_finalize     (GObject       *object);
static void     gimp_operation_tile_sink_get_property (GObject       *object,
                                                       guint          property_id,
                                                       GValue        *value,
                                                       GParamSpec    *pspec);
static void     gimp_operation_tile_sink_set_property (GObject       *object,
                                                       guint          property_id,
                                                       const GValue  *value,
                                                       GParamSpec    *pspec);

static gboolean gimp_operation_tile_sink_process      (GeglOperation *operation,
                                                       GeglBuffer          *input,
                                                       const GeglRectangle *result,
                                                       gint                 level);


G_DEFINE_TYPE (GimpOperationTileSink, gimp_operation_tile_sink,
               GEGL_TYPE_OPERATION_SINK)

#define parent_class gimp_operation_tile_sink_parent_class

static guint tile_sink_signals[LAST_SIGNAL] = { 0 };


static void
gimp_operation_tile_sink_class_init (GimpOperationTileSinkClass *klass)
{
  GObjectClass           *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationSinkClass *sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  tile_sink_signals[DATA_WRITTEN] =
    g_signal_new ("data-written",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpOperationTileSinkClass, data_written),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  object_class->finalize       = gimp_operation_tile_sink_finalize;
  object_class->set_property   = gimp_operation_tile_sink_set_property;
  object_class->get_property   = gimp_operation_tile_sink_get_property;

  gegl_operation_class_set_keys (operation_class,
      "name", "gimp:tilemanager-sink",
      "categories", "output",
      "description", "GIMP TileManager sink",
      NULL);

  sink_class->process          = gimp_operation_tile_sink_process;
  sink_class->needs_full       = FALSE;


  g_object_class_install_property (object_class, PROP_TILE_MANAGER,
                                   g_param_spec_boxed ("tile-manager",
                                                       "Tile Manager",
                                                       "The tile manager to use as a destination",
                                                       GIMP_TYPE_TILE_MANAGER,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LINEAR,
                                   g_param_spec_boolean ("linear",
                                                         "Linear data",
                                                         "Should the data written to the tile-manager be linear or gamma-corrected?",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_operation_tile_sink_init (GimpOperationTileSink *self)
{
}

static void
gimp_operation_tile_sink_finalize (GObject *object)
{
  GimpOperationTileSink *self = GIMP_OPERATION_TILE_SINK (object);

  if (self->tile_manager)
    {
      tile_manager_unref (self->tile_manager);
      self->tile_manager = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_operation_tile_sink_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpOperationTileSink *self = GIMP_OPERATION_TILE_SINK (object);

  switch (property_id)
    {
    case PROP_TILE_MANAGER:
      g_value_set_boxed (value, self->tile_manager);
      break;

    case PROP_LINEAR:
      g_value_set_boolean (value, self->linear);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_tile_sink_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpOperationTileSink *self = GIMP_OPERATION_TILE_SINK (object);

  switch (property_id)
    {
    case PROP_TILE_MANAGER:
      if (self->tile_manager)
        tile_manager_unref (self->tile_manager);
      self->tile_manager = g_value_dup_boxed (value);
      break;

    case PROP_LINEAR:
      self->linear = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static gboolean
gimp_operation_tile_sink_process (GeglOperation       *operation,
                                  GeglBuffer          *input,
                                  const GeglRectangle *result,
                                  gint                 level)
{
  GimpOperationTileSink *self = GIMP_OPERATION_TILE_SINK (operation);
  static GStaticMutex    mutex = G_STATIC_MUTEX_INIT;
  const Babl            *format;
  PixelRegion            destPR;
  gpointer               pr;

  if (! self->tile_manager)
    return FALSE;

  format = gimp_bpp_to_babl_format (tile_manager_bpp (self->tile_manager),
                                    self->linear);

  pixel_region_init (&destPR, self->tile_manager,
                     result->x,     result->y,
                     result->width, result->height,
                     TRUE);

  for (pr = pixel_regions_register (1, &destPR);
       pr;
       pr = pixel_regions_process (pr))
    {
      GeglRectangle rect = { destPR.x, destPR.y, destPR.w, destPR.h };

      gegl_buffer_get (input, &rect, 1.0,
                       format, destPR.data, destPR.rowstride,
                       GEGL_ABYSS_NONE);
    }

  g_static_mutex_lock (&mutex); 
  /* a lock here serializes all fired signals, this emit gets called from
   * different worker threads, but the main loop will be blocked by GEGL
   * when it happens
   */
  g_signal_emit (operation, tile_sink_signals[DATA_WRITTEN], 0, result);
  g_static_mutex_unlock (&mutex);

  return TRUE;
}
