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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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


typedef struct globals_tag globals_t;

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


typedef struct _Jigsaw      Jigsaw;
typedef struct _JigsawClass JigsawClass;

struct _Jigsaw
{
  GimpPlugIn parent_instance;
};

struct _JigsawClass
{
  GimpPlugInClass parent_class;
};


#define JIGSAW_TYPE  (jigsaw_get_type ())
#define JIGSAW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), JIGSAW_TYPE, Jigsaw))

GType                   jigsaw_get_type         (void) G_GNUC_CONST;

static GList          * jigsaw_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * jigsaw_create_procedure (GimpPlugIn           *plug_in,
                                                 const gchar          *name);

static GimpValueArray * jigsaw_run              (GimpProcedure        *procedure,
                                                 GimpRunMode           run_mode,
                                                 GimpImage            *image,
                                                 gint                  n_drawables,
                                                 GimpDrawable        **drawables,
                                                 GimpProcedureConfig  *config,
                                                 gpointer              run_data);

static void     jigsaw                          (GObject              *config,
                                                 GimpDrawable         *drawable,
                                                 GimpPreview          *preview);
static void     jigsaw_preview                  (GtkWidget            *widget,
                                                 GObject              *config);

static gboolean jigsaw_dialog                    (GimpProcedure       *procedure,
                                                  GObject             *config,
                                                  GimpDrawable        *drawable);

static void     draw_jigsaw        (GObject   *config,
                                    guchar    *buffer,
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
                                    gint px[2], gint py[2], gdouble delta);
static void lighten_vertical_line  (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint px[2], gint py[2], gdouble delta);
static void darken_horizontal_line (guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint px[2], gint py[2], gdouble delta);
static void lighten_horizontal_line(guchar *buffer, gint bufsize,
                                    gint width, gint bytes,
                                    gint px[2], gint py[2], gdouble delta);
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

static void malloc_cache           (gint blend_lines);
static void free_cache             (gint blend_lines);

static void init_right_bump        (gint x,
                                    gint y,
                                    gint blend_lines,
                                    gint width,
                                    gint height);
static void init_left_bump         (gint x,
                                    gint y,
                                    gint blend_lines,
                                    gint width,
                                    gint height);
static void init_up_bump           (gint x,
                                    gint y,
                                    gint blend_lines,
                                    gint width,
                                    gint height);
static void init_down_bump         (gint x,
                                    gint y,
                                    gint blend_lines,
                                    gint width,
                                    gint height);
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


G_DEFINE_TYPE (Jigsaw, jigsaw, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (JIGSAW_TYPE)
DEFINE_STD_SET_I18N


static globals_t globals;


static void
jigsaw_class_init (JigsawClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = jigsaw_query_procedures;
  plug_in_class->create_procedure = jigsaw_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
jigsaw_init (Jigsaw *jigsaw)
{
}

static GList *
jigsaw_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
jigsaw_create_procedure (GimpPlugIn  *plug_in,
                               const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            jigsaw_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Jigsaw..."));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Render/Pattern");

      gimp_procedure_set_documentation (procedure,
                                        _("Add a jigsaw-puzzle pattern "
                                          "to the image"),
                                        _("Jigsaw puzzle look"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Nigel Wetten",
                                      "Nigel Wetten",
                                      "May 2000");

      gimp_procedure_add_int_argument (procedure, "x",
                                       _("_Horizontal"),
                                       _("Number of pieces going across"),
                                       MIN_XTILES, MAX_XTILES, 5,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "y",
                                       _("_Vertical"),
                                       _("Number of pieces going down"),
                                       MIN_YTILES, MAX_YTILES, 5,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "style",
                                          _("_Jigsaw Style"),
                                          _("The style/shape of the jigsaw puzzle"),
                                          gimp_choice_new_with_values ("square", BEZIER_1, _("Square"), NULL,
                                                                       "curved", BEZIER_2, _("Curved"), NULL,
                                                                       NULL),
                                          "square",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "blend-lines",
                                       _("_Blend width"),
                                       _("Degree of slope of each piece's edge"),
                                       MIN_BLEND_LINES, MAX_BLEND_LINES, 3,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "blend-amount",
                                          _("Hi_ghlight"),
                                          _("The amount of highlighting on the edges "
                                            "of each piece"),
                                          MIN_BLEND_AMOUNT, MAX_BLEND_AMOUNT, 0.5,
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
jigsaw_run (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            gint                  n_drawables,
            GimpDrawable        **drawables,
            GimpProcedureConfig  *config,
            gpointer              run_data)
{
  GimpDrawable *drawable;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  if (run_mode == GIMP_RUN_INTERACTIVE && ! jigsaw_dialog (procedure, G_OBJECT (config), drawable))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  gimp_progress_init (_("Assembling jigsaw"));

  jigsaw (G_OBJECT (config), drawable, NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static void
jigsaw (GObject      *config,
        GimpDrawable *drawable,
        GimpPreview  *preview)
{
  GeglBuffer *gegl_buffer = NULL;
  const Babl *format      = NULL;
  guchar     *buffer;
  gint        width;
  gint        height;
  gint        bytes;
  gsize       buffer_size;
  gint        x;
  gint        y;
  gint        blend_lines;

  g_object_get (config,
                "x",           &x,
                "y",           &y,
                "blend-lines", &blend_lines,
                NULL);

  if (preview)
    {
      GBytes *buffer_bytes;

      gimp_preview_get_size (preview, &width, &height);

      buffer_bytes = gimp_drawable_get_thumbnail_data (drawable,
                                                       width, height,
                                                       &width, &height, &bytes);

      buffer = g_bytes_unref_to_data (buffer_bytes, &buffer_size);
    }
  else
    {
      gegl_buffer = gimp_drawable_get_buffer (drawable);

      width  = gimp_drawable_get_width  (drawable);
      height = gimp_drawable_get_height (drawable);

      if (gimp_drawable_has_alpha (drawable))
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");

      bytes = babl_format_get_bytes_per_pixel (format);

      /* setup image buffer */
      buffer_size = (gsize) bytes * width * height;
      buffer = g_new (guchar, buffer_size);

      gegl_buffer_get (gegl_buffer, GEGL_RECTANGLE (0, 0, width, height), 1.0,
                       format, buffer,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      g_object_unref (gegl_buffer);
    }

  globals.steps[LEFT] = globals.steps[RIGHT] = globals.steps[UP]
    = globals.steps[DOWN] = (x < y) ?
    (width / x * 2) : (height / y * 2);

  malloc_cache (blend_lines);
  draw_jigsaw (config, buffer, buffer_size, width, height, bytes,
               preview != NULL);
  free_cache (blend_lines);

  /* cleanup */
  if (preview)
    {
      gimp_preview_draw_buffer (preview, buffer, width * bytes);

      gimp_preview_set_bounds (preview, 0, 0, width, height);
      gimp_preview_set_size (preview, width, height);
    }
  else
    {
      gegl_buffer = gimp_drawable_get_shadow_buffer (drawable);

      gegl_buffer_set (gegl_buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                       format, buffer,
                       GEGL_AUTO_ROWSTRIDE);
      g_object_unref (gegl_buffer);

      gimp_drawable_merge_shadow (drawable, TRUE);
      gimp_drawable_update (drawable, 0, 0, width, height);
    }

  g_free (buffer);
}

static void
jigsaw_preview (GtkWidget *widget,
                GObject   *config)
{
  GimpDrawablePreview  *preview;
  GimpDrawable         *drawable;

  preview = GIMP_DRAWABLE_PREVIEW (widget);
  drawable = gimp_drawable_preview_get_drawable (preview);

  jigsaw (config, drawable, GIMP_PREVIEW (widget));
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
draw_jigsaw (GObject  *config,
             guchar   *buffer,
             gint      bufsize,
             gint      width,
             gint      height,
             gint      bytes,
             gboolean  preview_mode)
{
  gint i;
  gint *x, *y;
  gint xtiles = 5;
  gint ytiles = 5;
  gint xlines = 4;
  gint ylines = 4;
  gint blend_lines = 3;
  gdouble blend_amount = 0.5;
  gint steps = globals.steps[RIGHT];
  style_t style = BEZIER_1;
  gint progress_total = xlines + ylines - 1;

  g_return_if_fail (buffer != NULL);

  g_object_get (config,
                "x",            &xtiles,
                "y",            &ytiles,
                "blend-lines",  &blend_lines,
                "blend-amount", &blend_amount,
                NULL);
  style = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                               "style");

  xlines = xtiles - 1;
  ylines = ytiles - 1;
  progress_total = xlines + ylines - 1;

  globals.gridx = g_new (gint, xtiles);
  globals.gridy = g_new (gint, ytiles);
  x = globals.gridx;
  y = globals.gridy;

  generate_grid (width, height, xtiles, ytiles, globals.gridx, globals.gridy);

  init_right_bump (xtiles, ytiles, blend_lines, width, height);
  init_left_bump  (xtiles, ytiles, blend_lines, width, height);
  init_up_bump    (xtiles, ytiles, blend_lines, width, height);
  init_down_bump  (xtiles, ytiles, blend_lines, width, height);

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
      g_printerr (_("draw_jigsaw: bad style\n"));
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
malloc_cache (gint blend_lines)
{
  gint i, j;

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
free_cache (gint blend_lines)
{
  gint i, j;

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
init_right_bump (gint x,
                 gint y,
                 gint blend_lines,
                 gint width,
                 gint height)
{
  gint i;
  gint xtiles = x;
  gint ytiles = y;
  gint steps = globals.steps[RIGHT];
  gint px[4], py[4];
  gint x_offset = 0;
  gint tile_width =  width / xtiles;
  gint tile_height = height/ ytiles;
  gint tile_height_eighth = tile_height / 8;
  gint curve_start_offset = 0;
  gint curve_end_offset = curve_start_offset + 2 * tile_height_eighth;

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
init_left_bump (gint x,
                gint y,
                gint blend_lines,
                gint width,
                gint height)
{
  gint i;
  gint xtiles = x;
  gint ytiles = y;
  gint steps = globals.steps[LEFT];
  gint px[4], py[4];
  gint x_offset = 0;
  gint tile_width = width / xtiles;
  gint tile_height = height / ytiles;
  gint tile_height_eighth = tile_height / 8;
  gint curve_start_offset = 0;
  gint curve_end_offset = curve_start_offset + 2 * tile_height_eighth;

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
init_up_bump (gint x,
              gint y,
              gint blend_lines,
              gint width,
              gint height)
{
  gint i;
  gint xtiles = x;
  gint ytiles = y;
  gint steps = globals.steps[UP];
  gint px[4], py[4];
  gint y_offset = 0;
  gint tile_width =  width / xtiles;
  gint tile_height = height/ ytiles;
  gint tile_width_eighth = tile_width / 8;
  gint curve_start_offset = 0;
  gint curve_end_offset = curve_start_offset + 2 * tile_width_eighth;

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
init_down_bump (gint x,
                gint y,
                gint blend_lines,
                gint width,
                gint height)
{
  gint i;
  gint xtiles = x;
  gint ytiles = y;
  gint steps = globals.steps[DOWN];
  gint px[4], py[4];
  gint y_offset = 0;
  gint tile_width =  width / xtiles;
  gint tile_height = height/ ytiles;
  gint tile_width_eighth = tile_width / 8;
  gint curve_start_offset = 0;
  gint curve_end_offset = curve_start_offset + 2 * tile_width_eighth;

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

/********************************************************
  GUI
********************************************************/

static gboolean
jigsaw_dialog (GimpProcedure *procedure,
               GObject       *config,
               GimpDrawable  *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *preview;
  GtkWidget     *frame;
  GtkWidget     *scale;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Jigsaw"));

  /* xtiles */
  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "x", 1.0);

  /* ytiles */
  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "y", 1.0);
  gtk_widget_set_margin_bottom (scale, 12);

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "num-tiles-label", _("Number of Tiles"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "num-tiles-vbox", "x", "y", NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "num-tiles-frame", "num-tiles-label",
                                    FALSE, "num-tiles-vbox");

  /* number of blending lines */
  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "blend-lines", 1.0);

  /* blending amount */
  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "blend-amount", 1.0);
  gtk_widget_set_margin_bottom (scale, 12);

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "bevel-label", _("Bevel Edges"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "bevel-vbox", "blend-lines", "blend-amount",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "bevel-frame", "bevel-label", FALSE,
                                    "bevel-vbox");

  /* frame for primitive radio buttons */
  frame = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                            "style", GIMP_TYPE_INT_RADIO_FRAME);
  gtk_widget_set_margin_bottom (frame, 12);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "jigsaw-vbox", "num-tiles-frame",
                                  "bevel-frame", "style", NULL);

  preview = gimp_procedure_dialog_get_drawable_preview (GIMP_PROCEDURE_DIALOG (dialog),
                                                        "preview", drawable);
  gtk_widget_set_margin_bottom (preview, 12);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (jigsaw_preview),
                    config);

  g_signal_connect_swapped (config, "notify",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "preview", "jigsaw-vbox", NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
