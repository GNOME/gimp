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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"

#include "paint-funcs/paint-funcs.h"

#include "gimplayermask.h"
#include "gimpmarshal.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


enum
{
  APPLY_CHANGED,
  EDIT_CHANGED,
  SHOW_CHANGED,
  LAST_SIGNAL
};


static void   gimp_layer_mask_class_init (GimpLayerMaskClass *klass);
static void   gimp_layer_mask_init       (GimpLayerMask      *layermask);


static guint  layer_mask_signals[LAST_SIGNAL] = { 0 };

static GimpChannelClass *parent_class = NULL;


GType
gimp_layer_mask_get_type (void)
{
  static GType layer_mask_type = 0;

  if (! layer_mask_type)
    {
      static const GTypeInfo layer_mask_info =
      {
        sizeof (GimpLayerMaskClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_layer_mask_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpLayerMask),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_layer_mask_init,
      };

      layer_mask_type = g_type_register_static (GIMP_TYPE_CHANNEL,
						"GimpLayerMask",
						&layer_mask_info, 0);
    }

  return layer_mask_type;
}

static void
gimp_layer_mask_class_init (GimpLayerMaskClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  layer_mask_signals[APPLY_CHANGED] =
    g_signal_new ("apply_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpLayerMaskClass, apply_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  layer_mask_signals[EDIT_CHANGED] =
    g_signal_new ("edit_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpLayerMaskClass, edit_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  layer_mask_signals[SHOW_CHANGED] =
    g_signal_new ("show_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpLayerMaskClass, show_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}

static void
gimp_layer_mask_init (GimpLayerMask *layer_mask)
{
  layer_mask->layer      = NULL;
  layer_mask->apply_mask = TRUE;
  layer_mask->edit_mask  = TRUE;
  layer_mask->show_mask  = FALSE;
}

GimpLayerMask *
gimp_layer_mask_new (GimpImage     *gimage,
		     gint           width,
		     gint           height,
		     const gchar   *name,
		     const GimpRGB *color)
{
  GimpLayerMask *layer_mask;

  layer_mask = g_object_new (GIMP_TYPE_LAYER_MASK, NULL);

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

  GIMP_DRAWABLE (new_layer_mask)->visible   = GIMP_DRAWABLE (layer_mask)->visible;
  GIMP_DRAWABLE (new_layer_mask)->offset_x  = GIMP_DRAWABLE (layer_mask)->offset_x;
  GIMP_DRAWABLE (new_layer_mask)->offset_y  = GIMP_DRAWABLE (layer_mask)->offset_y;
  GIMP_CHANNEL (new_layer_mask)->show_masked = GIMP_CHANNEL (layer_mask)->show_masked;

  new_layer_mask->apply_mask = layer_mask->apply_mask;
  new_layer_mask->edit_mask  = layer_mask->edit_mask;
  new_layer_mask->show_mask  = layer_mask->show_mask;

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

void
gimp_layer_mask_set_apply (GimpLayerMask *layer_mask,
                           gboolean       apply)
{
  g_return_if_fail (GIMP_IS_LAYER_MASK (layer_mask));

  if (layer_mask->apply_mask != apply)
    {
      layer_mask->apply_mask = apply;

      if (layer_mask->layer)
        {
          GimpDrawable *drawable;

          drawable = GIMP_DRAWABLE (layer_mask->layer);

          gimp_drawable_update (drawable,
				0, 0,
				gimp_drawable_width  (drawable),
				gimp_drawable_height (drawable));
        }

      g_signal_emit (G_OBJECT (layer_mask),
		     layer_mask_signals[APPLY_CHANGED], 0);
    }
}

gboolean
gimp_layer_mask_get_apply (GimpLayerMask *layer_mask)
{
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (layer_mask), FALSE);

  return layer_mask->apply_mask;
}

void
gimp_layer_mask_set_edit (GimpLayerMask *layer_mask,
                          gboolean       edit)
{
  g_return_if_fail (GIMP_IS_LAYER_MASK (layer_mask));

  if (layer_mask->edit_mask != edit)
    {
      layer_mask->edit_mask = edit;

      g_signal_emit (G_OBJECT (layer_mask),
		     layer_mask_signals[EDIT_CHANGED], 0);
    }
}

gboolean
gimp_layer_mask_get_edit (GimpLayerMask *layer_mask)
{
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (layer_mask), FALSE);

  return layer_mask->edit_mask;
}

void
gimp_layer_mask_set_show (GimpLayerMask *layer_mask,
                          gboolean       show)
{
  g_return_if_fail (GIMP_IS_LAYER_MASK (layer_mask));

  if (layer_mask->show_mask != show)
    {
      layer_mask->show_mask = show;

      if (layer_mask->layer)
        {
          GimpDrawable *drawable;

          drawable = GIMP_DRAWABLE (layer_mask->layer);

          gimp_drawable_update (drawable,
				0, 0,
				gimp_drawable_width  (drawable),
				gimp_drawable_height (drawable));
        }

      g_signal_emit (G_OBJECT (layer_mask),
		     layer_mask_signals[SHOW_CHANGED], 0);
    }
}

gboolean
gimp_layer_mask_get_show (GimpLayerMask *layer_mask)
{
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (layer_mask), FALSE);

  return layer_mask->show_mask;
}
