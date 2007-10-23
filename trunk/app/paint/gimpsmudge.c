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


static void       gimp_smudge_finalize     (GObject          *object);

static void       gimp_smudge_paint        (GimpPaintCore    *paint_core,
                                            GimpDrawable     *drawable,
                                            GimpPaintOptions *paint_options,
                                            GimpPaintState    paint_state,
                                            guint32           time);
static gboolean   gimp_smudge_start        (GimpPaintCore    *paint_core,
                                            GimpDrawable     *drawable,
                                            GimpPaintOptions *paint_options);
static void       gimp_smudge_motion       (GimpPaintCore    *paint_core,
                                            GimpDrawable     *drawable,
                                            GimpPaintOptions *paint_options);

static void       gimp_smudge_brush_coords (GimpPaintCore    *paint_core,
                                            gint             *x,
                                            gint             *y,
                                            gint             *w,
                                            gint             *h);


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
}

static gboolean
gimp_smudge_start (GimpPaintCore    *paint_core,
                   GimpDrawable     *drawable,
                   GimpPaintOptions *paint_options)
{
  GimpSmudge  *smudge = GIMP_SMUDGE (paint_core);
  GimpImage   *image;
  TempBuf     *area;
  PixelRegion  srcPR;
  gint         bytes;
  gint         x, y, w, h;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (gimp_drawable_is_indexed (drawable))
    return FALSE;

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return FALSE;

  /*  adjust the x and y coordinates to the upper left corner of the brush  */
  gimp_smudge_brush_coords (paint_core, &x, &y, &w, &h);

  /*  Allocate the accumulation buffer */
  bytes = gimp_drawable_bytes (drawable);
  smudge->accum_data = g_malloc (w * h * bytes);

  /*  If clipped, prefill the smudge buffer with the color at the
   *  brush position.
   */
  if (x != area->x || y != area->y || w != area->width || h != area->height)
    {
      guchar fill[4];

      gimp_pickable_get_pixel_at (GIMP_PICKABLE (drawable),
                                  CLAMP ((gint) paint_core->cur_coords.x,
                                         0,
                                         gimp_item_width (GIMP_ITEM (drawable)) - 1),
                                  CLAMP ((gint) paint_core->cur_coords.y,
                                         0,
                                         gimp_item_height (GIMP_ITEM (drawable)) - 1),
                                  fill);

      pixel_region_init_data (&srcPR, smudge->accum_data,
                              bytes, bytes * w,
                              0, 0, w, h);

      color_region (&srcPR, fill);
    }

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     area->x, area->y, area->width, area->height, FALSE);

  pixel_region_init_data (&smudge->accumPR, smudge->accum_data,
                          bytes, bytes * w,
                          area->x - x,
                          area->y - y,
                          area->width,
                          area->height);

  /* copy the region under the original painthit. */
  copy_region (&srcPR, &smudge->accumPR);

  pixel_region_init_data (&smudge->accumPR, smudge->accum_data,
                          bytes, bytes * w,
                          area->x - x,
                          area->y - y,
                          area->width,
                          area->height);

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
  GimpImage           *image;
  TempBuf             *area;
  PixelRegion          srcPR, destPR, tempPR;
  gdouble              rate;
  gdouble              opacity;
  gint                 x, y, w, h;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (gimp_drawable_is_indexed (drawable))
    return;

  opacity = gimp_paint_options_get_fade (paint_options, image,
                                         paint_core->pixel_dist);
  if (opacity == 0.0)
    return;

  /*  Get the unclipped brush coordinates  */
  gimp_smudge_brush_coords (paint_core, &x, &y, &w, &h);

  /*  Get the paint area (Smudge won't scale!)  */
  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return;

  /* srcPR will be the pixels under the current painthit from the drawable */
  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     area->x, area->y, area->width, area->height, FALSE);

  /* Enable pressure sensitive rate */
  if (pressure_options->rate)
    rate = MIN (options->rate / 100.0 * PRESSURE_SCALE *
                paint_core->cur_coords.pressure, 1.0);
  else
    rate = options->rate / 100.0;

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

  if (pressure_options->opacity)
    opacity *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  GIMP_PAINT_INCREMENTAL);
}

static void
gimp_smudge_brush_coords (GimpPaintCore *paint_core,
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
