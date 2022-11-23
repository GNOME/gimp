/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextLayer
 * Copyright (C) 2002-2003  Sven Neumann <sven@ligma.org>
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

#include "libligmamath/ligmamath.h"

#include "text-types.h"

#include "core/ligma-transform-utils.h"
#include "core/ligmaimage-undo.h"

#include "ligmatext.h"
#include "ligmatextlayer.h"
#include "ligmatextlayer-transform.h"


static LigmaItemClass * ligma_text_layer_parent_class (void) G_GNUC_CONST;

static gboolean  ligma_text_layer_get_transformation  (LigmaTextLayer *layer,
                                                      LigmaMatrix3   *matrix);
static gboolean  ligma_text_layer_set_transformation  (LigmaTextLayer *layer,
                                                      LigmaMatrix3   *matrix);


void
ligma_text_layer_scale (LigmaItem               *item,
                       gint                    new_width,
                       gint                    new_height,
                       gint                    new_offset_x,
                       gint                    new_offset_y,
                       LigmaInterpolationType   interpolation_type,
                       LigmaProgress           *progress)
{
  /*  TODO  */
}

static gboolean
ligma_text_layer_transform_flip (LigmaTextLayer       *layer,
                                LigmaOrientationType  flip_type,
                                gdouble              axis)
{
  LigmaMatrix3  matrix;

  if (! ligma_text_layer_get_transformation (layer, &matrix))
    return FALSE;

  ligma_transform_matrix_flip (&matrix, flip_type, axis);

  return ligma_text_layer_set_transformation (layer, &matrix);
}

void
ligma_text_layer_flip (LigmaItem            *item,
                      LigmaContext         *context,
                      LigmaOrientationType  flip_type,
                      gdouble              axis,
                      gboolean             clip_result)
{
  LigmaTextLayer *layer = LIGMA_TEXT_LAYER (item);

  if (ligma_text_layer_transform_flip (layer, flip_type, axis))
    {
      LigmaLayerMask *mask = ligma_layer_get_mask (LIGMA_LAYER (layer));

      if (mask)
        ligma_item_flip (LIGMA_ITEM (mask), context,
                        flip_type, axis, clip_result);
    }
  else
    {
      ligma_text_layer_parent_class ()->flip (item, context,
                                             flip_type, axis, clip_result);
    }
}

static gboolean
ligma_text_layer_transform_rotate (LigmaTextLayer    *layer,
                                  LigmaRotationType  rotate_type,
                                  gdouble           center_x,
                                  gdouble           center_y)
{
  LigmaMatrix3  matrix;

  if (! ligma_text_layer_get_transformation (layer, &matrix))
    return FALSE;

  ligma_transform_matrix_rotate (&matrix, rotate_type, center_x, center_y);

  return ligma_text_layer_set_transformation (layer, &matrix);
}

void
ligma_text_layer_rotate (LigmaItem         *item,
                        LigmaContext      *context,
                        LigmaRotationType  rotate_type,
                        gdouble           center_x,
                        gdouble           center_y,
                        gboolean          clip_result)
{
  LigmaTextLayer *layer = LIGMA_TEXT_LAYER (item);

  if (! ligma_text_layer_transform_rotate (layer,
                                          rotate_type, center_x, center_y))
    {
      LigmaLayerMask *mask = ligma_layer_get_mask (LIGMA_LAYER (layer));

      if (mask)
        ligma_item_rotate (LIGMA_ITEM (mask), context,
                          rotate_type, center_x, center_y, clip_result);
    }
  else
    {
      ligma_text_layer_parent_class ()->rotate (item, context,
                                               rotate_type, center_x, center_y,
                                               clip_result);
    }
}

void
ligma_text_layer_transform (LigmaItem               *item,
                           LigmaContext            *context,
                           const LigmaMatrix3      *matrix,
                           LigmaTransformDirection  direction,
                           LigmaInterpolationType   interpolation_type,
                           gboolean                supersample,
                           LigmaTransformResize     clip_result,
                           LigmaProgress           *progress)
{
  /*  TODO  */
}

static LigmaItemClass *
ligma_text_layer_parent_class (void)
{
  static LigmaItemClass *parent_class = NULL;

  if (! parent_class)
    {
      gpointer klass = g_type_class_peek (LIGMA_TYPE_TEXT_LAYER);

      parent_class = g_type_class_peek_parent (klass);
    }

  return parent_class;
}

static gboolean
ligma_text_layer_get_transformation (LigmaTextLayer *layer,
                                    LigmaMatrix3   *matrix)
{
  if (! layer->text || layer->modified)
    return FALSE;

  ligma_text_get_transformation (layer->text, matrix);

  return TRUE;
}

static gboolean
ligma_text_layer_set_transformation (LigmaTextLayer *layer,
                                    LigmaMatrix3   *matrix)
{
  LigmaMatrix2  trafo;

  if (! ligma_matrix3_is_affine (matrix))
    return FALSE;

  trafo.coeff[0][0] = matrix->coeff[0][0];
  trafo.coeff[0][1] = matrix->coeff[0][1];
  trafo.coeff[1][0] = matrix->coeff[1][0];
  trafo.coeff[1][1] = matrix->coeff[1][1];

  ligma_text_layer_set (LIGMA_TEXT_LAYER (layer), NULL,
                       "transformation", &trafo,
                       "offset-x",       matrix->coeff[0][2],
                       "offset-y",       matrix->coeff[1][2],
                       NULL);

  return TRUE;
}
