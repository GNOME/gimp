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

#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"

#include "gimpsmudge.h"
#include "gimpsmudgeoptions.h"

#include "gimp-intl.h"


static void       gimp_smudge_finalize     (GObject          *object);

static void       gimp_smudge_paint        (GimpPaintCore    *paint_core,
                                            GimpDrawable     *drawable,
                                            GimpPaintOptions *paint_options,
                                            const GimpCoords *coords,
                                            GimpPaintState    paint_state,
                                            guint32           time);
static gboolean   gimp_smudge_start        (GimpPaintCore    *paint_core,
                                            GimpDrawable     *drawable,
                                            GimpPaintOptions *paint_options,
                                            const GimpCoords *coords);
static void       gimp_smudge_motion       (GimpPaintCore    *paint_core,
                                            GimpDrawable     *drawable,
                                            GimpPaintOptions *paint_options,
                                            const GimpCoords *coords);

static void       gimp_smudge_accumulator_coords (GimpPaintCore    *paint_core,
                                                  const GimpCoords *coords,
                                                  gint             *x,
                                                  gint             *y);

static void       gimp_smudge_accumulator_size   (GimpPaintOptions *paint_options,
                                                  gint             *accumulator_size);


G_DEFINE_TYPE (GimpSmudge, gimp_smudge, GIMP_TYPE_BRUSH_CORE)

#define parent_class gimp_smudge_parent_class


void
gimp_smudge_register (Gimp                      *gimp,
                      GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_SMUDGE,
                GIMP_TYPE_SMUDGE_OPTIONS,
                "gimp-smudge",
                _("Smudge"),
                "gimp-tool-smudge");
}

static void
gimp_smudge_class_init (GimpSmudgeClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  object_class->finalize  = gimp_smudge_finalize;

  paint_core_class->paint = gimp_smudge_paint;

  brush_core_class->handles_changing_brush = TRUE;
  brush_core_class->handles_transforming_brush = TRUE;
  brush_core_class->handles_dynamic_transforming_brush = TRUE;
}

static void
gimp_smudge_init (GimpSmudge *smudge)
{
  smudge->initialized = FALSE;
  smudge->accum_data  = NULL;
  smudge->accum_size  = 0;
}

static void
gimp_smudge_finalize (GObject *object)
{
  GimpSmudge *smudge = GIMP_SMUDGE (object);

  if (smudge->accum_data)
    {
      g_free (smudge->accum_data);
      smudge->accum_data = NULL;
    }

  smudge->accum_size = 0;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_smudge_paint (GimpPaintCore    *paint_core,
                   GimpDrawable     *drawable,
                   GimpPaintOptions *paint_options,
                   const GimpCoords *coords,
                   GimpPaintState    paint_state,
                   guint32           time)
{
  GimpSmudge *smudge = GIMP_SMUDGE (paint_core);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_MOTION:
      /* initialization fails if the user starts outside the drawable */
      if (! smudge->initialized)
        smudge->initialized = gimp_smudge_start (paint_core, drawable,
                                                 paint_options, coords);

      if (smudge->initialized)
        gimp_smudge_motion (paint_core, drawable, paint_options, coords);
      break;

    case GIMP_PAINT_STATE_FINISH:
      if (smudge->accum_data)
        {
          g_free (smudge->accum_data);
          smudge->accum_data = NULL;
        }
      smudge->initialized = FALSE;
      break;

    default:
      break;
    }
}

static gboolean
gimp_smudge_start (GimpPaintCore    *paint_core,
                   GimpDrawable     *drawable,
                   GimpPaintOptions *paint_options,
                   const GimpCoords *coords)
{
  GimpSmudge  *smudge = GIMP_SMUDGE (paint_core);
  TempBuf     *area;
  PixelRegion  srcPR;
  gint         bytes;
  gint         x, y;

  if (gimp_drawable_is_indexed (drawable))
    return FALSE;

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options,
                                         coords);
  if (! area)
    return FALSE;

  gimp_smudge_accumulator_size(paint_options, &smudge->accum_size);

  /*  adjust the x and y coordinates to the upper left corner of the
   *  accumulator
   */
  gimp_smudge_accumulator_coords (paint_core, coords, &x, &y);

  /*  Allocate the accumulation buffer */
  bytes = gimp_drawable_bytes (drawable);
  smudge->accum_data = g_malloc (SQR (smudge->accum_size) * bytes);

  /*  If clipped, prefill the smudge buffer with the color at the
   *  brush position.
   */
  if (x != area->x ||
      y != area->y ||
      smudge->accum_size != area->width ||
      smudge->accum_size != area->height)
    {
      guchar fill[4];

      gimp_pickable_get_pixel_at (GIMP_PICKABLE (drawable),
                                  CLAMP ((gint) coords->x,
                                         0,
                                         gimp_item_get_width (GIMP_ITEM (drawable)) - 1),
                                  CLAMP ((gint) coords->y,
                                         0,
                                         gimp_item_get_height (GIMP_ITEM (drawable)) - 1),
                                  fill);

      pixel_region_init_data (&srcPR, smudge->accum_data,
                              bytes, bytes * smudge->accum_size,
                              0, 0, smudge->accum_size, smudge->accum_size);

      color_region (&srcPR, fill);
    }

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     area->x, area->y, area->width, area->height, FALSE);

  pixel_region_init_data (&smudge->accumPR, smudge->accum_data,
                          bytes, bytes * smudge->accum_size,
                          area->x - x,
                          area->y - y,
                          area->width,
                          area->height);

  /* copy the region under the original painthit. */
  copy_region (&srcPR, &smudge->accumPR);

  pixel_region_init_data (&smudge->accumPR, smudge->accum_data,
                          bytes, bytes * smudge->accum_size,
                          0,
                          0,
                          smudge->accum_size,
                          smudge->accum_size);

  return TRUE;
}

static void
gimp_smudge_motion (GimpPaintCore    *paint_core,
                    GimpDrawable     *drawable,
                    GimpPaintOptions *paint_options,
                    const GimpCoords *coords)
{
  GimpSmudge         *smudge   = GIMP_SMUDGE (paint_core);
  GimpSmudgeOptions  *options  = GIMP_SMUDGE_OPTIONS (paint_options);
  GimpContext        *context  = GIMP_CONTEXT (paint_options);
  GimpDynamics       *dynamics = GIMP_BRUSH_CORE (paint_core)->dynamics;
  GimpDynamicsOutput *opacity_output;
  GimpDynamicsOutput *rate_output;
  GimpDynamicsOutput *hardness_output;
  GimpImage          *image;
  TempBuf            *area;
  PixelRegion         srcPR, destPR, tempPR;
  gdouble             fade_point;
  gdouble             opacity;
  gdouble             rate;
  gdouble             dynamic_rate;
  gint                x, y;
  gdouble             hardness;

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

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options,
                                         coords);
  if (! area)
    return;

  /*  Get the unclipped acumulator coordinates  */
  gimp_smudge_accumulator_coords (paint_core, coords, &x, &y);

  /* srcPR will be the pixels under the current painthit from the drawable */
  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     area->x, area->y, area->width, area->height, FALSE);

  /* Enable dynamic rate */
  rate_output = gimp_dynamics_get_output (dynamics,
                                          GIMP_DYNAMICS_OUTPUT_RATE);

  dynamic_rate = gimp_dynamics_output_get_linear_value (rate_output,
                                                        coords,
                                                        paint_options,
                                                        fade_point);

  rate = (options->rate / 100.0) * dynamic_rate;

  /* The tempPR will be the built up buffer (for smudge) */
  pixel_region_init_data (&tempPR, smudge->accum_data,
                          smudge->accumPR.bytes,
                          smudge->accumPR.rowstride,
                          area->x - x,
                          area->y - y,
                          area->width,
                          area->height);

  /* The dest will be the paint area we got above (= canvas_buf) */
  pixel_region_init_temp_buf (&destPR, area,
                              0, 0, area->width, area->height);

  /*  Smudge uses the buffer Accum.
   *  For each successive painthit Accum is built like this
   *    Accum =  rate*Accum  + (1-rate)*I.
   *  where I is the pixels under the current painthit.
   *  Then the paint area (canvas_buf) is built as
   *    (Accum,1) (if no alpha),
   */

  blend_region (&srcPR, &tempPR, &tempPR, ROUND (rate * 255.0));

  /* re-init the tempPR */
  pixel_region_init_data (&tempPR, smudge->accum_data,
                          smudge->accumPR.bytes,
                          smudge->accumPR.rowstride,
                          area->x - x,
                          area->y - y,
                          area->width,
                          area->height);

  if (! gimp_drawable_has_alpha (drawable))
    add_alpha_region (&tempPR, &destPR);
  else
    copy_region (&tempPR, &destPR);

  hardness_output = gimp_dynamics_get_output (dynamics,
                                              GIMP_DYNAMICS_OUTPUT_HARDNESS);

  hardness = gimp_dynamics_output_get_linear_value (hardness_output,
                                                    coords,
                                                    paint_options,
                                                    fade_point);

  gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                  coords,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  hardness,
                                  GIMP_PAINT_INCREMENTAL);
}

static void
gimp_smudge_accumulator_coords (GimpPaintCore    *paint_core,
                                const GimpCoords *coords,
                                gint             *x,
                                gint             *y)
{
  GimpSmudge *smudge = GIMP_SMUDGE (paint_core);

  *x = (gint) coords->x - smudge->accum_size / 2;
  *y = (gint) coords->y - smudge->accum_size / 2;
}

static void
gimp_smudge_accumulator_size (GimpPaintOptions *paint_options,
                              gint             *accumulator_size)
{

  /* Note: the max brush mask size plus a border of 1 pixel and a little
   * headroom */
  *accumulator_size = ceil (sqrt (2 * SQR (paint_options->brush_size + 1)) + 2);
}
