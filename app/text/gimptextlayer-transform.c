/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextLayer
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include <glib-object.h>
#include <pango/pangoft2.h>

#include "text-types.h"

#include "core/gimpimage-undo.h"

#include "gimptext.h"
#include "gimptextlayer.h"
#include "gimptextlayer-transform.h"

#include "gimp-intl.h"


void
gimp_text_layer_scale (GimpItem               *item,
		       gint                    new_width,
		       gint                    new_height,
		       gint                    new_offset_x,
		       gint                    new_offset_y,
		       GimpInterpolationType   interpolation_type)
{

}

void
gimp_text_layer_flip (GimpItem            *item,
		      GimpOrientationType  flip_type,
		      gdouble              axis,
		      gboolean             clip_result)
{
  GimpLayer *layer;
  GimpImage *gimage;

  layer  = GIMP_LAYER (item);
  gimage = gimp_item_get_image (item);

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_TRANSFORM,
                               _("Flip Text Layer"));

  {
    GimpText    *text  = GIMP_TEXT_LAYER (item)->text;
    GimpMatrix2  trafo = { { { 1.0, 0.0 }, { 0.0, 1.0 } } };

    switch (flip_type)
      {
      case GIMP_ORIENTATION_HORIZONTAL:
	trafo.coeff[0][0] = - 1.0;
	break;

      case GIMP_ORIENTATION_VERTICAL:
	trafo.coeff[1][1] = - 1.0;
	break;

      case GIMP_ORIENTATION_UNKNOWN:
	break;
      }

    gimp_matrix2_mult (&trafo, &text->transformation);
    g_object_notify (G_OBJECT (text), "transformation");
    gimp_text_layer_flush (GIMP_TEXT_LAYER (item));
  }

  /*  If there is a layer mask, make sure it gets flipped as well  */
  if (layer->mask)
    gimp_item_flip (GIMP_ITEM (layer->mask),
                    flip_type, axis, clip_result);

  gimp_image_undo_group_end (gimage);

  /*  Make sure we're not caching any old selection info  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));
}

void
gimp_text_layer_rotate (GimpItem         *item,
                        GimpRotationType  rotate_type,
                        gdouble           center_x,
                        gdouble           center_y,
                        gboolean          clip_result)
{
  GimpLayer   *layer;
  GimpImage   *gimage;
  gdouble      cos = 1.0;
  gdouble      sin = 0.0;

  layer  = GIMP_LAYER (item);
  gimage = gimp_item_get_image (item);

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_TRANSFORM,
                               _("Rotate Text Layer"));

  switch (rotate_type)
    {
    case GIMP_ROTATE_90:
      cos =   0.0;
      sin = - 1.0;
      break;
    case GIMP_ROTATE_180:
      cos = - 1.0;
      sin =   0.0;
      break;
    case GIMP_ROTATE_270:
      cos =   0.0;
      sin =   1.0;
      break;
    }

  {
    GimpText    *text = GIMP_TEXT_LAYER (item)->text;
    GimpMatrix2  trafo;

    trafo.coeff[0][0] = cos;
    trafo.coeff[0][1] = -sin;
    trafo.coeff[1][0] = sin;
    trafo.coeff[1][1] = cos;

    gimp_matrix2_mult (&trafo, &text->transformation);
    g_object_notify (G_OBJECT (text), "transformation");
    gimp_text_layer_flush (GIMP_TEXT_LAYER (item));
  }

  /*  If there is a layer mask, make sure it gets rotates as well  */
  if (layer->mask)
    gimp_item_rotate (GIMP_ITEM (layer->mask),
                      rotate_type, center_x, center_y, clip_result);

  gimp_image_undo_group_end (gimage);

  /*  Make sure we're not caching any old selection info  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));
}

void
gimp_text_layer_transform (GimpItem               *item,
			   const GimpMatrix3      *matrix,
			   GimpTransformDirection  direction,
			   GimpInterpolationType   interpolation_type,
			   gboolean                clip_result,
			   GimpProgressFunc        progress_callback,
			   gpointer                progress_data)
{

}
