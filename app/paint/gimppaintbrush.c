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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"

#include "gimppaintbrush.h"
#include "gimppaintoptions.h"

#include "gimp-intl.h"


static void   gimp_paintbrush_paint (GimpPaintCore    *paint_core,
                                     GimpDrawable     *drawable,
                                     GimpPaintOptions *paint_options,
                                     const GimpCoords *coords,
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
                       const GimpCoords *coords,
                       GimpPaintState    paint_state,
                       guint32           time)
{
  switch (paint_state)
    {
    case GIMP_PAINT_STATE_MOTION:
      _gimp_paintbrush_motion (paint_core, drawable, paint_options, coords,
                               GIMP_OPACITY_OPAQUE);
      break;

    default:
      break;
    }
}

void
_gimp_paintbrush_motion (GimpPaintCore    *paint_core,
                         GimpDrawable     *drawable,
                         GimpPaintOptions *paint_options,
                         const GimpCoords *coords,
                         gdouble           opacity)
{
  GimpBrushCore            *brush_core = GIMP_BRUSH_CORE (paint_core);
  GimpContext              *context    = GIMP_CONTEXT (paint_options);
  GimpDynamics             *dynamics   = brush_core->dynamics;
  GimpDynamicsOutput       *opacity_output;
  GimpDynamicsOutput       *color_output;
  GimpDynamicsOutput       *force_output;
  GimpImage                *image;
  GimpRGB                   gradient_color;
  TempBuf                  *area;
  guchar                    col[MAX_CHANNELS];
  GimpPaintApplicationMode  paint_appl_mode;
  gdouble                   fade_point;
  gdouble                   grad_point;
  gdouble                   force;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  opacity_output = gimp_dynamics_get_output (dynamics,
                                             GIMP_DYNAMICS_OUTPUT_OPACITY);

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  opacity *= gimp_dynamics_output_get_linear_value (opacity_output,
                                                    coords,
                                                    paint_options,
                                                    fade_point);
  if (opacity == 0.0)
    return;

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options,
                                         coords);
  if (! area)
    return;

  paint_appl_mode = paint_options->application_mode;

  color_output = gimp_dynamics_get_output (dynamics,
                                           GIMP_DYNAMICS_OUTPUT_COLOR);

  grad_point = gimp_dynamics_output_get_linear_value (color_output,
                                                      coords,
                                                      paint_options,
                                                      fade_point);

  /* optionally take the color from the current gradient */
  if (gimp_paint_options_get_gradient_color (paint_options, image,
                                             grad_point,
                                             paint_core->pixel_dist,
                                             &gradient_color))
    {
      guchar pixel[MAX_CHANNELS] = { OPAQUE_OPACITY,
                                     OPAQUE_OPACITY,
                                     OPAQUE_OPACITY,
                                     OPAQUE_OPACITY };

      opacity *= gradient_color.a;

      gimp_rgb_get_uchar (&gradient_color,
                          &col[RED],
                          &col[GREEN],
                          &col[BLUE]);

      gimp_image_transform_color (image, gimp_drawable_type (drawable), pixel,
                                  GIMP_RGB, col);

      color_pixels (temp_buf_get_data (area), pixel,
                    area->width * area->height,
                    area->bytes);

      paint_appl_mode = GIMP_PAINT_INCREMENTAL;
    }
  /* otherwise check if the brush has a pixmap and use that to color the area */
  else if (brush_core->brush && brush_core->brush->pixmap)
    {
      gimp_brush_core_color_area_with_pixmap (brush_core, drawable,
                                              coords,
                                              area,
                                              gimp_paint_options_get_brush_mode (paint_options));

      paint_appl_mode = GIMP_PAINT_INCREMENTAL;
    }
  /* otherwise fill the area with the foreground color */
  else
    {
      gimp_image_get_foreground (image, context, gimp_drawable_type (drawable),
                                 col);

      col[area->bytes - 1] = OPAQUE_OPACITY;

      color_pixels (temp_buf_get_data (area), col,
                    area->width * area->height,
                    area->bytes);
    }

  force_output = gimp_dynamics_get_output (dynamics,
                                           GIMP_DYNAMICS_OUTPUT_FORCE);

  force = gimp_dynamics_output_get_linear_value (force_output,
                                                 coords,
                                                 paint_options,
                                                 fade_point);

  /* finally, let the brush core paste the colored area on the canvas */
  gimp_brush_core_paste_canvas (brush_core, drawable,
                                coords,
                                MIN (opacity, GIMP_OPACITY_OPAQUE),
                                gimp_context_get_opacity (context),
                                gimp_context_get_paint_mode (context),
                                gimp_paint_options_get_brush_mode (paint_options),
                                force,
                                paint_appl_mode);
}
