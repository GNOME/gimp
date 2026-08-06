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
#include "core/gimprasterizable.h"

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
  GimpTextLayer *layer  = GIMP_TEXT_LAYER (item);
  GimpLayerMask *mask   = gimp_layer_get_mask (GIMP_LAYER (layer));
  GimpMatrix3    current;
  GimpMatrix3    scale;
  gint           width  = gimp_item_get_width  (item);
  gint           height = gimp_item_get_height (item);
  gint           ox;
  gint           oy;

  if ((width <= 0     || height <= 0)     ||
      (new_width <= 0 || new_height <= 0) ||
      ! gimp_text_layer_get_transformation (layer, &current))
    goto destructive;

  gimp_matrix3_identity (&scale);
  gimp_matrix3_scale (&scale,
                      (gdouble) new_width  / (gdouble) width,
                      (gdouble) new_height / (gdouble) height);

  /*  current := scale * current  */
  gimp_matrix3_mult (&scale, &current);

  if (! gimp_matrix3_is_affine (&current))
    goto destructive;

  current.coeff[0][2] = 0.0;
  current.coeff[1][2] = 0.0;

  if (gimp_text_layer_set_transformation (layer, &current))
    {
      /*  the glyphs are rendered through the matrix, so the layer lands at
       *  the requested origin even if its bounding box rounds differently
       *  from the requested size.
       */
      gimp_item_get_offset (item, &ox, &oy);
      gimp_item_translate (item, new_offset_x - ox, new_offset_y - oy, FALSE);

      if (mask)
        gimp_item_scale (GIMP_ITEM (mask), new_width, new_height,
                         new_offset_x, new_offset_y,
                         interpolation_type, progress);
      return;
    }

 destructive:
  gimp_text_layer_parent_class ()->scale (item,
                                          new_width, new_height,
                                          new_offset_x, new_offset_y,
                                          interpolation_type, progress);
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
  GimpLayerMask *mask  = gimp_layer_get_mask (GIMP_LAYER (layer));

  if (gimp_text_layer_transform_flip (layer, flip_type, axis))
    {
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
  GimpLayerMask *mask  = gimp_layer_get_mask (GIMP_LAYER (layer));

  if (gimp_text_layer_transform_rotate (layer,
                                        rotate_type, center_x, center_y))
    {
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
                           GimpTransformResize     clip_result,
                           GimpProgress           *progress,
                           gboolean                push_undo)
{
  GimpTextLayer *layer = GIMP_TEXT_LAYER (item);
  GimpLayerMask *mask  = gimp_layer_get_mask (GIMP_LAYER (layer));
  GimpMatrix3    local;
  GimpMatrix3    current;
  gdouble        cx;
  gdouble        cy;
  gint           ox;
  gint           oy;

  if (gimp_matrix3_is_identity (matrix))
    return;

  /*  Clipping depends on the intermediate result of each step, which a
   *  single composed matrix cannot reproduce, so only "adjust" transforms
   *  can be stored non-destructively.  Same reasoning as
   *  gimp_link_layer_transform().
   */
  if (clip_result != GIMP_TRANSFORM_RESIZE_ADJUST ||
      ! gimp_text_layer_get_transformation (layer, &current))
    goto destructive;

  local = *matrix;

  if (direction == GIMP_TRANSFORM_BACKWARD)
    gimp_matrix3_invert (&local);

  /*  Where the centre of the layer ends up in image space.  This has to be
   *  sampled before the matrix is stored, because storing it re-renders and
   *  resizes the layer.
   */
  gimp_item_get_offset (item, &ox, &oy);
  cx = ox + gimp_item_get_width  (item) / 2.0;
  cy = oy + gimp_item_get_height (item) / 2.0;
  gimp_matrix3_transform_point (&local, cx, cy, &cx, &cy);

  /*  compose: current := local * current  */
  gimp_matrix3_mult (&local, &current);

  if (! gimp_matrix3_is_affine (&current))
    goto destructive;

  /*  The translation column never reaches the renderer --
   *  gimp_text_layout_get_transform() hardcodes x0/y0 to zero -- so keep it
   *  out of the stored matrix and honour it by moving the item instead.
   */
  current.coeff[0][2] = 0.0;
  current.coeff[1][2] = 0.0;

  if (gimp_text_layer_set_transformation (layer, &current))
    {
      /*  set_transformation() re-rendered synchronously, so the item is
       *  already at its new size; re-centre it on the transformed centre.
       */
      /*  SIGNED_ROUND, not ROUND: a transformed layer routinely lands at a
       *  negative offset, and ROUND() truncates towards zero there.
       */
      gimp_item_get_offset (item, &ox, &oy);
      gimp_item_translate (item,
                           SIGNED_ROUND (cx - gimp_item_get_width  (item) / 2.0) - ox,
                           SIGNED_ROUND (cy - gimp_item_get_height (item) / 2.0) - oy,
                           push_undo);

      if (mask)
        gimp_item_transform (GIMP_ITEM (mask), context, matrix, direction,
                             interpolation_type, clip_result, progress);
      return;
    }

 destructive:
  gimp_text_layer_parent_class ()->transform (item, context,
                                              matrix, direction,
                                              interpolation_type, clip_result,
                                              progress, push_undo);
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
  if (! layer->text || gimp_rasterizable_is_rasterized (GIMP_RASTERIZABLE (layer)))
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
