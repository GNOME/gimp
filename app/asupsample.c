/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * General, non-jittered adaptive supersampling library
 * Copyright (C) 1997 Federico Mena Quintero
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
 *
 */

/* This code is *largely* based on the sources for POV-Ray 3.0.  I am
 * grateful to the POV-Team for such a great program and for making
 * their sources available.  All comments / bug reports /
 * etc. regarding this library should be addressed to me, not to the
 * POV-Ray team.  Any bugs are my responsibility, not theirs.
 */

#include "config.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "apptypes.h"

#include "appenv.h"
#include "asupsample.h"
#include "gimpprogress.h"

#include "libgimp/gimpmath.h"


typedef struct
{
  gchar   ready;
  GimpRGB color;
} GimpSampleType;


/***** Local functions *****/

static gulong gimp_render_sub_pixel (gint             max_depth,
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
				     GimpRGB         *color,
				     GimpRenderFunc   render_func,
				     gpointer         render_data);


/***** Functions *****/

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
  gint             sub_pixel_size;              /* Numbe of samples per pixel (1D) */
  GimpRGB          color;                       /* Rendered pixel's color */
  GimpSampleType   tmp_sample;                  /* For swapping samples */
  GimpSampleType  *top_row, *bot_row, *tmp_row; /* Sample rows */
  GimpSampleType **block;                       /* Sample block matrix */
  gulong           num_samples;

  /* Initialize color */

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 0.0);

  /* Calculate sub-pixel size */

  sub_pixel_size = 1 << max_depth;

  /* Create row arrays */

  width = x2 - x1 + 1;

  top_row = g_new (GimpSampleType, sub_pixel_size * width + 1);
  bot_row = g_new (GimpSampleType, sub_pixel_size * width + 1);

  for (x = 0; x < (sub_pixel_size * width + 1); x++)
    {
      top_row[x].ready = FALSE;

      gimp_rgba_set (&top_row[x].color, 0.0, 0.0, 0.0, 0.0);

      bot_row[x].ready = FALSE;
		
      gimp_rgba_set (&bot_row[x].color, 0.0, 0.0, 0.0, 0.0);
    }

  /* Allocate block matrix */

  block = g_new (GimpSampleType *, sub_pixel_size + 1); /* Rows */

  for (y = 0; y < (sub_pixel_size + 1); y++)
    {
      block[y] = g_new (GimpSampleType, sub_pixel_size + 1); /* Columns */

      for (x = 0; x < (sub_pixel_size + 1); x++)
	{
	  block[y][x].ready = FALSE;

	  gimp_rgba_set (&block[y][x].color, 0.0, 0.0, 0.0, 0.0);
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
	block[yt][0].ready = 0;

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
						&color,
						render_func, render_data);

	  if (put_pixel_func)
	    (* put_pixel_func) (x, y, &color, put_pixel_data);

	  /* Copy block information to rows */

	  top_row[((x - x1) + 1) * sub_pixel_size] = block[0][sub_pixel_size];

	  for (xtt = 0, xt = (x - x1) * sub_pixel_size;
	       xtt < (sub_pixel_size + 1);
	       xtt++, xt++)
	    bot_row[xt] = block[sub_pixel_size][xtt];

	  /* Swap first and last columns */

	  for (yt = 0; yt < (sub_pixel_size + 1); yt++)
	    {
	      tmp_sample   		= block[yt][0];
	      block[yt][0] 		= block[yt][sub_pixel_size];
	      block[yt][sub_pixel_size] = tmp_sample;
	    }
	}

      /* Swap rows */

      tmp_row = top_row;
      top_row = bot_row;
      bot_row = tmp_row;

      /* Call progress display function */

      if (progress_func && !(y & 0xf))
	(* progress_func) (y1, y2, y, progress_data);
    }

  /* Free memory */

  for (y = 0; y < (sub_pixel_size + 1); y++)
    g_free (block[y]);

  g_free (block);
  g_free (top_row);
  g_free (bot_row);

  return num_samples;
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
		       GimpRGB         *color,
		       GimpRenderFunc   render_func,
		       gpointer         render_data)
{
  gint     x2, y2;         /* Coords of center sample */
  gdouble  dx1, dy1;       /* Delta to upper left sample */
  gdouble  dx3, dy3;       /* Delta to lower right sample */
  GimpRGB  c1, c2, c3, c4; /* Sample colors */
  gulong   num_samples;

  /* Get offsets for corners */

  dx1 = (gdouble) (x1 - sub_pixel_size / 2) / sub_pixel_size;
  dx3 = (gdouble) (x3 - sub_pixel_size / 2) / sub_pixel_size;

  dy1 = (gdouble) (y1 - sub_pixel_size / 2) / sub_pixel_size;
  dy3 = (gdouble) (y3 - sub_pixel_size / 2) / sub_pixel_size;

  num_samples = 0;

  /* Render upper left sample */

  if (!block[y1][x1].ready)
    {
      num_samples++;

      if (render_func)
	(* render_func) (x + dx1, y + dy1, &c1, render_data);

      block[y1][x1].ready = TRUE;
      block[y1][x1].color = c1;
    }
  else
    {
      c1 = block[y1][x1].color;
    }

  /* Render upper right sample */

  if (! block[y1][x3].ready)
    {
      num_samples++;

      if (render_func)
	(* render_func) (x + dx3, y + dy1, &c2, render_data);

      block[y1][x3].ready = TRUE;
      block[y1][x3].color = c2;
    }
  else
    {
      c2 = block[y1][x3].color;
    }

  /* Render lower left sample */

  if (! block[y3][x1].ready)
    {
      num_samples++;

      if (render_func)
	(* render_func) (x + dx1, y + dy3, &c3, render_data);

      block[y3][x1].ready = TRUE;
      block[y3][x1].color = c3;
    }
  else
    {
      c3 = block[y3][x1].color;
    }

  /* Render lower right sample */

  if (! block[y3][x3].ready)
    {
      num_samples++;

      if (render_func)
	(* render_func) (x + dx3, y + dy3, &c4, render_data);

      block[y3][x3].ready = TRUE;
      block[y3][x3].color = c4;
    }
  else
    {
      c4 = block[y3][x3].color;
    }

  /* Check for supersampling */

  if (depth <= max_depth)
    {
      /* Check whether we have tu supersample */

      if ((gimp_rgba_distance (&c1, &c2) >= threshold) ||
	  (gimp_rgba_distance (&c1, &c3) >= threshold) ||
	  (gimp_rgba_distance (&c1, &c4) >= threshold) ||
	  (gimp_rgba_distance (&c2, &c3) >= threshold) ||
	  (gimp_rgba_distance (&c2, &c4) >= threshold) ||
	  (gimp_rgba_distance (&c3, &c4) >= threshold))
	{
	  /* Calc coordinates of center subsample */

	  x2 = (x1 + x3) / 2;
	  y2 = (y1 + y3) / 2;

	  /* Render sub-blocks */

	  num_samples += gimp_render_sub_pixel (max_depth, depth + 1, block,
						x, y, x1, y1, x2, y2,
						threshold, sub_pixel_size,
						&c1,
						render_func, render_data);

	  num_samples += gimp_render_sub_pixel (max_depth, depth + 1, block,
						x, y, x2, y1, x3, y2,
						threshold, sub_pixel_size,
						&c2,
						render_func, render_data);

	  num_samples += gimp_render_sub_pixel (max_depth, depth + 1, block,
						x, y, x1, y2, x2, y3,
						threshold, sub_pixel_size,
						&c3,
						render_func, render_data);

	  num_samples += gimp_render_sub_pixel (max_depth, depth + 1, block,
						x, y, x2, y2, x3, y3,
						threshold, sub_pixel_size,
						&c4,
						render_func, render_data);
	}
    }

  color->r = 0.25 * (c1.r + c2.r + c3.r + c4.r);
  color->g = 0.25 * (c1.g + c2.g + c3.g + c4.g);
  color->b = 0.25 * (c1.b + c2.b + c3.b + c4.b);
  color->a = 0.25 * (c1.a + c2.a + c3.a + c4.a);

  return num_samples;
}
