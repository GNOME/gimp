/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpops.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gegl/buffer/gegl-buffer.h>

#include "gegl/gegl-types.h"

#include "gegl-types.h"

#include "base/base-types.h"
#include "base/tile-manager.h"
#include "base/pixel-region.h"

#include "gimp-gegl-utils.h"
#include "gimpoperationtilesink.h"


enum
{
  PROP_0,
  PROP_TILE_MANAGER
};


static void     tile_sink_get_property (GObject       *gobject,
                                        guint          property_id,
                                        GValue        *value,
                                        GParamSpec    *pspec);
static void     tile_sink_set_property (GObject       *gobject,
                                        guint          property_id,
                                        const GValue  *value,
                                        GParamSpec    *pspec);

static gboolean tile_sink_process      (GeglOperation *operation,
                                        gpointer       context_id);


G_DEFINE_TYPE (GimpOperationTileSink, gimp_operation_tile_sink,
               GEGL_TYPE_OPERATION_SINK)

static void
gimp_operation_tile_sink_class_init (GimpOperationTileSinkClass * klass)
{
  GObjectClass           *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationSinkClass *sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  object_class->set_property = tile_sink_set_property;
  object_class->get_property = tile_sink_get_property;

  sink_class->process        = tile_sink_process;

  gegl_operation_class_set_name (operation_class, "gimp-tilemanager-sink");;

  g_object_class_install_property (object_class,
                                   PROP_TILE_MANAGER,
                                   g_param_spec_pointer ("tile-manager",
                                                         "Tile Manager",
                                                         "The tile manager to use as a destination",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_operation_tile_sink_init (GimpOperationTileSink *self)
{
}

static void
tile_sink_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpOperationTileSink *self = GIMP_OPERATION_TILE_SINK (object);

  switch (property_id)
    {
    case PROP_TILE_MANAGER:
      g_value_set_pointer (value, self->tile_manager);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
tile_sink_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpOperationTileSink *self = GIMP_OPERATION_TILE_SINK (object);

  switch (property_id)
    {
    case PROP_TILE_MANAGER:
      self->tile_manager = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
tile_sink_process (GeglOperation *operation,
                   gpointer       context_id)
{
  GimpOperationTileSink *self = GIMP_OPERATION_TILE_SINK (operation);

  if (self->tile_manager)
    {
      GeglBuffer    *input;
      GeglRectangle *extent;
      gpointer       pr;
      PixelRegion    destPR;
      const Babl    *format;

      /* is this somethings that should be done already for all sinks? */
      input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id,
                                                    "input"));

      extent = gegl_operation_result_rect (operation, context_id);

      pixel_region_init (&destPR, self->tile_manager,
                         extent->x, extent->y,
                         extent->width, extent->height,
                         TRUE);

      format = gimp_bpp_to_babl_format (tile_manager_bpp (self->tile_manager));

      for (pr = pixel_regions_register (1, &destPR);
           pr;
           pr = pixel_regions_process (pr))
        {
          GeglRectangle rect = { destPR.x, destPR.y, destPR.w, destPR.h };

          gegl_buffer_get (input,
                           1.0, &rect, format,
                           destPR.data, destPR.rowstride);
        }

    }
  else
    {
      g_warning ("no tilemanager?");
    }

  return TRUE;
}
