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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "base-types.h"

#include "curves.h"
#include "gimplut.h"


typedef gdouble CRMatrix[4][4];


/*  local function prototypes  */

static void   curves_plot_curve (Curves   *curves,
                                 gint      channel,
                                 gint      p1,
                                 gint      p2,
                                 gint      p3,
                                 gint      p4);
static void   curves_CR_compose (CRMatrix  a,
                                 CRMatrix  b,
                                 CRMatrix  ab);


/*  private variables  */

static CRMatrix CR_basis =
{
  { -0.5,  1.5, -1.5,  0.5 },
  {  1.0, -2.5,  2.0, -0.5 },
  { -0.5,  0.0,  0.5,  0.0 },
  {  0.0,  1.0,  0.0,  0.0 },
};


/*  public functions  */

void
curves_init (Curves *curves)
{
  GimpHistogramChannel channel;

  g_return_if_fail (curves != NULL);

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      curves->curve_type[channel] = GIMP_CURVE_SMOOTH;

      curves_channel_reset (curves, channel);
    }
}

void
curves_channel_reset (Curves               *curves,
                      GimpHistogramChannel  channel)
{
  gint j;

  g_return_if_fail (curves != NULL);

  for (j = 0; j < 256; j++)
    curves->curve[channel][j] = j;

  for (j = 0; j < CURVES_NUM_POINTS; j++)
    {
      curves->points[channel][j][0] = -1;
      curves->points[channel][j][1] = -1;
    }

  curves->points[channel][0][0]  = 0;
  curves->points[channel][0][1]  = 0;
  curves->points[channel][CURVES_NUM_POINTS - 1][0] = 255;
  curves->points[channel][CURVES_NUM_POINTS - 1][1] = 255;
}

void
curves_calculate_curve (Curves               *curves,
                        GimpHistogramChannel  channel)
{
  gint i;
  gint points[CURVES_NUM_POINTS];
  gint num_pts;
  gint p1, p2, p3, p4;

  g_return_if_fail (curves != NULL);

  switch (curves->curve_type[channel])
    {
    case GIMP_CURVE_FREE:
      break;

    case GIMP_CURVE_SMOOTH:
      /*  cycle through the curves  */
      num_pts = 0;
      for (i = 0; i < CURVES_NUM_POINTS; i++)
	if (curves->points[channel][i][0] != -1)
	  points[num_pts++] = i;

      /*  Initialize boundary curve points */
      if (num_pts != 0)
	{
	  for (i = 0; i < curves->points[channel][points[0]][0]; i++)
	    curves->curve[channel][i] = curves->points[channel][points[0]][1];

	  for (i = curves->points[channel][points[num_pts - 1]][0];
               i < 256;
               i++)
	    curves->curve[channel][i] =
              curves->points[channel][points[num_pts - 1]][1];
	}

      for (i = 0; i < num_pts - 1; i++)
	{
	  p1 = (i == 0) ? points[i] : points[(i - 1)];
	  p2 = points[i];
	  p3 = points[i + 1];
	  p4 = (i == (num_pts - 2)) ? points[num_pts - 1] : points[i + 2];

	  curves_plot_curve (curves, channel, p1, p2, p3, p4);
	}

      /* ensure that the control points are used exactly */
      for (i = 0; i < num_pts; i++)
        {
          gint x = curves->points[channel][points[i]][0];
          gint y = curves->points[channel][points[i]][1];

          curves->curve[channel][x] = y;
        }

      break;
    }
}

gfloat
curves_lut_func (Curves *curves,
		 gint    n_channels,
		 gint    channel,
		 gfloat  value)
{
  gfloat  f;
  gint    index;
  gdouble inten;
  gint    j;

  if (n_channels <= 2)
    j = channel;
  else
    j = channel + 1;

  inten = value;

  /* For RGB and RGBA images this runs through the loop with j = channel + 1
   * the first time and j = 0 the second time
   *
   * For GRAY images this runs through the loop with j = 0 the first and
   * only time
   */
  for (; j >= 0; j -= (channel + 1))
    {
      /* don't apply the overall curve to the alpha channel */
      if (j == 0 && (n_channels == 2 || n_channels == 4) &&
          channel == n_channels - 1)
        return inten;

      if (inten < 0.0)
        {
          inten = curves->curve[j][0]/255.0;
        }
      else if (inten >= 1.0)
        {
          inten = curves->curve[j][255]/255.0;
        }
      else /* interpolate the curve */
        {
          index = floor (inten * 255.0);
          f = inten * 255.0 - index;
          inten = ((1.0 - f) * curves->curve[j][index    ] +
                          f  * curves->curve[j][index + 1] ) / 255.0;
        }
    }

  return inten;
}


/*  private functions  */

/*  this can be adjusted to give a finer or coarser curve  */
#define CURVES_SUBDIVIDE  1000

static void
curves_plot_curve (Curves *curves,
                   gint    channel,
		   gint    p1,
		   gint    p2,
		   gint    p3,
		   gint    p4)
{
  CRMatrix geometry;
  CRMatrix tmp1, tmp2;
  CRMatrix deltas;
  gdouble  x, dx, dx2, dx3;
  gdouble  y, dy, dy2, dy3;
  gdouble  d, d2, d3;
  gint     lastx, lasty;
  gint32   newx, newy;
  gint     i;

  /* construct the geometry matrix from the segment */
  for (i = 0; i < 4; i++)
    {
      geometry[i][2] = 0;
      geometry[i][3] = 0;
    }

  for (i = 0; i < 2; i++)
    {
      geometry[0][i] = curves->points[channel][p1][i];
      geometry[1][i] = curves->points[channel][p2][i];
      geometry[2][i] = curves->points[channel][p3][i];
      geometry[3][i] = curves->points[channel][p4][i];
    }

  /* subdivide the curve */
  d = 1.0 / CURVES_SUBDIVIDE;
  d2 = d * d;
  d3 = d * d * d;

  /* construct a temporary matrix for determining the forward
   * differencing deltas
   */
  tmp2[0][0] = 0;       tmp2[0][1] = 0;       tmp2[0][2] = 0;  tmp2[0][3] = 1;
  tmp2[1][0] = d3;      tmp2[1][1] = d2;      tmp2[1][2] = d;  tmp2[1][3] = 0;
  tmp2[2][0] = 6 * d3;  tmp2[2][1] = 2 * d2;  tmp2[2][2] = 0;  tmp2[2][3] = 0;
  tmp2[3][0] = 6 * d3;  tmp2[3][1] = 0;       tmp2[3][2] = 0;  tmp2[3][3] = 0;

  /* compose the basis and geometry matrices */
  curves_CR_compose (CR_basis, geometry, tmp1);

  /* compose the above results to get the deltas matrix */
  curves_CR_compose (tmp2, tmp1, deltas);

  /* extract the x deltas */
  x   = deltas[0][0];
  dx  = deltas[1][0];
  dx2 = deltas[2][0];
  dx3 = deltas[3][0];

  /* extract the y deltas */
  y   = deltas[0][1];
  dy  = deltas[1][1];
  dy2 = deltas[2][1];
  dy3 = deltas[3][1];

  lastx = CLAMP (x, 0, 255);
  lasty = CLAMP (y, 0, 255);

  curves->curve[channel][lastx] = lasty;

  /* loop over the curve */
  for (i = 0; i < CURVES_SUBDIVIDE; i++)
    {
      /* increment the x values */
      x += dx;
      dx += dx2;
      dx2 += dx3;

      /* increment the y values */
      y += dy;
      dy += dy2;
      dy2 += dy3;

      newx = CLAMP0255 (ROUND (x));
      newy = CLAMP0255 (ROUND (y));

      /* if this point is different than the last one...then draw it */
      if ((lastx != newx) || (lasty != newy))
	curves->curve[channel][newx] = newy;

      lastx = newx;
      lasty = newy;
    }
}

static void
curves_CR_compose (CRMatrix a,
		   CRMatrix b,
		   CRMatrix ab)
{
  gint i, j;

  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      ab[i][j] = (a[i][0] * b[0][j] +
                  a[i][1] * b[1][j] +
                  a[i][2] * b[2][j] +
                  a[i][3] * b[3][j]);
}
