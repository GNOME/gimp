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

#include <string.h>

#include <glib-object.h>

#include "paint-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimppickable.h"

#include "gimppaintcore.h"
#include "gimppaintcoreundo.h"
#include "gimppaintoptions.h"

#include "gimpairbrush.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_UNDO_DESC
};


/*  local function prototypes  */

static void      gimp_paint_core_finalize            (GObject          *object);
static void      gimp_paint_core_set_property        (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void      gimp_paint_core_get_property        (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static gboolean  gimp_paint_core_real_start          (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *paint_options,
                                                      GimpCoords       *coords,
                                                      GError          **error);
static gboolean  gimp_paint_core_real_pre_paint      (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *options,
                                                      GimpPaintState    paint_state,
                                                      guint32           time);
static void      gimp_paint_core_real_paint          (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *options,
                                                      GimpPaintState    paint_state,
                                                      guint32           time);
static void      gimp_paint_core_real_post_paint     (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *options,
                                                      GimpPaintState    paint_state,
                                                      guint32           time);
static void      gimp_paint_core_real_interpolate    (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *options,
                                                      guint32           time);
static TempBuf * gimp_paint_core_real_get_paint_area (GimpPaintCore    *core,
                                                      GimpDrawable     *drawable,
                                                      GimpPaintOptions *options);
static GimpUndo* gimp_paint_core_real_push_undo      (GimpPaintCore    *core,
                                                      GimpImage        *image,
                                                      const gchar      *undo_desc);

static void      paint_mask_to_canvas_tiles          (GimpPaintCore    *core,
                                                      PixelRegion      *paint_maskPR,
                                                      gdouble           paint_opacity);
static void      paint_mask_to_canvas_buf            (GimpPaintCore    *core,
                                                      PixelRegion      *paint_maskPR,
                                                      gdouble           paint_opacity);
static void      canvas_tiles_to_canvas_buf          (GimpPaintCore    *core);


G_DEFINE_TYPE (GimpPaintCore, gimp_paint_core, GIMP_TYPE_OBJECT)

#define parent_class gimp_paint_core_parent_class

static gint global_core_ID = 1;


static void
gimp_paint_core_class_init (GimpPaintCoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_paint_core_finalize;
  object_class->set_property = gimp_paint_core_set_property;
  object_class->get_property = gimp_paint_core_get_property;

  klass->start               = gimp_paint_core_real_start;
  klass->pre_paint           = gimp_paint_core_real_pre_paint;
  klass->paint               = gimp_paint_core_real_paint;
  klass->post_paint          = gimp_paint_core_real_post_paint;
  klass->interpolate         = gimp_paint_core_real_interpolate;
  klass->get_paint_area      = gimp_paint_core_real_get_paint_area;
  klass->push_undo           = gimp_paint_core_real_push_undo;

  g_object_class_install_property (object_class, PROP_UNDO_DESC,
                                   g_param_spec_string ("undo-desc", NULL, NULL,
                                                        _("Paint"),
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_paint_core_init (GimpPaintCore *core)
{
  core->ID               = global_core_ID++;

  core->undo_desc        = NULL;

  core->distance         = 0.0;
  core->pixel_dist       = 0.0;
  core->x1               = 0;
  core->y1               = 0;
  core->x2               = 0;
  core->y2               = 0;

  core->use_pressure     = FALSE;
  core->use_saved_proj   = FALSE;

  core->undo_tiles       = NULL;
  core->saved_proj_tiles = NULL;
  core->canvas_tiles     = NULL;

  core->orig_buf         = NULL;
  core->orig_proj_buf    = NULL;
  core->canvas_buf       = NULL;
}

static void
gimp_paint_core_finalize (GObject *object)
{
  GimpPaintCore *core = GIMP_PAINT_CORE (object);

  gimp_paint_core_cleanup (core);

  g_free (core->undo_desc);
  core->undo_desc = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_paint_core_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpPaintCore *core = GIMP_PAINT_CORE (object);

  switch (property_id)
    {
    case PROP_UNDO_DESC:
      g_free (core->undo_desc);
      core->undo_desc = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_paint_core_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpPaintCore *core = GIMP_PAINT_CORE (object);

  switch (property_id)
    {
    case PROP_UNDO_DESC:
      g_value_set_string (value, core->undo_desc);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_paint_core_real_start (GimpPaintCore    *core,
                            GimpDrawable     *drawable,
                            GimpPaintOptions *paint_options,
                            GimpCoords       *coords,
                            GError          **error)
{
  return TRUE;
}

static gboolean
gimp_paint_core_real_pre_paint (GimpPaintCore    *core,
                                GimpDrawable     *drawable,
                                GimpPaintOptions *paint_options,
                                GimpPaintState    paint_state,
                                guint32           time)
{
  return TRUE;
}

static void
gimp_paint_core_real_paint (GimpPaintCore    *core,
                            GimpDrawable     *drawable,
                            GimpPaintOptions *paint_options,
                            GimpPaintState    paint_state,
                            guint32           time)
{
}

static void
gimp_paint_core_real_post_paint (GimpPaintCore    *core,
                                 GimpDrawable     *drawable,
                                 GimpPaintOptions *paint_options,
                                 GimpPaintState    paint_state,
                                 guint32           time)
{
}

static void
gimp_paint_core_real_interpolate (GimpPaintCore    *core,
                                  GimpDrawable     *drawable,
                                  GimpPaintOptions *paint_options,
                                  guint32           time)
{
  gimp_paint_core_paint (core, drawable, paint_options,
                         GIMP_PAINT_STATE_MOTION, time);

  core->last_coords = core->cur_coords;
}

static TempBuf *
gimp_paint_core_real_get_paint_area (GimpPaintCore    *core,
                                     GimpDrawable     *drawable,
                                     GimpPaintOptions *paint_options)
{
  return NULL;
}

static GimpUndo *
gimp_paint_core_real_push_undo (GimpPaintCore *core,
                                GimpImage     *image,
                                const gchar   *undo_desc)
{
  return gimp_image_undo_push (image, GIMP_TYPE_PAINT_CORE_UNDO,
                               GIMP_UNDO_PAINT, undo_desc,
                               0,
                               "paint-core", core,
                               NULL);
}


/*  public functions  */

void
gimp_paint_core_paint (GimpPaintCore    *core,
                       GimpDrawable     *drawable,
                       GimpPaintOptions *paint_options,
                       GimpPaintState    paint_state,
                       guint32           time)
{
  GimpPaintCoreClass *core_class;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options));

  core_class = GIMP_PAINT_CORE_GET_CLASS (core);

  if (core_class->pre_paint (core, drawable,
                             paint_options,
                             paint_state, time))
    {
      if (paint_state == GIMP_PAINT_STATE_MOTION)
        {
          /* Save coordinates for gimp_paint_core_interpolate() */
          core->last_paint.x = core->cur_coords.x;
          core->last_paint.y = core->cur_coords.y;
        }

      core_class->paint (core, drawable,
                         paint_options,
                         paint_state, time);

      core_class->post_paint (core, drawable,
                              paint_options,
                              paint_state, time);
    }
}

gboolean
gimp_paint_core_start (GimpPaintCore     *core,
                       GimpDrawable      *drawable,
                       GimpPaintOptions  *paint_options,
                       GimpCoords        *coords,
                       GError           **error)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item = GIMP_ITEM (drawable);

  core->cur_coords = *coords;

  if (! GIMP_PAINT_CORE_GET_CLASS (core)->start (core, drawable,
                                                 paint_options,
                                                 coords, error))
    {
      return FALSE;
    }

  /*  Allocate the undo structure  */
  if (core->undo_tiles)
    tile_manager_unref (core->undo_tiles);

  core->undo_tiles = tile_manager_new (gimp_item_width (item),
                                       gimp_item_height (item),
                                       gimp_drawable_bytes (drawable));

  /*  Allocate the saved proj structure  */
  if (core->saved_proj_tiles)
    tile_manager_unref (core->saved_proj_tiles);

  core->saved_proj_tiles = NULL;

  if (core->use_saved_proj)
    {
      GimpPickable *pickable;
      TileManager  *tiles;

      pickable = GIMP_PICKABLE (gimp_item_get_image (item)->projection);
      tiles    = gimp_pickable_get_tiles (pickable);

      core->saved_proj_tiles = tile_manager_new (tile_manager_width (tiles),
                                                 tile_manager_height (tiles),
                                                 tile_manager_bpp (tiles));
    }

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
  GimpImage *image;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((core->x2 == core->x1) || (core->y2 == core->y1))
    return;

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_PAINT, core->undo_desc);

  GIMP_PAINT_CORE_GET_CLASS (core)->push_undo (core, image, NULL);

  gimp_drawable_push_undo (drawable, NULL,
                           core->x1, core->y1,
                           core->x2, core->y2,
                           core->undo_tiles,
                           TRUE);

  tile_manager_unref (core->undo_tiles);
  core->undo_tiles = NULL;

  gimp_image_undo_group_end (image);

  if (core->saved_proj_tiles)
    {
      tile_manager_unref (core->saved_proj_tiles);
      core->saved_proj_tiles = NULL;
    }

  /*  invalidate the drawable preview -- have to do it here, because
   *  it is not done during the actual painting.
   */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (drawable));
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
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((core->x2 == core->x1) || (core->y2 == core->y1))
    return;

  gimp_paint_core_copy_valid_tiles (core->undo_tiles,
                                    gimp_drawable_get_tiles (drawable),
                                    core->x1, core->y1,
                                    core->x2 - core->x1,
                                    core->y2 - core->y1);

  tile_manager_unref (core->undo_tiles);
  core->undo_tiles = NULL;

  if (core->saved_proj_tiles)
    {
      tile_manager_unref (core->saved_proj_tiles);
      core->saved_proj_tiles = NULL;
    }

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

  if (core->saved_proj_tiles)
    {
      tile_manager_unref (core->saved_proj_tiles);
      core->saved_proj_tiles = NULL;
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

  if (core->orig_proj_buf)
    {
      temp_buf_free (core->orig_proj_buf);
      core->orig_proj_buf = NULL;
    }

  if (core->canvas_buf)
    {
      temp_buf_free (core->canvas_buf);
      core->canvas_buf = NULL;
    }
}

void
gimp_paint_core_interpolate (GimpPaintCore    *core,
                             GimpDrawable     *drawable,
                             GimpPaintOptions *paint_options,
                             guint32           time)
{
  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options));

  GIMP_PAINT_CORE_GET_CLASS (core)->interpolate (core, drawable,
                                                 paint_options, time);
}


/*  protected functions  */

TempBuf *
gimp_paint_core_get_paint_area (GimpPaintCore    *core,
                                GimpDrawable     *drawable,
                                GimpPaintOptions *paint_options)
{
  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_PAINT_OPTIONS (paint_options), NULL);

  return GIMP_PAINT_CORE_GET_CLASS (core)->get_paint_area (core, drawable,
                                                           paint_options);
}

TempBuf *
gimp_paint_core_get_orig_image (GimpPaintCore *core,
                                GimpDrawable  *drawable,
                                gint           x1,
                                gint           y1,
                                gint           x2,
                                gint           y2)
{
  PixelRegion   srcPR;
  PixelRegion   destPR;
  Tile         *undo_tile;
  gboolean      release_tile;
  gint          h;
  gint          pixelwidth;
  gint          drawable_width;
  gint          drawable_height;
  const guchar *s;
  guchar       *d;
  gpointer      pr;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (core->undo_tiles != NULL, NULL);

  core->orig_buf = temp_buf_resize (core->orig_buf,
                                    gimp_drawable_bytes (drawable),
                                    x1, y1,
                                    (x2 - x1), (y2 - y1));

  drawable_width  = gimp_item_width  (GIMP_ITEM (drawable));
  drawable_height = gimp_item_height (GIMP_ITEM (drawable));

  x1 = CLAMP (x1, 0, drawable_width);
  y1 = CLAMP (y1, 0, drawable_height);
  x2 = CLAMP (x2, 0, drawable_width);
  y2 = CLAMP (y2, 0, drawable_height);

  /*  configure the pixel regions  */
  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     x1, y1,
                     (x2 - x1), (y2 - y1),
                     FALSE);

  pixel_region_init_temp_buf (&destPR, core->orig_buf,
                              x1 - core->orig_buf->x,
                              y1 - core->orig_buf->y,
                              x2 - x1,
                              y2 - y1);

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
          release_tile = TRUE;

          undo_tile = tile_manager_get_tile (core->undo_tiles,
                                             srcPR.x, srcPR.y,
                                             TRUE, FALSE);
          s = tile_data_pointer (undo_tile,
                                 srcPR.x % TILE_WIDTH, srcPR.y % TILE_HEIGHT);
        }
      else
        {
          release_tile = FALSE;

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

      if (release_tile)
        tile_release (undo_tile, FALSE);
    }

  return core->orig_buf;
}

TempBuf *
gimp_paint_core_get_orig_proj (GimpPaintCore *core,
                               GimpPickable  *pickable,
                               gint           x1,
                               gint           y1,
                               gint           x2,
                               gint           y2)
{
  TileManager  *src_tiles;
  PixelRegion   srcPR;
  PixelRegion   destPR;
  Tile         *saved_tile;
  gboolean      release_tile;
  gint          h;
  gint          pixelwidth;
  gint          pickable_width;
  gint          pickable_height;
  const guchar *s;
  guchar       *d;
  gpointer      pr;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);
  g_return_val_if_fail (core->saved_proj_tiles != NULL, NULL);

  core->orig_proj_buf = temp_buf_resize (core->orig_proj_buf,
                                         gimp_pickable_get_bytes (pickable),
                                         x1, y1,
                                         (x2 - x1), (y2 - y1));

  src_tiles = gimp_pickable_get_tiles (pickable);

  pickable_width  = tile_manager_width  (src_tiles);
  pickable_height = tile_manager_height (src_tiles);

  x1 = CLAMP (x1, 0, pickable_width);
  y1 = CLAMP (y1, 0, pickable_height);
  x2 = CLAMP (x2, 0, pickable_width);
  y2 = CLAMP (y2, 0, pickable_height);

  /*  configure the pixel regions  */
  pixel_region_init (&srcPR, src_tiles,
                     x1, y1,
                     (x2 - x1), (y2 - y1),
                     FALSE);

  pixel_region_init_temp_buf (&destPR, core->orig_proj_buf,
                              x1 - core->orig_proj_buf->x,
                              y1 - core->orig_proj_buf->y,
                              x2 - x1,
                              y2 - y1);

  for (pr = pixel_regions_register (2, &srcPR, &destPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      /*  If the saved tile corresponding to this location is valid, use it  */
      saved_tile = tile_manager_get_tile (core->saved_proj_tiles,
                                          srcPR.x, srcPR.y,
                                          FALSE, FALSE);

      if (tile_is_valid (saved_tile))
        {
          release_tile = TRUE;

          saved_tile = tile_manager_get_tile (core->saved_proj_tiles,
                                              srcPR.x, srcPR.y,
                                              TRUE, FALSE);
          s = tile_data_pointer (saved_tile,
                                 srcPR.x % TILE_WIDTH, srcPR.y % TILE_HEIGHT);
        }
      else
        {
          release_tile = FALSE;

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

      if (release_tile)
        tile_release (saved_tile, FALSE);
    }

  return core->orig_proj_buf;
}

void
gimp_paint_core_paste (GimpPaintCore            *core,
                       PixelRegion              *paint_maskPR,
                       GimpDrawable             *drawable,
                       gdouble                   paint_opacity,
                       gdouble                   image_opacity,
                       GimpLayerModeEffects      paint_mode,
                       GimpPaintApplicationMode  mode)
{
  GimpImage   *image;
  PixelRegion  srcPR;
  TileManager *alt = NULL;
  gint         offx;
  gint         offy;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  set undo blocks  */
  gimp_paint_core_validate_undo_tiles (core, drawable,
                                       core->canvas_buf->x,
                                       core->canvas_buf->y,
                                       core->canvas_buf->width,
                                       core->canvas_buf->height);

  if (core->use_saved_proj)
    {
      GimpPickable *pickable = GIMP_PICKABLE (image->projection);
      gint          off_x;
      gint          off_y;

      gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

      gimp_paint_core_validate_saved_proj_tiles (core, pickable,
                                                 core->canvas_buf->x + off_x,
                                                 core->canvas_buf->y + off_y,
                                                 core->canvas_buf->width,
                                                 core->canvas_buf->height);
    }

  /*  If the mode is CONSTANT:
   *   combine the canvas buf, the paint mask to the canvas tiles
   */
  if (mode == GIMP_PAINT_CONSTANT)
    {
      /* Some tools (ink) paint the mask to paint_core->canvas_tiles
       * directly. Don't need to copy it in this case.
       */
      if (paint_maskPR->tiles != core->canvas_tiles)
        {
          /*  initialize any invalid canvas tiles  */
          gimp_paint_core_validate_canvas_tiles (core,
                                                 core->canvas_buf->x,
                                                 core->canvas_buf->y,
                                                 core->canvas_buf->width,
                                                 core->canvas_buf->height);

          paint_mask_to_canvas_tiles (core, paint_maskPR, paint_opacity);
        }

      canvas_tiles_to_canvas_buf (core);
      alt = core->undo_tiles;
    }
  /*  Otherwise:
   *   combine the canvas buf and the paint mask to the canvas buf
   */
  else
    {
      paint_mask_to_canvas_buf (core, paint_maskPR, paint_opacity);
    }

  /*  intialize canvas buf source pixel regions  */
  pixel_region_init_temp_buf (&srcPR, core->canvas_buf,
                              0, 0,
                              core->canvas_buf->width,
                              core->canvas_buf->height);

  /*  apply the paint area to the image  */
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

  /*  Update the image -- It is important to call gimp_image_update()
   *  instead of gimp_drawable_update() because we don't want the
   *  drawable and image previews to be constantly invalidated
   */
  gimp_item_offsets (GIMP_ITEM (drawable), &offx, &offy);
  gimp_image_update (image,
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
void
gimp_paint_core_replace (GimpPaintCore            *core,
                         PixelRegion              *paint_maskPR,
                         GimpDrawable             *drawable,
                         gdouble                   paint_opacity,
                         gdouble                   image_opacity,
                         GimpPaintApplicationMode  mode)
{
  GimpImage   *image;
  PixelRegion  srcPR;
  gint         offx;
  gint         offy;

  if (! gimp_drawable_has_alpha (drawable))
    {
      gimp_paint_core_paste (core, paint_maskPR, drawable,
                             paint_opacity,
                             image_opacity, GIMP_NORMAL_MODE,
                             mode);
      return;
    }

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  set undo blocks  */
  gimp_paint_core_validate_undo_tiles (core, drawable,
                                       core->canvas_buf->x,
                                       core->canvas_buf->y,
                                       core->canvas_buf->width,
                                       core->canvas_buf->height);

  if (mode == GIMP_PAINT_CONSTANT)
    {
      /* Some tools (ink) paint the mask to paint_core->canvas_tiles
       * directly. Don't need to copy it in this case.
       */
      if (paint_maskPR->tiles != core->canvas_tiles)
        {
          /*  initialize any invalid canvas tiles  */
          gimp_paint_core_validate_canvas_tiles (core,
                                                 core->canvas_buf->x,
                                                 core->canvas_buf->y,
                                                 core->canvas_buf->width,
                                                 core->canvas_buf->height);

          /* combine the paint mask and the canvas tiles */
          paint_mask_to_canvas_tiles (core, paint_maskPR, paint_opacity);

          /* initialize the maskPR from the canvas tiles */
          pixel_region_init (paint_maskPR, core->canvas_tiles,
                             core->canvas_buf->x,
                             core->canvas_buf->y,
                             core->canvas_buf->width,
                             core->canvas_buf->height,
                             FALSE);
        }
    }
  else
    {
      /* The mask is just the paint_maskPR */
    }

  /*  intialize canvas buf source pixel regions  */
  pixel_region_init_temp_buf (&srcPR, core->canvas_buf,
                              0, 0,
                              core->canvas_buf->width,
                              core->canvas_buf->height);

  /*  apply the paint area to the image  */
  gimp_drawable_replace_region (drawable, &srcPR,
                                FALSE, NULL,
                                image_opacity,
                                paint_maskPR,
                                core->canvas_buf->x,
                                core->canvas_buf->y);

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->canvas_buf->x);
  core->y1 = MIN (core->y1, core->canvas_buf->y);
  core->x2 = MAX (core->x2, core->canvas_buf->x + core->canvas_buf->width) ;
  core->y2 = MAX (core->y2, core->canvas_buf->y + core->canvas_buf->height) ;

  /*  Update the image -- It is important to call gimp_image_update()
   *  instead of gimp_drawable_update() because we don't want the
   *  drawable and image previews to be constantly invalidated
   */
  gimp_item_offsets (GIMP_ITEM (drawable), &offx, &offy);
  gimp_image_update (image,
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
  pixel_region_init_temp_buf (&srcPR, core->canvas_buf,
                              0, 0,
                              core->canvas_buf->width,
                              core->canvas_buf->height);

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
paint_mask_to_canvas_tiles (GimpPaintCore *core,
                            PixelRegion   *paint_maskPR,
                            gdouble        paint_opacity)
{
  PixelRegion srcPR;

  /*   combine the paint mask and the canvas tiles  */
  pixel_region_init (&srcPR, core->canvas_tiles,
                     core->canvas_buf->x,
                     core->canvas_buf->y,
                     core->canvas_buf->width,
                     core->canvas_buf->height,
                     TRUE);

  /*  combine the mask to the canvas tiles  */
  combine_mask_and_region (&srcPR, paint_maskPR,
                           paint_opacity * 255.999, GIMP_IS_AIRBRUSH (core));
}

static void
paint_mask_to_canvas_buf (GimpPaintCore *core,
                          PixelRegion   *paint_maskPR,
                          gdouble        paint_opacity)
{
  PixelRegion srcPR;

  /*  combine the canvas buf and the paint mask to the canvas buf  */
  pixel_region_init_temp_buf (&srcPR, core->canvas_buf,
                              0, 0,
                              core->canvas_buf->width,
                              core->canvas_buf->height);

  /*  apply the mask  */
  apply_mask_to_region (&srcPR, paint_maskPR, paint_opacity * 255.999);
}

void
gimp_paint_core_validate_undo_tiles (GimpPaintCore *core,
                                     GimpDrawable  *drawable,
                                     gint           x,
                                     gint           y,
                                     gint           w,
                                     gint           h)
{
  gint i, j;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (core->undo_tiles != NULL);

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
        {
          Tile *dest_tile = tile_manager_get_tile (core->undo_tiles,
                                                   j, i, FALSE, FALSE);

          if (! tile_is_valid (dest_tile))
            {
              Tile *src_tile =
                tile_manager_get_tile (gimp_drawable_get_tiles (drawable),
                                       j, i, TRUE, FALSE);
              tile_manager_map_tile (core->undo_tiles, j, i, src_tile);
              tile_release (src_tile, FALSE);
            }
        }
    }
}

void
gimp_paint_core_validate_saved_proj_tiles (GimpPaintCore *core,
                                           GimpPickable  *pickable,
                                           gint           x,
                                           gint           y,
                                           gint           w,
                                           gint           h)
{
  gint i, j;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (GIMP_IS_PICKABLE (pickable));
  g_return_if_fail (core->saved_proj_tiles != NULL);

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
        {
          Tile *dest_tile = tile_manager_get_tile (core->saved_proj_tiles,
                                                   j, i, FALSE, FALSE);

          if (! tile_is_valid (dest_tile))
            {
              Tile *src_tile =
                tile_manager_get_tile (gimp_pickable_get_tiles (pickable),
                                       j, i, TRUE, FALSE);

              /* copy the pixels instead of mapping the tile because
               * copy-on-write from the projection is broken
               */
              dest_tile = tile_manager_get_tile (core->saved_proj_tiles,
                                                 j, i, TRUE, TRUE);
              memcpy (tile_data_pointer (dest_tile, 0, 0),
                      tile_data_pointer (src_tile, 0, 0),
                      tile_size (src_tile));
              tile_release (dest_tile, TRUE);
              tile_release (src_tile, FALSE);
            }
        }
    }
}

void
gimp_paint_core_validate_canvas_tiles (GimpPaintCore *core,
                                       gint           x,
                                       gint           y,
                                       gint           w,
                                       gint           h)
{
  gint i, j;

  g_return_if_fail (GIMP_IS_PAINT_CORE (core));
  g_return_if_fail (core->canvas_tiles != NULL);

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
        {
          Tile *tile = tile_manager_get_tile (core->canvas_tiles, j, i,
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
