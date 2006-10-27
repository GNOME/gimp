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

#include "paint-funcs/scale-funcs.h"

#include "gimp-transform-region.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimppickable.h"
#include "gimpprogress.h"


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


/*  public functions  */

void
gimp_transform_region (GimpPickable          *pickable,
                       GimpContext           *context,
                       TileManager           *orig_tiles,
                       PixelRegion           *destPR,
                       gint                   dest_x1,
                       gint                   dest_y1,
                       gint                   dest_x2,
                       gint                   dest_y2,
                       const GimpMatrix3     *matrix,
                       GimpInterpolationType  interpolation_type,
                       gboolean               supersample,
                       gint                   recursion_level,
                       GimpProgress          *progress)
{
  PixelSurround  surround;
  GimpImageType  pickable_type;
  GimpMatrix3    m;

  gint           x, y;                 /* target coordinates */
  gint           u1, v1, u2, v2;       /* source bounding box */
  gdouble        uinc, vinc, winc;     /* increments in source coordinates
                                        * per horizontal target coordinate
                                        */
  gdouble        u[5], v[5];           /* source coordinates,
                                        *
                                        *   2     0    is sample in the center
                                        *  / \         of pixel
                                        * 1 0 3   1..4 is offset 1 pixel in
                                        *  \ /         each direction
                                        *   4          (in target space)
                                        */
  gdouble        tu[5], tv[5], tw[5];  /* undivided source coordinates and
                                        * divisor
                                        */
  gint           coords;
  gint           alpha;
  gint           width;
  gint           bytes;
  guchar        *dest, *d;
  guchar         bg_color[MAX_CHANNELS];

  tile_manager_get_offsets (orig_tiles, &u1, &v1);
  u2 = u1 + tile_manager_width (orig_tiles);
  v2 = v1 + tile_manager_height (orig_tiles);

  m = *matrix;
  gimp_matrix3_invert (&m);

  alpha = 0;
  width = dest_x2 - dest_x1;
  bytes = destPR->bytes;

  /*  turn interpolation off for simple transformations (e.g. rot90)  */
  if (gimp_matrix3_is_simple (matrix))
    interpolation_type = GIMP_INTERPOLATION_NONE;

  pickable_type = gimp_pickable_get_image_type (pickable);

  /*  Get the background color  */
  gimp_image_get_background (gimp_pickable_get_image (pickable), context,
                             pickable_type, bg_color);

  switch (GIMP_IMAGE_TYPE_BASE_TYPE (pickable_type))
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
  if (GIMP_IS_CHANNEL (pickable))
    bg_color[0] = TRANSPARENT_OPACITY;

  /*  setting alpha = 0 will cause the channel's value to be treated
   *  as alpha and the color channel loops never to be entered
   */
  if (tile_manager_bpp (orig_tiles) == 1)
    alpha = 0;

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

  dest = g_new (guchar, width * bytes);

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

      for (y = dest_y1; y < dest_y2; y++)
        {
          if (progress && !(y & 0xf))
            gimp_progress_set_value (progress,
                                     (gdouble) (y - dest_y1) /
                                     (gdouble) (dest_y2 - dest_y1));

          pixel_region_get_row (destPR, 0, (y - dest_y1), width, dest, 1);

          d = dest;

          for (x = dest_x1; x < dest_x2; x++)
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

              if (u <  u1 || v <  v1 ||
                  u >= u2 || v >= v2)
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

                        read_pixel_data_1 (orig_tiles, pu - u1, pv - v1,
                                           &lwin[pos][0]);
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
          pixel_region_set_row (destPR, 0, (y - dest_y1), width, dest);
        }

      g_free (lanczos);

      goto done;
    }

  uinc = m.coeff[0][0];
  vinc = m.coeff[1][0];
  winc = m.coeff[2][0];

  coords = (interpolation_type != GIMP_INTERPOLATION_NONE) ? 5 : 1;

  for (y = dest_y1; y < dest_y2; y++)
    {
      if (progress && !(y & 0xf))
        gimp_progress_set_value (progress,
                                 (gdouble) (y - dest_y1) /
                                 (gdouble) (dest_y2 - dest_y1));

      /* set up inverse transform steps */
      tu[0] = uinc * dest_x1 + m.coeff[0][1] * y + m.coeff[0][2];
      tv[0] = vinc * dest_x1 + m.coeff[1][1] * y + m.coeff[1][2];
      tw[0] = winc * dest_x1 + m.coeff[2][1] * y + m.coeff[2][2];

      if (interpolation_type != GIMP_INTERPOLATION_NONE)
        {
          gdouble xx = dest_x1;
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

      for (x = dest_x1; x < dest_x2; x++)
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
      pixel_region_set_row (destPR, 0, (y - dest_y1), width, dest);
    }

 done:

  if (progress)
    gimp_progress_set_value (progress, 1.0);

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
}


/*  private functions  */

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

static inline gdouble
sinc (gdouble x)
{
  gdouble y = x * G_PI;

  if (ABS (x) < EPSILON)
    return 1.0;

  return sin (y) / y;
}

static gdouble *
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
