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

#include <gegl.h>
#include <gegl/buffer/gegl-buffer.h>
#include "gegl/gegl-types.h"
#include "gimpoptilesource.h"
#include <string.h>
#include "app/base/base-types.h"
#include "app/base/tile-manager.h"
#include "app/base/pixel-region.h"

static void     get_property (GObject       *gobject,
                              guint          prop_id,
                              GValue        *value,
                              GParamSpec    *pspec);

static void     set_property (GObject       *gobject,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec);

static gboolean process      (GeglOperation *operation,
                              gpointer       context_id);

static GeglRectangle get_defined_region (GeglOperation *operation);

enum
{
  PROP_0,
  PROP_TILE_MANAGER
};

static void
gimp_operation_tile_source_init (GimpOperationTileSource *self)
{
}

G_DEFINE_TYPE (GimpOperationTileSource, gimp_operation_tile_source, GEGL_TYPE_OPERATION_SOURCE)

static void
gimp_operation_tile_source_class_init (GimpOperationTileSourceClass * klass)
{
  GObjectClass           *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationSourceClass *operation_source_class = GEGL_OPERATION_SOURCE_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  operation_source_class->process = process;
  operation_class->get_defined_region = get_defined_region;

  gegl_operation_class_set_name (operation_class, "gimp-tilemanager-source");;

  g_object_class_install_property (object_class,
                                   PROP_TILE_MANAGER,
                                   g_param_spec_pointer ("tile_manager",
                                                         "tile_manager",
                                                         "The tile manager to use as a destination",
                                                         (GParamFlags) ( (G_PARAM_READABLE | G_PARAM_WRITABLE) | G_PARAM_CONSTRUCT)));

}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GimpOperationTileSource *self = GIMP_OPERATION_TILE_SOURCE (object);
  switch (prop_id)
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
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GimpOperationTileSource *self = GIMP_OPERATION_TILE_SOURCE (object);
  switch (prop_id)
    {
      case PROP_TILE_MANAGER:
        self->tile_manager = g_value_get_pointer (value);
        break;
      default:
        break;
    }
}

static Babl *bpp_to_format (guint bpp)
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

  GeglBuffer          *output;
  Babl                *format;

  if (self->tile_manager)
    {
      PixelRegion   srcPR;
      gpointer      pr;
      GeglRectangle extent = {0,0,
                              tile_manager_width (self->tile_manager),
                              tile_manager_height (self->tile_manager)};
      format = bpp_to_format (tile_manager_bpp (self->tile_manager));


      output = gegl_buffer_new (&extent, format);

      pixel_region_init (&srcPR, self->tile_manager,
                         extent.x, extent.y, extent.width, extent.height, FALSE);

      for (pr = pixel_regions_register (1, &srcPR); pr; pr = pixel_regions_process (pr))
        {
          GeglRectangle rect = {srcPR.x, srcPR.y, srcPR.w, srcPR.h};

          gegl_buffer_set (output, &rect, format, srcPR.data);
        }
      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
    }
  return TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {0,0,0,0};
  GimpOperationTileSource *self = GIMP_OPERATION_TILE_SOURCE (operation);

  if (!self->tile_manager)
    {
      return result;
    }

  result.x = 0;
  result.y = 0;
  result.width  = tile_manager_width (self->tile_manager);
  result.height  = tile_manager_height (self->tile_manager);
  return result;
}

