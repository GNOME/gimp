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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "paint-types.h"

#include "gegl/ligma-gegl-loops.h"

#include "core/ligma.h"
#include "core/ligmabrush.h"
#include "core/ligmadrawable.h"
#include "core/ligmadynamics.h"
#include "core/ligmaimage.h"
#include "core/ligmapickable.h"
#include "core/ligmasymmetry.h"
#include "core/ligmatempbuf.h"

#include "ligmaconvolve.h"
#include "ligmaconvolveoptions.h"

#include "ligma-intl.h"


#define FIELD_COLS    4
#define MIN_BLUR      64         /*  (8/9 original pixel)   */
#define MAX_BLUR      0.25       /*  (1/33 original pixel)  */
#define MIN_SHARPEN   -512
#define MAX_SHARPEN   -64


static void    ligma_convolve_paint            (LigmaPaintCore    *paint_core,
                                               GList            *drawables,
                                               LigmaPaintOptions *paint_options,
                                               LigmaSymmetry     *sym,
                                               LigmaPaintState    paint_state,
                                               guint32           time);
static void    ligma_convolve_motion           (LigmaPaintCore    *paint_core,
                                               LigmaDrawable     *drawable,
                                               LigmaPaintOptions *paint_options,
                                               LigmaSymmetry     *sym);

static void    ligma_convolve_calculate_matrix (LigmaConvolve     *convolve,
                                               LigmaConvolveType  type,
                                               gint              radius_x,
                                               gint              radius_y,
                                               gdouble           rate);
static gdouble ligma_convolve_sum_matrix       (const gfloat     *matrix);


G_DEFINE_TYPE (LigmaConvolve, ligma_convolve, LIGMA_TYPE_BRUSH_CORE)


void
ligma_convolve_register (Ligma                      *ligma,
                        LigmaPaintRegisterCallback  callback)
{
  (* callback) (ligma,
                LIGMA_TYPE_CONVOLVE,
                LIGMA_TYPE_CONVOLVE_OPTIONS,
                "ligma-convolve",
                _("Convolve"),
                "ligma-tool-blur");
}

static void
ligma_convolve_class_init (LigmaConvolveClass *klass)
{
  LigmaPaintCoreClass *paint_core_class = LIGMA_PAINT_CORE_CLASS (klass);

  paint_core_class->paint = ligma_convolve_paint;
}

static void
ligma_convolve_init (LigmaConvolve *convolve)
{
  gint i;

  for (i = 0; i < 9; i++)
    convolve->matrix[i] = 1.0;

  convolve->matrix_divisor = 9.0;
}

static void
ligma_convolve_paint (LigmaPaintCore    *paint_core,
                     GList            *drawables,
                     LigmaPaintOptions *paint_options,
                     LigmaSymmetry     *sym,
                     LigmaPaintState    paint_state,
                     guint32           time)
{
  g_return_if_fail (g_list_length (drawables) == 1);

  switch (paint_state)
    {
    case LIGMA_PAINT_STATE_MOTION:
      for (GList *iter = drawables; iter; iter = iter->next)
        ligma_convolve_motion (paint_core, iter->data, paint_options, sym);
      break;

    default:
      break;
    }
}

static void
ligma_convolve_motion (LigmaPaintCore    *paint_core,
                      LigmaDrawable     *drawable,
                      LigmaPaintOptions *paint_options,
                      LigmaSymmetry     *sym)
{
  LigmaConvolve        *convolve   = LIGMA_CONVOLVE (paint_core);
  LigmaBrushCore       *brush_core = LIGMA_BRUSH_CORE (paint_core);
  LigmaConvolveOptions *options    = LIGMA_CONVOLVE_OPTIONS (paint_options);
  LigmaContext         *context    = LIGMA_CONTEXT (paint_options);
  LigmaDynamics        *dynamics   = LIGMA_BRUSH_CORE (paint_core)->dynamics;
  LigmaImage           *image      = ligma_item_get_image (LIGMA_ITEM (drawable));
  GeglBuffer          *paint_buffer;
  gint                 paint_buffer_x;
  gint                 paint_buffer_y;
  LigmaTempBuf         *temp_buf;
  GeglBuffer          *convolve_buffer;
  gdouble              fade_point;
  gdouble              opacity;
  gdouble              rate;
  LigmaCoords           coords;
  gint                 off_x, off_y;
  gint                 paint_width, paint_height;
  gint                 n_strokes;
  gint                 i;

  fade_point = ligma_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);
  coords = *(ligma_symmetry_get_origin (sym));
  coords.x -= off_x;
  coords.y -= off_y;

  opacity = ligma_dynamics_get_linear_value (dynamics,
                                            LIGMA_DYNAMICS_OUTPUT_OPACITY,
                                            &coords,
                                            paint_options,
                                            fade_point);
  if (opacity == 0.0)
    return;

  ligma_brush_core_eval_transform_dynamics (LIGMA_BRUSH_CORE (paint_core),
                                           image,
                                           paint_options,
                                           &coords);
  n_strokes = ligma_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      coords = *(ligma_symmetry_get_coords (sym, i));
      coords.x -= off_x;
      coords.y -= off_y;

      ligma_brush_core_eval_transform_symmetry (brush_core, sym, i);

      paint_buffer = ligma_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options,
                                                       LIGMA_LAYER_MODE_NORMAL,
                                                       &coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       &paint_width,
                                                       &paint_height);
      if (! paint_buffer)
        continue;

      rate = (options->rate *
              ligma_dynamics_get_linear_value (dynamics,
                                              LIGMA_DYNAMICS_OUTPUT_RATE,
                                              &coords,
                                              paint_options,
                                              fade_point));

      ligma_convolve_calculate_matrix (convolve, options->type,
                                      ligma_brush_get_width  (brush_core->brush) / 2,
                                      ligma_brush_get_height (brush_core->brush) / 2,
                                      rate);

      /*  need a linear buffer for ligma_gegl_convolve()  */
      temp_buf = ligma_temp_buf_new (gegl_buffer_get_width  (paint_buffer),
                                    gegl_buffer_get_height (paint_buffer),
                                    gegl_buffer_get_format (paint_buffer));
      convolve_buffer = ligma_temp_buf_create_buffer (temp_buf);
      ligma_temp_buf_unref (temp_buf);

      ligma_gegl_buffer_copy (
        ligma_drawable_get_buffer (drawable),
        GEGL_RECTANGLE (paint_buffer_x,
                        paint_buffer_y,
                        gegl_buffer_get_width  (paint_buffer),
                        gegl_buffer_get_height (paint_buffer)),
        GEGL_ABYSS_NONE,
        convolve_buffer,
        GEGL_RECTANGLE (0, 0, 0, 0));

      ligma_gegl_convolve (convolve_buffer,
                          GEGL_RECTANGLE (0, 0,
                                          gegl_buffer_get_width  (convolve_buffer),
                                          gegl_buffer_get_height (convolve_buffer)),
                          paint_buffer,
                          GEGL_RECTANGLE (0, 0,
                                          gegl_buffer_get_width  (paint_buffer),
                                          gegl_buffer_get_height (paint_buffer)),
                          convolve->matrix, 3, convolve->matrix_divisor,
                          LIGMA_NORMAL_CONVOL, TRUE);

      g_object_unref (convolve_buffer);

      ligma_brush_core_replace_canvas (brush_core, drawable,
                                      &coords,
                                      MIN (opacity, LIGMA_OPACITY_OPAQUE),
                                      ligma_context_get_opacity (context),
                                      ligma_paint_options_get_brush_mode (paint_options),
                                      1.0,
                                      LIGMA_PAINT_INCREMENTAL);
    }
}

static void
ligma_convolve_calculate_matrix (LigmaConvolve    *convolve,
                                LigmaConvolveType type,
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
    case LIGMA_CONVOLVE_BLUR:
      convolve->matrix[4] = MIN_BLUR + percent * (MAX_BLUR - MIN_BLUR);
      break;

    case LIGMA_CONVOLVE_SHARPEN:
      convolve->matrix[4] = MIN_SHARPEN + percent * (MAX_SHARPEN - MIN_SHARPEN);
      break;
    }

  convolve->matrix[5] = (radius_x)             ? 1.0 : 0.0;
  convolve->matrix[6] = (radius_x && radius_y) ? 1.0 : 0.0;
  convolve->matrix[7] = (radius_y)             ? 1.0 : 0.0;
  convolve->matrix[8] = (radius_x && radius_y) ? 1.0 : 0.0;

  convolve->matrix_divisor = ligma_convolve_sum_matrix (convolve->matrix);
}

static gdouble
ligma_convolve_sum_matrix (const gfloat *matrix)
{
  gdouble sum = 0.0;
  gint    i;

  for (i = 0; i < 9; i++)
    sum += matrix[i];

  return sum;
}
