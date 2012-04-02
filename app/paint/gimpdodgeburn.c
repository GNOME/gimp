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

#include "base/gimplut.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimpimage.h"

#include "gimpdodgeburn.h"
#include "gimpdodgeburnoptions.h"

#include "gimp-intl.h"


static void   gimp_dodge_burn_finalize   (GObject            *object);

static void   gimp_dodge_burn_paint      (GimpPaintCore      *paint_core,
                                          GimpDrawable       *drawable,
                                          GimpPaintOptions   *paint_options,
                                          const GimpCoords   *coords,
                                          GimpPaintState      paint_state,
                                          guint32             time);
static void   gimp_dodge_burn_motion     (GimpPaintCore      *paint_core,
                                          GimpDrawable       *drawable,
                                          GimpPaintOptions   *paint_options,
                                          const GimpCoords   *coords);

static void   gimp_dodge_burn_make_luts  (GimpDodgeBurn      *dodgeburn,
                                          gdouble             db_exposure,
                                          GimpDodgeBurnType   type,
                                          GimpTransferMode    mode,
                                          GimpDrawable       *drawable);

static gfloat gimp_dodge_burn_highlights_lut_func (gpointer   user_data,
                                                   gint       nchannels,
                                                   gint       channel,
                                                   gfloat     value);
static gfloat gimp_dodge_burn_midtones_lut_func   (gpointer   user_data,
                                                   gint       nchannels,
                                                   gint       channel,
                                                   gfloat     value);
static gfloat gimp_dodge_burn_shadows_lut_func    (gpointer   user_data,
                                                   gint       nchannels,
                                                   gint       channel,
                                                   gfloat     value);


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
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  object_class->finalize  = gimp_dodge_burn_finalize;

  paint_core_class->paint = gimp_dodge_burn_paint;

  brush_core_class->handles_changing_brush = TRUE;
}

static void
gimp_dodge_burn_init (GimpDodgeBurn *dodgeburn)
{
}

static void
gimp_dodge_burn_finalize (GObject *object)
{
  GimpDodgeBurn *dodgeburn = GIMP_DODGE_BURN (object);

  if (dodgeburn->lut)
    {
      gimp_lut_free (dodgeburn->lut);
      dodgeburn->lut = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dodge_burn_paint (GimpPaintCore    *paint_core,
                       GimpDrawable     *drawable,
                       GimpPaintOptions *paint_options,
                       const GimpCoords *coords,
                       GimpPaintState    paint_state,
                       guint32           time)
{
  GimpDodgeBurn        *dodgeburn = GIMP_DODGE_BURN (paint_core);
  GimpDodgeBurnOptions *options   = GIMP_DODGE_BURN_OPTIONS (paint_options);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      dodgeburn->lut = gimp_lut_new ();

      gimp_dodge_burn_make_luts (dodgeburn,
                                 options->exposure,
                                 options->type,
                                 options->mode,
                                 drawable);
      break;

    case GIMP_PAINT_STATE_MOTION:
      gimp_dodge_burn_motion (paint_core, drawable, paint_options, coords);
      break;

    case GIMP_PAINT_STATE_FINISH:
      if (dodgeburn->lut)
        {
          gimp_lut_free (dodgeburn->lut);
          dodgeburn->lut = NULL;
        }
      break;
    }
}

static void
gimp_dodge_burn_motion (GimpPaintCore    *paint_core,
                        GimpDrawable     *drawable,
                        GimpPaintOptions *paint_options,
                        const GimpCoords *coords)
{
  GimpDodgeBurn      *dodgeburn = GIMP_DODGE_BURN (paint_core);
  GimpContext        *context   = GIMP_CONTEXT (paint_options);
  GimpDynamics       *dynamics  = GIMP_BRUSH_CORE (paint_core)->dynamics;
  GimpDynamicsOutput *opacity_output;
  GimpDynamicsOutput *hardness_output;
  GimpImage          *image;
  GeglBuffer         *paint_buffer;
  gint                paint_buffer_x;
  gint                paint_buffer_y;
  const Babl         *orig_format;
  TempBuf            *orig_temp;
  GeglBuffer         *orig_buffer;
  GeglRectangle       orig_rect;
  PixelRegion         srcPR, tempPR;
  TempBuf            *db_temp;
  GeglBuffer         *db_buffer;
  gdouble             fade_point;
  gdouble             opacity;
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

  paint_buffer = gimp_paint_core_get_paint_buffer (paint_core, drawable,
                                                   paint_options, coords,
                                                   &paint_buffer_x,
                                                   &paint_buffer_y);
  if (! paint_buffer)
    return;

  /* Constant painting --get a copy of the orig drawable (with no
   * paint from this stroke yet)
   */
  if (! gimp_rectangle_intersect (paint_buffer_x,
                                  paint_buffer_y,
                                  gegl_buffer_get_width  (paint_buffer),
                                  gegl_buffer_get_height (paint_buffer),
                                  0, 0,
                                  gimp_item_get_width  (GIMP_ITEM (drawable)),
                                  gimp_item_get_height (GIMP_ITEM (drawable)),
                                  &orig_rect.x,
                                  &orig_rect.y,
                                  &orig_rect.width,
                                  &orig_rect.height))
    {
      return;
    }

  /*  get the original untouched image  */
  orig_format = gimp_drawable_get_format (drawable);

  orig_temp = temp_buf_new (orig_rect.width, orig_rect.height,
                            babl_format_get_bytes_per_pixel (orig_format),
                            0, 0, NULL);

  orig_buffer = gimp_temp_buf_create_buffer (orig_temp, orig_format, TRUE);

  gegl_buffer_copy (gimp_paint_core_get_orig_image (paint_core),
                    &orig_rect,
                    orig_buffer,
                    GEGL_RECTANGLE (0, 0, 0, 0));

  pixel_region_init_temp_buf (&srcPR, orig_temp,
                              0, 0, orig_rect.width, orig_rect.height);

  /* tempPR will hold the dodgeburned region */
  db_temp = temp_buf_new (orig_rect.width, orig_rect.height,
                          babl_format_get_bytes_per_pixel (orig_format),
                          0, 0, NULL);
  db_buffer = gimp_temp_buf_create_buffer (db_temp, orig_format, TRUE);

  pixel_region_init_temp_buf (&tempPR, db_temp,
                              0, 0, db_temp->width, db_temp->height);

  /*  DodgeBurn the region  */
  gimp_lut_process (dodgeburn->lut, &srcPR, &tempPR);

  g_object_unref (orig_buffer);

  gegl_buffer_copy (db_buffer, NULL, paint_buffer, NULL);
  g_object_unref (db_buffer);

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

static void
gimp_dodge_burn_make_luts (GimpDodgeBurn     *dodgeburn,
                           gdouble            db_exposure,
                           GimpDodgeBurnType  type,
                           GimpTransferMode   mode,
                           GimpDrawable      *drawable)
{
  GimpLutFunc   lut_func;
  gint          nchannels = gimp_drawable_bytes (drawable);
  static gfloat exposure;

  exposure = db_exposure / 100.0;

  /* make the exposure negative if burn for luts*/
  if (type == GIMP_BURN)
    exposure = -exposure;

  switch (mode)
    {
    case GIMP_HIGHLIGHTS:
      lut_func = gimp_dodge_burn_highlights_lut_func;
      break;
    case GIMP_MIDTONES:
      lut_func = gimp_dodge_burn_midtones_lut_func;
      break;
    case GIMP_SHADOWS:
      lut_func = gimp_dodge_burn_shadows_lut_func;
      break;
    default:
      lut_func = NULL;
      break;
    }

  gimp_lut_setup_exact (dodgeburn->lut,
                        lut_func, (gpointer) &exposure,
                        nchannels);
}

static gfloat
gimp_dodge_burn_highlights_lut_func (gpointer  user_data,
                                     gint      nchannels,
                                     gint      channel,
                                     gfloat    value)
{
  gfloat *exposure_ptr = (gfloat *) user_data;
  gfloat  exposure     = *exposure_ptr;
  gfloat  factor       = 1.0 + exposure * (.333333);

  if ((nchannels == 2 && channel == 1) ||
      (nchannels == 4 && channel == 3))
    return value;

  return factor * value;
}

static gfloat
gimp_dodge_burn_midtones_lut_func (gpointer  user_data,
                                   gint      nchannels,
                                   gint      channel,
                                   gfloat    value)
{
  gfloat *exposure_ptr = (gfloat *) user_data;
  gfloat  exposure     = *exposure_ptr;
  gfloat  factor;

  if ((nchannels == 2 && channel == 1) ||
      (nchannels == 4 && channel == 3))
    return value;

  if (exposure < 0)
    factor = 1.0 - exposure * (.333333);
  else
    factor = 1 / (1.0 + exposure);

  return pow (value, factor);
}

static gfloat
gimp_dodge_burn_shadows_lut_func (gpointer  user_data,
                                  gint      nchannels,
                                  gint      channel,
                                  gfloat    value)
{
  gfloat *exposure_ptr = (gfloat *) user_data;
  gfloat  exposure     = *exposure_ptr;
  gfloat  new_value;
  gfloat  factor;

  if ((nchannels == 2 && channel == 1) ||
      (nchannels == 4 && channel == 3))
    return value;

  if (exposure >= 0)
    {
      factor = 0.333333 * exposure;
      new_value =  factor + value - factor * value;
    }
  else /* exposure < 0 */
    {
      factor = -0.333333 * exposure;
      if (value < factor)
        new_value = 0;
      else /*factor <= value <=1*/
        new_value = (value - factor)/(1 - factor);
    }

  return new_value;
}
