/* The GIMP -- an image manipulation program
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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "base/gimplut.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "gimpdodgeburn.h"


static void     gimp_dodgeburn_class_init (GimpDodgeBurnClass *klass);
static void     gimp_dodgeburn_init       (GimpDodgeBurn      *dodgeburn);

static void     gimp_dodgeburn_make_luts  (GimpPaintCore      *paint_core,
                                           gdouble             db_exposure,
                                           DodgeBurnType       type,
                                           GimpTransferMode    mode,
                                           GimpLut            *lut,
                                           GimpDrawable       *drawable);

static void     gimp_dodgeburn_paint      (GimpPaintCore      *paint_core,
                                           GimpDrawable       *drawable,
                                           PaintOptions       *paint_options,
                                           GimpPaintCoreState  paint_state);

static void     gimp_dodgeburn_motion     (GimpPaintCore      *paint_core,
                                           GimpDrawable       *drawable,
                                           PaintOptions       *paint_options);

static gfloat   gimp_dodgeburn_highlights_lut_func (gpointer       user_data,
                                                    gint           nchannels,
                                                    gint           channel,
                                                    gfloat         value);
static gfloat   gimp_dodgeburn_midtones_lut_func   (gpointer       user_data,
                                                    gint           nchannels,
                                                    gint           channel,
                                                    gfloat         value);
static gfloat   gimp_dodgeburn_shadows_lut_func    (gpointer       user_data,
                                                    gint           nchannels,
                                                    gint           channel,
                                                    gfloat         value);


static GimpPaintCoreClass *parent_class = NULL;


GType
gimp_dodgeburn_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpDodgeBurnClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_dodgeburn_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpDodgeBurn),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_dodgeburn_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_CORE,
                                     "GimpDodgeBurn",
                                     &info, 0);
    }

  return type;
}

static void
gimp_dodgeburn_class_init (GimpDodgeBurnClass *klass)
{
  GimpPaintCoreClass *paint_core_class;

  paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  paint_core_class->paint = gimp_dodgeburn_paint;
}

static void
gimp_dodgeburn_init (GimpDodgeBurn *dodgeburn)
{
  GimpPaintCore *paint_core;

  paint_core = GIMP_PAINT_CORE (dodgeburn);

  paint_core->flags |= CORE_HANDLES_CHANGING_BRUSH;
}

static void 
gimp_dodgeburn_make_luts (GimpPaintCore     *paint_core,
                          gdouble            db_exposure,
                          DodgeBurnType      type,
                          GimpTransferMode   mode,
                          GimpLut           *lut,
                          GimpDrawable      *drawable)
{
  GimpLutFunc   lut_func;
  gint          nchannels = gimp_drawable_bytes (drawable);
  static gfloat exposure;

  exposure = db_exposure / 100.0;

  /* make the exposure negative if burn for luts*/
  if (type == BURN)
    exposure = -exposure;

  switch (mode)
    {
    case GIMP_HIGHLIGHTS:
      lut_func = gimp_dodgeburn_highlights_lut_func; 
      break;
    case GIMP_MIDTONES:
      lut_func = gimp_dodgeburn_midtones_lut_func; 
      break;
    case GIMP_SHADOWS:
      lut_func = gimp_dodgeburn_shadows_lut_func; 
      break;
    default:
      lut_func = NULL; 
      break;
    }

  gimp_lut_setup_exact (lut,
			lut_func, (gpointer) &exposure,
			nchannels);
}

static void
gimp_dodgeburn_paint (GimpPaintCore      *paint_core,
                      GimpDrawable       *drawable,
                      PaintOptions       *paint_options,
                      GimpPaintCoreState  paint_state)
{
  DodgeBurnOptions *options;

  options = (DodgeBurnOptions *) paint_options;

  switch (paint_state)
    {
    case INIT_PAINT:
      options->lut = gimp_lut_new ();

      gimp_dodgeburn_make_luts (paint_core,
                                options->exposure,
                                options->type,
                                options->mode,
                                options->lut,
                                drawable);
      break;

    case MOTION_PAINT:
      gimp_dodgeburn_motion (paint_core,
                             drawable,
                             paint_options);
      break;

    case FINISH_PAINT:
      if (options->lut)
	{
	  gimp_lut_free (options->lut);
	  options->lut = NULL;
	}
      break;

    default:
      break;
    }
}

static void
gimp_dodgeburn_motion (GimpPaintCore *paint_core,
                       GimpDrawable  *drawable,
                       PaintOptions  *paint_options)
{
  DodgeBurnOptions     *options;
  PaintPressureOptions *pressure_options;
  GimpImage            *gimage;
  TempBuf              *area;
  TempBuf              *orig;
  PixelRegion           srcPR, destPR, tempPR;
  guchar               *temp_data;
  gint                  opacity;
  gdouble               scale;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  /*  If the image type is indexed, don't dodgeburn  */
  if ((gimp_drawable_type (drawable) == GIMP_INDEXED_IMAGE) ||
      (gimp_drawable_type (drawable) == GIMP_INDEXEDA_IMAGE))
    return;

  options = (DodgeBurnOptions *) paint_options;

  pressure_options = paint_options->pressure_options;

  if (pressure_options->size)
    scale = paint_core->cur_coords.pressure;
  else
    scale = 1.0;

  /*  Get a region which can be used to paint to  */
  if (! (area = gimp_paint_core_get_paint_area (paint_core, drawable, scale)))
    return;

  /* Constant painting --get a copy of the orig drawable (with no
   * paint from this stroke yet)
   */
  {
    gint x1, y1, x2, y2;

    x1 = CLAMP (area->x, 0, gimp_drawable_width (drawable));
    y1 = CLAMP (area->y, 0, gimp_drawable_height (drawable));
    x2 = CLAMP (area->x + area->width,  0, gimp_drawable_width (drawable));
    y2 = CLAMP (area->y + area->height, 0, gimp_drawable_height (drawable));

    if (!(x2 - x1) || !(y2 - y1))
      return;

    /*  get the original untouched image  */
    orig = gimp_paint_core_get_orig_image (paint_core, drawable, x1, y1, x2, y2);

    srcPR.bytes     = orig->bytes;
    srcPR.x         = 0; 
    srcPR.y         = 0;
    srcPR.w         = x2 - x1;
    srcPR.h         = y2 - y1;
    srcPR.rowstride = srcPR.bytes * orig->width;
    srcPR.data      = temp_buf_data (orig);
  }

  /* tempPR will hold the dodgeburned region*/
  tempPR.bytes     = srcPR.bytes;
  tempPR.x         = srcPR.x; 
  tempPR.y         = srcPR.y;
  tempPR.w         = srcPR.w;
  tempPR.h         = srcPR.h;
  tempPR.rowstride = tempPR.bytes * tempPR.w;
  tempPR.data      = g_malloc (tempPR.h * tempPR.rowstride);

  temp_data        = tempPR.data;

  /*  DodgeBurn the region  */
  gimp_lut_process (options->lut, &srcPR, &tempPR);

  /* The dest is the paint area we got above (= canvas_buf) */ 
  destPR.bytes     = area->bytes;
  destPR.x         = 0;
  destPR.y         = 0;
  destPR.w         = area->width;
  destPR.h         = area->height;
  destPR.rowstride = area->width * destPR.bytes;
  destPR.data      = temp_buf_data (area);

  /* Now add an alpha to the dodgeburned region 
     and put this in area = canvas_buf */ 
  if (! gimp_drawable_has_alpha (drawable))
    add_alpha_region (&tempPR, &destPR);
  else
    copy_region (&tempPR, &destPR);

  opacity =
    255 * gimp_context_get_opacity (gimp_get_current_context (gimage->gimp));

  if (pressure_options->opacity)
    opacity = opacity * 2.0 * paint_core->cur_coords.pressure;

  /* Replace the newly dodgedburned area (canvas_buf) to the gimage*/   
  gimp_paint_core_replace_canvas (paint_core, drawable, 
			          MIN (opacity, 255),
		                  OPAQUE_OPACITY, 
			          pressure_options->pressure ? PRESSURE : SOFT,
			          scale, CONSTANT);
 
  g_free (temp_data);
}

static gfloat 
gimp_dodgeburn_highlights_lut_func (gpointer  user_data, 
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
gimp_dodgeburn_midtones_lut_func (gpointer  user_data, 
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
    factor = 1/(1.0 + exposure);

  return pow (value, factor); 
}

static gfloat 
gimp_dodgeburn_shadows_lut_func (gpointer  user_data, 
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
