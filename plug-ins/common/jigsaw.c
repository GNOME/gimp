/*
 * jigsaw - a plug-in for GIMP
 *
 * Copyright (C) Nigel Wetten
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact info: nigel@cs.nwu.edu
 *
 * Version: 1.0.0
 *
 * Version: 1.0.1
 *
 * tim coppefield [timecop@japan.co.jp]
 *
 * Added dynamic preview mode.
 *
 * Damn, this plugin is the tightest piece of code I ever seen.
 * I wish all filters in the plugins operated on guchar *buffer
 * of the entire image :) sweet stuff.
 *
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-jigsaw"
#define PLUG_IN_BINARY "jigsaw"
#define PLUG_IN_ROLE   "gimp-jigsaw"


typedef enum
{
  BEZIER_1,
  BEZIER_2
} style_t;

typedef enum
{
  LEFT,
  RIGHT,
  UP,
  DOWN
} bump_t;


static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

static void     jigsaw             (GimpDrawable *drawable,
                                    GimpPreview  *preview);

static gboolean jigsaw_dialog      (GimpDrawable *drawable);

static void     draw_jigsaw        (guchar    *buffer,
                                    gint       bufsize,
                                    gint       width,
                                    gint       height,
                                    gint       bytes,
                                    gboolean   preview_mode);

static void draw_vertical_border   (guchar *buffer, gint bufsize,
                                    gint width, gint height,
                                    gint bytes, gint x_offset, gint ytiles,
                                    gint blend_lines, gdouble blend_amount);
static void draw_horizontal_border (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint y_offset, gint xtiles,
                                    gint blend_lines, gdouble blend_amount);
static void draw_vertical_line     (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint px[2], gint py[2]);
static void draw_horizontal_line   (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint px[2], gint py[2]);
static void darken_vertical_line   (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint *px, gint *py, gdouble delta);
static void lighten_vertical_line  (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint *px, gint *py, gdouble delta);
static void darken_horizontal_line (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint *px, gint *py, gdouble delta);
static void lighten_horizontal_line(guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint *px, gint *py, gdouble delta);
static void draw_right_bump        (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint x_offset, gint curve_start_offset,
                                    gint steps);
static void draw_left_bump         (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint x_offset, gint curve_start_offset,
                                    gint steps);
static void draw_up_bump           (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint y_offset, gint curve_start_offset,
                                    gint steps);
static void draw_down_bump         (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint y_offset, gint curve_start_offset,
                                    gint steps);
static void darken_right_bump      (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint x_offset, gint curve_start_offset,
                                    gint steps, gdouble delta, gint counter);
static void lighten_right_bump     (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint x_offset, gint curve_start_offset,
                                    gint steps, gdouble delta, gint counter);
static void darken_left_bump       (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint x_offset, gint curve_start_offset,
                                    gint steps, gdouble delta, gint counter);
static void lighten_left_bump      (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint x_offset, gint curve_start_offset,
                                    gint steps, gdouble delta, gint counter);
static void darken_up_bump         (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint y_offset, gint curve_start_offest,
                                    gint steps, gdouble delta, gint counter);
static void lighten_up_bump        (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint y_offset, gint curve_start_offset,
                                    gint steps, gdouble delta, gint counter);
static void darken_down_bump       (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint y_offset, gint curve_start_offset,
                                    gint steps, gdouble delta, gint counter);
static void lighten_down_bump      (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint y_offset, gint curve_start_offset,
                                    gint steps, gdouble delta, gint counter);
static void generate_grid          (gint width, gint height, gint xtiles, gint ytiles,
                                    gint *x, gint *y);
static void generate_bezier        (gint px[4], gint py[4], gint steps,
                                    gint *cachex, gint *cachey);
static void malloc_cache           (void);
static void free_cache             (void);
static void init_right_bump        (gint width, gint height);
static void init_left_bump         (gint width, gint height);
static void init_up_bump           (gint width, gint height);
static void init_down_bump         (gint width, gint height);
static void draw_bezier_line       (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint steps, gint *cx, gint *cy);
static void darken_bezier_line     (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint x_offset, gint y_offset, gint steps,
                                    gint *cx, gint *cy, gdouble delta);
static void lighten_bezier_line    (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint x_offset, gint y_offset, gint steps,
                                    gint *cx, gint *cy, gdouble delta);
static void draw_bezier_vertical_border   (guchar *buffer, gint bufsize,
                                           gint width, gint height,
                                           gint bytes,
                                           gint x_offset, gint xtiles,
                                           gint ytiles, gint blend_lines,
                                           gdouble blend_amount, gint steps);
static void draw_bezier_horizontal_border (guchar *buffer, gint bufsize,
                                           gint width, gint height,
                                           gint bytes,
                                           gint x_offset, gint xtiles,
                                           gint ytiles, gint blend_lines,
                                           gdouble blend_amount, gint steps);
static void check_config           (gint width, gint height);



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

#define DRAW_POINT(buffer, bufsize, index)         \
  do                                               \
    {                                              \
      if ((index) >= 0 && (index) + 2 < (bufsize)) \
        {                                          \
          buffer[(index) + 0] = BLACK_R;           \
          buffer[(index) + 1] = BLACK_G;           \
          buffer[(index) + 2] = BLACK_B;           \
        }                                          \
    }                                              \
  while (0)

#define DARKEN_POINT(buffer, bufsize, index, delta, temp)                \
  do                                                                     \
    {                                                                    \
      if ((index) >= 0 && (index) + 2 < (bufsize))                       \
        {                                                                \
          temp = MAX (buffer[(index) + 0] * (1.0 - (delta)), MIN_VALUE); \
          buffer[(index) + 0] = temp;                                    \
          temp = MAX (buffer[(index) + 1] * (1.0 - (delta)), MIN_VALUE); \
          buffer[(index) + 1] = temp;                                    \
          temp = MAX (buffer[(index) + 2] * (1.0 - (delta)), MIN_VALUE); \
          buffer[(index) + 2] = temp;                                    \
        }                                                                \
    }                                                                    \
  while (0)

#define LIGHTEN_POINT(buffer, bufsize, index, delta, temp)               \
  do                                                                     \
    {                                                                    \
      if ((index) >= 0 && (index) + 2 < (bufsize))                       \
        {                                                                \
          temp = MIN (buffer[(index) + 0] * (1.0 + (delta)), MAX_VALUE); \
          buffer[(index) + 0] = temp;                                    \
          temp = MIN (buffer[(index) + 1] * (1.0 + (delta)), MAX_VALUE); \
          buffer[(index) + 1] = temp;                                    \
          temp = MIN (buffer[(index) + 2] * (1.0 + (delta)), MAX_VALUE); \
          buffer[(index) + 2] = temp;                                    \
        }                                                                \
    }                                                                    \
  while (0)


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};

struct config_tag
{
  gint     x;
  gint     y;
  style_t  style;
  gint     blend_lines;
  gdouble  blend_amount;
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
  gint  *cachex1[4];
  gint  *cachex2[4];
  gint  *cachey1[4];
  gint  *cachey2[4];
  gint   steps[4];
  gint  *gridx;
  gint  *gridy;
  gint **blend_outer_cachex1[4];
  gint **blend_outer_cachex2[4];
  gint **blend_outer_cachey1[4];
  gint **blend_outer_cachey2[4];
  gint **blend_inner_cachex1[4];
  gint **blend_inner_cachex2[4];
  gint **blend_inner_cachey1[4];
  gint **blend_inner_cachey2[4];
};

typedef struct globals_tag globals_t;

static globals_t globals;

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Input drawable" },
    { GIMP_PDB_INT32,    "x",            "Number of tiles across > 0" },
    { GIMP_PDB_INT32,    "y",            "Number of tiles down > 0" },
    { GIMP_PDB_INT32,    "style",        "The style/shape of the jigsaw puzzle { 0, 1 }" },
    { GIMP_PDB_INT32,    "blend-lines",  "Number of lines for shading bevels >= 0" },
    { GIMP_PDB_FLOAT,    "blend-amount", "The power of the light highlights 0 =< 5" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Add a jigsaw-puzzle pattern to the image"),
                          "Jigsaw puzzle look",
                          "Nigel Wetten",
                          "Nigel Wetten",
                          "May 2000",
                          N_("_Jigsaw..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Render/Pattern");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  GimpDrawable      *drawable;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get(param[2].data.d_drawable);
  gimp_tile_cache_ntiles (drawable->width / gimp_tile_width () + 1);

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      if (nparams == 8)
        {
          config.x = param[3].data.d_int32;
          config.y = param[4].data.d_int32;
          config.style = param[5].data.d_int32;
          config.blend_lines = param[6].data.d_int32;
          config.blend_amount = param[7].data.d_float;

          jigsaw (drawable, NULL);
        }
      else
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      break;

    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &config);
      if (! jigsaw_dialog (drawable))
        {
          status = GIMP_PDB_CANCEL;
          break;
        }
      gimp_progress_init (_("Assembling jigsaw"));

      jigsaw (drawable, NULL);
      gimp_set_data (PLUG_IN_PROC, &config, sizeof(config_t));
      gimp_displays_flush ();
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &config);
      jigsaw (drawable, NULL);
      gimp_displays_flush ();
    }  /* switch */

  gimp_drawable_detach (drawable);

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
jigsaw (GimpDrawable *drawable,
        GimpPreview  *preview)
{
  GimpPixelRgn  src_pr, dest_pr;
  guchar       *buffer;
  gint          width;
  gint          height;
  gint          bytes;
  gint          buffer_size;

  if (preview)
    {
      gimp_preview_get_size (preview, &width, &height);
      bytes  = drawable->bpp;
      buffer = gimp_drawable_get_thumbnail_data (drawable->drawable_id,
                                                 &width, &height, &bytes);
      buffer_size = bytes * width * height;
    }
  else
    {
      width  = drawable->width;
      height = drawable->height;
      bytes  = drawable->bpp;

      /* setup image buffer */
      buffer_size = bytes * width * height;
      buffer = g_new (guchar, buffer_size);

      gimp_pixel_rgn_init (&src_pr,  drawable, 0, 0, width, height,
                           FALSE, FALSE);
      gimp_pixel_rgn_get_rect (&src_pr, buffer, 0, 0, width, height);
    }

  check_config (width, height);
  globals.steps[LEFT] = globals.steps[RIGHT] = globals.steps[UP]
    = globals.steps[DOWN] = (config.x < config.y) ?
    (width / config.x * 2) : (height / config.y * 2);

  malloc_cache ();
  draw_jigsaw (buffer, buffer_size, width, height, bytes, preview != NULL);
  free_cache ();

  /* cleanup */
  if (preview)
    {
      gimp_preview_draw_buffer (preview, buffer, width * bytes);
    }
  else
    {
      gimp_pixel_rgn_init (&dest_pr, drawable, 0, 0, width, height,
                           TRUE, TRUE);
      gimp_pixel_rgn_set_rect (&dest_pr, buffer, 0, 0, width, height);

      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, 0, 0, width, height);
    }

  g_free(buffer);
}

static void
generate_bezier (gint  px[4],
                 gint  py[4],
                 gint  steps,
                 gint *cachex,
                 gint *cachey)
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
}

static void
draw_jigsaw (guchar   *buffer,
             gint      bufsize,
             gint      width,
             gint      height,
             gint      bytes,
             gboolean  preview_mode)
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

  g_return_if_fail (buffer != NULL);

  globals.gridx = g_new (gint, xtiles);
  globals.gridy = g_new (gint, ytiles);
  x = globals.gridx;
  y = globals.gridy;

  generate_grid (width, height, xtiles, ytiles, globals.gridx, globals.gridy);

  init_right_bump (width, height);
  init_left_bump  (width, height);
  init_up_bump    (width, height);
  init_down_bump  (width, height);

  if (style == BEZIER_1)
    {
      for (i = 0; i < xlines; i++)
        {
          draw_vertical_border (buffer, bufsize, width, height, bytes,
                                x[i], ytiles,
                                blend_lines, blend_amount);
          if (!preview_mode)
            gimp_progress_update ((gdouble) i / (gdouble) progress_total);
        }
      for (i = 0; i < ylines; i++)
        {
          draw_horizontal_border (buffer, bufsize, width, bytes, y[i], xtiles,
                                  blend_lines, blend_amount);
          if (!preview_mode)
            gimp_progress_update ((gdouble) (i + xlines) / (gdouble) progress_total);
        }
    }
  else if (style == BEZIER_2)
    {
      for (i = 0; i < xlines; i++)
        {
          draw_bezier_vertical_border (buffer, bufsize, width, height, bytes,
                                       x[i], xtiles, ytiles, blend_lines,
                                       blend_amount, steps);
          if (!preview_mode)
            gimp_progress_update ((gdouble) i / (gdouble) progress_total);
        }
      for (i = 0; i < ylines; i++)
        {
          draw_bezier_horizontal_border (buffer, bufsize, width, height, bytes,
                                         y[i], xtiles, ytiles, blend_lines,
                                         blend_amount, steps);
          if (!preview_mode)
            gimp_progress_update ((gdouble) (i + xlines) / (gdouble) progress_total);
        }
    }
  else
    {
      printf("draw_jigsaw: bad style\n");
      gimp_quit ();
    }
  gimp_progress_update (1.0);

  g_free (globals.gridx);
  g_free (globals.gridy);
}

static void
draw_vertical_border (guchar  *buffer,
                      gint     bufsize,
                      gint     width,
                      gint     height,
                      gint     bytes,
                      gint     x_offset,
                      gint     ytiles,
                      gint     blend_lines,
                      gdouble  blend_amount)
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

  for (i = 0; i < ytiles; i++)
    {
      right = g_random_int_range (0, 2);

      /* first straight line from top downwards */
      px[0] = px[1] = x_offset;
      py[0] = y_offset; py[1] = y_offset + curve_start_offset - 1;
      draw_vertical_line (buffer, bufsize, width, bytes, px, py);
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
          darken_vertical_line (buffer, bufsize, width, bytes, px, dy, delta);
          px[0] = x_offset + j + 1;
          lighten_vertical_line (buffer, bufsize, width, bytes, px, ly, delta);
          delta -= sigma;
        }

      /* top half of curve */
      if (right)
        {
          draw_right_bump (buffer, bufsize, width, bytes, x_offset,
                           y_offset + curve_start_offset,
                           globals.steps[RIGHT]);
          delta = blend_amount;
          for (j = 0; j < blend_lines; j++)
            {
              /* use to be -j -1 */
              darken_right_bump (buffer, bufsize, width, bytes, x_offset,
                                 y_offset + curve_start_offset,
                                 globals.steps[RIGHT], delta, j);
              /* use to be +j + 1 */
              lighten_right_bump (buffer, bufsize, width, bytes, x_offset,
                                  y_offset + curve_start_offset,
                                  globals.steps[RIGHT], delta, j);
              delta -= sigma;
            }
        }
      else
        {
          draw_left_bump (buffer, bufsize, width, bytes, x_offset,
                          y_offset + curve_start_offset,
                          globals.steps[LEFT]);
          delta = blend_amount;
          for (j = 0; j < blend_lines; j++)
            {
              /* use to be -j -1 */
              darken_left_bump (buffer, bufsize, width, bytes, x_offset,
                                y_offset + curve_start_offset,
                                globals.steps[LEFT], delta, j);
              /* use to be -j - 1 */
              lighten_left_bump (buffer, bufsize, width, bytes, x_offset,
                                 y_offset + curve_start_offset,
                                 globals.steps[LEFT], delta, j);
              delta -= sigma;
            }
        }
      /* bottom straight line of a tile wall */
      px[0] = px[1] = x_offset;
      py[0] = y_offset + curve_end_offset;
      py[1] = globals.gridy[i];
      draw_vertical_line (buffer, bufsize, width, bytes, px, py);
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
          darken_vertical_line (buffer, bufsize, width, bytes, px, dy, delta);
          px[0] = x_offset + j + 1;
          lighten_vertical_line (buffer, bufsize, width, bytes, px, ly, delta);
          delta -= sigma;
        }

      y_offset = globals.gridy[i];
    }  /* for */
}

/* assumes RGB* */
static void
draw_horizontal_border (guchar   *buffer,
                        gint      bufsize,
                        gint      width,
                        gint      bytes,
                        gint      y_offset,
                        gint      xtiles,
                        gint      blend_lines,
                        gdouble   blend_amount)
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
      up = g_random_int_range (0, 2);

      /* first horizontal line across */
      px[0] = x_offset; px[1] = x_offset + curve_start_offset - 1;
      py[0] = py[1] = y_offset;
      draw_horizontal_line (buffer, bufsize, width, bytes, px, py);
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
          darken_horizontal_line (buffer, bufsize, width, bytes, dx, py,
                                  delta);
          py[0] = y_offset + j + 1;
          lighten_horizontal_line (buffer, bufsize, width, bytes, lx, py,
                                   delta);
          delta -= sigma;
        }

      if (up)
        {
          draw_up_bump (buffer, bufsize, width, bytes, y_offset,
                        x_offset + curve_start_offset,
                        globals.steps[UP]);
          delta = blend_amount;
          for (j = 0; j < blend_lines; j++)
            {
              /* use to be -j -1 */
              darken_up_bump (buffer, bufsize, width, bytes, y_offset,
                              x_offset + curve_start_offset,
                              globals.steps[UP], delta, j);
              /* use to be +j + 1 */
              lighten_up_bump (buffer, bufsize, width, bytes, y_offset,
                               x_offset + curve_start_offset,
                               globals.steps[UP], delta, j);
              delta -= sigma;
            }
        }
      else
        {
          draw_down_bump (buffer, bufsize, width, bytes, y_offset,
                          x_offset + curve_start_offset,
                          globals.steps[DOWN]);
          delta = blend_amount;
          for (j = 0; j < blend_lines; j++)
            {
              /* use to be +j + 1 */
              darken_down_bump (buffer, bufsize, width, bytes, y_offset,
                                x_offset + curve_start_offset,
                                globals.steps[DOWN], delta, j);
              /* use to be -j -1 */
              lighten_down_bump (buffer, bufsize, width, bytes, y_offset,
                                 x_offset + curve_start_offset,
                                 globals.steps[DOWN], delta, j);
              delta -= sigma;
            }
        }
      /* right horizontal line of tile */
      px[0] = x_offset + curve_end_offset;
      px[1] = globals.gridx[i];
      py[0] = py[1] = y_offset;
      draw_horizontal_line (buffer, bufsize, width, bytes, px, py);
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
          darken_horizontal_line (buffer, bufsize, width, bytes, dx, py,
                                  delta);
          py[0] = y_offset + j + 1;
          lighten_horizontal_line (buffer, bufsize, width, bytes, lx, py,
                                   delta);
          delta -= sigma;
        }
      x_offset = globals.gridx[i];
    }  /* for */
}

/* assumes going top to bottom */
static void
draw_vertical_line (guchar   *buffer,
                    gint      bufsize,
                    gint      width,
                    gint      bytes,
                    gint      px[2],
                    gint      py[2])
{
  gint i;
  gint rowstride;
  gint index;
  gint length;

  rowstride = bytes * width;
  index = px[0] * bytes + rowstride * py[0];
  length = py[1] - py[0] + 1;

  for (i = 0; i < length; i++)
    {
      DRAW_POINT (buffer, bufsize, index);
      index += rowstride;
    }
}

/* assumes going left to right */
static void
draw_horizontal_line (guchar   *buffer,
                      gint      bufsize,
                      gint      width,
                      gint      bytes,
                      gint      px[2],
                      gint      py[2])
{
  gint i;
  gint rowstride;
  gint index;
  gint length;

  rowstride = bytes * width;
  index = px[0] * bytes + rowstride * py[0];
  length = px[1] - px[0] + 1;

  for (i = 0; i < length; i++)
    {
      DRAW_POINT (buffer, bufsize, index);
      index += bytes;
    }
}

static void
draw_right_bump (guchar   *buffer,
                 gint      bufsize,
                 gint      width,
                 gint      bytes,
                 gint      x_offset,
                 gint      curve_start_offset,
                 gint      steps)
{
  gint i;
  gint x, y;
  gint index;
  gint rowstride;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = globals.cachex1[RIGHT][i] + x_offset;
      y = globals.cachey1[RIGHT][i] + curve_start_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT (buffer, bufsize, index);

      x = globals.cachex2[RIGHT][i] + x_offset;
      y = globals.cachey2[RIGHT][i] + curve_start_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT (buffer, bufsize, index);
    }
}

static void
draw_left_bump (guchar   *buffer,
                gint      bufsize,
                gint      width,
                gint      bytes,
                gint      x_offset,
                gint      curve_start_offset,
                gint      steps)
{
  gint i;
  gint x, y;
  gint index;
  gint rowstride;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = globals.cachex1[LEFT][i] + x_offset;
      y = globals.cachey1[LEFT][i] + curve_start_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT (buffer, bufsize, index);

      x = globals.cachex2[LEFT][i] + x_offset;
      y = globals.cachey2[LEFT][i] + curve_start_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT (buffer, bufsize, index);
    }
}

static void
draw_up_bump (guchar   *buffer,
              gint      bufsize,
              gint      width,
              gint      bytes,
              gint      y_offset,
              gint      curve_start_offset,
              gint      steps)
{
  gint i;
  gint x, y;
  gint index;
  gint rowstride;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = globals.cachex1[UP][i] + curve_start_offset;
      y = globals.cachey1[UP][i] + y_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT (buffer, bufsize, index);

      x = globals.cachex2[UP][i] + curve_start_offset;
      y = globals.cachey2[UP][i] + y_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT (buffer, bufsize, index);
    }
}

static void
draw_down_bump (guchar   *buffer,
                gint      bufsize,
                gint      width,
                gint      bytes,
                gint      y_offset,
                gint      curve_start_offset,
                gint      steps)
{
  gint i;
  gint x, y;
  gint index;
  gint rowstride;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = globals.cachex1[DOWN][i] + curve_start_offset;
      y = globals.cachey1[DOWN][i] + y_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT (buffer, bufsize, index);

      x = globals.cachex2[DOWN][i] + curve_start_offset;
      y = globals.cachey2[DOWN][i] + y_offset;
      index = y * rowstride + x * bytes;
      DRAW_POINT (buffer, bufsize, index);
    }
}

static void
malloc_cache (void)
{
  gint i, j;
  gint blend_lines = config.blend_lines;

  for (i = 0; i < 4; i++)
    {
      gint steps = globals.steps[i];

      globals.cachex1[i] = g_new (gint, steps);
      globals.cachex2[i] = g_new (gint, steps);
      globals.cachey1[i] = g_new (gint, steps);
      globals.cachey2[i] = g_new (gint, steps);
      globals.blend_outer_cachex1[i] = g_new (gint *, blend_lines);
      globals.blend_outer_cachex2[i] = g_new (gint *, blend_lines);
      globals.blend_outer_cachey1[i] = g_new (gint *, blend_lines);
      globals.blend_outer_cachey2[i] = g_new (gint *, blend_lines);
      globals.blend_inner_cachex1[i] = g_new (gint *, blend_lines);
      globals.blend_inner_cachex2[i] = g_new (gint *, blend_lines);
      globals.blend_inner_cachey1[i] = g_new (gint *, blend_lines);
      globals.blend_inner_cachey2[i] = g_new (gint *, blend_lines);
      for (j = 0; j < blend_lines; j++)
        {
          globals.blend_outer_cachex1[i][j] = g_new (gint, steps);
          globals.blend_outer_cachex2[i][j] = g_new (gint, steps);
          globals.blend_outer_cachey1[i][j] = g_new (gint, steps);
          globals.blend_outer_cachey2[i][j] = g_new (gint, steps);
          globals.blend_inner_cachex1[i][j] = g_new (gint, steps);
          globals.blend_inner_cachex2[i][j] = g_new (gint, steps);
          globals.blend_inner_cachey1[i][j] = g_new (gint, steps);
          globals.blend_inner_cachey2[i][j] = g_new (gint, steps);
        }
    }
}

static void
free_cache (void)
{
  gint i, j;
  gint blend_lines = config.blend_lines;

  for (i = 0; i < 4; i ++)
    {
      g_free (globals.cachex1[i]);
      g_free (globals.cachex2[i]);
      g_free (globals.cachey1[i]);
      g_free (globals.cachey2[i]);
      for (j = 0; j < blend_lines; j++)
        {
          g_free (globals.blend_outer_cachex1[i][j]);
          g_free (globals.blend_outer_cachex2[i][j]);
          g_free (globals.blend_outer_cachey1[i][j]);
          g_free (globals.blend_outer_cachey2[i][j]);
          g_free (globals.blend_inner_cachex1[i][j]);
          g_free (globals.blend_inner_cachex2[i][j]);
          g_free (globals.blend_inner_cachey1[i][j]);
          g_free (globals.blend_inner_cachey2[i][j]);
        }
      g_free (globals.blend_outer_cachex1[i]);
      g_free (globals.blend_outer_cachex2[i]);
      g_free (globals.blend_outer_cachey1[i]);
      g_free (globals.blend_outer_cachey2[i]);
      g_free (globals.blend_inner_cachex1[i]);
      g_free (globals.blend_inner_cachex2[i]);
      g_free (globals.blend_inner_cachey1[i]);
      g_free (globals.blend_inner_cachey2[i]);
    }
}

static void
init_right_bump (gint width,
                 gint height)
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
                       globals.blend_outer_cachex1[RIGHT][i],
                       globals.blend_outer_cachey1[RIGHT][i]);
    }
  /* inside right bump */
  py[0] += blend_lines; py[1] += blend_lines; py[2] += blend_lines;
  px[3] -= blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      py[0]++; py[1]++; py[2]++; px[3]--;
      generate_bezier(px, py, steps,
                      globals.blend_inner_cachex1[RIGHT][i],
                      globals.blend_inner_cachey1[RIGHT][i]);
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
                      globals.blend_outer_cachex2[RIGHT][i],
                      globals.blend_outer_cachey2[RIGHT][i]);
    }
  /* inner right bump */
  py[1] -= blend_lines; py[2] -= blend_lines; py[3] -= blend_lines;
  px[0] -= blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      py[1]--; py[2]--; py[3]--; px[0]--;
      generate_bezier(px, py, steps,
                      globals.blend_inner_cachex2[RIGHT][i],
                      globals.blend_inner_cachey2[RIGHT][i]);
    }
}

static void
init_left_bump (gint width,
                gint height)
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
                      globals.blend_outer_cachex1[LEFT][i],
                      globals.blend_outer_cachey1[LEFT][i]);
    }
  /* inner left bump */
  py[0] += blend_lines; py[1] += blend_lines; py[2] += blend_lines;
  px[3] += blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      py[0]++; py[1]++; py[2]++; px[3]++;
      generate_bezier(px, py, steps,
                      globals.blend_inner_cachex1[LEFT][i],
                      globals.blend_inner_cachey1[LEFT][i]);
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
                      globals.blend_outer_cachex2[LEFT][i],
                      globals.blend_outer_cachey2[LEFT][i]);
    }
  /* inner left bump */
  py[1] -= blend_lines; py[2] -= blend_lines; py[3] -= blend_lines;
  px[0] += blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      py[1]--; py[2]--; py[3]--; px[0]++;
      generate_bezier(px, py, steps,
                      globals.blend_inner_cachex2[LEFT][i],
                      globals.blend_inner_cachey2[LEFT][i]);
    }
}

static void
init_up_bump (gint width,
              gint height)
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
                      globals.blend_outer_cachex1[UP][i],
                      globals.blend_outer_cachey1[UP][i]);
    }
  /* inner up bump */
  px[0] += blend_lines; px[1] += blend_lines; px[2] += blend_lines;
  py[3] += blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      px[0]++; px[1]++; px[2]++; py[3]++;
      generate_bezier(px, py, steps,
                      globals.blend_inner_cachex1[UP][i],
                      globals.blend_inner_cachey1[UP][i]);
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
                      globals.blend_outer_cachex2[UP][i],
                      globals.blend_outer_cachey2[UP][i]);
    }
  /* inner up bump */
  px[1] -= blend_lines; px[2] -= blend_lines; px[3] -= blend_lines;
  py[0] += blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      px[1]--; px[2]--; px[3]--; py[0]++;
      generate_bezier(px, py, steps,
                      globals.blend_inner_cachex2[UP][i],
                      globals.blend_inner_cachey2[UP][i]);
    }
}

static void
init_down_bump (gint width,
                gint height)
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
                      globals.blend_outer_cachex1[DOWN][i],
                      globals.blend_outer_cachey1[DOWN][i]);
    }
  /* inner down bump */
  px[0] += blend_lines; px[1] += blend_lines; px[2] += blend_lines;
  py[3] -= blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      px[0]++; px[1]++; px[2]++; py[3]--;
      generate_bezier(px, py, steps,
                      globals.blend_inner_cachex1[DOWN][i],
                      globals.blend_inner_cachey1[DOWN][i]);
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
                      globals.blend_outer_cachex2[DOWN][i],
                      globals.blend_outer_cachey2[DOWN][i]);
    }
  /* inner down bump */
  px[1] -= blend_lines; px[2] -= blend_lines; px[3] -= blend_lines;
  py[0] -= blend_lines;
  for (i = 0; i < blend_lines; i++)
    {
      px[1]--; px[2]--; px[3]--; py[0]--;
      generate_bezier(px, py, steps,
                      globals.blend_inner_cachex2[DOWN][i],
                      globals.blend_inner_cachey2[DOWN][i]);
    }
}

static void
generate_grid (gint  width,
               gint  height,
               gint  xtiles,
               gint  ytiles,
               gint *x,
               gint *y)
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
}

/* assumes RGB* */
/* assumes py[1] > py[0] and px[0] = px[1] */
static void
darken_vertical_line (guchar   *buffer,
                      gint      bufsize,
                      gint      width,
                      gint      bytes,
                      gint      px[2],
                      gint      py[2],
                      gdouble   delta)
{
  gint i;
  gint rowstride;
  gint index;
  gint length;
  gint temp;

  rowstride = bytes * width;
  index = px[0] * bytes + py[0] * rowstride;
  length = py[1] - py[0] + 1;

  for (i = 0; i < length; i++)
    {
      DARKEN_POINT (buffer, bufsize, index, delta, temp);
      index += rowstride;
    }
}

/* assumes RGB* */
/* assumes py[1] > py[0] and px[0] = px[1] */
static void
lighten_vertical_line (guchar   *buffer,
                       gint      bufsize,
                       gint      width,
                       gint      bytes,
                       gint      px[2],
                       gint      py[2],
                       gdouble   delta)
{
  gint i;
  gint rowstride;
  gint index;
  gint length;
  gint temp;

  rowstride = bytes * width;
  index = px[0] * bytes + py[0] * rowstride;
  length = py[1] - py[0] + 1;

  for (i = 0; i < length; i++)
    {
      LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
      index += rowstride;
    }
}

/* assumes RGB* */
/* assumes px[1] > px[0] and py[0] = py[1] */
static void
darken_horizontal_line (guchar   *buffer,
                        gint      bufsize,
                        gint      width,
                        gint      bytes,
                        gint      px[2],
                        gint      py[2],
                        gdouble   delta)
{
  gint i;
  gint rowstride;
  gint index;
  gint length;
  gint temp;

  rowstride = bytes * width;
  index = px[0] * bytes + py[0] * rowstride;
  length = px[1] - px[0] + 1;

  for (i = 0; i < length; i++)
    {
      DARKEN_POINT (buffer, bufsize, index, delta, temp);
      index += bytes;
    }
}

/* assumes RGB* */
/* assumes px[1] > px[0] and py[0] = py[1] */
static void
lighten_horizontal_line (guchar   *buffer,
                         gint      bufsize,
                         gint      width,
                         gint      bytes,
                         gint      px[2],
                         gint      py[2],
                         gdouble   delta)
{
  gint i;
  gint rowstride;
  gint index;
  gint length;
  gint temp;

  rowstride = bytes * width;
  index = px[0] * bytes + py[0] * rowstride;
  length = px[1] - px[0] + 1;

  for (i = 0; i < length; i++)
    {
      LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
      index += bytes;
    }
}

static void
darken_right_bump (guchar *buffer,
                   gint    bufsize,
                   gint    width,
                   gint    bytes,
                   gint    x_offset,
                   gint    curve_start_offset,
                   gint    steps,
                   gdouble delta,
                   gint    counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride;
  gint temp;
  gint j = counter;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = x_offset
        + globals.blend_inner_cachex1[RIGHT][j][i];
      y = curve_start_offset
        + globals.blend_inner_cachey1[RIGHT][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index1)
        {
          if (i < steps / 1.3)
            {
              LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
            }
          else
            {
              DARKEN_POINT (buffer, bufsize, index, delta, temp);
            }
          last_index1 = index;
        }

      x = x_offset
        + globals.blend_inner_cachex2[RIGHT][j][i];
      y = curve_start_offset
        + globals.blend_inner_cachey2[RIGHT][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index2)
        {
          DARKEN_POINT (buffer, bufsize, index, delta, temp);
          last_index2 = index;
        }
    }
}

static void
lighten_right_bump (guchar   *buffer,
                    gint      bufsize,
                    gint      width,
                    gint      bytes,
                    gint      x_offset,
                    gint      curve_start_offset,
                    gint      steps,
                    gdouble   delta,
                    gint      counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride;
  gint temp;
  gint j = counter;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = x_offset
        + globals.blend_outer_cachex1[RIGHT][j][i];
      y = curve_start_offset
        + globals.blend_outer_cachey1[RIGHT][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index1)
        {
          if (i < steps / 1.3)
            {
              DARKEN_POINT (buffer, bufsize, index, delta, temp);
            }
          else
            {
              LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
            }
          last_index1 = index;
        }

      x = x_offset
        + globals.blend_outer_cachex2[RIGHT][j][i];
      y = curve_start_offset
        + globals.blend_outer_cachey2[RIGHT][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index2)
        {
          LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
          last_index2 = index;
        }
    }
}

static void
darken_left_bump (guchar   *buffer,
                  gint      bufsize,
                  gint      width,
                  gint      bytes,
                  gint      x_offset,
                  gint      curve_start_offset,
                  gint      steps,
                  gdouble   delta,
                  gint      counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride;
  gint temp;
  gint j = counter;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = x_offset
        + globals.blend_outer_cachex1[LEFT][j][i];
      y = curve_start_offset
        + globals.blend_outer_cachey1[LEFT][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index1)
        {
          DARKEN_POINT (buffer, bufsize, index, delta, temp);
          last_index1 = index;
        }

      x = x_offset
        + globals.blend_outer_cachex2[LEFT][j][i];
      y = curve_start_offset
        + globals.blend_outer_cachey2[LEFT][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index2)
        {
          if (i < steps / 4)
            {
              DARKEN_POINT (buffer, bufsize, index, delta, temp);
            }
          else
            {
              LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
            }
          last_index2 = index;
        }
    }
}

static void
lighten_left_bump (guchar *buffer,
                   gint    bufsize,
                   gint    width,
                   gint    bytes,
                   gint    x_offset,
                   gint    curve_start_offset,
                   gint    steps,
                   gdouble delta,
                   gint    counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride;
  gint temp;
  gint j = counter;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = x_offset
        + globals.blend_inner_cachex1[LEFT][j][i];
      y = curve_start_offset
        + globals.blend_inner_cachey1[LEFT][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index1)
        {
          LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
          last_index1 = index;
        }

      x = x_offset
        + globals.blend_inner_cachex2[LEFT][j][i];
      y = curve_start_offset
        + globals.blend_inner_cachey2[LEFT][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index2)
        {
          if (i < steps / 4)
            {
              LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
            }
          else
            {
              DARKEN_POINT (buffer, bufsize, index, delta, temp);
            }
          last_index2 = index;
        }
    }
}

static void
darken_up_bump (guchar   *buffer,
                gint      bufsize,
                gint      width,
                gint      bytes,
                gint      y_offset,
                gint      curve_start_offset,
                gint      steps,
                gdouble   delta,
                gint      counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride;
  gint temp;
  gint j = counter;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = curve_start_offset
        + globals.blend_outer_cachex1[UP][j][i];
      y = y_offset
        + globals.blend_outer_cachey1[UP][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index1)
        {
          DARKEN_POINT (buffer, bufsize, index, delta, temp);
          last_index1 = index;
        }

      x = curve_start_offset
        + globals.blend_outer_cachex2[UP][j][i];
      y = y_offset
        + globals.blend_outer_cachey2[UP][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index2)
        {
          if (i < steps / 4)
            {
              DARKEN_POINT (buffer, bufsize, index, delta, temp);
            }
          else
            {
              LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
            }
          last_index2 = index;
        }
    }
}

static void
lighten_up_bump (guchar   *buffer,
                 gint      bufsize,
                 gint      width,
                 gint      bytes,
                 gint      y_offset,
                 gint      curve_start_offset,
                 gint      steps,
                 gdouble   delta,
                 gint      counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride;
  gint temp;
  gint j = counter;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = curve_start_offset
        + globals.blend_inner_cachex1[UP][j][i];
      y = y_offset
        + globals.blend_inner_cachey1[UP][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index1)
        {
          LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
          last_index1 = index;
        }

      x = curve_start_offset
        + globals.blend_inner_cachex2[UP][j][i];
      y = y_offset
        + globals.blend_inner_cachey2[UP][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index2)
        {
          if (i < steps / 4)
            {
              LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
            }
          else
            {
              DARKEN_POINT (buffer, bufsize, index, delta, temp);
            }
          last_index2 = index;
        }
    }
}

static void
darken_down_bump (guchar   *buffer,
                  gint      bufsize,
                  gint      width,
                  gint      bytes,
                  gint      y_offset,
                  gint      curve_start_offset,
                  gint      steps,
                  gdouble   delta,
                  gint      counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride;
  gint temp;
  gint j = counter;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = curve_start_offset
        + globals.blend_inner_cachex1[DOWN][j][i];
      y = y_offset
        + globals.blend_inner_cachey1[DOWN][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index1)
        {
          if (i < steps / 1.2)
            {
              LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
            }
          else
            {
              DARKEN_POINT (buffer, bufsize, index, delta, temp);
            }
          last_index1 = index;
        }

      x = curve_start_offset
        + globals.blend_inner_cachex2[DOWN][j][i];
      y = y_offset
        + globals.blend_inner_cachey2[DOWN][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index2)
        {
          DARKEN_POINT (buffer, bufsize, index, delta, temp);
          last_index2 = index;
        }
    }
}

static void
lighten_down_bump (guchar   *buffer,
                   gint      bufsize,
                   gint      width,
                   gint      bytes,
                   gint      y_offset,
                   gint      curve_start_offset,
                   gint      steps,
                   gdouble   delta,
                   gint      counter)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index1 = -1;
  gint last_index2 = -1;
  gint rowstride;
  gint temp;
  gint j = counter;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = curve_start_offset
        + globals.blend_outer_cachex1[DOWN][j][i];
      y = y_offset
        + globals.blend_outer_cachey1[DOWN][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index1)
        {
          if (i < steps / 1.2)
            {
              DARKEN_POINT (buffer, bufsize, index, delta, temp);
            }
          else
            {
              LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
            }
          last_index1 = index;
        }

      x = curve_start_offset
        + globals.blend_outer_cachex2[DOWN][j][i];
      y = y_offset
        + globals.blend_outer_cachey2[DOWN][j][i];
      index = y * rowstride + x * bytes;
      if (index != last_index2)
        {
          LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
          last_index2 = index;
        }
    }
}

static void
draw_bezier_line (guchar   *buffer,
                  gint      bufsize,
                  gint      width,
                  gint      bytes,
                  gint      steps,
                  gint     *cx,
                  gint     *cy)
{
  gint i;
  gint x, y;
  gint index;
  gint rowstride;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = cx[i];
      y = cy[i];
      index = y * rowstride + x * bytes;
      DRAW_POINT (buffer, bufsize, index);
    }
}

static void
darken_bezier_line (guchar   *buffer,
                    gint      bufsize,
                    gint      width,
                    gint      bytes,
                    gint      x_offset,
                    gint      y_offset,
                    gint      steps,
                    gint     *cx,
                    gint     *cy,
                    gdouble   delta)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index = -1;
  gint rowstride;
  gint temp;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = cx[i] + x_offset;
      y = cy[i] + y_offset;
      index = y * rowstride + x * bytes;
      if (index != last_index)
        {
          DARKEN_POINT (buffer, bufsize, index, delta, temp);
          last_index = index;
        }
    }
}

static void
lighten_bezier_line (guchar   *buffer,
                     gint      bufsize,
                     gint      width,
                     gint      bytes,
                     gint      x_offset,
                     gint      y_offset,
                     gint      steps,
                     gint     *cx,
                     gint     *cy,
                     gdouble   delta)
{
  gint i;
  gint x, y;
  gint index;
  gint last_index = -1;
  gint rowstride;
  gint temp;

  rowstride = bytes * width;

  for (i = 0; i < steps; i++)
    {
      x = cx[i] + x_offset;
      y = cy[i] + y_offset;
      index = y * rowstride + x * bytes;
      if (index != last_index)
        {
          LIGHTEN_POINT (buffer, bufsize, index, delta, temp);
          last_index = index;
        }
    }
}

static void
draw_bezier_vertical_border (guchar   *buffer,
                             gint      bufsize,
                             gint      width,
                             gint      height,
                             gint      bytes,
                             gint      x_offset,
                             gint      xtiles,
                             gint      ytiles,
                             gint      blend_lines,
                             gdouble   blend_amount,
                             gint      steps)
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
  gint *cachex, *cachey;

  cachex = g_new (gint, steps);
  cachey = g_new (gint, steps);

  for (i = 0; i < ytiles; i++)
    {
      right = g_random_int_range (0, 2);

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
      generate_bezier (px, py, steps, cachex, cachey);
      draw_bezier_line (buffer, bufsize, width, bytes, steps, cachex, cachey);
      delta = blend_amount;
      for (j = 0; j < blend_lines; j++)
        {
          px[0] =  -j - 1;
          darken_bezier_line (buffer, bufsize, width, bytes, px[0], 0,
                              steps, cachex, cachey, delta);
          px[0] =  j + 1;
          lighten_bezier_line (buffer, bufsize, width, bytes, px[0], 0,
                               steps, cachex, cachey, delta);
          delta -= sigma;
        }
      if (right)
        {
          draw_right_bump (buffer, bufsize, width, bytes, x_offset,
                           y_offset + curve_start_offset,
                           globals.steps[RIGHT]);
          delta = blend_amount;
          for (j = 0; j < blend_lines; j++)
            {
              /* use to be -j -1 */
              darken_right_bump (buffer, bufsize, width, bytes, x_offset,
                                 y_offset + curve_start_offset,
                                 globals.steps[RIGHT], delta, j);
              /* use to be +j + 1 */
              lighten_right_bump (buffer, bufsize, width, bytes, x_offset,
                                  y_offset + curve_start_offset,
                                  globals.steps[RIGHT], delta, j);
              delta -= sigma;
            }
        }
      else
        {
          draw_left_bump (buffer, bufsize, width, bytes, x_offset,
                          y_offset + curve_start_offset,
                          globals.steps[LEFT]);
          delta = blend_amount;
          for (j = 0; j < blend_lines; j++)
            {
              /* use to be -j -1 */
              darken_left_bump (buffer, bufsize, width, bytes, x_offset,
                                y_offset + curve_start_offset,
                                globals.steps[LEFT], delta, j);
              /* use to be -j - 1 */
              lighten_left_bump (buffer, bufsize, width, bytes, x_offset,
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
      generate_bezier (px, py, steps, cachex, cachey);
      draw_bezier_line (buffer, bufsize, width, bytes, steps, cachex, cachey);
      delta = blend_amount;
      for (j = 0; j < blend_lines; j++)
        {
          px[0] =  -j - 1;
          darken_bezier_line (buffer, bufsize, width, bytes, px[0], 0,
                              steps, cachex, cachey, delta);
          px[0] =  j + 1;
          lighten_bezier_line (buffer, bufsize, width, bytes, px[0], 0,
                               steps, cachex, cachey, delta);
          delta -= sigma;
        }
      y_offset = globals.gridy[i];
    }  /* for */
  g_free(cachex);
  g_free(cachey);
}

static void
draw_bezier_horizontal_border (guchar   *buffer,
                               gint      bufsize,
                               gint      width,
                               gint      height,
                               gint      bytes,
                               gint      y_offset,
                               gint      xtiles,
                               gint      ytiles,
                               gint      blend_lines,
                               gdouble   blend_amount,
                               gint      steps)
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
  gint *cachex, *cachey;

  cachex = g_new (gint, steps);
  cachey = g_new (gint, steps);

  for (i = 0; i < xtiles; i++)
    {
      up = g_random_int_range (0, 2);

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
      generate_bezier (px, py, steps, cachex, cachey);
      draw_bezier_line (buffer, bufsize, width, bytes, steps, cachex, cachey);
      delta = blend_amount;
      for (j = 0; j < blend_lines; j++)
        {
          py[0] = -j - 1;
          darken_bezier_line (buffer, bufsize, width, bytes, 0, py[0],
                              steps, cachex, cachey, delta);
          py[0] = j + 1;
          lighten_bezier_line (buffer, bufsize, width, bytes, 0, py[0],
                               steps, cachex, cachey, delta);
          delta -= sigma;
        }
      /* bumps */
      if (up)
        {
          draw_up_bump (buffer, bufsize, width, bytes, y_offset,
                        x_offset + curve_start_offset,
                        globals.steps[UP]);
          delta = blend_amount;
          for (j = 0; j < blend_lines; j++)
            {
              /* use to be -j -1 */
              darken_up_bump (buffer, bufsize, width, bytes, y_offset,
                              x_offset + curve_start_offset,
                              globals.steps[UP], delta, j);
              /* use to be +j + 1 */
              lighten_up_bump (buffer, bufsize, width, bytes, y_offset,
                               x_offset + curve_start_offset,
                               globals.steps[UP], delta, j);
              delta -= sigma;
            }
        }
      else
        {
          draw_down_bump (buffer, bufsize, width, bytes, y_offset,
                          x_offset + curve_start_offset,
                          globals.steps[DOWN]);
          delta = blend_amount;
          for (j = 0; j < blend_lines; j++)
            {
              /* use to be +j + 1 */
              darken_down_bump (buffer, bufsize, width, bytes, y_offset,
                                x_offset + curve_start_offset,
                                globals.steps[DOWN], delta, j);
              /* use to be -j -1 */
              lighten_down_bump (buffer, bufsize, width, bytes, y_offset,
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
      generate_bezier (px, py, steps, cachex, cachey);
      draw_bezier_line (buffer, bufsize, width, bytes, steps, cachex, cachey);
      delta = blend_amount;
      for (j = 0; j < blend_lines; j++)
        {
          py[0] =  -j - 1;
          darken_bezier_line (buffer, bufsize, width, bytes, 0, py[0],
                              steps, cachex, cachey, delta);
          py[0] =  j + 1;
          lighten_bezier_line (buffer, bufsize, width, bytes, 0, py[0],
                               steps, cachex, cachey, delta);
          delta -= sigma;
        }
      x_offset = globals.gridx[i];
    }  /* for */
  g_free(cachex);
  g_free(cachey);
}

static void
check_config (gint width,
              gint height)
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
}

/********************************************************
  GUI
********************************************************/

static gboolean
jigsaw_dialog (GimpDrawable *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkSizeGroup  *group;
  GtkWidget     *frame;
  GtkWidget     *rbutton1;
  GtkWidget     *rbutton2;
  GtkWidget     *table;
  GtkAdjustment *adj;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Jigsaw"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_aspect_preview_new_from_drawable_id (drawable->drawable_id);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (jigsaw),
                            drawable);

  frame = gimp_frame_new (_("Number of Tiles"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* xtiles */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Horizontal:"), SCALE_WIDTH, 0,
                              config.x, MIN_XTILES, MAX_XTILES, 1.0, 4.0, 0,
                              TRUE, 0, 0,
                              _("Number of pieces going across"), NULL);

  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));
  g_object_unref (group);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &config.x);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* ytiles */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("_Vertical:"), SCALE_WIDTH, 0,
                              config.y, MIN_YTILES, MAX_YTILES, 1.0, 4.0, 0,
                              TRUE, 0, 0,
                              _("Number of pieces going down"), NULL);

  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &config.y);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  frame = gimp_frame_new (_("Bevel Edges"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* number of blending lines */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Bevel width:"), SCALE_WIDTH, 4,
                              config.blend_lines,
                              MIN_BLEND_LINES, MAX_BLEND_LINES, 1.0, 2.0, 0,
                              TRUE, 0, 0,
                              _("Degree of slope of each piece's edge"), NULL);

  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &config.blend_lines);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* blending amount */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("H_ighlight:"), SCALE_WIDTH, 4,
                              config.blend_amount,
                              MIN_BLEND_AMOUNT, MAX_BLEND_AMOUNT, 0.05, 0.1, 2,
                              TRUE, 0, 0,
                              _("The amount of highlighting on the edges "
                                "of each piece"), NULL);

  gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &config.blend_amount);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  /* frame for primitive radio buttons */

  frame = gimp_int_radio_group_new (TRUE, _("Jigsaw Style"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &config.style, config.style,

                                    _("_Square"), BEZIER_1, &rbutton1,
                                    _("C_urved"), BEZIER_2, &rbutton2,

                                    NULL);

  gimp_help_set_help_data (rbutton1, _("Each piece has straight sides"), NULL);
  gimp_help_set_help_data (rbutton2, _("Each piece has curved sides"),   NULL);
  g_signal_connect_swapped (rbutton1, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (rbutton2, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
