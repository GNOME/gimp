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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "base-types.h"

#include "curves.h"
#include "gimplut.h"



/*  local function prototypes  */

static void  curves_plot_curve (Curves *curves,
                                gint    channel,
                                gint    p1,
                                gint    p2,
                                gint    p3,
                                gint    p4);


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
          p1 = points[MAX (i - 1, 0)];
          p2 = points[i];
          p3 = points[i + 1];
          p4 = points[MIN (i + 2, num_pts - 1)];

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

/*
 * This function calculates the curve values between the control points
 * p2 and p3, taking the potentially existing neighbors p1 and p4 into
 * account.
 *
 * This function uses a cubic bezier curve for the individual segments and
 * calculates the necessary intermediate control points depending on the
 * neighbor curve control points.
 */

static void
curves_plot_curve (Curves *curves,
                   gint    channel,
                   gint    p1,
                   gint    p2,
                   gint    p3,
                   gint    p4)
{
  gint    i;
  gdouble x0, x3;
  gdouble y0, y1, y2, y3;
  gdouble dx, dy;
  gdouble y, t;
  gdouble slope;

  /* the outer control points for the bezier curve. */
  x0 = curves->points[channel][p2][0];
  y0 = curves->points[channel][p2][1];
  x3 = curves->points[channel][p3][0];
  y3 = curves->points[channel][p3][1];

  /*
   * the x values of the inner control points are fixed at
   * x1 = 1/3*x0 + 2/3*x3   and  x2 = 2/3*x0 + 1/3*x3
   * this ensures that the x values increase linearily with the
   * parameter t and enables us to skip the calculation of the x
   * values altogehter - just calculate y(t) evenly spaced.
   */

  dx = x3 - x0;
  dy = y3 - y0;

  g_return_if_fail (dx > 0);

  if (p1 == p2 && p3 == p4)
    {
      /* No information about the neighbors,
       * calculate y1 and y2 to get a straight line
       */
      y1 = y0 + dy / 3.0;
      y2 = y0 + dy * 2.0 / 3.0;
    }
  else if (p1 == p2 && p3 != p4)
    {
      /* only the right neighbor is available. Make the tangent at the
       * right endpoint parallel to the line between the left endpoint
       * and the right neighbor. Then point the tangent at the left towards
       * the control handle of the right tangent, to ensure that the curve
       * does not have an inflection point.
       */
      slope = (curves->points[channel][p4][1] - y0) /
              (curves->points[channel][p4][0] - x0);

      y2 = y3 - slope * dx / 3.0;
      y1 = y0 + (y2 - y0) / 2.0;
    }
  else if (p1 != p2 && p3 == p4)
    {
      /* see previous case */
      slope = (y3 - curves->points[channel][p1][1]) /
              (x3 - curves->points[channel][p1][0]);

      y1 = y0 + slope * dx / 3.0;
      y2 = y3 + (y1 - y3) / 2.0;
    }
  else /* (p1 != p2 && p3 != p4) */
    {
      /* Both neighbors are available. Make the tangents at the endpoints
       * parallel to the line between the opposite endpoint and the adjacent
       * neighbor.
       */
      slope = (y3 - curves->points[channel][p1][1]) /
              (x3 - curves->points[channel][p1][0]);

      y1 = y0 + slope * dx / 3.0;

      slope = (curves->points[channel][p4][1] - y0) /
              (curves->points[channel][p4][0] - x0);

      y2 = y3 - slope * dx / 3.0;
    }

  /*
   * finally calculate the y(t) values for the given bezier values. We can
   * use homogenously distributed values for t, since x(t) increases linearily.
   */
  for (i = 0; i <= dx; i++)
    {
      t = i / dx;
      y =     y0 * (1-t) * (1-t) * (1-t) +
          3 * y1 * (1-t) * (1-t) * t     +
          3 * y2 * (1-t) * t     * t     +
              y3 * t     * t     * t;

      curves->curve[channel][ROUND(x0) + i] = CLAMP0255 (ROUND (y));
    }
}

