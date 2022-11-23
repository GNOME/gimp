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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "paint-types.h"

#include "gegl/ligma-gegl-loops.h"

#include "core/ligma.h"
#include "core/ligmadrawable.h"
#include "core/ligmadynamics.h"
#include "core/ligmaimage.h"
#include "core/ligmasymmetry.h"

#include "ligmadodgeburn.h"
#include "ligmadodgeburnoptions.h"

#include "ligma-intl.h"


static void   ligma_dodge_burn_paint  (LigmaPaintCore    *paint_core,
                                      GList            *drawables,
                                      LigmaPaintOptions *paint_options,
                                      LigmaSymmetry     *sym,
                                      LigmaPaintState    paint_state,
                                      guint32           time);
static void   ligma_dodge_burn_motion (LigmaPaintCore    *paint_core,
                                      LigmaDrawable     *drawable,
                                      LigmaPaintOptions *paint_options,
                                      LigmaSymmetry     *sym);


G_DEFINE_TYPE (LigmaDodgeBurn, ligma_dodge_burn, LIGMA_TYPE_BRUSH_CORE)

#define parent_class ligma_dodge_burn_parent_class


void
ligma_dodge_burn_register (Ligma                      *ligma,
                          LigmaPaintRegisterCallback  callback)
{
  (* callback) (ligma,
                LIGMA_TYPE_DODGE_BURN,
                LIGMA_TYPE_DODGE_BURN_OPTIONS,
                "ligma-dodge-burn",
                _("Dodge/Burn"),
                "ligma-tool-dodge");
}

static void
ligma_dodge_burn_class_init (LigmaDodgeBurnClass *klass)
{
  LigmaPaintCoreClass *paint_core_class = LIGMA_PAINT_CORE_CLASS (klass);
  LigmaBrushCoreClass *brush_core_class = LIGMA_BRUSH_CORE_CLASS (klass);

  paint_core_class->paint = ligma_dodge_burn_paint;

  brush_core_class->handles_changing_brush = TRUE;
}

static void
ligma_dodge_burn_init (LigmaDodgeBurn *dodgeburn)
{
}

static void
ligma_dodge_burn_paint (LigmaPaintCore    *paint_core,
                       GList            *drawables,
                       LigmaPaintOptions *paint_options,
                       LigmaSymmetry     *sym,
                       LigmaPaintState    paint_state,
                       guint32           time)
{
  g_return_if_fail (g_list_length (drawables) == 1);

  switch (paint_state)
    {
    case LIGMA_PAINT_STATE_INIT:
      break;

    case LIGMA_PAINT_STATE_MOTION:
      for (GList *iter = drawables; iter; iter = iter->next)
        ligma_dodge_burn_motion (paint_core, iter->data, paint_options, sym);
      break;

    case LIGMA_PAINT_STATE_FINISH:
      break;
    }
}

static void
ligma_dodge_burn_motion (LigmaPaintCore    *paint_core,
                        LigmaDrawable     *drawable,
                        LigmaPaintOptions *paint_options,
                        LigmaSymmetry     *sym)
{
  LigmaBrushCore        *brush_core = LIGMA_BRUSH_CORE (paint_core);
  LigmaDodgeBurnOptions *options    = LIGMA_DODGE_BURN_OPTIONS (paint_options);
  LigmaContext          *context    = LIGMA_CONTEXT (paint_options);
  LigmaDynamics         *dynamics   = LIGMA_BRUSH_CORE (paint_core)->dynamics;
  LigmaImage            *image      = ligma_item_get_image (LIGMA_ITEM (drawable));
  GeglBuffer           *src_buffer;
  GeglBuffer           *paint_buffer;
  gint                  paint_buffer_x;
  gint                  paint_buffer_y;
  gdouble               fade_point;
  gdouble               opacity;
  gdouble               force;
  LigmaCoords            coords;
  gint                  off_x, off_y;
  gint                  paint_width, paint_height;
  gint                  n_strokes;
  gint                  i;

  fade_point = ligma_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);
  coords = *(ligma_symmetry_get_origin (sym));
  coords.x -= off_x;
  coords.y -= off_y;

  opacity = ligma_dynamics_get_linear_value (dynamics,
                                            LIGMA_DYNAMICS_OUTPUT_OPACITY,
                                            &coords,
                                            paint_options,
                                            fade_point);
  if (opacity == 0.0)
    return;

  if (paint_options->application_mode == LIGMA_PAINT_CONSTANT)
    src_buffer = ligma_paint_core_get_orig_image (paint_core, drawable);
  else
    src_buffer = ligma_drawable_get_buffer (drawable);

  ligma_brush_core_eval_transform_dynamics (brush_core,
                                           image,
                                           paint_options,
                                           &coords);
  n_strokes = ligma_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      coords = *(ligma_symmetry_get_coords (sym, i));
      coords.x -= off_x;
      coords.y -= off_y;

      ligma_brush_core_eval_transform_symmetry (brush_core, sym, i);

      paint_buffer = ligma_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options,
                                                       LIGMA_LAYER_MODE_NORMAL,
                                                       &coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       &paint_width,
                                                       &paint_height);
      if (! paint_buffer)
        continue;

      /*  DodgeBurn the region  */
      ligma_gegl_dodgeburn (src_buffer,
                           GEGL_RECTANGLE (paint_buffer_x,
                                           paint_buffer_y,
                                           gegl_buffer_get_width  (paint_buffer),
                                           gegl_buffer_get_height (paint_buffer)),
                           paint_buffer,
                           GEGL_RECTANGLE (0, 0, 0, 0),
                           options->exposure / 100.0,
                           options->type,
                           options->mode);

      if (ligma_dynamics_is_output_enabled (dynamics, LIGMA_DYNAMICS_OUTPUT_FORCE))
        force = ligma_dynamics_get_linear_value (dynamics,
                                                LIGMA_DYNAMICS_OUTPUT_FORCE,
                                                &coords,
                                                paint_options,
                                                fade_point);
      else
        force = paint_options->brush_force;

      /* Replace the newly dodgedburned area (paint_area) to the image */
      ligma_brush_core_replace_canvas (LIGMA_BRUSH_CORE (paint_core), drawable,
                                      &coords,
                                      MIN (opacity, LIGMA_OPACITY_OPAQUE),
                                      ligma_context_get_opacity (context),
                                      ligma_paint_options_get_brush_mode (paint_options),
                                      force,
                                      paint_options->application_mode);
    }
}
