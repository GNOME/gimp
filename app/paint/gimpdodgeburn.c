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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "gegl/gimp-gegl-loops.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpimage.h"
#include "core/gimpsymmetry.h"

#include "gimpdodgeburn.h"
#include "gimpdodgeburnoptions.h"

#include "gimp-intl.h"


static void   gimp_dodge_burn_paint  (GimpPaintCore    *paint_core,
                                      GList            *drawables,
                                      GimpPaintOptions *paint_options,
                                      GimpSymmetry     *sym,
                                      GimpPaintState    paint_state,
                                      guint32           time);
static void   gimp_dodge_burn_motion (GimpPaintCore    *paint_core,
                                      GimpDrawable     *drawable,
                                      GimpPaintOptions *paint_options,
                                      GimpSymmetry     *sym);


G_DEFINE_TYPE (GimpDodgeBurn, gimp_dodge_burn, GIMP_TYPE_BRUSH_CORE)

#define parent_class gimp_dodge_burn_parent_class


void
gimp_dodge_burn_register (Gimp                      *gimp,
                          GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_DODGE_BURN,
                GIMP_TYPE_DODGE_BURN_OPTIONS,
                "gimp-dodge-burn",
                _("Dodge/Burn"),
                "gimp-tool-dodge");
}

static void
gimp_dodge_burn_class_init (GimpDodgeBurnClass *klass)
{
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  paint_core_class->paint = gimp_dodge_burn_paint;

  brush_core_class->handles_changing_brush = TRUE;
}

static void
gimp_dodge_burn_init (GimpDodgeBurn *dodgeburn)
{
}

static void
gimp_dodge_burn_paint (GimpPaintCore    *paint_core,
                       GList            *drawables,
                       GimpPaintOptions *paint_options,
                       GimpSymmetry     *sym,
                       GimpPaintState    paint_state,
                       guint32           time)
{
  g_return_if_fail (g_list_length (drawables) == 1);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      break;

    case GIMP_PAINT_STATE_MOTION:
      for (GList *iter = drawables; iter; iter = iter->next)
        gimp_dodge_burn_motion (paint_core, iter->data, paint_options, sym);
      break;

    case GIMP_PAINT_STATE_FINISH:
      break;
    }
}

static void
gimp_dodge_burn_motion (GimpPaintCore    *paint_core,
                        GimpDrawable     *drawable,
                        GimpPaintOptions *paint_options,
                        GimpSymmetry     *sym)
{
  GimpBrushCore        *brush_core = GIMP_BRUSH_CORE (paint_core);
  GimpDodgeBurnOptions *options    = GIMP_DODGE_BURN_OPTIONS (paint_options);
  GimpContext          *context    = GIMP_CONTEXT (paint_options);
  GimpDynamics         *dynamics   = GIMP_BRUSH_CORE (paint_core)->dynamics;
  GimpImage            *image      = gimp_item_get_image (GIMP_ITEM (drawable));
  GeglBuffer           *src_buffer;
  GeglBuffer           *paint_buffer;
  gint                  paint_buffer_x;
  gint                  paint_buffer_y;
  gdouble               fade_point;
  gdouble               opacity;
  gdouble               force;
  GimpCoords            coords;
  gint                  off_x, off_y;
  gint                  paint_width, paint_height;
  gint                  n_strokes;
  gint                  i;

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
  coords = *(gimp_symmetry_get_origin (sym));
  coords.x -= off_x;
  coords.y -= off_y;
  gimp_symmetry_set_origin (sym, drawable, &coords);

  opacity = gimp_dynamics_get_linear_value (dynamics,
                                            GIMP_DYNAMICS_OUTPUT_OPACITY,
                                            &coords,
                                            paint_options,
                                            fade_point);
  if (opacity == 0.0)
    return;

  if (paint_options->application_mode == GIMP_PAINT_CONSTANT)
    src_buffer = gimp_paint_core_get_orig_image (paint_core, drawable);
  else
    src_buffer = gimp_drawable_get_buffer (drawable);

  gimp_brush_core_eval_transform_dynamics (brush_core,
                                           image,
                                           paint_options,
                                           &coords);
  n_strokes = gimp_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      coords = *(gimp_symmetry_get_coords (sym, i));

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

      /*  DodgeBurn the region  */
      gimp_gegl_dodgeburn (src_buffer,
                           GEGL_RECTANGLE (paint_buffer_x,
                                           paint_buffer_y,
                                           gegl_buffer_get_width  (paint_buffer),
                                           gegl_buffer_get_height (paint_buffer)),
                           paint_buffer,
                           GEGL_RECTANGLE (0, 0, 0, 0),
                           options->exposure / 100.0,
                           options->type,
                           options->mode);

      if (gimp_dynamics_is_output_enabled (dynamics, GIMP_DYNAMICS_OUTPUT_FORCE))
        force = gimp_dynamics_get_linear_value (dynamics,
                                                GIMP_DYNAMICS_OUTPUT_FORCE,
                                                &coords,
                                                paint_options,
                                                fade_point);
      else
        force = paint_options->brush_force;

      /* Replace the newly dodgedburned area (paint_area) to the image */
      gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                      &coords,
                                      MIN (opacity, GIMP_OPACITY_OPAQUE),
                                      gimp_context_get_opacity (context),
                                      gimp_paint_options_get_brush_mode (paint_options),
                                      force,
                                      paint_options->application_mode);
    }
}
