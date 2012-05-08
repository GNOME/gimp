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

#include <string.h>

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "paint-funcs/paint-funcs.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"

#include "gimpheal.h"
#include "gimpsourceoptions.h"

#include "gimp-intl.h"



/* NOTES
 *
 * The method used here is similar to the lighting invariant correctin
 * method but slightly different: we do not divide the RGB components,
 * but substract them I2 = I0 - I1, where I0 is the sample image to be
 * corrected, I1 is the reference pattern. Then we solve DeltaI=0
 * (Laplace) with I2 Dirichlet conditions at the borders of the
 * mask. The solver is a unoptimized red/black checker Gauss-Siedel
 * with an over-relaxation factor of 1.8. It can benefit from a
 * multi-grid evaluation of an initial solution before the main
 * iteration loop.
 *
 * I reduced the convergence criteria to 0.1% (0.001) as we are
 * dealing here with RGB integer components, more is overkill.
 *
 * Jean-Yves Couleaud cjyves@free.fr
 */

static gboolean     gimp_heal_start              (GimpPaintCore    *paint_core,
                                                  GimpDrawable     *drawable,
                                                  GimpPaintOptions *paint_options,
                                                  const GimpCoords *coords,
                                                  GError          **error);

static void         gimp_heal_sub                (PixelRegion      *topPR,
                                                  PixelRegion      *bottomPR,
                                                  gdouble          *result);

static void         gimp_heal_add                (gdouble          *first,
                                                  PixelRegion      *secondPR,
                                                  PixelRegion      *resultPR);

static gdouble      gimp_heal_laplace_iteration  (gdouble          *matrix,
                                                  gint              height,
                                                  gint              depth,
                                                  gint              width,
                                                  gdouble          *solution,
                                                  guchar           *mask,
                                                  gint              mask_stride,
                                                  gint              mask_offx,
                                                  gint              mask_offy);

static void         gimp_heal_laplace_loop       (gdouble          *matrix,
                                                  gint              height,
                                                  gint              depth,
                                                  gint              width,
                                                  gdouble          *solution,
                                                  PixelRegion      *maskPR);

static PixelRegion *gimp_heal_region             (PixelRegion      *tempPR,
                                                  PixelRegion      *srcPR,
                                                  PixelRegion      *maskPR);

static void         gimp_heal_motion             (GimpSourceCore   *source_core,
                                                  GimpDrawable     *drawable,
                                                  GimpPaintOptions *paint_options,
                                                  const GimpCoords *coords,
                                                  gdouble           opacity,
                                                  GimpPickable     *src_pickable,
                                                  PixelRegion      *srcPR,
                                                  gint              src_offset_x,
                                                  gint              src_offset_y,
                                                  TempBuf          *paint_area,
                                                  gint              paint_area_offset_x,
                                                  gint              paint_area_offset_y,
                                                  gint              paint_area_width,
                                                  gint              paint_area_height);


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
  GimpPaintCoreClass  *paint_core_class  = GIMP_PAINT_CORE_CLASS (klass);
  GimpSourceCoreClass *source_core_class = GIMP_SOURCE_CORE_CLASS (klass);

  paint_core_class->start   = gimp_heal_start;

  source_core_class->motion = gimp_heal_motion;
}

static void
gimp_heal_init (GimpHeal *heal)
{
}

static gboolean
gimp_heal_start (GimpPaintCore     *paint_core,
                 GimpDrawable      *drawable,
                 GimpPaintOptions  *paint_options,
                 const GimpCoords  *coords,
                 GError           **error)
{
  GimpSourceCore *source_core = GIMP_SOURCE_CORE (paint_core);

  if (! GIMP_PAINT_CORE_CLASS (parent_class)->start (paint_core, drawable,
                                                     paint_options, coords,
                                                     error))
    {
      return FALSE;
    }

  if (! source_core->set_source && gimp_drawable_is_indexed (drawable))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Healing does not operate on indexed layers."));
      return FALSE;
    }

  return TRUE;
}

/* Subtract bottomPR from topPR and store the result as a double
 */
static void
gimp_heal_sub (PixelRegion *topPR,
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

  g_assert (topPR->bytes == bottomPR->bytes);

  for (i = 0; i < height; i++)
    {
      t = t_data;
      b = b_data;

      for (j = 0; j < width; j++)
        {
          for (k = 0; k < depth; k++)
            {
              r[k] = (gdouble) (t[k]) - (gdouble) (b[k]);
            }
          t += depth;
          b += depth;
          r += depth;
        }

      t_data += topPR->rowstride;
      b_data += bottomPR->rowstride;
    }
}

/* Add first to secondPR and store the result as a PixelRegion
 */
static void
gimp_heal_add (gdouble     *first,
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

  g_assert (secondPR->bytes == resultPR->bytes);

  for (i = 0; i < height; i++)
    {
      s = s_data;
      r = r_data;

      for (j = 0; j < width; j++)
        {
          for (k = 0; k < depth; k++)
            {
              r[k] = (guchar) CLAMP0255 (ROUND (((gdouble) (f[k])) +
                                                ((gdouble) (s[k]))));
            }

          f += depth;
          s += depth;
          r += depth;
        }

      s_data += secondPR->rowstride;
      r_data += resultPR->rowstride;
    }
}

/* Perform one iteration of the laplace solver for matrix.  Store the
 * result in solution and return the square of the cummulative error
 * of the solution.
 */
static gdouble
gimp_heal_laplace_iteration (gdouble *matrix,
                             gint     height,
                             gint     depth,
                             gint     width,
                             gdouble *solution,
                             guchar  *mask,
                             gint     mask_stride,
                             gint     mask_offx,
                             gint     mask_offy)
{
  const gint    rowstride = width * depth;
  gint          i, j, k, off, offm, offm0, off0;
  gdouble       tmp, diff;
  gdouble       err       = 0.0;
  const gdouble w         = 1.80 * 0.25; /* Over-relaxation = 1.8 */

  /* we use a red/black checker model of the discretization grid */

  /* do reds */
  for (i = 0; i < height; i++)
    {
      off0  = i * rowstride;
      offm0 = (i + mask_offy) * mask_stride;

      for (j = i % 2; j < width; j += 2)
        {
          off  = off0 + j * depth;
          offm = offm0 + j + mask_offx;

          if ((0 == mask[offm]) ||
              (i == 0) || (i == (height - 1)) ||
              (j == 0) || (j == (width - 1)))
            {
              /* do nothing at the boundary or outside mask */
              for (k = 0; k < depth; k++)
                solution[off + k] = matrix[off + k];
            }
          else
            {
              /* Use Gauss Siedel to get the correction factor then
               * over-relax it
               */
              for (k = 0; k < depth; k++)
                {
                  tmp = solution[off + k];
                  solution[off + k] = (matrix[off + k] +
                                       w *
                                       (matrix[off - depth + k] +     /* west */
                                        matrix[off + depth + k] +     /* east */
                                        matrix[off - rowstride + k] + /* north */
                                        matrix[off + rowstride + k] - 4.0 *
                                        matrix[off+k]));              /* south */

                  diff = solution[off + k] - tmp;
                  err += diff * diff;
                }
            }
        }
    }


  /* Do blacks
   *
   * As we've done the reds earlier, we can use them right now to
   * accelerate the convergence. So we have "solution" in the solver
   * instead of "matrix" above
   */
  for (i = 0; i < height; i++)
    {
      off0  = i * rowstride;
      offm0 = (i + mask_offy) * mask_stride;

      for (j = (i % 2) ? 0 : 1; j < width; j += 2)
        {
          off = off0 + j * depth;
          offm = offm0 + j + mask_offx;

          if ((0 == mask[offm]) ||
              (i == 0) || (i == (height - 1)) ||
              (j == 0) || (j == (width - 1)))
            {
              /* do nothing at the boundary or outside mask */
              for (k = 0; k < depth; k++)
                solution[off + k] = matrix[off + k];
            }
          else
            {
              /* Use Gauss Siedel to get the correction factor then
               * over-relax it
               */
              for (k = 0; k < depth; k++)
                {
                  tmp = solution[off + k];
                  solution[off + k] = (matrix[off + k] +
                                       w *
                                       (solution[off - depth + k] +     /* west */
                                        solution[off + depth + k] +     /* east */
                                        solution[off - rowstride + k] + /* north */
                                        solution[off + rowstride + k] - 4.0 *
                                        matrix[off+k]));                /* south */

                  diff = solution[off + k] - tmp;
                  err += diff*diff;
                }
            }
        }
    }

  return err;
}

/* Solve the laplace equation for matrix and store the result in solution.
 */
static void
gimp_heal_laplace_loop (gdouble     *matrix,
                        gint         height,
                        gint         depth,
                        gint         width,
                        gdouble     *solution,
                        PixelRegion *maskPR)
{
#define EPSILON   0.001
#define MAX_ITER  500
  gint i;

  /* repeat until convergence or max iterations */
  for (i = 0; i < MAX_ITER; i++)
    {
      gdouble sqr_err;

      /* do one iteration and store the amount of error */
      sqr_err = gimp_heal_laplace_iteration (matrix, height, depth, width,
                                             solution, maskPR->data, maskPR->rowstride,
                                             maskPR->x, maskPR->y);

      /* copy solution to matrix */
      memcpy (matrix, solution, width * height * depth * sizeof (double));

      if (sqr_err < EPSILON)
        break;
    }
}

/* Original Algorithm Design:
 *
 * T. Georgiev, "Photoshop Healing Brush: a Tool for Seamless Cloning
 * http://www.tgeorgiev.net/Photoshop_Healing.pdf
 */
static PixelRegion *
gimp_heal_region (PixelRegion   *tempPR,
                  PixelRegion   *srcPR,
                  PixelRegion   *maskPR)
{
  gdouble *i_1  = g_new (gdouble, tempPR->h * tempPR->bytes * tempPR->w);
  gdouble *i_2  = g_new (gdouble, tempPR->h * tempPR->bytes * tempPR->w);

  /* substract pattern to image and store the result as a double in i_1 */
  gimp_heal_sub (tempPR, srcPR, i_1);

  /* FIXME: is a faster implementation needed? */
  gimp_heal_laplace_loop (i_1, tempPR->h, tempPR->bytes, tempPR->w, i_2, maskPR);

  /* add solution to original image and store in tempPR */
  gimp_heal_add (i_2, srcPR, tempPR);

  /* clean up */
  g_free (i_1);
  g_free (i_2);

  return tempPR;
}

static void
gimp_heal_motion (GimpSourceCore   *source_core,
                  GimpDrawable     *drawable,
                  GimpPaintOptions *paint_options,
                  const GimpCoords *coords,
                  gdouble           opacity,
                  GimpPickable     *src_pickable,
                  PixelRegion      *srcPR,
                  gint              src_offset_x,
                  gint              src_offset_y,
                  TempBuf          *paint_area,
                  gint              paint_area_offset_x,
                  gint              paint_area_offset_y,
                  gint              paint_area_width,
                  gint              paint_area_height)
{
  GimpPaintCore      *paint_core = GIMP_PAINT_CORE (source_core);
  GimpContext        *context    = GIMP_CONTEXT (paint_options);
  GimpDynamics       *dynamics   = GIMP_BRUSH_CORE (paint_core)->dynamics;
  GimpDynamicsOutput *hardness_output;
  GimpImage          *image      = gimp_item_get_image (GIMP_ITEM (drawable));
  TempBuf            *src;
  TempBuf            *temp;
  PixelRegion         origPR;
  PixelRegion         tempPR;
  PixelRegion         destPR;
  PixelRegion         maskPR;
  GimpImageType       src_type;
  const TempBuf      *mask_buf;
  gdouble             fade_point;
  gdouble             hardness;

  hardness_output = gimp_dynamics_get_output (dynamics,
                                              GIMP_DYNAMICS_OUTPUT_HARDNESS);

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  hardness = gimp_dynamics_output_get_linear_value (hardness_output,
                                                    coords,
                                                    paint_options,
                                                    fade_point);

  mask_buf = gimp_brush_core_get_brush_mask (GIMP_BRUSH_CORE (source_core),
                                             coords,
                                             GIMP_BRUSH_HARD,
                                             hardness);

  src_type = gimp_pickable_get_image_type (src_pickable);

  /* we need the source area with alpha and we modify it, so make a copy */
  src = temp_buf_new (srcPR->w, srcPR->h,
                      GIMP_IMAGE_TYPE_BYTES (GIMP_IMAGE_TYPE_WITH_ALPHA (src_type)),
                      0, 0, NULL);

  pixel_region_init_temp_buf (&tempPR, src, 0, 0, src->width, src->height);

  /*
   * the effect of the following is to copy the contents of the source
   * region to the "src" temp-buf, adding an alpha channel if necessary
   */
  if (GIMP_IMAGE_TYPE_HAS_ALPHA (src_type))
    copy_region (srcPR, &tempPR);
  else
    add_alpha_region (srcPR, &tempPR);

  /* reinitialize srcPR */
  pixel_region_init_temp_buf (srcPR, src, 0, 0, src->width, src->height);

  if (GIMP_IMAGE_TYPE_WITH_ALPHA (src_type) !=
      gimp_drawable_type_with_alpha (drawable))
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (drawable));
      TempBuf   *temp2;
      gboolean   new_buf;

      temp2 = gimp_image_transform_temp_buf (image,
                                             gimp_drawable_type_with_alpha (drawable),
                                             src, &new_buf);

      if (new_buf)
        temp_buf_free (src);

      src = temp2;
    }

  /* reinitialize srcPR */
  pixel_region_init_temp_buf (srcPR, src, 0, 0, src->width, src->height);

  /* FIXME: the area under the cursor and the source area should be x% larger
   * than the brush size.  Otherwise the brush must be a lot bigger than the
   * area to heal to get good results.  Having the user pick such a large brush
   * is perhaps counter-intutitive?
   */

  pixel_region_init (&origPR, gimp_drawable_get_tiles (drawable),
                     paint_area->x, paint_area->y,
                     paint_area->width, paint_area->height, FALSE);

  temp = temp_buf_new (origPR.w, origPR.h,
                       gimp_drawable_bytes_with_alpha (drawable),
                       0, 0, NULL);
  pixel_region_init_temp_buf (&tempPR, temp, 0, 0, temp->width, temp->height);

  if (gimp_drawable_has_alpha (drawable))
    copy_region (&origPR, &tempPR);
  else
    add_alpha_region (&origPR, &tempPR);

  /* reinitialize tempPR */
  pixel_region_init_temp_buf (&tempPR, temp, 0, 0, temp->width, temp->height);

  /* now tempPR holds the data under the cursor and
   * srcPR holds the area to sample from
   */

  /* get the destination to paint to */
  pixel_region_init_temp_buf (&destPR, paint_area,
                              paint_area_offset_x, paint_area_offset_y,
                              paint_area_width, paint_area_height);

  /* check that srcPR, tempPR and destPR are the same size and tempPR is inside of layer */
  if ((srcPR->w != tempPR.w) || (srcPR->w != destPR.w) ||
      (srcPR->h != tempPR.h) || (srcPR->h != destPR.h) ||
      (tempPR.w <= 0) ||
      (tempPR.h <= 0))
    {
      /* this generally means that the source point has hit the edge of the
         layer, so it is not an error and we should not complain, just
         don't do anything */

      temp_buf_free (src);
      temp_buf_free (temp);

      return;
    }

  /* find the offset of the brush mask's rect */
  {
    gint x = (gint) floor (coords->x) - (mask_buf->width  >> 1);
    gint y = (gint) floor (coords->y) - (mask_buf->height >> 1);

    gint off_x = (x < 0) ? -x : 0;
    gint off_y = (y < 0) ? -y : 0;

    pixel_region_init_temp_buf (&maskPR, mask_buf, off_x, off_y,
                                paint_area_width, paint_area_height);
  }

  /* heal tempPR using srcPR */
  gimp_heal_region (&tempPR, srcPR, &maskPR);

  temp_buf_free (src);

  /* reinitialize tempPR */
  pixel_region_init_temp_buf (&tempPR, temp, 0, 0, temp->width, temp->height);

  copy_region (&tempPR, &destPR);

  temp_buf_free (temp);

  /* replace the canvas with our healed data */
  gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                  coords,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  hardness,
                                  GIMP_PAINT_INCREMENTAL);
}
