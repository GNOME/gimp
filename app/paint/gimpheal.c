/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpheal.c
 * Copyright (C) Jean-Yves Couleaud <cjyves@free.fr>
 * Copyright (C) 2013 Loren Merritt
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

#include <stdint.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-loops.h"

#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimptempbuf.h"

#include "gimpheal.h"
#include "gimpsourceoptions.h"

#include "gimp-intl.h"



/* NOTES
 *
 * The method used here is similar to the lighting invariant correction
 * method but slightly different: we do not divide the RGB components,
 * but subtract them I2 = I0 - I1, where I0 is the sample image to be
 * corrected, I1 is the reference pattern. Then we solve DeltaI=0
 * (Laplace) with I2 Dirichlet conditions at the borders of the
 * mask. The solver is a red/black checker Gauss-Seidel with over-relaxation.
 * It could benefit from a multi-grid evaluation of an initial solution
 * before the main iteration loop.
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
static GeglBuffer * gimp_heal_get_paint_buffer   (GimpPaintCore    *core,
                                                  GimpDrawable     *drawable,
                                                  GimpPaintOptions *paint_options,
                                                  GimpLayerMode     paint_mode,
                                                  const GimpCoords *coords,
                                                  gint             *paint_buffer_x,
                                                  gint             *paint_buffer_y,
                                                  gint             *paint_width,
                                                  gint             *paint_height);

static void         gimp_heal_motion             (GimpSourceCore   *source_core,
                                                  GimpDrawable     *drawable,
                                                  GimpPaintOptions *paint_options,
                                                  const GimpCoords *coords,
                                                  GeglNode         *op,
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

  paint_core_class->start            = gimp_heal_start;
  paint_core_class->get_paint_buffer = gimp_heal_get_paint_buffer;

  source_core_class->motion          = gimp_heal_motion;
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

static GeglBuffer *
gimp_heal_get_paint_buffer (GimpPaintCore    *core,
                            GimpDrawable     *drawable,
                            GimpPaintOptions *paint_options,
                            GimpLayerMode     paint_mode,
                            const GimpCoords *coords,
                            gint             *paint_buffer_x,
                            gint             *paint_buffer_y,
                            gint             *paint_width,
                            gint             *paint_height)
{
  return GIMP_PAINT_CORE_CLASS (parent_class)->get_paint_buffer (core,
                                                                 drawable,
                                                                 paint_options,
                                                                 GIMP_LAYER_MODE_NORMAL,
                                                                 coords,
                                                                 paint_buffer_x,
                                                                 paint_buffer_y,
                                                                 paint_width,
                                                                 paint_height);
}

/* Subtract bottom from top and store in result as a float
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
  const Babl         *format       = gegl_buffer_get_format (top_buffer);
  gint                n_components = babl_format_get_n_components (format);

  if (n_components == 2)
    format = babl_format ("Y'A float");
  else if (n_components == 4)
    format = babl_format ("R'G'B'A float");
  else
    g_return_if_reached ();

  iter = gegl_buffer_iterator_new (top_buffer, top_rect, 0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 3);

  gegl_buffer_iterator_add (iter, bottom_buffer, bottom_rect, 0, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, result_buffer, result_rect, 0,
                            babl_format_n (babl_type ("float"), n_components),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *t      = iter->items[0].data;
      gfloat *b      = iter->items[1].data;
      gfloat *r      = iter->items[2].data;
      gint    length = iter->length * n_components;

      while (length--)
        *r++ = *t++ - *b++;
    }
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
  const Babl         *format       = gegl_buffer_get_format (result_buffer);
  gint                n_components = babl_format_get_n_components (format);

  if (n_components == 2)
    format = babl_format ("Y'A float");
  else if (n_components == 4)
    format = babl_format ("R'G'B'A float");
  else
    g_return_if_reached ();

  iter = gegl_buffer_iterator_new (first_buffer, first_rect, 0,
                                   babl_format_n (babl_type ("float"),
                                                  n_components),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 3);

  gegl_buffer_iterator_add (iter, second_buffer, second_rect, 0, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, result_buffer, result_rect, 0, format,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *f      = iter->items[0].data;
      gfloat *s      = iter->items[1].data;
      gfloat *r      = iter->items[2].data;
      gint    length = iter->length * n_components;

      while (length--)
        *r++ = *f++ + *s++;
    }
}

#if defined(__SSE__) && defined(__GNUC__) && __GNUC__ >= 4
static float
gimp_heal_laplace_iteration_sse (gfloat *pixels,
                                 gfloat *Adiag,
                                 gint   *Aidx,
                                 gfloat  w,
                                 gint    nmask)
{
  typedef float v4sf __attribute__((vector_size(16)));
  gint i;
  v4sf wv  = { w, w, w, w };
  v4sf err = { 0, 0, 0, 0 };
  union { v4sf v; float f[4]; } erru;

#define Xv(j) (*(v4sf*)&pixels[Aidx[i * 5 + j]])

  for (i = 0; i < nmask; i++)
    {
      v4sf a    = { Adiag[i], Adiag[i], Adiag[i], Adiag[i] };
      v4sf diff = a * Xv(0) - wv * (Xv(1) + Xv(2) + Xv(3) + Xv(4));

      Xv(0) -= diff;
      err += diff * diff;
    }

  erru.v = err;

  return erru.f[0] + erru.f[1] + erru.f[2] + erru.f[3];
}
#endif

/* Perform one iteration of Gauss-Seidel, and return the sum squared residual.
 */
static float
gimp_heal_laplace_iteration (gfloat *pixels,
                             gfloat *Adiag,
                             gint   *Aidx,
                             gfloat  w,
                             gint    nmask,
                             gint    depth)
{
  gint   i, k;
  gfloat err = 0;

#if defined(__SSE__) && defined(__GNUC__) && __GNUC__ >= 4
  if (depth == 4)
    return gimp_heal_laplace_iteration_sse (pixels, Adiag, Aidx, w, nmask);
#endif

  for (i = 0; i < nmask; i++)
    {
      gint   j0 = Aidx[i * 5 + 0];
      gint   j1 = Aidx[i * 5 + 1];
      gint   j2 = Aidx[i * 5 + 2];
      gint   j3 = Aidx[i * 5 + 3];
      gint   j4 = Aidx[i * 5 + 4];
      gfloat a  = Adiag[i];

      for (k = 0; k < depth; k++)
        {
          gfloat diff = (a * pixels[j0 + k] -
                         w * (pixels[j1 + k] +
                              pixels[j2 + k] +
                              pixels[j3 + k] +
                              pixels[j4 + k]));

          pixels[j0 + k] -= diff;
          err += diff * diff;
        }
    }

  return err;
}

/* Solve the laplace equation for pixels and store the result in-place.
 */
static void
gimp_heal_laplace_loop (gfloat *pixels,
                        gint    height,
                        gint    depth,
                        gint    width,
                        guchar *mask)
{
  /* Tolerate a total deviation-from-smoothness of 0.1 LSBs at 8bit depth. */
#define EPSILON  (0.1/255)
#define MAX_ITER 500

  gint    i, j, iter, parity, nmask, zero;
  gfloat *Adiag;
  gint   *Aidx;
  gfloat  w;

  Adiag = g_new (gfloat, width * height);
  Aidx  = g_new (gint, 5 * width * height);

  /* All off-diagonal elements of A are either -1 or 0. We could store it as a
   * general-purpose sparse matrix, but that adds some unnecessary overhead to
   * the inner loop. Instead, assume exactly 4 off-diagonal elements in each
   * row, all of which have value -1. Any row that in fact wants less than 4
   * coefs can put them in a dummy column to be multiplied by an empty pixel.
   */
  zero = depth * width * height;
  memset (pixels + zero, 0, depth * sizeof (gfloat));

  /* Construct the system of equations.
   * Arrange Aidx in checkerboard order, so that a single linear pass over that
   * array results updating all of the red cells and then all of the black cells.
   */
  nmask = 0;
  for (parity = 0; parity < 2; parity++)
    for (i = 0; i < height; i++)
      for (j = (i&1)^parity; j < width; j+=2)
        if (mask[j + i * width])
          {
#define A_NEIGHBOR(o,di,dj) \
            if ((dj<0 && j==0) || (dj>0 && j==width-1) || (di<0 && i==0) || (di>0 && i==height-1)) \
              Aidx[o + nmask * 5] = zero; \
            else                                               \
              Aidx[o + nmask * 5] = ((i + di) * width + (j + dj)) * depth;

            /* Omit Dirichlet conditions for any neighbors off the
             * edge of the canvas.
             */
            Adiag[nmask] = 4 - (i==0) - (j==0) - (i==height-1) - (j==width-1);
            A_NEIGHBOR (0,  0,  0);
            A_NEIGHBOR (1,  0,  1);
            A_NEIGHBOR (2,  1,  0);
            A_NEIGHBOR (3,  0, -1);
            A_NEIGHBOR (4, -1,  0);
            nmask++;
          }

  /* Empirically optimal over-relaxation factor. (Benchmarked on
   * round brushes, at least. I don't know whether aspect ratio
   * affects it.)
   */
  w = 2.0 - 1.0 / (0.1575 * sqrt (nmask) + 0.8);
  w *= 0.25;
  for (i = 0; i < nmask; i++)
    Adiag[i] *= w;

  /* Gauss-Seidel with successive over-relaxation */
  for (iter = 0; iter < MAX_ITER; iter++)
    {
      gfloat err = gimp_heal_laplace_iteration (pixels, Adiag, Aidx,
                                                w, nmask, depth);
      if (err < EPSILON * EPSILON * w * w)
        break;
    }

  g_free (Adiag);
  g_free (Aidx);
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
  gint        src_components;
  gint        dest_components;
  gint        width;
  gint        height;
  gfloat     *diff, *diff_alloc;
  GeglBuffer *diff_buffer;
  guchar     *mask;

  src_format  = gegl_buffer_get_format (src_buffer);
  dest_format = gegl_buffer_get_format (dest_buffer);

  src_components  = babl_format_get_n_components (src_format);
  dest_components = babl_format_get_n_components (dest_format);

  width  = gegl_buffer_get_width  (src_buffer);
  height = gegl_buffer_get_height (src_buffer);

  g_return_if_fail (src_components == dest_components);

  diff_alloc = g_new (gfloat, 4 + (width * height + 1) * src_components);
  diff = (gfloat*)(((uintptr_t)diff_alloc + 15) & ~15);

  diff_buffer =
    gegl_buffer_linear_new_from_data (diff,
                                      babl_format_n (babl_type ("float"),
                                                     src_components),
                                      GEGL_RECTANGLE (0, 0, width, height),
                                      GEGL_AUTO_ROWSTRIDE,
                                      (GDestroyNotify) g_free, diff_alloc);

  /* subtract pattern from image and store the result as a float in diff */
  gimp_heal_sub (dest_buffer, dest_rect,
                 src_buffer, src_rect,
                 diff_buffer, GEGL_RECTANGLE (0, 0, width, height));

  mask = g_new (guchar, mask_rect->width * mask_rect->height);

  gegl_buffer_get (mask_buffer, mask_rect, 1.0, babl_format ("Y u8"),
                   mask, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gimp_heal_laplace_loop (diff, height, src_components, width, mask);

  g_free (mask);

  /* add solution to original image and store in dest */
  gimp_heal_add (diff_buffer, GEGL_RECTANGLE (0, 0, width, height),
                 src_buffer, src_rect,
                 dest_buffer, dest_rect);

  g_object_unref (diff_buffer);
}

static void
gimp_heal_motion (GimpSourceCore   *source_core,
                  GimpDrawable     *drawable,
                  GimpPaintOptions *paint_options,
                  const GimpCoords *coords,
                  GeglNode         *op,
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
  GimpPaintCore     *paint_core  = GIMP_PAINT_CORE (source_core);
  GimpContext       *context     = GIMP_CONTEXT (paint_options);
  GimpSourceOptions *src_options = GIMP_SOURCE_OPTIONS (paint_options);
  GimpDynamics      *dynamics    = GIMP_BRUSH_CORE (paint_core)->dynamics;
  GimpImage         *image       = gimp_item_get_image (GIMP_ITEM (drawable));
  GeglBuffer        *src_copy;
  GeglBuffer        *mask_buffer;
  GimpPickable      *dest_pickable;
  const GimpTempBuf *mask_buf;
  gdouble            fade_point;
  gdouble            force;
  gint               mask_off_x;
  gint               mask_off_y;
  gint               dest_pickable_off_x;
  gint               dest_pickable_off_y;

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  if (gimp_dynamics_is_output_enabled (dynamics, GIMP_DYNAMICS_OUTPUT_FORCE))
    force = gimp_dynamics_get_linear_value (dynamics,
                                            GIMP_DYNAMICS_OUTPUT_FORCE,
                                            coords,
                                            paint_options,
                                            fade_point);
  else
    force = paint_options->brush_force;

  mask_buf = gimp_brush_core_get_brush_mask (GIMP_BRUSH_CORE (source_core),
                                             coords,
                                             GIMP_BRUSH_HARD,
                                             force);

  if (! mask_buf)
    return;

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

  /*  heal should work in perceptual space, use R'G'B' instead of RGB  */
  src_copy = gegl_buffer_new (GEGL_RECTANGLE (paint_area_offset_x,
                                              paint_area_offset_y,
                                              src_rect->width,
                                              src_rect->height),
                              babl_format ("R'G'B'A float"));

  if (! op)
    {
      gimp_gegl_buffer_copy (src_buffer, src_rect, GEGL_ABYSS_NONE,
                             src_copy, gegl_buffer_get_extent (src_copy));
    }
  else
    {
      gimp_gegl_apply_operation (src_buffer, NULL, NULL, op,
                                 src_copy, gegl_buffer_get_extent (src_copy),
                                 FALSE);
    }

  if (src_options->sample_merged)
    {
      dest_pickable = GIMP_PICKABLE (image);

      gimp_item_get_offset (GIMP_ITEM (drawable),
                            &dest_pickable_off_x,
                            &dest_pickable_off_y);
    }
  else
    {
      dest_pickable = GIMP_PICKABLE (drawable);

      dest_pickable_off_x = 0;
      dest_pickable_off_y = 0;
    }

  gimp_gegl_buffer_copy (gimp_pickable_get_buffer (dest_pickable),
                         GEGL_RECTANGLE (paint_buffer_x + dest_pickable_off_x,
                                         paint_buffer_y + dest_pickable_off_y,
                                         gegl_buffer_get_width  (paint_buffer),
                                         gegl_buffer_get_height (paint_buffer)),
                         GEGL_ABYSS_NONE,
                         paint_buffer,
                         GEGL_RECTANGLE (paint_area_offset_x,
                                         paint_area_offset_y,
                                         paint_area_width,
                                         paint_area_height));

  mask_buffer = gimp_temp_buf_create_buffer ((GimpTempBuf *) mask_buf);

  /* find the offset of the brush mask's rect */
  {
    gint x = (gint) floor (coords->x) - (gegl_buffer_get_width  (mask_buffer) >> 1);
    gint y = (gint) floor (coords->y) - (gegl_buffer_get_height (mask_buffer) >> 1);

    mask_off_x = (x < 0) ? -x : 0;
    mask_off_y = (y < 0) ? -y : 0;
  }

  gimp_heal (src_copy, gegl_buffer_get_extent (src_copy),
             paint_buffer,
             GEGL_RECTANGLE (paint_area_offset_x,
                             paint_area_offset_y,
                             paint_area_width,
                             paint_area_height),
             mask_buffer,
             GEGL_RECTANGLE (mask_off_x, mask_off_y,
                             paint_area_width,
                             paint_area_height));

  g_object_unref (src_copy);
  g_object_unref (mask_buffer);

  /* replace the canvas with our healed data */
  gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                  coords,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  force,
                                  GIMP_PAINT_INCREMENTAL);
}
