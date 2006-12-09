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

#include "paint-types.h"

#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gimperaser.h"
#include "gimperaseroptions.h"

#include "gimp-intl.h"


static void   gimp_eraser_paint  (GimpPaintCore    *paint_core,
                                  GimpDrawable     *drawable,
                                  GimpPaintOptions *paint_options,
                                  GimpPaintState    paint_state,
                                  guint32           time);
static void   gimp_eraser_motion (GimpPaintCore    *paint_core,
                                  GimpDrawable     *drawable,
                                  GimpPaintOptions *paint_options);


G_DEFINE_TYPE (GimpEraser, gimp_eraser, GIMP_TYPE_BRUSH_CORE)


void
gimp_eraser_register (Gimp                      *gimp,
                      GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_ERASER,
                GIMP_TYPE_ERASER_OPTIONS,
                "gimp-eraser",
                _("Eraser"),
                "gimp-tool-eraser");
}

static void
gimp_eraser_class_init (GimpEraserClass *klass)
{
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  paint_core_class->paint = gimp_eraser_paint;

  brush_core_class->handles_changing_brush = TRUE;
}

static void
gimp_eraser_init (GimpEraser *eraser)
{
}

static void
gimp_eraser_paint (GimpPaintCore    *paint_core,
                   GimpDrawable     *drawable,
                   GimpPaintOptions *paint_options,
                   GimpPaintState    paint_state,
                   guint32           time)
{
  switch (paint_state)
    {
    case GIMP_PAINT_STATE_MOTION:
      gimp_eraser_motion (paint_core, drawable, paint_options);
      break;

    default:
      break;
    }
}

static void
gimp_eraser_motion (GimpPaintCore    *paint_core,
                    GimpDrawable     *drawable,
                    GimpPaintOptions *paint_options)
{
  GimpEraserOptions   *options          = GIMP_ERASER_OPTIONS (paint_options);
  GimpContext         *context          = GIMP_CONTEXT (paint_options);
  GimpPressureOptions *pressure_options = paint_options->pressure_options;
  GimpImage           *image;
  gdouble              opacity;
  TempBuf             *area;
  guchar               col[MAX_CHANNELS];

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  opacity = gimp_paint_options_get_fade (paint_options, image,
                                         paint_core->pixel_dist);
  if (opacity == 0.0)
    return;

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return;

  gimp_image_get_background (image, context, gimp_drawable_type (drawable),
                             col);

  /*  set the alpha channel  */
  col[area->bytes - 1] = OPAQUE_OPACITY;

  /*  color the pixels  */
  color_pixels (temp_buf_data (area), col,
                area->width * area->height, area->bytes);

  if (pressure_options->opacity)
    opacity *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  gimp_brush_core_paste_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                MIN (opacity, GIMP_OPACITY_OPAQUE),
                                gimp_context_get_opacity (context),
                                (options->anti_erase ?
                                 GIMP_ANTI_ERASE_MODE : GIMP_ERASE_MODE),
                                gimp_paint_options_get_brush_mode (paint_options),
                                paint_options->application_mode);
}
