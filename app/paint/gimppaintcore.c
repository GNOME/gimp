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

#include <string.h>

#include <glib-object.h>

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
#include "core/gimpbrush.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpmarshal.h"
#include "core/gimppaintinfo.h"

#include "gimppaintcore.h"
#include "gimppaintcore-kernels.h"
#include "gimppaintcore-undo.h"
#include "gimppaintoptions.h"

#include "gimp-intl.h"


#define EPSILON  0.00001


/*  local function prototypes  */

static void   gimp_paint_core_class_init          (GimpPaintCoreClass *klass);
static void   gimp_paint_core_init                (GimpPaintCore      *core);

static void   gimp_paint_core_finalize            (GObject          *object);

static void   gimp_paint_core_calc_brush_size     (GimpPaintCore    *core,
                                                   MaskBuf          *mask,
                                                   gdouble           scale,
                                                   gint             *width,
                                                   gint             *height);
static inline void rotate_pointers                (gulong          **p,
                                                   guint32           n);
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
                                                   MaskBuf          *brush_mask,
                                                   gdouble           x,
                                                   gdouble           y);
static MaskBuf * gimp_paint_core_scale_mask       (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask,
                                                   gdouble           scale);
static MaskBuf * gimp_paint_core_scale_pixmap     (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask,
                                                   gdouble           scale);

static MaskBuf * gimp_paint_core_get_brush_mask   (GimpPaintCore    *core,
                                                   GimpBrushApplicationMode  brush_hardness,
                                                   gdouble           scale);
static void      gimp_paint_core_paste            (GimpPaintCore    *core,
                                                   MaskBuf	    *brush_mask,
                                                   GimpDrawable	    *drawable,
                                                   gdouble	     brush_opacity,
                                                   gdouble	     image_opacity,
                                                   GimpLayerModeEffects  paint_mode,
                                                   GimpPaintApplicationMode  mode);
static void      gimp_paint_core_replace          (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask,
                                                   GimpDrawable	    *drawable,
                                                   gdouble	     brush_opacity,
                                                   gdouble           image_opacity,
                                                   GimpPaintApplicationMode  mode);

static void      brush_to_canvas_tiles            (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask,
                                                   gdouble           brush_opacity);
static void      brush_to_canvas_buf              (GimpPaintCore    *core,
                                                   MaskBuf          *brush_mask,
                                                   gdouble           brush_opacity);
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
                                                   GimpBrushApplicationMode  mode);


static GimpObjectClass *parent_class = NULL;

static gint global_core_ID = 1;


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

  core->ID                       = global_core_ID++;

  core->distance                 = 0.0;
  core->spacing                  = 0.0;
  core->x1                       = 0;
  core->y1                       = 0;
  core->x2                       = 0;
  core->y2                       = 0;

  core->brush                    = NULL;

  core->flags                    = 0;
  core->use_pressure             = FALSE;

  core->undo_tiles               = NULL;
  core->canvas_tiles             = NULL;

  core->orig_buf                 = NULL;
  core->canvas_buf               = NULL;

  core->pressure_brush           = NULL;

  for (i = 0; i < PAINT_CORE_SOLID_SUBSAMPLE; i++)
    for (j = 0; j < PAINT_CORE_SOLID_SUBSAMPLE; j++)
      core->solid_brushes[i][j] = NULL;

  core->last_solid_brush         = NULL;
  core->solid_cache_invalid      = FALSE;

  core->scale_brush              = NULL;
  core->last_scale_brush         = NULL;
  core->last_scale_width         = 0;
  core->last_scale_height        = 0;

  core->scale_pixmap             = NULL;
  core->last_scale_pixmap        = NULL;
  core->last_scale_pixmap_width  = 0;
  core->last_scale_pixmap_height = 0;

  g_assert (PAINT_CORE_SUBSAMPLE == KERNEL_SUBSAMPLE);

  for (i = 0; i < KERNEL_SUBSAMPLE + 1; i++)
    for (j = 0; j < KERNEL_SUBSAMPLE + 1; j++)
      core->kernel_brushes[i][j] = NULL;

  core->last_brush_mask          = NULL;
  core->cache_invalid            = FALSE;

  core->grr_brush                = NULL;
  core->brush_bound_segs         = NULL;
  core->n_brush_bound_segs       = 0;
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

  for (i = 0; i < PAINT_CORE_SOLID_SUBSAMPLE; i++)
    for (j = 0; j < PAINT_CORE_SOLID_SUBSAMPLE; j++)
      if (core->solid_brushes[i][j])
        {
          temp_buf_free (core->solid_brushes[i][j]);
          core->solid_brushes[i][j] = NULL;
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

  for (i = 0; i < KERNEL_SUBSAMPLE + 1; i++)
    for (j = 0; j < KERNEL_SUBSAMPLE + 1; j++)
      if (core->kernel_brushes[i][j])
        {
          temp_buf_free (core->kernel_brushes[i][j]);
          core->kernel_brushes[i][j] = NULL;
        }

  if (core->grr_brush)
    {
      g_signal_handlers_disconnect_by_func (core->grr_brush,
                                            gimp_paint_core_invalidate_cache,
                                            core);
      g_object_unref (core->grr_brush);
      core->grr_brush = NULL;
    }

  if (core->brush_bound_segs)
    {
      g_free (core->brush_bound_segs);
      core->brush_bound_segs   = NULL;
      core->n_brush_bound_segs = 0;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
gimp_paint_core_paint (GimpPaintCore      *core,
		       GimpDrawable       *drawable,
                       GimpPaintOptions   *paint_options,
		       GimpPaintCoreState  paint_state)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options));

  if (paint_state == MOTION_PAINT)
    {
      /* If we current point == last point, check if the brush
       * wants to be painted in that case. (Direction dependent
       * pixmap brush pipes don't, as they don't know which
       * pixmap to select.)
       */
      if (core->last_coords.x == core->cur_coords.x &&
	  core->last_coords.y == core->cur_coords.y &&
	  ! gimp_brush_want_null_motion (core->brush,
                                         &core->last_coords,
                                         &core->cur_coords))
        {
          return;
        }

      if (core->flags & CORE_HANDLES_CHANGING_BRUSH)
        {
          core->brush = gimp_brush_select_brush (core->brush,
                                                 &core->last_coords,
                                                 &core->cur_coords);
        }

      /* Save coordinates for gimp_paint_core_interpolate() */
      core->last_paint.x = core->cur_coords.x;
      core->last_paint.y = core->cur_coords.y;
    }

  GIMP_PAINT_CORE_GET_CLASS (core)->paint (core,
                                           drawable,
                                           paint_options,
                                           paint_state);
}

gboolean
gimp_paint_core_start (GimpPaintCore    *core,
		       GimpDrawable     *drawable,
                       GimpPaintOptions *paint_options,
                       GimpCoords       *coords)
{
  GimpItem  *item;
  GimpImage *gimage;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  item   = GIMP_ITEM (drawable);
  gimage = gimp_item_get_image (item);

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  core->cur_coords = *coords;

  /*  Each buffer is the same size as
   *  the maximum bounds of the active brush...
   */

  if (core->grr_brush != gimp_context_get_brush (GIMP_CONTEXT (paint_options)))
    {
      if (core->grr_brush)
        {
          g_signal_handlers_disconnect_by_func (core->grr_brush,
                                                gimp_paint_core_invalidate_cache,
                                                core);
          g_object_unref (core->grr_brush);
          core->grr_brush = NULL;
        }

      if (core->brush_bound_segs)
        {
          g_free (core->brush_bound_segs);
          core->brush_bound_segs   = NULL;
          core->n_brush_bound_segs = 0;
        }
    }

  if (! core->grr_brush)
    {
      core->grr_brush = gimp_context_get_brush (GIMP_CONTEXT (paint_options));

      if (! core->grr_brush)
        {
          g_message (_("No brushes available for use with this tool."));
          return FALSE;
        }

      g_object_ref (core->grr_brush);
      g_signal_connect (core->grr_brush, "invalidate_preview",
                        G_CALLBACK (gimp_paint_core_invalidate_cache),
                        core);
    }

  core->spacing = (gdouble) gimp_brush_get_spacing (core->grr_brush) / 100.0;

  core->brush = core->grr_brush;

  /*  Allocate the undo structure  */
  if (core->undo_tiles)
    tile_manager_unref (core->undo_tiles);

  core->undo_tiles = tile_manager_new (gimp_item_width (item),
                                       gimp_item_height (item),
                                       gimp_drawable_bytes (drawable));

  /*  Allocate the canvas blocks structure  */
  if (core->canvas_tiles)
    tile_manager_unref (core->canvas_tiles);

  core->canvas_tiles = tile_manager_new (gimp_item_width (item),
                                         gimp_item_height (item),
                                         1);

  /*  Get the initial undo extents  */

  core->x1           = core->x2 = core->cur_coords.x;
  core->y1           = core->y2 = core->cur_coords.y;

  core->last_paint.x = -1e6;
  core->last_paint.y = -1e6;

  return TRUE;
}

void
gimp_paint_core_finish (GimpPaintCore *core,
			GimpDrawable  *drawable)
{
  GimpPaintInfo *paint_info;
  GimpImage     *gimage;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((core->x2 == core->x1) || (core->y2 == core->y1))
    return;

  paint_info = (GimpPaintInfo *)
    gimp_container_get_child_by_name (gimage->gimp->paint_info_list,
                                      g_type_name (G_TYPE_FROM_INSTANCE (core)));

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_PAINT,
                               paint_info ? paint_info->blurb : _("Paint"));

  gimp_paint_core_push_undo (gimage, NULL, core);

  gimp_drawable_push_undo (drawable, NULL,
                           core->x1, core->y1,
                           core->x2, core->y2,
                           core->undo_tiles,
                           TRUE);

  tile_manager_unref (core->undo_tiles);
  core->undo_tiles = NULL;

  gimp_image_undo_group_end (gimage);

  /*  invalidate the previews -- have to do it here, because
   *  it is not done during the actual painting.
   */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (drawable));
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
}

static void
gimp_paint_core_copy_valid_tiles (TileManager *src_tiles,
                                  TileManager *dest_tiles,
                                  gint         x,
                                  gint         y,
                                  gint         w,
                                  gint         h)
{
  Tile *src_tile;
  gint  i, j;

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
        {
          src_tile = tile_manager_get_tile (src_tiles,
                                            j, i, FALSE, FALSE);

          if (tile_is_valid (src_tile))
            {
              src_tile = tile_manager_get_tile (src_tiles,
                                                j, i, TRUE, FALSE);

              tile_manager_map_tile (dest_tiles, j, i, src_tile);

              tile_release (src_tile, FALSE);
            }
        }
    }
}

void
gimp_paint_core_cancel (GimpPaintCore *core,
			GimpDrawable  *drawable)
{
  GimpImage   *gimage;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((core->x2 == core->x1) || (core->y2 == core->y1))
    return;

  gimp_paint_core_copy_valid_tiles (core->undo_tiles,
                                    gimp_drawable_data (drawable),
                                    core->x1, core->y1,
                                    core->x2 - core->x1,
                                    core->y2 - core->y1);

  tile_manager_unref (core->undo_tiles);
  core->undo_tiles = NULL;

  gimp_drawable_update (drawable,
                        core->x1, core->y1,
                        core->x2 - core->x1, core->y2 - core->y1);
}

void
gimp_paint_core_cleanup (GimpPaintCore *core)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));

  if (core->undo_tiles)
    {
      tile_manager_unref (core->undo_tiles);
      core->undo_tiles = NULL;
    }

  if (core->canvas_tiles)
    {
      tile_manager_unref (core->canvas_tiles);
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

/**
 * gimp_paint_core_constrain_helper:
 * @dx: the (fixed) delta-x
 * @dy: a suggested delta-y
 *
 * Returns an adjusted dy' near dy such that the slope (dx,dy') is a
 * multiple of 15 degrees.
 **/
static gdouble
gimp_paint_core_constrain_helper (gdouble dx,
                                  gdouble dy)
{
  static gdouble slope[4] = { 0, 0.26795, 0.57735, 1 };
  static gdouble divider[3] = { 0.13165, 0.41421, 0.76732 };
  gint i;

  if (dy < 0)
    return - gimp_paint_core_constrain_helper (dx,-dy);
  dx = fabs (dx);
  for (i = 0; i < 3; i ++)
    if (dy < dx * divider[i])
      break;
  dy = dx * slope[i];
  return dy;
}

/**
 * gimp_paint_core_constrain:
 * @core: the #GimpPaintCore.
 *
 * Restricts the (core->last_coords, core->cur_coords) vector to 15
 * degree steps, possibly changing core->cur_coords.
 **/
void
gimp_paint_core_constrain (GimpPaintCore *core)
{
  gdouble dx, dy;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));

  dx = core->cur_coords.x - core->last_coords.x;
  dy = core->cur_coords.y - core->last_coords.y;

  /*  This algorithm changes only one of dx and dy, and does not try
   *  to constrain the resulting dx and dy to integers. This gives
   *  at least two benefits:
   *    1. gimp_paint_core_constrain is idempotent, even if followed by
   *       a rounding operation.
   *    2. For any two lines with the same starting-point and ideal
   *       15-degree direction, the points plotted by
   *       gimp_paint_core_interpolate for the shorter line will always
   *       be a superset of those plotted for the longer line.
   */
  if (fabs(dx) > fabs(dy))
    core->cur_coords.y = core->last_coords.y +
                         gimp_paint_core_constrain_helper (dx,dy);
  else
    core->cur_coords.x = core->last_coords.x +
                         gimp_paint_core_constrain_helper (dy,dx);
}

/**
 * gimp_avoid_exact_integer
 * @x: points to a gdouble
 *
 * Adjusts *x such that it is not too close to an integer. This is used
 * for decision algorithms that would be vulnerable to rounding glitches
 * if exact integers were input.
 *
 * Side effects: Changes the value of *x
 **/
static void
gimp_avoid_exact_integer (gdouble *x)
{
  gdouble integral   = floor (*x);
  gdouble fractional = *x - integral;

  if (fractional < EPSILON)
    *x = integral + EPSILON;
  else if (fractional > (1-EPSILON))
    *x = integral + (1-EPSILON);
}

void
gimp_paint_core_interpolate (GimpPaintCore    *core,
			     GimpDrawable     *drawable,
                             GimpPaintOptions *paint_options)
{
  GimpVector2 delta_vec;
  gdouble     delta_pressure;
  gdouble     delta_xtilt, delta_ytilt;
  gdouble     delta_wheel;
  gint        n, num_points;
  gdouble     t0, dt, tn;
  gdouble     st_factor, st_offset;
  gdouble     initial;
  gdouble     dist;
  gdouble     total;
  gdouble     pixel_dist;
  gdouble     pixel_initial;
  gdouble     xd, yd;
  gdouble     mag;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options));

  gimp_avoid_exact_integer (&core->last_coords.x);
  gimp_avoid_exact_integer (&core->last_coords.y);
  gimp_avoid_exact_integer (&core->cur_coords.x);
  gimp_avoid_exact_integer (&core->cur_coords.y);

  delta_vec.x    = core->cur_coords.x        - core->last_coords.x;
  delta_vec.y    = core->cur_coords.y        - core->last_coords.y;
  delta_pressure = core->cur_coords.pressure - core->last_coords.pressure;
  delta_xtilt    = core->cur_coords.xtilt    - core->last_coords.xtilt;
  delta_ytilt    = core->cur_coords.ytilt    - core->last_coords.ytilt;
  delta_wheel    = core->cur_coords.wheel    - core->last_coords.wheel;

  /*  return if there has been no motion  */
  if (! delta_vec.x    &&
      ! delta_vec.y    &&
      ! delta_pressure &&
      ! delta_xtilt    &&
      ! delta_ytilt    &&
      ! delta_wheel)
    return;

  /* calculate the distance traveled in the coordinate space of the brush */
  mag = gimp_vector2_length (&(core->brush->x_axis));
  xd  = gimp_vector2_inner_product (&delta_vec,
				    &(core->brush->x_axis)) / (mag * mag);

  mag = gimp_vector2_length (&(core->brush->y_axis));
  yd  = gimp_vector2_inner_product (&delta_vec,
				    &(core->brush->y_axis)) / (mag * mag);

  dist    = 0.5 * sqrt (xd * xd + yd * yd);
  total   = dist + core->distance;
  initial = core->distance;

  pixel_dist    = gimp_vector2_length (&delta_vec);
  pixel_initial = core->pixel_dist;

  /*  FIXME: need to adapt the spacing to the size  */
  /*   lastscale = MIN (gimp_paint_tool->lastpressure, 1/256); */
  /*   curscale = MIN (gimp_paint_tool->curpressure,  1/256); */
  /*   spacing = gimp_paint_tool->spacing * sqrt (0.5 * (lastscale + curscale)); */

  /*  Compute spacing parameters such that a brush position will be
   *  made each time the line crosses the *center* of a pixel row or
   *  column, according to whether the line is mostly horizontal or
   *  mostly vertical. The term "stripe" will mean "column" if the
   *  line is horizontalish; "row" if the line is verticalish.
   *
   *  We start by deriving coefficients for a new parameter 's':
   *      s = t * st_factor + st_offset
   *  such that the "nice" brush positions are the ones with *integer*
   *  s values. (Actually the value of s will be 1/2 less than the nice
   *  brush position's x or y coordinate - note that st_factor may
   *  be negative!)
   */

  if (delta_vec.x * delta_vec.x > delta_vec.y * delta_vec.y)
    {
      st_factor = delta_vec.x;
      st_offset = core->last_coords.x - 0.5;
    }
  else
    {
      st_factor = delta_vec.y;
      st_offset = core->last_coords.y - 0.5;
    }

  if (fabs (st_factor) > dist / core->spacing)
    {
      /*  The stripe principle leads to brush positions that are spaced
       *  *closer* than the official brush spacing. Use the official
       *  spacing instead. This is the common case when the brush spacing
       *  is large.
       *  The net effect is then to put a lower bound on the spacing, but
       *  one that varies with the slope of the line. This is suppose to
       *  make thin lines (say, with a 1x1 brush) prettier while leaving
       *  lines with larger brush spacing as they used to look in 1.2.x.
       */

      dt = core->spacing / dist;
      n = (gint) (initial / core->spacing + 1.0 + EPSILON);
      t0 = (n * core->spacing - initial) / dist;
      num_points = 1 + (gint) floor ((1 + EPSILON - t0) / dt);

      /* if we arnt going to paint anything this time and the brush
       * has only moved on one axis return without updating the brush
       * position, distance etc. so that we can more accurately space
       * brush strokes when curves are supplied to us in single pixel
       * chunks.
       */

      if (num_points == 0 && (delta_vec.x == 0 || delta_vec.y == 0))
	return;
    }
  else if (fabs (st_factor) < EPSILON)
    {
      /* Hm, we've hardly moved at all. Don't draw anything, but reset the
       * old coordinates and hope we've gone longer the next time.
       */
      core->cur_coords.x = core->last_coords.x;
      core->cur_coords.y = core->last_coords.y;
      /* ... but go along with the current pressure, tilt and wheel */
      return;
    }
  else
    {
      gint direction = st_factor > 0 ? 1 : -1;
      gint x, y;
      gint s0, sn;

      /*  Choose the first and last stripe to paint.
       *    FIRST PRIORITY is to avoid gaps painting with a 1x1 aliasing
       *  brush when a horizontalish line segment follows a verticalish
       *  one or vice versa - no matter what the angle between the two
       *  lines is. This will also limit the local thinning that a 1x1
       *  subsampled brush may suffer in the same situation.
       *    SECOND PRIORITY is to avoid making free-hand drawings
       *  unpleasantly fat by plotting redundant points.
       *    These are achieved by the following rules, but it is a little
       *  tricky to see just why. Do not change this algorithm unless you
       *  are sure you know what you're doing!
       */

      /*  Basic case: round the beginning and ending point to nearest
       *  stripe center.
       */
      s0 = (gint) floor (st_offset + 0.5);
      sn = (gint) floor (st_offset + st_factor + 0.5);

      t0 = (s0 - st_offset) / st_factor;
      tn = (sn - st_offset) / st_factor;

      x = (gint) floor (core->last_coords.x + t0 * delta_vec.x);
      y = (gint) floor (core->last_coords.y + t0 * delta_vec.y);
      if (t0 < 0.0 && !( x == (gint) floor (core->last_coords.x) &&
                         y == (gint) floor (core->last_coords.y) ))
        {
          /*  Exception A: If the first stripe's brush position is
           *  EXTRApolated into a different pixel square than the
           *  ideal starting point, dont't plot it.
           */
          s0 += direction;
        }
      else if (x == (gint) floor (core->last_paint.x) &&
               y == (gint) floor (core->last_paint.y))
        {
          /*  Exception B: If first stripe's brush position is within the
           *  same pixel square as the last plot of the previous line,
           *  don't plot it either.
           */
          s0 += direction;
        }

      x = (gint) floor (core->last_coords.x + tn * delta_vec.x);
      y = (gint) floor (core->last_coords.y + tn * delta_vec.y);
      if (tn > 1.0 && !( x == (gint) floor( core->cur_coords.x ) &&
                         y == (gint) floor( core->cur_coords.y ) ))
        {
          /*  Exception C: If the last stripe's brush position is
           *  EXTRApolated into a different pixel square than the
           *  ideal ending point, don't plot it.
           */
          sn -= direction;
        }

      t0 = (s0 - st_offset) / st_factor;
      tn = (sn - st_offset) / st_factor;
      dt         =     direction * 1.0 / st_factor;
      num_points = 1 + direction * (sn - s0);

      if (num_points >= 1)
        {
          /*  Hack the reported total distance such that it looks to the
           *  next line as if the the last pixel plotted were at an integer
           *  multiple of the brush spacing. This helps prevent artifacts
           *  for connected lines when the brush spacing is such that some
           *  slopes will use the stripe regime and other slopes will use
           *  the nominal brush spacing.
           */

          if (tn < 1)
            total = initial + tn * dist;

          total = core->spacing * (gint) (total / core->spacing + 0.5);
          total += (1.0 - tn) * dist;
        }
    }

  for (n = 0; n < num_points; n++)
    {
      GimpBrush *current_brush;
      gdouble    t = t0 + n*dt;

      core->cur_coords.x        = core->last_coords.x        + t * delta_vec.x;
      core->cur_coords.y        = core->last_coords.y        + t * delta_vec.y;
      core->cur_coords.pressure = core->last_coords.pressure + t * delta_pressure;
      core->cur_coords.xtilt    = core->last_coords.xtilt    + t * delta_xtilt;
      core->cur_coords.ytilt    = core->last_coords.ytilt    + t * delta_ytilt;
      core->cur_coords.wheel    = core->last_coords.wheel    + t * delta_wheel;

      core->distance            = initial                    + t * dist;
      core->pixel_dist          = pixel_initial              + t * pixel_dist;

      /*  save the current brush  */
      current_brush = core->brush;

      gimp_paint_core_paint (core, drawable, paint_options, MOTION_PAINT);

      /*  restore the current brush pointer  */
      core->brush = current_brush;
    }

  core->cur_coords.x        = core->last_coords.x        + delta_vec.x;
  core->cur_coords.y        = core->last_coords.y        + delta_vec.y;
  core->cur_coords.pressure = core->last_coords.pressure + delta_pressure;
  core->cur_coords.xtilt    = core->last_coords.xtilt    + delta_xtilt;
  core->cur_coords.ytilt    = core->last_coords.ytilt    + delta_ytilt;
  core->cur_coords.wheel    = core->last_coords.wheel    + delta_wheel;

  core->distance   = total;
  core->pixel_dist = pixel_initial + pixel_dist;

  core->last_coords = core->cur_coords;

}


/*  protected functions  */

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

  bytes = gimp_drawable_bytes_with_alpha (drawable);

  gimp_paint_core_calc_brush_size (core,
                                   core->brush->mask,
                                   scale,
                                   &bwidth, &bheight);

  /*  adjust the x and y coordinates to the upper left corner of the brush  */
  x = (gint) floor (core->cur_coords.x) - (bwidth  >> 1);
  y = (gint) floor (core->cur_coords.y) - (bheight >> 1);

  dwidth  = gimp_item_width  (GIMP_ITEM (drawable));
  dheight = gimp_item_height (GIMP_ITEM (drawable));

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

  dwidth  = gimp_item_width  (GIMP_ITEM (drawable));
  dheight = gimp_item_height (GIMP_ITEM (drawable));

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

      if (tile_is_valid (undo_tile))
	{
	  refd = 1;
	  undo_tile = tile_manager_get_tile (core->undo_tiles,
                                             srcPR.x, srcPR.y,
					     TRUE, FALSE);
	  s = (guchar *) tile_data_pointer (undo_tile, 0, 0) +
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
gimp_paint_core_paste_canvas (GimpPaintCore            *core,
			      GimpDrawable	       *drawable,
			      gdouble		        brush_opacity,
			      gdouble		        image_opacity,
			      GimpLayerModeEffects      paint_mode,
			      GimpBrushApplicationMode  brush_hardness,
			      gdouble                   brush_scale,
			      GimpPaintApplicationMode  mode)
{
  MaskBuf *brush_mask = gimp_paint_core_get_brush_mask (core,
                                                        brush_hardness,
                                                        brush_scale);

  if (brush_mask)
    gimp_paint_core_paste (core, brush_mask, drawable,
                           brush_opacity,
                           image_opacity, paint_mode,
                           mode);
}

/* Similar to gimp_paint_core_paste_canvas, but replaces the alpha channel
 * rather than using it to composite (i.e. transparent over opaque
 * becomes transparent rather than opauqe.
 */
void
gimp_paint_core_replace_canvas (GimpPaintCore            *core,
				GimpDrawable	         *drawable,
				gdouble                   brush_opacity,
				gdouble                   image_opacity,
				GimpBrushApplicationMode  brush_hardness,
				gdouble                   brush_scale,
				GimpPaintApplicationMode  mode)
{
  MaskBuf *brush_mask = gimp_paint_core_get_brush_mask (core,
                                                        brush_hardness,
                                                        brush_scale);

  if (brush_mask)
    gimp_paint_core_replace (core, brush_mask, drawable,
                             brush_opacity,
                             image_opacity, mode);
}


static void
gimp_paint_core_invalidate_cache (GimpBrush     *brush,
				  GimpPaintCore *core)
{
  /* Make sure we don't cache data for a brush that has changed */

  core->cache_invalid       = TRUE;
  core->solid_cache_invalid = TRUE;

  if (core->brush_bound_segs)
    {
      g_free (core->brush_bound_segs);
      core->brush_bound_segs   = NULL;
      core->n_brush_bound_segs = 0;
    }
}

/************************************************************
 *             LOCAL FUNCTION DEFINITIONS                   *
 ************************************************************/

static void
gimp_paint_core_calc_brush_size (GimpPaintCore *core,
                                 MaskBuf       *mask,
                                 gdouble        scale,
                                 gint          *width,
                                 gint          *height)
{
  scale = CLAMP (scale, 0.0, 1.0);

  if (! core->use_pressure)
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

static inline void
rotate_pointers (gulong  **p,
		 guint32   n)
{
  guint32  i;
  gulong  *tmp;

  tmp = p[0];
  for (i = 0; i < n-1; i++)
    {
      p[i] = p[i+1];
    }
  p[i] = tmp;
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
  gint        dest_offset_x = 0;
  gint        dest_offset_y = 0;
  const gint *kernel;
  gint        i, j;
  gint        r, s;
  gulong     *accum[KERNEL_HEIGHT];
  gint        offs;
  gint        kernel_sum;

  while (x < 0) x += mask->width;
  left = x - floor (x);
  index1 = (gint) (left * (gdouble) (KERNEL_SUBSAMPLE + 1));

  while (y < 0) y += mask->height;
  left = y - floor (y);
  index2 = (gint) (left * (gdouble) (KERNEL_SUBSAMPLE + 1));


  if ((mask->width % 2) == 0)
    {
      index1 += KERNEL_SUBSAMPLE >> 1;

      if (index1 > KERNEL_SUBSAMPLE)
        {
          index1 -= KERNEL_SUBSAMPLE + 1;
          dest_offset_x = 1;
        }
    }

  if ((mask->height % 2) == 0)
    {
      index2 += KERNEL_SUBSAMPLE >> 1;

      if (index2 > KERNEL_SUBSAMPLE)
        {
          index2 -= KERNEL_SUBSAMPLE + 1;
          dest_offset_y = 1;
        }
    }


  kernel = subsample[index2][index1];

  if (mask == core->last_brush_mask && ! core->cache_invalid)
    {
      if (core->kernel_brushes[index2][index1])
	return core->kernel_brushes[index2][index1];
    }
  else
    {
      for (i = 0; i < KERNEL_SUBSAMPLE + 1; i++)
	for (j = 0; j < KERNEL_SUBSAMPLE + 1; j++)
          if (core->kernel_brushes[i][j])
            {
              mask_buf_free (core->kernel_brushes[i][j]);
              core->kernel_brushes[i][j] = NULL;
            }

      core->last_brush_mask = mask;
      core->cache_invalid   = FALSE;
    }

  dest = mask_buf_new (mask->width  + 2,
                       mask->height + 2);

  /* Allocate and initialize the accum buffer */
  for (i = 0; i < KERNEL_HEIGHT ; i++)
    accum[i] = g_new0 (gulong, dest->width);

  /* Investigate modifiying kernelgen to make the sum the same
   *  for all kernels. That way kernal_sum becomes a constant
   */
  kernel_sum = 0;
  for (i = 0; i < KERNEL_HEIGHT * KERNEL_WIDTH; i++)
    {
      kernel_sum += kernel[i];
    }

  core->kernel_brushes[index2][index1] = dest;

  m = mask_buf_data (mask);
  for (i = 0; i < mask->height; i++)
    {
      for (j = 0; j < mask->width; j++)
	{
	  k = kernel;
	  for (r = 0; r < KERNEL_HEIGHT; r++)
	    {
	      offs = j + dest_offset_x;
	      s = KERNEL_WIDTH;
	      while (s--)
		{
		  accum[r][offs++] += *m * *k++;
		}
	    }
	  m++;
	}

      /* store the accum buffer into the destination mask */
      d = mask_buf_data (dest) + (i + dest_offset_y) * dest->width;
      for (j = 0; j < dest->width; j++)
	*d++ = (accum[0][j] + 127) / kernel_sum;

      rotate_pointers (accum, KERNEL_HEIGHT);

      memset (accum[KERNEL_HEIGHT - 1], 0, sizeof (gulong) * dest->width);
    }

  /* store the rest of the accum buffer into the dest mask */
  while (i + dest_offset_y < dest->height)
    {
      d = mask_buf_data (dest) + (i + dest_offset_y) * dest->width;
      for (j = 0; j < dest->width; j++)
	*d++ = (accum[0][j] + (kernel_sum / 2)) / kernel_sum;

      rotate_pointers (accum, KERNEL_HEIGHT);
      i++;
    }

  for (i = 0; i < KERNEL_HEIGHT ; i++)
    g_free (accum[i]);

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
                               MaskBuf       *brush_mask,
                               gdouble        x,
                               gdouble        y)
{
  MaskBuf *dest;
  guchar  *m;
  guchar  *d;
  gint     dest_offset_x = 0;
  gint     dest_offset_y = 0;
  gint     i, j;

  if ((brush_mask->width % 2) == 0)
    {
      while (x < 0) x += brush_mask->width;

      if ((x - floor (x)) >= 0.5)
        dest_offset_x++;
    }

  if ((brush_mask->height % 2) == 0)
    {
      while (y < 0) y += brush_mask->height;

      if ((y - floor (y)) >= 0.5)
        dest_offset_y++;
    }

  if (brush_mask == core->last_solid_brush && ! core->solid_cache_invalid)
    {
      if (core->solid_brushes[dest_offset_y][dest_offset_x])
        return core->solid_brushes[dest_offset_y][dest_offset_x];
    }
  else
    {
      for (i = 0; i < PAINT_CORE_SOLID_SUBSAMPLE; i++)
	for (j = 0; j < PAINT_CORE_SOLID_SUBSAMPLE; j++)
          if (core->solid_brushes[i][j])
            {
              mask_buf_free (core->solid_brushes[i][j]);
              core->solid_brushes[i][j] = NULL;
            }

      core->last_solid_brush    = brush_mask;
      core->solid_cache_invalid = FALSE;
    }

  dest = mask_buf_new (brush_mask->width  + 2,
                       brush_mask->height + 2);

  core->solid_brushes[dest_offset_y][dest_offset_x] = dest;

  m = mask_buf_data (brush_mask);
  d = (mask_buf_data (dest) +
       (dest_offset_y + 1) * dest->width +
       (dest_offset_x + 1));

  for (i = 0; i < brush_mask->height; i++)
    {
      for (j = 0; j < brush_mask->width; j++)
	{
	  *d++ = (*m++) ? OPAQUE_OPACITY : TRANSPARENT_OPACITY;
	}

      d += 2;
    }

  return dest;
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

  gimp_paint_core_calc_brush_size (core,
                                   brush_mask,
                                   scale,
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

  core->cache_invalid       = TRUE;
  core->solid_cache_invalid = TRUE;

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

  gimp_paint_core_calc_brush_size (core,
                                   brush_mask,
                                   scale,
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
gimp_paint_core_get_brush_mask (GimpPaintCore            *core,
				GimpBrushApplicationMode  brush_hardness,
				gdouble                   scale)
{
  MaskBuf *mask;

  if (core->use_pressure)
    mask = gimp_paint_core_scale_mask (core, core->brush->mask, scale);
  else
    mask = core->brush->mask;

  if (!mask)
    return NULL;

  switch (brush_hardness)
    {
    case GIMP_BRUSH_SOFT:
      mask = gimp_paint_core_subsample_mask (core, mask,
					     core->cur_coords.x,
                                             core->cur_coords.y);
      break;

    case GIMP_BRUSH_HARD:
      mask = gimp_paint_core_solidify_mask (core, mask,
                                            core->cur_coords.x,
                                            core->cur_coords.y);
      break;

    case GIMP_BRUSH_PRESSURE:
      if (core->use_pressure)
        mask = gimp_paint_core_pressurize_mask (core, mask,
                                                core->cur_coords.x,
                                                core->cur_coords.y,
                                                core->cur_coords.pressure);
      else
        mask = gimp_paint_core_subsample_mask (core, mask,
                                               core->cur_coords.x,
                                               core->cur_coords.y);
      break;

    default:
      break;
    }

  return mask;
}

static void
gimp_paint_core_paste (GimpPaintCore            *core,
		       MaskBuf                  *brush_mask,
		       GimpDrawable             *drawable,
		       gdouble                   brush_opacity,
		       gdouble                   image_opacity,
		       GimpLayerModeEffects      paint_mode,
		       GimpPaintApplicationMode  mode)
{
  GimpImage   *gimage;
  PixelRegion  srcPR;
  TileManager *alt = NULL;
  gint         offx;
  gint         offy;

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  set undo blocks  */
  set_undo_tiles (core,
                  drawable,
		  core->canvas_buf->x,
                  core->canvas_buf->y,
		  core->canvas_buf->width,
                  core->canvas_buf->height);

  /*  If the mode is CONSTANT:
   *   combine the canvas buf, the brush mask to the canvas tiles
   */
  if (mode == GIMP_PAINT_CONSTANT)
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
  gimp_drawable_apply_region (drawable, &srcPR,
                              FALSE, NULL,
                              image_opacity, paint_mode,
                              alt,  /*  specify an alternative src1  */
                              core->canvas_buf->x,
                              core->canvas_buf->y);

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->canvas_buf->x);
  core->y1 = MIN (core->y1, core->canvas_buf->y);
  core->x2 = MAX (core->x2, core->canvas_buf->x + core->canvas_buf->width);
  core->y2 = MAX (core->y2, core->canvas_buf->y + core->canvas_buf->height);

  /*  Update the gimage -- It is important to call gimp_image_update()
   *  instead of gimp_drawable_update() because we don't want the
   *  drawable and image previews to be constantly invalidated
   */
  gimp_item_offsets (GIMP_ITEM (drawable), &offx, &offy);
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
gimp_paint_core_replace (GimpPaintCore            *core,
			 MaskBuf                  *brush_mask,
			 GimpDrawable             *drawable,
			 gdouble                   brush_opacity,
			 gdouble                   image_opacity,
			 GimpPaintApplicationMode  mode)
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
			     brush_opacity,
                             image_opacity, GIMP_NORMAL_MODE,
			     mode);
      return;
    }

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  set undo blocks  */
  set_undo_tiles (core,
                  drawable,
		  core->canvas_buf->x,
                  core->canvas_buf->y,
		  core->canvas_buf->width,
                  core->canvas_buf->height);

  if (mode == GIMP_PAINT_CONSTANT)
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
  gimp_drawable_replace_region (drawable, &srcPR,
                                FALSE, NULL,
                                image_opacity,
                                &maskPR,
                                core->canvas_buf->x,
                                core->canvas_buf->y);

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->canvas_buf->x);
  core->y1 = MIN (core->y1, core->canvas_buf->y);
  core->x2 = MAX (core->x2, core->canvas_buf->x + core->canvas_buf->width) ;
  core->y2 = MAX (core->y2, core->canvas_buf->y + core->canvas_buf->height) ;

  /*  Update the gimage -- It is important to call gimp_image_update()
   *  instead of gimp_drawable_update() because we don't want the
   *  drawable and image previews to be constantly invalidated
   */
  gimp_item_offsets (GIMP_ITEM (drawable), &offx, &offy);
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
		       gdouble        brush_opacity)
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
  combine_mask_and_region (&srcPR, &maskPR, brush_opacity * 255.999);
}

static void
brush_to_canvas_buf (GimpPaintCore *core,
		     MaskBuf       *brush_mask,
		     gdouble        brush_opacity)
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
  apply_mask_to_region (&srcPR, &maskPR, brush_opacity * 255.999);
}

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

	  if (! tile_is_valid (dest_tile))
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

	  if (! tile_is_valid (tile))
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
gimp_paint_core_color_area_with_pixmap (GimpPaintCore            *core,
					GimpImage                *dest,
					GimpDrawable             *drawable,
					TempBuf                  *area,
					gdouble                   scale,
					GimpBrushApplicationMode  mode)
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

  if (!pixmap_mask)
    return;

  if (mode != GIMP_BRUSH_HARD)
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
paint_line_pixmap_mask (GimpImage                *dest,
			GimpDrawable             *drawable,
			TempBuf                  *pixmap_mask,
			TempBuf                  *brush_mask,
			guchar                   *d,
			gint                      x,
			gint                      y,
			gint                      bytes,
			gint                      width,
			GimpBrushApplicationMode  mode)
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

  if (mode == GIMP_BRUSH_SOFT && brush_mask)
    {
      /* ditto, except for the brush mask,
         so we can pre-multiply the alpha value */
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
	  gimp_image_transform_color (dest, drawable, d, GIMP_RGB, p);
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
	  gimp_image_transform_color (dest, drawable, d, GIMP_RGB, p);
	  d += bytes;
	}
    }
}
