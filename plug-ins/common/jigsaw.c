/*
 * jigsaw - a plug-in for the GIMP
 *
 * Copyright (C) Nigel Wetten
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
 * Contact info: nigel@cs.nwu.edu
 *
 * Version: 1.0.0
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"

typedef enum {BEZIER_1, BEZIER_2} style_t;
typedef enum {LEFT, RIGHT, UP, DOWN} bump_t;


static void query(void);
static void run(gchar *name, gint nparams, GParam *param,
		gint *nreturn_vals, GParam **return_vals);
static gint jigsaw(void);
static void run_callback(GtkWidget *widget, gpointer data);
static void dialog_box(void);
static void entry_callback(GtkWidget *widget, gpointer data);
static void entry_double_callback(GtkWidget *widget, gpointer data);
static void radio_button_primitive_callback(GtkWidget *widget, gpointer data);
static void adjustment_callback(GtkAdjustment *adjustment, gpointer data);
static void adjustment_double_callback(GtkAdjustment *adjustment,
				       gpointer data);
static void check_button_callback(GtkWidget *widget, gpointer data);

static void draw_jigsaw(guchar *buffer, gint width, gint height, gint bytes);
static void draw_vertical_border(guchar *buffer, gint width, gint height,
				 gint bytes, gint x_offset, gint ytiles,
				 gint blend_lines, gdouble blend_amount);
static void draw_horizontal_border(guchar *buffer, gint width,
				   gint bytes, gint y_offset, gint xtiles,
				   gint blend_lines, gdouble blend_amount);
static void draw_vertical_line(guchar *buffer, gint width, gint bytes,
			       gint px[2], gint py[2]);
static void draw_horizontal_line(guchar *buffer, gint width, gint bytes,
				 gint px[2], gint py[2]);
static void darken_vertical_line(guchar *buffer, gint width, gint bytes,
				 gint *px, gint *py, gdouble delta);
static void lighten_vertical_line(guchar *buffer, gint width, gint bytes,
				  gint *px, gint *py, gdouble delta);
static void darken_horizontal_line(guchar *buffer, gint width, gint bytes,
				   gint *px, gint *py, gdouble delta);
static void lighten_horizontal_line(guchar *buffer, gint width, gint bytes,
				    gint *px, gint *py, gdouble delta);
static void draw_right_bump(guchar *buffer, gint width, gint bytes,
			    gint x_offset, gint curve_start_offset,
			    gint steps);
static void draw_left_bump(guchar *buffer, gint width, gint bytes,
			   gint x_offset, gint curve_start_offset,
			   gint steps);
static void draw_up_bump(guchar *buffer, gint width, gint bytes,
			 gint y_offset, gint curve_start_offset,
			 gint steps);
static void draw_down_bump(guchar *buffer, gint width, gint bytes,
			   gint y_offset, gint curve_start_offset,
			   gint steps);
static void darken_right_bump(guchar *buffer, gint width, gint bytes,
			      gint x_offset, gint curve_start_offset,
			      gint steps, gdouble delta, gint counter);
static void lighten_right_bump(guchar *buffer, gint width, gint bytes,
			       gint x_offset, gint curve_start_offset,
			       gint steps, gdouble delta, gint counter);
static void darken_left_bump(guchar *buffer, gint width, gint bytes,
			     gint x_offset, gint curve_start_offset,
			     gint steps, gdouble delta, gint counter);
static void lighten_left_bump(guchar *buffer, gint width, gint bytes,
			      gint x_offset, gint curve_start_offset,
			      gint steps, gdouble delta, gint counter);
static void darken_up_bump(guchar *buffer, gint width, gint bytes,
			   gint y_offset, gint curve_start_offest,
			   gint steps, gdouble delta, gint counter);
static void lighten_up_bump(guchar *buffer, gint width, gint bytes,
			    gint y_offset, gint curve_start_offset,
			    gint steps, gdouble delta, gint counter);
static void darken_down_bump(guchar *buffer, gint width, gint bytes,
			     gint y_offset, gint curve_start_offset,
			     gint steps, gdouble delta, gint counter);
static void lighten_down_bump(guchar *buffer, gint width, gint bytes,
			      gint y_offset, gint curve_start_offset,
			      gint steps, gdouble delta, gint counter);
static void generate_grid(gint width, gint height, gint xtiles, gint ytiles,
			  gint *x, gint *y);
static void generate_bezier(gint px[4], gint py[4], gint steps,
			    gint *cachex, gint *cachey);
static void malloc_cache(void);
static void free_cache(void);
static void init_right_bump(gint width, gint height);
static void init_left_bump(gint width, gint height);
static void init_up_bump(gint width, gint height);
static void init_down_bump(gint width, gint height);
static void draw_bezier_line(guchar *buffer, gint width, gint bytes,
			     gint steps, gint *cx, gint *cy);
static void darken_bezier_line(guchar *buffer, gint width, gint bytes,
			       gint x_offset, gint y_offset, gint steps,
			       gint *cx, gint *cy, gdouble delta);
static void lighten_bezier_line(guchar *buffer, gint width, gint bytes,
				gint x_offset, gint y_offset, gint steps,
				gint *cx, gint *cy, gdouble delta);
static void draw_bezier_vertical_border(guchar *buffer, gint width,
					gint height, gint bytes,
					gint x_offset, gint xtiles,
					gint ytiles, gint blend_lines,
					gdouble blend_amount, gint steps);
static void draw_bezier_horizontal_border(guchar *buffer, gint width,
					  gint height, gint bytes,
					  gint x_offset, gint xtiles,
					  gint ytiles, gint blend_lines,
					  gdouble blend_amount, gint steps);
static void check_config(gint width, gint height);

			    

#define PLUG_IN_NAME "jigsaw"
#define PLUG_IN_STORAGE "jigsaw-storage"

#define XFACTOR2 0.0833
#define XFACTOR3 0.2083
#define XFACTOR4 0.2500

#define XFACTOR5 0.2500
#define XFACTOR6 0.2083
#define XFACTOR7 0.0833

#define YFACTOR2 0.1000
#define YFACTOR3 0.2200
#define YFACTOR4 0.1000

#define YFACTOR5 0.1000
#define YFACTOR6 0.4666
#define YFACTOR7 0.1000
#define YFACTOR8 0.2000

#define MAX_VALUE 255
#define MIN_VALUE 0
#define DELTA 0.15

#define BLACK_R 30
#define BLACK_G 30
#define BLACK_B 30

#define WALL_XFACTOR2 0.05
#define WALL_XFACTOR3 0.05
#define WALL_YFACTOR2 0.05
#define WALL_YFACTOR3 0.05

#define WALL_XCONS2 0.2
#define WALL_XCONS3 0.3
#define WALL_YCONS2 0.2
#define WALL_YCONS3 0.3

#define FUDGE 1.2

#define MIN_XTILES 1
#define MAX_XTILES 20
#define MIN_YTILES 1
#define MAX_YTILES 20
#define MIN_BLEND_LINES 0
#define MAX_BLEND_LINES 10
#define MIN_BLEND_AMOUNT 0
#define MAX_BLEND_AMOUNT 1.0

#define SCALE_WIDTH 200
#define ENTRY_WIDTH 40

#define DRAW_POINT(buffer, index)		\
do {						\
  buffer[index] = BLACK_R;			\
  buffer[index+1] = BLACK_G;			\
  buffer[index+2] = BLACK_B;			\
} while (0)

#define DARKEN_POINT(buffer, index, delta, temp)	\
do {							\
  temp = buffer[index];					\
  temp -= buffer[index] * delta;			\
  if (temp < MIN_VALUE) temp = MIN_VALUE;		\
  buffer[index] = temp;					\
  temp = buffer[index+1];				\
  temp -= buffer[index+1] * delta;			\
  if (temp < MIN_VALUE) temp = MIN_VALUE;		\
  buffer[index+1] = temp;				\
  temp = buffer[index+2];				\
  temp -= buffer[index+2] * delta;			\
  if (temp < MIN_VALUE) temp = MIN_VALUE;		\
  buffer[index+2] = temp;				\
} while (0)

#define LIGHTEN_POINT(buffer, index, delta, temp)	\
do {							\
  temp = buffer[index] * delta;				\
  temp += buffer[index];				\
  if (temp > MAX_VALUE)	temp = MAX_VALUE;		\
  buffer[index] = temp;					\
  temp = buffer[index+1] * delta;			\
  temp += buffer[index+1];				\
  if (temp > MAX_VALUE) temp = MAX_VALUE;		\
  buffer[index+1] = temp;				\
  temp = buffer[index+2] * delta;			\
  temp += buffer[index+2];				\
  if (temp > MAX_VALUE) temp = MAX_VALUE;		\
  buffer[index+2] = temp;				\
} while (0)


GPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};

struct config_tag
{
  gint x;
  gint y;
  style_t style;
  gint blend_lines;
  gdouble blend_amount;
};


typedef struct config_tag config_t;

static config_t config =
{
  5,
  5,
  BEZIER_1,
  3,
  0.5
};

struct globals_tag
{
  GDrawable *drawable;
  gint *cachex1[4];
  gint *cachex2[4];
  gint *cachey1[4];
  gint *cachey2[4];
  gint steps[4];
  gint *gridx;
  gint *gridy;
  gint **blend_outer_cachex1[4];
  gint **blend_outer_cachex2[4];
  gint **blend_outer_cachey1[4];
  gint **blend_outer_cachey2[4];
  gint **blend_inner_cachex1[4];
  gint **blend_inner_cachex2[4];
  gint **blend_inner_cachey1[4];
  gint **blend_inner_cachey2[4];
  gint dialog_result;
  gint tooltips;
};

typedef struct globals_tag globals_t;

static globals_t globals =
{
  0,
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  0,
  0,
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  -1,
  1
};

MAIN()

static void
query(void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, Non-interactive, Last-Vals"},
    { PARAM_IMAGE, "image", "Input image"},
    { PARAM_DRAWABLE, "drawable", "Input drawable"},
    { PARAM_INT32, "x", "Number of tiles across > 0"},
    { PARAM_INT32, "y", "Number of tiles down > 0"},
    { PARAM_INT32, "style", "The style/shape of the jigsaw puzzle, 0 or 1"},
    { PARAM_INT32, "blend_lines", "Number of lines for shading bevels >= 0"},
    { PARAM_FLOAT, "blend_amount", "The power of the light highlights 0 =< 5"}
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof(args) / sizeof(args[0]);
  static gint nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure("plug_in_jigsaw",
			 _("Renders a jigsaw puzzle look"),
			 _("Jigsaw puzzle look"),
			 "Nigel Wetten",
			 "Nigel Wetten",
			 "1998",
			 N_("<Image>/Filters/Render/Pattern/Jigsaw..."),
			 "RGB*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);

  return;
}  /* query */

/*****/

static void
run(gchar *name, gint nparams, GParam *param, gint *nreturn_vals,
    GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get(param[2].data.d_drawable);
  globals.drawable = drawable;
  gimp_tile_cache_ntiles(drawable->width / gimp_tile_width() + 1);
  switch (run_mode)
    {
    case RUN_NONINTERACTIVE:
      INIT_I18N();
      if (nparams == 8)
	{
	  config.x = param[3].data.d_int32;
	  config.y = param[4].data.d_int32;
	  config.style = param[5].data.d_int32;
	  config.blend_lines = param[6].data.d_int32;
	  config.blend_amount = param[7].data.d_float;
	  if (jigsaw() == -1)
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }
	}
      else
	{
	  status = STATUS_CALLING_ERROR;
	}
      break;
      
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data("plug_in_jigsaw", &config);
      gimp_get_data(PLUG_IN_STORAGE, &globals.tooltips);
      dialog_box();
      if (globals.dialog_result == -1)
	{
	  status = STATUS_EXECUTION_ERROR;
	  break;
	}
      gimp_progress_init( _("Assembling Jigsaw"));
      if (jigsaw() == -1)
	{
	  status = STATUS_CALLING_ERROR;
	  break;
	}
      gimp_set_data("plug_in_jigsaw", &config, sizeof(config_t));
      gimp_set_data(PLUG_IN_STORAGE, &globals.tooltips,
		    sizeof(globals.tooltips));
      gimp_displays_flush();
      break;
      
    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data("plug_in_jigsaw", &config);
      if (jigsaw() == -1)
	{
	  status = STATUS_EXECUTION_ERROR;
	  gimp_message("An execution error occured.");
	}
      else
	{
	  gimp_displays_flush();
	}

    }  /* switch */
      
  gimp_drawable_detach(drawable);
  
  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  return;
}  /* run */

/*****/

static gint
jigsaw(void)
{
  GPixelRgn src_pr, dest_pr;
  guchar *buffer;
  GDrawable *drawable = globals.drawable;
  gint width = drawable->width;
  gint height = drawable->height;
  gint bytes = drawable->bpp;
  gint buffer_size = bytes * width * height;

  srand((gint)NULL);
  

  /* setup image buffer */
  gimp_pixel_rgn_init(&src_pr, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init(&dest_pr, drawable, 0, 0, width, height, TRUE, TRUE);
  buffer = g_malloc(buffer_size);
  gimp_pixel_rgn_get_rect(&src_pr, buffer, 0, 0, width, height);

  check_config(width, height);
  globals.steps[LEFT] = globals.steps[RIGHT] = globals.steps[UP]
    = globals.steps[DOWN] = (config.x < config.y) ?
    (width / config.x * 2) : (height / config.y * 2);

  malloc_cache();
  draw_jigsaw(buffer, width, height, bytes);
  free_cache();
  /* cleanup */
  gimp_pixel_rgn_set_rect(&dest_pr, buffer, 0, 0, width, height);
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, 0, 0, width, height);
  g_free(buffer);

  return 0;
}  /* jigsaw */

/*****/

static void
generate_bezier(gint px[4], gint py[4], gint steps,
		gint *cachex, gint *cachey)
{
  gdouble t = 0.0;
  gdouble sigma = 1.0 / steps;
  gint i;

  for (i = 0; i < steps; i++)
    {
      gdouble t2, t3, x, y, t_1;

      t += sigma;
      t2 = t * t;
      t3 = t2 * t;
      t_1 = 1 - t;
      x = t_1 * t_1 * t_1 * px[0]
	+ 3 * t * t_1 * t_1 * px[1]
	+ 3 * t2 * t_1 * px[2]
	+ t3 * px[3];
      y = t_1 * t_1 * t_1 * py[0]
	+ 3 * t * t_1 * t_1 * py[1]
	+ 3 * t2 * t_1 * py[2]
	+ t3 * py[3];
      cachex[i] = (gint) (x + 0.2);
      cachey[i] = (gint) (y + 0.2);
    }  /* for */
  return;
}  /* generate_bezier */

/*****/

static void
draw_jigsaw(guchar *buffer, gint width, gint height, gint bytes)
{
  gint i;
  gint *x, *y;
  gint xtiles = config.x;
  gint ytiles = config.y;
  gint xlines = xtiles - 1;
  gint ylines = ytiles - 1;
  gint blend_lines = config.blend_lines;
  gdouble blend_amount = config.blend_amount;
  gint steps = globals.steps[RIGHT];
  style_t style = config.style;
  gint progress_total = xlines + ylines - 1;

  globals.gridx = g_malloc(sizeof(gint) * xtiles);
  globals.gridy = g_malloc(sizeof(gint) * ytiles);
  x = globals.gridx;
  y = globals.gridy;

  generate_grid(width, height, xtiles, ytiles, globals.gridx, globals.gridy);

  init_right_bump(width, height);
  init_left_bump(width, height);
  init_up_bump(width, height);
  init_down_bump(width, height);

  if (style == BEZIER_1)
    {
      for (i = 0; i < xlines; i++)
	{
	  draw_vertical_border(buffer, width, height, bytes, x[i], ytiles,
			       blend_lines, blend_amount);
	  gimp_progress_update((gdouble) i / (gdouble) progress_total);
	}
      for (i = 0; i < ylines; i++)
	{
	  draw_horizontal_border(buffer, width, bytes, y[i], xtiles,
				 blend_lines, blend_amount);
	  gimp_progress_update((gdouble) (i + xlines)
				/ (gdouble) progress_total);
	}
    }
  else if (style == BEZIER_2)
    {
      for (i = 0; i < xlines; i++)
	{
	  draw_bezier_vertical_border(buffer, width, height, bytes, x[i],
				      xtiles, ytiles, blend_lines,
				      blend_amount, steps);
	  gimp_progress_update((gdouble) i / (gdouble) progress_total);
	}
      for (i = 0; i < ylines; i++)
	{
	  draw_bezier_horizontal_border(buffer, width, height, bytes, y[i],
					xtiles, ytiles, blend_lines,
					blend_amount, steps);
	  gimp_progress_update((gdouble) (i + xlines)
			       / (gdouble) progress_total);
	}
    }
  else
    {
      printf("draw_jigsaw: bad style\n");
      exit(1);
    }
  g_free(globals.gridx);
  g_free(globals.gridy);
  
  return;
}  /* draw_jigsaw */

/*****/

static void
draw_vertical_border(guchar *buffer, gint width, gint height, gint bytes,
		     gint x_offset, gint ytiles, gint blend_lines,
		     gdouble blend_amount)
{
  gint i, j;
  gint tile_height = height / ytiles;
  gint tile_height_eighth = tile_height / 8;
  gint curve_start_offset = 3 * tile_height_eighth;
  gint curve_end_offset = curve_start_offset + 2 * tile_height_eighth;
  gint px[2], py[2];
  gint ly[2], dy[2];
  gint y_offset = 0;
  gdouble delta;
  gdouble sigma = blend_amount / blend_lines;
  gint right;
  bump_t style_index;

  for (i = 0; i < ytiles; i++)
    {
      right = rand() & 1;
      if (right)
	{
	  style_index = RIGHT;
	}
      else
	{
	  style_index = LEFT;
	}
      
      /* first straight line from top downwards */
      px[0] = px[1] = x_offset;
      py[0] = y_offset; py[1] = y_offset + curve_start_offset - 1;
      draw_vertical_line(buffer, width, bytes, px, py);
      delta = blend_amount;
      dy[0] = ly[0] = py[0]; dy[1] = ly[1] = py[1];
      if (!right)
	{
	  ly[1] += blend_lines + 2;
	}
      for (j = 0; j < blend_lines; j++)
	{
	  dy[0]++; dy[1]--; ly[0]++; ly[1]--;
	  px[0] = x_offset - j - 1;
	  darken_vertical_line(buffer, width, bytes, px, dy, delta);
	  px[0] = x_offset + j + 1;
	  lighten_vertical_line(buffer, width, bytes, px, ly, delta);
	  delta -= sigma;
	}

      /* top half of curve */
      if (right)
	{
	  draw_right_bump(buffer, width, bytes, x_offset,
			  y_offset + curve_start_offset,
			  globals.steps[RIGHT]);
	  delta = blend_amount;
	  for (j = 0; j < blend_lines; j++)
	    {
	      /* use to be -j -1 */
	      darken_right_bump(buffer, width, bytes, x_offset,
				y_offset + curve_start_offset,
				globals.steps[RIGHT], delta, j);
	      /* use to be +j + 1 */
	      lighten_right_bump(buffer, width, bytes, x_offset,
				 y_offset + curve_start_offset,
				 globals.steps[RIGHT], delta, j);
	      delta -= sigma;
	    }
	}
      else
	{
	  draw_left_bump(buffer, width, bytes, x_offset,
			 y_offset + curve_start_offset,
			 globals.steps[LEFT]);
	  delta = blend_amount;
	  for (j = 0; j < blend_lines; j++)
	    {
	      /* use to be -j -1 */
	      darken_left_bump(buffer, width, bytes, x_offset,
			       y_offset + curve_start_offset,
			       globals.steps[LEFT], delta, j);
	      /* use to be -j - 1 */
	      lighten_left_bump(buffer, width, bytes, x_offset,
				y_offset + curve_start_offset,
				globals.steps[LEFT], delta, j);
	      delta -= sigma;
	    }
	}
      /* bottom straight line of a tile wall */
      px[0] = px[1] = x_offset;
      py[0] = y_offset + curve_end_offset;
      py[1] = globals.gridy[i];
      draw_vertical_line(buffer, width, bytes, px, py);
      delta = blend_amount;
      dy[0] = ly[0] = py[0]; dy[1] = ly[1] = py[1];
      if (right)
	{
	  dy[0] -= blend_lines + 2;
	}
      for (j = 0; j < blend_lines; j++)
	{
	  dy[0]++; dy[1]--; ly[0]++; ly[1]--;
	  px[0] = x_offset - j - 1;
	  darken_vertical_line(buffer, width, bytes, px, dy, delta);
	  px[0] = x_offset + j + 1;
	  lighten_vertical_line(buffer, width, bytes, px, ly, delta);
	  delta -= sigma;
	}

      y_offset = globals.gridy[i];
    }  /* for */
  
  return;
}  /* draw_vertical_border */

/*****/

/* assumes RGB* */
static void
draw_horizontal_border(guchar *buffer, gint width, gint bytes,
		       gint y_offset, gint xtiles, gint blend_lines,
		       gdouble blend_amount)
{
  gint i, j;
  gint tile_width = width / xtiles;
  gint tile_width_eighth = tile_width / 8;
  gint curve_start_offset = 3 * tile_width_eighth;
  gint curve_end_offset = curve_start_offset + 2 * tile_width_eighth;
  gint px[2], py[2];
  gint dx[2], lx[2];
  gint x_offset = 0;
  gdouble delta;
  gdouble sigma = blend_amount / blend_lines;
  gint up;

  for (i = 0; i < xtiles; i++)
    {
      up = rand() & 1;
      /* first horizontal line across */
      px[0] = x_offset; px[1] = x_offset + curve_start_offset - 1;
      py[0] = py[1] = y_offset;
      draw_horizontal_line(buffer, width, bytes, px, py);
      delta = blend_amount;
      dx[0] = lx[0] = px[0]; dx[1] = lx[1] = px[1];
      if (up)
	{
	  lx[1] += blend_lines + 2;
	}
      for (j = 0; j < blend_lines; j++)
	{
	  dx[0]++; dx[1]--; lx[0]++; lx[1]--;
	  py[0] = y_offset - j - 1;
	  darken_horizontal_line(buffer, width, bytes, dx, py, delta);
	  py[0] = y_offset + j + 1;
	  lighten_horizontal_line(buffer, width, bytes, lx, py, delta);
	  delta -= sigma;
	}

      if (up)
	{
	  draw_up_bump(buffer, width, bytes, y_offset,
		       x_offset + curve_start_offset,
		       globals.steps[UP]);
	  delta = blend_amount;
	  for (j = 0; j < blend_lines; j++)
	    {
	      /* use to be -j -1 */
	      darken_up_bump(buffer, width, bytes, y_offset,
			     x_offset + curve_start_offset,
			     globals.steps[UP], delta, j);
	      /* use to be +j + 1 */
	      lighten_up_bump(buffer, width, bytes, y_offset,
			      x_offset + curve_start_offset,
			      globals.steps[UP], delta, j);
	      delta -= sigma;
	    }
	}
      else
	{
	  draw_down_bump(buffer, width, bytes, y_offset,
			 x_offset + curve_start_offset,
			 globals.steps[DOWN]);
	  delta = blend_amount;
	  for (j = 0; j < blend_lines; j++)
	    {
	      /* use to be +j + 1 */
	      darken_down_bump(buffer, width, bytes, y_offset,
			       x_offset + curve_start_offset,
			       globals.steps[DOWN], delta, j);
	      /* use to be -j -1 */
	      lighten_down_bump(buffer, width, bytes, y_offset,
				x_offset + curve_start_offset,
				globals.steps[DOWN], delta, j);
	      delta -= sigma;
	    }
	}
      /* right horizontal line of tile */
      px[0] = x_offset + curve_end_offset;
      px[1] = globals.gridx[i];
      py[0] = py[1] = y_offset;
      draw_horizontal_line(buffer, width, bytes, px, py);
      delta = blend_amount;
      dx[0] = lx[0] = px[0]; dx[1] = lx[1] = px[1];
      if (!up)
	{
	  dx[0] -= blend_lines + 2;
	}
      for (j = 0;j < blend_lines; j++)
	{
	  dx[0]++; dx[1]--; lx[0]++; lx[1]--;
	  py[0] = y_offset - j - 1;
	  darken_horizontal_line(buffer, width, bytes, dx, py, delta);
	  py[0] = y_offset + j + 1;
	  lighten_horizontal_line(buffer, width, bytes, lx, py, delta);
	  delta -= sigma;
	}
      x_offset = globals.gridx[i];
    }  /* for */

    return;
}  /* draw_horizontal_border */

/*****/

/* assumes going top to bottom */
static void
draw_vertical_line(guchar *buffer, gint width, gint bytes,
		   gint px[2], gint py[2])
{
  gint i;
  gint rowstride = bytes * width;
  gint index = px[0] * bytes + rowstride * py[0];
  gint length = py[1] - py[0] + 1;
  
  for (i = 0; i < length; i++)
    {
      DRAW_POINT(buffer, index);
      index += rowstride;
    }
  return;
}  /* draw_vertical_line */

/*****/

/* assumes going left to right */
static void
draw_horizontal_line(guchar *buffer, gint width, gint bytes,
		     gint px[2], gint py[2])
{
  gint i;
  gint rowstride = bytes * width;
  gint index = px[0] * bytes + py[0] * rowstride;
  gint length = px[1] - px[0] + 1;

  for (i = 0; i < length; i++)
    {
      DRAW_POINT(buffer, index);
      index += bytes;
    }
  return;
}  /* draw_horizontal_line */

/*****/

static void
draw_right_bump(guchar *buffer, gint width, gint bytes, gint x_offset,
		gint curve_start_offset, gint steps)
{
  gint i;
  gint x, y;
  gint index;
  gint rowstride = width * bytes;

  for (i = 0; i < steps; i++)
    {
      x = *(globals.cachex1[RIGHT] + i) + x_offset;
      y = *(globals.cachey1[RIGHT] + i) + curve_start_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT(buffer, index);

      x = *(globals.cachex2[RIGHT] + i) + x_offset;
      y = *(globals.cachey2[RIGHT] + i) + curve_start_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT(buffer, index);
    }
  return;
}  /* draw_right_bump */

/*****/

static void
draw_left_bump(guchar *buffer, gint width, gint bytes, gint x_offset,
	       gint curve_start_offset, gint steps)
{
  gint i;
  gint x, y;
  gint index;
  gint rowstride = width * bytes;

  for (i = 0; i < steps; i++)
    {
      x = *(globals.cachex1[LEFT] + i) + x_offset;
      y = *(globals.cachey1[LEFT] + i) + curve_start_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT(buffer, index);

      x = *(globals.cachex2[LEFT] + i) + x_offset;
      y = *(globals.cachey2[LEFT] + i) + curve_start_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT(buffer, index);
    }
  return;
}  /* draw_left_bump */

/*****/

static void
draw_up_bump(guchar *buffer, gint width, gint bytes, gint y_offset,
	     gint curve_start_offset, gint steps)
{
  gint i;
  gint x, y;
  gint index;
  gint rowstride = width * bytes;

  for (i = 0; i < steps; i++)
    {
      x = *(globals.cachex1[UP] + i) + curve_start_offset;
      y = *(globals.cachey1[UP] + i) + y_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT(buffer, index);

      x = *(globals.cachex2[UP] + i) + curve_start_offset;
      y = *(globals.cachey2[UP] + i) + y_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT(buffer, index);
    }
  return;
}  /* draw_up_bump */

/*****/

static void
draw_down_bump(guchar *buffer, gint width, gint bytes, gint y_offset,
	       gint curve_start_offset, gint steps)
{
  gint i;
  gint x, y;
  gint index;
  gint rowstride = width * bytes;

  for (i = 0; i < steps; i++)
    {
      x = *(globals.cachex1[DOWN] + i) + curve_start_offset;
      y = *(globals.cachey1[DOWN] + i) + y_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT(buffer, index);

      x = *(globals.cachex2[DOWN] + i) + curve_start_offset;
      y = *(globals.cachey2[DOWN] + i) + y_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT(buffer, index);
    }
  return;
}  /* draw_down_bump */

/*****/

static void
malloc_cache(void)
{
  gint i, j;
  gint blend_lines = config.blend_lines;
  gint length = blend_lines * sizeof(gint *);

  for (i = 0; i < 4; i++)
    {
      gint steps_length = globals.steps[i] * sizeof(gint);
      
      globals.cachex1[i] = g_malloc(steps_length);
      globals.cachex2[i] = g_malloc(steps_length);
      globals.cachey1[i] = g_malloc(steps_length);
      globals.cachey2[i] = g_malloc(steps_length);
      globals.blend_outer_cachex1[i] = g_malloc(length);
      globals.blend_outer_cachex2[i] = g_malloc(length);
      globals.blend_outer_cachey1[i] = g_malloc(length);
      globals.blend_outer_cachey2[i] = g_malloc(length);
      globals.blend_inner_cachex1[i] = g_malloc(length);
      globals.blend_inner_cachex2[i] = g_malloc(length);
      globals.blend_inner_cachey1[i] = g_malloc(length);
      globals.blend_inner_cachey2[i] = g_malloc(length);
      for (j = 0; j < blend_lines; j++)
	{
	  *(globals.blend_outer_cachex1[i] + j) = g_malloc(steps_length);
	  *(globals.blend_outer_cachex2[i] + j) = g_malloc(steps_length);
	  *(globals.blend_outer_cachey1[i] + j) = g_malloc(steps_length);
	  *(globals.blend_outer_cachey2[i] + j) = g_malloc(steps_length);
	  *(globals.blend_inner_cachex1[i] + j) = g_malloc(steps_length);
	  *(globals.blend_inner_cachex2[i] + j) = g_malloc(steps_length);
	  *(globals.blend_inner_cachey1[i] + j) = g_malloc(steps_length);
	  *(globals.blend_inner_cachey2[i] + j) = g_malloc(steps_length);
	}
    } /* for */
  return;
}  /* malloc_cache() */

/*****/

static void
free_cache(void)
{
  gint i, j;
  gint blend_lines = config.blend_lines;

  for (i = 0; i < 4; i ++)
    {
      g_free(globals.cachex1[i]);
      g_free(globals.cachex2[i]);
      g_free(globals.cachey1[i]);
      g_free(globals.cachey2[i]);
      for (j = 0; j < blend_lines; j++)
	{
	  g_free(*(globals.blend_outer_cachex1[i] + j));
	  g_free(*(globals.blend_outer_cachex2[i] + j));
	  g_free(*(globals.blend_outer_cachey1[i] + j));
	  g_free(*(globals.blend_outer_cachey2[i] + j));
	  g_free(*(globals.blend_inner_cachex1[i] + j));
	  g_free(*(globals.blend_inner_cachex2[i] + j));
	  g_free(*(globals.blend_inner_cachey1[i] + j));
	  g_free(*(globals.blend_inner_cachey2[i] + j));
	}
      g_free(globals.blend_outer_cachex1[i]);
      g_free(globals.blend_outer_cachex2[i]);
      g_free(globals.blend_outer_cachey1[i]);
      g_free(globals.blend_outer_cachey2[i]);
      g_free(globals.blend_inner_cachex1[i]);
      g_free(globals.blend_inner_cachex2[i]);
      g_free(globals.blend_inner_cachey1[i]);
      g_free(globals.blend_inner_cachey2[i]);
    }  /* for */
  return;
}  /* free_cache */

/*****/

static void
init_right_bump(gint width, gint height)
{
  gint i;
  gint xtiles = config.x;
  gint ytiles = config.y;
  gint steps = globals.steps[RIGHT];
  gint px[4], py[4];
  gint x_offset = 0;
  gint tile_width =  width / xtiles;
  gint tile_height = height/ ytiles;
  gint tile_height_eighth = tile_height / 8;
  gint curve_start_offset = 0;
  gint curve_end_offset = curve_start_offset + 2 * tile_height_eighth;
  gint blend_lines = config.blend_lines;

  px[0] = x_offset;
  px[1] = x_offset + XFACTOR2 * tile_width;
  px[2] = x_offset + XFACTOR3 * tile_width;
  px[3] = x_offset + XFACTOR4 * tile_width;
  py[0] = curve_start_offset;
  py[1] = curve_start_offset + YFACTOR2 * tile_height;
  py[2] = curve_start_offset - YFACTOR3 * tile_height;
  py[3] = curve_start_offset + YFACTOR4 * tile_height;
  generate_bezier(px, py, steps, globals.cachex1[RIGHT],
		  globals.cachey1[RIGHT]);
  /* outside right bump */
  for (i = 0; i < blend_lines; i++)
    {
       py[0]--; py[1]--; py[2]--; px[3]++;
       generate_bezier(px, py, steps,
		       *(globals.blend_outer_cachex1[RIGHT] + i),
		       *(globals.blend_outer_cachey1[RIGHT] + i));
    }
  /* inside right bump */
  py[0] += blend_lines; py[1] += blend_lines; py[2] += blend_lines;
  px[3] -= blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      py[0]++; py[1]++; py[2]++; px[3]--;
      generate_bezier(px, py, steps,
		      *(globals.blend_inner_cachex1[RIGHT] + i),
		      *(globals.blend_inner_cachey1[RIGHT] + i));
    }

  /* bottom half of bump */
  px[0] = x_offset + XFACTOR5 * tile_width;
  px[1] = x_offset + XFACTOR6 * tile_width;
  px[2] = x_offset + XFACTOR7 * tile_width;
  px[3] = x_offset;
  py[0] = curve_start_offset + YFACTOR5 * tile_height;
  py[1] = curve_start_offset + YFACTOR6 * tile_height;
  py[2] = curve_start_offset + YFACTOR7 * tile_height;
  py[3] = curve_end_offset;
  generate_bezier(px, py, steps, globals.cachex2[RIGHT],
		  globals.cachey2[RIGHT]);
  /* outer right bump */
  for (i = 0; i < blend_lines; i++)
    {
      py[1]++; py[2]++; py[3]++; px[0]++;
      generate_bezier(px, py, steps,
		      *(globals.blend_outer_cachex2[RIGHT] + i),
		      *(globals.blend_outer_cachey2[RIGHT] + i));
    }
  /* inner right bump */
  py[1] -= blend_lines; py[2] -= blend_lines; py[3] -= blend_lines;
  px[0] -= blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      py[1]--; py[2]--; py[3]--; px[0]--;
      generate_bezier(px, py, steps,
		      *(globals.blend_inner_cachex2[RIGHT] + i),
		      *(globals.blend_inner_cachey2[RIGHT] + i));
    }
  return;
}  /* init_right_bump */

/*****/

static void
init_left_bump(gint width, gint height)
{
  gint i;
  gint xtiles = config.x;
  gint ytiles = config.y;
  gint steps = globals.steps[LEFT];
  gint px[4], py[4];
  gint x_offset = 0;
  gint tile_width = width / xtiles;
  gint tile_height = height / ytiles;
  gint tile_height_eighth = tile_height / 8;
  gint curve_start_offset = 0;
  gint curve_end_offset = curve_start_offset + 2 * tile_height_eighth;
  gint blend_lines = config.blend_lines;

  px[0] = x_offset;
  px[1] = x_offset - XFACTOR2 * tile_width;
  px[2] = x_offset - XFACTOR3 * tile_width;
  px[3] = x_offset - XFACTOR4 * tile_width;
  py[0] = curve_start_offset;
  py[1] = curve_start_offset + YFACTOR2 * tile_height;
  py[2] = curve_start_offset - YFACTOR3 * tile_height;
  py[3] = curve_start_offset + YFACTOR4 * tile_height;
  generate_bezier(px, py, steps, globals.cachex1[LEFT],
		  globals.cachey1[LEFT]);
  /* outer left bump */
  for (i = 0; i < blend_lines; i++)
    {
      py[0]--; py[1]--; py[2]--; px[3]--;
      generate_bezier(px, py, steps,
		      *(globals.blend_outer_cachex1[LEFT] + i),
		      *(globals.blend_outer_cachey1[LEFT] + i));
    }
  /* inner left bump */
  py[0] += blend_lines; py[1] += blend_lines; py[2] += blend_lines;
  px[3] += blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      py[0]++; py[1]++; py[2]++; px[3]++;
      generate_bezier(px, py, steps,
		      *(globals.blend_inner_cachex1[LEFT] + i),
		      *(globals.blend_inner_cachey1[LEFT] + i));
    }

  /* bottom half of bump */
  px[0] = x_offset - XFACTOR5 * tile_width;
  px[1] = x_offset - XFACTOR6 * tile_width;
  px[2] = x_offset - XFACTOR7 * tile_width;
  px[3] = x_offset;
  py[0] = curve_start_offset + YFACTOR5 * tile_height;
  py[1] = curve_start_offset + YFACTOR6 * tile_height;
  py[2] = curve_start_offset + YFACTOR7 * tile_height;
  py[3] = curve_end_offset;
  generate_bezier(px, py, steps, globals.cachex2[LEFT],
		  globals.cachey2[LEFT]);
  /* outer left bump */
  for (i = 0; i < blend_lines; i++)
    {
      py[1]++; py[2]++; py[3]++; px[0]--;
      generate_bezier(px, py, steps,
		      *(globals.blend_outer_cachex2[LEFT] + i),
		      *(globals.blend_outer_cachey2[LEFT] + i));
    }
  /* inner left bump */
  py[1] -= blend_lines; py[2] -= blend_lines; py[3] -= blend_lines;
  px[0] += blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      py[1]--; py[2]--; py[3]--; px[0]++;
      generate_bezier(px, py, steps,
		      *(globals.blend_inner_cachex2[LEFT] + i),
		      *(globals.blend_inner_cachey2[LEFT] + i));
    }
  return;
}  /* init_left_bump */

/*****/

static void
init_up_bump(gint width, gint height)
{
  gint i;
  gint xtiles = config.x;
  gint ytiles = config.y;
  gint steps = globals.steps[UP];
  gint px[4], py[4];
  gint y_offset = 0;
  gint tile_width =  width / xtiles;
  gint tile_height = height/ ytiles;
  gint tile_width_eighth = tile_width / 8;
  gint curve_start_offset = 0;
  gint curve_end_offset = curve_start_offset + 2 * tile_width_eighth;
  gint blend_lines = config.blend_lines;

  px[0] = curve_start_offset;
  px[1] = curve_start_offset + YFACTOR2 * tile_width;
  px[2] = curve_start_offset - YFACTOR3 * tile_width;
  px[3] = curve_start_offset + YFACTOR4 * tile_width;
  py[0] = y_offset;
  py[1] = y_offset - XFACTOR2 * tile_height;
  py[2] = y_offset - XFACTOR3 * tile_height;
  py[3] = y_offset - XFACTOR4 * tile_height;
  generate_bezier(px, py, steps, globals.cachex1[UP],
		  globals.cachey1[UP]);
  /* outer up bump */
  for (i = 0; i < blend_lines; i++)
    {
      px[0]--; px[1]--; px[2]--; py[3]--;
      generate_bezier(px, py, steps,
		      *(globals.blend_outer_cachex1[UP] + i),
		      *(globals.blend_outer_cachey1[UP] + i));
    }
  /* inner up bump */
  px[0] += blend_lines; px[1] += blend_lines; px[2] += blend_lines;
  py[3] += blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      px[0]++; px[1]++; px[2]++; py[3]++;
      generate_bezier(px, py, steps,
		      *(globals.blend_inner_cachex1[UP] + i),
		      *(globals.blend_inner_cachey1[UP] + i));
    }

  /* bottom half of bump */
  px[0] = curve_start_offset + YFACTOR5 * tile_width;
  px[1] = curve_start_offset + YFACTOR6 * tile_width;
  px[2] = curve_start_offset + YFACTOR7 * tile_width;
  px[3] = curve_end_offset;
  py[0] = y_offset - XFACTOR5 * tile_height;
  py[1] = y_offset - XFACTOR6 * tile_height;
  py[2] = y_offset - XFACTOR7 * tile_height;
  py[3] = y_offset;
  generate_bezier(px, py, steps, globals.cachex2[UP],
		  globals.cachey2[UP]);
  /* outer up bump */
  for (i = 0; i < blend_lines; i++)
    {
      px[1]++; px[2]++; px[3]++; py[0]--;
      generate_bezier(px, py, steps,
		      *(globals.blend_outer_cachex2[UP] + i),
		      *(globals.blend_outer_cachey2[UP] + i));
    }
  /* inner up bump */
  px[1] -= blend_lines; px[2] -= blend_lines; px[3] -= blend_lines;
  py[0] += blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      px[1]--; px[2]--; px[3]--; py[0]++;
      generate_bezier(px, py, steps,
		      *(globals.blend_inner_cachex2[UP] + i),
		      *(globals.blend_inner_cachey2[UP] + i));
    }
  return;
}  /* init_top_bump */

/*****/

static void
init_down_bump(gint width, gint height)
{
  gint i;
  gint xtiles = config.x;
  gint ytiles = config.y;
  gint steps = globals.steps[DOWN];
  gint px[4], py[4];
  gint y_offset = 0;
  gint tile_width =  width / xtiles;
  gint tile_height = height/ ytiles;
  gint tile_width_eighth = tile_width / 8;
  gint curve_start_offset = 0;
  gint curve_end_offset = curve_start_offset + 2 * tile_width_eighth;
  gint blend_lines = config.blend_lines;

  px[0] = curve_start_offset;
  px[1] = curve_start_offset + YFACTOR2 * tile_width;
  px[2] = curve_start_offset - YFACTOR3 * tile_width;
  px[3] = curve_start_offset + YFACTOR4 * tile_width;
  py[0] = y_offset;
  py[1] = y_offset + XFACTOR2 * tile_height;
  py[2] = y_offset + XFACTOR3 * tile_height;
  py[3] = y_offset + XFACTOR4 * tile_height;
  generate_bezier(px, py, steps, globals.cachex1[DOWN],
		  globals.cachey1[DOWN]);
  /* outer down bump */
  for (i = 0; i < blend_lines; i++)
    {
      px[0]--; px[1]--; px[2]--; py[3]++;
      generate_bezier(px, py, steps,
		      *(globals.blend_outer_cachex1[DOWN] + i),
		      *(globals.blend_outer_cachey1[DOWN] + i));
    }
  /* inner down bump */
  px[0] += blend_lines; px[1] += blend_lines; px[2] += blend_lines;
  py[3] -= blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      px[0]++; px[1]++; px[2]++; py[3]--;
      generate_bezier(px, py, steps,
		      *(globals.blend_inner_cachex1[DOWN] + i),
		      *(globals.blend_inner_cachey1[DOWN] + i));
    }

  /* bottom half of bump */
  px[0] = curve_start_offset + YFACTOR5 * tile_width;
  px[1] = curve_start_offset + YFACTOR6 * tile_width;
  px[2] = curve_start_offset + YFACTOR7 * tile_width;
  px[3] = curve_end_offset;
  py[0] = y_offset + XFACTOR5 * tile_height;
  py[1] = y_offset + XFACTOR6 * tile_height;
  py[2] = y_offset + XFACTOR7 * tile_height;
  py[3] = y_offset;
  generate_bezier(px, py, steps, globals.cachex2[DOWN],
		  globals.cachey2[DOWN]);
  /* outer down bump */
  for (i = 0; i < blend_lines; i++)
    {
      px[1]++; px[2]++; px[3]++; py[0]++;
      generate_bezier(px, py, steps,
		      *(globals.blend_outer_cachex2[DOWN] + i),
		      *(globals.blend_outer_cachey2[DOWN] + i));
    }
  /* inner down bump */
  px[1] -= blend_lines; px[2] -= blend_lines; px[3] -= blend_lines;
  py[0] -= blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      px[1]--; px[2]--; px[3]--; py[0]--;
      generate_bezier(px, py, steps,
		      *(globals.blend_inner_cachex2[DOWN] + i),
		      *(globals.blend_inner_cachey2[DOWN] + i));
    }
  return;
}  /* init_down_bump */

/*****/
  
static void
generate_grid(gint width, gint height, gint xtiles, gint ytiles,
	      gint *x, gint *y)
{
  gint i;
  gint xlines = xtiles - 1;
  gint ylines = ytiles - 1;
  gint tile_width = width / xtiles;
  gint tile_height = height / ytiles;
  gint tile_width_leftover = width % xtiles;
  gint tile_height_leftover = height % ytiles;
  gint x_offset = tile_width;
  gint y_offset = tile_height;
  gint carry;

  for (i = 0; i < xlines; i++)
    {
      x[i] = x_offset;
      x_offset += tile_width;
    }
  carry = 0;
  while (tile_width_leftover)
    {
      for (i = carry; i < xlines; i++)
	{
	  x[i] += 1;
	}
      tile_width_leftover--;
      carry++;
    }
  x[xlines] = width - 1;    /* padding for draw_horizontal_border */
  
  for (i = 0; i < ytiles; i++)
    {
      y[i] = y_offset;
      y_offset += tile_height;
    }
  carry = 0;
  while (tile_height_leftover)
    {
      for (i = carry; i < ylines; i++)
	{
	  y[i] += 1;
	}
      tile_height_leftover--;
      carry++;
    }
  y[ylines] = height - 1;   /* padding for draw_vertical_border */
  return;
}  /* generate_grid */

/*****/

/* assumes RGB* */
/* assumes py[1] > py[0] and px[0] = px[1] */
static void
darken_vertical_line(guchar *buffer, gint width, gint bytes,
		     gint px[2], gint py[2], gdouble delta )
{
  gint i;
  gint rowstride = width * bytes;
  gint index = px[0] * bytes + py[0] * rowstride;
  gint length = py[1] - py[0] + 1;
  gint temp;

  for (i = 0; i < length; i++)
    {
      DARKEN_POINT(buffer, index, delta, temp);
      index += rowstride;
    }
  return;
}  /* darken_vertical_line */

/*****/

/* assumes RGB* */
/* assumes py[1] > py[0] and px[0] = px[1] */
static void
lighten_vertical_line(guchar *buffer, gint width, gint bytes,
		      gint px[2], gint py[2], gdouble delta)
{
  gint i;
  gint rowstride = width * bytes;
  gint index = px[0] * bytes + py[0] * rowstride;
  gint length = py[1] - py[0] + 1;
  gint temp;

  for (i = 0; i < length; i++)
    {
      LIGHTEN_POINT(buffer, index, delta, temp);
      index += rowstride;
    }
  return;
}  /* lighten_vertical_line */

/*****/

/* assumes RGB* */
/* assumes px[1] > px[0] and py[0] = py[1] */
static void
darken_horizontal_line(guchar *buffer, gint width, gint bytes,
		       gint px[2], gint py[2], gdouble delta)
{
  gint i;
  gint rowstride = width * bytes;
  gint index = px[0] * bytes + py[0] * rowstride;
  gint length = px[1] - px[0] + 1;
  gint temp;

  for (i = 0; i < length; i++)
    {
      DARKEN_POINT(buffer, index, delta, temp);
      index += bytes;
    }
  return;
}  /* darken_horizontal_line */

/*****/

/* assumes RGB* */
/* assumes px[1] > px[0] and py[0] = py[1] */
static void
lighten_horizontal_line(guchar *buffer, gint width, gint bytes,
			gint px[2], gint py[2], gdouble delta)
{
  gint i;
  gint rowstride = width * bytes;
  gint index = px[0] * bytes + py[0] * rowstride;
  gint length = px[1] - px[0] + 1;
  gint temp;

  for (i = 0; i < length; i++)
    {
      LIGHTEN_POINT(buffer, index, delta, temp);
      index += bytes;
    }
  return;
}  /* lighten_horizontal_line */

/*****/

static void
darken_right_bump(guchar *buffer, gint width, gint bytes, gint x_offset,
		  gint curve_start_offset, gint steps, gdouble delta,
		  gint counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride = width * bytes;
  gint temp;
  gint j = counter;

  for (i = 0; i < steps; i++)
    {
      x = x_offset
	+ *(*(globals.blend_inner_cachex1[RIGHT] + j) + i);
      y = curve_start_offset
	+ *(*(globals.blend_inner_cachey1[RIGHT] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index1)
	{
	  if (i < steps / 1.3)
	    {
	      LIGHTEN_POINT(buffer, index, delta, temp);
	    }
	  else
	    {
	      DARKEN_POINT(buffer, index, delta, temp);
	    }
	  last_index1 = index;
	}

      x = x_offset
	+ *(*(globals.blend_inner_cachex2[RIGHT] + j) + i);
      y = curve_start_offset
	+ *(*(globals.blend_inner_cachey2[RIGHT] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index2)
	{
	  DARKEN_POINT(buffer, index, delta, temp);
	  last_index2 = index;
	}
    }
  return;
}  /* darken_right_bump */

/*****/

static void
lighten_right_bump(guchar *buffer, gint width, gint bytes, gint x_offset,
		   gint curve_start_offset, gint steps, gdouble delta,
		   gint counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride = width * bytes;
  gint temp;
  gint j = counter;

  for (i = 0; i < steps; i++)
    {
      x = x_offset
	+ *(*(globals.blend_outer_cachex1[RIGHT] + j) + i);
      y = curve_start_offset
	+ *(*(globals.blend_outer_cachey1[RIGHT] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index1)
	{
	  if (i < steps / 1.3)
	    {
	      DARKEN_POINT(buffer, index, delta, temp);
	    }
	  else
	    {
	      LIGHTEN_POINT(buffer, index, delta, temp);
	    }
	  last_index1 = index;
	}
      
      x = x_offset
	+ *(*(globals.blend_outer_cachex2[RIGHT] + j) + i);
      y = curve_start_offset
	+ *(*(globals.blend_outer_cachey2[RIGHT] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index2)
	{
	  LIGHTEN_POINT(buffer, index, delta, temp);
	  last_index2 = index;
	}
    }
  return;
}  /* lighten_right_bump */

/*****/

static void
darken_left_bump(guchar *buffer, gint width, gint bytes, gint x_offset,
		 gint curve_start_offset, gint steps, gdouble delta,
		 gint counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride = width * bytes;
  gint temp;
  gint j = counter;

  for (i = 0; i < steps; i++)
    {
      x = x_offset
	+ *(*(globals.blend_outer_cachex1[LEFT] + j) + i);
      y = curve_start_offset
	+ *(*(globals.blend_outer_cachey1[LEFT] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index1)
	{
	  DARKEN_POINT(buffer, index, delta, temp);
	  last_index1 = index;
	}

      x = x_offset
	+ *(*(globals.blend_outer_cachex2[LEFT] + j) + i);
      y = curve_start_offset
	+ *(*(globals.blend_outer_cachey2[LEFT] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index2)
	{
	  if (i < steps / 4)
	    {
	      DARKEN_POINT(buffer, index, delta, temp);
	    }
	  else
	    {
	      LIGHTEN_POINT(buffer, index, delta, temp);
	    }
	  last_index2 = index;
	}
    }
  return;
}  /* darken_left_bump */

/*****/

static void
lighten_left_bump(guchar *buffer, gint width, gint bytes, gint x_offset,
		  gint curve_start_offset, gint steps, gdouble delta,
		  gint counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1; 
  gint last_index2 = -1;
  gint rowstride = width * bytes;
  gint temp;
  gint j = counter;

  for (i = 0; i < steps; i++)
    {
      x = x_offset
	+ *(*(globals.blend_inner_cachex1[LEFT] + j) + i);
      y = curve_start_offset
	+ *(*(globals.blend_inner_cachey1[LEFT] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index1)
	{
	  LIGHTEN_POINT(buffer, index, delta, temp);
	  last_index1 = index;
	}

      x = x_offset
	+ *(*(globals.blend_inner_cachex2[LEFT] + j) + i);
      y = curve_start_offset
	+ *(*(globals.blend_inner_cachey2[LEFT] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index2)
	{
	  if (i < steps / 4)
	    {
	      LIGHTEN_POINT(buffer, index, delta, temp);
	    }
	  else
	    {
	      DARKEN_POINT(buffer, index, delta, temp);
	    }
	  last_index2 = index;
	}
    }
  return;
}  /* lighten_left_bump */

/*****/

static void
darken_up_bump(guchar *buffer, gint width, gint bytes, gint y_offset,
	       gint curve_start_offset, gint steps, gdouble delta,
	       gint counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride = width * bytes;
  gint temp;
  gint j = counter;

  for (i = 0; i < steps; i++)
    {
      x = curve_start_offset
	+ *(*(globals.blend_outer_cachex1[UP] + j) + i);
      y = y_offset
	+ *(*(globals.blend_outer_cachey1[UP] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index1)
	{
	  DARKEN_POINT(buffer, index, delta, temp);
	  last_index1 = index;
	}

      x = curve_start_offset
	+ *(*(globals.blend_outer_cachex2[UP] + j) + i);
      y = y_offset
	+ *(*(globals.blend_outer_cachey2[UP] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index2)
	{
	  if (i < steps / 4)
	    {
	      DARKEN_POINT(buffer, index, delta, temp);
	    }
	  else
	    {
	      LIGHTEN_POINT(buffer, index, delta, temp);
	    }
	  last_index2 = index;
	}
    }
  return;
}  /* darken_up_bump */

/*****/

static void
lighten_up_bump(guchar *buffer, gint width, gint bytes, gint y_offset,
		gint curve_start_offset, gint steps, gdouble delta,
		gint counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride = width * bytes;
  gint temp;
  gint j = counter;

  for (i = 0; i < steps; i++)
    {
      x = curve_start_offset
	+ *(*(globals.blend_inner_cachex1[UP] + j) + i);
      y = y_offset
	+ *(*(globals.blend_inner_cachey1[UP] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index1)
	{
	  LIGHTEN_POINT(buffer, index, delta, temp);
	  last_index1 = index;
	}

      x = curve_start_offset
	+ *(*(globals.blend_inner_cachex2[UP] + j) + i);
      y = y_offset
	+ *(*(globals.blend_inner_cachey2[UP] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index2)
	{
	  if (i < steps / 4)
	    {
	      LIGHTEN_POINT(buffer, index, delta, temp);
	    }
	  else
	    {
	      DARKEN_POINT(buffer, index, delta, temp);
	    }
	  last_index2 = index;
	}
    }
  return;
}  /* lighten_up_bump */

/*****/

static void
darken_down_bump(guchar *buffer, gint width, gint bytes, gint y_offset,
		 gint curve_start_offset, gint steps, gdouble delta,
		 gint counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride = width * bytes;
  gint temp;
  gint j = counter;

  for (i = 0; i < steps; i++)
    {
      x = curve_start_offset
	+ *(*(globals.blend_inner_cachex1[DOWN] + j) + i);
      y = y_offset
	+ *(*(globals.blend_inner_cachey1[DOWN] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index1)
	{
	  if (i < steps / 1.2)
	    {
	      LIGHTEN_POINT(buffer, index, delta, temp);
	    }
	  else
	    {
	      DARKEN_POINT(buffer, index, delta, temp);
	    }
	  last_index1 = index;
	}

      x = curve_start_offset
	+ *(*(globals.blend_inner_cachex2[DOWN] + j) + i);
      y = y_offset
	+ *(*(globals.blend_inner_cachey2[DOWN] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index2)
	{
	  DARKEN_POINT(buffer, index, delta, temp);
	  last_index2 = index;
	}
    }
  return;
}  /* darken_down_bump */

/*****/

static void
lighten_down_bump(guchar *buffer, gint width, gint bytes, gint y_offset,
		  gint curve_start_offset, gint steps, gdouble delta,
		  gint counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride = width * bytes;
  gint temp;
  gint j = counter;

  for (i = 0; i < steps; i++)
    {
      x = curve_start_offset
	+ *(*(globals.blend_outer_cachex1[DOWN] + j) + i);
      y = y_offset
	+ *(*(globals.blend_outer_cachey1[DOWN] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index1)
	{
	  if (i < steps / 1.2)
	    {
	      DARKEN_POINT(buffer, index, delta, temp);
	    }
	  else
	    {
	      LIGHTEN_POINT(buffer, index, delta, temp);
	    }
	  last_index1 = index;
	}

      x = curve_start_offset
	+ *(*(globals.blend_outer_cachex2[DOWN] + j) + i);
      y = y_offset
	+ *(*(globals.blend_outer_cachey2[DOWN] + j) + i);
      index = y * rowstride + x * bytes;
      if (index != last_index2)
	{
	  LIGHTEN_POINT(buffer, index, delta, temp);
	  last_index2 = index;
	}
    }
  return;
}  /* lighten_down_bump */

/*****/

static void
draw_bezier_line(guchar *buffer, gint width, gint bytes,
		 gint steps, gint *cx, gint *cy)
{
  gint i;
  gint x, y;
  gint index;
  gint rowstride = width * bytes;

  for (i = 0; i < steps; i++)
    {
      x = cx[i];
      y = cy[i];
      index = y * rowstride + x * bytes;
      DRAW_POINT(buffer, index);
    }
  return;
}  /* draw_bezier_line */

/*****/

static void
darken_bezier_line(guchar *buffer, gint width, gint bytes,
		   gint x_offset, gint y_offset, gint steps,
		   gint *cx, gint *cy, gdouble delta)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index = -1;
  gint rowstride = width * bytes;
  gint temp;

  for (i = 0; i < steps; i++)
    {
      x = cx[i] + x_offset;
      y = cy[i] + y_offset;
      index = y * rowstride + x * bytes;
      if (index != last_index)
	{
	  DARKEN_POINT(buffer, index, delta, temp);
	  last_index = index;
	}
    }
  return;
}  /* darken_bezier_line */

/*****/

static void
lighten_bezier_line(guchar *buffer, gint width, gint bytes,
		    gint x_offset, gint y_offset, gint steps,
		    gint *cx, gint *cy, gdouble delta)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index = -1;
  gint rowstride = width * bytes;
  gint temp;

  for (i = 0; i < steps; i++)
    {
      x = cx[i] + x_offset;
      y = cy[i] + y_offset;
      index = y * rowstride + x * bytes;
      if (index != last_index)
	{
	  LIGHTEN_POINT(buffer, index, delta, temp);
	  last_index = index;
	}
    }
  return;
}  /* lighten_bezier_line */

/*****/

static void
draw_bezier_vertical_border(guchar *buffer, gint width, gint height,
			    gint bytes, gint x_offset, gint xtiles,
			    gint ytiles, gint blend_lines,
			    gdouble blend_amount, gint steps)
{
  gint i, j;
  gint tile_width = width / xtiles;
  gint tile_height = height / ytiles;
  gint tile_height_eighth = tile_height / 8;
  gint curve_start_offset = 3 * tile_height_eighth;
  gint curve_end_offset = curve_start_offset + 2 * tile_height_eighth;
  gint px[4], py[4];
  gint y_offset = 0;
  gdouble delta;
  gdouble sigma = blend_amount / blend_lines;
  gint right;
  bump_t style_index;
  gint *cachex, *cachey;

  cachex = g_malloc(steps * sizeof(gint));
  cachey = g_malloc(steps * sizeof(gint));
  
  for (i = 0; i < ytiles; i++)
    {
      right = rand() & 1;
      if (right)
	{
	  style_index = RIGHT;
	}
      else
	{
	  style_index = LEFT;
	}
      px[0] = px[3] = x_offset;
      px[1] = x_offset + WALL_XFACTOR2 * tile_width * FUDGE;
      px[2] = x_offset + WALL_XFACTOR3 * tile_width * FUDGE;
      py[0] = y_offset;
      py[1] = y_offset + WALL_YCONS2 * tile_height;
      py[2] = y_offset + WALL_YCONS3 * tile_height;
      py[3] = y_offset + curve_start_offset;

      if (right)
	{
	  px[1] = x_offset - WALL_XFACTOR2 * tile_width;
	  px[2] = x_offset - WALL_XFACTOR3 * tile_width;
	}
      generate_bezier(px, py, steps, cachex, cachey);
      draw_bezier_line(buffer, width, bytes, steps, cachex, cachey);
      delta = blend_amount;
      for (j = 0; j < blend_lines; j++)
	{
	  px[0] =  -j - 1;
	  darken_bezier_line(buffer, width, bytes, px[0], 0,
			     steps, cachex, cachey, delta);
	  px[0] =  j + 1;
	  lighten_bezier_line(buffer, width, bytes, px[0], 0,
			      steps, cachex, cachey, delta);
	  delta -= sigma;
	}
      if (right)
	{
	  draw_right_bump(buffer, width, bytes, x_offset,
			  y_offset + curve_start_offset,
			  globals.steps[RIGHT]);
	  delta = blend_amount;
	  for (j = 0; j < blend_lines; j++)
	    {
	      /* use to be -j -1 */
	      darken_right_bump(buffer, width, bytes, x_offset,
				y_offset + curve_start_offset,
				globals.steps[RIGHT], delta, j);
	      /* use to be +j + 1 */
	      lighten_right_bump(buffer, width, bytes, x_offset,
				 y_offset + curve_start_offset,
				 globals.steps[RIGHT], delta, j);
	      delta -= sigma;
	    }
	}
      else
	{
	  draw_left_bump(buffer, width, bytes, x_offset,
			 y_offset + curve_start_offset,
			 globals.steps[LEFT]);
	  delta = blend_amount;
	  for (j = 0; j < blend_lines; j++)
	    {
	      /* use to be -j -1 */
	      darken_left_bump(buffer, width, bytes, x_offset,
			       y_offset + curve_start_offset,
			       globals.steps[LEFT], delta, j);
	      /* use to be -j - 1 */
	      lighten_left_bump(buffer, width, bytes, x_offset,
				y_offset + curve_start_offset,
				globals.steps[LEFT], delta, j);
	      delta -= sigma;
	    }
	}
      px[0] = px[3] = x_offset;
      px[1] = x_offset + WALL_XFACTOR2 * tile_width * FUDGE;
      px[2] = x_offset + WALL_XFACTOR3 * tile_width * FUDGE;
      py[0] = y_offset + curve_end_offset;
      py[1] = y_offset + curve_end_offset + WALL_YCONS2 * tile_height;
      py[2] = y_offset + curve_end_offset + WALL_YCONS3 * tile_height;
      py[3] = globals.gridy[i];
      if (right)
	{
	  px[1] = x_offset - WALL_XFACTOR2 * tile_width;
	  px[2] = x_offset - WALL_XFACTOR3 * tile_width;
	}
      generate_bezier(px, py, steps, cachex, cachey);
      draw_bezier_line(buffer, width, bytes, steps, cachex, cachey);
      delta = blend_amount;
      for (j = 0; j < blend_lines; j++)
	{
	  px[0] =  -j - 1;
	  darken_bezier_line(buffer, width, bytes, px[0], 0,
			     steps, cachex, cachey, delta);
	  px[0] =  j + 1;
	  lighten_bezier_line(buffer, width, bytes, px[0], 0,
			      steps, cachex, cachey, delta);
	  delta -= sigma;
	}
      y_offset = globals.gridy[i];
    }  /* for */
  g_free(cachex);
  g_free(cachey);
  
  return;
}  /* draw_bezier_vertical_border */

/*****/

static void
draw_bezier_horizontal_border(guchar *buffer, gint width, gint height,
			      gint bytes, gint y_offset, gint xtiles,
			      gint ytiles, gint blend_lines,
			      gdouble blend_amount, gint steps)
{
  gint i, j;
  gint tile_width = width / xtiles;
  gint tile_height = height / ytiles;
  gint tile_width_eighth = tile_width / 8;
  gint curve_start_offset = 3 * tile_width_eighth;
  gint curve_end_offset = curve_start_offset + 2 * tile_width_eighth;
  gint px[4], py[4];
  gint x_offset = 0;
  gdouble delta;
  gdouble sigma = blend_amount / blend_lines;
  gint up;
  style_t style_index;
  gint *cachex, *cachey;

  cachex = g_malloc(steps * sizeof(gint));
  cachey = g_malloc(steps * sizeof(gint));

  for (i = 0; i < xtiles; i++)
    {
      up = rand() & 1;
      if (up)
	{
	  style_index = UP;
	}
      else
	{
	  style_index = DOWN;
	}
      px[0] = x_offset;
      px[1] = x_offset + WALL_XCONS2 * tile_width;
      px[2] = x_offset + WALL_XCONS3 * tile_width;
      px[3] = x_offset + curve_start_offset;
      py[0] = py[3] = y_offset;
      py[1] = y_offset + WALL_YFACTOR2 * tile_height * FUDGE;
      py[2] = y_offset + WALL_YFACTOR3 * tile_height * FUDGE;
      if (!up)
	{
	  py[1] = y_offset - WALL_YFACTOR2 * tile_height;
	  py[2] = y_offset - WALL_YFACTOR3 * tile_height;
	}
      generate_bezier(px, py, steps, cachex, cachey);
      draw_bezier_line(buffer, width, bytes, steps, cachex, cachey);
      delta = blend_amount;
      for (j = 0; j < blend_lines; j++)
	{
	  py[0] = -j - 1;
	  darken_bezier_line(buffer, width, bytes, 0, py[0], 
			     steps, cachex, cachey, delta);
	  py[0] = j + 1;
	  lighten_bezier_line(buffer, width, bytes, 0, py[0],
			      steps, cachex, cachey, delta);
	  delta -= sigma;
	}
      /* bumps */
      if (up)
	{
	  draw_up_bump(buffer, width, bytes, y_offset,
		       x_offset + curve_start_offset,
		       globals.steps[UP]);
	  delta = blend_amount;
	  for (j = 0; j < blend_lines; j++)
	    {
	      /* use to be -j -1 */
	      darken_up_bump(buffer, width, bytes, y_offset,
			     x_offset + curve_start_offset,
			     globals.steps[UP], delta, j);
	      /* use to be +j + 1 */
	      lighten_up_bump(buffer, width, bytes, y_offset,
			      x_offset + curve_start_offset,
			      globals.steps[UP], delta, j);
	      delta -= sigma;
	    }
	}
      else
	{
	  draw_down_bump(buffer, width, bytes, y_offset,
			 x_offset + curve_start_offset,
			 globals.steps[DOWN]);
	  delta = blend_amount;
	  for (j = 0; j < blend_lines; j++)
	    {
	      /* use to be +j + 1 */
	      darken_down_bump(buffer, width, bytes, y_offset,
			       x_offset + curve_start_offset,
			       globals.steps[DOWN], delta, j);
	      /* use to be -j -1 */
	      lighten_down_bump(buffer, width, bytes, y_offset,
				x_offset + curve_start_offset,
				globals.steps[DOWN], delta, j);
	      delta -= sigma;
	    }
	}
      /* ending side wall line */
      px[0] = x_offset + curve_end_offset;
      px[1] = x_offset + curve_end_offset + WALL_XCONS2 * tile_width;
      px[2] = x_offset + curve_end_offset + WALL_XCONS3 * tile_width;
      px[3] = globals.gridx[i];
      py[0] = py[3] = y_offset;
      py[1] = y_offset + WALL_YFACTOR2 * tile_height * FUDGE;
      py[2] = y_offset + WALL_YFACTOR3 * tile_height * FUDGE;
      if (!up)
	{
	  py[1] = y_offset - WALL_YFACTOR2 * tile_height;
	  py[2] = y_offset - WALL_YFACTOR3 * tile_height;
	}
      generate_bezier(px, py, steps, cachex, cachey);
      draw_bezier_line(buffer, width, bytes, steps, cachex, cachey);
      delta = blend_amount;
      for (j = 0; j < blend_lines; j++)
	{
	  py[0] =  -j - 1;
	  darken_bezier_line(buffer, width, bytes, 0, py[0],
			     steps, cachex, cachey, delta);
	  py[0] =  j + 1;
	  lighten_bezier_line(buffer, width, bytes, 0, py[0],
			      steps, cachex, cachey, delta);
	  delta -= sigma;
	}
      x_offset = globals.gridx[i];
    }  /* for */
  g_free(cachex);
  g_free(cachey);
  
  return;
}  /* draw_bezier_horizontal_border */

/*****/

static void
check_config(gint width, gint height)
{
  gint tile_width, tile_height;
  gint tile_width_limit, tile_height_limit;
  
  if (config.x < 1)
    {
      config.x = 1;
    }
  if (config.y < 1)
    {
      config.y = 1;
    }
  if (config.blend_amount < 0)
    {
      config.blend_amount = 0;
    }
  if (config.blend_amount > 5)
    {
      config.blend_amount = 5;
    }
  tile_width = width / config.x;
  tile_height = height / config.y;
  tile_width_limit = 0.4 * tile_width;
  tile_height_limit = 0.4 * tile_height;
  if ((config.blend_lines > tile_width_limit)
      || (config.blend_lines > tile_height_limit))
    {
      config.blend_lines = MIN(tile_width_limit, tile_height_limit);
    }
  return;
}  /*check_config */
  
/********************************************************
  GUI
********************************************************/

static void
dialog_box (void)
{
  GtkWidget *dlg;
  GtkWidget *rbutton;
  GtkWidget *cbutton;
  GSList *list;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *table;
  GtkWidget *scale;
  GtkObject *adjustment;

  gchar buffer[12];

  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup (PLUG_IN_NAME);

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  /* Create the dialog box */
  
  dlg = gimp_dialog_new (_("Jigsaw"), "jigsaw",
			 gimp_plugin_help_func, "filters/jigsaw.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), run_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* init tooltips */
  gimp_help_init ();

  if (globals.tooltips == 0)
    {
      gimp_help_disable_tooltips ();
    }

  /* paramters frame */
  frame = gtk_frame_new (_("Number of Tiles"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* xtiles */
  label = gtk_label_new (_("Horizontal:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (config.x, MIN_XTILES, MAX_XTILES,
				   1.0, 1.0, 0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (adjustment_callback),
		      &config.x);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show (scale);
  gimp_help_set_help_data (scale, _("Number of pieces going across"), NULL);
  
  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), adjustment);
  gtk_object_set_user_data (GTK_OBJECT (adjustment), entry);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  g_snprintf (buffer, sizeof (buffer), "%i", config.x);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (entry_callback),
		      (gpointer) &config.x);
  gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (entry);
  gimp_help_set_help_data (entry, _("Number of pieces going across"), NULL);

  /* ytiles */
  label = gtk_label_new (_("Vertical:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (config.y, MIN_YTILES, MAX_YTILES,
				   1.0, 1.0, 0 );
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (adjustment_callback),
		      &config.y);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show (scale);
  gimp_help_set_help_data (scale, _("Number of pieces going down"), NULL);

  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), adjustment);
  gtk_object_set_user_data (GTK_OBJECT (adjustment), entry);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  g_snprintf (buffer, sizeof (buffer), "%i", config.y);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (entry_callback),
		      (gpointer) &config.y);
  gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show (entry);
  gimp_help_set_help_data (entry, _("Number of pieces going down"), NULL);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  /* frame for bevel blending */

  frame = gtk_frame_new (_("Bevel Edges"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* number of blending lines */
  label = gtk_label_new (_("Bevel width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (config.blend_lines, MIN_BLEND_LINES,
				   MAX_BLEND_LINES, 1.0, 1.0, 0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (adjustment_callback),
		      &config.blend_lines);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show (scale);
  gimp_help_set_help_data (scale, _("Degree of slope of each piece's edge"),
			   NULL);

  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), adjustment);
  gtk_object_set_user_data (GTK_OBJECT (adjustment), entry);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  g_snprintf (buffer, sizeof (buffer), "%i", config.blend_lines);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (entry_callback),
		      (gpointer) &config.blend_lines);
  gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (entry);
  gimp_help_set_help_data (entry, _("Degree of slope of each piece's edge"),
			   NULL);

  /* blending amount */
  label = gtk_label_new (_("Highlight:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (config.blend_amount, MIN_BLEND_AMOUNT,
				   MAX_BLEND_AMOUNT, 1.0, 0.05, 0.0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (adjustment_double_callback),
		      &config.blend_amount);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show (scale);
  gimp_help_set_help_data (scale, _("The amount of highlighting on the edges "
				    "of each piece"), NULL);

  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), adjustment);
  gtk_object_set_user_data (GTK_OBJECT (adjustment), entry);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  g_snprintf (buffer, sizeof (buffer), "%0.2f", config.blend_amount);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (entry_double_callback),
		      (gpointer) &config.blend_amount);
  gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show (entry);
  gimp_help_set_help_data (entry, _("The amount of highlighting on the edges "
				    "of each piece"), NULL);

  gtk_widget_show (table);
  gtk_widget_show (frame);
  
  /* frame for primitive radio buttons */

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 0);

  frame = gtk_frame_new (_("Jigsaw Style"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (2, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_container_add (GTK_CONTAINER (frame), table);

  rbutton = gtk_radio_button_new_with_label (NULL, _("Square"));
  list = gtk_radio_button_group ((GtkRadioButton *) rbutton);
  gtk_toggle_button_set_active ((GtkToggleButton *) rbutton,
				config.style == BEZIER_1 ? TRUE : FALSE);
  gtk_signal_connect (GTK_OBJECT (rbutton), "toggled",
		      GTK_SIGNAL_FUNC (radio_button_primitive_callback),
		      (gpointer) BEZIER_1);
  gtk_table_attach (GTK_TABLE (table), rbutton, 0, 1, 0, 1, GTK_FILL, 0, 10, 0);
  gtk_widget_show (rbutton);
  gimp_help_set_help_data (rbutton, _("Each piece has straight sides"), NULL);

  rbutton = gtk_radio_button_new_with_label (list, _("Curved"));
  list = gtk_radio_button_group ((GtkRadioButton *) rbutton);
  gtk_toggle_button_set_active ((GtkToggleButton *) rbutton,
				config.style == BEZIER_2 ? TRUE : FALSE);
  gtk_signal_connect (GTK_OBJECT (rbutton), "toggled",
		      GTK_SIGNAL_FUNC (radio_button_primitive_callback),
		      (gpointer) BEZIER_2);
  gtk_table_attach (GTK_TABLE (table), rbutton, 1, 2, 0, 1, GTK_FILL, 0, 10, 0);
  gtk_widget_show (rbutton);
  gimp_help_set_help_data (rbutton, _("Each piece has curved sides"), NULL);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  table = gtk_table_new (1, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 3);
  gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

  cbutton = gtk_check_button_new_with_label (_("Disable Tooltips"));
  gtk_toggle_button_set_active ((GtkToggleButton *) cbutton,
				globals.tooltips ? FALSE : TRUE);
  gtk_signal_connect (GTK_OBJECT (cbutton), "toggled",
		      GTK_SIGNAL_FUNC (check_button_callback),
		      NULL);
  gtk_table_attach (GTK_TABLE (table), cbutton, 0, 1, 1, 2, 0, 0, 0, 20);
  gtk_widget_show (cbutton);
  gimp_help_set_help_data (cbutton, _("Toggle Tooltips on/off"), NULL);

  gtk_widget_show (table);
  gtk_widget_show (hbox);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return;
}

/***************************************************
  callbacks
 ***************************************************/

static void
run_callback (GtkWidget *widget,
	      gpointer   data)
{
  globals.dialog_result = 1;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
entry_callback (GtkWidget *widget,
		gpointer   data)
{
  GtkAdjustment *adjustment;
  gint new_value;

  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
  
  if (*(gint *)data != new_value)
    {
      *(gint *)data = new_value;
      adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));
      if ((new_value >= adjustment->lower)
	  && (new_value <= adjustment->upper))
	{
	  adjustment->value = new_value;
	  gtk_signal_handler_block_by_data(GTK_OBJECT(adjustment), data);
	  gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
	  gtk_signal_handler_unblock_by_data(GTK_OBJECT(adjustment), data);
	}
    }
}

static void
radio_button_primitive_callback (GtkWidget *widget,
				 gpointer   data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active) /* button just got checked */
    {
      if (data == (gpointer) BEZIER_1)
	{
	  config.style = BEZIER_1;
	}
      else if (data == (gpointer) BEZIER_2)
	{
	  config.style = BEZIER_2;
	}
      else
	{
	  printf("radio_button_callback: bad data\n");
	}
    }
}

static void
check_button_callback (GtkWidget *widget,
		       gpointer   data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      gimp_help_disable_tooltips ();
      globals.tooltips = 0;
    }
  else
    {
      gimp_help_enable_tooltips ();
      globals.tooltips = 1;
    }
}

static void
adjustment_callback (GtkAdjustment *adjustment,
		     gpointer       data)
{
  GtkWidget *entry;
  gchar buffer[50];

  if (*(gint *)data != adjustment->value)
    {
      *(gint *)data = adjustment->value;
      entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
      g_snprintf (buffer, sizeof (buffer), "%d", *(gint *)data);

      gtk_signal_handler_block_by_data(GTK_OBJECT(entry), data);
      gtk_entry_set_text(GTK_ENTRY(entry), buffer);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), data);
    }
}

static void
adjustment_double_callback (GtkAdjustment *adjustment,
			    gpointer       data)
{
  GtkWidget *entry;
  gchar buffer[50];

  if (*(gdouble *)data != adjustment->value)
    {
      *(gdouble *)data = adjustment->value;
      entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
      g_snprintf (buffer, sizeof (buffer), "%0.2f", *(gdouble *)data);

      gtk_signal_handler_block_by_data(GTK_OBJECT(entry), data);
      gtk_entry_set_text(GTK_ENTRY(entry), buffer);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), data);
    }
}

static void
entry_double_callback (GtkWidget *widget,
		       gpointer   data)
{
  GtkAdjustment *adjustment;
  gdouble new_value;

  new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (*(gdouble *)data != new_value)
    {
      *(gdouble *)data = new_value;
      adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));
      if ((new_value >= adjustment->lower)
	  && (new_value <= adjustment->upper))
	{
	  adjustment->value = new_value;
	  gtk_signal_handler_block_by_data(GTK_OBJECT(adjustment), data);
	  gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
	  gtk_signal_handler_unblock_by_data(GTK_OBJECT(adjustment), data);
	}
    }
}
