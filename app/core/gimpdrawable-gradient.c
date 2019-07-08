/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-gradient.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_gradient (GimpDrawable                *drawable,
                        GimpContext                 *context,
                        GimpGradient                *gradient,
                        GeglDistanceMetric           metric,
                        GimpLayerMode                paint_mode,
                        GimpGradientType             gradient_type,
                        gdouble                      opacity,
                        gdouble                      offset,
                        GimpRepeatMode               repeat,
                        gboolean                     reverse,
                        GimpGradientBlendColorSpace  blend_color_space,
                        gboolean                     supersample,
                        gint                         max_depth,
                        gdouble                      threshold,
                        gboolean                     dither,
                        gdouble                      startx,
                        gdouble                      starty,
                        gdouble                      endx,
                        gdouble                      endy,
                        GimpProgress                *progress)
{
  GimpImage  *image;
  GeglBuffer *buffer;
  GeglBuffer *shapeburst = NULL;
  GeglNode   *render;
  gint        x, y, width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &width, &height))
    return;

  gimp_set_busy (image->gimp);

  /*  Always create an alpha temp buf (for generality) */
  buffer = gegl_buffer_new (GEGL_RECTANGLE (x, y, width, height),
                            gimp_drawable_get_format_with_alpha (drawable));

  if (gradient_type >= GIMP_GRADIENT_SHAPEBURST_ANGULAR &&
      gradient_type <= GIMP_GRADIENT_SHAPEBURST_DIMPLED)
    {
      shapeburst =
        gimp_drawable_gradient_shapeburst_distmap (drawable, metric,
                                                   GEGL_RECTANGLE (x, y, width, height),
                                                   progress);
    }

  gimp_drawable_gradient_adjust_coords (drawable,
                                        gradient_type,
                                        GEGL_RECTANGLE (x, y, width, height),
                                        &startx, &starty, &endx, &endy);

  render = gegl_node_new_child (NULL,
                                "operation",                  "gimp:gradient",
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

  gimp_gegl_apply_operation (shapeburst, progress, C_("undo-type", "Gradient"),
                             render,
                             buffer, GEGL_RECTANGLE (x, y, width, height),
                             FALSE);

  g_object_unref (render);

  if (shapeburst)
    g_object_unref (shapeburst);

  gimp_drawable_apply_buffer (drawable, buffer,
                              GEGL_RECTANGLE (x, y, width, height),
                              TRUE, C_("undo-type", "Gradient"),
                              opacity, paint_mode,
                              GIMP_LAYER_COLOR_SPACE_AUTO,
                              GIMP_LAYER_COLOR_SPACE_AUTO,
                              gimp_layer_mode_get_paint_composite_mode (paint_mode),
                              NULL, x, y);

  gimp_drawable_update (drawable, x, y, width, height);

  g_object_unref (buffer);

  gimp_unset_busy (image->gimp);
}

GeglBuffer *
gimp_drawable_gradient_shapeburst_distmap (GimpDrawable        *drawable,
                                           GeglDistanceMetric   metric,
                                           const GeglRectangle *region,
                                           GimpProgress        *progress)
{
  GimpChannel *mask;
  GimpImage   *image;
  GeglBuffer  *dist_buffer;
  GeglBuffer  *temp_buffer;
  GeglNode    *shapeburst;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  allocate the distance map  */
  dist_buffer = gegl_buffer_new (region, babl_format ("Y float"));

  /*  allocate the selection mask copy  */
  temp_buffer = gegl_buffer_new (region, babl_format ("Y float"));

  mask = gimp_image_get_mask (image);

  /*  If the image mask is not empty, use it as the shape burst source  */
  if (! gimp_channel_is_empty (mask))
    {
      gint x, y, width, height;
      gint off_x, off_y;

      gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &width, &height);
      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

      /*  copy the mask to the temp mask  */
      gimp_gegl_buffer_copy (
        gimp_drawable_get_buffer (GIMP_DRAWABLE (mask)),
        GEGL_RECTANGLE (x + off_x, y + off_y, width, height),
        GEGL_ABYSS_NONE, temp_buffer, region);
    }
  else
    {
      /*  If the intended drawable has an alpha channel, use that  */
      if (gimp_drawable_has_alpha (drawable))
        {
          const Babl *component_format;

          component_format = babl_format ("A float");

          /*  extract the aplha into the temp mask  */
          gegl_buffer_set_format (temp_buffer, component_format);
          gimp_gegl_buffer_copy (gimp_drawable_get_buffer (drawable), region,
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
    gimp_gegl_progress_connect (shapeburst, progress,
                                _("Calculating distance map"));

  gimp_gegl_apply_operation (temp_buffer, NULL, NULL,
                             shapeburst,
                             dist_buffer, region, FALSE);

  g_object_unref (shapeburst);

  g_object_unref (temp_buffer);

  return dist_buffer;
}

void
gimp_drawable_gradient_adjust_coords (GimpDrawable        *drawable,
                                      GimpGradientType     gradient_type,
                                      const GeglRectangle *region,
                                      gdouble             *startx,
                                      gdouble             *starty,
                                      gdouble             *endx,
                                      gdouble             *endy)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (region != NULL);
  g_return_if_fail (startx != NULL);
  g_return_if_fail (starty != NULL);
  g_return_if_fail (endx != NULL);
  g_return_if_fail (endy != NULL);

  /* we potentially adjust the gradient coordinates according to the gradient
   * type, so that in cases where the gradient span is not related to the
   * segment length, the gradient cache (in GimpOperationGradient) is big
   * enough not to produce banding.
   */

  switch (gradient_type)
    {
    /* for conical gradients, use a segment with the original origin and
     * direction, whose length is the circumference of the largest circle
     * centered at the origin, passing through one of the regions's vertices.
     */
    case GIMP_GRADIENT_CONICAL_SYMMETRIC:
    case GIMP_GRADIENT_CONICAL_ASYMMETRIC:
      {
        gdouble     r = 0.0;
        GimpVector2 v;

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
        if (gradient_type == GIMP_GRADIENT_CONICAL_SYMMETRIC)
          r /= 2.0;

        gimp_vector2_set (&v, *endx - *startx, *endy - *starty);
        gimp_vector2_normalize (&v);
        gimp_vector2_mul (&v, 2.0 * G_PI * r);

        *endx = *startx + v.x;
        *endy = *starty + v.y;
      }
      break;

    /* for shaped gradients, only the segment's length matters; use the
     * regions's diagonal, which is the largest possible distance between two
     * points in the region.
     */
    case GIMP_GRADIENT_SHAPEBURST_ANGULAR:
    case GIMP_GRADIENT_SHAPEBURST_SPHERICAL:
    case GIMP_GRADIENT_SHAPEBURST_DIMPLED:
      *startx = region->x;
      *starty = region->y;
      *endx   = region->x + region->width;
      *endy   = region->y + region->height;
      break;

    default:
      break;
    }
}
