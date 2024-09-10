/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "gimpcolortypes.h"

#include "gimpadaptivesupersample.h"


/**
 * SECTION: gimpadaptivesupersample
 * @title: GimpAdaptiveSupersample
 * @short_description: Functions to perform adaptive supersampling on
 *                     an area.
 *
 * Functions to perform adaptive supersampling on an area.
 **/


/*********************************************************************/
/* Sumpersampling code (Quartic)                                     */
/* This code is *largely* based on the sources for POV-Ray 3.0. I am */
/* grateful to the POV-Team for such a great program and for making  */
/* their sources available.  All comments / bug reports /            */
/* etc. regarding this code should be addressed to me, not to the    */
/* POV-Ray team.  Any bugs are my responsibility, not theirs.        */
/*********************************************************************/


typedef struct _GimpSampleType GimpSampleType;

struct _GimpSampleType
{
  guchar  ready;
  gdouble color[4];
};

static gdouble
gimp_rgba_distance_legacy (gdouble *rgba1,
                           gdouble *rgba2)
{
  g_return_val_if_fail (rgba1 != NULL, 0.0);
  g_return_val_if_fail (rgba2 != NULL, 0.0);

  return (fabs (rgba1[0] - rgba2[0]) +
          fabs (rgba1[1] - rgba2[1]) +
          fabs (rgba1[2] - rgba2[2]) +
          fabs (rgba1[3] - rgba2[3]));
}

static gulong
gimp_render_sub_pixel (gint             max_depth,
                       gint             depth,
                       GimpSampleType **block,
                       gint             x,
                       gint             y,
                       gint             x1,
                       gint             y1,
                       gint             x3,
                       gint             y3,
                       gdouble          threshold,
                       gint             sub_pixel_size,
                       gdouble         *color,
                       GimpRenderFunc   render_func,
                       gpointer         render_data)
{
  gint     x2, y2;                     /* Coords of center sample */
  gdouble  dx1, dy1;                   /* Delta to upper left sample */
  gdouble  dx3, dy3;                   /* Delta to lower right sample */
  gdouble  c0[4], c1[4], c2[4], c3[4]; /* Sample colors */
  gulong   num_samples = 0;

  g_return_val_if_fail (render_func != NULL, 0);

  /* Get offsets for corners */

  dx1 = (gdouble) (x1 - sub_pixel_size / 2) / sub_pixel_size;
  dx3 = (gdouble) (x3 - sub_pixel_size / 2) / sub_pixel_size;

  dy1 = (gdouble) (y1 - sub_pixel_size / 2) / sub_pixel_size;
  dy3 = (gdouble) (y3 - sub_pixel_size / 2) / sub_pixel_size;

  /* Render upper left sample */

  if (! block[y1][x1].ready)
    {
      num_samples++;

      render_func (x + dx1, y + dy1, c0, render_data);

      block[y1][x1].ready = TRUE;
      for (gint i = 0; i < 4; i++)
        block[y1][x1].color[i] = c0[i];
    }
  else
    {
      for (gint i = 0; i < 4; i++)
        c0[i] = block[y1][x1].color[i];
    }

  /* Render upper right sample */

  if (! block[y1][x3].ready)
    {
      num_samples++;

      render_func (x + dx3, y + dy1, c1, render_data);

      block[y1][x3].ready = TRUE;
      for (gint i = 0; i < 4; i++)
        block[y1][x3].color[i] = c1[i];
    }
  else
    {
      for (gint i = 0; i < 4; i++)
        c1[i] = block[y1][x3].color[i];
    }

  /* Render lower left sample */

  if (! block[y3][x1].ready)
    {
      num_samples++;

      render_func (x + dx1, y + dy3, c2, render_data);

      block[y3][x1].ready = TRUE;
      for (gint i = 0; i < 4; i++)
        block[y3][x1].color[i] = c2[i];
    }
  else
    {
      for (gint i = 0; i < 4; i++)
        c2[i] = block[y3][x1].color[i];
    }

  /* Render lower right sample */

  if (! block[y3][x3].ready)
    {
      num_samples++;

      render_func (x + dx3, y + dy3, c3, render_data);

      block[y3][x3].ready = TRUE;
      for (gint i = 0; i < 4; i++)
        block[y3][x3].color[i] = c3[i];
    }
  else
    {
      for (gint i = 0; i < 4; i++)
        c3[i] = block[y3][x3].color[i];
    }

  /* Check for supersampling */

  if (depth <= max_depth)
    {
      /* Check whether we have to supersample */

      if ((gimp_rgba_distance_legacy (c0, c1) >= threshold) ||
          (gimp_rgba_distance_legacy (c0, c2) >= threshold) ||
          (gimp_rgba_distance_legacy (c0, c3) >= threshold) ||
          (gimp_rgba_distance_legacy (c1, c2) >= threshold) ||
          (gimp_rgba_distance_legacy (c1, c3) >= threshold) ||
          (gimp_rgba_distance_legacy (c2, c3) >= threshold))
        {
          /* Calc coordinates of center subsample */

          x2 = (x1 + x3) / 2;
          y2 = (y1 + y3) / 2;

          /* Render sub-blocks */

          num_samples += gimp_render_sub_pixel (max_depth, depth + 1, block,
                                                x, y, x1, y1, x2, y2,
                                                threshold, sub_pixel_size,
                                                c0,
                                                render_func, render_data);

          num_samples += gimp_render_sub_pixel (max_depth, depth + 1, block,
                                                x, y, x2, y1, x3, y2,
                                                threshold, sub_pixel_size,
                                                c1,
                                                render_func, render_data);

          num_samples += gimp_render_sub_pixel (max_depth, depth + 1, block,
                                                x, y, x1, y2, x2, y3,
                                                threshold, sub_pixel_size,
                                                c2,
                                                render_func, render_data);

          num_samples += gimp_render_sub_pixel (max_depth, depth + 1, block,
                                                x, y, x2, y2, x3, y3,
                                                threshold, sub_pixel_size,
                                                c3,
                                                render_func, render_data);
        }
    }

  if (c0[3] == 0.0 || c1[3] == 0.0 || c2[3] == 0.0 || c3[3] == 0.0)
    {
      gdouble tmpcol[3] = { 0.0, 0.0, 0.0 };
      gdouble weight;

      weight = 2.0;

      if (c0[3] != 0.0)
        {
          tmpcol[0] += c0[0];
          tmpcol[1] += c0[1];
          tmpcol[2] += c0[2];

          weight /= 2.0;
        }
      if (c1[3] != 0.0)
        {
          tmpcol[0] += c1[0];
          tmpcol[1] += c1[1];
          tmpcol[2] += c1[2];

          weight /= 2.0;
        }
      if (c2[3] != 0.0)
        {
          tmpcol[0] += c2[0];
          tmpcol[1] += c2[1];
          tmpcol[2] += c2[2];

          weight /= 2.0;
        }
      if (c3[3] != 0.0)
        {
          tmpcol[0] += c3[0];
          tmpcol[1] += c3[1];
          tmpcol[2] += c3[2];

          weight /= 2.0;
        }

      color[0] = weight * tmpcol[0];
      color[1] = weight * tmpcol[1];
      color[2] = weight * tmpcol[2];
    }
  else
    {
      color[0] = 0.25 * (c0[0] + c1[0] + c2[0] + c3[0]);
      color[1] = 0.25 * (c0[1] + c1[1] + c2[1] + c3[1]);
      color[2] = 0.25 * (c0[2] + c1[2] + c2[2] + c3[2]);
    }

  color[3] = 0.25 * (c0[3] + c1[3] + c2[3] + c3[3]);

  return num_samples;
}

/**
 * gimp_adaptive_supersample_area:
 * @x1:             left x coordinate of the area to process.
 * @y1:             top y coordinate of the area to process.
 * @x2:             right x coordinate of the area to process.
 * @y2:             bottom y coordinate of the area to process.
 * @max_depth:      maximum depth of supersampling.
 * @threshold:      lower threshold of pixel difference that stops
 *                  supersampling.
 * @render_func:    (scope call): function calculate the color value at
 *                  given  coordinates.
 * @render_data:    user data passed to @render_func.
 * @put_pixel_func: (scope call): function to a pixels to a color at
 *                  given coordinates.
 * @put_pixel_data: user data passed to @put_pixel_func.
 * @progress_func:  (scope call): function to report progress.
 * @progress_data:  user data passed to @progress_func.
 *
 * Returns: the number of pixels processed.
 **/
gulong
gimp_adaptive_supersample_area (gint              x1,
                                gint              y1,
                                gint              x2,
                                gint              y2,
                                gint              max_depth,
                                gdouble           threshold,
                                GimpRenderFunc    render_func,
                                gpointer          render_data,
                                GimpPutPixelFunc  put_pixel_func,
                                gpointer          put_pixel_data,
                                GimpProgressFunc  progress_func,
                                gpointer          progress_data)
{
  gint             x, y, width;                 /* Counters, width of region */
  gint             xt, xtt, yt;                 /* Temporary counters */
  gint             sub_pixel_size;              /* Number of samples per pixel (1D) */
  gdouble          color[4];                    /* Rendered pixel's color */
  GimpSampleType   tmp_sample;                  /* For swapping samples */
  GimpSampleType  *top_row, *bot_row, *tmp_row; /* Sample rows */
  GimpSampleType **block;                       /* Sample block matrix */
  gulong           num_samples;

  g_return_val_if_fail (render_func != NULL, 0);
  g_return_val_if_fail (put_pixel_func != NULL, 0);

  /* Initialize color */

  for (gint i = 0; i < 4; i++)
    color[i] = 0.0;

  /* Calculate sub-pixel size */

  sub_pixel_size = 1 << max_depth;

  /* Create row arrays */

  width = x2 - x1 + 1;

  top_row = gegl_scratch_new (GimpSampleType, sub_pixel_size * width + 1);
  bot_row = gegl_scratch_new (GimpSampleType, sub_pixel_size * width + 1);

  for (x = 0; x < (sub_pixel_size * width + 1); x++)
    {
      top_row[x].ready = FALSE;
      bot_row[x].ready = FALSE;

      for (gint i = 0; i < 4; i++)
        {
          top_row[x].color[i] = 0.0;
          bot_row[x].color[i] = 0.0;
        }
    }

  /* Allocate block matrix */

  block = gegl_scratch_new (GimpSampleType *, sub_pixel_size + 1); /* Rows */

  for (y = 0; y < (sub_pixel_size + 1); y++)
    {
      block[y] = gegl_scratch_new (GimpSampleType, sub_pixel_size + 1); /* Columns */

      for (x = 0; x < (sub_pixel_size + 1); x++)
        {
          block[y][x].ready = FALSE;

          for (gint i = 0; i < 4; i++)
            block[y][x].color[i] = 0.0;
        }
    }

  /* Render region */

  num_samples = 0;

  for (y = y1; y <= y2; y++)
    {
      /* Clear the bottom row */

      for (xt = 0; xt < (sub_pixel_size * width + 1); xt++)
        bot_row[xt].ready = FALSE;

      /* Clear first column */

      for (yt = 0; yt < (sub_pixel_size + 1); yt++)
        block[yt][0].ready = FALSE;

      /* Render row */

      for (x = x1; x <= x2; x++)
        {
          /* Initialize block by clearing all but first row/column */

          for (yt = 1; yt < (sub_pixel_size + 1); yt++)
            for (xt = 1; xt < (sub_pixel_size + 1); xt++)
              block[yt][xt].ready = FALSE;

          /* Copy samples from top row to block */

          for (xtt = 0, xt = (x - x1) * sub_pixel_size;
               xtt < (sub_pixel_size + 1);
               xtt++, xt++)
            block[0][xtt] = top_row[xt];

          /* Render pixel on (x, y) */

          num_samples += gimp_render_sub_pixel (max_depth, 1, block, x, y, 0, 0,
                                                sub_pixel_size, sub_pixel_size,
                                                threshold, sub_pixel_size,
                                                color,
                                                render_func, render_data);

          if (put_pixel_func)
            (* put_pixel_func) (x, y, color, put_pixel_data);

          /* Copy block information to rows */

          top_row[(x - x1 + 1) * sub_pixel_size] = block[0][sub_pixel_size];

          for (xtt = 0, xt = (x - x1) * sub_pixel_size;
               xtt < (sub_pixel_size + 1);
               xtt++, xt++)
            bot_row[xt] = block[sub_pixel_size][xtt];

          /* Swap first and last columns */

          for (yt = 0; yt < (sub_pixel_size + 1); yt++)
            {
              tmp_sample                = block[yt][0];
              block[yt][0]              = block[yt][sub_pixel_size];
              block[yt][sub_pixel_size] = tmp_sample;
            }
        }

      /* Swap rows */

      tmp_row = top_row;
      top_row = bot_row;
      bot_row = tmp_row;

      /* Call progress display function (if any) */

      if (progress_func != NULL)
        (* progress_func) (y1, y2, y, progress_data);
    }

  /* Free memory */

  for (y = 0; y < (sub_pixel_size + 1); y++)
    gegl_scratch_free (block[y]);

  gegl_scratch_free (block);
  gegl_scratch_free (top_row);
  gegl_scratch_free (bot_row);

  return num_samples;
}
