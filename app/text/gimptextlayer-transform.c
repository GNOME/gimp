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


void
gimp_text_layer_scale (GimpItem               *item,
                       gint                    new_width,
                       gint                    new_height,
                       gint                    new_offset_x,
                       gint                    new_offset_y,
                       GimpInterpolationType   interpolation_type,
                       GimpProgress           *progress)
{
  /*  TODO  */
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

  if (! gimp_text_layer_transform_rotate (layer,
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

void
gimp_text_layer_transform (GimpItem               *item,
                           GimpContext            *context,
                           const GimpMatrix3      *matrix,
                           GimpTransformDirection  direction,
                           GimpInterpolationType   interpolation_type,
                           gboolean                supersample,
                           GimpTransformResize     clip_result,
                           GimpProgress           *progress)
{
  /*  TODO  */
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

  return TRUE;
}

static gboolean
gimp_text_layer_set_transformation (GimpTextLayer *layer,
                                    GimpMatrix3   *matrix)
{
  GimpMatrix2  trafo;

  if (! gimp_matrix3_is_affine (matrix))
    return FALSE;

  trafo.coeff[0][0] = matrix->coeff[0][0];
  trafo.coeff[0][1] = matrix->coeff[0][1];
  trafo.coeff[1][0] = matrix->coeff[1][0];
  trafo.coeff[1][1] = matrix->coeff[1][1];

  gimp_text_layer_set (GIMP_TEXT_LAYER (layer), NULL,
                       "transformation", &trafo,
                       "offset-x",       matrix->coeff[0][2],
                       "offset-y",       matrix->coeff[1][2],
                       NULL);

  return TRUE;
}
