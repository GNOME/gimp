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

#include "gimpoptilesource.h"


enum
{
  PROP_0,
  PROP_TILE_MANAGER
};


static void     get_property (GObject       *gobject,
                              guint          property_id,
                              GValue        *value,
                              GParamSpec    *pspec);
static void     set_property (GObject       *gobject,
                              guint          property_id,
                              const GValue  *value,
                              GParamSpec    *pspec);

static gboolean process      (GeglOperation *operation,
                              gpointer       context_id);

static GeglRectangle get_defined_region (GeglOperation *operation);


G_DEFINE_TYPE (GimpOperationTileSource, gimp_operation_tile_source,
               GEGL_TYPE_OPERATION_SOURCE)


static void
gimp_operation_tile_source_class_init (GimpOperationTileSourceClass * klass)
{
  GObjectClass             *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationSourceClass *source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  object_class->set_property          = set_property;
  object_class->get_property          = get_property;

  operation_class->get_defined_region = get_defined_region;

  source_class->process               = process;

  gegl_operation_class_set_name (operation_class, "gimp-tilemanager-source");;

  g_object_class_install_property (object_class,
                                   PROP_TILE_MANAGER,
                                   g_param_spec_pointer ("tile-manager",
                                                         "Tile Manager",
                                                         "The tile manager to use as a destination",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

}

static void
gimp_operation_tile_source_init (GimpOperationTileSource *self)
{
}

static void
get_property (GObject    *object,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GimpOperationTileSource *self = GIMP_OPERATION_TILE_SOURCE (object);

  switch (property_id)
    {
    case PROP_TILE_MANAGER:
      g_value_set_pointer (value, self->tile_manager);
      break;

    default:
      break;
    }
}

static void
set_property (GObject      *object,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GimpOperationTileSource *self = GIMP_OPERATION_TILE_SOURCE (object);

  switch (property_id)
    {
    case PROP_TILE_MANAGER:
      self->tile_manager = g_value_get_pointer (value);
      break;

    default:
      break;
    }
}

static Babl *
bpp_to_format (guint bpp)
{
  switch (bpp)
    {
    case 1: return babl_format ("Y' u8");
    case 2: return babl_format ("Y'A u8");
    case 3: return babl_format ("R'G'B' u8");
    case 4: return babl_format ("R'G'B'A u8");
    }

  g_warning ("bpp !(>0 && <=4)");

  return NULL;
}

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GimpOperationTileSource *self = GIMP_OPERATION_TILE_SOURCE (operation);
  GeglBuffer              *output;
  Babl                    *format;

  if (self->tile_manager)
    {
      PixelRegion   srcPR;
      gpointer      pr;
      GeglRectangle extent = { 0, 0,
                               tile_manager_width (self->tile_manager),
                               tile_manager_height (self->tile_manager) };

      format = bpp_to_format (tile_manager_bpp (self->tile_manager));


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

static GeglRectangle
get_defined_region (GeglOperation *operation)
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
