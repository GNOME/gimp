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

/* Different clip relationships between a blur-blob and edges:
 * see convolve_motion
 */
typedef enum
{
  CONVOLVE_NCLIP,       /* Left or top edge     */
  CONVOLVE_NOT_CLIPPED, /* No edges             */
  CONVOLVE_PCLIP        /* Right or bottom edge */
} ConvolveClipType;


static void    gimp_convolve_paint            (GimpPaintCore     *paint_core,
                                               GimpDrawable      *drawable,
                                               GimpPaintOptions  *paint_options,
                                               GimpPaintState     paint_state,
                                               guint32            time);
static void    gimp_convolve_motion           (GimpPaintCore     *paint_core,
                                               GimpDrawable      *drawable,
                                               GimpPaintOptions  *paint_options);

static void    gimp_convolve_calculate_matrix (GimpConvolveType   type,
                                               gdouble            rate);
static void    gimp_convolve_copy_matrix      (const gfloat      *src,
                                               gfloat            *dest,
                                               gint               size);
static gdouble gimp_convolve_sum_matrix       (const gfloat      *matrix,
                                               gint               size);


/* FIXME: this code is not MT-safe */

static const gint  matrix_size = 5;
static gdouble     matrix_divisor;

static gfloat matrix[25] =
{
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 1, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
};

static const gfloat blur_matrix[25] =
{
  0, 0, 0, 0, 0,
  0, 1, 1, 1, 0,
  0, 1, MIN_BLUR, 1, 0,
  0, 1, 1, 1, 0,
  0, 0 ,0, 0, 0,
};

static const gfloat sharpen_matrix[25] =
{
  0, 0, 0, 0, 0,
  0, 1, 1, 1, 0,
  0, 1, MIN_SHARPEN, 1, 0,
  0, 1, 1, 1, 0,
  0, 0, 0, 0, 0,
};


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
  GimpBrushCore       *brush_core       = GIMP_BRUSH_CORE (paint_core);
  GimpConvolveOptions *options          = GIMP_CONVOLVE_OPTIONS (paint_options);
  GimpContext         *context          = GIMP_CONTEXT (paint_options);
  GimpPressureOptions *pressure_options = paint_options->pressure_options;
  GimpImage           *image;
  TempBuf             *area;
  guchar              *temp_data;
  PixelRegion          srcPR;
  PixelRegion          destPR;
  gdouble              opacity;
  gdouble              rate;
  ConvolveClipType     area_hclip = CONVOLVE_NOT_CLIPPED;
  ConvolveClipType     area_vclip = CONVOLVE_NOT_CLIPPED;
  gint                 marginx    = 0;
  gint                 marginy    = 0;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (gimp_drawable_is_indexed (drawable))
    return;

  /* If the brush is smaller than the convolution matrix, don't convolve */
  if (brush_core->brush->mask->width  < matrix_size ||
      brush_core->brush->mask->height < matrix_size)
    return;

  opacity = gimp_paint_options_get_fade (paint_options, image,
                                         paint_core->pixel_dist);
  if (opacity == 0.0)
    return;

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return;

  /*  configure the source pixel region  */
  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     area->x, area->y, area->width, area->height, FALSE);

  /*  configure the destination pixel region  */
  pixel_region_init_temp_buf (&destPR, area,
                              0, 0, area->width, area->height);

  rate = options->rate;

  if (pressure_options->rate)
    rate *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  gimp_convolve_calculate_matrix (options->type, rate);

  /*  Image region near edges? If so, paint area will be clipped   */
  /*  with respect to brush mask + 1 pixel border (# 19285)        */

  marginx = ((gint) paint_core->cur_coords.x -
             brush_core->brush->mask->width / 2 - 1);

  if (marginx != area->x)
    {
      area_hclip = CONVOLVE_NCLIP;
    }
  else
    {
      marginx = area->width - brush_core->brush->mask->width - 2;

      if (marginx != 0)
        area_hclip = CONVOLVE_PCLIP;
    }

  marginy = ((gint) paint_core->cur_coords.y -
             brush_core->brush->mask->height / 2 - 1);

  if (marginy != area->y)
    {
      area_vclip = CONVOLVE_NCLIP;
    }
  else
    {
      marginy = area->height - brush_core->brush->mask->height - 2;

      if (marginy != 0)
        area_vclip = CONVOLVE_PCLIP;
    }

  /*  Has the TempBuf been clipped by a canvas edge or two?  */
  if (area_hclip == CONVOLVE_NOT_CLIPPED &&
      area_vclip == CONVOLVE_NOT_CLIPPED)
    {
      /* No clipping...                                              */
      /* Standard case: copy src to temp. convolve temp to dest.     */
      /* Brush defines pipe size and no edge adjustments are needed. */

      /*  If the source has no alpha, then add alpha pixels          */
      /*  Because paint_core.c is alpha-only code. See below.        */

      PixelRegion tempPR;
      gint        bytes;

      if (! gimp_drawable_has_alpha (drawable))
        {
          /* note: this particular approach needlessly convolves the totally-
             opaque alpha channel. A faster approach would be to keep
             tempPR the same number of bytes as srcPR, and extend the
             paint_core_replace_canvas API to handle non-alpha images. */

          bytes = srcPR.bytes + 1;

          temp_data = g_malloc (area->height * bytes * area->width);

          pixel_region_init_data (&tempPR, temp_data,
                                  bytes, bytes * area->width,
                                  0, 0, area->width, area->height);

          add_alpha_region (&srcPR, &tempPR);
        }
      else
        {
          bytes = srcPR.bytes;

          temp_data = g_malloc (area->height * bytes * area->width);

          pixel_region_init_data (&tempPR, temp_data,
                                  bytes, bytes * area->width,
                                  0, 0, area->width, area->height);

          copy_region (&srcPR, &tempPR);
        }

      /*  Convolve the region  */
      pixel_region_init_data (&tempPR, temp_data,
                              bytes, bytes * area->width,
                              0, 0, area->width, area->height);

      convolve_region (&tempPR, &destPR, matrix, matrix_size,
                       matrix_divisor, GIMP_NORMAL_CONVOL, TRUE);

      /*  Free the allocated temp space  */
      g_free (temp_data);
    }
  else
    {
      /* TempBuf clipping has occurred on at least one edge...
       * Edge case: expand area under brush margin px on near edge(s), convolve
       * expanded buffers. copy src -> ovrsz1 convolve ovrsz1 -> ovrsz2
       * copy-with-crop ovrsz2 -> dest
       */
      PixelRegion  ovrsz1PR;
      PixelRegion  ovrsz2PR;
      guchar      *ovrsz1_data = NULL;
      guchar      *ovrsz2_data = NULL;
      guchar       fill[4];
      gint         bytes;
      gint         rowstride;

      gimp_pickable_get_pixel_at (GIMP_PICKABLE (drawable),
                                  CLAMP ((gint) paint_core->cur_coords.x,
                                         0,
                                         gimp_item_width  (GIMP_ITEM (drawable)) - 1),
                                  CLAMP ((gint) paint_core->cur_coords.y,
                                         0,
                                         gimp_item_height (GIMP_ITEM (drawable)) - 1),
                                  fill);

      marginx *= (marginx < 0) ? -1 : 0;
      marginy *= (marginy < 0) ? -1 : 0;

      bytes     = (gimp_drawable_has_alpha (drawable) ?
                   srcPR.bytes : srcPR.bytes + 1);
      rowstride = (area->width + marginx) * bytes;

      if (! gimp_drawable_has_alpha (drawable))
        fill[bytes - 1] = OPAQUE_OPACITY;

      ovrsz1_data = g_malloc ((area->height + marginy) * rowstride);
      ovrsz2_data = g_malloc ((area->height + marginy) * rowstride);

      pixel_region_init_data (&ovrsz2PR, ovrsz2_data,
                              bytes, rowstride,
                              0, 0,
                              area->width  + marginx,
                              area->height + marginy);

      pixel_region_init_data (&ovrsz1PR, ovrsz1_data,
                              bytes, rowstride,
                              0, 0,
                              area->width  + marginx,
                              area->height + marginy);

      color_region (&ovrsz1PR, fill);

      pixel_region_init_data (&ovrsz1PR, ovrsz1_data,
                              bytes, rowstride,
                              (area_hclip == CONVOLVE_NCLIP) ? marginx : 0,
                              (area_vclip == CONVOLVE_NCLIP) ? marginy : 0,
                              area->width,
                              area->height);

      if (! gimp_drawable_has_alpha (drawable))
        add_alpha_region (&srcPR, &ovrsz1PR);
      else
        copy_region (&srcPR, &ovrsz1PR);

      /*  Convolve the region  */
      pixel_region_init_data (&ovrsz1PR, ovrsz1_data,
                              bytes, rowstride,
                              0, 0,
                              area->width  + marginx,
                              area->height + marginy);

      convolve_region (&ovrsz1PR, &ovrsz2PR, matrix, matrix_size,
                       matrix_divisor, GIMP_NORMAL_CONVOL, TRUE);

      /* Crop and copy to destination */
      pixel_region_init_data (&ovrsz2PR, ovrsz2_data,
                              bytes, rowstride,
                              (area_hclip == CONVOLVE_NCLIP) ? marginx : 0,
                              (area_vclip == CONVOLVE_NCLIP) ? marginy : 0,
                              area->width,
                              area->height);

      copy_region (&ovrsz2PR, &destPR);

      g_free (ovrsz1_data);
      g_free (ovrsz2_data);
    }

  gimp_brush_core_replace_canvas (brush_core, drawable,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  GIMP_PAINT_INCREMENTAL);
}

static void
gimp_convolve_calculate_matrix (GimpConvolveType type,
                                gdouble          rate)
{
  /*  find percent of tool pressure  */
  gdouble percent = MIN (rate / 100.0, 1.0);

  /*  get the appropriate convolution matrix and size and divisor  */
  switch (type)
    {
    case GIMP_BLUR_CONVOLVE:
      gimp_convolve_copy_matrix (blur_matrix, matrix, matrix_size);
      matrix[12] = MIN_BLUR + percent * (MAX_BLUR - MIN_BLUR);
      break;

    case GIMP_SHARPEN_CONVOLVE:
      gimp_convolve_copy_matrix (sharpen_matrix, matrix, matrix_size);
      matrix[12] = MIN_SHARPEN + percent * (MAX_SHARPEN - MIN_SHARPEN);
      break;

    case GIMP_CUSTOM_CONVOLVE:
      break;
    }

  matrix_divisor = gimp_convolve_sum_matrix (matrix, matrix_size);

  if (G_UNLIKELY (matrix_divisor == 0.0))
    matrix_divisor = 1.0;
}

static void
gimp_convolve_copy_matrix (const gfloat *src,
                           gfloat       *dest,
                           gint          size)
{
  size *= size;

  while (size--)
    *dest++ = *src++;
}

static gdouble
gimp_convolve_sum_matrix (const gfloat *matrix,
                          gint          size)
{
  gdouble sum = 0.0;

  size *= size;

  while (size--)
    sum += *matrix++;

  return sum;
}
