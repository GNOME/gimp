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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"

#include "paint-types.h"

#include "gegl/ligma-gegl-utils.h"

#include "core/ligma.h"
#include "core/ligma-palettes.h"
#include "core/ligmabrush.h"
#include "core/ligmadrawable.h"
#include "core/ligmadynamics.h"
#include "core/ligmagradient.h"
#include "core/ligmaimage.h"
#include "core/ligmapickable.h"
#include "core/ligmasymmetry.h"
#include "core/ligmatempbuf.h"

#include "ligmapaintbrush.h"
#include "ligmapaintoptions.h"

#include "ligma-intl.h"


static void       ligma_paintbrush_paint                        (LigmaPaintCore             *paint_core,
                                                                GList                     *drawables,
                                                                LigmaPaintOptions          *paint_options,
                                                                LigmaSymmetry              *sym,
                                                                LigmaPaintState             paint_state,
                                                                guint32                    time);

static gboolean   ligma_paintbrush_real_get_color_history_color (LigmaPaintbrush            *paintbrush,
                                                                LigmaDrawable              *drawable,
                                                                LigmaPaintOptions          *paint_options,
                                                                LigmaRGB                   *color);
static void       ligma_paintbrush_real_get_paint_params        (LigmaPaintbrush            *paintbrush,
                                                                LigmaDrawable              *drawable,
                                                                LigmaPaintOptions          *paint_options,
                                                                LigmaSymmetry              *sym,
                                                                gdouble                    grad_point,
                                                                LigmaLayerMode             *paint_mode,
                                                                LigmaPaintApplicationMode  *paint_appl_mode,
                                                                const LigmaTempBuf        **paint_pixmap,
                                                                LigmaRGB                   *paint_color);


G_DEFINE_TYPE (LigmaPaintbrush, ligma_paintbrush, LIGMA_TYPE_BRUSH_CORE)


void
ligma_paintbrush_register (Ligma                      *ligma,
                          LigmaPaintRegisterCallback  callback)
{
  (* callback) (ligma,
                LIGMA_TYPE_PAINTBRUSH,
                LIGMA_TYPE_PAINT_OPTIONS,
                "ligma-paintbrush",
                _("Paintbrush"),
                "ligma-tool-paintbrush");
}

static void
ligma_paintbrush_class_init (LigmaPaintbrushClass *klass)
{
  LigmaPaintCoreClass *paint_core_class = LIGMA_PAINT_CORE_CLASS (klass);
  LigmaBrushCoreClass *brush_core_class = LIGMA_BRUSH_CORE_CLASS (klass);

  paint_core_class->paint                  = ligma_paintbrush_paint;

  brush_core_class->handles_changing_brush = TRUE;

  klass->get_color_history_color           = ligma_paintbrush_real_get_color_history_color;
  klass->get_paint_params                  = ligma_paintbrush_real_get_paint_params;
}

static void
ligma_paintbrush_init (LigmaPaintbrush *paintbrush)
{
}

static void
ligma_paintbrush_paint (LigmaPaintCore    *paint_core,
                       GList            *drawables,
                       LigmaPaintOptions *paint_options,
                       LigmaSymmetry     *sym,
                       LigmaPaintState    paint_state,
                       guint32           time)
{
  LigmaPaintbrush *paintbrush = LIGMA_PAINTBRUSH (paint_core);

  g_return_if_fail (g_list_length (drawables) == 1);

  switch (paint_state)
    {
    case LIGMA_PAINT_STATE_INIT:
      {
        LigmaRGB color;

        for (GList *iter = drawables; iter; iter = iter->next)
          if (LIGMA_PAINTBRUSH_GET_CLASS (paintbrush)->get_color_history_color &&
              LIGMA_PAINTBRUSH_GET_CLASS (paintbrush)->get_color_history_color (paintbrush,
                                                                               iter->data,
                                                                               paint_options,
                                                                               &color))
            {
              LigmaContext *context = LIGMA_CONTEXT (paint_options);

              ligma_palettes_add_color_history (context->ligma, &color);
            }
      }
      break;

    case LIGMA_PAINT_STATE_MOTION:
      for (GList *iter = drawables; iter; iter = iter->next)
        _ligma_paintbrush_motion (paint_core, iter->data, paint_options,
                                 sym, LIGMA_OPACITY_OPAQUE);
      break;

    case LIGMA_PAINT_STATE_FINISH:
      {
        if (paintbrush->paint_buffer)
          {
            g_object_remove_weak_pointer (
              G_OBJECT (paintbrush->paint_buffer),
              (gpointer) &paintbrush->paint_buffer);

            paintbrush->paint_buffer = NULL;
          }

        g_clear_pointer (&paintbrush->paint_pixmap, ligma_temp_buf_unref);
      }
      break;
    }
}

static gboolean
ligma_paintbrush_real_get_color_history_color (LigmaPaintbrush   *paintbrush,
                                              LigmaDrawable     *drawable,
                                              LigmaPaintOptions *paint_options,
                                              LigmaRGB          *color)
{
  LigmaContext   *context    = LIGMA_CONTEXT (paint_options);
  LigmaBrushCore *brush_core = LIGMA_BRUSH_CORE (paintbrush);
  LigmaDynamics  *dynamics   = ligma_context_get_dynamics (context);

  /* We don't save gradient color history and pixmap brushes
   * have no color to save.
   */
  if (ligma_dynamics_is_output_enabled (dynamics, LIGMA_DYNAMICS_OUTPUT_COLOR) ||
      (brush_core->brush && ligma_brush_get_pixmap (brush_core->brush)))
    {
      return FALSE;
    }

  ligma_context_get_foreground (context, color);

  return TRUE;
}

static void
ligma_paintbrush_real_get_paint_params (LigmaPaintbrush            *paintbrush,
                                       LigmaDrawable              *drawable,
                                       LigmaPaintOptions          *paint_options,
                                       LigmaSymmetry              *sym,
                                       gdouble                    grad_point,
                                       LigmaLayerMode             *paint_mode,
                                       LigmaPaintApplicationMode  *paint_appl_mode,
                                       const LigmaTempBuf        **paint_pixmap,
                                       LigmaRGB                   *paint_color)
{
  LigmaPaintCore *paint_core = LIGMA_PAINT_CORE (paintbrush);
  LigmaBrushCore *brush_core = LIGMA_BRUSH_CORE (paintbrush);
  LigmaContext   *context    = LIGMA_CONTEXT (paint_options);
  LigmaImage     *image      = ligma_item_get_image (LIGMA_ITEM (drawable));

  *paint_mode = ligma_context_get_paint_mode (context);

  if (ligma_paint_options_get_gradient_color (paint_options, image,
                                             grad_point,
                                             paint_core->pixel_dist,
                                             paint_color))
    {
      /* optionally take the color from the current gradient */
      ligma_pickable_srgb_to_image_color (LIGMA_PICKABLE (drawable),
                                         paint_color, paint_color);

      *paint_appl_mode = LIGMA_PAINT_INCREMENTAL;
    }
  else if (brush_core->brush && ligma_brush_get_pixmap (brush_core->brush))
    {
      /* otherwise check if the brush has a pixmap and use that to
       * color the area
       */
      *paint_pixmap = ligma_brush_core_get_brush_pixmap (brush_core);

      *paint_appl_mode = LIGMA_PAINT_INCREMENTAL;
    }
  else
    {
      /* otherwise fill the area with the foreground color */
      ligma_context_get_foreground (context, paint_color);

      ligma_pickable_srgb_to_image_color (LIGMA_PICKABLE (drawable),
                                         paint_color, paint_color);
    }
}

void
_ligma_paintbrush_motion (LigmaPaintCore    *paint_core,
                         LigmaDrawable     *drawable,
                         LigmaPaintOptions *paint_options,
                         LigmaSymmetry     *sym,
                         gdouble           opacity)
{
  LigmaBrushCore    *brush_core = LIGMA_BRUSH_CORE (paint_core);
  LigmaPaintbrush   *paintbrush = LIGMA_PAINTBRUSH (paint_core);
  LigmaContext      *context    = LIGMA_CONTEXT (paint_options);
  LigmaDynamics     *dynamics   = brush_core->dynamics;
  LigmaImage        *image      = ligma_item_get_image (LIGMA_ITEM (drawable));
  gdouble           fade_point;
  gdouble           grad_point;
  gdouble           force;
  LigmaCoords        coords;
  gint              n_strokes;
  gint              off_x, off_y;
  gint              i;

  fade_point = ligma_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

  coords    = *(ligma_symmetry_get_origin (sym));
  coords.x -= off_x;
  coords.y -= off_y;

  /* Some settings are based on the original stroke. */
  opacity *= ligma_dynamics_get_linear_value (dynamics,
                                             LIGMA_DYNAMICS_OUTPUT_OPACITY,
                                             &coords,
                                             paint_options,
                                             fade_point);
  if (opacity == 0.0)
    return;

  if (LIGMA_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
    {
      ligma_brush_core_eval_transform_dynamics (brush_core,
                                               image,
                                               paint_options,
                                               &coords);
    }

  grad_point = ligma_dynamics_get_linear_value (dynamics,
                                               LIGMA_DYNAMICS_OUTPUT_COLOR,
                                               &coords,
                                               paint_options,
                                               fade_point);

  n_strokes = ligma_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      LigmaLayerMode             paint_mode;
      LigmaPaintApplicationMode  paint_appl_mode;
      GeglBuffer               *paint_buffer;
      gint                      paint_buffer_x;
      gint                      paint_buffer_y;
      const LigmaTempBuf        *paint_pixmap = NULL;
      LigmaRGB                   paint_color;
      gint                      paint_width, paint_height;

      paint_appl_mode = paint_options->application_mode;

      LIGMA_PAINTBRUSH_GET_CLASS (paintbrush)->get_paint_params (paintbrush,
                                                                drawable,
                                                                paint_options,
                                                                sym,
                                                                grad_point,
                                                                &paint_mode,
                                                                &paint_appl_mode,
                                                                &paint_pixmap,
                                                                &paint_color);

      coords    = *(ligma_symmetry_get_coords (sym, i));
      coords.x -= off_x;
      coords.y -= off_y;

      if (LIGMA_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
        ligma_brush_core_eval_transform_symmetry (brush_core, sym, i);

      paint_buffer = ligma_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options,
                                                       paint_mode,
                                                       &coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       &paint_width,
                                                       &paint_height);
      if (! paint_buffer)
        continue;

      if (! paint_pixmap)
        {
          opacity *= paint_color.a;
          ligma_rgb_set_alpha (&paint_color, LIGMA_OPACITY_OPAQUE);
        }

      /* fill the paint buffer.  we can skip this step when reusing the
       * previous paint buffer, if the paint color/pixmap hasn't changed
       * (unless using an applicator, which currently modifies the paint buffer
       * in-place).
       */
      if (paint_core->applicators                  ||
          paint_buffer != paintbrush->paint_buffer ||
          paint_pixmap != paintbrush->paint_pixmap ||
          (! paint_pixmap && (ligma_rgba_distance (&paint_color,
                                                  &paintbrush->paint_color))))
        {
          if (paint_buffer != paintbrush->paint_buffer)
            {
              if (paintbrush->paint_buffer)
                {
                  g_object_remove_weak_pointer (
                    G_OBJECT (paintbrush->paint_buffer),
                    (gpointer) &paintbrush->paint_buffer);
                }

              paintbrush->paint_buffer = paint_buffer;

              g_object_add_weak_pointer (
                G_OBJECT (paintbrush->paint_buffer),
                (gpointer) &paintbrush->paint_buffer);
            }

          if (paint_pixmap != paintbrush->paint_pixmap)
            {
              g_clear_pointer (&paintbrush->paint_pixmap, ligma_temp_buf_unref);

              if (paint_pixmap)
                paintbrush->paint_pixmap = ligma_temp_buf_ref (paint_pixmap);
            }

          paintbrush->paint_color = paint_color;

          if (paint_pixmap)
            {
              ligma_brush_core_color_area_with_pixmap (brush_core, drawable,
                                                      &coords,
                                                      paint_buffer,
                                                      paint_buffer_x,
                                                      paint_buffer_y,
                                                      FALSE);
            }
          else
            {
              GeglColor *color;

              color = ligma_gegl_color_new (&paint_color,
                                           ligma_drawable_get_space (drawable));

              gegl_buffer_set_color (paint_buffer, NULL, color);

              g_object_unref (color);
            }
        }

      if (ligma_dynamics_is_output_enabled (dynamics, LIGMA_DYNAMICS_OUTPUT_FORCE))
        force = ligma_dynamics_get_linear_value (dynamics,
                                                LIGMA_DYNAMICS_OUTPUT_FORCE,
                                                &coords,
                                                paint_options,
                                                fade_point);
      else
        force = paint_options->brush_force;

      /* finally, let the brush core paste the colored area on the canvas */
      ligma_brush_core_paste_canvas (brush_core, drawable,
                                    &coords,
                                    MIN (opacity, LIGMA_OPACITY_OPAQUE),
                                    ligma_context_get_opacity (context),
                                    paint_mode,
                                    ligma_paint_options_get_brush_mode (paint_options),
                                    force,
                                    paint_appl_mode);
    }
}
