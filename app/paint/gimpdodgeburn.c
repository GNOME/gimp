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

#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "base/gimplut.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gimpdodgeburn.h"
#include "gimpdodgeburnoptions.h"

#include "gimp-intl.h"


static void   gimp_dodge_burn_finalize   (GObject            *object);

static void   gimp_dodge_burn_paint      (GimpPaintCore      *paint_core,
                                          GimpDrawable       *drawable,
                                          GimpPaintOptions   *paint_options,
                                          GimpPaintState      paint_state,
                                          guint32             time);
static void   gimp_dodge_burn_motion     (GimpPaintCore      *paint_core,
                                          GimpDrawable       *drawable,
                                          GimpPaintOptions   *paint_options);

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
      gimp_dodge_burn_motion (paint_core, drawable, paint_options);
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
                        GimpPaintOptions *paint_options)
{
  GimpDodgeBurn        *dodgeburn        = GIMP_DODGE_BURN (paint_core);
  GimpContext          *context          = GIMP_CONTEXT (paint_options);
  GimpPressureOptions  *pressure_options = paint_options->pressure_options;
  GimpImage            *image;
  TempBuf              *area;
  TempBuf              *orig;
  PixelRegion           srcPR, destPR, tempPR;
  guchar               *temp_data;
  gdouble               opacity;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (gimp_drawable_is_indexed (drawable))
    return;

  opacity = gimp_paint_options_get_fade (paint_options, image,
                                         paint_core->pixel_dist);
  if (opacity == 0.0)
    return;

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return;

  /* Constant painting --get a copy of the orig drawable (with no
   * paint from this stroke yet)
   */
  {
    GimpItem *item = GIMP_ITEM (drawable);
    gint      x1, y1, x2, y2;

    x1 = CLAMP (area->x, 0, gimp_item_width  (item));
    y1 = CLAMP (area->y, 0, gimp_item_height (item));
    x2 = CLAMP (area->x + area->width,  0, gimp_item_width  (item));
    y2 = CLAMP (area->y + area->height, 0, gimp_item_height (item));

    if (!(x2 - x1) || !(y2 - y1))
      return;

    /*  get the original untouched image  */
    orig = gimp_paint_core_get_orig_image (paint_core, drawable, x1, y1, x2, y2);

    pixel_region_init_temp_buf (&srcPR, orig,
                                0, 0, x2 - x1, y2 - y1);
  }

  /* tempPR will hold the dodgeburned region */
  temp_data = g_malloc (srcPR.h * srcPR.bytes * srcPR.w);

  pixel_region_init_data (&tempPR, temp_data,
                          srcPR.bytes,
                          srcPR.bytes * srcPR.w,
                          srcPR.x,
                          srcPR.y,
                          srcPR.w,
                          srcPR.h);

  /*  DodgeBurn the region  */
  gimp_lut_process (dodgeburn->lut, &srcPR, &tempPR);

  /* The dest is the paint area we got above (= canvas_buf) */
  pixel_region_init_temp_buf (&destPR, area,
                              0, 0, area->width, area->height);

  /* Now add an alpha to the dodgeburned region
   * and put this in area = canvas_buf
   */
  if (! gimp_drawable_has_alpha (drawable))
    add_alpha_region (&tempPR, &destPR);
  else
    copy_region (&tempPR, &destPR);

  if (pressure_options->opacity)
    opacity *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  /* Replace the newly dodgedburned area (canvas_buf) to the image */
  gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  GIMP_PAINT_CONSTANT);

  g_free (temp_data);
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
