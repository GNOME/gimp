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

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gimpsmudge.h"


static void       gimp_smudge_class_init (GimpSmudgeClass      *klass);
static void       gimp_smudge_init       (GimpSmudge           *smudge);

static void       gimp_smudge_paint      (GimpPaintCore        *paint_core,
                                          GimpDrawable         *drawable,
                                          PaintOptions         *paint_options,
                                          GimpPaintCoreState    paint_state);

static gboolean   gimp_smudge_start      (GimpPaintCore        *paint_core,
                                          GimpDrawable         *drawable);
static void       gimp_smudge_motion     (GimpPaintCore        *paint_core,
                                          GimpDrawable         *drawable,
                                          PaintPressureOptions *pressure_options,
                                          gdouble               smudge_rate);
static void       gimp_smudge_finish     (GimpPaintCore        *paint_core,
                                          GimpDrawable         *drawable);

static void  gimp_smudge_nonclipped_painthit_coords (GimpPaintCore *paint_core,
                                                     gint          *x,
                                                     gint          *y, 
                                                     gint          *w,
                                                     gint          *h);
static void       gimp_smudge_allocate_accum_buffer (gint           w,
                                                     gint           h, 
                                                     gint           bytes,
                                                     guchar        *do_fill);


static PixelRegion  accumPR;
static guchar      *accum_data = NULL;

static GimpPaintCoreClass *parent_class = NULL;


GType
gimp_smudge_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpSmudgeClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_smudge_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpSmudge),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_smudge_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_CORE,
                                     "GimpSmudge",
                                     &info, 0);
    }

  return type;
}

static void
gimp_smudge_class_init (GimpSmudgeClass *klass)
{
  GimpPaintCoreClass *paint_core_class;

  paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  paint_core_class->paint = gimp_smudge_paint;
}

static void
gimp_smudge_init (GimpSmudge *smudge)
{
  GimpPaintCore *paint_core;

  paint_core = GIMP_PAINT_CORE (smudge);

  paint_core->flags |= CORE_HANDLES_CHANGING_BRUSH;
}

static void
gimp_smudge_paint (GimpPaintCore      *paint_core,
                   GimpDrawable       *drawable,
                   PaintOptions       *paint_options,
                   GimpPaintCoreState  paint_state)
{
  SmudgeOptions *options;

  /* initialization fails if the user starts outside the drawable */
  static gboolean initialized = FALSE;

  options = (SmudgeOptions *) paint_options;

  switch (paint_state)
    {
    case MOTION_PAINT:
      if (! initialized)
	initialized = gimp_smudge_start (paint_core, drawable);

      if (initialized)
	gimp_smudge_motion (paint_core,
                            drawable,
                            paint_options->pressure_options,
                            options->rate);
      break;

    case FINISH_PAINT:
      gimp_smudge_finish (paint_core, drawable);
      initialized = FALSE;
      break;

    default:
      break;
    }

  return;
}

static gboolean
gimp_smudge_start (GimpPaintCore *paint_core,
			GimpDrawable  *drawable)
{
  GimpImage   *gimage;
  TempBuf     *area;
  PixelRegion  srcPR;
  gint         x, y, w, h;
  gint         was_clipped;
  guchar      *do_fill = NULL;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return FALSE;

  /*  If the image type is indexed, don't smudge  */
  if (gimp_drawable_is_indexed (drawable))
    return FALSE;

  area = gimp_paint_core_get_paint_area (paint_core, drawable, 1.0);

  if (!area) 
    return FALSE;
  
  /*  adjust the x and y coordinates to the upper left corner of the brush  */
  gimp_smudge_nonclipped_painthit_coords (paint_core, &x, &y, &w, &h);
  
  if (x != area->x || y != area->y || w != area->width || h != area->height)
    was_clipped = TRUE;
  else 
    was_clipped = FALSE;

  /* When clipped, accum_data may contain pixels that map to
     off-canvas pixels of the under-the-brush image, particulary
     when the brush image contains an edge or corner of the
     image. These off-canvas pixels are not a part of the current
     composite, but may be composited in later generations. do_fill
     contains a copy of the color of the pixel at the center of the
     brush; assumed this is a reasonable choice for off- canvas pixels
     that may enter into the blend */

  if (was_clipped)
    do_fill = gimp_drawable_get_color_at (drawable,
                 CLAMP ((gint) paint_core->cur_coords.x,
                        0, gimp_drawable_width (drawable) - 1),
                 CLAMP ((gint) paint_core->cur_coords.y,
                        0, gimp_drawable_height (drawable) - 1));

  gimp_smudge_allocate_accum_buffer (w, h, 
					  gimp_drawable_bytes (drawable), 
					  do_fill);

  accumPR.x = area->x - x; 
  accumPR.y = area->y - y;
  accumPR.w = area->width;
  accumPR.h = area->height;
  accumPR.rowstride = accumPR.bytes * w; 
  accumPR.data = accum_data 
	+ accumPR.rowstride * accumPR.y 
	+ accumPR.x * accumPR.bytes;

  pixel_region_init (&srcPR, gimp_drawable_data (drawable), 
		     area->x, area->y, area->width, area->height, FALSE);

  /* copy the region under the original painthit. */
  copy_region (&srcPR, &accumPR);

  accumPR.x = area->x - x; 
  accumPR.y = area->y - y;
  accumPR.w = area->width;
  accumPR.h = area->height;
  accumPR.rowstride = accumPR.bytes * w;
  accumPR.data = accum_data
	+ accumPR.rowstride * accumPR.y 
	+ accumPR.x * accumPR.bytes;

  if (do_fill) 
    g_free(do_fill);

  return TRUE;
}

static void
gimp_smudge_motion (GimpPaintCore        *paint_core,
                    GimpDrawable         *drawable,
                    PaintPressureOptions *pressure_options,
                    gdouble               smudge_rate)
{
  GimpImage   *gimage;
  GimpContext *context;
  TempBuf     *area;
  PixelRegion  srcPR, destPR, tempPR;
  gdouble      rate;
  gint         opacity;
  gint         x, y, w, h;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  context = gimp_get_current_context (gimage->gimp);

  /*  If the image type is indexed, don't smudge  */
  if (gimp_drawable_is_indexed (drawable))
    return;

  gimp_smudge_nonclipped_painthit_coords (paint_core, &x, &y, &w, &h);

  /*  Get the paint area */
  /*  Smudge won't scale!  */
  if (! (area = gimp_paint_core_get_paint_area (paint_core, drawable, 1.0)))
    return;

  /* srcPR will be the pixels under the current painthit from 
     the drawable*/

  pixel_region_init (&srcPR, gimp_drawable_data (drawable), 
		     area->x, area->y, area->width, area->height, FALSE);

  /* Enable pressure sensitive rate */
  if (pressure_options->rate)
    rate = MIN (smudge_rate / 100.0 * paint_core->cur_coords.pressure * 2.0, 1.0);
  else
    rate = smudge_rate / 100.0;

  /* The tempPR will be the built up buffer (for smudge) */ 
  tempPR.bytes = accumPR.bytes;
  tempPR.rowstride = accumPR.rowstride;
  tempPR.x = area->x - x; 
  tempPR.y = area->y - y;
  tempPR.w = area->width;
  tempPR.h = area->height;
  tempPR.data = accum_data +
    tempPR.rowstride * tempPR.y + tempPR.x * tempPR.bytes;

  /* The dest will be the paint area we got above (= canvas_buf) */    

  destPR.bytes = area->bytes;                                     
  destPR.x = 0; destPR.y = 0;                                     
  destPR.w = area->width;                                         
  destPR.h = area->height;                                        
  destPR.rowstride = area->width * area->bytes;                  
  destPR.data = temp_buf_data (area); 

  /*  
     Smudge uses the buffer Accum.
     For each successive painthit Accum is built like this
	Accum =  rate*Accum  + (1-rate)*I.
     where I is the pixels under the current painthit. 
     Then the paint area (canvas_buf) is built as 
	(Accum,1) (if no alpha),
  */

  blend_region (&srcPR, &tempPR, &tempPR, ROUND (rate * 255.0));

  /* re-init the tempPR */

  tempPR.bytes = accumPR.bytes;
  tempPR.rowstride = accumPR.rowstride;
  tempPR.x = area->x - x; 
  tempPR.y = area->y - y;
  tempPR.w = area->width;
  tempPR.h = area->height;
  tempPR.data = accum_data 
	+ tempPR.rowstride * tempPR.y 
	+ tempPR.x * tempPR.bytes;

  if (! gimp_drawable_has_alpha (drawable))                             
    add_alpha_region (&tempPR, &destPR);                          
  else                                                            
    copy_region (&tempPR, &destPR);

  opacity = 255 * gimp_context_get_opacity (context);
  if (pressure_options->opacity)
    opacity = opacity * 2.0 * paint_core->cur_coords.pressure;

  /*Replace the newly made paint area to the gimage*/ 
  gimp_paint_core_replace_canvas (paint_core, drawable, 
				  MIN (opacity, 255),
				  OPAQUE_OPACITY, 
				  pressure_options->pressure ? PRESSURE : SOFT,
				  1.0, INCREMENTAL);
}

static void
gimp_smudge_finish (GimpPaintCore *paint_core,
                    GimpDrawable  *drawable)
{
  if (accum_data)
    {
      g_free (accum_data);
      accum_data = NULL;
    }
}

static void 
gimp_smudge_nonclipped_painthit_coords (GimpPaintCore *paint_core,
                                        gint          *x, 
                                        gint          *y, 
                                        gint          *w, 
                                        gint          *h)
{
  /* Note: these are the brush mask size plus a border of 1 pixel */
  *x = (gint) paint_core->cur_coords.x - paint_core->brush->mask->width/2 - 1;
  *y = (gint) paint_core->cur_coords.y - paint_core->brush->mask->height/2 - 1;
  *w = paint_core->brush->mask->width + 2;
  *h = paint_core->brush->mask->height + 2;
}

static void
gimp_smudge_allocate_accum_buffer (gint    w,
                                   gint    h,
                                   gint    bytes,
                                   guchar *do_fill)
{
  /*  Allocate the accumulation buffer */
  accumPR.bytes = bytes;
  accum_data = g_malloc (w * h * bytes);
 
  if (do_fill != NULL)
    {
      /* guchar color[3] = {0,0,0}; */
      accumPR.x = 0; 
      accumPR.y = 0;
      accumPR.w = w;
      accumPR.h = h;
      accumPR.rowstride = accumPR.bytes * w;
      accumPR.data = accum_data;
      color_region (&accumPR, (const guchar*)do_fill);
    }
}
