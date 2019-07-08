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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimp-palettes.h"
#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimpsymmetry.h"
#include "core/gimptempbuf.h"

#include "gimppaintbrush.h"
#include "gimppaintoptions.h"

#include "gimp-intl.h"


static void       gimp_paintbrush_paint                        (GimpPaintCore             *paint_core,
                                                                GimpDrawable              *drawable,
                                                                GimpPaintOptions          *paint_options,
                                                                GimpSymmetry              *sym,
                                                                GimpPaintState             paint_state,
                                                                guint32                    time);

static gboolean   gimp_paintbrush_real_get_color_history_color (GimpPaintbrush            *paintbrush,
                                                                GimpDrawable              *drawable,
                                                                GimpPaintOptions          *paint_options,
                                                                GimpRGB                   *color);
static void       gimp_paintbrush_real_get_paint_params        (GimpPaintbrush            *paintbrush,
                                                                GimpDrawable              *drawable,
                                                                GimpPaintOptions          *paint_options,
                                                                GimpSymmetry              *sym,
                                                                GimpLayerMode             *paint_mode,
                                                                GimpPaintApplicationMode  *paint_appl_mode,
                                                                const GimpTempBuf        **paint_pixmap,
                                                                GimpRGB                   *paint_color);


G_DEFINE_TYPE (GimpPaintbrush, gimp_paintbrush, GIMP_TYPE_BRUSH_CORE)


void
gimp_paintbrush_register (Gimp                      *gimp,
                          GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_PAINTBRUSH,
                GIMP_TYPE_PAINT_OPTIONS,
                "gimp-paintbrush",
                _("Paintbrush"),
                "gimp-tool-paintbrush");
}

static void
gimp_paintbrush_class_init (GimpPaintbrushClass *klass)
{
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  paint_core_class->paint                  = gimp_paintbrush_paint;

  brush_core_class->handles_changing_brush = TRUE;

  klass->get_color_history_color           = gimp_paintbrush_real_get_color_history_color;
  klass->get_paint_params                  = gimp_paintbrush_real_get_paint_params;
}

static void
gimp_paintbrush_init (GimpPaintbrush *paintbrush)
{
}

static void
gimp_paintbrush_paint (GimpPaintCore    *paint_core,
                       GimpDrawable     *drawable,
                       GimpPaintOptions *paint_options,
                       GimpSymmetry     *sym,
                       GimpPaintState    paint_state,
                       guint32           time)
{
  GimpPaintbrush *paintbrush = GIMP_PAINTBRUSH (paint_core);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      {
        GimpRGB color;

        if (GIMP_PAINTBRUSH_GET_CLASS (paintbrush)->get_color_history_color &&
            GIMP_PAINTBRUSH_GET_CLASS (paintbrush)->get_color_history_color (
              paintbrush, drawable, paint_options, &color))
          {
            GimpContext *context = GIMP_CONTEXT (paint_options);

            gimp_palettes_add_color_history (context->gimp, &color);
          }
      }
      break;

    case GIMP_PAINT_STATE_MOTION:
      _gimp_paintbrush_motion (paint_core, drawable, paint_options,
                               sym, GIMP_OPACITY_OPAQUE);
      break;

    case GIMP_PAINT_STATE_FINISH:
      {
        if (paintbrush->paint_buffer)
          {
            g_object_remove_weak_pointer (
              G_OBJECT (paintbrush->paint_buffer),
              (gpointer) &paintbrush->paint_buffer);

            paintbrush->paint_buffer = NULL;
          }

        g_clear_pointer (&paintbrush->paint_pixmap, gimp_temp_buf_unref);
      }
      break;
    }
}

static gboolean
gimp_paintbrush_real_get_color_history_color (GimpPaintbrush   *paintbrush,
                                              GimpDrawable     *drawable,
                                              GimpPaintOptions *paint_options,
                                              GimpRGB          *color)
{
  GimpContext   *context    = GIMP_CONTEXT (paint_options);
  GimpBrushCore *brush_core = GIMP_BRUSH_CORE (paintbrush);
  GimpDynamics  *dynamics   = gimp_context_get_dynamics (context);

  /* We don't save gradient color history and pixmap brushes
   * have no color to save.
   */
  if (gimp_dynamics_is_output_enabled (dynamics, GIMP_DYNAMICS_OUTPUT_COLOR) ||
      (brush_core->brush && gimp_brush_get_pixmap (brush_core->brush)))
    {
      return FALSE;
    }

  gimp_context_get_foreground (context, color);

  return TRUE;
}

static void
gimp_paintbrush_real_get_paint_params (GimpPaintbrush            *paintbrush,
                                       GimpDrawable              *drawable,
                                       GimpPaintOptions          *paint_options,
                                       GimpSymmetry              *sym,
                                       GimpLayerMode             *paint_mode,
                                       GimpPaintApplicationMode  *paint_appl_mode,
                                       const GimpTempBuf        **paint_pixmap,
                                       GimpRGB                   *paint_color)
{
  GimpPaintCore    *paint_core = GIMP_PAINT_CORE (paintbrush);
  GimpBrushCore    *brush_core = GIMP_BRUSH_CORE (paintbrush);
  GimpContext      *context    = GIMP_CONTEXT (paint_options);
  GimpDynamics     *dynamics   = brush_core->dynamics;
  GimpImage        *image      = gimp_item_get_image (GIMP_ITEM (drawable));
  const GimpCoords *coords     = gimp_symmetry_get_origin (sym);
  gdouble           fade_point;
  gdouble           grad_point;

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  grad_point = gimp_dynamics_get_linear_value (dynamics,
                                               GIMP_DYNAMICS_OUTPUT_COLOR,
                                               coords,
                                               paint_options,
                                               fade_point);

  *paint_mode = gimp_context_get_paint_mode (context);

  if (gimp_paint_options_get_gradient_color (paint_options, image,
                                             grad_point,
                                             paint_core->pixel_dist,
                                             paint_color))
    {
      /* optionally take the color from the current gradient */
      gimp_pickable_srgb_to_image_color (GIMP_PICKABLE (drawable),
                                         paint_color, paint_color);

      *paint_appl_mode = GIMP_PAINT_INCREMENTAL;
    }
  else if (brush_core->brush && gimp_brush_get_pixmap (brush_core->brush))
    {
      /* otherwise check if the brush has a pixmap and use that to
       * color the area
       */
      *paint_pixmap = gimp_brush_core_get_brush_pixmap (brush_core);

      *paint_appl_mode = GIMP_PAINT_INCREMENTAL;
    }
  else
    {
      /* otherwise fill the area with the foreground color */
      gimp_context_get_foreground (context, paint_color);

      gimp_pickable_srgb_to_image_color (GIMP_PICKABLE (drawable),
                                         paint_color, paint_color);
    }
}

void
_gimp_paintbrush_motion (GimpPaintCore    *paint_core,
                         GimpDrawable     *drawable,
                         GimpPaintOptions *paint_options,
                         GimpSymmetry     *sym,
                         gdouble           opacity)
{
  GimpBrushCore    *brush_core = GIMP_BRUSH_CORE (paint_core);
  GimpPaintbrush   *paintbrush = GIMP_PAINTBRUSH (paint_core);
  GimpContext      *context    = GIMP_CONTEXT (paint_options);
  GimpDynamics     *dynamics   = brush_core->dynamics;
  GimpImage        *image      = gimp_item_get_image (GIMP_ITEM (drawable));
  gdouble           fade_point;
  gdouble           force;
  const GimpCoords *coords;
  gint              n_strokes;
  gint              i;

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  coords = gimp_symmetry_get_origin (sym);
  /* Some settings are based on the original stroke. */
  opacity *= gimp_dynamics_get_linear_value (dynamics,
                                             GIMP_DYNAMICS_OUTPUT_OPACITY,
                                             coords,
                                             paint_options,
                                             fade_point);
  if (opacity == 0.0)
    return;

  if (GIMP_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
    {
      gimp_brush_core_eval_transform_dynamics (brush_core,
                                               drawable,
                                               paint_options,
                                               coords);
    }

  n_strokes = gimp_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      GimpLayerMode             paint_mode;
      GimpPaintApplicationMode  paint_appl_mode;
      GeglBuffer               *paint_buffer;
      gint                      paint_buffer_x;
      gint                      paint_buffer_y;
      const GimpTempBuf        *paint_pixmap = NULL;
      GimpRGB                   paint_color;
      gint                      paint_width, paint_height;

      paint_appl_mode = paint_options->application_mode;

      GIMP_PAINTBRUSH_GET_CLASS (paintbrush)->get_paint_params (paintbrush,
                                                                drawable,
                                                                paint_options,
                                                                sym,
                                                                &paint_mode,
                                                                &paint_appl_mode,
                                                                &paint_pixmap,
                                                                &paint_color);

      coords = gimp_symmetry_get_coords (sym, i);

      if (GIMP_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
        gimp_brush_core_eval_transform_symmetry (brush_core, sym, i);

      paint_buffer = gimp_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options,
                                                       paint_mode,
                                                       coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       &paint_width,
                                                       &paint_height);
      if (! paint_buffer)
        continue;

      if (! paint_pixmap)
        {
          opacity *= paint_color.a;
          gimp_rgb_set_alpha (&paint_color, GIMP_OPACITY_OPAQUE);
        }

      /* fill the paint buffer.  we can skip this step when reusing the
       * previous paint buffer, if the paint color/pixmap hasn't changed
       * (unless using an applicator, which currently modifies the paint buffer
       * in-place).
       */
      if (paint_core->applicator                   ||
          paint_buffer != paintbrush->paint_buffer ||
          paint_pixmap != paintbrush->paint_pixmap ||
          (! paint_pixmap && (gimp_rgba_distance (&paint_color,
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
              g_clear_pointer (&paintbrush->paint_pixmap, gimp_temp_buf_unref);

              if (paint_pixmap)
                paintbrush->paint_pixmap = gimp_temp_buf_ref (paint_pixmap);
            }

          paintbrush->paint_color = paint_color;

          if (paint_pixmap)
            {
              gimp_brush_core_color_area_with_pixmap (brush_core, drawable,
                                                      coords,
                                                      paint_buffer,
                                                      paint_buffer_x,
                                                      paint_buffer_y,
                                                      FALSE);
            }
          else
            {
              GeglColor *color;

              color = gimp_gegl_color_new (&paint_color,
                                           gimp_drawable_get_space (drawable));

              gegl_buffer_set_color (paint_buffer, NULL, color);

              g_object_unref (color);
            }
        }

      if (gimp_dynamics_is_output_enabled (dynamics, GIMP_DYNAMICS_OUTPUT_FORCE))
        force = gimp_dynamics_get_linear_value (dynamics,
                                                GIMP_DYNAMICS_OUTPUT_FORCE,
                                                coords,
                                                paint_options,
                                                fade_point);
      else
        force = paint_options->brush_force;

      /* finally, let the brush core paste the colored area on the canvas */
      gimp_brush_core_paste_canvas (brush_core, drawable,
                                    coords,
                                    MIN (opacity, GIMP_OPACITY_OPAQUE),
                                    gimp_context_get_opacity (context),
                                    paint_mode,
                                    gimp_paint_options_get_brush_mode (paint_options),
                                    force,
                                    paint_appl_mode);
    }
}
