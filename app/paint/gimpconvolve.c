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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "paint-types.h"

#include "gegl/gimp-gegl-loops.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimptempbuf.h"

#include "gimpconvolve.h"
#include "gimpconvolveoptions.h"

#include "gimp-intl.h"


#define FIELD_COLS    4
#define MIN_BLUR      64         /*  (8/9 original pixel)   */
#define MAX_BLUR      0.25       /*  (1/33 original pixel)  */
#define MIN_SHARPEN   -512
#define MAX_SHARPEN   -64


static void    gimp_convolve_paint            (GimpPaintCore    *paint_core,
                                               GimpDrawable     *drawable,
                                               GimpPaintOptions *paint_options,
                                               const GimpCoords *coords,
                                               GimpPaintState    paint_state,
                                               guint32           time);
static void    gimp_convolve_motion           (GimpPaintCore    *paint_core,
                                               GimpDrawable     *drawable,
                                               GimpPaintOptions *paint_options,
                                               const GimpCoords *coords);

static void    gimp_convolve_calculate_matrix (GimpConvolve     *convolve,
                                               GimpConvolveType  type,
                                               gint              radius_x,
                                               gint              radius_y,
                                               gdouble           rate);
static gdouble gimp_convolve_sum_matrix       (const gfloat     *matrix);


G_DEFINE_TYPE (GimpConvolve, gimp_convolve, GIMP_TYPE_BRUSH_CORE)


void
gimp_convolve_register (Gimp                      *gimp,
                        GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_CONVOLVE,
                GIMP_TYPE_CONVOLVE_OPTIONS,
                "gimp-convolve",
                _("Convolve"),
                "gimp-tool-blur");
}

static void
gimp_convolve_class_init (GimpConvolveClass *klass)
{
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  paint_core_class->paint = gimp_convolve_paint;
}

static void
gimp_convolve_init (GimpConvolve *convolve)
{
  gint i;

  for (i = 0; i < 9; i++)
    convolve->matrix[i] = 1.0;

  convolve->matrix_divisor = 9.0;
}

static void
gimp_convolve_paint (GimpPaintCore    *paint_core,
                     GimpDrawable     *drawable,
                     GimpPaintOptions *paint_options,
                     const GimpCoords *coords,
                     GimpPaintState    paint_state,
                     guint32           time)
{
  switch (paint_state)
    {
    case GIMP_PAINT_STATE_MOTION:
      gimp_convolve_motion (paint_core, drawable, paint_options, coords);
      break;

    default:
      break;
    }
}

static void
gimp_convolve_motion (GimpPaintCore    *paint_core,
                      GimpDrawable     *drawable,
                      GimpPaintOptions *paint_options,
                      const GimpCoords *coords)
{
  GimpConvolve        *convolve   = GIMP_CONVOLVE (paint_core);
  GimpBrushCore       *brush_core = GIMP_BRUSH_CORE (paint_core);
  GimpConvolveOptions *options    = GIMP_CONVOLVE_OPTIONS (paint_options);
  GimpContext         *context    = GIMP_CONTEXT (paint_options);
  GimpDynamics        *dynamics   = GIMP_BRUSH_CORE (paint_core)->dynamics;
  GimpImage           *image      = gimp_item_get_image (GIMP_ITEM (drawable));
  GeglBuffer          *paint_buffer;
  gint                 paint_buffer_x;
  gint                 paint_buffer_y;
  GimpTempBuf         *temp_buf;
  GeglBuffer          *convolve_buffer;
  gdouble              fade_point;
  gdouble              opacity;
  gdouble              rate;

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  opacity = gimp_dynamics_get_linear_value (dynamics,
                                            GIMP_DYNAMICS_OUTPUT_OPACITY,
                                            coords,
                                            paint_options,
                                            fade_point);
  if (opacity == 0.0)
    return;

  paint_buffer = gimp_paint_core_get_paint_buffer (paint_core, drawable,
                                                   paint_options, coords,
                                                   &paint_buffer_x,
                                                   &paint_buffer_y);
  if (! paint_buffer)
    return;

  rate = (options->rate *
          gimp_dynamics_get_linear_value (dynamics,
                                          GIMP_DYNAMICS_OUTPUT_RATE,
                                          coords,
                                          paint_options,
                                          fade_point));

  gimp_convolve_calculate_matrix (convolve, options->type,
                                  gimp_temp_buf_get_width  (brush_core->brush->mask) / 2,
                                  gimp_temp_buf_get_height (brush_core->brush->mask) / 2,
                                  rate);

  /*  need a linear buffer for gimp_gegl_convolve()  */
  temp_buf = gimp_temp_buf_new (gegl_buffer_get_width  (paint_buffer),
                                gegl_buffer_get_height (paint_buffer),
                                gegl_buffer_get_format (paint_buffer));
  convolve_buffer = gimp_temp_buf_create_buffer (temp_buf);
  gimp_temp_buf_unref (temp_buf);

  gegl_buffer_copy (gimp_drawable_get_buffer (drawable),
                    GEGL_RECTANGLE (paint_buffer_x,
                                    paint_buffer_y,
                                    gegl_buffer_get_width  (paint_buffer),
                                    gegl_buffer_get_height (paint_buffer)),
                    convolve_buffer,
                    GEGL_RECTANGLE (0, 0, 0, 0));

  gimp_gegl_convolve (convolve_buffer,
                      GEGL_RECTANGLE (0, 0,
                                      gegl_buffer_get_width  (convolve_buffer),
                                      gegl_buffer_get_height (convolve_buffer)),
                      paint_buffer,
                      GEGL_RECTANGLE (0, 0,
                                      gegl_buffer_get_width  (paint_buffer),
                                      gegl_buffer_get_height (paint_buffer)),
                      convolve->matrix, 3, convolve->matrix_divisor,
                      GIMP_NORMAL_CONVOL, TRUE);

  g_object_unref (convolve_buffer);

  gimp_brush_core_replace_canvas (brush_core, drawable,
                                  coords,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  1.0,
                                  GIMP_PAINT_INCREMENTAL);
}

static void
gimp_convolve_calculate_matrix (GimpConvolve    *convolve,
                                GimpConvolveType type,
                                gint             radius_x,
                                gint             radius_y,
                                gdouble          rate)
{
  /*  find percent of tool pressure  */
  const gdouble percent = MIN (rate / 100.0, 1.0);

  convolve->matrix[0] = (radius_x && radius_y) ? 1.0 : 0.0;
  convolve->matrix[1] = (radius_y)             ? 1.0 : 0.0;
  convolve->matrix[2] = (radius_x && radius_y) ? 1.0 : 0.0;
  convolve->matrix[3] = (radius_x)             ? 1.0 : 0.0;

  /*  get the appropriate convolution matrix and size and divisor  */
  switch (type)
    {
    case GIMP_CONVOLVE_BLUR:
      convolve->matrix[4] = MIN_BLUR + percent * (MAX_BLUR - MIN_BLUR);
      break;

    case GIMP_CONVOLVE_SHARPEN:
      convolve->matrix[4] = MIN_SHARPEN + percent * (MAX_SHARPEN - MIN_SHARPEN);
      break;
    }

  convolve->matrix[5] = (radius_x)             ? 1.0 : 0.0;
  convolve->matrix[6] = (radius_x && radius_y) ? 1.0 : 0.0;
  convolve->matrix[7] = (radius_y)             ? 1.0 : 0.0;
  convolve->matrix[8] = (radius_x && radius_y) ? 1.0 : 0.0;

  convolve->matrix_divisor = gimp_convolve_sum_matrix (convolve->matrix);
}

static gdouble
gimp_convolve_sum_matrix (const gfloat *matrix)
{
  gdouble sum = 0.0;
  gint    i;

  for (i = 0; i < 9; i++)
    sum += matrix[i];

  return sum;
}
