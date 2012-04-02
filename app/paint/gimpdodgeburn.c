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

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimpimage.h"

#include "gimpdodgeburn.h"
#include "gimpdodgeburnoptions.h"

#include "gimp-intl.h"


static void   gimp_dodge_burn_paint  (GimpPaintCore    *paint_core,
                                      GimpDrawable     *drawable,
                                      GimpPaintOptions *paint_options,
                                      const GimpCoords *coords,
                                      GimpPaintState    paint_state,
                                      guint32           time);
static void   gimp_dodge_burn_motion (GimpPaintCore    *paint_core,
                                      GimpDrawable     *drawable,
                                      GimpPaintOptions *paint_options,
                                      const GimpCoords *coords);


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
                       GimpDrawable     *drawable,
                       GimpPaintOptions *paint_options,
                       const GimpCoords *coords,
                       GimpPaintState    paint_state,
                       guint32           time)
{
  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      break;

    case GIMP_PAINT_STATE_MOTION:
      gimp_dodge_burn_motion (paint_core, drawable, paint_options, coords);
      break;

    case GIMP_PAINT_STATE_FINISH:
      break;
    }
}

static void
gimp_dodge_burn_motion (GimpPaintCore    *paint_core,
                        GimpDrawable     *drawable,
                        GimpPaintOptions *paint_options,
                        const GimpCoords *coords)
{
  GimpDodgeBurnOptions *options   = GIMP_DODGE_BURN_OPTIONS (paint_options);
  GimpContext          *context   = GIMP_CONTEXT (paint_options);
  GimpDynamics         *dynamics  = GIMP_BRUSH_CORE (paint_core)->dynamics;
  GimpDynamicsOutput   *opacity_output;
  GimpDynamicsOutput   *hardness_output;
  GimpImage            *image;
  GeglBuffer           *paint_buffer;
  gint                  paint_buffer_x;
  gint                  paint_buffer_y;
  gdouble               fade_point;
  gdouble               opacity;
  gdouble               hardness;

  if (gimp_drawable_is_indexed (drawable))
    return;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  opacity_output = gimp_dynamics_get_output (dynamics,
                                             GIMP_DYNAMICS_OUTPUT_OPACITY);

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  opacity = gimp_dynamics_output_get_linear_value (opacity_output,
                                                   coords,
                                                   paint_options,
                                                   fade_point);
  if (opacity == 0.0)
    return;

  paint_buffer = gimp_paint_core_get_paint_buffer (paint_core, drawable,
                                                   paint_options, coords,
                                                   &paint_buffer_x,
                                                   &paint_buffer_y);
  if (! paint_buffer)
    return;

  /*  DodgeBurn the region  */
  gimp_gegl_dodgeburn (gimp_paint_core_get_orig_image (paint_core),
                       GEGL_RECTANGLE (paint_buffer_x,
                                       paint_buffer_y,
                                       gegl_buffer_get_width  (paint_buffer),
                                       gegl_buffer_get_height (paint_buffer)),
                       paint_buffer,
                       GEGL_RECTANGLE (0, 0, 0, 0),
                       options->exposure / 100.0,
                       options->type,
                       options->mode);

  hardness_output = gimp_dynamics_get_output (dynamics,
                                              GIMP_DYNAMICS_OUTPUT_HARDNESS);

  hardness = gimp_dynamics_output_get_linear_value (hardness_output,
                                                    coords,
                                                    paint_options,
                                                    fade_point);

  /* Replace the newly dodgedburned area (paint_area) to the image */
  gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                  coords,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  hardness,
                                  GIMP_PAINT_CONSTANT);
}
