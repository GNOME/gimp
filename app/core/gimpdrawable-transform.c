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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/pixel-surround.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"
#include "paint-funcs/scale-funcs.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimpdrawable-transform.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
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


/*  forward function prototypes  */

static gboolean supersample_dtest (gdouble u0, gdouble v0,
                                   gdouble u1, gdouble v1,
                                   gdouble u2, gdouble v2,
                                   gdouble u3, gdouble v3);

static void     sample_adapt      (TileManager   *tm,
                                   gdouble        uc,     gdouble     vc,
                                   gdouble        u0,     gdouble     v0,
                                   gdouble        u1,     gdouble     v1,
                                   gdouble        u2,     gdouble     v2,
                                   gdouble        u3,     gdouble     v3,
                                   gint           level,
                                   guchar        *color,
                                   guchar        *bg_color,
                                   gint           bpp,
                                   gint           alpha);

static void     sample_cubic      (PixelSurround *surround,
                                   gdouble        u,
                                   gdouble        v,
                                   guchar        *color,
                                   gint           bytes,
                                   gint           alpha);

static void     sample_linear     (PixelSurround *surround,
                                   gdouble        u,
                                   gdouble        v,
                                   guchar        *color,
                                   gint           bytes,
                                   gint           alpha);
static gdouble *create_lanczos_lookup_transform (void);


static inline gdouble
sinc (gdouble x)
{
  gdouble y = x * G_PI;

  if (ABS (x) < EPSILON)
    return 1.0;

  return sin (y) / y;
}

gdouble *
create_lanczos_lookup_transform (void)
{
  const gdouble dx = (gdouble) LANCZOS_WIDTH / (gdouble) (LANCZOS_SAMPLES - 1);

  gdouble *lanczos = g_new (gdouble, LANCZOS_SAMPLES);
  gdouble  x      = 0.0;
  gint     i;

  for (i = 0; i < LANCZOS_SAMPLES; i++)
    {
      lanczos[i] = ((ABS (x) < LANCZOS_WIDTH) ?
                   (sinc (x) * sinc (x / LANCZOS_WIDTH)) : 0.0);
      x += dx;
    }

  return lanczos;
}

/*  public functions  */

TileManager *
gimp_drawable_transform_tiles_affine (GimpDrawable           *drawable,
                                      GimpContext            *context,
                                      TileManager            *orig_tiles,
                                      const GimpMatrix3      *matrix,
                                      GimpTransformDirection  direction,
                                      GimpInterpolationType   interpolation_type,
                                      gboolean                supersample,
                                      gint                    recursion_level,
                                      gboolean                clip_result,
                                      GimpProgress           *progress)
{
  GimpImage     *image;
  PixelRegion    destPR;
  TileManager   *new_tiles;
  GimpMatrix3    m;
  GimpMatrix3    inv;
  PixelSurround  surround;

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
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (orig_tiles != NULL, NULL);
  g_return_val_if_fail (matrix != NULL, NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  m   = *matrix;
  inv = *matrix;

  alpha = 0;

  /*  turn interpolation off for simple transformations (e.g. rot90)  */
  if (gimp_matrix3_is_simple (matrix))
    interpolation_type = GIMP_INTERPOLATION_NONE;

  /*  Get the background color  */
  gimp_image_get_background (image, drawable, context, bg_color);

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
      /*  If the image is indexed color, ignore interpolation value  */
      interpolation_type = GIMP_INTERPOLATION_NONE;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  /*  "Outside" a channel is transparency, not the bg color  */
  if (GIMP_IS_CHANNEL (drawable))
    bg_color[0] = TRANSPARENT_OPACITY;

  /*  setting alpha = 0 will cause the channel's value to be treated
   *  as alpha and the color channel loops never to be entered
   */
  if (tile_manager_bpp (orig_tiles) == 1)
    alpha = 0;

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
  if (G_TYPE_FROM_INSTANCE (drawable) == GIMP_TYPE_CHANNEL && alpha == 0)
    clip_result = TRUE;

  /*  Find the bounding coordinates of target */
  if (clip_result)
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

      gimp_matrix3_transform_point (&inv, u1, v1, &dx1, &dy1);
      gimp_matrix3_transform_point (&inv, u2, v1, &dx2, &dy2);
      gimp_matrix3_transform_point (&inv, u1, v2, &dx3, &dy3);
      gimp_matrix3_transform_point (&inv, u2, v2, &dx4, &dy4);

      if (! FINITE (dx1) || ! FINITE (dy1) ||
          ! FINITE (dx2) || ! FINITE (dy2) ||
          ! FINITE (dx3) || ! FINITE (dy3) ||
          ! FINITE (dx4) || ! FINITE (dy4))
        {
          /*  fallback to clip_result if the passed matrix is broken  */

          x1 = u1;
          y1 = v1;
          x2 = u2;
          y2 = v2;
        }
      else
        {
          x1 = (gint) floor (MIN4 (dx1, dx2, dx3, dx4));
          y1 = (gint) floor (MIN4 (dy1, dy2, dy3, dy4));

          x2 = (gint) ceil (MAX4 (dx1, dx2, dx3, dx4));
          y2 = (gint) ceil (MAX4 (dy1, dy2, dy3, dy4));

          if (x1 == x2)
            x2++;

          if (y1 == y2)
            y2++;
        }
    }

  /*  Get the new temporary buffer for the transformed result  */
  new_tiles = tile_manager_new (x2 - x1, y2 - y1,
                                tile_manager_bpp (orig_tiles));
  pixel_region_init (&destPR, new_tiles,
                     0, 0, x2 - x1, y2 - y1, TRUE);
  tile_manager_set_offsets (new_tiles, x1, y1);

  width  = tile_manager_width (new_tiles);
  bytes  = tile_manager_bpp (new_tiles);

  /* initialise the pixel_surround and pixel_cache accessors */
  switch (interpolation_type)
    {
    case GIMP_INTERPOLATION_NONE:
      break;
    case GIMP_INTERPOLATION_CUBIC:
      pixel_surround_init (&surround, orig_tiles, 4, 4, bg_color);
      break;
    case GIMP_INTERPOLATION_LINEAR:
      pixel_surround_init (&surround, orig_tiles, 2, 2, bg_color);
      break;
    case GIMP_INTERPOLATION_LANCZOS:
      break;
    }

  dest = g_new (guchar, tile_manager_width (new_tiles) * bytes);

  uinc = m.coeff[0][0];
  vinc = m.coeff[1][0];
  winc = m.coeff[2][0];

  coords = (interpolation_type != GIMP_INTERPOLATION_NONE) ? 5 : 1;

  /* these loops could be rearranged, depending on which bit of code
   * you'd most like to write more than once.
   */
  if (interpolation_type == GIMP_INTERPOLATION_LANCZOS)
    {
      gdouble *lanczos  = NULL;          /* Lanczos lookup table                */
      gdouble  x_kernel[LANCZOS_WIDTH2]; /* 1-D kernels of Lanczos window coeffs */
      gdouble  y_kernel[LANCZOS_WIDTH2];
      gdouble  x_sum, y_sum;             /* sum of Lanczos weights              */

      gdouble uw;
      gdouble ww;
      gdouble vw;
      gdouble du;
      gdouble dv;

      gint    pos;
      gdouble newval;
      gdouble arecip;
      gdouble aval;

      guchar  lwin[LANCZOS_WIDTH2 * LANCZOS_WIDTH2][MAX_CHANNELS + 1];
      gint    b, u, v, i, j;
      gint    pu, pv, su, sv;

      /* allocate and fill lanczos lookup table */
      lanczos = create_lanczos_lookup_transform ();

      for (y = y1; y < y2; y++)
        {
          if (progress && !(y & 0xf))
            gimp_progress_set_value (progress,
                                     (gdouble) (y - y1) / (gdouble) (y2 - y1));

          pixel_region_get_row (&destPR, 0, (y - y1), width, dest, 1);

          d = dest;

          for (x = x1; x < x2; x++)
            {
              du = uw = m.coeff[0][0] * x + m.coeff[0][1] * y + m.coeff[0][2];
              dv = vw = m.coeff[1][0] * x + m.coeff[1][1] * y + m.coeff[1][2];
              ww =      m.coeff[2][0] * x + m.coeff[2][1] * y + m.coeff[2][2];

              if (ww == 1.0)
                {
                  du = uw;
                  dv = vw;
                }
              else if (ww != 0.0)
                {
                  du = uw / ww;
                  dv = vw / ww;
                }
              else
                {
                  g_warning ("homogeneous coordinate = 0...\n");
                }

              u = (gint) du;
              v = (gint) dv;
              /* get weight for fractional error */
              su = (gint) ((du - u) * LANCZOS_SPP);
              sv = (gint) ((dv - v) * LANCZOS_SPP);

              if (u <  u1 || v <  v1 || u >= u2 || v >= v2 )
                {
                  /* not in source range */
                  /* increment the destination pointers  */
                  for (b = 0; b < bytes; b++)
                    *d++ = bg_color[b];
                }
              else
                {
                  pos = 0;
                  for (j = 0; j < LANCZOS_WIDTH2 ; j++)
                    for (i = 0; i < LANCZOS_WIDTH2 ; i++, pos++)
                      {
                        pu = CLAMP (u + i - LANCZOS_WIDTH, u1, u2 - 1);
                        pv = CLAMP (v + j - LANCZOS_WIDTH, v1, v2 - 1);
                        read_pixel_data_1 (orig_tiles, pu, pv, &lwin[pos][0]);
                      }

                  /* fill 1D kernels */
                  for (x_sum = y_sum = 0.0, i = LANCZOS_WIDTH; i >= -LANCZOS_WIDTH; i--)
                    {
                      pos = i * LANCZOS_SPP;
                      x_sum += x_kernel[LANCZOS_WIDTH + i] = lanczos[ABS (su - pos)];
                      y_sum += y_kernel[LANCZOS_WIDTH + i] = lanczos[ABS (sv - pos)];
                    }

                  /* normalise the weighted arrays */
                  for (i = 0; i < LANCZOS_WIDTH2 ; i++)
                    {
                      x_kernel[i] /= x_sum;
                      y_kernel[i] /= y_sum;
                    }

                  pos = 0;
                  aval = 0.0;
                  for (j = 0; j < LANCZOS_WIDTH2 ; j++)
                    for (i = 0; i < LANCZOS_WIDTH2 ; i++, pos++)
                      {
                        aval += y_kernel[j] * x_kernel[i] * lwin[pos][alpha];
                      }

                  if (aval <= 0.0)
                    {
                      arecip = 0.0;
                      aval = 0;
                    }
                  else if (aval > 255.0)
                    {
                      arecip = 1.0 / aval;
                      aval = 255;
                    }
                  else
                    {
                      arecip = 1.0 / aval;
                    }

                  for (b = 0; b < alpha; b++)
                    {
                      pos = 0;
                      newval = 0.0;
                      for (j = 0; j < LANCZOS_WIDTH2 ; j++)
                        for (i = 0; i < LANCZOS_WIDTH2 ; i++, pos++)
                          {
                            newval += y_kernel[j] * x_kernel[i] * lwin[pos][b] * lwin[pos][alpha];
                          }
                      newval *= arecip;
                      *d++ = CLAMP (newval, 0, 255);
                    }

                  *d++ = RINT (aval);
                }
            }

          /*  set the pixel region row  */
          pixel_region_set_row (&destPR, 0, (y - y1), width, dest);
        }

      g_free (lanczos);
      g_free (dest);

      return new_tiles;
    }

  for (y = y1; y < y2; y++)
    {
      if (progress && !(y & 0xf))
        gimp_progress_set_value (progress,
                                 (gdouble) (y - y1) / (gdouble) (y2 - y1));

      /* set up inverse transform steps */
      tu[0] = uinc * x1 + m.coeff[0][1] * y + m.coeff[0][2];
      tv[0] = vinc * x1 + m.coeff[1][1] * y + m.coeff[1][2];
      tw[0] = winc * x1 + m.coeff[2][1] * y + m.coeff[2][2];

      if (interpolation_type != GIMP_INTERPOLATION_NONE)
        {
          gdouble xx = x1;
          gdouble yy = y;

          tu[1] = uinc * (xx - 1) + m.coeff[0][1] * (yy    ) + m.coeff[0][2];
          tv[1] = vinc * (xx - 1) + m.coeff[1][1] * (yy    ) + m.coeff[1][2];
          tw[1] = winc * (xx - 1) + m.coeff[2][1] * (yy    ) + m.coeff[2][2];

          tu[2] = uinc * (xx    ) + m.coeff[0][1] * (yy - 1) + m.coeff[0][2];
          tv[2] = vinc * (xx    ) + m.coeff[1][1] * (yy - 1) + m.coeff[1][2];
          tw[2] = winc * (xx    ) + m.coeff[2][1] * (yy - 1) + m.coeff[2][2];

          tu[3] = uinc * (xx + 1) + m.coeff[0][1] * (yy    ) + m.coeff[0][2];
          tv[3] = vinc * (xx + 1) + m.coeff[1][1] * (yy    ) + m.coeff[1][2];
          tw[3] = winc * (xx + 1) + m.coeff[2][1] * (yy    ) + m.coeff[2][2];

          tu[4] = uinc * (xx    ) + m.coeff[0][1] * (yy + 1) + m.coeff[0][2];
          tv[4] = vinc * (xx    ) + m.coeff[1][1] * (yy + 1) + m.coeff[1][2];
          tw[4] = winc * (xx    ) + m.coeff[2][1] * (yy + 1) + m.coeff[2][2];
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
              gint   iu = (gint) u[0];
              gint   iv = (gint) v[0];
              gint   b;

              if (iu >= u1 && iu < u2 &&
                  iv >= v1 && iv < v2)
                {
                  /*  u, v coordinates into source tiles  */
                  gint u = iu - u1;
                  gint v = iv - v1;

                  read_pixel_data_1 (orig_tiles, u, v, color);

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

              if (u [0] <  u1 || v [0] <  v1 ||
                  u [0] >= u2 || v [0] >= v2 )
                {
                  /* not in source range */
                  /* increment the destination pointers  */

                  for (b = 0; b < bytes; b++)
                    *d++ = bg_color[b];
                }
              else
                {
                  guchar color[MAX_CHANNELS];

                  /* clamp texture coordinates */
                  for (b = 0; b < 5; b++)
                    {
                      u[b] = CLAMP (u[b], u1, u2 - 1);
                      v[b] = CLAMP (v[b], v1, v2 - 1);
                    }

                  if (supersample &&
                      supersample_dtest (u[1], v[1], u[2], v[2],
                                         u[3], v[3], u[4], v[4]))
                    {
                      sample_adapt (orig_tiles,
                                    u[0]-u1, v[0]-v1,
                                    u[1]-u1, v[1]-v1,
                                    u[2]-u1, v[2]-v1,
                                    u[3]-u1, v[3]-v1,
                                    u[4]-u1, v[4]-v1,
                                    recursion_level,
                                    color, bg_color, bytes, alpha);
                    }
                  else
                    {
                      switch (interpolation_type)
                        {
                        case GIMP_INTERPOLATION_NONE:
                          break;
                        case GIMP_INTERPOLATION_LINEAR:
                          sample_linear (&surround, u[0] - u1, v[0] - v1,
                                         color, bytes, alpha);
                          break;
                        case GIMP_INTERPOLATION_CUBIC:
                          sample_cubic (&surround, u[0] - u1, v[0] - v1,
                                        color, bytes, alpha);
                          break;
                        case GIMP_INTERPOLATION_LANCZOS:
                          break;
                        }
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

  switch (interpolation_type)
    {
    case GIMP_INTERPOLATION_NONE:
      break;
    case GIMP_INTERPOLATION_CUBIC:
    case GIMP_INTERPOLATION_LINEAR:
      pixel_surround_clear (&surround);
      break;
    case GIMP_INTERPOLATION_LANCZOS:
      break;
    }

  g_free (dest);

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

    default:
      break;
    }

  new_tiles = tile_manager_new (new_width, new_height, orig_bpp);

  if (clip_result && (new_x != orig_y || new_y != orig_y))
    {
      guchar bg_color[MAX_CHANNELS];
      gint   clip_x, clip_y;
      gint   clip_width, clip_height;

      tile_manager_set_offsets (new_tiles, orig_x, orig_y);

      gimp_image_get_background (image, drawable, context, bg_color);

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

  if (flip_type == GIMP_ORIENTATION_HORIZONTAL)
    {
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
    }
  else
    {
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
      g_assert_not_reached ();
      return NULL;
    }

  if (clip_result && (new_x != orig_x || new_y != orig_y ||
                      new_width != orig_width || new_height != orig_height))

    {
      guchar bg_color[MAX_CHANNELS];
      gint   clip_x, clip_y;
      gint   clip_width, clip_height;

      new_tiles = tile_manager_new (orig_width, orig_height, orig_bpp);

      tile_manager_set_offsets (new_tiles, orig_x, orig_y);

      gimp_image_get_background (image, drawable, context, bg_color);

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
                                gboolean                supersample,
                                gint                    recursion_level,
                                gboolean                clip_result,
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
        clip_result = TRUE;

      /* transform the buffer */
      new_tiles = gimp_drawable_transform_tiles_affine (drawable, context,
                                                        orig_tiles,
                                                        matrix,
                                                        GIMP_TRANSFORM_FORWARD,
                                                        interpolation_type,
                                                        supersample,
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
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM, Q_("command|Flip"));

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
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM, Q_("command|Rotate"));

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
                                          drawable, context, TRUE, FALSE, TRUE);

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
                                        drawable, context, FALSE, TRUE, TRUE);
      else
        tiles = gimp_selection_extract (gimp_image_get_mask (image),
                                        drawable, context, FALSE, TRUE, FALSE);

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
                    alphachan[row], alphachan[row + bytes], du, dv);
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
                     BILINEAR (alphachan[0]           * data[i],
                               alphachan[bytes]       * data[bytes + i],
                               alphachan[row]         * data[row + i],
                               alphachan[row + bytes] * data[row + bytes + i],
                               du, dv));

      color[i] = CLAMP (newval, 0, 255);
    }

  pixel_surround_release (surround);
}

/* macros to handle conversion to/from fixed point, this fixed point code
 * uses signed integers, by using 8 bits for the fractional part we have
 *
 *  1 bit  sign
 * 21 bits integer part
 *  8 bit  fractional part
 *
 * 1023 discrete subpixel sample positions should be enough for the needs
 * of the supersampling algorithm, drawables where the dimensions have a need
 * exceeding 2^21 ( 2097152px, will typically use terabytes of memory, when
 * that is the common need, we can probably assume 64 bit integers and adjust
 * FIXED_SHIFT accordingly.
 */
#define FIXED_SHIFT          10
#define FIXED_UNIT           (1 << FIXED_SHIFT)
#define DOUBLE2FIXED(val)    ((val) * FIXED_UNIT)
#define FIXED2DOUBLE(val)    ((val) / FIXED_UNIT)

/*
    bilinear interpolation of a fixed point pixel
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
  gint   xscale = (x & (FIXED_UNIT-1));
  gint   yscale = (y & (FIXED_UNIT-1));

  gint   x0 = x >> FIXED_SHIFT;
  gint   y0 = y >> FIXED_SHIFT;
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
        (((guint)(v1) * (FIXED_UNIT - (guint)(r)) + \
          (guint)(v2) * (guint)(r)) >> FIXED_SHIFT)

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
  if (abs (x0 - x1) > FIXED_UNIT ||
      abs (x1 - x2) > FIXED_UNIT ||
      abs (x2 - x3) > FIXED_UNIT ||
      abs (x3 - x0) > FIXED_UNIT ||

      abs (y0 - y1) > FIXED_UNIT ||
      abs (y1 - y2) > FIXED_UNIT ||
      abs (y2 - y3) > FIXED_UNIT ||
      abs (y3 - y0) > FIXED_UNIT) return TRUE;

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
  if (fabs (x0 - x1) > 1.0 ||
      fabs (x1 - x2) > 1.0 ||
      fabs (x2 - x3) > 1.0 ||
      fabs (x3 - x0) > 1.0 ||

      fabs (y0 - y1) > 1.0 ||
      fabs (y1 - y2) > 1.0 ||
      fabs (y2 - y3) > 1.0 ||
      fabs (y3 - y0) > 1.0)
    return TRUE;

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
                DOUBLE2FIXED (xc), DOUBLE2FIXED (yc),
                DOUBLE2FIXED (x0), DOUBLE2FIXED (y0),
                DOUBLE2FIXED (x1), DOUBLE2FIXED (y1),
                DOUBLE2FIXED (x2), DOUBLE2FIXED (y2),
                DOUBLE2FIXED (x3), DOUBLE2FIXED (y3),
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


/*  Note: cubic function no longer clips result. */
/*  Inlining this function makes sample_cubic() run about 10% faster. (Sven) */
static inline gdouble
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
                      CUBIC_SCALED_ROW (du,
                                        i + data + row * 0,
                                        data + alpha + row * 0,
                                        bytes),
                      CUBIC_SCALED_ROW (du,
                                        i + data + row * 1,
                                        data + alpha + row * 1,
                                        bytes),
                      CUBIC_SCALED_ROW (du,
                                        i + data + row * 2,
                                        data + alpha + row * 2,
                                        bytes),
                      CUBIC_SCALED_ROW (du,
                                        i + data + row * 3,
                                        data + alpha + row * 3,
                                        bytes)));

      color[i] = CLAMP (newval, 0, 255);
    }

  pixel_surround_release (surround);
}

