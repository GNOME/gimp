/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"

#include "gimppaintbrush.h"
#include "gimppaintoptions.h"

#include "gimp-intl.h"


static void   gimp_paintbrush_paint (GimpPaintCore    *paint_core,
                                     GimpDrawable     *drawable,
                                     GimpPaintOptions *paint_options,
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
                       GimpPaintState    paint_state,
                       guint32           time)
{
  switch (paint_state)
    {
    case GIMP_PAINT_STATE_MOTION:
      _gimp_paintbrush_motion (paint_core, drawable, paint_options,
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
                         gdouble           opacity)
{
  GimpBrushCore            *brush_core       = GIMP_BRUSH_CORE (paint_core);
  GimpContext              *context          = GIMP_CONTEXT (paint_options);
  GimpPressureOptions      *pressure_options = paint_options->pressure_options;
  GimpImage                *image;
  GimpRGB                   gradient_color;
  TempBuf                  *area;
  guchar                    col[MAX_CHANNELS];
  GimpPaintApplicationMode  paint_appl_mode;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  opacity *= gimp_paint_options_get_fade (paint_options, image,
                                          paint_core->pixel_dist);
  if (opacity == 0.0)
    return;

  paint_appl_mode = paint_options->application_mode;

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return;

  /* optionally take the color from the current gradient */
  if (gimp_paint_options_get_gradient_color (paint_options, image,
                                             paint_core->cur_coords.pressure,
                                             paint_core->pixel_dist,
                                             &gradient_color))
    {
      opacity *= gradient_color.a;

      gimp_rgb_get_uchar (&gradient_color,
                          &col[RED_PIX],
                          &col[GREEN_PIX],
                          &col[BLUE_PIX]);
      col[ALPHA_PIX] = OPAQUE_OPACITY;

      color_pixels (temp_buf_data (area), col,
                    area->width * area->height,
                    area->bytes);

      paint_appl_mode = GIMP_PAINT_INCREMENTAL;
    }
  /* otherwise check if the brush has a pixmap and use that to color the area */
  else if (brush_core->brush && brush_core->brush->pixmap)
    {
      gimp_brush_core_color_area_with_pixmap (brush_core, drawable,
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

      color_pixels (temp_buf_data (area), col,
                    area->width * area->height,
                    area->bytes);
    }

  if (paint_core->use_pressure && pressure_options->opacity)
    opacity *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  /* finally, let the brush core paste the colored area on the canvas */
  gimp_brush_core_paste_canvas (brush_core, drawable,
                                MIN (opacity, GIMP_OPACITY_OPAQUE),
                                gimp_context_get_opacity (context),
                                gimp_context_get_paint_mode (context),
                                gimp_paint_options_get_brush_mode (paint_options),
                                paint_appl_mode);
}
