/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball, Peter Mattis, and others
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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimp-transform-region.h"
#include "gimp-transform-resize.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-transform.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimppickable.h"
#include "gimpprogress.h"
#include "gimpselection.h"

#include "gimp-intl.h"


#if defined (HAVE_FINITE)
#define FINITE(x) finite(x)
#elif defined (HAVE_ISFINITE)
#define FINITE(x) isfinite(x)
#elif defined (G_OS_WIN32)
#define FINITE(x) _finite(x)
#else
#error "no FINITE() implementation available?!"
#endif


#define MIN4(a,b,c,d) MIN(MIN(a,b),MIN(c,d))
#define MAX4(a,b,c,d) MAX(MAX(a,b),MAX(c,d))


/*  public functions  */

TileManager *
gimp_drawable_transform_tiles_affine (GimpDrawable           *drawable,
                                      GimpContext            *context,
                                      TileManager            *orig_tiles,
                                      const GimpMatrix3      *matrix,
                                      GimpTransformDirection  direction,
                                      GimpInterpolationType   interpolation_type,
                                      gint                    recursion_level,
                                      GimpTransformResize     clip_result,
                                      GimpProgress           *progress)
{
  GimpImage   *image;
  PixelRegion  destPR;
  TileManager *new_tiles;
  GimpMatrix3  m;
  GimpMatrix3  inv;
  gint         u1, v1, u2, v2;  /* source bounding box */
  gint         x1, y1, x2, y2;  /* target bounding box */

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (orig_tiles != NULL, NULL);
  g_return_val_if_fail (matrix != NULL, NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  m   = *matrix;
  inv = *matrix;

  if (direction == GIMP_TRANSFORM_BACKWARD)
    {
      /*  keep the original matrix here, so we dont need to recalculate
       *  the inverse later
       */
      gimp_matrix3_invert (&inv);
    }
  else
    {
      /*  Find the inverse of the transformation matrix  */
      gimp_matrix3_invert (&m);
    }

  tile_manager_get_offsets (orig_tiles, &u1, &v1);

  u2 = u1 + tile_manager_width (orig_tiles);
  v2 = v1 + tile_manager_height (orig_tiles);

  /*  Always clip unfloated tiles since they must keep their size  */
  if (G_TYPE_FROM_INSTANCE (drawable) == GIMP_TYPE_CHANNEL &&
      tile_manager_bpp (orig_tiles)   == 1)
    clip_result = GIMP_TRANSFORM_RESIZE_CLIP;

  /*  Find the bounding coordinates of target */
  gimp_transform_resize_boundary (&inv, clip_result,
                                  u1, v1, u2, v2,
                                  &x1, &y1, &x2, &y2);

  /*  Get the new temporary buffer for the transformed result  */
  new_tiles = tile_manager_new (x2 - x1, y2 - y1,
                                tile_manager_bpp (orig_tiles));
  pixel_region_init (&destPR, new_tiles,
                     0, 0, x2 - x1, y2 - y1, TRUE);
  tile_manager_set_offsets (new_tiles, x1, y1);

  gimp_transform_region (GIMP_PICKABLE (drawable),
                         context,
                         orig_tiles,
                         &destPR,
                         x1,
                         y1,
                         x2,
                         y2,
                         &inv,
                         interpolation_type,
                         recursion_level,
                         progress);

  return new_tiles;
}

TileManager *
gimp_drawable_transform_tiles_flip (GimpDrawable        *drawable,
                                    GimpContext         *context,
                                    TileManager         *orig_tiles,
                                    GimpOrientationType  flip_type,
                                    gdouble              axis,
                                    gboolean             clip_result)
{
  GimpImage   *image;
  TileManager *new_tiles;
  PixelRegion  srcPR, destPR;
  gint         orig_x, orig_y;
  gint         orig_width, orig_height;
  gint         orig_bpp;
  gint         new_x, new_y;
  gint         new_width, new_height;
  gint         i;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (orig_tiles != NULL, NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  orig_width  = tile_manager_width (orig_tiles);
  orig_height = tile_manager_height (orig_tiles);
  orig_bpp    = tile_manager_bpp (orig_tiles);

  tile_manager_get_offsets (orig_tiles, &orig_x, &orig_y);

  new_x      = orig_x;
  new_y      = orig_y;
  new_width  = orig_width;
  new_height = orig_height;

  switch (flip_type)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      new_x = RINT (-((gdouble) orig_x +
                      (gdouble) orig_width - axis) + axis);
      break;

    case GIMP_ORIENTATION_VERTICAL:
      new_y = RINT (-((gdouble) orig_y +
                      (gdouble) orig_height - axis) + axis);
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      g_return_val_if_reached (NULL);
      break;
    }

  new_tiles = tile_manager_new (new_width, new_height, orig_bpp);

  if (clip_result && (new_x != orig_x || new_y != orig_y))
    {
      guchar bg_color[MAX_CHANNELS];
      gint   clip_x, clip_y;
      gint   clip_width, clip_height;

      tile_manager_set_offsets (new_tiles, orig_x, orig_y);

      gimp_image_get_background (image, context, gimp_drawable_type (drawable),
                                 bg_color);

      /*  "Outside" a channel is transparency, not the bg color  */
      if (GIMP_IS_CHANNEL (drawable))
        bg_color[0] = TRANSPARENT_OPACITY;

      pixel_region_init (&destPR, new_tiles,
                         0, 0, new_width, new_height, TRUE);
      color_region (&destPR, bg_color);

      if (gimp_rectangle_intersect (orig_x, orig_y, orig_width, orig_height,
                                    new_x, new_y, new_width, new_height,
                                    &clip_x, &clip_y,
                                    &clip_width, &clip_height))
        {
          orig_x = new_x = clip_x - orig_x;
          orig_y = new_y = clip_y - orig_y;
        }

      orig_width  = new_width  = clip_width;
      orig_height = new_height = clip_height;
    }
  else
    {
      tile_manager_set_offsets (new_tiles, new_x, new_y);

      orig_x = 0;
      orig_y = 0;
      new_x  = 0;
      new_y  = 0;
    }

  if (new_width == 0 && new_height == 0)
    return new_tiles;

  switch (flip_type)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      for (i = 0; i < orig_width; i++)
        {
          pixel_region_init (&srcPR, orig_tiles,
                             i + orig_x, orig_y,
                             1, orig_height, FALSE);
          pixel_region_init (&destPR, new_tiles,
                             new_x + new_width - i - 1, new_y,
                             1, new_height, TRUE);
          copy_region (&srcPR, &destPR);
        }
      break;

    case GIMP_ORIENTATION_VERTICAL:
      for (i = 0; i < orig_height; i++)
        {
          pixel_region_init (&srcPR, orig_tiles,
                             orig_x, i + orig_y,
                             orig_width, 1, FALSE);
          pixel_region_init (&destPR, new_tiles,
                             new_x, new_y + new_height - i - 1,
                             new_width, 1, TRUE);
          copy_region (&srcPR, &destPR);
        }
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      break;
    }

  return new_tiles;
}

static void
gimp_drawable_transform_rotate_point (gint              x,
                                      gint              y,
                                      GimpRotationType  rotate_type,
                                      gdouble           center_x,
                                      gdouble           center_y,
                                      gint             *new_x,
                                      gint             *new_y)
{
  g_return_if_fail (new_x != NULL);
  g_return_if_fail (new_y != NULL);

  switch (rotate_type)
    {
    case GIMP_ROTATE_90:
      *new_x = RINT (center_x - (gdouble) y + center_y);
      *new_y = RINT (center_y + (gdouble) x - center_x);
      break;

    case GIMP_ROTATE_180:
      *new_x = RINT (center_x - ((gdouble) x - center_x));
      *new_y = RINT (center_y - ((gdouble) y - center_y));
      break;

    case GIMP_ROTATE_270:
      *new_x = RINT (center_x + (gdouble) y - center_y);
      *new_y = RINT (center_y - (gdouble) x + center_x);
      break;

    default:
      g_assert_not_reached ();
    }
}

TileManager *
gimp_drawable_transform_tiles_rotate (GimpDrawable     *drawable,
                                      GimpContext      *context,
                                      TileManager      *orig_tiles,
                                      GimpRotationType  rotate_type,
                                      gdouble           center_x,
                                      gdouble           center_y,
                                      gboolean          clip_result)
{
  GimpImage   *image;
  TileManager *new_tiles;
  PixelRegion  srcPR, destPR;
  guchar      *buf = NULL;
  gint         orig_x, orig_y;
  gint         orig_width, orig_height;
  gint         orig_bpp;
  gint         new_x, new_y;
  gint         new_width, new_height;
  gint         i, j, k;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (orig_tiles != NULL, NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  orig_width  = tile_manager_width (orig_tiles);
  orig_height = tile_manager_height (orig_tiles);
  orig_bpp    = tile_manager_bpp (orig_tiles);

  tile_manager_get_offsets (orig_tiles, &orig_x, &orig_y);

  switch (rotate_type)
    {
    case GIMP_ROTATE_90:
      gimp_drawable_transform_rotate_point (orig_x,
                                            orig_y + orig_height,
                                            rotate_type, center_x, center_y,
                                            &new_x, &new_y);
      new_width  = orig_height;
      new_height = orig_width;
      break;

    case GIMP_ROTATE_180:
      gimp_drawable_transform_rotate_point (orig_x + orig_width,
                                            orig_y + orig_height,
                                            rotate_type, center_x, center_y,
                                            &new_x, &new_y);
      new_width  = orig_width;
      new_height = orig_height;
      break;

    case GIMP_ROTATE_270:
      gimp_drawable_transform_rotate_point (orig_x + orig_width,
                                            orig_y,
                                            rotate_type, center_x, center_y,
                                            &new_x, &new_y);
      new_width  = orig_height;
      new_height = orig_width;
      break;

    default:
      g_return_val_if_reached (NULL);
      break;
    }

  if (clip_result && (new_x != orig_x || new_y != orig_y ||
                      new_width != orig_width || new_height != orig_height))

    {
      guchar bg_color[MAX_CHANNELS];
      gint   clip_x, clip_y;
      gint   clip_width, clip_height;

      new_tiles = tile_manager_new (orig_width, orig_height, orig_bpp);

      tile_manager_set_offsets (new_tiles, orig_x, orig_y);

      gimp_image_get_background (image, context, gimp_drawable_type (drawable),
                                 bg_color);

      /*  "Outside" a channel is transparency, not the bg color  */
      if (GIMP_IS_CHANNEL (drawable))
        bg_color[0] = TRANSPARENT_OPACITY;

      pixel_region_init (&destPR, new_tiles,
                         0, 0, orig_width, orig_height, TRUE);
      color_region (&destPR, bg_color);

      if (gimp_rectangle_intersect (orig_x, orig_y, orig_width, orig_height,
                                    new_x, new_y, new_width, new_height,
                                    &clip_x, &clip_y,
                                    &clip_width, &clip_height))
        {
          gint saved_orig_x = orig_x;
          gint saved_orig_y = orig_y;

          new_x = clip_x - orig_x;
          new_y = clip_y - orig_y;

          switch (rotate_type)
            {
            case GIMP_ROTATE_90:
              gimp_drawable_transform_rotate_point (clip_x + clip_width,
                                                    clip_y,
                                                    GIMP_ROTATE_270,
                                                    center_x,
                                                    center_y,
                                                    &orig_x,
                                                    &orig_y);
              orig_x      -= saved_orig_x;
              orig_y      -= saved_orig_y;
              orig_width   = clip_height;
              orig_height  = clip_width;
              break;

            case GIMP_ROTATE_180:
              orig_x      = clip_x - orig_x;
              orig_y      = clip_y - orig_y;
              orig_width  = clip_width;
              orig_height = clip_height;
              break;

            case GIMP_ROTATE_270:
              gimp_drawable_transform_rotate_point (clip_x,
                                                    clip_y + clip_height,
                                                    GIMP_ROTATE_90,
                                                    center_x,
                                                    center_y,
                                                    &orig_x,
                                                    &orig_y);
              orig_x      -= saved_orig_x;
              orig_y      -= saved_orig_y;
              orig_width   = clip_height;
              orig_height  = clip_width;
              break;
            }
        }

      new_width  = clip_width;
      new_height = clip_height;
    }
  else
    {
      new_tiles = tile_manager_new (new_width, new_height, orig_bpp);

      tile_manager_set_offsets (new_tiles, new_x, new_y);

      orig_x = 0;
      orig_y = 0;
      new_x  = 0;
      new_y  = 0;
    }

  if (new_width == 0 && new_height == 0)
    return new_tiles;

  pixel_region_init (&srcPR, orig_tiles,
                     orig_x, orig_y, orig_width, orig_height, FALSE);
  pixel_region_init (&destPR, new_tiles,
                     new_x, new_y, new_width, new_height, TRUE);

  switch (rotate_type)
    {
    case GIMP_ROTATE_90:
      g_assert (new_height == orig_width);
      buf = g_new (guchar, new_height * orig_bpp);

      for (i = 0; i < orig_height; i++)
        {
          pixel_region_get_row (&srcPR, orig_x, orig_y + orig_height - 1 - i,
                                orig_width, buf, 1);
          pixel_region_set_col (&destPR, new_x + i, new_y, new_height, buf);
        }
      break;

    case GIMP_ROTATE_180:
      g_assert (new_width == orig_width);
      buf = g_new (guchar, new_width * orig_bpp);

      for (i = 0; i < orig_height; i++)
        {
          pixel_region_get_row (&srcPR, orig_x, orig_y + orig_height - 1 - i,
                                orig_width, buf, 1);

          for (j = 0; j < orig_width / 2; j++)
            {
              guchar *left  = buf + j * orig_bpp;
              guchar *right = buf + (orig_width - 1 - j) * orig_bpp;

              for (k = 0; k < orig_bpp; k++)
                {
                  guchar tmp = left[k];
                  left[k]    = right[k];
                  right[k]   = tmp;
                }
            }

          pixel_region_set_row (&destPR, new_x, new_y + i, new_width, buf);
        }
      break;

    case GIMP_ROTATE_270:
      g_assert (new_width == orig_height);
      buf = g_new (guchar, new_width * orig_bpp);

      for (i = 0; i < orig_width; i++)
        {
          pixel_region_get_col (&srcPR, orig_x + orig_width - 1 - i, orig_y,
                                orig_height, buf, 1);
          pixel_region_set_row (&destPR, new_x, new_y + i, new_width, buf);
        }
      break;
    }

  g_free (buf);

  return new_tiles;
}

gboolean
gimp_drawable_transform_affine (GimpDrawable           *drawable,
                                GimpContext            *context,
                                const GimpMatrix3      *matrix,
                                GimpTransformDirection  direction,
                                GimpInterpolationType   interpolation_type,
                                gint                    recursion_level,
                                GimpTransformResize     clip_result,
                                GimpProgress           *progress)
{
  GimpImage   *image;
  TileManager *orig_tiles;
  gboolean     new_layer;
  gboolean     success = FALSE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (matrix != NULL, FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /* Start a transform undo group */
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_TRANSFORM, _("Transform"));

  /* Cut/Copy from the specified drawable */
  orig_tiles = gimp_drawable_transform_cut (drawable, context, &new_layer);

  if (orig_tiles)
    {
      TileManager *new_tiles;

      /*  always clip unfloated tiles so they keep their size  */
      if (GIMP_IS_CHANNEL (drawable) && tile_manager_bpp (orig_tiles) == 1)
        clip_result = GIMP_TRANSFORM_RESIZE_CLIP;

      /* transform the buffer */
      new_tiles = gimp_drawable_transform_tiles_affine (drawable, context,
                                                        orig_tiles,
                                                        matrix,
                                                        GIMP_TRANSFORM_FORWARD,
                                                        interpolation_type,
                                                        recursion_level,
                                                        clip_result,
                                                        progress);

      /* Free the cut/copied buffer */
      tile_manager_unref (orig_tiles);

      if (new_tiles)
        {
          success = gimp_drawable_transform_paste (drawable, new_tiles,
                                                   new_layer);
          tile_manager_unref (new_tiles);
        }
    }

  /*  push the undo group end  */
  gimp_image_undo_group_end (image);

  return success;
}

gboolean
gimp_drawable_transform_flip (GimpDrawable        *drawable,
                              GimpContext         *context,
                              GimpOrientationType  flip_type,
                              gboolean             auto_center,
                              gdouble              axis,
                              gboolean             clip_result)
{
  GimpImage   *image;
  TileManager *orig_tiles;
  gboolean     new_layer;
  gboolean     success = FALSE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /* Start a transform undo group */
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_TRANSFORM, Q_("command|Flip"));

  /* Cut/Copy from the specified drawable */
  orig_tiles = gimp_drawable_transform_cut (drawable, context, &new_layer);

  if (orig_tiles)
    {
      TileManager *new_tiles = NULL;

      if (auto_center)
        {
          gint off_x, off_y;
          gint width, height;

          tile_manager_get_offsets (orig_tiles, &off_x, &off_y);

          width  = tile_manager_width  (orig_tiles);
          height = tile_manager_height (orig_tiles);

          switch (flip_type)
            {
            case GIMP_ORIENTATION_HORIZONTAL:
              axis = ((gdouble) off_x + (gdouble) width / 2.0);
              break;

            case GIMP_ORIENTATION_VERTICAL:
              axis = ((gdouble) off_y + (gdouble) height / 2.0);
              break;

            default:
              break;
            }
        }

      /*  always clip unfloated tiles so they keep their size  */
      if (GIMP_IS_CHANNEL (drawable) && tile_manager_bpp (orig_tiles) == 1)
        clip_result = TRUE;

      /* transform the buffer */
      if (orig_tiles)
        {
          new_tiles = gimp_drawable_transform_tiles_flip (drawable, context,
                                                          orig_tiles,
                                                          flip_type, axis,
                                                          clip_result);

          /* Free the cut/copied buffer */
          tile_manager_unref (orig_tiles);
        }

      if (new_tiles)
        {
          success = gimp_drawable_transform_paste (drawable, new_tiles,
                                                   new_layer);
          tile_manager_unref (new_tiles);
        }
    }

  /*  push the undo group end  */
  gimp_image_undo_group_end (image);

  return success;
}

gboolean
gimp_drawable_transform_rotate (GimpDrawable     *drawable,
                                GimpContext      *context,
                                GimpRotationType  rotate_type,
                                gboolean          auto_center,
                                gdouble           center_x,
                                gdouble           center_y,
                                gboolean          clip_result)
{
  GimpImage   *image;
  TileManager *orig_tiles;
  gboolean     new_layer;
  gboolean     success = FALSE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /* Start a transform undo group */
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_TRANSFORM, Q_("command|Rotate"));

  /* Cut/Copy from the specified drawable */
  orig_tiles = gimp_drawable_transform_cut (drawable, context, &new_layer);

  if (orig_tiles)
    {
      TileManager *new_tiles;

      if (auto_center)
        {
          gint off_x, off_y;
          gint width, height;

          tile_manager_get_offsets (orig_tiles, &off_x, &off_y);

          width  = tile_manager_width  (orig_tiles);
          height = tile_manager_height (orig_tiles);

          center_x = (gdouble) off_x + (gdouble) width  / 2.0;
          center_y = (gdouble) off_y + (gdouble) height / 2.0;
        }

      /*  always clip unfloated tiles so they keep their size  */
      if (GIMP_IS_CHANNEL (drawable) && tile_manager_bpp (orig_tiles) == 1)
        clip_result = TRUE;

      /* transform the buffer */
      new_tiles = gimp_drawable_transform_tiles_rotate (drawable, context,
                                                        orig_tiles,
                                                        rotate_type,
                                                        center_x, center_y,
                                                        clip_result);

      /* Free the cut/copied buffer */
      tile_manager_unref (orig_tiles);

      if (new_tiles)
        {
          success = gimp_drawable_transform_paste (drawable, new_tiles,
                                                   new_layer);
          tile_manager_unref (new_tiles);
        }
    }

  /*  push the undo group end  */
  gimp_image_undo_group_end (image);

  return success;
}

TileManager *
gimp_drawable_transform_cut (GimpDrawable *drawable,
                             GimpContext  *context,
                             gboolean     *new_layer)
{
  GimpImage   *image;
  TileManager *tiles;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (new_layer != NULL, NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  extract the selected mask if there is a selection  */
  if (! gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      gint x, y, w, h;

      /* set the keep_indexed flag to FALSE here, since we use
       * gimp_layer_new_from_tiles() later which assumes that the tiles
       * are either RGB or GRAY.  Eeek!!!              (Sven)
       */
      if (gimp_drawable_mask_intersect (drawable, &x, &y, &w, &h))
        {
          tiles = gimp_selection_extract (gimp_image_get_mask (image),
                                          GIMP_PICKABLE (drawable),
                                          context, TRUE, FALSE, TRUE);

          *new_layer = TRUE;
        }
      else
        {
          tiles = NULL;
          *new_layer = FALSE;
        }
    }
  else  /*  otherwise, just copy the layer  */
    {
      if (GIMP_IS_LAYER (drawable))
        tiles = gimp_selection_extract (gimp_image_get_mask (image),
                                        GIMP_PICKABLE (drawable),
                                        context, FALSE, TRUE, TRUE);
      else
        tiles = gimp_selection_extract (gimp_image_get_mask (image),
                                        GIMP_PICKABLE (drawable),
                                        context, FALSE, TRUE, FALSE);

      *new_layer = FALSE;
    }

  return tiles;
}

gboolean
gimp_drawable_transform_paste (GimpDrawable *drawable,
                               TileManager  *tiles,
                               gboolean      new_layer)
{
  GimpImage   *image;
  GimpLayer   *layer     = NULL;
  const gchar *undo_desc = NULL;
  gint         offset_x;
  gint         offset_y;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (tiles != NULL, FALSE);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (GIMP_IS_LAYER (drawable))
    undo_desc = _("Transform Layer");
  else if (GIMP_IS_CHANNEL (drawable))
    undo_desc = _("Transform Channel");
  else
    return FALSE;

  tile_manager_get_offsets (tiles, &offset_x, &offset_y);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE, undo_desc);

  if (new_layer)
    {
      layer =
        gimp_layer_new_from_tiles (tiles, image,
                                   gimp_drawable_type_with_alpha (drawable),
                                   _("Transformation"),
                                   GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

      GIMP_ITEM (layer)->offset_x = offset_x;
      GIMP_ITEM (layer)->offset_y = offset_y;

      floating_sel_attach (layer, drawable);
    }
  else
    {
      GimpImageType drawable_type;

      if (GIMP_IS_LAYER (drawable) && (tile_manager_bpp (tiles) == 2 ||
                                       tile_manager_bpp (tiles) == 4))
        {
          drawable_type = gimp_drawable_type_with_alpha (drawable);
        }
      else
        {
          drawable_type = gimp_drawable_type (drawable);
        }

      gimp_drawable_set_tiles_full (drawable, TRUE, NULL,
                                    tiles, drawable_type,
                                    offset_x, offset_y);
    }

  gimp_image_undo_group_end (image);

  return TRUE;
}
