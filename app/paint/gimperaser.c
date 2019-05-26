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

#include "paint-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimp-palettes.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimpsymmetry.h"

#include "gimperaser.h"
#include "gimperaseroptions.h"

#include "gimp-intl.h"


static void   gimp_eraser_paint            (GimpPaintCore             *paint_core,
                                            GimpDrawable              *drawable,
                                            GimpPaintOptions          *paint_options,
                                            GimpSymmetry              *sym,
                                            GimpPaintState             paint_state,
                                            guint32                    time);

static void   gimp_eraser_get_paint_params (GimpPaintbrush            *paintbrush,
                                            GimpDrawable              *drawable,
                                            GimpPaintOptions          *paint_options,
                                            GimpSymmetry              *sym,
                                            GimpLayerMode             *paint_mode,
                                            GimpPaintApplicationMode  *paint_appl_mode,
                                            const GimpTempBuf        **paint_pixmap,
                                            GimpRGB                   *paint_color);


G_DEFINE_TYPE (GimpEraser, gimp_eraser, GIMP_TYPE_PAINTBRUSH)

#define parent_class gimp_eraser_parent_class


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
  GimpPaintCoreClass  *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpPaintbrushClass *paintbrush_class = GIMP_PAINTBRUSH_CLASS (klass);

  paint_core_class->paint            = gimp_eraser_paint;

  paintbrush_class->get_paint_params = gimp_eraser_get_paint_params;
}

static void
gimp_eraser_init (GimpEraser *eraser)
{
}

static void
gimp_eraser_paint (GimpPaintCore    *paint_core,
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
          if (! gimp_drawable_has_alpha (drawable))
            {
              /* Erasing on a drawable without alpha is equivalent to
               * drawing with background color. So let's save history.
               */
              GimpContext *context = GIMP_CONTEXT (paint_options);
              GimpRGB      background;

              gimp_context_get_background (context, &background);
              gimp_palettes_add_color_history (context->gimp,
                                               &background);

            }
        }
      break;

    default:
      break;
    }

  GIMP_PAINT_CORE_CLASS (parent_class)->paint (paint_core, drawable,
                                               paint_options, sym,
                                               paint_state, time);
}

static void
gimp_eraser_get_paint_params (GimpPaintbrush            *paintbrush,
                              GimpDrawable              *drawable,
                              GimpPaintOptions          *paint_options,
                              GimpSymmetry              *sym,
                              GimpLayerMode             *paint_mode,
                              GimpPaintApplicationMode  *paint_appl_mode,
                              const GimpTempBuf        **paint_pixmap,
                              GimpRGB                   *paint_color)
{
  GimpEraserOptions *options = GIMP_ERASER_OPTIONS (paint_options);
  GimpContext       *context = GIMP_CONTEXT (paint_options);

  gimp_context_get_background (context, paint_color);
  gimp_pickable_srgb_to_image_color (GIMP_PICKABLE (drawable),
                                     paint_color, paint_color);

  if (options->anti_erase)
    *paint_mode = GIMP_LAYER_MODE_ANTI_ERASE;
  else if (gimp_drawable_has_alpha (drawable))
    *paint_mode = GIMP_LAYER_MODE_ERASE;
  else
    *paint_mode = GIMP_LAYER_MODE_NORMAL_LEGACY;
}
