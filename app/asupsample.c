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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


/* This code is *largely* based on the sources for POV-Ray 3.0.  I am
 * grateful to the POV-Team for such a great program and for making
 * their sources available.  All comments / bug reports /
 * etc. regarding this library should be addressed to me, not to the
 * POV-Ray team.  Any bugs are my responsibility, not theirs.
 */


#include <math.h>

#include "appenv.h"
#include "asupsample.h"


/***** Types *****/

typedef struct {
	char    ready;
	color_t color;
} sample_t;


/***** Local functions *****/

static unsigned long render_sub_pixel(int max_depth, int depth, sample_t **block,
				      int x, int y, int x1, int y1, int x3, int y3,
				      double threshold, int sub_pixel_size, render_func_t render_func,
				      color_t *color, void *render_data);

static double color_dist(color_t c1, color_t c2);


/***** Functions *****/

/*****/

unsigned long
adaptive_supersample_area(int x1, int y1, int x2, int y2, int max_depth, double threshold,
			  render_func_t render_func, void *render_data,
			  put_pixel_func_t put_pixel_func, void *put_pixel_data,
			  progress_func_t progress_func, void *progress_data)
{
	int          	x, y, width;                 /* Counters, width of region */
	int          	xt, xtt, yt;                 /* Temporary counters */
	int          	sub_pixel_size;              /* Numbe of samples per pixel (1D) */
	size_t       	row_size;                    /* Memory needed for one row */
 	color_t      	color;                       /* Rendered pixel's color */
	sample_t     	tmp_sample;                  /* For swapping samples */
	sample_t       *top_row, *bot_row, *tmp_row; /* Sample rows */
	sample_t      **block;                       /* Sample block matrix */
	unsigned long   num_samples;

	/* Initialize color */

	color.r = 0.0;
	color.g = 0.0;
	color.b = 0.0;
	color.a = 0.0;

	/* Calculate sub-pixel size */

	sub_pixel_size = 1 << max_depth; /* 2**max_depth */

	/* Create row arrays */

	width = x2 - x1 + 1;

	row_size = (sub_pixel_size * width + 1) * sizeof(sample_t);

	top_row = g_malloc(row_size);
	bot_row = g_malloc(row_size);

	for (x = 0; x < (sub_pixel_size * width + 1); x++) {
		top_row[x].ready   = 0;
		
		top_row[x].color.r = 0.0;
		top_row[x].color.g = 0.0;
		top_row[x].color.b = 0.0;
		top_row[x].color.a = 0.0;

		bot_row[x].ready   = 0;
		
		bot_row[x].color.r = 0.0;
		bot_row[x].color.g = 0.0;
		bot_row[x].color.b = 0.0;
		bot_row[x].color.a = 0.0;
	} /* for */

	/* Allocate block matrix */

	block = g_malloc((sub_pixel_size + 1) * sizeof(sample_t *)); /* Rows */

	for (y = 0; y < (sub_pixel_size + 1); y++)
		block[y] = g_malloc((sub_pixel_size + 1) * sizeof(sample_t)); /* Columns */

	for (y = 0; y < (sub_pixel_size + 1); y++)
		for (x = 0; x < (sub_pixel_size + 1); x++) {
			block[y][x].ready = 0;

			block[y][x].color.r = 0.0;
			block[y][x].color.g = 0.0;
			block[y][x].color.b = 0.0;
			block[y][x].color.a = 0.0;
		} /* for */

	/* Render region */

	num_samples = 0;

	for (y = y1; y <= y2; y++) {
		/* Clear the bottom row */

		for (xt = 0; xt < (sub_pixel_size * width + 1); xt++)
			bot_row[xt].ready = 0;

		/* Clear first column */

		for (yt = 0; yt < (sub_pixel_size + 1); yt++)
			block[yt][0].ready = 0;

		/* Render row */

		for (x = x1; x <= x2; x++) {
			/* Initialize block by clearing all but first row/column */

			for (yt = 1; yt < (sub_pixel_size + 1); yt++)
				for (xt = 1; xt < (sub_pixel_size + 1); xt++)
					block[yt][xt].ready = 0;

			/* Copy samples from top row to block */

			for (xtt = 0, xt = (x - x1) * sub_pixel_size;
			     xtt < (sub_pixel_size + 1);
			     xtt++, xt++)
				block[0][xtt] = top_row[xt];

			/* Render pixel on (x, y) */

			num_samples += render_sub_pixel(max_depth, 1, block, x, y, 0, 0,
							sub_pixel_size, sub_pixel_size,
							threshold, sub_pixel_size, render_func, &color,
							render_data);

			if (put_pixel_func)
				(*put_pixel_func)(x, y, color, put_pixel_data);

			/* Copy block information to rows */

			top_row[((x - x1) + 1) * sub_pixel_size] = block[0][sub_pixel_size];

			for (xtt = 0, xt = (x - x1) * sub_pixel_size;
			     xtt < (sub_pixel_size + 1);
			     xtt++, xt++)
				bot_row[xt] = block[sub_pixel_size][xtt];

			/* Swap first and last columns */

			for (yt = 0; yt < (sub_pixel_size + 1); yt++) {
				tmp_sample   		  = block[yt][0];
				block[yt][0] 		  = block[yt][sub_pixel_size];
				block[yt][sub_pixel_size] = tmp_sample;
			} /* for */
		} /* for */

		/* Swap rows */

		tmp_row = top_row;
		top_row = bot_row;
		bot_row = tmp_row;

		/* Call progress display function */

		if (progress_func)
			(*progress_func)(y1, y2, y, progress_data);
	} /* for */

	/* Free memory */

	for (y = 0; y < (sub_pixel_size + 1); y++)
		g_free(block[y]);

	g_free(block);
	g_free(top_row);
	g_free(bot_row);

	return num_samples;
} /* adaptive_supersample_area */


/*****/

static unsigned long
render_sub_pixel(int max_depth, int depth, sample_t **block,
		 int x, int y, int x1, int y1, int x3, int y3,
		 double threshold, int sub_pixel_size, render_func_t render_func,
		 color_t *color, void *render_data)
{
	int           x2, y2;   /* Coords of center sample */
	double        dx1, dy1; /* Delta to upper left sample */
	double        dx3, dy3; /* Delta to lower right sample */
	color_t       c1, c2, c3, c4; /* Sample colors */
	unsigned long num_samples;

	/* Get offsets for corners */

	dx1 = (double) (x1 - sub_pixel_size / 2) / sub_pixel_size;
	dx3 = (double) (x3 - sub_pixel_size / 2) / sub_pixel_size;

	dy1 = (double) (y1 - sub_pixel_size / 2) / sub_pixel_size;
	dy3 = (double) (y3 - sub_pixel_size / 2) / sub_pixel_size;

	num_samples = 0;

	/* Render upper left sample */

	if (!block[y1][x1].ready) {
		num_samples++;
		(*render_func)(x + dx1, y + dy1, &c1, render_data);

		block[y1][x1].ready = 1;
		block[y1][x1].color = c1;
	} else
		c1 = block[y1][x1].color;

	/* Render upper right sample */

	if (!block[y1][x3].ready) {
		num_samples++;
		(*render_func)(x + dx3, y + dy1, &c2, render_data);

		block[y1][x3].ready = 1;
		block[y1][x3].color = c2;
	} else
		c2 = block[y1][x3].color;

	/* Render lower left sample */

	if (!block[y3][x1].ready) {
		num_samples++;
		(*render_func)(x + dx1, y + dy3, &c3, render_data);

		block[y3][x1].ready = 1;
		block[y3][x1].color = c3;
	} else
		c3 = block[y3][x1].color;

	/* Render lower right sample */

	if (!block[y3][x3].ready) {
		num_samples++;
		(*render_func)(x + dx3, y + dy3, &c4, render_data);

		block[y3][x3].ready = 1;
		block[y3][x3].color = c4;
	} else
		c4 = block[y3][x3].color;

	/* Check for supersampling */

	if (depth <= max_depth) {
		/* Check whether we have tu supersample */

		if ((color_dist(c1, c2) >= threshold) ||
		    (color_dist(c1, c3) >= threshold) ||
		    (color_dist(c1, c4) >= threshold) ||
		    (color_dist(c2, c3) >= threshold) ||
		    (color_dist(c2, c4) >= threshold) ||
		    (color_dist(c3, c4) >= threshold)) {
			/* Calc coordinates of center subsample */

			x2 = (x1 + x3) / 2;
			y2 = (y1 + y3) / 2;

			/* Render sub-blocks */

			num_samples += render_sub_pixel(max_depth, depth + 1, block, x, y, x1, y1, x2, y2,
							threshold, sub_pixel_size, render_func, &c1,
							render_data);

			num_samples += render_sub_pixel(max_depth, depth + 1, block, x, y, x2, y1, x3, y2,
							threshold, sub_pixel_size, render_func, &c2,
							render_data);

			num_samples += render_sub_pixel(max_depth, depth + 1, block, x, y, x1, y2, x2, y3,
							threshold, sub_pixel_size, render_func, &c3,
							render_data);

			num_samples += render_sub_pixel(max_depth, depth + 1, block, x, y, x2, y2, x3, y3,
							threshold, sub_pixel_size, render_func, &c4,
							render_data);
		} /* if */
	} /* if */

	color->r = 0.25 * (c1.r + c2.r + c3.r + c4.r);
	color->g = 0.25 * (c1.g + c2.g + c3.g + c4.g);
	color->b = 0.25 * (c1.b + c2.b + c3.b + c4.b);
	color->a = 0.25 * (c1.a + c2.a + c3.a + c4.a);

	return num_samples;
} /* render_sub_pixel */


/*****/

static double
color_dist(color_t c1, color_t c2)
{
	return fabs(c1.r - c2.r) +
	       fabs(c1.g - c2.g) +
	       fabs(c1.b - c2.b) +
	       fabs(c1.a - c2.a);
} /* color_dist */
