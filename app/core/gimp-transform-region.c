/* The GIMP -- an image manipulation program
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

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

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
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"

#include "gimp-intl.h"


#define MIN4(a,b,c,d) MIN(MIN(a,b),MIN(c,d))
#define MAX4(a,b,c,d) MAX(MAX(a,b),MAX(c,d))


/* recursion level should be a usersettable parameter,
   3 seems to be a reasonable default */
#define RECURSION_LEVEL 3


/*  forward function prototypes  */

static gboolean supersample_dtest (gdouble u0, gdouble v0,
                                   gdouble u1, gdouble v1,
                                   gdouble u2, gdouble v2,
                                   gdouble u3, gdouble v3);

static void     sample_adapt      (TileManager *tm,
                                   gdouble      uc,     gdouble     vc,
                                   gdouble      u0,     gdouble     v0,
                                   gdouble      u1,     gdouble     v1,
                                   gdouble      u2,     gdouble     v2,
                                   gdouble      u3,     gdouble     v3,
                                   gint         level,
                                   guchar      *color,
                                   guchar      *bg_color,
                                   gint         bpp, 
                                   gint         alpha);


static void 
sample_cubic (PixelSurround *surround, 
              gdouble        u, 
              gdouble        v, 
              guchar        *color,
              gint           bytes, 
              gint           alpha);

static void 
sample_linear(PixelSurround *surround, 
              gdouble        u,
              gdouble        v, 
              guchar        *color,
              gint           bytes,
              gint           alpha);


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
  PixelSurround surround;


  gint         x1, y1, x2, y2;        /* target bounding box */
  gint         x, y;                  /* target coordinates */
  gint         u1, v1, u2, v2;        /* source bounding box */
  gdouble      uinc, vinc, winc;      /* increments in source coordinates 
                                         pr horizontal target coordinate */

  gdouble      u[5],v[5];             /* source coordinates,    
                                  2
                                 / \    0 is sample in the centre of pixel
                                1 0 3   1..4 is offset 1 pixel in each
                                 \ /    direction (in target space)
                                  4
                                       */

  gdouble      tu[5],tv[5],tw[5];     /* undivided source coordinates and
                                         divisor */

  gint         coords;
  gint         width;
  gint         alpha;
  gint         bytes;
  guchar      *dest, *d;
  guchar       bg_color[MAX_CHANNELS];

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (float_tiles != NULL, NULL);

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  alpha = 0;

  /*  turn interpolation off for simple transformations (e.g. rot90)  */
  if (gimp_matrix3_is_simple (matrix))
    interpolation_type = GIMP_INTERPOLATION_NONE;

  /*  Get the background color  */
  gimp_image_get_background (gimage, drawable, bg_color);


  switch (GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable)))
    {
    case GIMP_RGB:
      bg_color[ALPHA_PIX] = TRANSPARENT_OPACITY;
      alpha = ALPHA_PIX;
      break;
    case GIMP_GRAY:
      bg_color[ALPHA_G_PIX] = TRANSPARENT_OPACITY;
      alpha = ALPHA_G_PIX;
      break;
    case GIMP_INDEXED:
      bg_color[ALPHA_I_PIX] = TRANSPARENT_OPACITY;
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
      bg_color[0] = OPAQUE_OPACITY;

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

  tile_manager_get_offsets (float_tiles, &u1, &v1);
  u2 = u1 + tile_manager_width (float_tiles);
  v2 = v1 + tile_manager_height (float_tiles);

  /*  Find the bounding coordinates of target */
  if (alpha == 0 || clip_result)
    {
      x1 = u1;
      y1 = v1;
      x2 = u2;
      y2 = v2;
    }
  else
    {
      gdouble dx1, dy1;
      gdouble dx2, dy2;
      gdouble dx3, dy3;
      gdouble dx4, dy4;

      gimp_matrix3_transform_point (matrix, u1, v1, &dx1, &dy1);
      gimp_matrix3_transform_point (matrix, u2, v1, &dx2, &dy2);
      gimp_matrix3_transform_point (matrix, u1, v2, &dx3, &dy3);
      gimp_matrix3_transform_point (matrix, u2, v2, &dx4, &dy4);

      x1 = ROUND (MIN4 (dx1, dx2, dx3, dx4));
      y1 = ROUND (MIN4 (dy1, dy2, dy3, dy4));

      x2 = ROUND (MAX4 (dx1, dx2, dx3, dx4));
      y2 = ROUND (MAX4 (dy1, dy2, dy3, dy4));
    }

  /*  Get the new temporary buffer for the transformed result  */
  tiles = tile_manager_new ((x2 - x1), (y2 - y1),
                tile_manager_bpp (float_tiles));
  pixel_region_init (&destPR, tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);
  tile_manager_set_offsets (tiles, x1, y1);

  /* initialise the pixel_surround and pixel_cache accessors */
  switch (interpolation_type)
    {
    case GIMP_INTERPOLATION_NONE:
      break;
    case GIMP_INTERPOLATION_CUBIC:
      pixel_surround_init (&surround, float_tiles, 4, 4, bg_color);
      break;
    case GIMP_INTERPOLATION_LINEAR:
      pixel_surround_init (&surround, float_tiles, 2, 2, bg_color);
      break;
    }

  width  = tile_manager_width (tiles);
  bytes  = tile_manager_bpp (tiles);

  dest = g_new (guchar, tile_manager_width(tiles) * bytes);

  uinc = m[0][0];
  vinc = m[1][0];
  winc = m[2][0];

  coords = ((interpolation_type != GIMP_INTERPOLATION_NONE) ?
            5 : 1);

  /* these loops could be rearranged, depending on which bit of code
   * you'd most like to write more than once.
   */

  for (y = y1; y < y2; y++)
    {
      if (progress_callback && !(y & 0xf))
        (* progress_callback) (y1, y2, y, progress_data);

      /* set up inverse transform steps */
      tu[0] = uinc * (x1 + 0.5) + m[0][1] * (y + 0.5) + m[0][2] - 0.5;
      tv[0] = vinc * (x1 + 0.5) + m[1][1] * (y + 0.5) + m[1][2] - 0.5;
      tw[0] = winc * (x1 + 0.5) + m[2][1] * (y + 0.5) + m[2][2];

      if (interpolation_type != GIMP_INTERPOLATION_NONE)
        {
          gdouble xx = x1 + 0.5;
          gdouble yy = y + 0.5;

          tu[1] = uinc * (xx - 1) + m[0][1] * (yy    ) + m[0][2] - 0.5;
          tv[1] = vinc * (xx - 1) + m[1][1] * (yy    ) + m[1][2] - 0.5;
          tw[1] = winc * (xx - 1) + m[2][1] * (yy    ) + m[2][2];

          tu[2] = uinc * (xx    ) + m[0][1] * (yy - 1) + m[0][2] - 0.5;
          tv[2] = vinc * (xx    ) + m[1][1] * (yy - 1) + m[1][2] - 0.5;
          tw[2] = winc * (xx    ) + m[2][1] * (yy - 1) + m[2][2];

          tu[3] = uinc * (xx + 1) + m[0][1] * (yy    ) + m[0][2] - 0.5;
          tv[3] = vinc * (xx + 1) + m[1][1] * (yy    ) + m[1][2] - 0.5;
          tw[3] = winc * (xx + 1) + m[2][1] * (yy    ) + m[2][2];

          tu[4] = uinc * (xx    ) + m[0][1] * (yy + 1) + m[0][2] - 0.5;
          tv[4] = vinc * (xx    ) + m[1][1] * (yy + 1) + m[1][2] - 0.5;
          tw[4] = winc * (xx    ) + m[2][1] * (yy + 1) + m[2][2];
        }

      d = dest;

      for (x = x1; x < x2; x++)
        {
          gint i;     /*  normalize homogeneous coords  */

          for (i = 0; i < coords; i++)
            {
              if (tw[i] == 1.0)
                {
                  u[i] = tu[i];
                  v[i] = tv[i];
                }
              else if (tw[i] != 0.0)
                {
                  u[i] = tu[i] / tw[i];
                  v[i] = tv[i] / tw[i];
                }
              else
                {
                  g_warning ("homogeneous coordinate = 0...\n");
                }
            }

          /*  Set the destination pixels  */
          if (interpolation_type == GIMP_INTERPOLATION_NONE)
            {
              guchar color[MAX_CHANNELS];
              gint iu = RINT (u[0]);
              gint iv = RINT (v[0]);
              gint b;

              if (iu >= u1 && iu < u2 &&
                  iv >= v1 && iv < v2)
                {
                  /*  u, v coordinates into source tiles  */
                  gint u = iu - u1;
                  gint v = iv - v1;

                  read_pixel_data_1 (float_tiles, u, v, color);

                  for (b = 0; b < bytes; b++)
                    *d++ = color[b];
                }
              else /* not in source range */
                {
                  /*  increment the destination pointers  */

                  for (b = 0; b < bytes; b++)
                    *d++ = bg_color[b];
                }
            }
          else
            {
              gint b;

              if (MAX4 (u[1], u[2], u[3], u[4]) < u1  ||
                  MAX4 (v[1], v[2], v[3], v[4]) < v1  ||
                  MIN4 (u[1], u[2], u[3], u[4]) >= u2 ||
                  MIN4 (v[1], v[2], v[3], v[4]) >= v2)
                {
                  /* not in source range */
                  /* increment the destination pointers  */

                  for (b = 0; b < bytes; b++)
                    *d++ = bg_color[b];
                }
              else 
                {
                  guchar color[MAX_CHANNELS];

                  if (RECURSION_LEVEL &&
                      supersample_dtest (u[1], v[1], u[2], v[2],
                                         u[3], v[3], u[4], v[4]))
                    {
                      sample_adapt (float_tiles,
                                    u[0]-u1, v[0]-v1,
                                    u[1]-u1, v[1]-v1,
                                    u[2]-u1, v[2]-v1,
                                    u[3]-u1, v[3]-v1,
                                    u[4]-u1, v[4]-v1,
                                    RECURSION_LEVEL,
                                    color, bg_color, bytes, alpha);
                    }
                  else
                    {
                      /* cubic only needs to be done if no supersampling
                         is needed */

                      if (interpolation_type == GIMP_INTERPOLATION_LINEAR)
                        sample_linear (&surround, u[0] - u1, v[0] - v1,
                                       color, bytes, alpha);
                      else
                        sample_cubic (&surround, u[0] - u1, v[0] - v1,
                                      color, bytes, alpha);
                    }

                  /*  Set the destination pixel  */
                  for (b = 0; b < bytes; b++)
                    *d++ = color[b];
                }
            }

          for (i = 0; i < coords; i++)
            {
              tu[i] += uinc;
              tv[i] += vinc;
              tw[i] += winc;
            }
        }

      /*  set the pixel region row  */
      pixel_region_set_row (&destPR, 0, (y - y1), width, dest);
    }
  
  if (interpolation_type != GIMP_INTERPOLATION_NONE)
    pixel_surround_clear (&surround);

  g_free (dest);

  return tiles;
}

TileManager *
gimp_drawable_transform_tiles_flip (GimpDrawable        *drawable,
                                    TileManager         *orig,
                                    GimpOrientationType  flip_type)
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

  if (flip_type == GIMP_ORIENTATION_HORIZONTAL)
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
  if (flip_type == GIMP_ORIENTATION_HORIZONTAL)
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

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  /* Start a transform undo group */
  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_TRANSFORM,
                               _("Transform"));

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
        success = gimp_drawable_transform_paste (drawable,
                                                 new_tiles, new_layer);
    }

  /*  push the undo group end  */
  gimp_image_undo_group_end (gimage);

  return success;
}

gboolean
gimp_drawable_transform_flip (GimpDrawable        *drawable,
                              GimpOrientationType  flip_type)
{
  GimpImage   *gimage;
  TileManager *float_tiles;
  gboolean     new_layer;
  gboolean     success = FALSE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  /* Start a transform undo group */
  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_TRANSFORM,
                               _("Flip"));

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
        success = gimp_drawable_transform_paste (drawable,
                                                 new_tiles, new_layer);
    }

  /*  push the undo group end  */
  gimp_image_undo_group_end (gimage);

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

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

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

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if (new_layer)
    {
      layer =
        gimp_layer_new_from_tiles (tiles,
                                   gimage,
                                   gimp_drawable_type_with_alpha (drawable),
                   _("Transformation"),
                   GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);
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
      gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_EDIT_PASTE,
                                   _("Paste Transform"));

      floating_sel_attach (layer, drawable);

      /*  End the group undo  */
      gimp_image_undo_group_end (gimage);

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
        gimp_image_undo_push_layer_mod (gimage, _("Transform Layer"),
                                        layer);
      else if (channel)
        gimp_image_undo_push_channel_mod (gimage, _("Transform Channel"),
                                          channel);

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

#define BILINEAR(jk, j1k, jk1, j1k1, dx, dy) \
                ((1 - dy) * (jk  + dx * (j1k  - jk)) + \
                      dy  * (jk1 + dx * (j1k1 - jk1)))

  /*  u & v are the subpixel coordinates of the point in
   *  the original selection's floating buffer.
   *  We need the two pixel coords around them:
   *  iu to iu + 1, iv to iv + 1
   */
static void 
sample_linear (PixelSurround *surround, 
               gdouble        u,
               gdouble        v, 
               guchar        *color,
               gint           bytes,
               gint           alpha)
{
  gdouble  a_val, a_recip;
  gint     i;
  gint     iu = floor (u);
  gint     iv = floor (v);
  gint     row;
  gdouble  du,dv;
  guchar  *alphachan;
  guchar  *data;

  /* lock the pixel surround */
  data = pixel_surround_lock (surround, iu, iv);

  row = pixel_surround_rowstride (surround);

  /* the fractional error */
  du = u - iu;
  dv = v - iv;

  /* calculate alpha value of result pixel */
  alphachan = &data[alpha];
  a_val = BILINEAR (alphachan[0],   alphachan[bytes], 
                    alphachan[row], alphachan[row+bytes], du, dv);
  if (a_val <= 0.0)
    {
      a_recip = 0.0;
      color[alpha] = 0.0;
    }
  else if (a_val >= 255.0)
    {
      a_recip = 1.0 / a_val;
      color[alpha] = 255;
    }
  else
    {
      a_recip = 1.0 / a_val;
      color[alpha] = RINT (a_val);
    }

  /*  for colour channels c,
   *  result = bilinear (c * alpha) / bilinear (alpha)
   *
   *  never entered for alpha == 0
   */
  for (i = 0; i < alpha; i++)
    {
      gint newval = (a_recip *
                     BILINEAR (alphachan[0]         * data[i],
                               alphachan[bytes]     * data[bytes+i],
                               alphachan[row]       * data[row+i],
                               alphachan[row+bytes] * data[row+bytes+i],
                               du, dv));

      color[i] = CLAMP (newval, 0, 255);
    }

  pixel_surround_release (surround);
}


/*
    bilinear interpolation of a 16.16 pixel
*/
static void 
sample_bi (TileManager *tm,
           gint         x,
           gint         y,
           guchar      *color,
           guchar      *bg_color,
           gint         bpp,
           gint         alpha)
{
  guchar C[4][4];
  gint   i;
  gint   xscale = (x & 65535);
  gint   yscale = (y & 65535);
  
  gint   x0 = x >> 16;
  gint   y0 = y >> 16;
  gint   x1 = x0 + 1;
  gint   y1 = y0 + 1;


  /*  fill the color with default values, since read_pixel_data_1
   *  does nothing, when accesses are out of bounds.
   */
  for (i = 0; i < 4; i++) 
    *(guint*) (&C[i]) = *(guint*) (bg_color);

  read_pixel_data_1 (tm, x0, y0, C[0]);
  read_pixel_data_1 (tm, x1, y0, C[2]);
  read_pixel_data_1 (tm, x0, y1, C[1]);
  read_pixel_data_1 (tm, x1, y1, C[3]);
    
#define lerp(v1,v2,r) \
        (((guint)(v1) * (65536 - (guint)(r)) + (guint)(v2)*(guint)(r)) / 65536)

  color[alpha]= lerp (lerp (C[0][alpha], C[1][alpha], yscale),
                      lerp (C[2][alpha], C[3][alpha], yscale), xscale);

  if (color[alpha])
    { /* to avoid problems, calculate with premultiplied alpha */
      for (i=0; i<alpha; i++)
        {
          C[0][i] = (C[0][i] * C[0][alpha] / 255);
          C[1][i] = (C[1][i] * C[1][alpha] / 255);
          C[2][i] = (C[2][i] * C[2][alpha] / 255);
          C[3][i] = (C[3][i] * C[3][alpha] / 255);
        }

      for (i = 0; i < alpha; i++)
        color[i] = lerp (lerp (C[0][i], C[1][i], yscale),
                         lerp (C[2][i], C[3][i], yscale), xscale);
    }
  else
    {
      for (i = 0; i < alpha; i++)     
        color[i] = 0;
    }
#undef lerp
}

/* 
 * Returns TRUE if one of the deltas of the
 * quad edge is > 1.0 (16.16 fixed values).
 */
static gboolean       
supersample_test (gint x0, gint y0,
                  gint x1, gint y1,
                  gint x2, gint y2,
                  gint x3, gint y3)
{
  if (abs (x0 - x1) > 65535) return TRUE;
  if (abs (x1 - x2) > 65535) return TRUE;
  if (abs (x2 - x3) > 65535) return TRUE;
  if (abs (x3 - x0) > 65535) return TRUE;
  
  if (abs (y0 - y1) > 65535) return TRUE;
  if (abs (y1 - y2) > 65535) return TRUE;
  if (abs (y2 - y3) > 65535) return TRUE;
  if (abs (y3 - y0) > 65535) return TRUE;

  return FALSE;
}

/* 
 *  Returns TRUE if one of the deltas of the
 *  quad edge is > 1.0 (double values).
 */
static gboolean   
supersample_dtest (gdouble x0, gdouble y0,
                   gdouble x1, gdouble y1,
                   gdouble x2, gdouble y2,
                   gdouble x3, gdouble y3)
{
  if (fabs (x0 - x1) > 1.0) return TRUE;
  if (fabs (x1 - x2) > 1.0) return TRUE;
  if (fabs (x2 - x3) > 1.0) return TRUE;
  if (fabs (x3 - x0) > 1.0) return TRUE;
  
  if (fabs (y0 - y1) > 1.0) return TRUE;
  if (fabs (y1 - y2) > 1.0) return TRUE;
  if (fabs (y2 - y3) > 1.0) return TRUE;
  if (fabs (y3 - y0) > 1.0) return TRUE;

  return FALSE;
}

/*
    sample a grid that is spaced according to the quadraliteral's edges,
    it subdivides a maximum of level times before sampling.
    0..3 is a cycle around the quad
*/
static void
get_sample (TileManager *tm,
            gint         xc,  gint yc,
            gint         x0,  gint y0,
            gint         x1,  gint y1,
            gint         x2,  gint y2,
            gint         x3,  gint y3,
            gint        *cc,
            gint         level,
            guint       *color,
            guchar      *bg_color,
            gint         bpp,
            gint         alpha)
{
  if (!level || !supersample_test (x0, y0, x1, y1, x2, y2, x3, y3))
    {
      gint   i;
      guchar C[4];
      
      sample_bi (tm, xc, yc, C, bg_color, bpp, alpha);

      for (i = 0; i < bpp; i++)
        color[i]+= C[i];

      (*cc)++;  /* increase number of samples taken */
    }
  else 
    {
      gint tx, lx, rx, bx, tlx, trx, blx, brx;
      gint ty, ly, ry, by, tly, try, bly, bry;

      /* calculate subdivided corner coordinates (including centercoords
         thus using a bilinear interpolation,. almost as good as 
         doing the perspective transform for each subpixel coordinate*/

      tx  = (x0 + x1) / 2;
      tlx = (x0 + xc) / 2;
      trx = (x1 + xc) / 2;
      lx  = (x0 + x3) / 2;
      rx  = (x1 + x2) / 2;
      blx = (x3 + xc) / 2;
      brx = (x2 + xc) / 2;
      bx  = (x3 + x2) / 2;

      ty  = (y0 + y1) / 2;
      tly = (y0 + yc) / 2;
      try = (y1 + yc) / 2;
      ly  = (y0 + y3) / 2;
      ry  = (y1 + y2) / 2;
      bly = (y3 + yc) / 2;
      bry = (y2 + yc) / 2;
      by  = (y3 + y2) / 2;

      get_sample (tm,
                  tlx,tly,         
                  x0,y0, tx,ty, xc,yc, lx,ly,
                  cc, level-1, color, bg_color, bpp, alpha);

      get_sample (tm,
                  trx,try,
                  tx,ty, x1,y1, rx,ry, xc,yc,
                  cc, level-1, color, bg_color, bpp, alpha);

      get_sample (tm,
                  brx,bry,
                  xc,yc, rx,ry, x2,y2, bx,by,
                  cc, level-1, color, bg_color, bpp, alpha);

      get_sample (tm,
                  blx,bly,
                  lx,ly, xc,yc, bx,by, x3,y3,
                  cc, level-1, color, bg_color, bpp, alpha);
    }
}

static void 
sample_adapt (TileManager *tm,
              gdouble     xc,  gdouble yc,
              gdouble     x0,  gdouble y0,
              gdouble     x1,  gdouble y1,
              gdouble     x2,  gdouble y2,
              gdouble     x3,  gdouble y3,
              gint        level,
              guchar     *color,
              guchar     *bg_color,
              gint        bpp,
              gint        alpha)
{
    gint  cc = 0;
    gint  i;
    guint C[MAX_CHANNELS];

    C[0] = C[1] = C[2] = C[3] = 0;

    get_sample (tm,
                xc * 65535, yc * 65535,
                x0 * 65535, y0 * 65535,
                x1 * 65535, y1 * 65535,
                x2 * 65535, y2 * 65535,
                x3 * 65535, y3 * 65535,
                &cc, level, C, bg_color, bpp, alpha);

    if (!cc)
      cc=1;

    color[alpha] = C[alpha] / cc;

    if (color[alpha])
      {
         /* go from premultiplied to postmultiplied alpha */
        for (i = 0; i < alpha; i++)
          color[i] = ((C[i] / cc) * 255) / color[alpha];
      }
    else
      {
        for (i = 0; i < alpha; i++)
          color[i] = 0;
      }
}

/* access interleaved pixels */
#define CUBIC_ROW(dx, row, step) \
  gimp_drawable_transform_cubic(dx,\
            (row)[0], (row)[step], (row)[step+step], (row)[step+step+step])

#define CUBIC_SCALED_ROW(dx, row, arow, step) \
  gimp_drawable_transform_cubic(dx, \
            (arow)[0]              * (row)[0], \
            (arow)[step]           * (row)[step], \
            (arow)[step+step]      * (row)[step+step], \
            (arow)[step+step+step] * (row)[step+step+step])


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


  /*  u & v are the subpixel coordinates of the point in
   *  the original selection's floating buffer.
   *  We need the four integer pixel coords around them:
   *  iu to iu + 3, iv to iv + 3
   */
static void 
sample_cubic (PixelSurround *surround, 
              gdouble        u,
              gdouble        v, 
              guchar        *color,
              gint           bytes,
              gint           alpha)
{
  gdouble  a_val, a_recip;
  gint     i;
  gint     iu = floor(u);
  gint     iv = floor(v);
  gint     row;
  gdouble  du,dv;
  guchar  *data;

  /* lock the pixel surround */
  data = pixel_surround_lock (surround, iu - 1 , iv - 1 );

  row = pixel_surround_rowstride (surround);

  /* the fractional error */
  du = u - iu;
  dv = v - iv;

  /* calculate alpha of result */
  a_val = gimp_drawable_transform_cubic
    (dv,
     CUBIC_ROW (du, data + alpha + row * 0, bytes),
     CUBIC_ROW (du, data + alpha + row * 1, bytes),
     CUBIC_ROW (du, data + alpha + row * 2, bytes),
     CUBIC_ROW (du, data + alpha + row * 3, bytes));

  if (a_val <= 0.0)
    {
      a_recip = 0.0;
      color[alpha] = 0;
    }
  else if (a_val > 255.0)
    {
      a_recip = 1.0 / a_val;
      color[alpha] = 255;
    }
  else
    {
      a_recip = 1.0 / a_val;
      color[alpha] = RINT (a_val);
    }

  /*  for colour channels c,
   *  result = bicubic (c * alpha) / bicubic (alpha)
   *
   *  never entered for alpha == 0
   */
  for (i = 0; i < alpha; i++)
    {
      gint newval = (a_recip *
                     gimp_drawable_transform_cubic
                     (dv,
                      CUBIC_SCALED_ROW (du, i + data + row * 0, data + alpha + row * 0, bytes),
                      CUBIC_SCALED_ROW (du, i + data + row * 1, data + alpha + row * 1, bytes),
                      CUBIC_SCALED_ROW (du, i + data + row * 2, data + alpha + row * 2, bytes),
                      CUBIC_SCALED_ROW (du, i + data + row * 3, data + alpha + row * 3, bytes)));

      color[i] = CLAMP (newval, 0, 255);
    }

  pixel_surround_release (surround);
}
