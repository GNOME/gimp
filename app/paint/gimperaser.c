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

#include "gegl/ligma-gegl-utils.h"

#include "core/ligma.h"
#include "core/ligma-palettes.h"
#include "core/ligmadrawable.h"
#include "core/ligmadynamics.h"
#include "core/ligmaimage.h"
#include "core/ligmapickable.h"
#include "core/ligmasymmetry.h"

#include "ligmaeraser.h"
#include "ligmaeraseroptions.h"

#include "ligma-intl.h"


static gboolean   ligma_eraser_get_color_history_color (LigmaPaintbrush            *paintbrush,
                                                       LigmaDrawable              *drawable,
                                                       LigmaPaintOptions          *paint_options,
                                                       LigmaRGB                   *color);
static void       ligma_eraser_get_paint_params        (LigmaPaintbrush            *paintbrush,
                                                       LigmaDrawable              *drawable,
                                                       LigmaPaintOptions          *paint_options,
                                                       LigmaSymmetry              *sym,
                                                       gdouble                    grad_point,
                                                       LigmaLayerMode             *paint_mode,
                                                       LigmaPaintApplicationMode  *paint_appl_mode,
                                                       const LigmaTempBuf        **paint_pixmap,
                                                       LigmaRGB                   *paint_color);


G_DEFINE_TYPE (LigmaEraser, ligma_eraser, LIGMA_TYPE_PAINTBRUSH)


void
ligma_eraser_register (Ligma                      *ligma,
                      LigmaPaintRegisterCallback  callback)
{
  (* callback) (ligma,
                LIGMA_TYPE_ERASER,
                LIGMA_TYPE_ERASER_OPTIONS,
                "ligma-eraser",
                _("Eraser"),
                "ligma-tool-eraser");
}

static void
ligma_eraser_class_init (LigmaEraserClass *klass)
{
  LigmaPaintbrushClass *paintbrush_class = LIGMA_PAINTBRUSH_CLASS (klass);

  paintbrush_class->get_color_history_color = ligma_eraser_get_color_history_color;
  paintbrush_class->get_paint_params        = ligma_eraser_get_paint_params;
}

static void
ligma_eraser_init (LigmaEraser *eraser)
{
}

static gboolean
ligma_eraser_get_color_history_color (LigmaPaintbrush   *paintbrush,
                                     LigmaDrawable     *drawable,
                                     LigmaPaintOptions *paint_options,
                                     LigmaRGB          *color)
{
  /* Erasing on a drawable without alpha is equivalent to
   * drawing with background color. So let's save history.
   */
  if (! ligma_drawable_has_alpha (drawable))
    {
      LigmaContext *context = LIGMA_CONTEXT (paint_options);

      ligma_context_get_background (context, color);

      return TRUE;
    }

  return FALSE;
}

static void
ligma_eraser_get_paint_params (LigmaPaintbrush            *paintbrush,
                              LigmaDrawable              *drawable,
                              LigmaPaintOptions          *paint_options,
                              LigmaSymmetry              *sym,
                              gdouble                    grad_point,
                              LigmaLayerMode             *paint_mode,
                              LigmaPaintApplicationMode  *paint_appl_mode,
                              const LigmaTempBuf        **paint_pixmap,
                              LigmaRGB                   *paint_color)
{
  LigmaEraserOptions *options = LIGMA_ERASER_OPTIONS (paint_options);
  LigmaContext       *context = LIGMA_CONTEXT (paint_options);

  ligma_context_get_background (context, paint_color);
  ligma_pickable_srgb_to_image_color (LIGMA_PICKABLE (drawable),
                                     paint_color, paint_color);

  if (options->anti_erase)
    *paint_mode = LIGMA_LAYER_MODE_ANTI_ERASE;
  else if (ligma_drawable_has_alpha (drawable))
    *paint_mode = LIGMA_LAYER_MODE_ERASE;
  else
    *paint_mode = LIGMA_LAYER_MODE_NORMAL_LEGACY;
}
