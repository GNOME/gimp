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
#include "core/gimpsymmetry.h"
#include "core/gimptempbuf.h"

#include "gimppaintbrush.h"
#include "gimppaintoptions.h"

#include "gimp-intl.h"


static void   gimp_paintbrush_paint (GimpPaintCore    *paint_core,
                                     GimpDrawable     *drawable,
                                     GimpPaintOptions *paint_options,
                                     GimpSymmetry     *sym,
                                     GimpPaintState    paint_state,
                                     guint32           time);


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
  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
        {
          GimpContext   *context = GIMP_CONTEXT (paint_options);
          GimpBrushCore *brush_core = GIMP_BRUSH_CORE (paint_core);
          GimpDynamics  *dynamics;

          dynamics = gimp_context_get_dynamics (GIMP_CONTEXT (paint_options));

          if (! gimp_dynamics_is_output_enabled (dynamics, GIMP_DYNAMICS_OUTPUT_COLOR) &&
              (! brush_core->brush || ! gimp_brush_get_pixmap (brush_core->brush)))
            {
              /* We don't save gradient color history and
               * pixmap brushes have no color to save.
               */
              GimpRGB foreground;

              gimp_context_get_foreground (context, &foreground);
              gimp_palettes_add_color_history (context->gimp,
                                               &foreground);
            }
        }
      break;
    case GIMP_PAINT_STATE_MOTION:
      _gimp_paintbrush_motion (paint_core, drawable, paint_options,
                               sym, GIMP_OPACITY_OPAQUE);
      break;

    default:
      break;
    }
}

void
_gimp_paintbrush_motion (GimpPaintCore    *paint_core,
                         GimpDrawable     *drawable,
                         GimpPaintOptions *paint_options,
                         GimpSymmetry     *sym,
                         gdouble           opacity)
{
  GimpBrushCore            *brush_core = GIMP_BRUSH_CORE (paint_core);
  GimpContext              *context    = GIMP_CONTEXT (paint_options);
  GimpDynamics             *dynamics   = brush_core->dynamics;
  GimpImage                *image;
  GimpRGB                   gradient_color;
  GeglBuffer               *paint_buffer;
  gint                      paint_buffer_x;
  gint                      paint_buffer_y;
  GimpPaintApplicationMode  paint_appl_mode;
  gdouble                   fade_point;
  gdouble                   grad_point;
  gdouble                   force;
  const GimpCoords         *coords;
  GeglNode                 *op;
  gint                      n_strokes;
  gint                      i;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

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

  paint_appl_mode = paint_options->application_mode;

  grad_point = gimp_dynamics_get_linear_value (dynamics,
                                               GIMP_DYNAMICS_OUTPUT_COLOR,
                                               coords,
                                               paint_options,
                                               fade_point);


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
      gint paint_width, paint_height;

      coords = gimp_symmetry_get_coords (sym, i);

      paint_buffer = gimp_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options, coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       &paint_width,
                                                       &paint_height);
      if (! paint_buffer)
        continue;

      op = gimp_symmetry_get_operation (sym, i,
                                        paint_width,
                                        paint_height);
      if (gimp_paint_options_get_gradient_color (paint_options, image,
                                                 grad_point,
                                                 paint_core->pixel_dist,
                                                 &gradient_color))
        {
          /* optionally take the color from the current gradient */

          GeglColor *color;

          opacity *= gradient_color.a;
          gimp_rgb_set_alpha (&gradient_color, GIMP_OPACITY_OPAQUE);

          color = gimp_gegl_color_new (&gradient_color);

          gegl_buffer_set_color (paint_buffer, NULL, color);
          g_object_unref (color);

          paint_appl_mode = GIMP_PAINT_INCREMENTAL;
        }
      else if (brush_core->brush && gimp_brush_get_pixmap (brush_core->brush))
        {
          /* otherwise check if the brush has a pixmap and use that to
           * color the area
           */
          gimp_brush_core_color_area_with_pixmap (brush_core, drawable,
                                                  coords, op,
                                                  paint_buffer,
                                                  paint_buffer_x,
                                                  paint_buffer_y,
                                                  gimp_paint_options_get_brush_mode (paint_options));

          paint_appl_mode = GIMP_PAINT_INCREMENTAL;
        }
      else
        {
          /* otherwise fill the area with the foreground color */

          GimpRGB    foreground;
          GeglColor *color;

          gimp_context_get_foreground (context, &foreground);
          color = gimp_gegl_color_new (&foreground);

          gegl_buffer_set_color (paint_buffer, NULL, color);
          g_object_unref (color);
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
                                    gimp_context_get_paint_mode (context),
                                    gimp_paint_options_get_brush_mode (paint_options),
                                    force,
                                    paint_appl_mode, op);
    }
}
