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

#include "base/tile-manager.h"
#include "base/pixel-region.h"

#include "gimp-gegl-utils.h"
#include "gimpoperationtilesource.h"


enum
{
  PROP_0,
  PROP_TILE_MANAGER
};


static void     gimp_operation_tile_source_finalize     (GObject       *object);
static void     gimp_operation_tile_source_get_property (GObject       *object,
                                                         guint          property_id,
                                                         GValue        *value,
                                                         GParamSpec    *pspec);
static void     gimp_operation_tile_source_set_property (GObject       *object,
                                                         guint          property_id,
                                                         const GValue  *value,
                                                         GParamSpec    *pspec);

static GeglRectangle
          gimp_operation_tile_source_get_defined_region (GeglOperation *operation);

static gboolean gimp_operation_tile_source_process      (GeglOperation *operation,
                                                         gpointer       context_id);


G_DEFINE_TYPE (GimpOperationTileSource, gimp_operation_tile_source,
               GEGL_TYPE_OPERATION_SOURCE)

#define parent_class gimp_operation_tile_source_parent_class


static void
gimp_operation_tile_source_class_init (GimpOperationTileSourceClass * klass)
{
  GObjectClass             *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationSourceClass *source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  object_class->finalize              = gimp_operation_tile_source_finalize;
  object_class->set_property          = gimp_operation_tile_source_set_property;
  object_class->get_property          = gimp_operation_tile_source_get_property;

  operation_class->get_defined_region = gimp_operation_tile_source_get_defined_region;

  source_class->process               = gimp_operation_tile_source_process;

  gegl_operation_class_set_name (operation_class, "gimp-tilemanager-source");;

  g_object_class_install_property (object_class,
                                   PROP_TILE_MANAGER,
                                   g_param_spec_boxed ("tile-manager",
                                                       "Tile Manager",
                                                       "The tile manager to use as source",
                                                       GIMP_TYPE_TILE_MANAGER,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));
}

static void
gimp_operation_tile_source_init (GimpOperationTileSource *self)
{
}

static void
gimp_operation_tile_source_finalize (GObject *object)
{
  GimpOperationTileSource *self = GIMP_OPERATION_TILE_SOURCE (object);

  if (self->tile_manager)
    {
      tile_manager_unref (self->tile_manager);
      self->tile_manager = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_operation_tile_source_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  GimpOperationTileSource *self = GIMP_OPERATION_TILE_SOURCE (object);

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
gimp_operation_tile_source_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  GimpOperationTileSource *self = GIMP_OPERATION_TILE_SOURCE (object);

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

static GeglRectangle
gimp_operation_tile_source_get_defined_region (GeglOperation *operation)
{
  GimpOperationTileSource *self   = GIMP_OPERATION_TILE_SOURCE (operation);
  GeglRectangle            result = { 0, };

  if (self->tile_manager)
    {
      result.x      = 0;
      result.y      = 0;
      result.width  = tile_manager_width (self->tile_manager);
      result.height = tile_manager_height (self->tile_manager);
    }

  return result;
}

static gboolean
gimp_operation_tile_source_process (GeglOperation *operation,
                                    gpointer       context_id)
{
  GimpOperationTileSource *self = GIMP_OPERATION_TILE_SOURCE (operation);

  if (self->tile_manager)
    {
      GeglBuffer    *output;
      const Babl    *format;
      PixelRegion    srcPR;
      gpointer       pr;
      GeglRectangle  extent = { 0, 0,
                                tile_manager_width  (self->tile_manager),
                                tile_manager_height (self->tile_manager) };

      format = gimp_bpp_to_babl_format (tile_manager_bpp (self->tile_manager));

      output = gegl_buffer_new (&extent, format);

      pixel_region_init (&srcPR, self->tile_manager,
                         extent.x, extent.y,
                         extent.width, extent.height,
                         FALSE);

      for (pr = pixel_regions_register (1, &srcPR);
           pr;
           pr = pixel_regions_process (pr))
        {
          GeglRectangle rect = { srcPR.x, srcPR.y, srcPR.w, srcPR.h };

          gegl_buffer_set (output, &rect, format, srcPR.data);
        }

      gegl_operation_set_data (operation, context_id,
                               "output", G_OBJECT (output));
    }

  return TRUE;
}
