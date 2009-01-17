/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

/* This plugin by thorsten@arch.usyd.edu.au           */
/* Based on S&P's Gauss and Blur filters              */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-sobel"
#define PLUG_IN_BINARY "edge-sobel"


typedef struct
{
  gboolean horizontal;
  gboolean vertical;
  gboolean keep_sign;
} SobelValues;


/* Declare local functions.
 */
static void   query  (void);
static void   run    (const gchar      *name,
                      gint              nparams,
                      const GimpParam  *param,
                      gint             *nreturn_vals,
                      GimpParam       **return_vals);

static void   sobel  (GimpDrawable     *drawable,
                      gboolean          horizontal,
                      gboolean          vertical,
                      gboolean          keep_sign,
                      GimpPreview      *preview);

/*
 * Sobel interface
 */
static gboolean  sobel_dialog         (GimpDrawable *drawable);
static void      sobel_preview_update (GimpPreview  *preview);

/*
 * Sobel helper functions
 */
static void      sobel_prepare_row (GimpPixelRgn *pixel_rgn,
                                    guchar       *data,
                                    gint          x,
                                    gint          y,
                                    gint          w);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static SobelValues bvals =
{
  TRUE,  /*  horizontal sobel  */
  TRUE,  /*  vertical sobel    */
  TRUE   /*  keep sign         */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",   "Interactive, non-interactive"  },
    { GIMP_PDB_IMAGE,    "image",      "Input image (unused)"          },
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable"                },
    { GIMP_PDB_INT32,    "horizontal", "Sobel in horizontal direction" },
    { GIMP_PDB_INT32,    "vertical",   "Sobel in vertical direction"   },
    { GIMP_PDB_INT32,    "keep-sign",  "Keep sign of result (one direction only)" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Specialized direction-dependent edge detection"),
                          "This plugin calculates the gradient with a sobel "
                          "operator. The user can specify which direction to "
                          "use. When both directions are used, the result is "
                          "the RMS of the two gradients; if only one direction "
                          "is used, the result either the absolut value of the "
                          "gradient, or 127 + gradient (if the 'keep sign' "
                          "switch is on). This way, information about the "
                          "direction of the gradient is preserved. Resulting "
                          "images are not autoscaled.",
                          "Thorsten Schnier",
                          "Thorsten Schnier",
                          "1997",
                          N_("_Sobel..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Edge-Detect");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  gimp_tile_cache_ntiles (2 * drawable->ntile_cols);

  switch (run_mode)
   {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &bvals);

      /*  First acquire information with a dialog  */
      if (! sobel_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 6)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          bvals.horizontal = (param[4].data.d_int32) ? TRUE : FALSE;
          bvals.vertical   = (param[5].data.d_int32) ? TRUE : FALSE;
          bvals.keep_sign  = (param[6].data.d_int32) ? TRUE : FALSE;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &bvals);
      break;

    default:
      break;
    }

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      sobel (drawable,
             bvals.horizontal, bvals.vertical, bvals.keep_sign,
             NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();


      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &bvals, sizeof (bvals));
    }
  else
    {
      status        = GIMP_PDB_EXECUTION_ERROR;
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = _("Cannot operate on indexed color images.");
    }

  gimp_drawable_detach (drawable);

  values[0].data.d_status = status;
}

static gboolean
sobel_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *toggle;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Sobel Edge Detection"), PLUG_IN_BINARY,
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

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (sobel_preview_update),
                    NULL);

  toggle = gtk_check_button_new_with_mnemonic (_("Sobel _horizontally"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.horizontal);
  gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &bvals.horizontal);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic (_("Sobel _vertically"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.vertical);
  gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &bvals.vertical);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic (_("_Keep sign of result "
                                                 "(one direction only)"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.keep_sign);
  gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &bvals.keep_sign);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
sobel_preview_update (GimpPreview *preview)
{
  sobel (gimp_drawable_preview_get_drawable (GIMP_DRAWABLE_PREVIEW (preview)),
         bvals.horizontal,
         bvals.vertical,
         bvals.keep_sign,
         preview);
}

static void
sobel_prepare_row (GimpPixelRgn *pixel_rgn,
                   guchar       *data,
                   gint          x,
                   gint          y,
                   gint          w)
{
  gint b;

  y = CLAMP (y, 0, pixel_rgn->h - 1);
  gimp_pixel_rgn_get_row (pixel_rgn, data, x, y, w);

  /*  Fill in edge pixels  */
  for (b = 0; b < pixel_rgn->bpp; b++)
    {
      data[-(int)pixel_rgn->bpp + b] = data[b];
      data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

#define RMS(a, b) (sqrt ((a) * (a) + (b) * (b)))

static void
sobel (GimpDrawable *drawable,
       gboolean      do_horizontal,
       gboolean      do_vertical,
       gboolean      keep_sign,
       GimpPreview  *preview)
{
  GimpPixelRgn  srcPR, destPR;
  gint          width, height;
  gint          bytes;
  gint          gradient, hor_gradient, ver_gradient;
  guchar       *dest, *d;
  guchar       *prev_row, *pr;
  guchar       *cur_row, *cr;
  guchar       *next_row, *nr;
  guchar       *tmp;
  gint          row, col;
  gint          x1, y1, x2, y2;
  gboolean      alpha;
  gint          counter;
  guchar       *preview_buffer = NULL;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
      x2 = x1 + width;
      y2 = y1 + height;
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
      gimp_progress_init (_("Sobel edge detecting"));
      width  = x2 - x1;
      height = y2 - y1;
    }

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  bytes  = drawable->bpp;
  alpha  = gimp_drawable_has_alpha (drawable->drawable_id);

  /*  allocate row buffers  */
  prev_row = g_new (guchar, (width + 2) * bytes);
  cur_row  = g_new (guchar, (width + 2) * bytes);
  next_row = g_new (guchar, (width + 2) * bytes);
  dest     = g_new (guchar, width * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0,
                       drawable->width, drawable->height,
                       FALSE, FALSE);

  if (preview)
    {
      preview_buffer = g_new (guchar, width * height * bytes);
    }
  else
    {
      gimp_pixel_rgn_init (&destPR, drawable, 0, 0,
                           drawable->width, drawable->height,
                           TRUE, TRUE);
    }

  pr = prev_row + bytes;
  cr = cur_row  + bytes;
  nr = next_row + bytes;

  sobel_prepare_row (&srcPR, pr, x1, y1 - 1, width);
  sobel_prepare_row (&srcPR, cr, x1, y1, width);
  counter =0;
  /*  loop through the rows, applying the sobel convolution  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      sobel_prepare_row (&srcPR, nr, x1, row + 1, width);

      d = dest;
      for (col = 0; col < width * bytes; col++)
        {
          hor_gradient = (do_horizontal ?
                          ((pr[col - bytes] +  2 * pr[col] + pr[col + bytes]) -
                           (nr[col - bytes] + 2 * nr[col] + nr[col + bytes]))
                          : 0);
          ver_gradient = (do_vertical ?
                          ((pr[col - bytes] + 2 * cr[col - bytes] + nr[col - bytes]) -
                           (pr[col + bytes] + 2 * cr[col + bytes] + nr[col + bytes]))
                          : 0);
          gradient = (do_vertical && do_horizontal) ?
            (ROUND (RMS (hor_gradient, ver_gradient)) / 5.66) /* always >0 */
            : (keep_sign ? (127 + (ROUND ((hor_gradient + ver_gradient) / 8.0)))
               : (ROUND (abs (hor_gradient + ver_gradient) / 4.0)));

          if (alpha && (((col + 1) % bytes) == 0))
            { /* the alpha channel */
              *d++ = (counter == 0) ? 0 : 255;
              counter = 0;
            }
          else
            {
              *d++ = gradient;
              if (gradient > 10) counter ++;
            }
        }
      /*  shuffle the row pointers  */
      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;

      /*  store the dest  */
      if (preview)
        {
          memcpy (preview_buffer + width * (row - y1) * bytes,
                  dest,
                  width * bytes);
        }
      else
        {
          gimp_pixel_rgn_set_row (&destPR, dest, x1, row, width);

          if ((row % 20) == 0)
            gimp_progress_update ((double) row / (double) (y2 - y1));
        }
    }

  if (preview)
    {
      gimp_preview_draw_buffer (preview, preview_buffer, width * bytes);
      g_free (preview_buffer);
    }
  else
    {
      /*  update the sobeled region  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
    }

  g_free (prev_row);
  g_free (cur_row);
  g_free (next_row);
  g_free (dest);
}
