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

#include "text-types.h"

#include "core/gimpimage-undo.h"

#include "gimptext.h"
#include "gimptextlayer.h"
#include "gimptextlayer-transform.h"



static void  gimp_text_layer_get_trafo (GimpTextLayer *layer,
                                        GimpMatrix2   *trafo);


void
gimp_text_layer_scale (GimpItem               *item,
                       gint                    new_width,
                       gint                    new_height,
                       gint                    new_offset_x,
                       gint                    new_offset_y,
                       GimpInterpolationType   interpolation_type,
                       GimpProgressFunc        progress_callback,
                       gpointer                progress_data)
{

}

void
gimp_text_layer_flip (GimpItem            *item,
                      GimpContext         *context,
                      GimpOrientationType  flip_type,
                      gdouble              axis,
                      gboolean             clip_result)
{
  GimpLayer   *layer = GIMP_LAYER (item);
  GimpMatrix2  text_trafo;
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

  gimp_text_layer_get_trafo (GIMP_TEXT_LAYER (layer), &text_trafo);
  gimp_matrix2_mult (&trafo, &text_trafo);

  gimp_text_layer_set (GIMP_TEXT_LAYER (layer), NULL,
                       "transformation", &text_trafo,
                       NULL);

  if (layer->mask)
    gimp_item_flip (GIMP_ITEM (layer->mask), context,
                    flip_type, axis, clip_result);
}

void
gimp_text_layer_rotate (GimpItem         *item,
                        GimpContext      *context,
                        GimpRotationType  rotate_type,
                        gdouble           center_x,
                        gdouble           center_y,
                        gboolean          clip_result)
{
  GimpLayer   *layer = GIMP_LAYER (item);
  GimpMatrix2  text_trafo;
  GimpMatrix2  trafo;
  gdouble      cos   = 1.0;
  gdouble      sin   = 0.0;

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

  trafo.coeff[0][0] = cos;
  trafo.coeff[0][1] = -sin;
  trafo.coeff[1][0] = sin;
  trafo.coeff[1][1] = cos;

  gimp_text_layer_get_trafo (GIMP_TEXT_LAYER (layer), &text_trafo);
  gimp_matrix2_mult (&trafo, &text_trafo);

  gimp_text_layer_set (GIMP_TEXT_LAYER (layer), NULL,
                       "transformation", &text_trafo,
                       NULL);

  if (layer->mask)
    gimp_item_rotate (GIMP_ITEM (layer->mask), context,
                      rotate_type, center_x, center_y, clip_result);
}

void
gimp_text_layer_transform (GimpItem               *item,
                           GimpContext            *context,
                           const GimpMatrix3      *matrix,
                           GimpTransformDirection  direction,
                           GimpInterpolationType   interpolation_type,
                           gboolean                supersample,
                           gint                    recursion_level,
                           gboolean                clip_result,
                           GimpProgressFunc        progress_callback,
                           gpointer                progress_data)
{

}

static void
gimp_text_layer_get_trafo (GimpTextLayer *layer,
                           GimpMatrix2   *trafo)
{
  *trafo = layer->text->transformation;
}
