/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "core-types.h"

#include "base/base-config.h"
#include "base/pixel-region.h"
#include "base/pixel-surround.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimpdrawable-transform.h"
#include "gimpimage.h"
#include "gimpimage-mask.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


#define BILINEAR(jk,j1k,jk1,j1k1,dx,dy) \
                ((1-dy) * (jk + dx * (j1k - jk)) + \
		    dy  * (jk1 + dx * (j1k1 - jk1)))

/* access interleaved pixels */
#define CUBIC_ROW(dx, row, step) \
  gimp_drawable_transform_cubic(dx, (row)[0], (row)[step], (row)[step+step], (row)[step+step+step])
#define CUBIC_SCALED_ROW(dx, row, step, i) \
  gimp_drawable_transform_cubic(dx, (row)[0] * (row)[i], \
            (row)[step] * (row)[step + i], \
            (row)[step+step]* (row)[step+step + i], \
            (row)[step+step+step] * (row)[step+step+step + i])

#define REF_TILE(i,x,y) \
     tile[i] = tile_manager_get_tile (float_tiles, x, y, TRUE, FALSE); \
     src[i] = tile_data_pointer (tile[i], (x) % TILE_WIDTH, (y) % TILE_HEIGHT);


/*  forward function prototypes  */

static gdouble   gimp_drawable_transform_cubic (gdouble dx,
					        gint    jm1,
					        gint    j,
					        gint    jp1,
					        gint    jp2);


/*  public functions  */

TileManager *
gimp_drawable_transform_tiles_affine (GimpDrawable           *drawable,
                                      TileManager            *float_tiles,
                                      GimpInterpolationType   interpolation_type,
                                      gboolean                clip_result,
                                      GimpMatrix3             matrix,
                                      GimpTransformDirection  direction,
                                      GimpProgressFunc        progress_callback,
                                      gpointer                progress_data)
{
  GimpImage   *gimage;
  PixelRegion  destPR;
  TileManager *tiles;
  GimpMatrix3  m;
  GimpMatrix3  im;
  gint         itx, ity;
  gint         tx1, ty1, tx2, ty2;
  gint         width, height;
  gint         alpha;
  gint         bytes, b;
  gint         x, y;
  gint         sx, sy;
  gint         x1, y1, x2, y2;
  gdouble      xinc, yinc, winc;
  gdouble      tx, ty, tw;
  gdouble      ttx = 0.0, tty = 0.0;
  guchar      *dest;
  guchar      *d;
  guchar      *src[16];
  Tile        *tile[16];
  guchar       bg_col[MAX_CHANNELS];
  gint         i;
  gdouble      a_val, a_recip;
  gint         newval;

  PixelSurround surround;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (float_tiles != NULL, NULL);

  gimage = gimp_drawable_gimage (drawable);

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  alpha = 0;

  /*  turn interpolation off for simple transformations (e.g. rot90)  */
  if (gimp_matrix3_is_simple (matrix))
    interpolation_type = GIMP_INTERPOLATION_NONE;

  /*  Get the background color  */
  gimp_image_get_background (gimage, drawable, bg_col);

  switch (GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable)))
    {
    case GIMP_RGB:
      bg_col[ALPHA_PIX] = TRANSPARENT_OPACITY;
      alpha = ALPHA_PIX;
      break;
    case GIMP_GRAY:
      bg_col[ALPHA_G_PIX] = TRANSPARENT_OPACITY;
      alpha = ALPHA_G_PIX;
      break;
    case GIMP_INDEXED:
      bg_col[ALPHA_I_PIX] = TRANSPARENT_OPACITY;
      alpha = ALPHA_I_PIX;
      /*  If the gimage is indexed color, ignore interpolation value  */
      interpolation_type = GIMP_INTERPOLATION_NONE;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  /*  enable rotating un-floated non-layers  */
  if (tile_manager_bpp (float_tiles) == 1)
    {
      bg_col[0] = OPAQUE_OPACITY;

      /*  setting alpha = 0 will cause the channel's value to be treated
       *  as alpha and the color channel loops never to be entered
       */
      alpha = 0;
    }

  if (direction == GIMP_TRANSFORM_BACKWARD)
    {
      /*  keep the original matrix here, so we dont need to recalculate
       *  the inverse later
       */
      gimp_matrix3_duplicate (matrix, m);
      gimp_matrix3_invert (matrix, im);
      matrix = im;
    }
  else
    {
      /*  Find the inverse of the transformation matrix  */
      gimp_matrix3_invert (matrix, m);
    }

#ifdef __GNUC__
#warning FIXME: path_transform_current_path
#endif
#if 0
  path_transform_current_path (gimage, matrix, FALSE);
#endif

  tile_manager_get_offsets (float_tiles, &x1, &y1);
  x2 = x1 + tile_manager_width (float_tiles);
  y2 = y1 + tile_manager_height (float_tiles);

  /*  Find the bounding coordinates  */
  if (alpha == 0 || clip_result)
    {
      tx1 = x1;
      ty1 = y1;
      tx2 = x2;
      ty2 = y2;
    }
  else
    {
      gdouble dx1, dy1;
      gdouble dx2, dy2;
      gdouble dx3, dy3;
      gdouble dx4, dy4;

      gimp_matrix3_transform_point (matrix, x1, y1, &dx1, &dy1);
      gimp_matrix3_transform_point (matrix, x2, y1, &dx2, &dy2);
      gimp_matrix3_transform_point (matrix, x1, y2, &dx3, &dy3);
      gimp_matrix3_transform_point (matrix, x2, y2, &dx4, &dy4);

#define MIN4(a,b,c,d) MIN(MIN(MIN(a,b),c),d)
#define MAX4(a,b,c,d) MAX(MAX(MAX(a,b),c),d)

      tx1 = ROUND (MIN4 (dx1, dx2, dx3, dx4));
      ty1 = ROUND (MIN4 (dy1, dy2, dy3, dy4));

      tx2 = ROUND (MAX4 (dx1, dx2, dx3, dx4));
      ty2 = ROUND (MAX4 (dy1, dy2, dy3, dy4));

#undef MIN2
#undef MAX4
    }

  /*  Get the new temporary buffer for the transformed result  */
  tiles = tile_manager_new ((tx2 - tx1), (ty2 - ty1),
			    tile_manager_bpp (float_tiles));
  pixel_region_init (&destPR, tiles, 0, 0, (tx2 - tx1), (ty2 - ty1), TRUE);
  tile_manager_set_offsets (tiles, tx1, ty1);

  /* initialise the pixel_surround accessor */
  switch (interpolation_type)
    {
    case GIMP_INTERPOLATION_NONE:
      /* not actually useful, keeps the code cleaner */
      pixel_surround_init (&surround, float_tiles, 1, 1, bg_col);
      break;

    case GIMP_INTERPOLATION_CUBIC:
      pixel_surround_init (&surround, float_tiles, 4, 4, bg_col);
      break;

    case GIMP_INTERPOLATION_LINEAR:
      pixel_surround_init (&surround, float_tiles, 2, 2, bg_col);
      break;
    }

  width  = tile_manager_width (tiles);
  height = tile_manager_height (tiles);
  bytes  = tile_manager_bpp (tiles);

  dest = g_new (guchar, width * bytes);

  xinc = m[0][0];
  yinc = m[1][0];
  winc = m[2][0];

  /* these loops could be rearranged, depending on which bit of code
   * you'd most like to write more than once.
   */

  for (y = ty1; y < ty2; y++)
    {
      if (progress_callback && !(y & 0xf))
	(* progress_callback) (ty1, ty2, y, progress_data);

      /* set up inverse transform steps */
      if (interpolation_type == GIMP_INTERPOLATION_NONE)
        {
          /*  need to transform the pixel's center for INTERPOLATION_NONE,
           *  as we end up at discrete pixel positions and are not aware of
           *  errors in the algorithm below
           */
          tx = xinc * (tx1 + 0.5) + m[0][1] * (y + 0.5) + m[0][2];
          ty = yinc * (tx1 + 0.5) + m[1][1] * (y + 0.5) + m[1][2];
          tw = winc * (tx1 + 0.5) + m[2][1] * (y + 0.5) + m[2][2];
        }
      else
        {
          tx = xinc * tx1 + m[0][1] * y + m[0][2];
          ty = yinc * tx1 + m[1][1] * y + m[1][2];
          tw = winc * tx1 + m[2][1] * y + m[2][2];
        }

      d = dest;

      for (x = tx1; x < tx2; x++)
	{
	  /*  normalize homogeneous coords  */
	  if (tw == 0.0)
	    {
	      g_warning ("homogeneous coordinate = 0...\n");
	    }
	  else if (tw != 1.0)
	    {
	      ttx = tx / tw;
	      tty = ty / tw;
	    }
	  else
	    {
	      ttx = tx;
	      tty = ty;
	    }

          /*  Set the destination pixels  */

          switch (interpolation_type)
            {
            case GIMP_INTERPOLATION_CUBIC:
              /*  ttx & tty are the subpixel coordinates of the point in
               *  the original selection's floating buffer.
               *  We need the four integer pixel coords around them:
               *  itx to itx + 3, ity to ity + 3
               */
              itx = floor (ttx);
              ity = floor (tty);

              /* check if any part of our region overlaps the buffer */

              if ((itx + 2) >= x1 && (itx - 1) < x2 &&
                  (ity + 2) >= y1 && (ity - 1) < y2)
                {
                  guchar  *data;
                  gint     row;
                  gdouble  dx, dy;
                  guchar  *start;

                  /* lock the pixel surround */
                  data = pixel_surround_lock (&surround,
                                              itx - 1 - x1, ity - 1 - y1);

                  row = pixel_surround_rowstride (&surround);

                  /* the fractional error */
                  dx = ttx - itx;
                  dy = tty - ity;

                  /* calculate alpha of result */
                  start = &data[alpha];
                  a_val = gimp_drawable_transform_cubic
                    (dy,
                     CUBIC_ROW (dx, start, bytes),
                     CUBIC_ROW (dx, start + row, bytes),
                     CUBIC_ROW (dx, start + row + row, bytes),
                     CUBIC_ROW (dx, start + row + row + row, bytes));

                  if (a_val <= 0.0)
                    {
                      a_recip = 0.0;
                      d[alpha] = 0;
                    }
                  else if (a_val > 255.0)
                    {
                      a_recip = 1.0 / a_val;
                      d[alpha] = 255;
                    }
                  else
                    {
                      a_recip = 1.0 / a_val;
                      d[alpha] = RINT(a_val);
                    }

                  /*  for colour channels c,
                   *  result = bicubic (c * alpha) / bicubic (alpha)
                   *
                   *  never entered for alpha == 0
                   */
                  for (i = -alpha; i < 0; ++i)
                    {
                      start = &data[alpha];
                      newval =
                        RINT (a_recip *
                              gimp_drawable_transform_cubic
                              (dy,
                               CUBIC_SCALED_ROW (dx, start, bytes, i),
                               CUBIC_SCALED_ROW (dx, start + row, bytes, i),
                               CUBIC_SCALED_ROW (dx, start + row + row, bytes, i),
                               CUBIC_SCALED_ROW (dx, start + row + row + row, bytes, i)));
                      if (newval <= 0)
                        {
                          *d++ = 0;
                        }
                      else if (newval > 255)
                        {
                          *d++ = 255;
                        }
                      else
                        {
                          *d++ = newval;
                        }
                    }

                  /*  alpha already done  */
                  d++;

                  pixel_surround_release (&surround);
                }
              else /* not in source range */
                {
                  /*  increment the destination pointers  */
                  for (b = 0; b < bytes; b++)
                    *d++ = bg_col[b];
                }
              break;

            case GIMP_INTERPOLATION_LINEAR:
              itx = floor (ttx);
              ity = floor (tty);

              /*  expand source area to cover interpolation region
               *  (which runs from itx to itx + 1, same in y)
               */
              if ((itx + 1) >= x1 && itx < x2 &&
                  (ity + 1) >= y1 && ity < y2)
                {
                  guchar  *data;
                  gint     row;
                  double   dx, dy;
                  guchar  *chan;

                  /* lock the pixel surround */
                  data = pixel_surround_lock (&surround, itx - x1, ity - y1);

                  row = pixel_surround_rowstride (&surround);

                  /* the fractional error */
                  dx = ttx - itx;
                  dy = tty - ity;

                  /* calculate alpha value of result pixel */
                  chan = &data[alpha];
                  a_val = BILINEAR (chan[0], chan[bytes], chan[row],
                                    chan[row+bytes], dx, dy);
                  if (a_val <= 0.0)
                    {
                      a_recip = 0.0;
                      d[alpha] = 0.0;
                    }
                  else if (a_val >= 255.0)
                    {
                      a_recip = 1.0 / a_val;
                      d[alpha] = 255;
                    }
                  else
                    {
                      a_recip = 1.0 / a_val;
                      d[alpha] = RINT (a_val);
                    }

                  /*  for colour channels c,
                   *  result = bilinear (c * alpha) / bilinear (alpha)
                   *
                   *  never entered for alpha == 0
                   */
                  for (i = -alpha; i < 0; ++i)
                    {
                      chan = &data[alpha];
                      newval =
                        RINT (a_recip *
                              BILINEAR (chan[0] * chan[i],
                                        chan[bytes] * chan[bytes+i],
                                        chan[row] * chan[row+i],
                                        chan[row+bytes] * chan[row+bytes+i],
                                        dx, dy));
                      if (newval <= 0)
                        {
                          *d++ = 0;
                        }
                      else if (newval > 255)
                        {
                          *d++ = 255;
                        }
                      else
                        {
                          *d++ = newval;
                        }
                    }

                  /*  alpha already done  */
                  d++;

                  pixel_surround_release (&surround);
                }
              else /* not in source range */
                {
                  /*  increment the destination pointers  */
                  for (b = 0; b < bytes; b++)
                    *d++ = bg_col[b];
                }
              break;

            case GIMP_INTERPOLATION_NONE:
              itx = floor (ttx);
              ity = floor (tty);

              if (itx >= x1 && itx < x2 &&
                  ity >= y1 && ity < y2)
                {
                  /*  x, y coordinates into source tiles  */
                  sx = itx - x1;
                  sy = ity - y1;

                  REF_TILE (0, sx, sy);

                  for (b = 0; b < bytes; b++)
                    *d++ = src[0][b];

                  tile_release (tile[0], FALSE);
                }
              else /* not in source range */
                {
                  /*  increment the destination pointers  */
                  for (b = 0; b < bytes; b++)
                    *d++ = bg_col[b];
                }
              break;
            }

	  /*  increment the transformed coordinates  */
	  tx += xinc;
	  ty += yinc;
	  tw += winc;
	}

      /*  set the pixel region row  */
      pixel_region_set_row (&destPR, 0, (y - ty1), width, dest);
    }

  pixel_surround_clear (&surround);

  g_free (dest);

  return tiles;
}

TileManager *
gimp_drawable_transform_tiles_flip (GimpDrawable            *drawable,
                                    TileManager             *orig,
                                    InternalOrientationType  flip_type)
{
  TileManager *new;
  PixelRegion  srcPR, destPR;
  gint         orig_width;
  gint         orig_height;
  gint         orig_bpp;
  gint         orig_x, orig_y;
  gint         i;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (orig != NULL, NULL);

  orig_width  = tile_manager_width (orig);
  orig_height = tile_manager_height (orig);
  orig_bpp    = tile_manager_bpp (orig);
  tile_manager_get_offsets (orig, &orig_x, &orig_y);

  new = tile_manager_new (orig_width, orig_height, orig_bpp);
  tile_manager_set_offsets (new, orig_x, orig_y);

  if (flip_type == ORIENTATION_HORIZONTAL)
    {
      for (i = 0; i < orig_width; i++)
        {
          pixel_region_init (&srcPR, orig, i, 0, 1, orig_height, FALSE);
          pixel_region_init (&destPR, new,
                             (orig_width - i - 1), 0, 1, orig_height, TRUE);
          copy_region (&srcPR, &destPR); 
        }
    }
  else
    {
      for (i = 0; i < orig_height; i++)
        {
          pixel_region_init (&srcPR, orig, 0, i, orig_width, 1, FALSE);
          pixel_region_init (&destPR, new,
                             0, (orig_height - i - 1), orig_width, 1, TRUE);
          copy_region (&srcPR, &destPR);
        }
    }

#ifdef __GNUC__
#warning FIXME: path_transform_flip_horz/vert
#endif
#if 0
  /* flip locked paths */
  /* Note that the undo structures etc are setup before we enter this
   * function.
   */
  if (flip_type == ORIENTATION_HORIZONTAL)
    path_transform_flip_horz (gimage);
  else
    path_transform_flip_vert (gimage);
#endif

  return new;
}

gboolean
gimp_drawable_transform_affine (GimpDrawable           *drawable,
                                GimpInterpolationType   interpolation_type,
                                gboolean                clip_result,
                                GimpMatrix3             matrix,
                                GimpTransformDirection  direction)
{
  GimpImage   *gimage;
  TileManager *float_tiles;
  gboolean     new_layer;
  gboolean     success = FALSE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  gimage = gimp_drawable_gimage (drawable);

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  /* Start a transform undo group */
  undo_push_group_start (gimage, TRANSFORM_CORE_UNDO);

  /* Cut/Copy from the specified drawable */
  float_tiles = gimp_drawable_transform_cut (drawable, &new_layer);

  if (float_tiles)
    {
      TileManager *new_tiles;

      /* transform the buffer */
      new_tiles = gimp_drawable_transform_tiles_affine (drawable,
                                                        float_tiles,
                                                        interpolation_type, 
                                                        FALSE,
                                                        matrix,
                                                        GIMP_TRANSFORM_FORWARD,
                                                        NULL, NULL);

      /* Free the cut/copied buffer */
      tile_manager_destroy (float_tiles);

      if (new_tiles)
        success = gimp_drawable_transform_paste (drawable, new_tiles, new_layer);
    }

  /*  push the undo group end  */
  undo_push_group_end (gimage);

  return success;
}

gboolean
gimp_drawable_transform_flip (GimpDrawable            *drawable,
                              InternalOrientationType  flip_type)
{
  GimpImage   *gimage;
  TileManager *float_tiles;
  gboolean     new_layer;
  gboolean     success = FALSE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  gimage = gimp_drawable_gimage (drawable);

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  /* Start a transform undo group */
  undo_push_group_start (gimage, TRANSFORM_CORE_UNDO);

  /* Cut/Copy from the specified drawable */
  float_tiles = gimp_drawable_transform_cut (drawable, &new_layer);

  if (float_tiles)
    {
      TileManager *new_tiles;

      /* transform the buffer */
      new_tiles = gimp_drawable_transform_tiles_flip (drawable,
                                                      float_tiles,
                                                      flip_type);

      /* Free the cut/copied buffer */
      tile_manager_destroy (float_tiles);

      if (new_tiles)
        success = gimp_drawable_transform_paste (drawable, new_tiles, new_layer);
    }

  /*  push the undo group end  */
  undo_push_group_end (gimage);

  return success;
}

TileManager *
gimp_drawable_transform_cut (GimpDrawable *drawable,
                             gboolean     *new_layer)
{
  GimpImage   *gimage;
  TileManager *tiles;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (new_layer != NULL, NULL);

  gimage = gimp_drawable_gimage (drawable);

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  /*  extract the selected mask if there is a selection  */
  if (! gimp_image_mask_is_empty (gimage))
    {
      /* set the keep_indexed flag to FALSE here, since we use
       * gimp_layer_new_from_tiles() later which assumes that the tiles
       * are either RGB or GRAY.  Eeek!!!              (Sven)
       */
      tiles = gimp_image_mask_extract (gimage, drawable, TRUE, FALSE, TRUE);

      *new_layer = TRUE;
    }
  /*  otherwise, just copy the layer  */
  else
    {
      if (GIMP_IS_LAYER (drawable))
	tiles = gimp_image_mask_extract (gimage, drawable, FALSE, TRUE, TRUE);
      else
	tiles = gimp_image_mask_extract (gimage, drawable, FALSE, TRUE, FALSE);

      *new_layer = FALSE;
    }

  return tiles;
}

gboolean
gimp_drawable_transform_paste (GimpDrawable *drawable,
                               TileManager  *tiles,
                               gboolean      new_layer)
{
  GimpImage   *gimage;
  GimpLayer   *layer   = NULL;
  GimpChannel *channel = NULL;
  GimpLayer   *floating_layer;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (tiles != NULL, FALSE);

  gimage = gimp_drawable_gimage (drawable);

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if (new_layer)
    {
      layer =
	gimp_layer_new_from_tiles (gimage,
				   tiles,
				   _("Transformation"),
				   OPAQUE_OPACITY, GIMP_NORMAL_MODE);
      if (! layer)
        {
          g_warning ("%s: gimp_layer_new_frome_tiles() failed",
		     G_GNUC_FUNCTION);
          return FALSE;
        }

      tile_manager_get_offsets (tiles, 
				&(GIMP_DRAWABLE (layer)->offset_x),
				&(GIMP_DRAWABLE (layer)->offset_y));

      /*  Start a group undo  */
      undo_push_group_start (gimage, EDIT_PASTE_UNDO);

      floating_sel_attach (layer, drawable);

      /*  End the group undo  */
      undo_push_group_end (gimage);

      /*  Free the tiles  */
      tile_manager_destroy (tiles);

      return TRUE;
    }
  else
    {
      if (GIMP_IS_LAYER (drawable))
	layer = GIMP_LAYER (drawable);
      else if (GIMP_IS_CHANNEL (drawable))
	channel = GIMP_CHANNEL (drawable);
      else
	return FALSE;

      if (layer)
	gimp_layer_add_alpha (layer);

      floating_layer = gimp_image_floating_sel (gimage);

      if (floating_layer)
	floating_sel_relax (floating_layer, TRUE);

      gimp_image_update (gimage,
                         drawable->offset_x,
                         drawable->offset_y,
                         drawable->width,
                         drawable->height);

      /*  Push an undo  */
      if (layer)
	undo_push_layer_mod (gimage, layer);
      else if (channel)
	undo_push_channel_mod (gimage, channel);

      /*  set the current layer's data  */
      drawable->tiles = tiles;

      /*  Fill in the new layer's attributes  */
      drawable->width    = tile_manager_width (tiles);
      drawable->height   = tile_manager_height (tiles);
      drawable->bytes    = tile_manager_bpp (tiles);
      tile_manager_get_offsets (tiles, 
				&drawable->offset_x, &drawable->offset_y);

      if (floating_layer)
	floating_sel_rigor (floating_layer, TRUE);

      gimp_drawable_update (drawable,
			    0, 0,
			    gimp_drawable_width (drawable),
			    gimp_drawable_height (drawable));

      /*  if we were operating on the floating selection, then it's boundary 
       *  and previews need invalidating
       */
      if (drawable == (GimpDrawable *) floating_layer)
	floating_sel_invalidate (floating_layer);

      return TRUE;
    }
}

/* Note: cubic function no longer clips result */
static gdouble
gimp_drawable_transform_cubic (gdouble dx,
                               gint    jm1,
                               gint    j,
                               gint    jp1,
                               gint    jp2)
{
  gdouble result;

#if 0
  /* Equivalent to Gimp 1.1.1 and earlier - some ringing */
  result = ((( ( - jm1 + j - jp1 + jp2 ) * dx +
               ( jm1 + jm1 - j - j + jp1 - jp2 ) ) * dx +
               ( - jm1 + jp1 ) ) * dx + j );
  /* Recommended by Mitchell and Netravali - too blurred? */
  result = ((( ( - 7 * jm1 + 21 * j - 21 * jp1 + 7 * jp2 ) * dx +
               ( 15 * jm1 - 36 * j + 27 * jp1 - 6 * jp2 ) ) * dx +
               ( - 9 * jm1 + 9 * jp1 ) ) * dx + (jm1 + 16 * j + jp1) ) / 18.0;
#endif

  /* Catmull-Rom - not bad */
  result = ((( ( - jm1 + 3 * j - 3 * jp1 + jp2 ) * dx +
               ( 2 * jm1 - 5 * j + 4 * jp1 - jp2 ) ) * dx +
               ( - jm1 + jp1 ) ) * dx + (j + j) ) / 2.0;

  return result;
}
