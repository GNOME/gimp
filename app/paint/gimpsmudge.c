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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "core/gimp-palettes.h"
#include "core/gimpbrush.h"
#include "core/gimpbrush-header.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimpsymmetry.h"
#include "core/gimptempbuf.h"

#include "gimpsmudge.h"
#include "gimpsmudgeoptions.h"

#include "gimp-intl.h"


static void       gimp_smudge_finalize     (GObject          *object);

static void       gimp_smudge_paint        (GimpPaintCore    *paint_core,
                                            GList            *drawables,
                                            GimpPaintOptions *paint_options,
                                            GimpSymmetry     *sym,
                                            GimpPaintState    paint_state,
                                            guint32           time);
static gboolean   gimp_smudge_start        (GimpPaintCore    *paint_core,
                                            GimpDrawable     *drawable,
                                            GimpPaintOptions *paint_options,
                                            GimpSymmetry     *sym);
static void       gimp_smudge_motion       (GimpPaintCore    *paint_core,
                                            GimpDrawable     *drawable,
                                            GimpPaintOptions *paint_options,
                                            GimpSymmetry     *sym);

static void       gimp_smudge_accumulator_coords (GimpPaintCore    *paint_core,
                                                  const GimpCoords *coords,
                                                  gint              stroke,
                                                  gint             *x,
                                                  gint             *y);

static void       gimp_smudge_accumulator_size   (GimpPaintOptions *paint_options,
                                                  const GimpCoords *coords,
                                                  gint             *accumulator_size);


G_DEFINE_TYPE (GimpSmudge, gimp_smudge, GIMP_TYPE_BRUSH_CORE)

#define parent_class gimp_smudge_parent_class


void
gimp_smudge_register (Gimp                      *gimp,
                      GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_SMUDGE,
                GIMP_TYPE_SMUDGE_OPTIONS,
                "gimp-smudge",
                _("Smudge"),
                "gimp-tool-smudge");
}

static void
gimp_smudge_class_init (GimpSmudgeClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  object_class->finalize  = gimp_smudge_finalize;

  paint_core_class->paint = gimp_smudge_paint;

  brush_core_class->handles_changing_brush             = TRUE;
  brush_core_class->handles_transforming_brush         = TRUE;
  brush_core_class->handles_dynamic_transforming_brush = TRUE;
}

static void
gimp_smudge_init (GimpSmudge *smudge)
{
}

static void
gimp_smudge_finalize (GObject *object)
{
  GimpSmudge *smudge = GIMP_SMUDGE (object);

  if (smudge->accum_buffers)
    {
      GList *iter;

      for (iter = smudge->accum_buffers; iter; iter = g_list_next (iter))
        {
          if (iter->data)
            g_object_unref (iter->data);
        }

      g_list_free (smudge->accum_buffers);
      smudge->accum_buffers = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_smudge_paint (GimpPaintCore    *paint_core,
                   GList            *drawables,
                   GimpPaintOptions *paint_options,
                   GimpSymmetry     *sym,
                   GimpPaintState    paint_state,
                   guint32           time)
{
  GimpSmudge *smudge = GIMP_SMUDGE (paint_core);

  g_return_if_fail (g_list_length (drawables) == 1);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      {
        GimpContext       *context    = GIMP_CONTEXT (paint_options);
        GimpSmudgeOptions *options    = GIMP_SMUDGE_OPTIONS (paint_options);
        GimpBrushCore     *brush_core = GIMP_BRUSH_CORE (paint_core);
        GimpDynamics      *dynamics   = gimp_context_get_dynamics (context);

        /* Don't add to color history when
         * 1. pure smudging (flow=0)
         * 2. color is from gradient or pixmap brushes
         */
        if (options->flow > 0.0 &&
            ! gimp_dynamics_is_output_enabled (dynamics, GIMP_DYNAMICS_OUTPUT_COLOR) &&
            ! (brush_core->brush && gimp_brush_get_pixmap (brush_core->brush)))
          gimp_palettes_add_color_history (context->gimp, gimp_context_get_foreground (context));
      }
      break;

    case GIMP_PAINT_STATE_MOTION:
      /* initialization fails if the user starts outside the drawable */
      if (! smudge->initialized)
        smudge->initialized = gimp_smudge_start (paint_core, drawables->data,
                                                 paint_options, sym);

      if (smudge->initialized)
        gimp_smudge_motion (paint_core, drawables->data, paint_options, sym);
      break;

    case GIMP_PAINT_STATE_FINISH:
      if (smudge->accum_buffers)
        {
          GList *iter;

          for  (iter = smudge->accum_buffers; iter; iter = g_list_next (iter))
            {
              if (iter->data)
                g_object_unref (iter->data);
            }

          g_list_free (smudge->accum_buffers);
          smudge->accum_buffers = NULL;
        }

      smudge->initialized = FALSE;
      break;

    default:
      break;
    }
}

static gboolean
gimp_smudge_start (GimpPaintCore    *paint_core,
                   GimpDrawable     *drawable,
                   GimpPaintOptions *paint_options,
                   GimpSymmetry     *sym)
{
  GimpSmudge        *smudge     = GIMP_SMUDGE (paint_core);
  GimpBrushCore     *brush_core = GIMP_BRUSH_CORE (paint_core);
  GimpSmudgeOptions *options    = GIMP_SMUDGE_OPTIONS (paint_options);
  GimpPickable      *dest_pickable;
  GeglBuffer        *pickable_buffer;
  GeglBuffer        *paint_buffer;
  GimpCoords         coords;
  gint               dest_pickable_off_x;
  gint               dest_pickable_off_y;
  gint               paint_buffer_x;
  gint               paint_buffer_y;
  gint               accum_size;
  gint               n_strokes;
  gint               i;
  gint               x, y;
  gint               off_x, off_y;

  coords = *(gimp_symmetry_get_origin (sym));

  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
  coords.x -= off_x;
  coords.y -= off_y;

  gimp_brush_core_eval_transform_dynamics (brush_core,
                                           gimp_item_get_image (GIMP_ITEM (drawable)),
                                           paint_options,
                                           &coords);

  if (options->sample_merged)
    {
      dest_pickable = gimp_paint_core_get_image_pickable (paint_core);

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

  pickable_buffer = gimp_pickable_get_buffer (dest_pickable);

  n_strokes = gimp_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      GeglBuffer *accum_buffer;

      coords = *(gimp_symmetry_get_coords (sym, i));
      coords.x -= off_x;
      coords.y -= off_y;

      gimp_smudge_accumulator_size (paint_options, &coords, &accum_size);

      /*  Allocate the accumulation buffer */
      accum_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                      accum_size,
                                                      accum_size),
                                      babl_format ("RGBA float"));
      smudge->accum_buffers = g_list_prepend (smudge->accum_buffers,
                                              accum_buffer);

      /*  adjust the x and y coordinates to the upper left corner of the
       *  accumulator
       */
      paint_buffer = gimp_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options,
                                                       GIMP_LAYER_MODE_NORMAL,
                                                       &coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       NULL, NULL);
      if (! paint_buffer)
        continue;

      gimp_smudge_accumulator_coords (paint_core, &coords, 0, &x, &y);

      /*  If clipped, prefill the smudge buffer with the color at the
       *  brush position.
       */
      if (x != paint_buffer_x ||
          y != paint_buffer_y ||
          accum_size != gegl_buffer_get_width  (paint_buffer) ||
          accum_size != gegl_buffer_get_height (paint_buffer))
        {
          gfloat     pixel[4];
          GeglColor *color;
          gint       pick_x;
          gint       pick_y;

          pick_x = CLAMP ((gint) coords.x + dest_pickable_off_x,
                          0,
                          gegl_buffer_get_width (pickable_buffer) - 1);
          pick_y = CLAMP ((gint) coords.y + dest_pickable_off_y,
                          0,
                          gegl_buffer_get_height (pickable_buffer) - 1);

          gimp_pickable_get_pixel_at (dest_pickable,
                                      pick_x, pick_y,
                                      babl_format ("RGBA float"),
                                      pixel);

          color = gegl_color_new (NULL);
          gegl_color_set_pixel (color, babl_format ("RGBA float"), pixel);
          gegl_buffer_set_color (accum_buffer, NULL, color);
          g_object_unref (color);
        }

      /* copy the region under the original painthit. */
      gimp_gegl_buffer_copy
        (pickable_buffer,
         GEGL_RECTANGLE (paint_buffer_x + dest_pickable_off_x,
                         paint_buffer_y + dest_pickable_off_y,
                         gegl_buffer_get_width  (paint_buffer),
                         gegl_buffer_get_height (paint_buffer)),
         GEGL_ABYSS_NONE,
         accum_buffer,
         GEGL_RECTANGLE (paint_buffer_x - x,
                         paint_buffer_y - y,
                         0, 0));
    }

  smudge->accum_buffers = g_list_reverse (smudge->accum_buffers);

  return TRUE;
}

static void
gimp_smudge_motion (GimpPaintCore    *paint_core,
                    GimpDrawable     *drawable,
                    GimpPaintOptions *paint_options,
                    GimpSymmetry     *sym)
{
  GimpSmudge         *smudge     = GIMP_SMUDGE (paint_core);
  GimpBrushCore      *brush_core = GIMP_BRUSH_CORE (paint_core);
  GimpSmudgeOptions  *options    = GIMP_SMUDGE_OPTIONS (paint_options);
  GimpContext        *context    = GIMP_CONTEXT (paint_options);
  GimpDynamics       *dynamics   = GIMP_BRUSH_CORE (paint_core)->dynamics;
  GimpImage          *image      = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpPickable       *dest_pickable;
  GeglBuffer         *paint_buffer;
  gint                dest_pickable_off_x;
  gint                dest_pickable_off_y;
  gint                paint_buffer_x;
  gint                paint_buffer_y;
  gint                paint_buffer_width;
  gint                paint_buffer_height;
  /* brush dynamics */
  gdouble             fade_point;
  gdouble             opacity;
  gdouble             rate;
  gdouble             flow;
  gdouble             grad_point;
  /* brush color */
  GeglColor          *brush_color  = NULL; /* whether use single color or pixmap */
  /* accum buffer */
  gint                x, y;
  GeglBuffer         *accum_buffer;
  /* other variables */
  gdouble             force;
  GimpCoords          coords;
  gint                off_x, off_y;
  gint                paint_width, paint_height;
  gint                n_strokes;
  gint                i;

  if (options->sample_merged)
    {
      dest_pickable = gimp_paint_core_get_image_pickable (paint_core);

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

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  coords    = *(gimp_symmetry_get_origin (sym));
  coords.x -= off_x;
  coords.y -= off_y;
  gimp_symmetry_set_origin (sym, drawable, &coords);
  paint_core->sym = sym;

  opacity = gimp_dynamics_get_linear_value (dynamics,
                                            GIMP_DYNAMICS_OUTPUT_OPACITY,
                                            &coords,
                                            paint_options,
                                            fade_point);
  if (opacity == 0.0)
    return;

  gimp_brush_core_eval_transform_dynamics (brush_core,
                                           image,
                                           paint_options,
                                           &coords);

  /* Get brush dynamic values other than opacity */
  rate = ((options->rate / 100.0) *
          gimp_dynamics_get_linear_value (dynamics,
                                          GIMP_DYNAMICS_OUTPUT_RATE,
                                          &coords,
                                          paint_options,
                                          fade_point));

  flow = ((options->flow / 100.0) *
          gimp_dynamics_get_linear_value (dynamics,
                                          GIMP_DYNAMICS_OUTPUT_FLOW,
                                          &coords,
                                          paint_options,
                                          fade_point));

  grad_point = gimp_dynamics_get_linear_value (dynamics,
                                               GIMP_DYNAMICS_OUTPUT_COLOR,
                                               &coords,
                                               paint_options,
                                               fade_point);

  /* Get current gradient color, brush pixmap, or foreground color */
  if (gimp_paint_options_get_gradient_color (paint_options, image,
                                             grad_point,
                                             paint_core->pixel_dist,
                                             &brush_color))
    {
      /* No more processing needed */
    }
  else if (brush_core->brush && gimp_brush_get_pixmap (brush_core->brush))
    {
    }
  else
    {
      brush_color = g_object_ref (gimp_context_get_foreground (context));
    }

  n_strokes = gimp_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      coords    = *(gimp_symmetry_get_coords (sym, i));

      gimp_brush_core_eval_transform_symmetry (brush_core, sym, i);

      paint_buffer = gimp_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options,
                                                       GIMP_LAYER_MODE_NORMAL,
                                                       &coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       &paint_width,
                                                       &paint_height);
      if (! paint_buffer)
        continue;

      paint_buffer_width  = gegl_buffer_get_width  (paint_buffer);
      paint_buffer_height = gegl_buffer_get_height (paint_buffer);

      /*  Get the unclipped accumulator coordinates  */
      gimp_smudge_accumulator_coords (paint_core, &coords, i, &x, &y);

      accum_buffer = g_list_nth_data (smudge->accum_buffers, i);

      /* Old smudge tool:
       *  Smudge uses the buffer Accum.
       *  For each successive painthit Accum is built like this
       *    Accum =  rate*Accum  + (1-rate)*I.
       *  where I is the pixels under the current painthit.
       *  Then the paint area (paint_area) is built as
       *    (Accum,1) (if no alpha),
       */

      /* 2017/4/22: New smudge painting tool:
       * Accum=rate*Accum + (1-rate)*I
       * if brush_color_ptr!=NULL
       *   Paint=(1-flow)*Accum + flow*BrushColor
       * else, draw brush pixmap on the paint_buffer and
       *   Paint=(1-flow)*Accum + flow*Paint
       *
       * For non-pixmap brushes, calculate blending in
       * gimp_gegl_smudge_with_paint() instead of calling
       * gegl_buffer_set_color() to reduce gegl's internal processing.
       */
      if (! brush_color && flow > 0.0)
        {
          gimp_brush_core_color_area_with_pixmap (brush_core, drawable,
                                                  &coords,
                                                  paint_buffer,
                                                  paint_buffer_x,
                                                  paint_buffer_y,
                                                  TRUE);
        }

      gimp_gegl_smudge_with_paint (accum_buffer,
                                   GEGL_RECTANGLE (paint_buffer_x - x,
                                                   paint_buffer_y - y,
                                                   paint_buffer_width,
                                                   paint_buffer_height),
                                   gimp_pickable_get_buffer (dest_pickable),
                                   GEGL_RECTANGLE (paint_buffer_x +
                                                   dest_pickable_off_x,
                                                   paint_buffer_y +
                                                   dest_pickable_off_y,
                                                   paint_buffer_width,
                                                   paint_buffer_height),
                                   brush_color,
                                   paint_buffer,
                                   options->no_erasing,
                                   flow,
                                   rate);

      if (gimp_dynamics_is_output_enabled (dynamics, GIMP_DYNAMICS_OUTPUT_FORCE))
        force = gimp_dynamics_get_linear_value (dynamics,
                                                GIMP_DYNAMICS_OUTPUT_FORCE,
                                                &coords,
                                                paint_options,
                                                fade_point);
      else
        force = paint_options->brush_force;

      gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                      &coords,
                                      MIN (opacity, GIMP_OPACITY_OPAQUE),
                                      gimp_context_get_opacity (context),
                                      gimp_paint_options_get_brush_mode (paint_options),
                                      force,
                                      GIMP_PAINT_INCREMENTAL);
    }

  g_clear_object (&brush_color);
}

static void
gimp_smudge_accumulator_coords (GimpPaintCore    *paint_core,
                                const GimpCoords *coords,
                                gint              stroke,
                                gint             *x,
                                gint             *y)
{
  GimpSmudge *smudge = GIMP_SMUDGE (paint_core);
  GeglBuffer *accum_buffer;

  accum_buffer = g_list_nth_data (smudge->accum_buffers, stroke);

  *x = (gint) coords->x - gegl_buffer_get_width  (accum_buffer) / 2;
  *y = (gint) coords->y - gegl_buffer_get_height (accum_buffer) / 2;
}

static void
gimp_smudge_accumulator_size (GimpPaintOptions *paint_options,
                              const GimpCoords *coords,
                              gint             *accumulator_size)
{
  gdouble max_view_scale = 1.0;
  gdouble max_brush_size;

  if (paint_options->brush_lock_to_view)
    max_view_scale = MAX (coords->xscale, coords->yscale);

  max_brush_size = MIN (paint_options->brush_size / max_view_scale,
                        GIMP_BRUSH_MAX_SIZE);

  /* Note: the max brush mask size plus a border of 1 pixel and a
   * little headroom
   */
  *accumulator_size = ceil (sqrt (2 * SQR (max_brush_size + 1)) + 2);
}
