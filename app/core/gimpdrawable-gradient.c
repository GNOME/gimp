/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "gegl/ligma-gegl-apply-operation.h"
#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-utils.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "ligma.h"
#include "ligmachannel.h"
#include "ligmacontext.h"
#include "ligmadrawable-gradient.h"
#include "ligmagradient.h"
#include "ligmaimage.h"
#include "ligmaprogress.h"

#include "ligma-intl.h"


/*  public functions  */

void
ligma_drawable_gradient (LigmaDrawable                *drawable,
                        LigmaContext                 *context,
                        LigmaGradient                *gradient,
                        GeglDistanceMetric           metric,
                        LigmaLayerMode                paint_mode,
                        LigmaGradientType             gradient_type,
                        gdouble                      opacity,
                        gdouble                      offset,
                        LigmaRepeatMode               repeat,
                        gboolean                     reverse,
                        LigmaGradientBlendColorSpace  blend_color_space,
                        gboolean                     supersample,
                        gint                         max_depth,
                        gdouble                      threshold,
                        gboolean                     dither,
                        gdouble                      startx,
                        gdouble                      starty,
                        gdouble                      endx,
                        gdouble                      endy,
                        LigmaProgress                *progress)
{
  LigmaImage  *image;
  GeglBuffer *buffer;
  GeglBuffer *shapeburst = NULL;
  GeglNode   *render;
  gint        x, y, width, height;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (LIGMA_IS_GRADIENT (gradient));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  image = ligma_item_get_image (LIGMA_ITEM (drawable));

  if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable), &x, &y, &width, &height))
    return;

  ligma_set_busy (image->ligma);

  /*  Always create an alpha temp buf (for generality) */
  buffer = gegl_buffer_new (GEGL_RECTANGLE (x, y, width, height),
                            ligma_drawable_get_format_with_alpha (drawable));

  if (gradient_type >= LIGMA_GRADIENT_SHAPEBURST_ANGULAR &&
      gradient_type <= LIGMA_GRADIENT_SHAPEBURST_DIMPLED)
    {
      shapeburst =
        ligma_drawable_gradient_shapeburst_distmap (drawable, metric,
                                                   GEGL_RECTANGLE (x, y, width, height),
                                                   progress);
    }

  ligma_drawable_gradient_adjust_coords (drawable,
                                        gradient_type,
                                        GEGL_RECTANGLE (x, y, width, height),
                                        &startx, &starty, &endx, &endy);

  render = gegl_node_new_child (NULL,
                                "operation",                  "ligma:gradient",
                                "context",                    context,
                                "gradient",                   gradient,
                                "start-x",                    startx,
                                "start-y",                    starty,
                                "end-x",                      endx,
                                "end-y",                      endy,
                                "gradient-type",              gradient_type,
                                "gradient-repeat",            repeat,
                                "offset",                     offset,
                                "gradient-reverse",           reverse,
                                "gradient-blend-color-space", blend_color_space,
                                "supersample",                supersample,
                                "supersample-depth",          max_depth,
                                "supersample-threshold",      threshold,
                                "dither",                     dither,
                                NULL);

  ligma_gegl_apply_operation (shapeburst, progress, C_("undo-type", "Gradient"),
                             render,
                             buffer, GEGL_RECTANGLE (x, y, width, height),
                             FALSE);

  g_object_unref (render);

  if (shapeburst)
    g_object_unref (shapeburst);

  ligma_drawable_apply_buffer (drawable, buffer,
                              GEGL_RECTANGLE (x, y, width, height),
                              TRUE, C_("undo-type", "Gradient"),
                              opacity, paint_mode,
                              LIGMA_LAYER_COLOR_SPACE_AUTO,
                              LIGMA_LAYER_COLOR_SPACE_AUTO,
                              ligma_layer_mode_get_paint_composite_mode (paint_mode),
                              NULL, x, y);

  ligma_drawable_update (drawable, x, y, width, height);

  g_object_unref (buffer);

  ligma_unset_busy (image->ligma);
}

GeglBuffer *
ligma_drawable_gradient_shapeburst_distmap (LigmaDrawable        *drawable,
                                           GeglDistanceMetric   metric,
                                           const GeglRectangle *region,
                                           LigmaProgress        *progress)
{
  LigmaChannel *mask;
  LigmaImage   *image;
  GeglBuffer  *dist_buffer;
  GeglBuffer  *temp_buffer;
  GeglNode    *shapeburst;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);

  image = ligma_item_get_image (LIGMA_ITEM (drawable));

  /*  allocate the distance map  */
  dist_buffer = gegl_buffer_new (region, babl_format ("Y float"));

  /*  allocate the selection mask copy  */
  temp_buffer = gegl_buffer_new (region, babl_format ("Y float"));

  mask = ligma_image_get_mask (image);

  /*  If the image mask is not empty, use it as the shape burst source  */
  if (! ligma_channel_is_empty (mask))
    {
      gint x, y, width, height;
      gint off_x, off_y;

      ligma_item_mask_intersect (LIGMA_ITEM (drawable), &x, &y, &width, &height);
      ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

      /*  copy the mask to the temp mask  */
      ligma_gegl_buffer_copy (
        ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask)),
        GEGL_RECTANGLE (x + off_x, y + off_y, width, height),
        GEGL_ABYSS_NONE, temp_buffer, region);
    }
  else
    {
      /*  If the intended drawable has an alpha channel, use that  */
      if (ligma_drawable_has_alpha (drawable))
        {
          const Babl *component_format;

          component_format = babl_format ("A float");

          /*  extract the alpha into the temp mask  */
          gegl_buffer_set_format (temp_buffer, component_format);
          ligma_gegl_buffer_copy (ligma_drawable_get_buffer (drawable), region,
                                 GEGL_ABYSS_NONE,
                                 temp_buffer, region);
          gegl_buffer_set_format (temp_buffer, NULL);
        }
      else
        {
          GeglColor *white = gegl_color_new ("white");

          /*  Otherwise, just fill the shapeburst to white  */
          gegl_buffer_set_color (temp_buffer, NULL, white);
          g_object_unref (white);
        }
    }

  shapeburst = gegl_node_new_child (NULL,
                                    "operation", "gegl:distance-transform",
                                    "normalize", TRUE,
                                    "metric",    metric,
                                    NULL);

  if (progress)
    ligma_gegl_progress_connect (shapeburst, progress,
                                _("Calculating distance map"));

  ligma_gegl_apply_operation (temp_buffer, NULL, NULL,
                             shapeburst,
                             dist_buffer, region, FALSE);

  g_object_unref (shapeburst);

  g_object_unref (temp_buffer);

  return dist_buffer;
}

void
ligma_drawable_gradient_adjust_coords (LigmaDrawable        *drawable,
                                      LigmaGradientType     gradient_type,
                                      const GeglRectangle *region,
                                      gdouble             *startx,
                                      gdouble             *starty,
                                      gdouble             *endx,
                                      gdouble             *endy)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (region != NULL);
  g_return_if_fail (startx != NULL);
  g_return_if_fail (starty != NULL);
  g_return_if_fail (endx != NULL);
  g_return_if_fail (endy != NULL);

  /* we potentially adjust the gradient coordinates according to the gradient
   * type, so that in cases where the gradient span is not related to the
   * segment length, the gradient cache (in LigmaOperationGradient) is big
   * enough not to produce banding.
   */

  switch (gradient_type)
    {
    /* for conical gradients, use a segment with the original origin and
     * direction, whose length is the circumference of the largest circle
     * centered at the origin, passing through one of the regions's vertices.
     */
    case LIGMA_GRADIENT_CONICAL_SYMMETRIC:
    case LIGMA_GRADIENT_CONICAL_ASYMMETRIC:
      {
        gdouble     r = 0.0;
        LigmaVector2 v;

        r = MAX (r, hypot (region->x - *startx,
                           region->y - *starty));
        r = MAX (r, hypot (region->x + region->width - *startx,
                           region->y - *starty));
        r = MAX (r, hypot (region->x - *startx,
                           region->y + region->height - *starty));
        r = MAX (r, hypot (region->x + region->width - *startx,
                           region->y + region->height - *starty));

        /* symmetric conical gradients only span half a revolution, and
         * therefore require only half the cache size.
         */
        if (gradient_type == LIGMA_GRADIENT_CONICAL_SYMMETRIC)
          r /= 2.0;

        ligma_vector2_set (&v, *endx - *startx, *endy - *starty);
        ligma_vector2_normalize (&v);
        ligma_vector2_mul (&v, 2.0 * G_PI * r);

        *endx = *startx + v.x;
        *endy = *starty + v.y;
      }
      break;

    /* for shaped gradients, only the segment's length matters; use the
     * regions's diagonal, which is the largest possible distance between two
     * points in the region.
     */
    case LIGMA_GRADIENT_SHAPEBURST_ANGULAR:
    case LIGMA_GRADIENT_SHAPEBURST_SPHERICAL:
    case LIGMA_GRADIENT_SHAPEBURST_DIMPLED:
      *startx = region->x;
      *starty = region->y;
      *endx   = region->x + region->width;
      *endy   = region->y + region->height;
      break;

    default:
      break;
    }
}
