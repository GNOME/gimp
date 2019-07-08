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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "core/gimpbrush-header.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpmarshal.h"
#include "core/gimpsymmetry.h"
#include "core/gimptempbuf.h"

#include "gimpbrushcore.h"
#include "gimpbrushcore-loops.h"
#include "gimpbrushcore-kernels.h"

#include "gimppaintoptions.h"

#include "gimp-intl.h"


#define EPSILON  0.00001

enum
{
  SET_BRUSH,
  SET_DYNAMICS,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void      gimp_brush_core_finalize           (GObject          *object);

static gboolean  gimp_brush_core_start              (GimpPaintCore    *core,
                                                     GimpDrawable     *drawable,
                                                     GimpPaintOptions *paint_options,
                                                     const GimpCoords *coords,
                                                     GError          **error);
static gboolean  gimp_brush_core_pre_paint          (GimpPaintCore    *core,
                                                     GimpDrawable     *drawable,
                                                     GimpPaintOptions *paint_options,
                                                     GimpPaintState    paint_state,
                                                     guint32           time);
static void      gimp_brush_core_post_paint         (GimpPaintCore    *core,
                                                     GimpDrawable     *drawable,
                                                     GimpPaintOptions *paint_options,
                                                     GimpPaintState    paint_state,
                                                     guint32           time);
static void      gimp_brush_core_interpolate        (GimpPaintCore    *core,
                                                     GimpDrawable     *drawable,
                                                     GimpPaintOptions *paint_options,
                                                     guint32           time);

static GeglBuffer * gimp_brush_core_get_paint_buffer(GimpPaintCore    *paint_core,
                                                     GimpDrawable     *drawable,
                                                     GimpPaintOptions *paint_options,
                                                     GimpLayerMode     paint_mode,
                                                     const GimpCoords *coords,
                                                     gint             *paint_buffer_x,
                                                     gint             *paint_buffer_y,
                                                     gint             *paint_width,
                                                     gint             *paint_height);

static void      gimp_brush_core_real_set_brush     (GimpBrushCore    *core,
                                                     GimpBrush        *brush);
static void      gimp_brush_core_real_set_dynamics  (GimpBrushCore    *core,
                                                     GimpDynamics     *dynamics);

static gdouble   gimp_brush_core_get_angle          (GimpBrushCore     *core);
static gboolean  gimp_brush_core_get_reflect        (GimpBrushCore     *core);

static const GimpTempBuf *
                 gimp_brush_core_transform_mask     (GimpBrushCore     *core,
                                                     GimpBrush         *brush);

static void      gimp_brush_core_invalidate_cache   (GimpBrush         *brush,
                                                     GimpBrushCore     *core);


G_DEFINE_TYPE (GimpBrushCore, gimp_brush_core, GIMP_TYPE_PAINT_CORE)

#define parent_class gimp_brush_core_parent_class

static guint core_signals[LAST_SIGNAL] = { 0, };


static void
gimp_brush_core_class_init (GimpBrushCoreClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  core_signals[SET_BRUSH] =
    g_signal_new ("set-brush",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpBrushCoreClass, set_brush),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_BRUSH);

  core_signals[SET_DYNAMICS] =
    g_signal_new ("set-dynamics",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpBrushCoreClass, set_dynamics),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DYNAMICS);

  object_class->finalize                    = gimp_brush_core_finalize;

  paint_core_class->start                   = gimp_brush_core_start;
  paint_core_class->pre_paint               = gimp_brush_core_pre_paint;
  paint_core_class->post_paint              = gimp_brush_core_post_paint;
  paint_core_class->interpolate             = gimp_brush_core_interpolate;
  paint_core_class->get_paint_buffer        = gimp_brush_core_get_paint_buffer;

  klass->handles_changing_brush             = FALSE;
  klass->handles_transforming_brush         = TRUE;
  klass->handles_dynamic_transforming_brush = TRUE;

  klass->set_brush                          = gimp_brush_core_real_set_brush;
  klass->set_dynamics                       = gimp_brush_core_real_set_dynamics;
}

static void
gimp_brush_core_init (GimpBrushCore *core)
{
  gint i, j;

  core->main_brush                   = NULL;
  core->brush                        = NULL;
  core->dynamics                     = NULL;
  core->spacing                      = 1.0;
  core->scale                        = 1.0;
  core->angle                        = 0.0;
  core->reflect                      = FALSE;
  core->hardness                     = 1.0;
  core->aspect_ratio                 = 0.0;

  core->symmetry_angle               = 0.0;
  core->symmetry_reflect             = FALSE;

  core->pressure_brush               = NULL;

  core->last_solid_brush_mask        = NULL;
  core->solid_cache_invalid          = FALSE;

  core->transform_brush              = NULL;
  core->transform_pixmap             = NULL;

  core->last_subsample_brush_mask    = NULL;
  core->subsample_cache_invalid      = FALSE;

  core->rand                         = g_rand_new ();

  for (i = 0; i < BRUSH_CORE_SOLID_SUBSAMPLE; i++)
    {
      for (j = 0; j < BRUSH_CORE_SOLID_SUBSAMPLE; j++)
        {
          core->solid_brushes[i][j] = NULL;
        }
    }

  for (i = 0; i < BRUSH_CORE_JITTER_LUTSIZE - 1; ++i)
    {
      core->jitter_lut_y[i] = cos (gimp_deg_to_rad (i * 360 /
                                                    BRUSH_CORE_JITTER_LUTSIZE));
      core->jitter_lut_x[i] = sin (gimp_deg_to_rad (i * 360 /
                                                    BRUSH_CORE_JITTER_LUTSIZE));
    }

  gimp_assert (BRUSH_CORE_SUBSAMPLE == KERNEL_SUBSAMPLE);

  for (i = 0; i < KERNEL_SUBSAMPLE + 1; i++)
    {
      for (j = 0; j < KERNEL_SUBSAMPLE + 1; j++)
        {
          core->subsample_brushes[i][j] = NULL;
        }
    }
}

static void
gimp_brush_core_finalize (GObject *object)
{
  GimpBrushCore *core = GIMP_BRUSH_CORE (object);
  gint           i, j;

  g_clear_pointer (&core->pressure_brush, gimp_temp_buf_unref);

  for (i = 0; i < BRUSH_CORE_SOLID_SUBSAMPLE; i++)
    for (j = 0; j < BRUSH_CORE_SOLID_SUBSAMPLE; j++)
      g_clear_pointer (&core->solid_brushes[i][j], gimp_temp_buf_unref);

  g_clear_pointer (&core->rand, g_rand_free);

  for (i = 0; i < KERNEL_SUBSAMPLE + 1; i++)
    for (j = 0; j < KERNEL_SUBSAMPLE + 1; j++)
      g_clear_pointer (&core->subsample_brushes[i][j], gimp_temp_buf_unref);

  if (core->main_brush)
    {
      g_signal_handlers_disconnect_by_func (core->main_brush,
                                            gimp_brush_core_invalidate_cache,
                                            core);
      gimp_brush_end_use (core->main_brush);
      g_clear_object (&core->main_brush);
    }

  g_clear_object (&core->dynamics);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_brush_core_pre_paint (GimpPaintCore    *paint_core,
                           GimpDrawable     *drawable,
                           GimpPaintOptions *paint_options,
                           GimpPaintState    paint_state,
                           guint32           time)
{
  GimpBrushCore *core = GIMP_BRUSH_CORE (paint_core);

  if (paint_state == GIMP_PAINT_STATE_MOTION)
    {
      GimpCoords last_coords;
      GimpCoords current_coords;
      gdouble scale;

      gimp_paint_core_get_last_coords (paint_core, &last_coords);
      gimp_paint_core_get_current_coords (paint_core, &current_coords);

      /* If we current point == last point, check if the brush
       * wants to be painted in that case. (Direction dependent
       * pixmap brush pipes don't, as they don't know which
       * pixmap to select.)
       */
      if (last_coords.x == current_coords.x &&
          last_coords.y == current_coords.y &&
          ! gimp_brush_want_null_motion (core->main_brush,
                                         &last_coords,
                                         &current_coords))
        {
          return FALSE;
        }
      /*No drawing anything if the scale is too small*/
      if (GIMP_BRUSH_CORE_GET_CLASS (core)->handles_transforming_brush)
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (drawable));
          gdouble    fade_point;

          if (GIMP_BRUSH_CORE_GET_CLASS (core)->handles_dynamic_transforming_brush)
            {
              gdouble width;
              gdouble height;

              fade_point = gimp_paint_options_get_fade (paint_options, image,
                                                        paint_core->pixel_dist);
              width = gimp_brush_get_width  (core->main_brush);
              height = gimp_brush_get_height (core->main_brush);

              scale = paint_options->brush_size /
                      MAX (width, height) *
                      gimp_dynamics_get_linear_value (core->dynamics,
                                                      GIMP_DYNAMICS_OUTPUT_SIZE,
                                                      &current_coords,
                                                      paint_options,
                                                      fade_point);

              if (paint_options->brush_lock_to_view &&
                  MAX (current_coords.xscale, current_coords.yscale) > 0)
                {
                  scale /= MAX (current_coords.xscale, current_coords.yscale);

                  /* Cap transform result for brushes or OOM can occur */
                  if ((scale * MAX (width, height)) > GIMP_BRUSH_MAX_SIZE)
                    {
                      scale = GIMP_BRUSH_MAX_SIZE / MAX (width, height);
                    }
                }

              if (scale < 0.0000001)
                return FALSE;
            }
        }

      if (GIMP_BRUSH_CORE_GET_CLASS (paint_core)->handles_changing_brush)
        {
          core->brush = gimp_brush_select_brush (core->main_brush,
                                                 &last_coords,
                                                 &current_coords);
        }
      if ((! GIMP_IS_BRUSH_GENERATED(core->main_brush)) &&
          (paint_options->brush_hardness != gimp_brush_get_blur_hardness(core->main_brush)))
        {
          gimp_brush_flush_blur_caches(core->main_brush);
        }
    }

  return TRUE;
}

static void
gimp_brush_core_post_paint (GimpPaintCore    *paint_core,
                            GimpDrawable     *drawable,
                            GimpPaintOptions *paint_options,
                            GimpPaintState    paint_state,
                            guint32           time)
{
  GimpBrushCore *core = GIMP_BRUSH_CORE (paint_core);

  if (paint_state == GIMP_PAINT_STATE_MOTION)
    {
      core->brush = core->main_brush;
    }
}

static gboolean
gimp_brush_core_start (GimpPaintCore     *paint_core,
                       GimpDrawable      *drawable,
                       GimpPaintOptions  *paint_options,
                       const GimpCoords  *coords,
                       GError           **error)
{
  GimpBrushCore *core    = GIMP_BRUSH_CORE (paint_core);
  GimpContext   *context = GIMP_CONTEXT (paint_options);

  gimp_brush_core_set_brush (core, gimp_context_get_brush (context));

  gimp_brush_core_set_dynamics (core, gimp_context_get_dynamics (context));

  if (! core->main_brush)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("No brushes available for use with this tool."));
      return FALSE;
    }

  if (! core->dynamics)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("No paint dynamics available for use with this tool."));
      return FALSE;
    }

  if (GIMP_BRUSH_CORE_GET_CLASS (core)->handles_transforming_brush)
    {
      gimp_brush_core_eval_transform_dynamics (core,
                                               drawable,
                                               paint_options,
                                               coords);

      gimp_brush_core_eval_transform_symmetry (core, NULL, 0);
    }

  core->spacing = paint_options->brush_spacing;

  core->brush = core->main_brush;

  core->jitter =
    gimp_paint_options_get_jitter (paint_options,
                                   gimp_item_get_image (GIMP_ITEM (drawable)));

  return TRUE;
}

/**
 * gimp_avoid_exact_integer
 * @x: points to a gdouble
 *
 * Adjusts *x such that it is not too close to an integer. This is used
 * for decision algorithms that would be vulnerable to rounding glitches
 * if exact integers were input.
 *
 * Side effects: Changes the value of *x
 **/
static void
gimp_avoid_exact_integer (gdouble *x)
{
  const gdouble integral   = floor (*x);
  const gdouble fractional = *x - integral;

  if (fractional < EPSILON)
    {
      *x = integral + EPSILON;
    }
  else if (fractional > (1 -EPSILON))
    {
      *x = integral + (1 - EPSILON);
    }
}

static void
gimp_brush_core_interpolate (GimpPaintCore    *paint_core,
                             GimpDrawable     *drawable,
                             GimpPaintOptions *paint_options,
                             guint32           time)
{
  GimpBrushCore      *core  = GIMP_BRUSH_CORE (paint_core);
  GimpImage          *image = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpDynamicsOutput *spacing_output;
  GimpCoords          last_coords;
  GimpCoords          current_coords;
  GimpVector2         delta_vec;
  gdouble             delta_pressure;
  gdouble             delta_xtilt, delta_ytilt;
  gdouble             delta_wheel;
  gdouble             delta_velocity;
  gdouble             temp_direction;
  GimpVector2         temp_vec;
  gint                n, num_points;
  gdouble             t0, dt, tn;
  gdouble             st_factor, st_offset;
  gdouble             initial;
  gdouble             dist;
  gdouble             total;
  gdouble             pixel_dist;
  gdouble             pixel_initial;
  gdouble             xd, yd;
  gdouble             mag;
  gdouble             dyn_spacing = core->spacing;
  gdouble             fade_point;
  gboolean            use_dyn_spacing;

  g_return_if_fail (GIMP_IS_BRUSH (core->brush));

  gimp_paint_core_get_last_coords (paint_core, &last_coords);
  gimp_paint_core_get_current_coords (paint_core, &current_coords);

  gimp_avoid_exact_integer (&last_coords.x);
  gimp_avoid_exact_integer (&last_coords.y);
  gimp_avoid_exact_integer (&current_coords.x);
  gimp_avoid_exact_integer (&current_coords.y);

  delta_vec.x    = current_coords.x        - last_coords.x;
  delta_vec.y    = current_coords.y        - last_coords.y;
  delta_pressure = current_coords.pressure - last_coords.pressure;
  delta_xtilt    = current_coords.xtilt    - last_coords.xtilt;
  delta_ytilt    = current_coords.ytilt    - last_coords.ytilt;
  delta_wheel    = current_coords.wheel    - last_coords.wheel;
  delta_velocity = current_coords.velocity - last_coords.velocity;
  temp_direction = current_coords.direction;

  /*  return if there has been no motion  */
  if (! delta_vec.x    &&
      ! delta_vec.y    &&
      ! delta_pressure &&
      ! delta_xtilt    &&
      ! delta_ytilt    &&
      ! delta_wheel    &&
      ! delta_velocity)
    return;

  pixel_dist    = gimp_vector2_length (&delta_vec);
  pixel_initial = paint_core->pixel_dist;

  /*  Zero sized brushes are unfit for interpolate, so we just let
   *  paint core fail on its own
   */
  if (core->scale == 0.0)
    {
      gimp_paint_core_set_last_coords (paint_core, &current_coords);

      gimp_paint_core_paint (paint_core, drawable, paint_options,
                             GIMP_PAINT_STATE_MOTION, time);

      paint_core->pixel_dist = pixel_initial + pixel_dist; /* Don't forget to update pixel distance*/

      return;
    }

  /* Handle dynamic spacing */
  spacing_output = gimp_dynamics_get_output (core->dynamics,
                                             GIMP_DYNAMICS_OUTPUT_SPACING);

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  use_dyn_spacing = gimp_dynamics_output_is_enabled (spacing_output);

  if (use_dyn_spacing)
    {
      dyn_spacing = gimp_dynamics_output_get_linear_value (spacing_output,
                                                           &current_coords,
                                                           paint_options,
                                                           fade_point);

      /* Dynamic spacing assumes that the value set in core is the min
       * value and the max is full 200% spacing. This approach differs
       * from the usual factor from user input approach because making
       * spacing smaller than the nominal value is unlikely and
       * spacing has a hard defined max.
       */
      dyn_spacing = (core->spacing +
                     ((2.0 - core->spacing) * (1.0 - dyn_spacing)));

      /*  Limiting spacing to minimum 1%  */
      dyn_spacing = MAX (core->spacing, dyn_spacing);
    }

  /* calculate the distance traveled in the coordinate space of the brush */
  temp_vec = gimp_brush_get_x_axis (core->brush);
  gimp_vector2_mul (&temp_vec, core->scale);
  gimp_vector2_rotate (&temp_vec, core->angle * G_PI * 2);

  mag = gimp_vector2_length (&temp_vec);
  xd  = gimp_vector2_inner_product (&delta_vec, &temp_vec) / (mag * mag);

  temp_vec = gimp_brush_get_y_axis (core->brush);
  gimp_vector2_mul (&temp_vec, core->scale);
  gimp_vector2_rotate (&temp_vec, core->angle * G_PI * 2);

  mag = gimp_vector2_length (&temp_vec);
  yd  = gimp_vector2_inner_product (&delta_vec, &temp_vec) / (mag * mag);

  dist    = 0.5 * sqrt (xd * xd + yd * yd);
  total   = dist + paint_core->distance;
  initial = paint_core->distance;


  if (delta_vec.x * delta_vec.x > delta_vec.y * delta_vec.y)
    {
      st_factor = delta_vec.x;
      st_offset = last_coords.x - 0.5;
    }
  else
    {
      st_factor = delta_vec.y;
      st_offset = last_coords.y - 0.5;
    }

  if (use_dyn_spacing)
    {
      gint s0;

      num_points = dist / dyn_spacing;

      s0 = (gint) floor (st_offset + 0.5);
      t0 = (s0 - st_offset) / st_factor;
      dt = dyn_spacing / dist;

      if (num_points == 0)
        return;
    }
  else if (fabs (st_factor) > dist / core->spacing)
    {
      /*  The stripe principle leads to brush positions that are spaced
       *  *closer* than the official brush spacing. Use the official
       *  spacing instead. This is the common case when the brush spacing
       *  is large.
       *  The net effect is then to put a lower bound on the spacing, but
       *  one that varies with the slope of the line. This is suppose to
       *  make thin lines (say, with a 1x1 brush) prettier while leaving
       *  lines with larger brush spacing as they used to look in 1.2.x.
       */

      dt = core->spacing / dist;
      n = (gint) (initial / core->spacing + 1.0 + EPSILON);
      t0 = (n * core->spacing - initial) / dist;
      num_points = 1 + (gint) floor ((1 + EPSILON - t0) / dt);

      /* if we arnt going to paint anything this time and the brush
       * has only moved on one axis return without updating the brush
       * position, distance etc. so that we can more accurately space
       * brush strokes when curves are supplied to us in single pixel
       * chunks.
       */

      if (num_points == 0 && (delta_vec.x == 0 || delta_vec.y == 0))
        return;
    }
  else if (fabs (st_factor) < EPSILON)
    {
      /* Hm, we've hardly moved at all. Don't draw anything, but reset the
       * old coordinates and hope we've gone longer the next time...
       */
      current_coords.x = last_coords.x;
      current_coords.y = last_coords.y;

      gimp_paint_core_set_current_coords (paint_core, &current_coords);

      /* ... but go along with the current pressure, tilt and wheel */
      return;
    }
  else
    {
      gint direction = st_factor > 0 ? 1 : -1;
      gint x, y;
      gint s0, sn;

      /*  Choose the first and last stripe to paint.
       *    FIRST PRIORITY is to avoid gaps painting with a 1x1 aliasing
       *  brush when a horizontalish line segment follows a verticalish
       *  one or vice versa - no matter what the angle between the two
       *  lines is. This will also limit the local thinning that a 1x1
       *  subsampled brush may suffer in the same situation.
       *    SECOND PRIORITY is to avoid making free-hand drawings
       *  unpleasantly fat by plotting redundant points.
       *    These are achieved by the following rules, but it is a little
       *  tricky to see just why. Do not change this algorithm unless you
       *  are sure you know what you're doing!
       */

      /*  Basic case: round the beginning and ending point to nearest
       *  stripe center.
       */
      s0 = (gint) floor (st_offset + 0.5);
      sn = (gint) floor (st_offset + st_factor + 0.5);

      t0 = (s0 - st_offset) / st_factor;
      tn = (sn - st_offset) / st_factor;

      x = (gint) floor (last_coords.x + t0 * delta_vec.x);
      y = (gint) floor (last_coords.y + t0 * delta_vec.y);

      if (t0 < 0.0 && !( x == (gint) floor (last_coords.x) &&
                         y == (gint) floor (last_coords.y) ))
        {
          /*  Exception A: If the first stripe's brush position is
           *  EXTRApolated into a different pixel square than the
           *  ideal starting point, don't plot it.
           */
          s0 += direction;
        }
      else if (x == (gint) floor (paint_core->last_paint.x) &&
               y == (gint) floor (paint_core->last_paint.y))
        {
          /*  Exception B: If first stripe's brush position is within the
           *  same pixel square as the last plot of the previous line,
           *  don't plot it either.
           */
          s0 += direction;
        }

      x = (gint) floor (last_coords.x + tn * delta_vec.x);
      y = (gint) floor (last_coords.y + tn * delta_vec.y);

      if (tn > 1.0 && !( x == (gint) floor (current_coords.x) &&
                         y == (gint) floor (current_coords.y)))
        {
          /*  Exception C: If the last stripe's brush position is
           *  EXTRApolated into a different pixel square than the
           *  ideal ending point, don't plot it.
           */
          sn -= direction;
        }

      t0 = (s0 - st_offset) / st_factor;
      tn = (sn - st_offset) / st_factor;
      dt         =     direction * 1.0 / st_factor;
      num_points = 1 + direction * (sn - s0);

      if (num_points >= 1)
        {
          /*  Hack the reported total distance such that it looks to the
           *  next line as if the the last pixel plotted were at an integer
           *  multiple of the brush spacing. This helps prevent artifacts
           *  for connected lines when the brush spacing is such that some
           *  slopes will use the stripe regime and other slopes will use
           *  the nominal brush spacing.
           */

          if (tn < 1)
            total = initial + tn * dist;

          total = core->spacing * (gint) (total / core->spacing + 0.5);
          total += (1.0 - tn) * dist;
        }
    }

  for (n = 0; n < num_points; n++)
    {
      gdouble t = t0 + n * dt;
      gdouble p = (gdouble) n / num_points;

      current_coords.x         = last_coords.x        + t * delta_vec.x;
      current_coords.y         = last_coords.y        + t * delta_vec.y;
      current_coords.pressure  = last_coords.pressure + p * delta_pressure;
      current_coords.xtilt     = last_coords.xtilt    + p * delta_xtilt;
      current_coords.ytilt     = last_coords.ytilt    + p * delta_ytilt;
      current_coords.wheel     = last_coords.wheel    + p * delta_wheel;
      current_coords.velocity  = last_coords.velocity + p * delta_velocity;
      current_coords.direction = temp_direction;
      current_coords.xscale    = last_coords.xscale;
      current_coords.yscale    = last_coords.yscale;
      current_coords.angle     = last_coords.angle;
      current_coords.reflect   = last_coords.reflect;

      if (core->jitter > 0.0)
        {
          GimpVector2 x_axis;
          GimpVector2 y_axis;
          gdouble     dyn_jitter;
          gdouble     jitter_dist;
          gint32      jitter_angle;

          x_axis = gimp_brush_get_x_axis (core->brush);
          y_axis = gimp_brush_get_y_axis (core->brush);

          dyn_jitter = (core->jitter *
                        gimp_dynamics_get_linear_value (core->dynamics,
                                                        GIMP_DYNAMICS_OUTPUT_JITTER,
                                                        &current_coords,
                                                        paint_options,
                                                        fade_point));

          jitter_dist  = g_rand_double_range (core->rand, 0, dyn_jitter);
          jitter_angle = g_rand_int_range (core->rand,
                                           0, BRUSH_CORE_JITTER_LUTSIZE);

          current_coords.x +=
            (x_axis.x + y_axis.x) *
            jitter_dist * core->jitter_lut_x[jitter_angle] * core->scale;

          current_coords.y +=
            (y_axis.y + x_axis.y) *
            jitter_dist * core->jitter_lut_y[jitter_angle] * core->scale;
        }

      gimp_paint_core_set_current_coords (paint_core, &current_coords);

      paint_core->distance   = initial       + t * dist;
      paint_core->pixel_dist = pixel_initial + t * pixel_dist;

      gimp_paint_core_paint (paint_core, drawable, paint_options,
                             GIMP_PAINT_STATE_MOTION, time);
    }

  current_coords.x        = last_coords.x        + delta_vec.x;
  current_coords.y        = last_coords.y        + delta_vec.y;
  current_coords.pressure = last_coords.pressure + delta_pressure;
  current_coords.xtilt    = last_coords.xtilt    + delta_xtilt;
  current_coords.ytilt    = last_coords.ytilt    + delta_ytilt;
  current_coords.wheel    = last_coords.wheel    + delta_wheel;
  current_coords.velocity = last_coords.velocity + delta_velocity;
  current_coords.xscale   = last_coords.xscale;
  current_coords.yscale   = last_coords.yscale;
  current_coords.angle    = last_coords.angle;
  current_coords.reflect  = last_coords.reflect;

  gimp_paint_core_set_current_coords (paint_core, &current_coords);
  gimp_paint_core_set_last_coords (paint_core, &current_coords);

  paint_core->distance   = total;
  paint_core->pixel_dist = pixel_initial + pixel_dist;
}

static GeglBuffer *
gimp_brush_core_get_paint_buffer (GimpPaintCore    *paint_core,
                                  GimpDrawable     *drawable,
                                  GimpPaintOptions *paint_options,
                                  GimpLayerMode     paint_mode,
                                  const GimpCoords *coords,
                                  gint             *paint_buffer_x,
                                  gint             *paint_buffer_y,
                                  gint             *paint_width,
                                  gint             *paint_height)
{
  GimpBrushCore *core = GIMP_BRUSH_CORE (paint_core);
  gint           x, y;
  gint           x1, y1, x2, y2;
  gint           drawable_width, drawable_height;
  gint           brush_width, brush_height;

  gimp_brush_transform_size (core->brush,
                             core->scale, core->aspect_ratio,
                             gimp_brush_core_get_angle (core),
                             gimp_brush_core_get_reflect (core),
                             &brush_width, &brush_height);

  if (paint_width)
    *paint_width  = brush_width;
  if (paint_height)
    *paint_height = brush_height;

  /*  adjust the x and y coordinates to the upper left corner of the brush  */
  x = (gint) floor (coords->x) - (brush_width  / 2);
  y = (gint) floor (coords->y) - (brush_height / 2);

  drawable_width  = gimp_item_get_width  (GIMP_ITEM (drawable));
  drawable_height = gimp_item_get_height (GIMP_ITEM (drawable));

  x1 = CLAMP (x - 1, 0, drawable_width);
  y1 = CLAMP (y - 1, 0, drawable_height);
  x2 = CLAMP (x + brush_width  + 1, 0, drawable_width);
  y2 = CLAMP (y + brush_height + 1, 0, drawable_height);

  /*  configure the canvas buffer  */
  if ((x2 - x1) && (y2 - y1))
    {
      GimpTempBuf            *temp_buf;
      const Babl             *format;
      GimpLayerCompositeMode  composite_mode;

      composite_mode = gimp_layer_mode_get_paint_composite_mode (paint_mode);

      format = gimp_layer_mode_get_format (paint_mode,
                                           GIMP_LAYER_COLOR_SPACE_AUTO,
                                           GIMP_LAYER_COLOR_SPACE_AUTO,
                                           composite_mode,
                                           gimp_drawable_get_format (drawable));

      if (paint_core->paint_buffer                                       &&
          gegl_buffer_get_width  (paint_core->paint_buffer) == (x2 - x1) &&
          gegl_buffer_get_height (paint_core->paint_buffer) == (y2 - y1) &&
          gegl_buffer_get_format (paint_core->paint_buffer) == format)
        {
          *paint_buffer_x = x1;
          *paint_buffer_y = y1;

          return paint_core->paint_buffer;
        }

      g_clear_object (&paint_core->paint_buffer);

      temp_buf = gimp_temp_buf_new ((x2 - x1), (y2 - y1),
                                    format);

      *paint_buffer_x = x1;
      *paint_buffer_y = y1;

      paint_core->paint_buffer = gimp_temp_buf_create_buffer (temp_buf);

      gimp_temp_buf_unref (temp_buf);

      return paint_core->paint_buffer;
    }

  return NULL;
}

static void
gimp_brush_core_real_set_brush (GimpBrushCore *core,
                                GimpBrush     *brush)
{
  if (brush == core->main_brush)
    return;

  if (core->main_brush)
    {
      g_signal_handlers_disconnect_by_func (core->main_brush,
                                            gimp_brush_core_invalidate_cache,
                                            core);
      gimp_brush_end_use (core->main_brush);
    }

  g_set_object (&core->main_brush, brush);

  if (core->main_brush)
    {
      gimp_brush_begin_use (core->main_brush);
      g_signal_connect (core->main_brush, "invalidate-preview",
                        G_CALLBACK (gimp_brush_core_invalidate_cache),
                        core);
    }
}

static void
gimp_brush_core_real_set_dynamics (GimpBrushCore *core,
                                   GimpDynamics  *dynamics)
{
  g_set_object (&core->dynamics, dynamics);
}

void
gimp_brush_core_set_brush (GimpBrushCore *core,
                           GimpBrush     *brush)
{
  g_return_if_fail (GIMP_IS_BRUSH_CORE (core));
  g_return_if_fail (brush == NULL || GIMP_IS_BRUSH (brush));

  if (brush != core->main_brush)
    g_signal_emit (core, core_signals[SET_BRUSH], 0, brush);
}

void
gimp_brush_core_set_dynamics (GimpBrushCore *core,
                              GimpDynamics  *dynamics)
{
  g_return_if_fail (GIMP_IS_BRUSH_CORE (core));
  g_return_if_fail (dynamics == NULL || GIMP_IS_DYNAMICS (dynamics));

  if (dynamics != core->dynamics)
    g_signal_emit (core, core_signals[SET_DYNAMICS], 0, dynamics);
}

void
gimp_brush_core_paste_canvas (GimpBrushCore            *core,
                              GimpDrawable             *drawable,
                              const GimpCoords         *coords,
                              gdouble                   brush_opacity,
                              gdouble                   image_opacity,
                              GimpLayerMode             paint_mode,
                              GimpBrushApplicationMode  brush_hardness,
                              gdouble                   dynamic_force,
                              GimpPaintApplicationMode  mode)
{
  const GimpTempBuf *brush_mask;

  brush_mask = gimp_brush_core_get_brush_mask (core, coords,
                                               brush_hardness,
                                               dynamic_force);

  if (brush_mask)
    {
      GimpPaintCore *paint_core = GIMP_PAINT_CORE (core);
      gint           x;
      gint           y;
      gint           off_x;
      gint           off_y;

      x = (gint) floor (coords->x) - (gimp_temp_buf_get_width  (brush_mask) >> 1);
      y = (gint) floor (coords->y) - (gimp_temp_buf_get_height (brush_mask) >> 1);

      off_x = (x < 0) ? -x : 0;
      off_y = (y < 0) ? -y : 0;

      gimp_paint_core_paste (paint_core, brush_mask,
                             off_x, off_y,
                             drawable,
                             brush_opacity,
                             image_opacity,
                             paint_mode,
                             mode);
    }
}

/* Similar to gimp_brush_core_paste_canvas, but replaces the alpha channel
 * rather than using it to composite (i.e. transparent over opaque
 * becomes transparent rather than opauqe.
 */
void
gimp_brush_core_replace_canvas (GimpBrushCore            *core,
                                GimpDrawable             *drawable,
                                const GimpCoords         *coords,
                                gdouble                   brush_opacity,
                                gdouble                   image_opacity,
                                GimpBrushApplicationMode  brush_hardness,
                                gdouble                   dynamic_force,
                                GimpPaintApplicationMode  mode)
{
  const GimpTempBuf *brush_mask;

  brush_mask = gimp_brush_core_get_brush_mask (core, coords,
                                               brush_hardness,
                                               dynamic_force);

  if (brush_mask)
    {
      GimpPaintCore *paint_core = GIMP_PAINT_CORE (core);
      gint           x;
      gint           y;
      gint           off_x;
      gint           off_y;

      x = (gint) floor (coords->x) - (gimp_temp_buf_get_width  (brush_mask) >> 1);
      y = (gint) floor (coords->y) - (gimp_temp_buf_get_height (brush_mask) >> 1);

      off_x = (x < 0) ? -x : 0;
      off_y = (y < 0) ? -y : 0;

      gimp_paint_core_replace (paint_core, brush_mask,
                               off_x, off_y,
                               drawable,
                               brush_opacity,
                               image_opacity,
                               mode);
    }
}


static void
gimp_brush_core_invalidate_cache (GimpBrush     *brush,
                                  GimpBrushCore *core)
{
  /* Make sure we don't cache data for a brush that has changed */

  core->subsample_cache_invalid = TRUE;
  core->solid_cache_invalid     = TRUE;

  /* Notify of the brush change */

  g_signal_emit (core, core_signals[SET_BRUSH], 0, brush);
}


/************************************************************
 *             LOCAL FUNCTION DEFINITIONS                   *
 ************************************************************/

static gdouble
gimp_brush_core_get_angle (GimpBrushCore *core)
{
  gdouble angle = core->angle;

  if (core->reflect)
    angle -= core->symmetry_angle;
  else
    angle += core->symmetry_angle;

  angle = fmod (angle, 1.0);

  if (angle < 0.0)
    angle += 1.0;

  return angle;
}

static gboolean
gimp_brush_core_get_reflect (GimpBrushCore *core)
{
  return core->reflect ^ core->symmetry_reflect;
}

static const GimpTempBuf *
gimp_brush_core_transform_mask (GimpBrushCore *core,
                                GimpBrush     *brush)
{
  const GimpTempBuf *mask;

  if (core->scale <= 0.0)
    return NULL;

  mask = gimp_brush_transform_mask (brush,
                                    core->scale,
                                    core->aspect_ratio,
                                    gimp_brush_core_get_angle (core),
                                    gimp_brush_core_get_reflect (core),
                                    core->hardness);

  if (mask == core->transform_brush)
    return mask;

  core->transform_brush         = mask;
  core->subsample_cache_invalid = TRUE;
  core->solid_cache_invalid     = TRUE;

  return core->transform_brush;
}

const GimpTempBuf *
gimp_brush_core_get_brush_mask (GimpBrushCore            *core,
                                const GimpCoords         *coords,
                                GimpBrushApplicationMode  brush_hardness,
                                gdouble                   dynamic_force)
{
  const GimpTempBuf *mask;

  if (dynamic_force <= 0.0)
    return NULL;

  mask = gimp_brush_core_transform_mask (core, core->brush);

  if (! mask)
    return NULL;

  switch (brush_hardness)
    {
    case GIMP_BRUSH_SOFT:
      return gimp_brush_core_subsample_mask (core, mask,
                                             coords->x,
                                             coords->y);
      break;

    case GIMP_BRUSH_HARD:
      return gimp_brush_core_solidify_mask (core, mask,
                                            coords->x,
                                            coords->y);
      break;

    case GIMP_BRUSH_PRESSURE:
      return gimp_brush_core_pressurize_mask (core, mask,
                                              coords->x,
                                              coords->y,
                                              dynamic_force);
      break;
    }

  g_return_val_if_reached (NULL);
}

const GimpTempBuf *
gimp_brush_core_get_brush_pixmap (GimpBrushCore *core)
{
  const GimpTempBuf *pixmap;

  if (core->scale <= 0.0)
    return NULL;

  pixmap = gimp_brush_transform_pixmap (core->brush,
                                        core->scale,
                                        core->aspect_ratio,
                                        gimp_brush_core_get_angle (core),
                                        gimp_brush_core_get_reflect (core),
                                        core->hardness);

  if (pixmap == core->transform_pixmap)
    return pixmap;

  core->transform_pixmap        = pixmap;
  core->subsample_cache_invalid = TRUE;

  return core->transform_pixmap;
}

void
gimp_brush_core_eval_transform_dynamics (GimpBrushCore     *core,
                                         GimpDrawable      *drawable,
                                         GimpPaintOptions  *paint_options,
                                         const GimpCoords  *coords)
{
  if (core->main_brush)
    {
      gdouble max_side;

      max_side = MAX (gimp_brush_get_width  (core->main_brush),
                      gimp_brush_get_height (core->main_brush));

      core->scale = paint_options->brush_size / max_side;

      if (paint_options->brush_lock_to_view &&
          MAX (coords->xscale, coords->yscale) > 0)
        {
          core->scale /= MAX (coords->xscale, coords->yscale);

          /* Cap transform result for brushes or OOM can occur */
          if ((core->scale * max_side) > GIMP_BRUSH_MAX_SIZE)
            {
              core->scale = GIMP_BRUSH_MAX_SIZE / max_side;
            }
        }
   }
  else
    core->scale = -1;

  core->aspect_ratio = paint_options->brush_aspect_ratio;
  core->angle        = paint_options->brush_angle;
  core->reflect      = FALSE;
  core->hardness     = paint_options->brush_hardness;

  if (paint_options->brush_lock_to_view)
    {
      core->angle   += coords->angle;
      core->reflect  = coords->reflect;
    }

  if (! GIMP_IS_DYNAMICS (core->dynamics))
    return;

  if (GIMP_BRUSH_CORE_GET_CLASS (core)->handles_dynamic_transforming_brush)
    {
      gdouble fade_point = 1.0;

      if (drawable)
        {
          GimpImage     *image      = gimp_item_get_image (GIMP_ITEM (drawable));
          GimpPaintCore *paint_core = GIMP_PAINT_CORE (core);

          fade_point = gimp_paint_options_get_fade (paint_options, image,
                                                    paint_core->pixel_dist);
        }

      core->scale *= gimp_dynamics_get_linear_value (core->dynamics,
                                                     GIMP_DYNAMICS_OUTPUT_SIZE,
                                                     coords,
                                                     paint_options,
                                                     fade_point);

      core->angle += gimp_dynamics_get_angular_value (core->dynamics,
                                                      GIMP_DYNAMICS_OUTPUT_ANGLE,
                                                      coords,
                                                      paint_options,
                                                      fade_point);

      core->hardness *= gimp_dynamics_get_linear_value (core->dynamics,
                                                        GIMP_DYNAMICS_OUTPUT_HARDNESS,
                                                        coords,
                                                        paint_options,
                                                        fade_point);

      if (gimp_dynamics_is_output_enabled (core->dynamics,
                                           GIMP_DYNAMICS_OUTPUT_ASPECT_RATIO))
        {
          gdouble dyn_aspect;

          dyn_aspect = gimp_dynamics_get_aspect_value (core->dynamics,
                                                       GIMP_DYNAMICS_OUTPUT_ASPECT_RATIO,
                                                       coords,
                                                       paint_options,
                                                       fade_point);

          /* Zero aspect ratio is special cased to half of all ar range,
           * to force dynamics to have any effect. Forcing to full results
           * in disappearing stamp if applied to maximum.
           */
          if (core->aspect_ratio == 0.0)
            core->aspect_ratio = 10.0 * dyn_aspect;
          else
            core->aspect_ratio *= dyn_aspect;
        }
    }
}

void
gimp_brush_core_eval_transform_symmetry (GimpBrushCore *core,
                                         GimpSymmetry  *symmetry,
                                         gint           stroke)
{
  g_return_if_fail (GIMP_IS_BRUSH_CORE (core));
  g_return_if_fail (symmetry == NULL || GIMP_IS_SYMMETRY (symmetry));

  core->symmetry_angle   = 0.0;
  core->symmetry_reflect = FALSE;

  if (symmetry)
    {
      gimp_symmetry_get_transform (symmetry,
                                   stroke,
                                   &core->symmetry_angle,
                                   &core->symmetry_reflect);

      core->symmetry_angle /= 360.0;
    }
}

void
gimp_brush_core_color_area_with_pixmap (GimpBrushCore    *core,
                                        GimpDrawable     *drawable,
                                        const GimpCoords *coords,
                                        GeglBuffer       *area,
                                        gint              area_x,
                                        gint              area_y,
                                        gboolean          apply_mask)
{
  const GimpTempBuf *pixmap;
  GeglBuffer        *pixmap_buffer;
  const GimpTempBuf *mask;
  GeglBuffer        *mask_buffer;
  gint               area_width;
  gint               area_height;
  gint               ul_x;
  gint               ul_y;
  gint               offset_x;
  gint               offset_y;

  g_return_if_fail (GIMP_IS_BRUSH (core->brush));
  g_return_if_fail (gimp_brush_get_pixmap (core->brush) != NULL);

  /*  scale the brush  */
  pixmap = gimp_brush_core_get_brush_pixmap (core);

  if (! pixmap)
    return;

  if (apply_mask)
    mask = gimp_brush_core_transform_mask (core, core->brush);
  else
    mask = NULL;

  /*  Calculate upper left corner of brush as in
   *  gimp_paint_core_get_paint_area.  Ugly to have to do this here, too.
   */
  ul_x = (gint) floor (coords->x) - (gimp_temp_buf_get_width  (pixmap) >> 1);
  ul_y = (gint) floor (coords->y) - (gimp_temp_buf_get_height (pixmap) >> 1);

  /*  Not sure why this is necessary, but empirically the code does
   *  not work without it for even-sided brushes.  See bug #166622.
   */
  if (gimp_temp_buf_get_width (pixmap) % 2 == 0)
    ul_x += ROUND (coords->x) - floor (coords->x);
  if (gimp_temp_buf_get_height (pixmap) % 2 == 0)
    ul_y += ROUND (coords->y) - floor (coords->y);

  offset_x = area_x - ul_x;
  offset_y = area_y - ul_y;

  area_width  = gegl_buffer_get_width  (area);
  area_height = gegl_buffer_get_height (area);

  pixmap_buffer = gimp_temp_buf_create_buffer (pixmap);

  gimp_gegl_buffer_copy (pixmap_buffer,
                         GEGL_RECTANGLE (offset_x,   offset_y,
                                         area_width, area_height),
                         GEGL_ABYSS_NONE,
                         area,
                         GEGL_RECTANGLE (0,          0,
                                         area_width, area_height));

  g_object_unref (pixmap_buffer);

  if (mask)
    {
      mask_buffer = gimp_temp_buf_create_buffer (mask);

      gimp_gegl_apply_mask (mask_buffer,
                            GEGL_RECTANGLE (offset_x,   offset_y,
                                            area_width, area_height),
                            area,
                            GEGL_RECTANGLE (0,          0,
                                            area_width, area_height),
                            1.0);

      g_object_unref (mask_buffer);
    }
}
