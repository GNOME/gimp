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

#include "paint-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"

#include "gimpconvolve.h"
#include "gimpconvolveoptions.h"

#include "gimp-intl.h"


#define FIELD_COLS    4
#define MIN_BLUR      64         /*  (8/9 original pixel)   */
#define MAX_BLUR      0.25       /*  (1/33 original pixel)  */
#define MIN_SHARPEN   -512
#define MAX_SHARPEN   -64


static void    gimp_convolve_paint            (GimpPaintCore     *paint_core,
                                               GimpDrawable      *drawable,
                                               GimpPaintOptions  *paint_options,
                                               GimpPaintState     paint_state,
                                               guint32            time);
static void    gimp_convolve_motion           (GimpPaintCore     *paint_core,
                                               GimpDrawable      *drawable,
                                               GimpPaintOptions  *paint_options);

static void    gimp_convolve_calculate_matrix (GimpConvolve      *convolve,
                                               GimpConvolveType   type,
                                               gdouble            rate);
static gdouble gimp_convolve_sum_matrix       (const gfloat      *matrix);


G_DEFINE_TYPE (GimpConvolve, gimp_convolve, GIMP_TYPE_BRUSH_CORE)


void
gimp_convolve_register (Gimp                      *gimp,
                        GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_CONVOLVE,
                GIMP_TYPE_CONVOLVE_OPTIONS,
                "gimp-convolve",
                _("Convolve"),
                "gimp-tool-blur");
}

static void
gimp_convolve_class_init (GimpConvolveClass *klass)
{
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  paint_core_class->paint = gimp_convolve_paint;
}

static void
gimp_convolve_init (GimpConvolve *convolve)
{
  gint i;

  for (i = 0; i < 9; i++)
    convolve->matrix[i] = 1.0;

  convolve->matrix_divisor = 9.0;
}

static void
gimp_convolve_paint (GimpPaintCore    *paint_core,
                     GimpDrawable     *drawable,
                     GimpPaintOptions *paint_options,
                     GimpPaintState    paint_state,
                     guint32           time)
{
  switch (paint_state)
    {
    case GIMP_PAINT_STATE_MOTION:
      gimp_convolve_motion (paint_core, drawable, paint_options);
      break;

    default:
      break;
    }
}

static void
gimp_convolve_motion (GimpPaintCore    *paint_core,
                      GimpDrawable     *drawable,
                      GimpPaintOptions *paint_options)
{
  GimpConvolve        *convolve         = GIMP_CONVOLVE (paint_core);
  GimpBrushCore       *brush_core       = GIMP_BRUSH_CORE (paint_core);
  GimpConvolveOptions *options          = GIMP_CONVOLVE_OPTIONS (paint_options);
  GimpContext         *context          = GIMP_CONTEXT (paint_options);
  GimpPressureOptions *pressure_options = paint_options->pressure_options;
  GimpImage           *image;
  TempBuf             *area;
  PixelRegion          srcPR;
  PixelRegion          destPR;
  PixelRegion          tempPR;
  guchar              *buffer;
  gdouble              opacity;
  gdouble              rate;
  gint                 bytes;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (gimp_drawable_is_indexed (drawable))
    return;

  /* If the brush is smaller than the convolution matrix, don't convolve */
  if (brush_core->brush->mask->width  < 3 ||
      brush_core->brush->mask->height < 3)
    return;

  opacity = gimp_paint_options_get_fade (paint_options, image,
                                         paint_core->pixel_dist);
  if (opacity == 0.0)
    return;

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return;

  rate = options->rate;

  if (pressure_options->rate)
    rate *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  gimp_convolve_calculate_matrix (convolve, options->type, rate);

  /*  configure the source pixel region  */
  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     area->x, area->y, area->width, area->height, FALSE);

  if (gimp_drawable_has_alpha (drawable))
    {
      bytes = srcPR.bytes;

      buffer = g_malloc (area->height * bytes * area->width);

      pixel_region_init_data (&tempPR, buffer,
                              bytes, bytes * area->width,
                              0, 0, area->width, area->height);

      copy_region (&srcPR, &tempPR);
    }
  else
    {
      /* note: this particular approach needlessly convolves the totally-
         opaque alpha channel. A faster approach would be to keep
         tempPR the same number of bytes as srcPR, and extend the
         paint_core_replace_canvas API to handle non-alpha images. */

      bytes = srcPR.bytes + 1;

      buffer = g_malloc (area->height * bytes * area->width);

      pixel_region_init_data (&tempPR, buffer,
                              bytes, bytes * area->width,
                              0, 0, area->width, area->height);

      add_alpha_region (&srcPR, &tempPR);
    }

  /*  Convolve the region  */
  pixel_region_init_data (&tempPR, buffer,
                          bytes, bytes * area->width,
                          0, 0, area->width, area->height);

  pixel_region_init_temp_buf (&destPR, area,
                              0, 0, area->width, area->height);

  convolve_region (&tempPR, &destPR,
                   convolve->matrix, 3, convolve->matrix_divisor,
                   GIMP_NORMAL_CONVOL, TRUE);

  g_free (buffer);

  gimp_brush_core_replace_canvas (brush_core, drawable,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  GIMP_PAINT_INCREMENTAL);
}

static void
gimp_convolve_calculate_matrix (GimpConvolve    *convolve,
                                GimpConvolveType type,
                                gdouble          rate)
{
  /*  find percent of tool pressure  */
  gdouble percent = MIN (rate / 100.0, 1.0);

  /*  get the appropriate convolution matrix and size and divisor  */
  switch (type)
    {
    case GIMP_BLUR_CONVOLVE:
      convolve->matrix[4] = MIN_BLUR + percent * (MAX_BLUR - MIN_BLUR);
      break;

    case GIMP_SHARPEN_CONVOLVE:
      convolve->matrix[4] = MIN_SHARPEN + percent * (MAX_SHARPEN - MIN_SHARPEN);
      break;

    case GIMP_CUSTOM_CONVOLVE:
      break;
    }

  convolve->matrix_divisor = gimp_convolve_sum_matrix (convolve->matrix);
}

static gdouble
gimp_convolve_sum_matrix (const gfloat *matrix)
{
  gdouble sum = 0.0;
  gint    i;

  for (i = 0; i < 9; i++)
    sum += matrix[i];

  return sum;
}
