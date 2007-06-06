/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Colormap-Rotation plug-in. Exchanges two color ranges.
 *
 * Copyright (C) 1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                    Based on code from Pavel Grinfeld (pavel@ml.com)
 *
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

/*----------------------------------------------------------------------------
 * Change log:
 *
 * Version 2.0, 04 April 1999.
 *  Nearly complete rewrite, made plug-in stable.
 *  (Works with GIMP 1.1 and GTK+ 1.2)
 *
 * Version 1.0, 27 March 1997.
 *  Initial (unstable) release by Pavel Grinfeld
 *
 *----------------------------------------------------------------------------*/

#include "config.h"

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "rcm.h"
#include "rcm_misc.h"
#include "rcm_gdk.h"

float
arctg (float y,
       float x)
{
  float temp = atan2(y,x);
  return (temp<0) ? (temp+TP) : temp;
}

float
min_prox (float alpha,
	  float beta,
	  float angle)
{
  gfloat temp1 = MIN (angle_mod_2PI (alpha - angle),
                      TP - angle_mod_2PI (alpha - angle));
  gfloat temp2 = MIN (angle_mod_2PI (beta - angle),
                      TP - angle_mod_2PI (beta - angle));

  return MIN(temp1, temp2);
}

float*
closest (float *alpha,
	 float *beta,
	 float  angle)
{
  gfloat temp_alpha = MIN (angle_mod_2PI (*alpha-angle),
                           TP - angle_mod_2PI (*alpha-angle));

  gfloat temp_beta  = MIN (angle_mod_2PI (*beta -angle),
                           TP - angle_mod_2PI (*beta -angle));

  if (temp_alpha-temp_beta < 0)
    return alpha;
  else
    return beta;
}

float
angle_mod_2PI (float angle)
{
  if (angle < 0)
    return angle + TP;
  else if (angle > TP)
    return angle - TP;
  else
    return angle;
}


/* supporting routines  */

float
rcm_linear (float A,
	    float B,
	    float C,
	    float D,
	    float x)
{
  if (B > A)
    {
      if (A<=x && x<=B)
        return C+(D-C)/(B-A)*(x-A);
      else if (A<=x+TP && x+TP<=B)
        return C+(D-C)/(B-A)*(x+TP-A);
      else
        return x;
    }
  else
    {
      if (B<=x && x<=A)
        return C+(D-C)/(B-A)*(x-A);
      else if (B<=x+TP && x+TP<=A)
        return C+(D-C)/(B-A)*(x+TP-A);
      else
        return x;
    }
}

float
rcm_left_end (RcmAngle *angle)
{
  gfloat alpha  = angle->alpha;
  gfloat beta   = angle->beta;
  gint   cw_ccw = angle->cw_ccw;

  switch (cw_ccw)
    {
    case (-1):
      if (alpha < beta) return alpha + TP;

    default:
      return alpha; /* 1 */
    }
}

float
rcm_right_end (RcmAngle *angle)
{
  gfloat alpha  = angle->alpha;
  gfloat beta   = angle->beta;
  gint   cw_ccw = angle->cw_ccw;

  switch (cw_ccw)
    {
    case 1:
      if (beta < alpha) return beta + TP;

    default:
      return beta; /* -1 */
    }
}

float
rcm_angle_inside_slice (float     angle,
			RcmAngle *slice)
{
  return angle_mod_2PI (slice->cw_ccw * (slice->beta-angle)) /
         angle_mod_2PI (slice->cw_ccw * (slice->beta-slice->alpha));
}

gboolean
rcm_is_gray (float s)
{
  return (s <= Current.Gray->gray_sat);
}


/* reduce image/selection for preview */

ReducedImage*
rcm_reduce_image (GimpDrawable *drawable,
		  GimpDrawable *mask,
		  gint          LongerSize,
		  gint          Slctn)
{
  guint32       image;
  GimpPixelRgn  srcPR, srcMask;
  ReducedImage *temp;
  guchar       *tempRGB, *src_row, *tempmask, *src_mask_row;
  gint          i, j, x1, x2, y1, y2;
  gint          RH, RW, width, height, bytes;
  gboolean      NoSelectionMade;
  gint          offx, offy;
  gdouble      *tempHSV, H, S, V;

  bytes = drawable->bpp;
  temp = g_new0 (ReducedImage, 1);

  /* get bounds of image or selection */

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  if (((x2-x1) != drawable->width) || ((y2-y1) != drawable->height))
    NoSelectionMade = FALSE;
  else
    NoSelectionMade = TRUE;

  switch (Slctn)
    {
    case ENTIRE_IMAGE:
      x1 = 0;
      x2 = drawable->width;
      y1 = 0;
      y2 = drawable->height;
      break;

    case SELECTION_IN_CONTEXT:
      x1 = MAX (0, x1 - (x2-x1) / 2.0);
      x2 = MIN (drawable->width, x2 + (x2-x1) / 2.0);
      y1 = MAX (0, y1 - (y2-y1) / 2.0);
      y2 = MIN (drawable->height, y2 + (y2-y1) / 2.0);
      break;

    default:
      break; /* take selection dimensions */
    }

  /* clamp to image size since this is the size of the mask */

  gimp_drawable_offsets (drawable->drawable_id, &offx, &offy);
  image = gimp_drawable_get_image (drawable->drawable_id);

  x1 = CLAMP (x1, - offx, gimp_image_width (image) - offx);
  x2 = CLAMP (x2, - offx, gimp_image_width (image) - offx);
  y1 = CLAMP (y1, - offy, gimp_image_height (image) - offy);
  y2 = CLAMP (y2, - offy, gimp_image_height (image) - offy);

  /* calculate size of preview */

  width  = x2 - x1;
  height = y2 - y1;

  if (width < 1 || height < 1)
    return temp;

  if (width > height)
    {
      RW = LongerSize;
      RH = (float) height * (float) LongerSize / (float) width;
    }
  else
    {
      RH = LongerSize;
      RW = (float)width * (float) LongerSize / (float) height;
    }

  /* allocate memory */

  tempRGB  = g_new (guchar, RW * RH * bytes);
  tempHSV  = g_new (gdouble, RW * RH * bytes);
  tempmask = g_new (guchar, RW * RH);

  gimp_pixel_rgn_init (&srcPR, drawable, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&srcMask, mask,
                       x1 + offx, y1 + offy, width, height, FALSE, FALSE);

  src_row = g_new (guchar, width * bytes);
  src_mask_row = g_new (guchar, width * bytes);

  /* reduce image */

  for (i = 0; i < RH; i++)
    {
      gint whichcol, whichrow;

      whichrow = (float)i * (float)height / (float)RH;
      gimp_pixel_rgn_get_row (&srcPR, src_row, x1, y1 + whichrow, width);
      gimp_pixel_rgn_get_row (&srcMask, src_mask_row,
                              x1 + offx, y1 + offy + whichrow, width);

      for (j = 0; j < RW; j++)
        {
          whichcol = (float)j * (float)width / (float)RW;

          if (NoSelectionMade)
            tempmask[i*RW+j] = 255;
          else
            tempmask[i*RW+j] = src_mask_row[whichcol];

          gimp_rgb_to_hsv4 (&src_row[whichcol*bytes], &H, &S, &V);

          tempRGB[i*RW*bytes+j*bytes+0] = src_row[whichcol*bytes+0];
          tempRGB[i*RW*bytes+j*bytes+1] = src_row[whichcol*bytes+1];
          tempRGB[i*RW*bytes+j*bytes+2] = src_row[whichcol*bytes+2];

          tempHSV[i*RW*bytes+j*bytes+0] = H;
          tempHSV[i*RW*bytes+j*bytes+1] = S;
          tempHSV[i*RW*bytes+j*bytes+2] = V;

          if (bytes == 4)
            tempRGB[i*RW*bytes+j*bytes+3] = src_row[whichcol*bytes+3];
        }
    }

  /* return values */

  temp->width  = RW;
  temp->height = RH;
  temp->rgb    = tempRGB;
  temp->hsv    = tempHSV;
  temp->mask   = tempmask;

  return temp;
}


/* render before/after preview */
void
rcm_render_preview (GtkWidget *preview)
{
  ReducedImage *reduced;
  gint          version;
  gint          RW, RH, bytes, i, j;
  guchar       *a;
  guchar       *rgb_array;
  gdouble      *hsv_array;
  gfloat        degree;

  g_return_if_fail (preview != NULL);

  version = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (preview), "mode"));

  reduced   = Current.reduced;
  RW        = reduced->width;
  RH        = reduced->height;
  bytes     = Current.drawable->bpp;
  hsv_array = reduced->hsv;
  rgb_array = reduced->rgb;

  a = g_new (guchar, 4 * RW * RH);

  if (version == CURRENT)
    {
      gdouble H, S, V;
      guchar  rgb[3];

      for (i = 0; i < RH; i++)
        {
          for (j = 0; j < RW; j++)
            {
              gboolean unchanged = FALSE;
              gboolean skip      = FALSE;

              H = hsv_array[i*RW*bytes + j*bytes + 0];
              S = hsv_array[i*RW*bytes + j*bytes + 1];
              V = hsv_array[i*RW*bytes + j*bytes + 2];

              if (rcm_is_gray(S) && (reduced->mask[i*RW+j] != 0))
                {
                  switch (Current.Gray_to_from)
                    {
                    case GRAY_FROM:
                      if (rcm_angle_inside_slice (Current.Gray->hue,
                                                  Current.From->angle) <= 1)
                        {
                          H = Current.Gray->hue/TP;
                          S = Current.Gray->satur;
                        }
                      else
                        skip = TRUE;
                      break;

                    case GRAY_TO:
                      unchanged = FALSE;
                      skip = TRUE;
                      gimp_hsv_to_rgb4 (rgb,
                                        Current.Gray->hue/TP,
                                        Current.Gray->satur,
                                        V);
                      break;

                    default:
                      break;
                    }
                }

              if (!skip)
                {
                  unchanged = FALSE;
                  H = rcm_linear (rcm_left_end (Current.From->angle),
                                  rcm_right_end (Current.From->angle),
                                  rcm_left_end (Current.To->angle),
                                  rcm_right_end (Current.To->angle),
                                  H * TP);

                  H = angle_mod_2PI(H) / TP;
                  gimp_hsv_to_rgb4 (rgb, H,S,V);
                }

              if (unchanged)
                degree = 0;
              else
                degree = reduced->mask[i*RW+j] / 255.0;

              a[(i*RW+j)*4+0] = (1-degree) * rgb_array[i*RW*bytes + j*bytes + 0] + degree * rgb[0];
              a[(i*RW+j)*4+1] = (1-degree) * rgb_array[i*RW*bytes + j*bytes + 1] + degree * rgb[1];
              a[(i*RW+j)*4+2] = (1-degree) * rgb_array[i*RW*bytes + j*bytes + 2] + degree * rgb[2];

              /* apply transparency */
              if (bytes == 4)
                a[(i*RW+j)*4+3] = rgb_array[i*RW*bytes+j*bytes+3];
              else
                a[(i*RW+j)*4+3] = 255;
            }
        }
    }
  else /* ORIGINAL */
    {
      for (i = 0; i < RH; i++)
        {
          for (j = 0; j < RW; j++)
            {
              a[(i*RW+j)*4+0] = rgb_array[i*RW*bytes + j*bytes + 0];
              a[(i*RW+j)*4+1] = rgb_array[i*RW*bytes + j*bytes + 1];
              a[(i*RW+j)*4+2] = rgb_array[i*RW*bytes + j*bytes + 2];

              if (bytes == 4)
                a[(i*RW+j)*4+3] = rgb_array[i*RW*bytes+j*bytes+3];
              else
                a[(i*RW+j)*4+3] = 255;
            }
        }
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                          0, 0, RW, RH,
                          GIMP_RGBA_IMAGE,
                          a,
                          RW * 4);
  g_free (a);
}

/* render circle */

void
rcm_render_circle (GtkWidget *preview,
		   int        sum,
		   int        margin)
{
  gint i, j;
  gdouble h, s, v;
  guchar *a;

  if (preview == NULL) return;

  a = g_new (guchar, 3*sum*sum);

  for (j = 0; j < sum; j++)
    {
      for (i = 0; i < sum; i++)
        {
          s = sqrt ((SQR (i - sum / 2.0) + SQR (j - sum / 2.0)) / (float) SQR (sum / 2.0 - margin));
          if (s > 1)
            {
              a[(j*sum+i)*3+0] = 255;
              a[(j*sum+i)*3+1] = 255;
              a[(j*sum+i)*3+2] = 255;
            }
          else
            {
              h = arctg (sum / 2.0 - j, i - sum / 2.0) / (2 * G_PI);
              v = 1 - sqrt (s) / 4;
              gimp_hsv_to_rgb4 (&a[(j*sum+i)*3], h, s, v);
            }
        }
    }
  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                          0, 0, sum, sum,
                          GIMP_RGB_IMAGE,
                          a,
                          sum * 3);
  g_free (a);
}
