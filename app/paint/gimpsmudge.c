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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"

#include "gimpsmudge.h"
#include "gimpsmudgeoptions.h"

#include "gimp-intl.h"


static void       gimp_smudge_class_init (GimpSmudgeClass  *klass);
static void       gimp_smudge_init       (GimpSmudge       *smudge);

static void       gimp_smudge_finalize   (GObject          *object);

static void       gimp_smudge_paint      (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
                                          GimpPaintOptions *paint_options,
                                          GimpPaintState    paint_state,
                                          guint32           time);
static gboolean   gimp_smudge_start      (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
                                          GimpPaintOptions *paint_options);
static void       gimp_smudge_motion     (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
                                          GimpPaintOptions *paint_options);

static void  gimp_smudge_nonclipped_painthit_coords (GimpPaintCore *paint_core,
                                                     gint          *x,
                                                     gint          *y,
                                                     gint          *w,
                                                     gint          *h);


static GimpPaintCoreClass *parent_class = NULL;


void
gimp_smudge_register (Gimp                      *gimp,
                      GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_SMUDGE,
                GIMP_TYPE_SMUDGE_OPTIONS,
                _("Smudge"));
}

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

      type = g_type_register_static (GIMP_TYPE_BRUSH_CORE,
                                     "GimpSmudge",
                                     &info, 0);
    }

  return type;
}

static void
gimp_smudge_class_init (GimpSmudgeClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize      = gimp_smudge_finalize;

  paint_core_class->paint     = gimp_smudge_paint;

  brush_core_class->use_scale = FALSE;
}

static void
gimp_smudge_init (GimpSmudge *smudge)
{
  smudge->initialized = FALSE;
  smudge->accum_data  = NULL;
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

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_smudge_paint (GimpPaintCore    *paint_core,
                   GimpDrawable     *drawable,
                   GimpPaintOptions *paint_options,
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
                                                 paint_options);

      if (smudge->initialized)
	gimp_smudge_motion (paint_core, drawable, paint_options);
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

  return;
}

static gboolean
gimp_smudge_start (GimpPaintCore    *paint_core,
                   GimpDrawable     *drawable,
                   GimpPaintOptions *paint_options)
{
  GimpSmudge  *smudge = GIMP_SMUDGE (paint_core);
  GimpImage   *gimage;
  TempBuf     *area;
  PixelRegion  srcPR;
  gint         x, y, w, h;

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  if (gimp_drawable_is_indexed (drawable))
    return FALSE;

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return FALSE;

  /*  adjust the x and y coordinates to the upper left corner of the brush  */
  gimp_smudge_nonclipped_painthit_coords (paint_core, &x, &y, &w, &h);

  /*  Allocate the accumulation buffer */
  smudge->accumPR.bytes = gimp_drawable_bytes (drawable);
  smudge->accum_data    = g_malloc (w * h * smudge->accumPR.bytes);

  /*  If clipped, prefill the smudge buffer
      with the color at the brush position.  */
  if (x != area->x || y != area->y || w != area->width || h != area->height)
    {
      guchar *fill;

      fill = gimp_pickable_get_color_at (GIMP_PICKABLE (drawable),
                                         CLAMP ((gint) paint_core->cur_coords.x,
                                                0, gimp_item_width (GIMP_ITEM (drawable)) - 1),
                                         CLAMP ((gint) paint_core->cur_coords.y,
                                                0, gimp_item_height (GIMP_ITEM (drawable)) - 1));
      g_return_val_if_fail (fill != NULL, FALSE);

      srcPR.w         = w;
      srcPR.h         = h;
      srcPR.bytes     = smudge->accumPR.bytes;
      srcPR.rowstride = srcPR.bytes * w;
      srcPR.data      = smudge->accum_data;

      color_region (&srcPR, fill);
      g_free (fill);
    }

  smudge->accumPR.x         = area->x - x;
  smudge->accumPR.y         = area->y - y;
  smudge->accumPR.w         = area->width;
  smudge->accumPR.h         = area->height;
  smudge->accumPR.rowstride = smudge->accumPR.bytes * w;
  smudge->accumPR.data      = (smudge->accum_data +
                               smudge->accumPR.rowstride * smudge->accumPR.y +
                               smudge->accumPR.x * smudge->accumPR.bytes);

  pixel_region_init (&srcPR, gimp_drawable_data (drawable),
		     area->x, area->y, area->width, area->height, FALSE);

  /* copy the region under the original painthit. */
  copy_region (&srcPR, &smudge->accumPR);

  smudge->accumPR.x         = area->x - x;
  smudge->accumPR.y         = area->y - y;
  smudge->accumPR.w         = area->width;
  smudge->accumPR.h         = area->height;
  smudge->accumPR.rowstride = smudge->accumPR.bytes * w;
  smudge->accumPR.data      = (smudge->accum_data +
                               smudge->accumPR.rowstride * smudge->accumPR.y +
                               smudge->accumPR.x * smudge->accumPR.bytes);

  return TRUE;
}

static void
gimp_smudge_motion (GimpPaintCore    *paint_core,
                    GimpDrawable     *drawable,
                    GimpPaintOptions *paint_options)
{
  GimpSmudge          *smudge           = GIMP_SMUDGE (paint_core);
  GimpSmudgeOptions   *options          = GIMP_SMUDGE_OPTIONS (paint_options);
  GimpContext         *context          = GIMP_CONTEXT (paint_options);
  GimpPressureOptions *pressure_options = paint_options->pressure_options;
  GimpImage           *gimage;
  TempBuf             *area;
  PixelRegion          srcPR, destPR, tempPR;
  gdouble              rate;
  gdouble              opacity;
  gint                 x, y, w, h;

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  if (gimp_drawable_is_indexed (drawable))
    return;

  opacity = gimp_paint_options_get_fade (paint_options, gimage,
                                         paint_core->pixel_dist);
  if (opacity == 0.0)
    return;

  gimp_smudge_nonclipped_painthit_coords (paint_core, &x, &y, &w, &h);

  /*  Get the paint area (Smudge won't scale!)  */
  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return;

  /* srcPR will be the pixels under the current painthit from the drawable */
  pixel_region_init (&srcPR, gimp_drawable_data (drawable),
		     area->x, area->y, area->width, area->height, FALSE);

  /* Enable pressure sensitive rate */
  if (pressure_options->rate)
    rate = MIN (options->rate / 100.0 * PRESSURE_SCALE *
                paint_core->cur_coords.pressure, 1.0);
  else
    rate = options->rate / 100.0;

  /* The tempPR will be the built up buffer (for smudge) */
  tempPR.x         = area->x - x;
  tempPR.y         = area->y - y;
  tempPR.w         = area->width;
  tempPR.h         = area->height;
  tempPR.bytes     = smudge->accumPR.bytes;
  tempPR.rowstride = smudge->accumPR.rowstride;
  tempPR.data      = (smudge->accum_data +
                      tempPR.y * tempPR.rowstride +
                      tempPR.x * tempPR.bytes);

  /* The dest will be the paint area we got above (= canvas_buf) */

  destPR.x         = 0;
  destPR.y         = 0;
  destPR.w         = area->width;
  destPR.h         = area->height;
  destPR.bytes     = area->bytes;
  destPR.rowstride = area->width * area->bytes;
  destPR.data      = temp_buf_data (area);

  /*  Smudge uses the buffer Accum.
   *  For each successive painthit Accum is built like this
   *    Accum =  rate*Accum  + (1-rate)*I.
   *  where I is the pixels under the current painthit.
   *  Then the paint area (canvas_buf) is built as
   *    (Accum,1) (if no alpha),
   */

  blend_region (&srcPR, &tempPR, &tempPR, ROUND (rate * 255.0));

  /* re-init the tempPR */

  tempPR.x         = area->x - x;
  tempPR.y         = area->y - y;
  tempPR.w         = area->width;
  tempPR.h         = area->height;
  tempPR.bytes     = smudge->accumPR.bytes;
  tempPR.rowstride = smudge->accumPR.rowstride;
  tempPR.data      = (smudge->accum_data +
                      tempPR.y * tempPR.rowstride +
                      tempPR.x * tempPR.bytes);

  if (! gimp_drawable_has_alpha (drawable))
    add_alpha_region (&tempPR, &destPR);
  else
    copy_region (&tempPR, &destPR);

  if (pressure_options->opacity)
    opacity *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
				  MIN (opacity, GIMP_OPACITY_OPAQUE),
				  gimp_context_get_opacity (context),
				  gimp_paint_options_get_brush_mode (paint_options),
                                  GIMP_PAINT_INCREMENTAL);
}

static void
gimp_smudge_nonclipped_painthit_coords (GimpPaintCore *paint_core,
                                        gint          *x,
                                        gint          *y,
                                        gint          *w,
                                        gint          *h)
{
  GimpBrushCore *brush_core = GIMP_BRUSH_CORE (paint_core);

  /* Note: these are the brush mask size plus a border of 1 pixel */
  *x = (gint) paint_core->cur_coords.x - brush_core->brush->mask->width  / 2 - 1;
  *y = (gint) paint_core->cur_coords.y - brush_core->brush->mask->height / 2 - 1;
  *w = brush_core->brush->mask->width  + 2;
  *h = brush_core->brush->mask->height + 2;
}
