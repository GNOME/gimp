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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "base/brush-scale.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrushpipe.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimpmarshal.h"

#include "gimppaintcore.h"
#include "gimppaintcore-kernels.h"

#include "app_procs.h"
#include "gimprc.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


#define EPSILON  0.00001


/*  local function prototypes  */

static void   gimp_paint_core_class_init          (GimpPaintCoreClass *klass);
static void   gimp_paint_core_init                (GimpPaintCore      *core);

static void   gimp_paint_core_finalize            (GObject          *object);

static void   gimp_paint_core_calc_brush_size     (MaskBuf          *mask,
                                                   gdouble           scale,
                                                   gint             *width,
                                                   gint             *height);
static MaskBuf * gimp_paint_core_subsample_mask   (GimpPaintCore    *core,
                                                   MaskBuf          *mask,
                                                   gdouble           x,
                                                   gdouble           y);
static MaskBuf * gimp_paint_core_pressurize_mask  (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask,
                                                   gdouble           x,
                                                   gdouble           y,
                                                   gdouble           pressure);
static MaskBuf * gimp_paint_core_solidify_mask    (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask);
static MaskBuf * gimp_paint_core_scale_mask       (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask,
                                                   gdouble           scale);
static MaskBuf * gimp_paint_core_scale_pixmap     (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask,
                                                   gdouble           scale);

static MaskBuf * gimp_paint_core_get_brush_mask   (GimpPaintCore    *core,
                                                   BrushApplicationMode  brush_hardness,
                                                   gdouble           scale);
static void      gimp_paint_core_paste            (GimpPaintCore    *core,
                                                   MaskBuf	    *brush_mask,
                                                   GimpDrawable	    *drawable,
                                                   gint		     brush_opacity,
                                                   gint		     image_opacity,
                                                   GimpLayerModeEffects  paint_mode,
                                                   PaintApplicationMode  mode);
static void      gimp_paint_core_replace          (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask,
                                                   GimpDrawable	    *drawable,
                                                   gint		     brush_opacity,
                                                   gint              image_opacity,
                                                   PaintApplicationMode  mode);

static void      brush_to_canvas_tiles            (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask,
                                                   gint              brush_opacity);
static void      brush_to_canvas_buf              (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask,
                                                   gint              brush_opacity);
static void      canvas_tiles_to_canvas_buf       (GimpPaintCore    *core);

static void      set_undo_tiles                   (GimpPaintCore    *core,
                                                   GimpDrawable     *drawable,
                                                   gint              x,
                                                   gint              y,
                                                   gint              w,
                                                   gint              h);
static void      set_canvas_tiles                 (GimpPaintCore    *core,
                                                   gint              x,
                                                   gint              y,
                                                   gint              w,
                                                   gint              h);
static void      gimp_paint_core_invalidate_cache (GimpBrush        *brush,
                                                   GimpPaintCore    *core);


/*  brush pipe utility functions  */
static void      paint_line_pixmap_mask           (GimpImage        *dest,
                                                   GimpDrawable     *drawable,
                                                   TempBuf          *pixmap_mask,
                                                   TempBuf          *brush_mask,
                                                   guchar           *d,
                                                   gint              x,
                                                   gint              y,
                                                   gint              bytes,
                                                   gint              width,
                                                   BrushApplicationMode  mode);


static GimpObjectClass *parent_class = NULL;

static gint global_core_ID = 0;


GType
gimp_paint_core_get_type (void)
{
  static GType core_type = 0;

  if (! core_type)
    {
      static const GTypeInfo core_info =
      {
        sizeof (GimpPaintCoreClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_paint_core_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPaintCore),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_paint_core_init,
      };

      core_type = g_type_register_static (GIMP_TYPE_OBJECT,
					  "GimpPaintCore", 
                                          &core_info, 0);
    }

  return core_type;
}

static void
gimp_paint_core_class_init (GimpPaintCoreClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_paint_core_finalize;

  klass->paint           = NULL;
}

static void
gimp_paint_core_init (GimpPaintCore *core)
{
  gint i, j;

  core->ID             = global_core_ID++;

  core->distance       = 0.0;
  core->spacing        = 0.0;
  core->x1             = 0;
  core->y1             = 0;
  core->x2             = 0;
  core->y2             = 0;

  core->brush          = NULL;

  core->flags          = 0;

  core->undo_tiles     = NULL;
  core->canvas_tiles   = NULL;

  core->orig_buf       = NULL;
  core->canvas_buf     = NULL;

  core->pressure_brush = NULL;
  core->solid_brush    = NULL;
  core->scale_brush    = NULL;
  core->scale_pixmap   = NULL;

  g_assert (PAINT_CORE_SUBSAMPLE == KERNEL_SUBSAMPLE);

  for (i = 0; i < KERNEL_SUBSAMPLE + 1; i++)
    for (j = 0; j < KERNEL_SUBSAMPLE + 1; j++)
      core->kernel_brushes[i][j] = NULL;

  core->last_brush_mask = NULL;
  core->cache_invalid   = FALSE;

  core->grr_brush       = NULL;
}

static void
gimp_paint_core_finalize (GObject *object)
{
  GimpPaintCore *core;
  gint           i, j;

  core = GIMP_PAINT_CORE (object);

  gimp_paint_core_cleanup (core);

  if (core->pressure_brush)
    {
      temp_buf_free (core->pressure_brush);
      core->pressure_brush = NULL;
    }

  if (core->solid_brush)
    {
      temp_buf_free (core->solid_brush);
      core->solid_brush = NULL;
    }

  if (core->scale_brush)
    {
      temp_buf_free (core->scale_brush);
      core->scale_brush = NULL;
    }

  if (core->scale_pixmap)
    {
      temp_buf_free (core->scale_pixmap);
      core->scale_pixmap = NULL;
    }

  g_assert (PAINT_CORE_SUBSAMPLE == KERNEL_SUBSAMPLE);

  for (i = 0; i < KERNEL_SUBSAMPLE + 1; i++)
    for (j = 0; j < KERNEL_SUBSAMPLE + 1; j++)
      if (core->kernel_brushes[i][j])
        {
          temp_buf_free (core->kernel_brushes[i][j]);
          core->kernel_brushes[i][j] = NULL;
        }

  if (core->grr_brush)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (core->grr_brush),
                                            gimp_paint_core_invalidate_cache,
                                            core);
      g_object_unref (G_OBJECT (core->grr_brush));
      core->grr_brush = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
gimp_paint_core_paint (GimpPaintCore      *core,
		       GimpDrawable       *drawable,
                       PaintOptions       *paint_options,
		       GimpPaintCoreState  paint_state)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (paint_options != NULL);

  GIMP_PAINT_CORE_GET_CLASS (core)->paint (core,
                                           drawable,
                                           paint_options,
                                           paint_state);
}

gboolean
gimp_paint_core_start (GimpPaintCore *core,
		       GimpDrawable  *drawable,
                       GimpCoords    *coords)
{
  GimpContext *context;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  context = gimp_get_current_context (gimp_drawable_gimage (drawable)->gimp);

  core->cur_coords = *coords;

  /*  Each buffer is the same size as
   *  the maximum bounds of the active brush...
   */

  if (core->grr_brush &&
      core->grr_brush != gimp_context_get_brush (context))
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (core->grr_brush),
                                            gimp_paint_core_invalidate_cache,
                                            core);
      g_object_unref (G_OBJECT (core->grr_brush));
    }

  core->grr_brush = gimp_context_get_brush (context);

  if (! core->grr_brush)
    {
      g_message (_("No brushes available for use with this tool."));
      return FALSE;
    }

  g_object_ref (G_OBJECT (core->grr_brush));
  g_signal_connect (G_OBJECT (core->grr_brush), "invalidate_preview",
                    G_CALLBACK (gimp_paint_core_invalidate_cache),
                    core);

  core->spacing = (gdouble) gimp_brush_get_spacing (core->grr_brush) / 100.0;

  core->brush = core->grr_brush;

  /*  Allocate the undo structure  */
  if (core->undo_tiles)
    tile_manager_destroy (core->undo_tiles);

  core->undo_tiles = tile_manager_new (gimp_drawable_width (drawable),
                                       gimp_drawable_height (drawable),
                                       gimp_drawable_bytes (drawable));

  /*  Allocate the canvas blocks structure  */
  if (core->canvas_tiles)
    tile_manager_destroy (core->canvas_tiles);

  core->canvas_tiles = tile_manager_new (gimp_drawable_width (drawable),
                                         gimp_drawable_height (drawable),
                                         1);

  /*  Get the initial undo extents  */
  core->x1         = core->x2 = core->cur_coords.x;
  core->y1         = core->y2 = core->cur_coords.y;
  core->distance   = 0.0;
  core->pixel_dist = 0.0;

  return TRUE;
}

void
gimp_paint_core_interpolate (GimpPaintCore *core,
			     GimpDrawable  *drawable,
                             PaintOptions  *paint_options)
{
  GimpCoords  delta;
  /*   double spacing; */
  /*   double lastscale, curscale; */
  gdouble     n;
  gdouble     left;
  gdouble     t;
  gdouble     initial;
  gdouble     dist;
  gdouble     total;
  gdouble     pixel_dist;
  gdouble     pixel_initial;
  gdouble     xd, yd;
  gdouble     mag;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (paint_options != NULL);

  delta.x        = core->cur_coords.x        - core->last_coords.x;
  delta.y        = core->cur_coords.y        - core->last_coords.y;
  delta.pressure = core->cur_coords.pressure - core->last_coords.pressure;
  delta.xtilt    = core->cur_coords.xtilt    - core->last_coords.xtilt;
  delta.ytilt    = core->cur_coords.ytilt    - core->last_coords.ytilt;
  delta.wheel    = core->cur_coords.wheel    - core->last_coords.wheel;

  /* return if there has been no motion */
  if (! delta.x &&
      ! delta.y &&
      ! delta.pressure &&
      ! delta.xtilt &&
      ! delta.ytilt &&
      ! delta.wheel)
    return;

  /* calculate the distance traveled in the coordinate space of the brush */
  mag = gimp_vector2_length (&(core->brush->x_axis));
  xd  = gimp_vector2_inner_product ((GimpVector2 *) &delta,
				    &(core->brush->x_axis)) / (mag * mag);

  mag = gimp_vector2_length (&(core->brush->y_axis));
  yd  = gimp_vector2_inner_product ((GimpVector2 *) &delta,
				    &(core->brush->y_axis)) / (mag * mag);

  dist    = 0.5 * sqrt (xd * xd + yd * yd);
  total   = dist + core->distance;
  initial = core->distance;

  pixel_dist    = gimp_vector2_length ((GimpVector2 *) &delta);
  pixel_initial = core->pixel_dist;

  /*  FIXME: need to adapt the spacing to the size  */
  /*   lastscale = MIN (gimp_paint_tool->lastpressure, 1/256); */
  /*   curscale = MIN (gimp_paint_tool->curpressure,  1/256); */
  /*   spacing = gimp_paint_tool->spacing * sqrt (0.5 * (lastscale + curscale)); */

  while (core->distance < total)
    {
      GimpBrush *current_brush;

      n    = (gint) (core->distance / core->spacing + 1.0 + EPSILON);
      left = n * core->spacing - core->distance;

      core->distance += left;

      if (core->distance <= (total + EPSILON))
	{
	  t = (core->distance - initial) / dist;

	  core->cur_coords.x        = (core->last_coords.x +
                                       delta.x * t);
	  core->cur_coords.y        = (core->last_coords.y +
                                       delta.y * t);
	  core->cur_coords.pressure = (core->last_coords.pressure +
                                       delta.pressure * t);
	  core->cur_coords.xtilt    = (core->last_coords.xtilt +
                                       delta.xtilt * t);
	  core->cur_coords.ytilt    = (core->last_coords.ytilt +
                                       delta.ytilt * t);
	  core->cur_coords.wheel    = (core->last_coords.wheel +
                                       delta.ytilt * t);

	  core->pixel_dist = pixel_initial + pixel_dist * t;

	  /*  save the current brush  */
	  current_brush = core->brush;

	  if (core->flags & CORE_CAN_HANDLE_CHANGING_BRUSH)
	    {
	      core->brush = gimp_brush_select_brush (core->brush,
                                                     &core->last_coords,
                                                     &core->cur_coords);
	    }

	  gimp_paint_core_paint (core, drawable, paint_options, MOTION_PAINT);

	  /*  restore the current brush pointer  */
          core->brush = current_brush;
	}
    }

  core->cur_coords.x        = core->last_coords.x        + delta.x;
  core->cur_coords.y        = core->last_coords.y        + delta.y;
  core->cur_coords.pressure = core->last_coords.pressure + delta.pressure;
  core->cur_coords.xtilt    = core->last_coords.xtilt    + delta.xtilt;
  core->cur_coords.ytilt    = core->last_coords.ytilt    + delta.ytilt;
  core->cur_coords.wheel    = core->last_coords.wheel    + delta.wheel;

  core->distance   = total;
  core->pixel_dist = pixel_initial + pixel_dist;
}

void
gimp_paint_core_finish (GimpPaintCore *core,
			GimpDrawable  *drawable)
{
  GimpImage         *gimage;
  GimpPaintCoreUndo *pu;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */

  if ((core->x2 == core->x1) ||
      (core->y2 == core->y1))
    return;

  undo_push_group_start (gimage, PAINT_CORE_UNDO);

  pu = g_new0 (GimpPaintCoreUndo, 1);
  pu->core_ID      = core->ID;
  pu->core_type    = G_TYPE_FROM_INSTANCE (core);
  pu->last_coords  = core->start_coords;

  /*  Push a paint undo  */
  undo_push_paint (gimage, pu);

  /*  push an undo  */
  gimp_drawable_apply_image (drawable,
                             core->x1, core->y1,
			     core->x2, core->y2,
                             core->undo_tiles,
                             TRUE);
  core->undo_tiles = NULL;

  /*  push the group end  */
  undo_push_group_end (gimage);

  /*  invalidate the drawable--have to do it here, because
   *  it is not done during the actual painting.
   */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (drawable));
}

void
gimp_paint_core_cleanup (GimpPaintCore *core)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));

  if (core->undo_tiles)
    {
      tile_manager_destroy (core->undo_tiles);
      core->undo_tiles = NULL;
    }

  if (core->canvas_tiles)
    {
      tile_manager_destroy (core->canvas_tiles);
      core->canvas_tiles = NULL;
    }

  if (core->orig_buf)
    {
      temp_buf_free (core->orig_buf);
      core->orig_buf = NULL;
    }

  if (core->canvas_buf)
    {
      temp_buf_free (core->canvas_buf);
      core->canvas_buf = NULL;
    }
}

void
gimp_paint_core_get_color_from_gradient (GimpPaintCore     *core,
                                         GimpGradient      *gradient,
					 gdouble            gradient_length,
					 GimpRGB           *color,
					 GradientPaintMode  mode)
{
  gdouble distance;  /* distance in current brush stroke */
  gdouble y;

  distance = core->pixel_dist;
  y        = (gdouble) distance / gradient_length;

  /* for the once modes, set y close to 1.0 after the first chunk */
  if ((mode == ONCE_FORWARD || mode == ONCE_BACKWARDS) && y >= 1.0)
    y = 0.9999999;

  if ((((gint) y & 1) && mode != LOOP_SAWTOOTH) || mode == ONCE_BACKWARDS )
    y = 1.0 - (y - (gint) y);
  else
    y = y - (gint) y;

  gimp_gradient_get_color_at (gradient, y, color);
}


/************************/
/*  Painting functions  */
/************************/

TempBuf *
gimp_paint_core_get_paint_area (GimpPaintCore *core,
				GimpDrawable  *drawable,
				gdouble        scale)
{
  gint x, y;
  gint x1, y1, x2, y2;
  gint bytes;
  gint dwidth, dheight;
  gint bwidth, bheight;

  bytes = (gimp_drawable_has_alpha (drawable) ?
           gimp_drawable_bytes (drawable) : gimp_drawable_bytes (drawable) + 1);

  gimp_paint_core_calc_brush_size (core->brush->mask,
                                   scale,
                                   &bwidth, &bheight);

  /*  adjust the x and y coordinates to the upper left corner of the brush  */
  x = (gint) floor (core->cur_coords.x) - (bwidth  >> 1);
  y = (gint) floor (core->cur_coords.y) - (bheight >> 1);

  dwidth  = gimp_drawable_width  (drawable);
  dheight = gimp_drawable_height (drawable);

  x1 = CLAMP (x - 1, 0, dwidth);
  y1 = CLAMP (y - 1, 0, dheight);
  x2 = CLAMP (x + bwidth + 1, 0, dwidth);
  y2 = CLAMP (y + bheight + 1, 0, dheight);

  /*  configure the canvas buffer  */
  if ((x2 - x1) && (y2 - y1))
    core->canvas_buf = temp_buf_resize (core->canvas_buf, bytes,
                                        x1, y1,
                                        (x2 - x1), (y2 - y1));
  else
    return NULL;

  return core->canvas_buf;
}

TempBuf *
gimp_paint_core_get_orig_image (GimpPaintCore *core,
				GimpDrawable  *drawable,
				gint           x1,
				gint           y1,
				gint           x2,
				gint           y2)
{
  PixelRegion  srcPR;
  PixelRegion  destPR;
  Tile        *undo_tile;
  gint         h;
  gint         refd;
  gint         pixelwidth;
  gint         dwidth;
  gint         dheight;
  guchar      *s;
  guchar      *d;
  gpointer     pr;

  core->orig_buf = temp_buf_resize (core->orig_buf,
                                    gimp_drawable_bytes (drawable),
                                    x1, y1,
                                    (x2 - x1), (y2 - y1));

  dwidth  = gimp_drawable_width  (drawable);
  dheight = gimp_drawable_height (drawable);

  x1 = CLAMP (x1, 0, dwidth);
  y1 = CLAMP (y1, 0, dheight);
  x2 = CLAMP (x2, 0, dwidth);
  y2 = CLAMP (y2, 0, dheight);

  /*  configure the pixel regions  */
  pixel_region_init (&srcPR, gimp_drawable_data (drawable),
                     x1, y1,
		     (x2 - x1), (y2 - y1),
                     FALSE);

  destPR.bytes     = core->orig_buf->bytes;
  destPR.x         = 0;
  destPR.y         = 0;
  destPR.w         = (x2 - x1);
  destPR.h         = (y2 - y1);
  destPR.rowstride = core->orig_buf->bytes * core->orig_buf->width;
  destPR.data      = (temp_buf_data (core->orig_buf) +
                      (y1 - core->orig_buf->y) * destPR.rowstride +
                      (x1 - core->orig_buf->x) * destPR.bytes);

  for (pr = pixel_regions_register (2, &srcPR, &destPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      /*  If the undo tile corresponding to this location is valid, use it  */
      undo_tile = tile_manager_get_tile (core->undo_tiles,
                                         srcPR.x, srcPR.y,
					 FALSE, FALSE);
      if (tile_is_valid (undo_tile) == TRUE)
	{
	  refd = 1;
	  undo_tile = tile_manager_get_tile (core->undo_tiles,
                                             srcPR.x, srcPR.y,
					     TRUE, FALSE);
	  s = (unsigned char*)tile_data_pointer (undo_tile, 0, 0) +
	    srcPR.rowstride * (srcPR.y % TILE_HEIGHT) +
	    srcPR.bytes * (srcPR.x % TILE_WIDTH); /* dubious... */
	}
      else
	{
	  refd = 0;
	  s = srcPR.data;
	}

      d = destPR.data;
      pixelwidth = srcPR.w * srcPR.bytes;
      h = srcPR.h;
      while (h --)
	{
	  memcpy (d, s, pixelwidth);
	  s += srcPR.rowstride;
	  d += destPR.rowstride;
	}

      if (refd)
	tile_release (undo_tile, FALSE);
    }

  return core->orig_buf;
}

void
gimp_paint_core_paste_canvas (GimpPaintCore        *core,
			      GimpDrawable	   *drawable,
			      gint		    brush_opacity,
			      gint		    image_opacity,
			      GimpLayerModeEffects  paint_mode,
			      BrushApplicationMode  brush_hardness,
			      gdouble               brush_scale,
			      PaintApplicationMode  mode)
{
  MaskBuf *brush_mask;

  /*  get the brush mask  */
  brush_mask = gimp_paint_core_get_brush_mask (core,
					       brush_hardness,
                                               brush_scale);

  /*  paste the canvas buf  */
  gimp_paint_core_paste (core, brush_mask, drawable,
			 brush_opacity, image_opacity, paint_mode, mode);
}

/* Similar to gimp_paint_core_paste_canvas, but replaces the alpha channel
 * rather than using it to composite (i.e. transparent over opaque
 * becomes transparent rather than opauqe.
 */
void
gimp_paint_core_replace_canvas (GimpPaintCore        *core,
				GimpDrawable	     *drawable,
				gint                  brush_opacity,
				gint                  image_opacity,
				BrushApplicationMode  brush_hardness,
				gdouble               brush_scale,
				PaintApplicationMode  mode)
{
  MaskBuf *brush_mask;

  /*  get the brush mask  */
  brush_mask = gimp_paint_core_get_brush_mask (core,
                                               brush_hardness,
                                               brush_scale);

  /*  paste the canvas buf  */
  gimp_paint_core_replace (core, brush_mask, drawable,
			   brush_opacity, image_opacity, mode);
}


static void
gimp_paint_core_invalidate_cache (GimpBrush     *brush,
				  GimpPaintCore *core)
{
  /* Make sure we don't cache data for a brush that has changed */

  if (core->last_brush_mask == brush->mask)
    core->cache_invalid = TRUE;
}

/************************************************************
 *             LOCAL FUNCTION DEFINITIONS                   *
 ************************************************************/

static void
gimp_paint_core_calc_brush_size (MaskBuf *mask,
                                 gdouble  scale,
                                 gint    *width,
                                 gint    *height)
{
  scale = CLAMP (scale, 0.0, 1.0);

  if (gimp_devices_get_current (the_gimp) == gdk_device_get_core_pointer ())
    {
      *width  = mask->width;
      *height = mask->height;
    }
  else
    {
      gdouble ratio;

      if (scale < 1 / 256)
	ratio = 1 / 16;
      else
	ratio = sqrt (scale);

      *width  = MAX ((gint) (mask->width  * ratio + 0.5), 1);
      *height = MAX ((gint) (mask->height * ratio + 0.5), 1);
    }
}

static MaskBuf *
gimp_paint_core_subsample_mask (GimpPaintCore *core,
                                MaskBuf       *mask,
				gdouble        x,
				gdouble        y)
{
  MaskBuf    *dest;
  gdouble     left;
  guchar     *m;
  guchar     *d;
  const gint *k;
  gint        index1;
  gint        index2;
  const gint *kernel;
  gint        new_val;
  gint        i, j;
  gint        r, s;

  x += (x < 0) ? mask->width : 0;
  left = x - floor (x);
  index1 = (gint) (left * (gdouble) (KERNEL_SUBSAMPLE + 1));

  y += (y < 0) ? mask->height : 0;
  left = y - floor (y);
  index2 = (gint) (left * (gdouble) (KERNEL_SUBSAMPLE + 1));

  kernel = subsample[index2][index1];

  if (mask == core->last_brush_mask && ! core->cache_invalid)
    {
      if (core->kernel_brushes[index2][index1])
	return core->kernel_brushes[index2][index1];
    }
  else
    {
      for (i = 0; i <= KERNEL_SUBSAMPLE; i++)
	for (j = 0; j <= KERNEL_SUBSAMPLE; j++)
	  {
	    if (core->kernel_brushes[i][j])
              {
                mask_buf_free (core->kernel_brushes[i][j]);
                core->kernel_brushes[i][j] = NULL;
              }
	  }

      core->last_brush_mask = mask;
      core->cache_invalid   = FALSE;
    }

  dest = mask_buf_new (mask->width  + 2,
                       mask->height + 2);

  core->kernel_brushes[index2][index1] = dest;

  m = mask_buf_data (mask);
  for (i = 0; i < mask->height; i++)
    {
      for (j = 0; j < mask->width; j++)
	{
	  k = kernel;
	  for (r = 0; r < KERNEL_HEIGHT; r++)
	    {
	      d = mask_buf_data (dest) + (i+r) * dest->width + j;
	      s = KERNEL_WIDTH;
	      while (s--)
		{
		  new_val = *d + ((*m * *k++ + 128) >> 8);
		  *d++ = MIN (new_val, 255);
		}
	    }
	  m++;
	}
    }

  return dest;
}

/* #define FANCY_PRESSURE */

static MaskBuf *
gimp_paint_core_pressurize_mask (GimpPaintCore *core,
                                 MaskBuf       *brush_mask,
				 gdouble        x,
				 gdouble        y,
				 gdouble        pressure)
{
  static guchar   mapi[256];
  guchar         *source;
  guchar         *dest;
  MaskBuf        *subsample_mask;
  gint            i;
#ifdef FANCY_PRESSURE
  static gdouble  map[256];
  gdouble         ds, s, c;
#endif

  /* Get the raw subsampled mask */
  subsample_mask = gimp_paint_core_subsample_mask (core,
                                                   brush_mask,
                                                   x, y);

  /* Special case pressure = 0.5 */
  if ((int)(pressure * 100 + 0.5) == 50)
    return subsample_mask;

  if (core->pressure_brush)
    mask_buf_free (core->pressure_brush);

  core->pressure_brush = mask_buf_new (brush_mask->width  + 2,
                                       brush_mask->height + 2);

#ifdef FANCY_PRESSURE
  /* Create the pressure profile

     It is: I'(I) = tanh(20*(pressure-0.5)*I) : pressure > 0.5
            I'(I) = 1 - tanh(20*(0.5-pressure)*(1-I)) : pressure < 0.5

     It looks like:

        low pressure      medium pressure     high pressure

             |                   /                 --
             |    		/                 /
            /     	       /                 |
          --                  /	                 |

  */

  ds = (pressure - 0.5)*(20./256.);
  s = 0;
  c = 1.0;

  if (ds > 0)
    {
      for (i=0;i<256;i++)
	{
	  map[i] = s/c;
	  s += c*ds;
	  c += s*ds;
	}
      for (i=0;i<256;i++)
	mapi[i] = (int)(255*map[i]/map[255]);
    }
  else
    {
      ds = -ds;
      for (i=255;i>=0;i--)
	{
	  map[i] = s/c;
	  s += c*ds;
	  c += s*ds;
	}
      for (i=0;i<256;i++)
	mapi[i] = (int)(255*(1-map[i]/map[0]));
    }
#else /* ! FANCY_PRESSURE */

  for (i = 0; i < 256; i++)
    {
      gint tmp = (pressure / 0.5) * i;

      if (tmp > 255)
	mapi[i] = 255;
      else
	mapi[i] = tmp;
    }

#endif /* FANCY_PRESSURE */

  /* Now convert the brush */

  source = mask_buf_data (subsample_mask);
  dest   = mask_buf_data (core->pressure_brush);

  i = subsample_mask->width * subsample_mask->height;
  while (i--)
    {
      *dest++ = mapi[(*source++)];
    }

  return core->pressure_brush;
}

static MaskBuf *
gimp_paint_core_solidify_mask (GimpPaintCore *core,
                               MaskBuf       *brush_mask)
{
  gint    i;
  gint    j;
  guchar *data;
  guchar *src;

  if (brush_mask == core->last_solid_brush &&
      core->solid_brush                    &&
      ! core->cache_invalid)
    {
      return core->solid_brush;
    }

  core->last_solid_brush = brush_mask;

  if (core->solid_brush)
    mask_buf_free (core->solid_brush);

  core->solid_brush = mask_buf_new (brush_mask->width  + 2,
                                    brush_mask->height + 2);

  /*  get the data and advance one line into it  */
  data = (mask_buf_data (core->solid_brush) +
          core->solid_brush->width);
  src   = mask_buf_data (brush_mask);

  for (i = 0; i < brush_mask->height; i++)
    {
      data++;
      for (j = 0; j < brush_mask->width; j++)
	{
	  *data++ = (*src++) ? OPAQUE_OPACITY : TRANSPARENT_OPACITY;
	}
      data++;
    }

  return core->solid_brush;
}

static MaskBuf *
gimp_paint_core_scale_mask (GimpPaintCore *core,
                            MaskBuf       *brush_mask,
			    gdouble        scale)
{
  gint dest_width;
  gint dest_height;

  scale = CLAMP (scale, 0.0, 1.0);

  if (scale == 0.0)
    return NULL;

  if (scale == 1.0)
    return brush_mask;

  gimp_paint_core_calc_brush_size (brush_mask, scale,
                                   &dest_width, &dest_height);

  if (brush_mask == core->last_scale_brush  &&
      core->scale_brush                     &&
      ! core->cache_invalid                 &&
      dest_width  == core->last_scale_width &&
      dest_height == core->last_scale_height)
    {
      return core->scale_brush;
    }

  core->last_scale_brush  = brush_mask;
  core->last_scale_width  = dest_width;
  core->last_scale_height = dest_height;

  if (core->scale_brush)
    mask_buf_free (core->scale_brush);

  core->scale_brush = brush_scale_mask (brush_mask,
                                        dest_width, dest_height);
  core->cache_invalid = TRUE;

  return core->scale_brush;
}

static MaskBuf *
gimp_paint_core_scale_pixmap (GimpPaintCore *core,
                              MaskBuf       *brush_mask,
			      gdouble        scale)
{
  gint dest_width;
  gint dest_height;

  scale = CLAMP (scale, 0.0, 1.0);

  if (scale == 0.0)
    return NULL;

  if (scale == 1.0)
    return brush_mask;

  gimp_paint_core_calc_brush_size (brush_mask, scale,
                                   &dest_width, &dest_height);

  if (brush_mask == core->last_scale_pixmap        &&
      core->scale_pixmap                           &&
      ! core->cache_invalid                        &&
      dest_width  == core->last_scale_pixmap_width &&
      dest_height == core->last_scale_pixmap_height)
    {
      return core->scale_pixmap;
    }

  core->last_scale_pixmap        = brush_mask;
  core->last_scale_pixmap_width  = dest_width;
  core->last_scale_pixmap_height = dest_height;

  if (core->scale_pixmap)
    mask_buf_free (core->scale_pixmap);

  core->scale_pixmap = brush_scale_pixmap (brush_mask,
                                           dest_width, dest_height);
  core->cache_invalid = TRUE;

  return core->scale_pixmap;
}

static MaskBuf *
gimp_paint_core_get_brush_mask (GimpPaintCore        *core,
				BrushApplicationMode  brush_hardness,
				gdouble               scale)
{
  MaskBuf *mask;

  if (gimp_devices_get_current (the_gimp) == gdk_device_get_core_pointer ())
    {
      mask = core->brush->mask;
    }
  else
    {
      mask = gimp_paint_core_scale_mask (core, core->brush->mask,
                                         scale);
    }

  switch (brush_hardness)
    {
    case SOFT:
      mask = gimp_paint_core_subsample_mask (core, mask,
					     core->cur_coords.x,
                                             core->cur_coords.y);
      break;
    case HARD:
      mask = gimp_paint_core_solidify_mask (core, mask);
      break;
    case PRESSURE:
      mask = gimp_paint_core_pressurize_mask (core, mask,
					      core->cur_coords.x,
                                              core->cur_coords.y,
					      core->cur_coords.pressure);
      break;
    default:
      break;
    }

  return mask;
}

static void
gimp_paint_core_paste (GimpPaintCore        *core,
		       MaskBuf              *brush_mask,
		       GimpDrawable         *drawable,
		       gint                  brush_opacity,
		       gint                  image_opacity,
		       GimpLayerModeEffects  paint_mode,
		       PaintApplicationMode  mode)
{
  GimpImage   *gimage;
  PixelRegion  srcPR;
  TileManager *alt = NULL;
  gint         offx;
  gint         offy;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  /*  set undo blocks  */
  set_undo_tiles (core,
                  drawable,
		  core->canvas_buf->x, core->canvas_buf->y,
		  core->canvas_buf->width, core->canvas_buf->height);

  /*  If the mode is CONSTANT:
   *   combine the canvas buf, the brush mask to the canvas tiles
   */
  if (mode == CONSTANT)
    {
      /*  initialize any invalid canvas tiles  */
      set_canvas_tiles (core,
                        core->canvas_buf->x,
                        core->canvas_buf->y,
			core->canvas_buf->width,
                        core->canvas_buf->height);

      brush_to_canvas_tiles (core, brush_mask, brush_opacity);
      canvas_tiles_to_canvas_buf (core);
      alt = core->undo_tiles;
    }
  /*  Otherwise:
   *   combine the canvas buf and the brush mask to the canvas buf
   */
  else
    {
      brush_to_canvas_buf (core, brush_mask, brush_opacity);
    }

  /*  intialize canvas buf source pixel regions  */
  srcPR.bytes     = core->canvas_buf->bytes;
  srcPR.x         = 0;
  srcPR.y         = 0;
  srcPR.w         = core->canvas_buf->width;
  srcPR.h         = core->canvas_buf->height;
  srcPR.rowstride = core->canvas_buf->width * core->canvas_buf->bytes;
  srcPR.data      = temp_buf_data (core->canvas_buf);

  /*  apply the paint area to the gimage  */
  gimp_image_apply_image (gimage, drawable, &srcPR,
			  FALSE, image_opacity, paint_mode,
			  alt,  /*  specify an alternative src1  */
			  core->canvas_buf->x,
                          core->canvas_buf->y);

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->canvas_buf->x);
  core->y1 = MIN (core->y1, core->canvas_buf->y);
  core->x2 = MAX (core->x2, core->canvas_buf->x + core->canvas_buf->width);
  core->y2 = MAX (core->y2, core->canvas_buf->y + core->canvas_buf->height);

  /*  Update the gimage--it is important to call gimp_image_update
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  gimp_drawable_offsets (drawable, &offx, &offy);
  gimp_image_update (gimage,
                     core->canvas_buf->x + offx,
                     core->canvas_buf->y + offy,
                     core->canvas_buf->width,
                     core->canvas_buf->height);
}

/* This works similarly to gimp_paint_core_paste. However, instead of
 * combining the canvas to the paint core drawable using one of the
 * combination modes, it uses a "replace" mode (i.e. transparent
 * pixels in the canvas erase the paint core drawable).

 * When not drawing on alpha-enabled images, it just paints using
 * NORMAL mode.
 */
static void
gimp_paint_core_replace (GimpPaintCore        *core,
			 MaskBuf              *brush_mask,
			 GimpDrawable         *drawable,
			 gint                  brush_opacity,
			 gint                  image_opacity,
			 PaintApplicationMode  mode)
{
  GimpImage   *gimage;
  PixelRegion  srcPR;
  PixelRegion  maskPR;
  TileManager *alt = NULL;
  gint         offx;
  gint         offy;

  if (! gimp_drawable_has_alpha (drawable))
    {
      gimp_paint_core_paste (core, brush_mask, drawable,
			     brush_opacity, image_opacity, GIMP_NORMAL_MODE,
			     mode);
      return;
    }

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  /*  set undo blocks  */
  set_undo_tiles (core,
                  drawable,
		  core->canvas_buf->x,
                  core->canvas_buf->y,
		  core->canvas_buf->width,
                  core->canvas_buf->height);

  if (mode == CONSTANT)
    {
      /*  initialize any invalid canvas tiles  */
      set_canvas_tiles (core,
                        core->canvas_buf->x,
                        core->canvas_buf->y,
			core->canvas_buf->width,
                        core->canvas_buf->height);

      /* combine the brush mask and the canvas tiles */
      brush_to_canvas_tiles (core, brush_mask, brush_opacity);

      /* set the alt source as the unaltered undo_tiles */
      alt = core->undo_tiles;

      /* initialize the maskPR from the canvas tiles */
      pixel_region_init (&maskPR, core->canvas_tiles,
			 core->canvas_buf->x,
                         core->canvas_buf->y,
			 core->canvas_buf->width,
                         core->canvas_buf->height,
                         FALSE);
    }
  else
    {
      /* The mask is just the brush mask */
      maskPR.bytes     = 1;
      maskPR.x         = 0;
      maskPR.y         = 0;
      maskPR.w         = core->canvas_buf->width;
      maskPR.h         = core->canvas_buf->height;
      maskPR.rowstride = maskPR.bytes * brush_mask->width;
      maskPR.data      = mask_buf_data (brush_mask);
    }

  /*  intialize canvas buf source pixel regions  */
  srcPR.bytes     = core->canvas_buf->bytes;
  srcPR.x         = 0;
  srcPR.y         = 0;
  srcPR.w         = core->canvas_buf->width;
  srcPR.h         = core->canvas_buf->height;
  srcPR.rowstride = core->canvas_buf->width * core->canvas_buf->bytes;
  srcPR.data      = temp_buf_data (core->canvas_buf);

  /*  apply the paint area to the gimage  */
  gimp_image_replace_image (gimage, drawable, &srcPR,
			    FALSE, image_opacity,
			    &maskPR,
			    core->canvas_buf->x,
                            core->canvas_buf->y);

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->canvas_buf->x);
  core->y1 = MIN (core->y1, core->canvas_buf->y);
  core->x2 = MAX (core->x2, core->canvas_buf->x + core->canvas_buf->width) ;
  core->y2 = MAX (core->y2, core->canvas_buf->y + core->canvas_buf->height) ;

  /*  Update the gimage--it is important to call gimp_image_update
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  gimp_drawable_offsets (drawable, &offx, &offy);
  gimp_image_update (gimage,
                     core->canvas_buf->x + offx,
                     core->canvas_buf->y + offy,
                     core->canvas_buf->width,
                     core->canvas_buf->height);
}

static void
canvas_tiles_to_canvas_buf (GimpPaintCore *core)
{
  PixelRegion srcPR;
  PixelRegion maskPR;

  /*  combine the canvas tiles and the canvas buf  */
  srcPR.bytes     = core->canvas_buf->bytes;
  srcPR.x         = 0;
  srcPR.y         = 0;
  srcPR.w         = core->canvas_buf->width;
  srcPR.h         = core->canvas_buf->height;
  srcPR.rowstride = core->canvas_buf->width * core->canvas_buf->bytes;
  srcPR.data      = temp_buf_data (core->canvas_buf);

  pixel_region_init (&maskPR, core->canvas_tiles,
		     core->canvas_buf->x,
                     core->canvas_buf->y,
		     core->canvas_buf->width,
                     core->canvas_buf->height,
                     FALSE);

  /*  apply the canvas tiles to the canvas buf  */
  apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
}

static void
brush_to_canvas_tiles (GimpPaintCore *core,
		       MaskBuf       *brush_mask,
		       gint           brush_opacity)
{
  PixelRegion srcPR;
  PixelRegion maskPR;
  gint        x;
  gint        y;
  gint        xoff;
  gint        yoff;

  /*   combine the brush mask and the canvas tiles  */
  pixel_region_init (&srcPR, core->canvas_tiles,
		     core->canvas_buf->x,
                     core->canvas_buf->y,
		     core->canvas_buf->width,
                     core->canvas_buf->height,
                     TRUE);

  x = (gint) floor (core->cur_coords.x) - (brush_mask->width  >> 1);
  y = (gint) floor (core->cur_coords.y) - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;

  maskPR.bytes     = 1;
  maskPR.x         = 0;
  maskPR.y         = 0;
  maskPR.w         = srcPR.w;
  maskPR.h         = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data      = mask_buf_data (brush_mask) + (yoff * maskPR.rowstride +
                                                   xoff * maskPR.bytes);

  /*  combine the mask to the canvas tiles  */
  combine_mask_and_region (&srcPR, &maskPR, brush_opacity);
}

static void
brush_to_canvas_buf (GimpPaintCore *core,
		     MaskBuf       *brush_mask,
		     gint           brush_opacity)
{
  PixelRegion srcPR;
  PixelRegion maskPR;
  gint        x;
  gint        y;
  gint        xoff;
  gint        yoff;

  x = (gint) floor (core->cur_coords.x) - (brush_mask->width  >> 1);
  y = (gint) floor (core->cur_coords.y) - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;

  /*  combine the canvas buf and the brush mask to the canvas buf  */
  srcPR.bytes     = core->canvas_buf->bytes;
  srcPR.x         = 0;
  srcPR.y         = 0;
  srcPR.w         = core->canvas_buf->width;
  srcPR.h         = core->canvas_buf->height;
  srcPR.rowstride = core->canvas_buf->width * core->canvas_buf->bytes;
  srcPR.data      = temp_buf_data (core->canvas_buf);

  maskPR.bytes     = 1;
  maskPR.x         = 0;
  maskPR.y         = 0;
  maskPR.w         = srcPR.w;
  maskPR.h         = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data      = mask_buf_data (brush_mask) + yoff * maskPR.rowstride + xoff * maskPR.bytes;

  /*  apply the mask  */
  apply_mask_to_region (&srcPR, &maskPR, brush_opacity);
}

#if 0
static void
paint_to_canvas_tiles (GimpPaintCore *core,
		       Maskbuf       *brush_mask,
		       gint           brush_opacity)
{
  PixelRegion srcPR;
  PixelRegion maskPR;
  gint x;
  gint y;
  gint xoff;
  gint yoff;

  /*   combine the brush mask and the canvas tiles  */
  pixel_region_init (&srcPR, core->canvas_tiles,
		     core->canvas_buf->x,
                     core->canvas_buf->y,
		     core->canvas_buf->width,
                     core->canvas_buf->height,
                     TRUE);

  x = (gint) floor (core->cur_coords.x) - (brush_mask->width  >> 1);
  y = (gint) floor (core->cur_coords.y) - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;

  maskPR.bytes     = 1;
  maskPR.x         = 0;
  maskPR.y         = 0;
  maskPR.w         = srcPR.w;
  maskPR.h         = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data      = mask_buf_data (brush_mask) + (yoff * maskPR.rowstride +
                                                   xoff * maskPR.bytes);

  /*  combine the mask and canvas tiles  */
  combine_mask_and_region (&srcPR, &maskPR, brush_opacity);

  /*  combine the canvas tiles and the canvas buf  */
  srcPR.bytes     = core->canvas_buf->bytes;
  srcPR.x         = 0;
  srcPR.y         = 0;
  srcPR.w         = core->canvas_buf->width;
  srcPR.h         = core->canvas_buf->height;
  srcPR.rowstride = core->canvas_buf->width * core->canvas_buf->bytes;
  srcPR.data      = temp_buf_data (core->canvas_buf);

  pixel_region_init (&maskPR, core->canvas_tiles,
		     core->canvas_buf->x,
                     core->canvas_buf->y,
		     core->canvas_buf->width,
                     core->canvas_buf->height,
                     FALSE);

  /*  apply the canvas tiles to the canvas buf  */
  apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
}

static void
paint_to_canvas_buf (GimpPaintCore *core,
		     MaskBuf       *brush_mask,
		     gint           brush_opacity)
{
  PixelRegion srcPR;
  PixelRegion maskPR;
  gint x;
  gint y;
  gint xoff;
  gint yoff;

  x = (gint) floor (core->cur_coords.x) - (brush_mask->width  >> 1);
  y = (gint) floor (core->cur_coords.y) - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;


  /*  combine the canvas buf and the brush mask to the canvas buf  */
  srcPR.bytes     = core->canvas_buf->bytes;
  srcPR.x         = 0;
  srcPR.y         = 0;
  srcPR.w         = core->canvas_buf->width;
  srcPR.h         = core->canvas_buf->height;
  srcPR.rowstride = core->canvas_buf->width * canvas_buf->bytes;
  srcPR.data      = temp_buf_data (core->canvas_buf);

  maskPR.bytes     = 1;
  maskPR.x         = 0;
  maskPR.y         = 0;
  maskPR.w         = srcPR.w;
  maskPR.h         = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data      = mask_buf_data (brush_mask) + yoff * maskPR.rowstride + xoff * maskPR.bytes;

  /*  apply the mask  */
  apply_mask_to_region (&srcPR, &maskPR, brush_opacity);
}
#endif

static void
set_undo_tiles (GimpPaintCore *core,
                GimpDrawable  *drawable,
		gint           x,
		gint           y,
		gint           w,
		gint           h)
{
  gint  i;
  gint  j;
  Tile *src_tile;
  Tile *dest_tile;

  if (! core->undo_tiles)
    {
      g_warning ("set_undo_tiles: undo_tiles is null");
      return;
    }

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
	{
	  dest_tile = tile_manager_get_tile (core->undo_tiles, j, i,
                                             FALSE, FALSE);

	  if (tile_is_valid (dest_tile) == FALSE)
	    {
	      src_tile = tile_manager_get_tile (gimp_drawable_data (drawable),
						j, i, TRUE, FALSE);
	      tile_manager_map_tile (core->undo_tiles, j, i, src_tile);
	      tile_release (src_tile, FALSE);
	    }
	}
    }
}

static void
set_canvas_tiles (GimpPaintCore *core,
                  gint           x,
		  gint           y,
		  gint           w,
		  gint           h)
{
  gint  i;
  gint  j;
  Tile *tile;

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
	{
	  tile = tile_manager_get_tile (core->canvas_tiles, j, i,
                                        FALSE, FALSE);

	  if (tile_is_valid (tile) == FALSE)
	    {
	      tile = tile_manager_get_tile (core->canvas_tiles, j, i,
                                            TRUE, TRUE);
	      memset (tile_data_pointer (tile, 0, 0), 0, tile_size (tile));
	      tile_release (tile, TRUE);
	    }
	}
    }
}


/**************************************************/
/*  Brush pipe utility functions                  */
/**************************************************/

void
gimp_paint_core_color_area_with_pixmap (GimpPaintCore        *core,
					GimpImage            *dest,
					GimpDrawable         *drawable,
					TempBuf              *area,
					gdouble               scale,
					BrushApplicationMode  mode)
{
  PixelRegion  destPR;
  void        *pr;
  guchar      *d;
  gint         ulx;
  gint         uly;
  gint         offsetx;
  gint         offsety;
  gint         y;
  TempBuf     *pixmap_mask;
  TempBuf     *brush_mask;

  g_return_if_fail (GIMP_IS_BRUSH (core->brush));
  g_return_if_fail (core->brush->pixmap != NULL);

  /*  scale the brushes  */
  pixmap_mask = gimp_paint_core_scale_pixmap (core,
                                              core->brush->pixmap,
                                              scale);

  if (mode == SOFT)
    brush_mask = gimp_paint_core_scale_mask (core,
                                             core->brush->mask,
                                             scale);
  else
    brush_mask = NULL;

  destPR.bytes     = area->bytes;
  destPR.x         = 0;
  destPR.y         = 0;
  destPR.w         = area->width;
  destPR.h         = area->height;
  destPR.rowstride = destPR.bytes * area->width;
  destPR.data      = temp_buf_data (area);
		
  pr = pixel_regions_register (1, &destPR);

  /* Calculate upper left corner of brush as in
   * gimp_paint_core_get_paint_area.  Ugly to have to do this here, too.
   */

  ulx = (gint) floor (core->cur_coords.x) - (pixmap_mask->width  >> 1);
  uly = (gint) floor (core->cur_coords.y) - (pixmap_mask->height >> 1);

  offsetx = area->x - ulx;
  offsety = area->y - uly;

  for (; pr != NULL; pr = pixel_regions_process (pr))
    {
      d = destPR.data;
      for (y = 0; y < destPR.h; y++)
	{
	  paint_line_pixmap_mask (dest, drawable, pixmap_mask, brush_mask,
				  d, offsetx, y + offsety,
				  destPR.bytes, destPR.w, mode);
	  d += destPR.rowstride;
	}
    }
}

static void
paint_line_pixmap_mask (GimpImage            *dest,
			GimpDrawable         *drawable,
			TempBuf              *pixmap_mask,
			TempBuf              *brush_mask,
			guchar               *d,
			gint                  x,
			gint                  y,
			gint                  bytes,
			gint                  width,
			BrushApplicationMode  mode)
{
  guchar  *b;
  guchar  *p;
  guchar  *mask;
  gdouble  alpha;
  gdouble  factor = 0.00392156986;  /*  1.0 / 255.0  */
  gint     x_index;
  gint     i,byte_loop;

  /*  Make sure x, y are positive  */
  while (x < 0)
    x += pixmap_mask->width;
  while (y < 0)
    y += pixmap_mask->height;

  /* Point to the approriate scanline */
  b = temp_buf_data (pixmap_mask) +
    (y % pixmap_mask->height) * pixmap_mask->width * pixmap_mask->bytes;
    
  if (mode == SOFT && brush_mask)
    {
      /* ditto, except for the brush mask, so we can pre-multiply the alpha value */
      mask = temp_buf_data (brush_mask) +
	(y % brush_mask->height) * brush_mask->width;
      for (i = 0; i < width; i++)
	{
	  /* attempt to avoid doing this calc twice in the loop */
	  x_index = ((i + x) % pixmap_mask->width);
	  p = b + x_index * pixmap_mask->bytes;
	  d[bytes-1] = mask[x_index];
	  
	  /* multiply alpha into the pixmap data
           * maybe we could do this at tool creation or brush switch time?
           * and compute it for the whole brush at once and cache it?
           */
	  alpha = d[bytes-1] * factor;
	  if (alpha)
	    for (byte_loop = 0; byte_loop < bytes - 1; byte_loop++) 
	      d[byte_loop] *= alpha;

	  /* printf("i: %i d->r: %i d->g: %i d->b: %i d->a: %i\n",i,(int)d[0], (int)d[1], (int)d[2], (int)d[3]); */
	  gimp_image_transform_color (dest, drawable, p, d, GIMP_RGB);
	  d += bytes;
	}
    }
  else
    {
      for (i = 0; i < width; i++)
	{
	  /* attempt to avoid doing this calc twice in the loop */
	  x_index = ((i + x) % pixmap_mask->width);
	  p = b + x_index * pixmap_mask->bytes;
	  d[bytes-1] = 255;
	  
	  /* multiply alpha into the pixmap data */
	  /* maybe we could do this at tool creation or brush switch time? */
	  /* and compute it for the whole brush at once and cache it?  */
	  gimp_image_transform_color (dest, drawable, p, d, GIMP_RGB);
	  d += bytes;
	}
    }
}
