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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "paint-funcs/paint-funcs.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "core/gimppickable.h"
#include "core/gimpimage.h"
#include "core/gimpdrawable.h"

#include "gimpheal.h"
#include "gimpsourceoptions.h"

#include "gimp-intl.h"


/* NOTES:
 *
 * I had the code working for healing from a pattern, but the results look
 * terrible and I can't see a use for it right now.
 */


static void   gimp_heal_motion (GimpSourceCore   *source_core,
                                GimpDrawable     *drawable,
                                GimpPaintOptions *paint_options);


G_DEFINE_TYPE (GimpHeal, gimp_heal, GIMP_TYPE_SOURCE_CORE)

#define parent_class gimp_heal_parent_class


void
gimp_heal_register (Gimp                      *gimp,
                    GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_HEAL,
                GIMP_TYPE_SOURCE_OPTIONS,
                "gimp-heal",
                _("Heal"),
                "gimp-tool-heal");
}

static void
gimp_heal_class_init (GimpHealClass *klass)
{
  GimpSourceCoreClass *source_core_class = GIMP_SOURCE_CORE_CLASS (klass);

  source_core_class->motion = gimp_heal_motion;
}

static void
gimp_heal_init (GimpHeal *heal)
{
}

/*
 * Substitute any zeros in the input PixelRegion for ones.  This is needed by
 * the algorithm to avoid division by zero, and to get a more realistic image
 * since multiplying by zero is often incorrect (i.e., healing from a dark to
 * a light region will have incorrect spots of zero)
 */
static void
gimp_heal_substitute_0_for_1 (PixelRegion *pr)
{
  gint     i, j, k;

  gint     height = pr->h;
  gint     width  = pr->w;
  gint     depth  = pr->bytes;

  guchar  *pr_data = pr->data;

  guchar  *p;

  for (i = 0; i < height; i++)
    {
      p = pr_data;

      for (j = 0; j < width; j++)
        {
          for (k = 0; k < depth; k++)
            {
              if (p[k] == 0)
                p[k] = 1;
            }

          p += depth;
        }

      pr_data += pr->rowstride;
    }
}

/*
 * Divide topPR by bottomPR and store the result as a double
 */
static void
gimp_heal_divide (PixelRegion *topPR,
                  PixelRegion *bottomPR,
                  gdouble     *result)
{
  gint     i, j, k;

  gint     height = topPR->h;
  gint     width  = topPR->w;
  gint     depth  = topPR->bytes;

  guchar  *t_data = topPR->data;
  guchar  *b_data = bottomPR->data;

  guchar  *t;
  guchar  *b;
  gdouble *r      = result;

  for (i = 0; i < height; i++)
    {
      t = t_data;
      b = b_data;

      for (j = 0; j < width; j++)
        {
          for (k = 0; k < depth; k++)
            {
              r[k] = (gdouble) (t[k]) / (gdouble) (b[k]);
            }

          t += depth;
          b += depth;
          r += depth;
        }

      t_data += topPR->rowstride;
      b_data += bottomPR->rowstride;
    }
}

/*
 * multiply first by secondPR and store the result as a PixelRegion
 */
static void
gimp_heal_multiply (gdouble     *first,
                    PixelRegion *secondPR,
                    PixelRegion *resultPR)
{
  gint     i, j, k;

  gint     height = secondPR->h;
  gint     width  = secondPR->w;
  gint     depth  = secondPR->bytes;

  guchar  *s_data = secondPR->data;
  guchar  *r_data = resultPR->data;

  gdouble *f      = first;
  guchar  *s;
  guchar  *r;

  for (i = 0; i < height; i++)
    {
      s = s_data;
      r = r_data;

      for (j = 0; j < width; j++)
        {
          for (k = 0; k < depth; k++)
            {
              r[k] = (guchar) CLAMP0255 (ROUND (((gdouble) (f[k])) * ((gdouble) (s[k]))));
            }

          f += depth;
          s += depth;
          r += depth;
        }

      s_data += secondPR->rowstride;
      r_data += resultPR->rowstride;
    }
}

/*
 * Perform one iteration of the laplace solver for matrix.  Store the result in
 * solution and return the cummulative error of the solution.
 */
gdouble
gimp_heal_laplace_iteration (gdouble *matrix,
                             gint     height,
                             gint     depth,
                             gint     width,
                             gdouble *solution)
{
  gint     rowstride  = width * depth;
  gint     i, j, k;
  gdouble  err        = 0.0;
  gdouble  tmp, diff;

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
        {
          if ((i == 0) || (i == (height - 1)) || (j == 0) || (j == (height - 1)))
            /* do nothing at the boundary */
            for (k = 0; k < depth; k++)
              *(solution + k) = *(matrix + k);
          else
            {
              /* calculate the mean of four neighbours for all color channels */
              /* v[i][j] = 0.45 * (v[i][j-1]+v[i][j+1]+v[i-1][j]+v[i+1][j]) */
              for (k = 0; k < depth; k++)
                {
                  tmp = *(solution + k);

                  *(solution + k) = 0.25 *
                                    ( *(matrix - depth + k)       /* west */
                                    + *(matrix + depth + k)       /* east */
                                    + *(matrix - rowstride + k)   /* north */
                                    + *(matrix + rowstride + k)); /* south */

                  diff = *(solution + k) - tmp;
                  err += diff*diff;
                }
            }

          /* advance pointers to data */
          matrix += depth; solution += depth;
        }
    }

  return sqrt (err);
}

/*
 * Solve the laplace equation for matrix and store the result in solution.
 */
void
gimp_heal_laplace_loop (gdouble *matrix,
                        gint     height,
                        gint     depth,
                        gint     width,
                        gdouble *solution)
{
#define EPSILON   0.0001
#define MAX_ITER  500

  gint num_iter = 0;
  gdouble err;

  /* do one iteration and store the amount of error */
  err = gimp_heal_laplace_iteration (matrix, height, depth, width, solution);

  /* copy solution to matrix */
  memcpy (matrix, solution, width * height * depth * sizeof(double));

  /* repeat until convergence or max iterations */
  while (err > EPSILON)
    {
      err = gimp_heal_laplace_iteration (matrix, height, depth, width, solution);
      memcpy (matrix, solution, width * height * depth * sizeof(double));

      num_iter++;

      if (num_iter >= MAX_ITER)
        break;
    }
}

/*
 * Algorithm Design:
 *
 * T. Georgiev, "Image Reconstruction Invariant to Relighting", EUROGRAPHICS
 * 2005, http://www.tgeorgiev.net/
 */
PixelRegion *
gimp_heal_region (PixelRegion *tempPR,
                  PixelRegion *srcPR)
{
  gdouble *i_1 = g_malloc (tempPR->h * tempPR->bytes * tempPR->w * sizeof (gdouble));
  gdouble *i_2 = g_malloc (tempPR->h * tempPR->bytes * tempPR->w * sizeof (gdouble));

  /* substitute 0's for 1's for the division and multiplication operations that
   * come later */
  gimp_heal_substitute_0_for_1 (srcPR);

  /* divide tempPR by srcPR and store the result as a double in i_1 */
  gimp_heal_divide (tempPR, srcPR, i_1);

  /* FIXME: is a faster implementation needed? */
  gimp_heal_laplace_loop (i_1, tempPR->h, tempPR->bytes, tempPR->w, i_2);

  /* multiply a double by srcPR and store in tempPR */
  gimp_heal_multiply (i_2, srcPR, tempPR);

  g_free (i_1);
  g_free (i_2);

  return tempPR;
}

static void
gimp_heal_motion (GimpSourceCore   *source_core,
                  GimpDrawable     *drawable,
                  GimpPaintOptions *paint_options)
{
  GimpPaintCore       *paint_core       = GIMP_PAINT_CORE (source_core);
  GimpSourceOptions   *options          = GIMP_SOURCE_OPTIONS (paint_options);
  GimpContext         *context          = GIMP_CONTEXT (paint_options);
  GimpPressureOptions *pressure_options = paint_options->pressure_options;
  GimpImage           *image;
  GimpImage           *src_image        = NULL;
  GimpPickable        *src_pickable     = NULL;
  TileManager         *src_tiles;
  TempBuf             *area;
  TempBuf             *temp;
  gdouble              opacity;
  PixelRegion          tempPR;
  PixelRegion          srcPR;
  PixelRegion          destPR;
  gint                 offset_x;
  gint                 offset_y;

  /* get the image */
  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /* display a warning about indexed images and return */
  if (GIMP_IMAGE_TYPE_IS_INDEXED (drawable->type))
    {
      g_message (_("Indexed images are not currently supported."));
      return;
    }

  opacity = gimp_paint_options_get_fade (paint_options, image,
                                         paint_core->pixel_dist);

  if (opacity == 0.0)
    return;

  if (! source_core->src_drawable)
    return;

  /* prepare the regions to get data from */
  src_pickable = GIMP_PICKABLE (source_core->src_drawable);
  src_image    = gimp_pickable_get_image (src_pickable);

  /* make local copies */
  offset_x = source_core->offset_x;
  offset_y = source_core->offset_y;

  /* adjust offsets and pickable if we are merging layers */
  if (options->sample_merged)
    {
      gint off_x, off_y;

      src_pickable = GIMP_PICKABLE (src_image->projection);

      gimp_item_offsets (GIMP_ITEM (source_core->src_drawable), &off_x, &off_y);

      offset_x += off_x;
      offset_y += off_y;
    }

  /* get the canvas area */
  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (!area)
    return;

  /* clear the area where we want to paint */
  temp_buf_data_clear (area);

  /* get the source tiles */
  src_tiles = gimp_pickable_get_tiles (src_pickable);

  /* FIXME: the area under the cursor and the source area should be x% larger
   * than the brush size.  Otherwise the brush must be a lot bigger than the
   * area to heal to get good results.  Having the user pick such a large brush
   * is perhaps counter-intutitive? */

  /* Get the area underneath the cursor */
  {
    TempBuf  *orig;
    gint      x1, x2, y1, y2;

    x1 = CLAMP (area->x,
                0, tile_manager_width  (src_tiles));
    y1 = CLAMP (area->y,
                0, tile_manager_height (src_tiles));
    x2 = CLAMP (area->x + area->width,
                0, tile_manager_width  (src_tiles));
    y2 = CLAMP (area->y + area->height,
                0, tile_manager_height (src_tiles));

    if (! (x2 - x1) || (! (y2 - y1)))
      return;

    /*  get the original image data at the cursor location */
    if (options->sample_merged)
      orig = gimp_paint_core_get_orig_proj (paint_core,
                                            src_pickable,
                                            x1, y1, x2, y2);
    else
      orig = gimp_paint_core_get_orig_image (paint_core,
                                             GIMP_DRAWABLE (src_pickable),
                                             x1, y1, x2, y2);

    pixel_region_init_temp_buf (&srcPR, orig, 0, 0, x2 - x1, y2 - y1);
  }

  temp = temp_buf_new (srcPR.w, srcPR.h, srcPR.bytes, 0, 0, NULL);

  pixel_region_init_temp_buf (&tempPR, temp, 0, 0, srcPR.w, srcPR.h);

  copy_region (&srcPR, &tempPR);

  /* now tempPR holds the data under the cursor */

  /* get a copy of the location we will sample from */
  {
    TempBuf  *orig;
    gint      x1, x2, y1, y2;

    x1 = CLAMP (area->x + offset_x,
                0, tile_manager_width  (src_tiles));
    y1 = CLAMP (area->y + offset_y,
                0, tile_manager_height (src_tiles));
    x2 = CLAMP (area->x + offset_x + area->width,
                0, tile_manager_width  (src_tiles));
    y2 = CLAMP (area->y + offset_y + area->height,
                0, tile_manager_height (src_tiles));

    if (! (x2 - x1) || (! (y2 - y1)))
      return;

    /* get the original image data at the sample location */
    if (options->sample_merged)
      orig = gimp_paint_core_get_orig_proj (paint_core,
                                            src_pickable,
                                            x1, y1, x2, y2);
    else
      orig = gimp_paint_core_get_orig_image (paint_core,
                                             GIMP_DRAWABLE (src_pickable),
                                             x1, y1, x2, y2);

    pixel_region_init_temp_buf (&srcPR, orig, 0, 0, x2 - x1, y2 - y1);

    /* set the proper offset */
    offset_x = x1 - (area->x + offset_x);
    offset_y = y1 - (area->y + offset_y);
  }

  /* now srcPR holds the source area to sample from */

  /* get the destination to paint to */
  pixel_region_init_temp_buf (&destPR, area, offset_x, offset_y, srcPR.w, srcPR.h);

  /* FIXME: Can we ensure that this is true in the code above?
   * Is it already guaranteed to be true before we get here? */
  /* check that srcPR, tempPR, and destPR are the same size */
  if ((srcPR.w     != tempPR.w    ) || (srcPR.w     != destPR.w    ) ||
      (srcPR.h     != tempPR.h    ) || (srcPR.h     != destPR.h    ))
    {
      g_message (_("Source and destination regions are not the same size."));
      return;
    }

  /* heal tempPR using srcPR */
  gimp_heal_region (&tempPR, &srcPR);

  /* re-initialize tempPR so that it can be used within copy_region */
  pixel_region_init_data (&tempPR, tempPR.data, tempPR.bytes, tempPR.rowstride,
                          0, 0, tempPR.w, tempPR.h);

  /* add an alpha region to the area if necessary.
   * sample_merged doesn't need an alpha because its always 4 bpp */
  if ((! gimp_drawable_has_alpha (drawable)) && (! options->sample_merged))
    add_alpha_region (&tempPR, &destPR);
  else
    copy_region (&tempPR, &destPR);

  /* check the brush pressure */
  if (pressure_options->opacity)
    opacity *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  /* replace the canvas with our healed data */
  gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core),
                                  drawable,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  GIMP_PAINT_CONSTANT);
}
