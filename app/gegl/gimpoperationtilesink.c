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
#include <glib-object.h>

#include "gegl/gegl-types.h"
#include <gegl/buffer/gegl-buffer.h>

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
                                                       gpointer       context_id);


G_DEFINE_TYPE (GimpOperationTileSink, gimp_operation_tile_sink,
               GEGL_TYPE_OPERATION_SINK)

#define parent_class gimp_operation_tile_sink_parent_class


static void
gimp_operation_tile_sink_class_init (GimpOperationTileSinkClass * klass)
{
  GObjectClass           *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationSinkClass *sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  object_class->finalize     = gimp_operation_tile_sink_finalize;
  object_class->set_property = gimp_operation_tile_sink_set_property;
  object_class->get_property = gimp_operation_tile_sink_get_property;

  sink_class->process        = gimp_operation_tile_sink_process;

  gegl_operation_class_set_name (operation_class, "gimp-tilemanager-sink");;

  g_object_class_install_property (object_class,
                                   PROP_TILE_MANAGER,
                                   g_param_spec_boxed ("tile-manager",
                                                       "Tile Manager",
                                                       "The tile manager to use as a destination",
                                                       GIMP_TYPE_TILE_MANAGER,
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_operation_tile_sink_process (GeglOperation *operation,
                                  gpointer       context_id)
{
  GimpOperationTileSink *self = GIMP_OPERATION_TILE_SINK (operation);

  if (self->tile_manager)
    {
      GeglBuffer          *input;
      const Babl          *format;
      const GeglRectangle *extent;
      PixelRegion          destPR;
      gpointer             pr;

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
