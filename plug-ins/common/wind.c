/*
 * wind 1.1.0 - a plug-in for GIMP
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
 * Version: 1.0.0
 *
 * Version: 1.1.0
 * May 2000 tim copperfield [timecop@japan.co.jp]
 *
 * Added dynamic preview.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-wind"
#define PLUG_IN_BINARY "wind"

#define COMPARE_WIDTH    3

#define SCALE_WIDTH    200
#define MIN_THRESHOLD    0
#define MAX_THRESHOLD   50
#define MIN_STRENGTH     1
#define MAX_STRENGTH   100

typedef enum
{
  LEFT,
  RIGHT
} direction_t;

typedef enum
{
  RENDER_WIND,
  RENDER_BLAST
} algorithm_t;

typedef enum
{
  BOTH,
  LEADING,
  TRAILING
} edge_t;


static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

static gint dialog_box       (GimpDrawable *drawable);

static gint render_effect    (GimpDrawable *drawable,
                              GimpPreview  *preview);
static void render_wind      (GimpDrawable *drawable,
                              gint          threshold,
                              gint          strength,
                              direction_t   direction,
                              edge_t        edge,
                              GimpPreview  *preview);
static void render_blast     (GimpDrawable *drawable,
                              gint          threshold,
                              gint          strength,
                              direction_t   direction,
                              edge_t        edge,
                              GimpPreview  *preview);
static gint render_blast_row (guchar       *buffer,
                              gint          bytes,
                              gint          lpi,
                              gint          threshold,
                              gint          strength,
                              edge_t        edge);
static void render_wind_row  (guchar       *sb,
                              gint          bytes,
                              gint          lpi,
                              gint          threshold,
                              gint          strength,
                              edge_t        edge);

static void get_derivative         (guchar   *pixel_R1,
                                    guchar   *pixel_R2,
                                    edge_t    edge,
                                    gboolean  has_alpha,
                                    gint     *derivative_R,
                                    gint     *derivative_G,
                                    gint     *derivative_B,
                                    gint     *derivative_A);
static gboolean threshold_exceeded (guchar   *pixel_R1,
                                    guchar   *pixel_R2,
                                    edge_t    edge,
                                    gint      threshold,
                                    gboolean  has_alpha);
static void reverse_buffer         (guchar   *buffer,
                                    gint      length,
                                    gint      bytes);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};


/*********************
  Globals
  *******************/

struct config_tag
{
  gint        threshold;      /* derivative comparison for edge detection */
  direction_t direction;      /* of wind, LEFT or RIGHT */
  gint        strength;       /* how many pixels to bleed */
  algorithm_t alg;            /* which algorithm */
  edge_t      edge;           /* controls abs, negation of derivative */
  gboolean    update_preview; /* should the preview be active? */
};

typedef struct config_tag config_t;
config_t config =
{
  10,          /* threshold for derivative edge detection */
  LEFT,        /* bleed to the right */
  10,          /* how many pixels to bleed */
  RENDER_WIND, /* default algorithm */
  LEADING,     /* abs(derivative); */
  TRUE         /* update_preview */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable" },
    { GIMP_PDB_INT32,    "threshold", "Controls where blending will be done >= 0" },
    { GIMP_PDB_INT32,    "direction", "Left or Right: 0 or 1" },
    { GIMP_PDB_INT32,    "strength",  "Controls the extent of the blending > 1" },
    { GIMP_PDB_INT32,    "algorithm", "WIND, BLAST" },
    { GIMP_PDB_INT32,    "edge",      "LEADING, TRAILING, or BOTH" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Smear image to give windblown effect"),
                          "Renders a wind effect.",
                          "Nigel Wetten",
                          "Nigel Wetten",
                          "May 2000",
                          N_("Wi_nd..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Distorts");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

  INIT_I18N ();

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 8)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          config.threshold = param[3].data.d_int32;
          config.direction = param[4].data.d_int32;
          config.strength = param[5].data.d_int32;
          config.alg = param[6].data.d_int32;
          config.edge = param[7].data.d_int32;

          if (render_effect (drawable, NULL) == -1)
            status = GIMP_PDB_EXECUTION_ERROR;
        }
      break;

    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &config);
      if (! dialog_box (drawable))
        {
          status = GIMP_PDB_CANCEL;
          break;
        }
      if (render_effect(drawable, NULL) == -1)
        {
          status = GIMP_PDB_CALLING_ERROR;
          break;
        }
      gimp_set_data (PLUG_IN_PROC, &config, sizeof (config_t));
      gimp_displays_flush ();
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &config);
      if (render_effect (drawable, NULL) == -1)
        {
          status = GIMP_PDB_EXECUTION_ERROR;
          gimp_message ("An execution error occurred.");
        }
      else
        {
          gimp_displays_flush ();
        }
    }

  gimp_drawable_detach (drawable);

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static gint
render_effect (GimpDrawable *drawable,
               GimpPreview  *preview)
{
  if (config.alg == RENDER_WIND)
    {
      render_wind (drawable, config.threshold, config.strength,
                   config.direction, config.edge, preview);
    }
  else if (config.alg == RENDER_BLAST)
    {
      render_blast (drawable, config.threshold, config.strength,
                    config.direction, config.edge, preview);
    }
  return 0;
}

static void
render_blast (GimpDrawable *drawable,
              gint          threshold,
              gint          strength,
              direction_t   direction,
              edge_t        edge,
              GimpPreview  *preview)
{
  gint          x1, x2, y1, y2;
  gint          width;
  gint          height;
  gint          bytes = drawable->bpp;
  guchar       *buffer, *preview_buffer = NULL;
  GimpPixelRgn  src_region, dest_region;
  gint          row;
  gint          row_stride;
  gint          marker = 0;
  gint          lpi;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);

      x2 = x1 + width;
      y2 = y1 + height;

      preview_buffer = g_new (guchar, width * height * bytes);
    }
  else
    {
      if (gimp_drawable_mask_intersect (drawable->drawable_id,
                                        &x1, &y1, &x2, &y2))
        {
          gimp_progress_init (_("Rendering blast"));

          width = x2 - x1;
          height = y2 - y1;

          gimp_pixel_rgn_init (&dest_region, drawable,
                               x1, y1, width, height, TRUE, TRUE);
        }
      else
        return;
    }

  gimp_pixel_rgn_init (&src_region,  drawable,
                       x1, y1, width, height, FALSE, FALSE);
  row_stride = width * bytes;
  lpi = row_stride - bytes;

  buffer = g_new (guchar, row_stride);

  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row (&src_region, buffer, x1, row, width);

      if (direction == RIGHT)
        {
          reverse_buffer (buffer, row_stride, bytes);
        }

      marker = render_blast_row (buffer, bytes, lpi, threshold, strength, edge);

      if (direction == RIGHT)
        {
          reverse_buffer (buffer, row_stride, bytes);
        }

      if (preview)
        {
          memcpy (preview_buffer + (row - y1) * row_stride,
                  buffer,
                  row_stride);
        }
      else
        {
          gimp_pixel_rgn_set_row (&dest_region, buffer, x1, row, width);
          gimp_progress_update ((double) (row - y1)/ (double) (height));
        }

      if (marker)
        {
          gint j, limit;

          limit = 1 + g_random_int_range (0, 2);
          for (j = 0; (j < limit) && (row < y2); j++)
            {
              row++;
              if (row < y2)
                {
                  gimp_pixel_rgn_get_row (&src_region,
                                          buffer, x1, row, width);
                  if (preview)
                    {
                      memcpy (preview_buffer + (row - y1) * row_stride,
                              buffer,
                              row_stride);
                    }
                  else
                    {
                      gimp_pixel_rgn_set_row (&dest_region,
                                              buffer, x1, row, width);
                    }
                }
            }
          marker = 0;
        }
    }

  g_free(buffer);

  /*  update the region  */
  if (preview)
    {
      gimp_preview_draw_buffer (preview, preview_buffer, width * bytes);
      g_free (preview_buffer);
    }
  else
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
    }
}

static void
render_wind (GimpDrawable *drawable,
             gint          threshold,
             gint          strength,
             direction_t   direction,
             edge_t        edge,
             GimpPreview  *preview)
{
  GimpPixelRgn  src_region, dest_region;
  gint          width;
  gint          height;
  gint          bytes;
  gint          row_stride;
  gint          comp_stride;
  gint          row;
  guchar       *sb, *preview_buffer = NULL;
  gint          lpi;
  gint          x1, y1, x2, y2;

  bytes = drawable->bpp;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);

      x2 = x1 + width;
      y2 = y1 + height;

      preview_buffer = g_new (guchar, width * height * bytes);
    }
  else
    {
      if (gimp_drawable_mask_intersect (drawable->drawable_id, &x1, &y1, &x2, &y2))
        {
          gimp_progress_init (_("Rendering wind"));

          width = x2 - x1;
          height = y2 - y1;

          gimp_pixel_rgn_init (&dest_region, drawable,
                               x1, y1, width, height, TRUE, TRUE);
        }
      else
        return;
    }

  gimp_pixel_rgn_init (&src_region, drawable,
                       x1, y1, width, height, FALSE, FALSE);
  row_stride = width * bytes;
  comp_stride = bytes * COMPARE_WIDTH;
  lpi = row_stride - comp_stride;

  sb = g_new (guchar, row_stride);

  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row (&src_region, sb, x1, row, width);

      if (direction == RIGHT)
        reverse_buffer (sb, row_stride, bytes);

      render_wind_row (sb, bytes, lpi, threshold, strength, edge);

      if (direction == RIGHT)
        reverse_buffer(sb, row_stride, bytes);

      if (preview)
        {
          memcpy (preview_buffer + (row - y1) * row_stride, sb, row_stride);
        }
      else
        {
          gimp_pixel_rgn_set_row (&dest_region, sb, x1, row, width);
          gimp_progress_update ((double) (row - y1)/ (double) (height));
        }
    }

  g_free(sb);

  /*  update the region  */
  if (preview)
    {
      gimp_preview_draw_buffer (preview, preview_buffer, width * bytes);
      g_free (preview_buffer);
    }
  else
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
    }
}

static gint
render_blast_row (guchar *buffer,
                  gint    bytes,
                  gint    lpi,
                  gint    threshold,
                  gint    strength,
                  edge_t  edge)
{
  gint Ri, Gi, Bi, Ai = 0;
  gint sbi, lbi;
  gint bleed_length;
  gint i, j;
  gint weight, random_factor;
  gint skip = 0;

  for (j = 0; j < lpi; j += bytes)
    {
      Ri = j; Gi = j + 1; Bi = j + 2;

      if (bytes > 3)
        Ai = j + 3;

      if (threshold_exceeded (buffer + Ri,
                              buffer + Ri + bytes,
                              edge,
                              threshold,
                              bytes > 3))
        {
          /* we have found an edge, do bleeding */
          sbi = Ri;

          weight = g_random_int_range (0, 10);
          if (weight > 5) /* 40% */
            {
              random_factor = 2;
            }
          else if (weight > 3) /* 20% */
            {
              random_factor = 3;
            }
          else /* 40% */
            {
              random_factor = 4;
            }
          bleed_length = 0;
          switch (g_random_int_range (0, random_factor))
            {
            case 3:
              bleed_length += strength;
              /* fall through to add up multiples of strength */
            case 2:
              bleed_length += strength;
              /* fall through */
            case 1:
              bleed_length += strength;
              /* fall through */
            case 0:
              bleed_length += strength;
              /* fall through */
            }

          lbi = sbi + bytes * bleed_length;
          if (lbi > lpi)
            {
              lbi = lpi;
            }

          for (i = sbi; i < lbi; i += bytes)
            {
              buffer[i] = buffer[Ri];
              buffer[i+1] = buffer[Gi];
              buffer[i+2] = buffer[Bi];
              if (bytes > 3)
                buffer[i+3] = buffer[Ai];
            }
          j = lbi - bytes;
          if (g_random_int_range (0, 10) > 7) /* 20% */
            {
              skip = 1;
            }
        }
    }
  return skip;
}

static void
render_wind_row (guchar *sb,
                 gint    bytes,
                 gint    lpi,
                 gint    threshold,
                 gint    strength,
                 edge_t  edge)
{
  gint i, j;
  gint bleed_length;
  gint blend_amt_R, blend_amt_G, blend_amt_B, blend_amt_A = 0 ;
  gint blend_colour_R, blend_colour_G, blend_colour_B, blend_colour_A = 0 ;
  gint target_colour_R, target_colour_G, target_colour_B, target_colour_A = 0;
  gdouble bleed_length_max;
  gint bleed_variation;
  gint n;
  gint sbi;  /* starting bleed index */
  gint lbi;      /* last bleed index */
  gdouble denominator;
  gint comp_stride = bytes * COMPARE_WIDTH;

  for (j = 0; j < lpi; j += bytes)
    {
      gint Ri = j;
      gint Gi = j + 1;
      gint Bi = j + 2;
      gint Ai = 0;

      if (bytes > 3)
        Ai = j + 3;

      if (threshold_exceeded (sb + Ri,
                              sb + Ri + comp_stride,
                              edge,
                              threshold,
                              (bytes > 3)))
        {
          /* we have found an edge, do bleeding */
          sbi = Ri + comp_stride;
          blend_colour_R = sb[Ri];
          blend_colour_G = sb[Gi];
          blend_colour_B = sb[Bi];
          target_colour_R = sb[sbi];
          target_colour_G = sb[sbi+1];
          target_colour_B = sb[sbi+2];
          bleed_length_max = strength;

          if (bytes > 3)
            {
              blend_colour_A = sb[Ai];
              target_colour_A = sb[sbi+3];
            }

          if (g_random_int_range (0, 3)) /* introduce weighted randomness */
            {
              bleed_length_max = strength;
            }
          else
            {
              bleed_length_max = 4 * strength;
            }

          bleed_variation = 1 + (gint) (bleed_length_max * g_random_double ());

          lbi = sbi + bleed_variation * bytes;
          if (lbi > lpi)
            {
              lbi = lpi; /* stop overunning the buffer */
            }

          bleed_length = bleed_variation;

          blend_amt_R = target_colour_R - blend_colour_R;
          blend_amt_G = target_colour_G - blend_colour_G;
          blend_amt_B = target_colour_B - blend_colour_B;
          if (bytes > 3)
            {
              blend_amt_A = target_colour_A - blend_colour_A;
            }
          denominator = bleed_length * bleed_length + bleed_length;
          denominator = 2.0 / denominator;
          n = bleed_length;
          for (i = sbi; i < lbi; i += bytes)
            {
              /* check against original colour */
              if (!threshold_exceeded (sb + Ri,
                                       sb + i,
                                       edge,
                                       threshold,
                                       bytes > 3)
                  && g_random_boolean ())
                {
                  break;
                }

              blend_colour_R += blend_amt_R * n * denominator;
              blend_colour_G += blend_amt_G * n * denominator;
              blend_colour_B += blend_amt_B * n * denominator;

              if (bytes > 3)
                {
                  blend_colour_A += blend_amt_A * n * denominator;
                  if (blend_colour_A > 255)
                    blend_colour_A = 255;
                  else if (blend_colour_A < 0)
                    blend_colour_A = 0;
                }

              if (blend_colour_R > 255)
                blend_colour_R = 255;
              else if (blend_colour_R < 0)
                blend_colour_R = 0;

              if (blend_colour_G > 255)
                blend_colour_G = 255;
              else if (blend_colour_G < 0)
                blend_colour_G = 0;

              if (blend_colour_B > 255)
                blend_colour_B = 255;
              else if (blend_colour_B < 0)
                blend_colour_B = 0;

              sb[i + 0] = (blend_colour_R * 2 + sb[i + 0]) / 3;
              sb[i + 1] = (blend_colour_G * 2 + sb[i + 1]) / 3;
              sb[i + 2] = (blend_colour_B * 2 + sb[i + 2]) / 3;

              if (bytes > 3)
                sb[i + 3] = (blend_colour_A * 2 + sb[i + 3]) / 3;

              if (threshold_exceeded (sb + i,
                                      sb + i + comp_stride,
                                      BOTH,
                                      threshold,
                                      bytes > 3))
                {
                  target_colour_R = sb[i + comp_stride + 0];
                  target_colour_G = sb[i + comp_stride + 1];
                  target_colour_B = sb[i + comp_stride + 2];
                  if (bytes > 3)
                    target_colour_A = sb[i + comp_stride + 3];
                  blend_amt_R = target_colour_R - blend_colour_R;
                  blend_amt_G = target_colour_G - blend_colour_G;
                  blend_amt_B = target_colour_B - blend_colour_B;
                  if (bytes > 3)
                    blend_amt_A = target_colour_A - blend_colour_A;
                  denominator = n * n + n;
                  denominator = 2.0 / denominator;
                }
              n--;
            }
        }
    }
}

static gboolean
threshold_exceeded (guchar  *pixel_R1,
                    guchar  *pixel_R2,
                    edge_t   edge,
                    gint     threshold,
                    gboolean has_alpha)
{
  gint derivative_R, derivative_G, derivative_B, derivative_A;

  get_derivative (pixel_R1, pixel_R2, edge, has_alpha,
                  &derivative_R, &derivative_G, &derivative_B, &derivative_A);

  return (((derivative_R +
            derivative_G +
            derivative_B +
            derivative_A) / 4) > threshold);
}

static void
get_derivative (guchar   *pixel_R1,
                guchar   *pixel_R2,
                edge_t    edge,
                gboolean  has_alpha,
                gint     *derivative_R,
                gint     *derivative_G,
                gint     *derivative_B,
                gint     *derivative_A)
{
  guchar *pixel_G1 = pixel_R1 + 1;
  guchar *pixel_B1 = pixel_R1 + 2;
  guchar *pixel_G2 = pixel_R2 + 1;
  guchar *pixel_B2 = pixel_R2 + 2;
  guchar *pixel_A1;
  guchar *pixel_A2;

  if (has_alpha)
    {
      pixel_A1 = pixel_R1 + 3;
      pixel_A2 = pixel_R2 + 3;
      *derivative_A = *pixel_A2 - *pixel_A1;
    }
  else
    {
      *derivative_A = 0;
    }

  *derivative_R = *pixel_R2 - *pixel_R1;
  *derivative_G = *pixel_G2 - *pixel_G1;
  *derivative_B = *pixel_B2 - *pixel_B1;

  if (edge == BOTH)
    {
      *derivative_R = abs (*derivative_R);
      *derivative_G = abs (*derivative_G);
      *derivative_B = abs (*derivative_B);
      *derivative_A = abs (*derivative_A);
    }
  else if (edge == LEADING)
    {
      *derivative_R = -(*derivative_R);
      *derivative_G = -(*derivative_G);
      *derivative_B = -(*derivative_B);
      *derivative_A = -(*derivative_A);
    }
  else if (edge == TRAILING)
    {
      /* no change needed */
    }
}

static void
reverse_buffer (guchar *buffer,
                gint    length,
                gint    bytes)
{
  gint i, si;
  gint temp;
  gint midpoint;

  midpoint = length / 2;
  for (i = 0; i < midpoint; i += bytes)
    {
      si = length - bytes - i;

      temp = buffer[i];
      buffer[i] = buffer[si];
      buffer[si] = (guchar) temp;

      temp = buffer[i + 1];
      buffer[i + 1] = buffer[si + 1];
      buffer[si + 1] = (guchar) temp;

      temp = buffer[i + 2];
      buffer[i + 2] = buffer[si + 2];
      buffer[si + 2] = (guchar) temp;

      if (bytes > 3)
        {
          temp = buffer[i + 3];
          buffer[i + 3] = buffer[si + 3];
          buffer[si + 3] = (guchar) temp;
        }
    }
}

/***************************************************
  GUI
 ***************************************************/

static gint
dialog_box (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *table;
  GtkWidget *preview;
  GtkObject *adj;
  GtkWidget *frame;
  GtkWidget *style1;
  GtkWidget *style2;
  GtkWidget *dir1;
  GtkWidget *dir2;
  GtkWidget *edge1;
  GtkWidget *edge2;
  GtkWidget *edge3;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Wind"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, &config.update_preview);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (render_effect), drawable);

  /*****************************************************
    outer frame and table
  ***************************************************/

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

  /*********************************************************
    radio buttons for choosing wind rendering algorithm
    ******************************************************/

  frame = gimp_int_radio_group_new (TRUE, _("Style"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &config.alg, config.alg,

                                    _("_Wind"),  RENDER_WIND,  &style1,
                                    _("_Blast"), RENDER_BLAST, &style2,

                                    NULL);

  g_signal_connect_swapped (style1, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (style2, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  /******************************************************
    radio buttons for choosing LEFT or RIGHT
    **************************************************/

  frame = gimp_int_radio_group_new (TRUE, _("Direction"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &config.direction, config.direction,

                                    _("_Left"),  LEFT,  &dir1,
                                    _("_Right"), RIGHT, &dir2,

                                    NULL);

  g_signal_connect_swapped (dir1, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (dir2, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  /*****************************************************
    radio buttons for choosing BOTH, LEADING, TRAILING
    ***************************************************/

  frame = gimp_int_radio_group_new (TRUE, _("Edge Affected"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &config.edge, config.edge,

                                    _("L_eading"),  LEADING,  &edge1,
                                    _("Tr_ailing"), TRAILING, &edge2,
                                    _("Bot_h"),     BOTH,     &edge3,

                                    NULL);

  g_signal_connect_swapped (edge1, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (edge2, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (edge3, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_table_attach (GTK_TABLE (table), frame, 2, 3, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  gtk_widget_show (table);

  /****************************************************
   table for sliders
   ****************************************************/
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

  /*****************************************************
    slider and entry for threshold
    ***************************************************/

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Threshold:"), SCALE_WIDTH, 0,
                              config.threshold,
                              MIN_THRESHOLD, MAX_THRESHOLD, 1.0, 10, 0,
                              TRUE, 0, 0,
                              _("Higher values restrict the effect to fewer areas of the image"), NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &config.threshold);

  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*****************************************************
    slider and entry for strength of wind
    ****************************************************/

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("_Strength:"), SCALE_WIDTH, 0,
                              config.strength,
                              MIN_STRENGTH, MAX_STRENGTH, 1.0, 10.0, 0,
                              TRUE, 0, 0,
                              _("Higher values increase the magnitude of the effect"), NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &config.strength);

  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (table);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
