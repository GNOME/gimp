/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "gimpbuffer.h"


static void      gimp_buffer_class_init      (GimpBufferClass *klass);
static void      gimp_buffer_init            (GimpBuffer      *buffer);

static void      gimp_buffer_finalize        (GObject         *object);

static TempBuf * gimp_buffer_get_new_preview (GimpViewable    *viewable,
					      gint             width,
					      gint             height);


static GimpViewableClass *parent_class = NULL;


GType
gimp_buffer_get_type (void)
{
  static GType buffer_type = 0;

  if (! buffer_type)
    {
      static const GTypeInfo buffer_info =
      {
        sizeof (GimpBufferClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_buffer_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpBuffer),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_buffer_init,
      };

      buffer_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
					    "GimpBuffer",
					    &buffer_info, 0);
  }

  return buffer_type;
}

static void
gimp_buffer_class_init (GimpBufferClass *klass)
{
  GObjectClass      *object_class;
  GimpViewableClass *viewable_class;

  object_class   = G_OBJECT_CLASS (klass);
  viewable_class = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize          = gimp_buffer_finalize;

  viewable_class->get_new_preview = gimp_buffer_get_new_preview;
}

static void
gimp_buffer_init (GimpBuffer *buffer)
{
  buffer->tiles = NULL;
}

static void
gimp_buffer_finalize (GObject *object)
{
  GimpBuffer *buffer;

  buffer = GIMP_BUFFER (object);

  if (buffer->tiles)
    {
      tile_manager_destroy (buffer->tiles);
      buffer->tiles = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static TempBuf *
gimp_buffer_get_new_preview (GimpViewable *viewable,
			     gint          width,
			     gint          height)
{
  GimpBuffer  *buffer;
  TempBuf     *temp_buf;
  gint         buffer_width;
  gint         buffer_height;
  PixelRegion  srcPR;
  PixelRegion  destPR;
  gint         bytes;
  gint         subsample;

  buffer        = GIMP_BUFFER (viewable);
  buffer_width  = tile_manager_width (buffer->tiles);
  buffer_height = tile_manager_height (buffer->tiles);

  bytes = tile_manager_bpp (buffer->tiles);

  /*  calculate 'acceptable' subsample  */
  subsample = 1;

  while ((width  * (subsample + 1) * 2 < buffer_width) &&
	 (height * (subsample + 1) * 2 < buffer_height))
    subsample += 1;

  pixel_region_init (&srcPR, buffer->tiles,
		     0, 0,
		     buffer_width,
		     buffer_height,
		     FALSE);

  temp_buf = temp_buf_new (width, height, bytes, 0, 0, NULL);

  destPR.bytes     = temp_buf->bytes;
  destPR.x         = 0;
  destPR.y         = 0;
  destPR.w         = width;
  destPR.h         = height;
  destPR.rowstride = width * destPR.bytes;
  destPR.data      = temp_buf_data (temp_buf);

  subsample_region (&srcPR, &destPR, subsample);

  return temp_buf;
}

GimpBuffer *
gimp_buffer_new (TileManager *tiles,
		 const gchar *name)
{
  GimpBuffer  *buffer;
  PixelRegion  srcPR, destPR;
  gint         width, height;

  g_return_val_if_fail (tiles != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  width  = tile_manager_width (tiles);
  height = tile_manager_height (tiles);

  buffer = GIMP_BUFFER (g_object_new (GIMP_TYPE_BUFFER, NULL));

  gimp_object_set_name (GIMP_OBJECT (buffer), name);

  buffer->tiles = tile_manager_new (width, height, tile_manager_bpp (tiles));

  pixel_region_init (&srcPR, tiles, 0, 0, width, height, FALSE); 
  pixel_region_init (&destPR, buffer->tiles, 0, 0, width, height, TRUE);
  copy_region (&srcPR, &destPR);

  return GIMP_BUFFER (buffer);
}
