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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-blend.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


typedef struct
{
  GimpGradient     *gradient;
  GimpContext      *context;
  gboolean          reverse;
#ifdef USE_GRADIENT_CACHE
  GimpRGB          *gradient_cache;
  gint              gradient_cache_size;
#endif
  gdouble           offset;
  gdouble           sx, sy;
  GimpGradientType  gradient_type;
  gdouble           dist;
  gdouble           vec[2];
  GimpRepeatMode    repeat;
  GRand            *seed;
  GeglBuffer       *dist_buffer;
} RenderBlendData;

typedef struct
{
  GeglBuffer    *buffer;
  gfloat        *row_data;
  gint           width;
  GRand         *dither_rand;
} PutPixelData;

/*  public functions  */

void
gimp_drawable_blend (GimpDrawable     *drawable,
                     GimpContext      *context,
                     GimpGradient     *gradient,
                     GimpLayerMode     paint_mode,
                     GimpGradientType  gradient_type,
                     gdouble           opacity,
                     gdouble           offset,
                     GimpRepeatMode    repeat,
                     gboolean          reverse,
                     gboolean          supersample,
                     gint              max_depth,
                     gdouble           threshold,
                     gboolean          dither,
                     gdouble           startx,
                     gdouble           starty,
                     gdouble           endx,
                     gdouble           endy,
                     GimpProgress     *progress)
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
        gimp_drawable_blend_shapeburst_distmap (drawable, TRUE,
                                                GEGL_RECTANGLE (x, y, width, height),
                                                progress);
    }

  render = gegl_node_new_child (NULL,
                                "operation",             "gimp:blend",
                                "context",               context,
                                "gradient",              gradient,
                                "start-x",               startx,
                                "start-y",               starty,
                                "end-x",                 endx,
                                "end-y",                 endy,
                                "gradient-type",         gradient_type,
                                "gradient-repeat",       repeat,
                                "offset",                offset,
                                "gradient-reverse",      reverse,
                                "supersample",           supersample,
                                "supersample-depth",     max_depth,
                                "supersample-threshold", threshold,
                                "dither",                dither,
                                NULL);

  gimp_gegl_apply_operation (shapeburst, progress, NULL,
                             render,
                             buffer, GEGL_RECTANGLE (x, y, width, height));

  g_object_unref (render);

  if (shapeburst)
    g_object_unref (shapeburst);

  gimp_drawable_apply_buffer (drawable, buffer,
                              GEGL_RECTANGLE (x, y, width, height),
                              TRUE, C_("undo-type", "Blend"),
                              opacity, paint_mode,
                              NULL, x, y);

  gimp_drawable_update (drawable, x, y, width, height);

  g_object_unref (buffer);

  gimp_unset_busy (image->gimp);
}

GeglBuffer *
gimp_drawable_blend_shapeburst_distmap (GimpDrawable        *drawable,
                                        gboolean             legacy_shapeburst,
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
      gegl_buffer_copy (gimp_drawable_get_buffer (GIMP_DRAWABLE (mask)),
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
          gegl_buffer_copy (gimp_drawable_get_buffer (drawable), region,
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

  if (legacy_shapeburst)
    shapeburst = gegl_node_new_child (NULL,
                                      "operation", "gimp:shapeburst",
                                      "normalize", TRUE,
                                      NULL);
  else
    shapeburst = gegl_node_new_child (NULL,
                                      "operation", "gegl:distance-transform",
                                      "normalize", TRUE,
                                      NULL);

  if (progress)
    gimp_gegl_progress_connect (shapeburst, progress,
                                _("Calculating distance map"));

  gimp_gegl_apply_operation (temp_buffer, NULL, NULL,
                             shapeburst,
                             dist_buffer, region);

  g_object_unref (shapeburst);

  g_object_unref (temp_buffer);

  return dist_buffer;
}
