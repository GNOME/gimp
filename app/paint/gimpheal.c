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

#include "gegl/gimp-gegl-utils.h"

#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimptempbuf.h"

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

static void         gimp_heal_motion             (GimpSourceCore   *source_core,
                                                  GimpDrawable     *drawable,
                                                  GimpPaintOptions *paint_options,
                                                  const GimpCoords *coords,
                                                  gdouble           opacity,
                                                  GimpPickable     *src_pickable,
                                                  GeglBuffer       *src_buffer,
                                                  GeglRectangle    *src_rect,
                                                  gint              src_offset_x,
                                                  gint              src_offset_y,
                                                  GeglBuffer       *paint_buffer,
                                                  gint              paint_buffer_x,
                                                  gint              paint_buffer_y,
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

/* Subtract bottom from top and store in result as a double
 */
static void
gimp_heal_sub (GeglBuffer          *top_buffer,
               const GeglRectangle *top_rect,
               GeglBuffer          *bottom_buffer,
               const GeglRectangle *bottom_rect,
               GeglBuffer          *result_buffer,
               const GeglRectangle *result_rect)
{
  GeglBufferIterator *iter;
  const Babl         *format = gegl_buffer_get_format (top_buffer);
  gint                bpp    = babl_format_get_bytes_per_pixel (format);

  gegl_buffer_set_format (top_buffer, babl_format_n (babl_type ("u8"), bpp));
  gegl_buffer_set_format (bottom_buffer, babl_format_n (babl_type ("u8"), bpp));

  iter = gegl_buffer_iterator_new (top_buffer, top_rect, 0, NULL,
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, bottom_buffer, bottom_rect, 0, NULL,
                            GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, result_buffer, result_rect, 0,
                            babl_format_n (babl_type ("double"), bpp),
                            GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      guchar  *t      = iter->data[0];
      guchar  *b      = iter->data[1];
      gdouble *r      = iter->data[2];
      gint     length = iter->length * bpp;

      while (length--)
        *r++ = (gdouble) *t++ - (gdouble) *b++;
    }

  gegl_buffer_set_format (top_buffer, NULL);
  gegl_buffer_set_format (bottom_buffer, NULL);
}

/* Add first to second and store in result
 */
static void
gimp_heal_add (GeglBuffer          *first_buffer,
               const GeglRectangle *first_rect,
               GeglBuffer          *second_buffer,
               const GeglRectangle *second_rect,
               GeglBuffer          *result_buffer,
               const GeglRectangle *result_rect)
{
  GeglBufferIterator *iter;
  const Babl         *format = gegl_buffer_get_format (result_buffer);
  gint                bpp    = babl_format_get_bytes_per_pixel (format);

  gegl_buffer_set_format (second_buffer, babl_format_n (babl_type ("u8"), bpp));
  gegl_buffer_set_format (result_buffer, babl_format_n (babl_type ("u8"), bpp));

  iter = gegl_buffer_iterator_new (first_buffer, first_rect, 0,
                                   babl_format_n (babl_type ("double"), bpp),
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, second_buffer, second_rect, 0, NULL,
                            GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, result_buffer, result_rect, 0, NULL,
                            GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gdouble *f      = iter->data[0];
      guchar  *s      = iter->data[1];
      guchar  *r      = iter->data[2];
      gint     length = iter->length * bpp;

      while (length--)
        {
          gdouble tmp = ROUND (*f++ + (gdouble) *s++);

          *r++ = (guchar) CLAMP0255 (tmp);
        }
    }

  gegl_buffer_set_format (second_buffer, NULL);
  gegl_buffer_set_format (result_buffer, NULL);
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
                             guchar  *mask)
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
      offm0 = i * width;

      for (j = i % 2; j < width; j += 2)
        {
          off  = off0 + j * depth;
          offm = offm0 + j;

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
      off0 =  i * rowstride;
      offm0 = i * width;

      for (j = (i % 2) ? 0 : 1; j < width; j += 2)
        {
          off = off0 + j * depth;
          offm = offm0 + j;

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
gimp_heal_laplace_loop (gdouble *matrix,
                        gint     height,
                        gint     depth,
                        gint     width,
                        gdouble *solution,
                        guchar  *mask)
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
                                             solution, mask);

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
static void
gimp_heal (GeglBuffer          *src_buffer,
           const GeglRectangle *src_rect,
           GeglBuffer          *dest_buffer,
           const GeglRectangle *dest_rect,
           GeglBuffer          *mask_buffer,
           const GeglRectangle *mask_rect)
{
  const Babl *src_format;
  const Babl *dest_format;
  gint        src_bpp;
  gint        dest_bpp;
  gint        width;
  gint        height;
  gdouble    *i_1;
  gdouble    *i_2;
  GeglBuffer *i_1_buffer;
  GeglBuffer *i_2_buffer;
  guchar     *mask;

  src_format  = gegl_buffer_get_format (src_buffer);
  dest_format = gegl_buffer_get_format (dest_buffer);

  src_bpp  = babl_format_get_bytes_per_pixel (src_format);
  dest_bpp = babl_format_get_bytes_per_pixel (dest_format);

  width  = gegl_buffer_get_width  (src_buffer);
  height = gegl_buffer_get_height (src_buffer);

  g_return_if_fail (src_bpp == dest_bpp);

  i_1  = g_new (gdouble, width * height * src_bpp);
  i_2  = g_new (gdouble, width * height * src_bpp);

  i_1_buffer =
    gegl_buffer_linear_new_from_data (i_1,
                                      babl_format_n (babl_type ("double"),
                                                     src_bpp),
                                      GEGL_RECTANGLE (0, 0, width, height),
                                      GEGL_AUTO_ROWSTRIDE,
                                      (GDestroyNotify) g_free, i_1);
  i_2_buffer =
    gegl_buffer_linear_new_from_data (i_2,
                                      babl_format_n (babl_type ("double"),
                                                     src_bpp),
                                      GEGL_RECTANGLE (0, 0, width, height),
                                      GEGL_AUTO_ROWSTRIDE,
                                      (GDestroyNotify) g_free, i_2);

  /* substract pattern from image and store the result as a double in i_1 */
  gimp_heal_sub (dest_buffer, dest_rect,
                 src_buffer, src_rect,
                 i_1_buffer, GEGL_RECTANGLE (0, 0, width, height));

  mask = g_new (guchar, mask_rect->width * mask_rect->height);

  gegl_buffer_get (mask_buffer, mask_rect, 1.0, babl_format ("Y u8"),
                   mask, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /* FIXME: is a faster implementation needed? */
  gimp_heal_laplace_loop (i_1, height, src_bpp, width, i_2, mask);

  g_free (mask);

  /* add solution to original image and store in dest */
  gimp_heal_add (i_2_buffer, GEGL_RECTANGLE (0, 0, width, height),
                 src_buffer, src_rect,
                 dest_buffer, dest_rect);

  g_object_unref (i_1_buffer);
  g_object_unref (i_2_buffer);
}

static void
gimp_heal_motion (GimpSourceCore   *source_core,
                  GimpDrawable     *drawable,
                  GimpPaintOptions *paint_options,
                  const GimpCoords *coords,
                  gdouble           opacity,
                  GimpPickable     *src_pickable,
                  GeglBuffer       *src_buffer,
                  GeglRectangle    *src_rect,
                  gint              src_offset_x,
                  gint              src_offset_y,
                  GeglBuffer       *paint_buffer,
                  gint              paint_buffer_x,
                  gint              paint_buffer_y,
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
  GeglBuffer         *src_copy;
  GeglBuffer         *mask_buffer;
  const GimpTempBuf  *mask_buf;
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

  /* check that all buffers are of the same size */
  if (src_rect->width  != gegl_buffer_get_width  (paint_buffer) ||
      src_rect->height != gegl_buffer_get_height (paint_buffer))
    {
      /* this generally means that the source point has hit the edge
       * of the layer, so it is not an error and we should not
       * complain, just don't do anything
       */
      return;
    }

  src_copy =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                     src_rect->width,
                                     src_rect->height),
                     gimp_drawable_get_format_with_alpha (drawable));

  gegl_buffer_copy (src_buffer,
                    src_rect,
                    src_copy,
                    GEGL_RECTANGLE (0, 0,
                                    src_rect->width,
                                    src_rect->height));

  gegl_buffer_copy (gimp_drawable_get_buffer (drawable),
                    GEGL_RECTANGLE (paint_buffer_x, paint_buffer_y,
                                    gegl_buffer_get_width  (paint_buffer),
                                    gegl_buffer_get_height (paint_buffer)),
                    paint_buffer,
                    GEGL_RECTANGLE (paint_area_offset_x,
                                    paint_area_offset_y,
                                    paint_area_width,
                                    paint_area_height));

  mask_buffer = gimp_temp_buf_create_buffer ((GimpTempBuf *) mask_buf);

  gimp_heal (src_copy,
             GEGL_RECTANGLE (0, 0,
                             gegl_buffer_get_width  (src_copy),
                             gegl_buffer_get_height (src_copy)),
             paint_buffer,
             GEGL_RECTANGLE (paint_area_offset_x,
                             paint_area_offset_y,
                             paint_area_width,
                             paint_area_height),
             mask_buffer,
             GEGL_RECTANGLE (0, 0,
                             gegl_buffer_get_width  (mask_buffer),
                             gegl_buffer_get_height (mask_buffer)));

  g_object_unref (src_copy);
  g_object_unref (mask_buffer);

  /* replace the canvas with our healed data */
  gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                  coords,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  hardness,
                                  GIMP_PAINT_INCREMENTAL);
}
