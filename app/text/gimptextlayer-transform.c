/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextLayer
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "text-types.h"

#include "core/gimp-transform-resize.h"
#include "core/gimp-transform-utils.h"
#include "core/gimpimage-undo.h"

#include "gimptext.h"
#include "gimptextlayer.h"
#include "gimptextlayer-transform.h"


static GimpItemClass * gimp_text_layer_parent_class (void) G_GNUC_CONST;

static gboolean  gimp_text_layer_get_transformation  (GimpTextLayer *layer,
                                                      GimpMatrix3   *matrix);
static gboolean  gimp_text_layer_set_transformation  (GimpTextLayer *layer,
                                                      GimpMatrix3   *matrix);


static gboolean
gimp_text_layer_transform_scale (GimpTextLayer *layer,
                                 gint           new_width,
                                 gint           new_height,
                                 gint           new_offset_x,
                                 gint           new_offset_y)
{
  GimpItem    *item = GIMP_ITEM (layer);
  GimpMatrix3   matrix;
  GimpMatrix3   scale;

  if (! gimp_text_layer_get_transformation (layer, &matrix))
    return FALSE;

  gimp_transform_matrix_scale (&scale,
                               gimp_item_get_offset_x (item),
                               gimp_item_get_offset_y (item),
                               gimp_item_get_width (item),
                               gimp_item_get_height (item),
                               new_offset_x, new_offset_y,
                               new_width, new_height);

  gimp_matrix3_mult (&scale, &matrix);

  return gimp_text_layer_set_transformation (layer, &matrix);
}

void
gimp_text_layer_scale (GimpItem               *item,
                       gint                    new_width,
                       gint                    new_height,
                       gint                    new_offset_x,
                       gint                    new_offset_y,
                       GimpInterpolationType   interpolation_type,
                       GimpProgress           *progress)
{
  GimpTextLayer *layer = GIMP_TEXT_LAYER (item);

  if (gimp_text_layer_transform_scale (layer, new_width, new_height,
                                       new_offset_x, new_offset_y))
    {
      GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (layer));

      if (mask)
        gimp_item_scale (GIMP_ITEM (mask),
                         new_width, new_height,
                         new_offset_x, new_offset_y,
                         interpolation_type, progress);
    }
  else
    {
      gimp_text_layer_parent_class ()->scale (item, new_width, new_height,
                                              new_offset_x, new_offset_y,
                                              interpolation_type, progress);
    }
}

static gboolean
gimp_text_layer_transform_flip (GimpTextLayer       *layer,
                                GimpOrientationType  flip_type,
                                gdouble              axis)
{
  GimpMatrix3  matrix;

  if (! gimp_text_layer_get_transformation (layer, &matrix))
    return FALSE;

  gimp_transform_matrix_flip (&matrix, flip_type, axis);

  return gimp_text_layer_set_transformation (layer, &matrix);
}

void
gimp_text_layer_flip (GimpItem            *item,
                      GimpContext         *context,
                      GimpOrientationType  flip_type,
                      gdouble              axis,
                      gboolean             clip_result)
{
  GimpTextLayer *layer = GIMP_TEXT_LAYER (item);

  if (gimp_text_layer_transform_flip (layer, flip_type, axis))
    {
      GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (layer));

      if (mask)
        gimp_item_flip (GIMP_ITEM (mask), context,
                        flip_type, axis, clip_result);
    }
  else
    {
      gimp_text_layer_parent_class ()->flip (item, context,
                                             flip_type, axis, clip_result);
    }
}

static gboolean
gimp_text_layer_transform_rotate (GimpTextLayer    *layer,
                                  GimpRotationType  rotate_type,
                                  gdouble           center_x,
                                  gdouble           center_y)
{
  GimpMatrix3  matrix;

  if (! gimp_text_layer_get_transformation (layer, &matrix))
    return FALSE;

  gimp_transform_matrix_rotate (&matrix, rotate_type, center_x, center_y);

  return gimp_text_layer_set_transformation (layer, &matrix);
}

void
gimp_text_layer_rotate (GimpItem         *item,
                        GimpContext      *context,
                        GimpRotationType  rotate_type,
                        gdouble           center_x,
                        gdouble           center_y,
                        gboolean          clip_result)
{
  GimpTextLayer *layer = GIMP_TEXT_LAYER (item);

  if (gimp_text_layer_transform_rotate (layer,
                                        rotate_type, center_x, center_y))
    {
      GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (layer));

      if (mask)
        gimp_item_rotate (GIMP_ITEM (mask), context,
                          rotate_type, center_x, center_y, clip_result);
    }
  else
    {
      gimp_text_layer_parent_class ()->rotate (item, context,
                                               rotate_type, center_x, center_y,
                                               clip_result);
    }
}

static gboolean
gimp_text_layer_transform_matrix (GimpTextLayer          *layer,
                                  const GimpMatrix3      *matrix,
                                  GimpTransformDirection  direction)
{
  GimpMatrix3 left = *matrix;
  GimpMatrix3 right;

  if (! gimp_text_layer_get_transformation (layer, &right))
    return FALSE;

  if (direction == GIMP_TRANSFORM_BACKWARD)
    gimp_matrix3_invert (&left);

  gimp_matrix3_mult (&left, &right);

  return gimp_text_layer_set_transformation (layer, &right);
}

void
gimp_text_layer_transform (GimpItem               *item,
                           GimpContext            *context,
                           const GimpMatrix3      *matrix,
                           GimpTransformDirection  direction,
                           GimpInterpolationType   interpolation_type,
                           GimpTransformResize     clip_result,
                           GimpProgress           *progress)
{
  GimpTextLayer *layer = GIMP_TEXT_LAYER (item);

  if (gimp_text_layer_transform_matrix (layer, matrix, direction))
    {
      GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (layer));

      if (mask)
        gimp_item_transform (GIMP_ITEM (mask), context,
                             matrix, direction, interpolation_type,
                             clip_result, progress);
    }
  else
    {
      gimp_text_layer_parent_class ()->transform (item, context,
                                                  matrix, direction,
                                                  interpolation_type,
                                                  clip_result, progress);
    }
}

static GimpItemClass *
gimp_text_layer_parent_class (void)
{
  static GimpItemClass *parent_class = NULL;

  if (! parent_class)
    {
      gpointer klass = g_type_class_peek (GIMP_TYPE_TEXT_LAYER);

      parent_class = g_type_class_peek_parent (klass);
    }

  return parent_class;
}

static gboolean
gimp_text_layer_get_transformation (GimpTextLayer *layer,
                                    GimpMatrix3   *matrix)
{
  if (! layer->text || layer->modified)
    return FALSE;

  gimp_text_get_transformation (layer->text, matrix);

  gimp_matrix3_translate (matrix,
                          gimp_item_get_offset_x (GIMP_ITEM (layer)),
                          gimp_item_get_offset_y (GIMP_ITEM (layer)));


  return TRUE;
}

static gboolean
gimp_text_layer_set_transformation (GimpTextLayer *layer,
                                    GimpMatrix3   *matrix)
{
  GimpMatrix2 trafo;
  gint        x;
  gint        y;
  gint        unused;

  if (! gimp_matrix3_is_affine (matrix))
    return FALSE;

  gimp_transform_resize_boundary (matrix,
                                  GIMP_TRANSFORM_RESIZE_ADJUST,
                                  0, 0, layer->layout_width,
                                  layer->layout_height, &x, &y,
                                  &unused, &unused);


  gimp_item_translate (GIMP_ITEM (layer),
                       x - gimp_item_get_offset_x (GIMP_ITEM (layer)),
                       y - gimp_item_get_offset_y (GIMP_ITEM (layer)),
                       FALSE);

  gimp_matrix3_translate (matrix,
                          - gimp_item_get_offset_x (GIMP_ITEM (layer)),
                          - gimp_item_get_offset_y (GIMP_ITEM (layer)));

  trafo.coeff[0][0] = matrix->coeff[0][0];
  trafo.coeff[0][1] = matrix->coeff[0][1];
  trafo.coeff[1][0] = matrix->coeff[1][0];
  trafo.coeff[1][1] = matrix->coeff[1][1];

  gimp_text_layer_set (layer, NULL,
                       "transformation", &trafo,
                       "offset-x",       matrix->coeff[0][2],
                       "offset-y",       matrix->coeff[1][2],
                       NULL);

  return TRUE;
}
