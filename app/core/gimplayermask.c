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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "apptypes.h"

#include "boundary.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimpimage.h"
#include "gimplayermask.h"
#include "gimppreviewcache.h"
#include "layer.h"
#include "parasitelist.h"
#include "paint_funcs.h"
#include "pixel_region.h"
#include "undo.h"
#include "temp_buf.h"
#include "tile_manager.h"
#include "tile.h"

#include "libgimp/gimpparasite.h"

#include "libgimp/gimpintl.h"


static void      gimp_layer_mask_class_init      (GimpLayerMaskClass *klass);
static void      gimp_layer_mask_init            (GimpLayerMask      *layermask);
static void      gimp_layer_mask_destroy         (GtkObject          *object);


static GimpChannelClass  *parent_class = NULL;


GtkType
gimp_layer_mask_get_type (void)
{
  static GtkType layer_mask_type = 0;

  if (! layer_mask_type)
    {
      GtkTypeInfo layer_mask_info =
      {
	"GimpLayerMask",
	sizeof (GimpLayerMask),
	sizeof (GimpLayerMaskClass),
	(GtkClassInitFunc) gimp_layer_mask_class_init,
	(GtkObjectInitFunc) gimp_layer_mask_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      layer_mask_type = gtk_type_unique (GIMP_TYPE_CHANNEL, &layer_mask_info);
    }

  return layer_mask_type;
}

static void
gimp_layer_mask_class_init (GimpLayerMaskClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_CHANNEL);

  /*
  gtk_object_class_add_signals (object_class, layer_mask_signals, LAST_SIGNAL);
  */

  object_class->destroy = gimp_layer_mask_destroy;
}

static void
gimp_layer_mask_init (GimpLayerMask *layer_mask)
{
  layer_mask->layer = NULL;
}

static void
gimp_layer_mask_destroy (GtkObject *object)
{
  GimpLayerMask *layer_mask;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_LAYER_MASK (object));

  layer_mask = GIMP_LAYER_MASK (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GimpLayerMask *
gimp_layer_mask_new (GimpImage     *gimage,
		     gint           width,
		     gint           height,
		     const gchar   *name,
		     const GimpRGB *color)
{
  GimpLayerMask *layer_mask;

  layer_mask = gtk_type_new (GIMP_TYPE_LAYER_MASK);

  gimp_drawable_configure (GIMP_DRAWABLE (layer_mask), 
			   gimage, width, height, GRAY_GIMAGE, name);

  /*  set the layer_mask color and opacity  */
  GIMP_CHANNEL (layer_mask)->color          = *color;

  GIMP_CHANNEL (layer_mask)->show_masked    = TRUE;

  /*  selection mask variables  */
  GIMP_CHANNEL (layer_mask)->empty          = TRUE;
  GIMP_CHANNEL (layer_mask)->segs_in        = NULL;
  GIMP_CHANNEL (layer_mask)->segs_out       = NULL;
  GIMP_CHANNEL (layer_mask)->num_segs_in    = 0;
  GIMP_CHANNEL (layer_mask)->num_segs_out   = 0;
  GIMP_CHANNEL (layer_mask)->bounds_known   = TRUE;
  GIMP_CHANNEL (layer_mask)->boundary_known = TRUE;
  GIMP_CHANNEL (layer_mask)->x1             = 0;
  GIMP_CHANNEL (layer_mask)->y1             = 0;
  GIMP_CHANNEL (layer_mask)->x2             = width;
  GIMP_CHANNEL (layer_mask)->y2             = height;

  return layer_mask;
}

GimpLayerMask *
gimp_layer_mask_copy (GimpLayerMask *layer_mask)
{
  gchar         *layer_mask_name;
  GimpLayerMask *new_layer_mask;
  PixelRegion    srcPR, destPR;

  /*  formulate the new layer_mask name  */
  layer_mask_name =
    g_strdup_printf (_("%s copy"), 
		     gimp_object_get_name (GIMP_OBJECT (layer_mask)));

  /*  allocate a new layer_mask object  */
  new_layer_mask = gimp_layer_mask_new (GIMP_DRAWABLE (layer_mask)->gimage, 
					GIMP_DRAWABLE (layer_mask)->width, 
					GIMP_DRAWABLE (layer_mask)->height, 
					layer_mask_name,
					&GIMP_CHANNEL (layer_mask)->color);
  GIMP_DRAWABLE(new_layer_mask)->visible = 
    GIMP_DRAWABLE(layer_mask)->visible;
  GIMP_DRAWABLE(new_layer_mask)->offset_x = 
    GIMP_DRAWABLE(layer_mask)->offset_x;
  GIMP_DRAWABLE(new_layer_mask)->offset_y = 
    GIMP_DRAWABLE(layer_mask)->offset_y;
  GIMP_CHANNEL(new_layer_mask)->show_masked = 
    GIMP_CHANNEL(layer_mask)->show_masked;

  /*  copy the contents across layer masks  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE (layer_mask)->tiles, 
		     0, 0, 
		     GIMP_DRAWABLE (layer_mask)->width, 
		     GIMP_DRAWABLE (layer_mask)->height, 
		     FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE (new_layer_mask)->tiles, 
		     0, 0, 
		     GIMP_DRAWABLE (layer_mask)->width, 
		     GIMP_DRAWABLE (layer_mask)->height, 
		     TRUE);
  copy_region (&srcPR, &destPR);

  /*  free up the layer_mask_name memory  */
  g_free (layer_mask_name);

  return new_layer_mask;
}

void
gimp_layer_mask_set_layer (GimpLayerMask *mask,
			   GimpLayer     *layer)
{
  mask->layer = layer;
}

GimpLayer *
gimp_layer_mask_get_layer (GimpLayerMask *mask)
{
  return mask->layer;
}
