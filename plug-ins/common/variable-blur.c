/****************************************************************************
 * This is a plugin for the GIMP v 2.0
 * Install if by running "gimptool-2.0 --install variable-blur.c"
 * It installs as: Filters->Blur->Variable Blur
 *
 * Copyright 2004, William Skaggs <weskaggs@primate.ucdavis.edu>
 * The code was created by modifying
 * the source for the "blur" plug-in from GIMP 2.0pre4.  See the source
 * for that plug-in for more detailed history.
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
 ****************************************************************************/

/****************************************************************************
 * Variable Blur:
 *
 * variable-blur 1.0 (23 March, 2004, WES)
 *    based on
 * blur version 2.0 (1 May 1998, MEO)
 * history
 *     2.0 -  1 May 1998 MEO
 *         based on randomize 1.7
 *
 * Variable blur applies a 3x3 blurring convolution kernel to the specified drawable.
 * The relative weights of central and surrounding pixels vary spatially,
 * determined by the local value of a user-specified "map" drawable.
 *
 * This works only with RGB and grayscale images.
 *
 ****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/*
 *  progress meter update frequency
 */
#define PROG_UPDATE_TIME ((row % 10) == 0)

#define PLUG_IN_NAME "plug-in-varible-blur"
#define HELP_ID      "plug-in-variable-blur"

#define SEED_DEFAULT 10
#define SEED_USER 11

#define SCALE_WIDTH 100

typedef struct
{
  gdouble  blur_rcount;    /* repeat count */
  gint32   drawable_id;
  gint32   map_id;
  gboolean map_is_rgb;
  gboolean run;
} BlurVals;

static BlurVals pivals =
  {
    1.0,
    -1,
    -1,
    FALSE,
    FALSE
  };

static const gchar *blurb = "Applies a 3x3 blurring convolution kernel, with spatially varying strength.";

static const gchar *help = "The Variable Blur filter blurs the target drawable by varying amounts at different locations.  You specify the number of times blurring is applied, and a drawable whose pixel values determine the intensity of blurring at each point.  Supports RGB and grayscale images.";

/*********************************
 *
 *  LOCAL FUNCTIONS
 *
 ********************************/

static void
query (void);

static void run (const gchar      *name,
		 gint              nparams,
		 const GimpParam  *param,
		 gint             *nreturn_vals,
		 GimpParam       **return_vals);

GimpPlugInInfo PLUG_IN_INFO =
  {
    NULL,  /* init_proc  */
    NULL,  /* quit_proc  */
    query, /* query_proc */
    run,   /* run_proc   */
  };

static void        blur                   (GimpDrawable     *drawable,
                                           GimpPreview      *preview);
static inline void blur_prepare_row       (GimpPixelRgn     *pixel_rgn,
                                           guchar           *data,
                                           gint              x,
                                           gint              y,
                                           gint              w);
static void        blur_dialog            (GimpDrawable     *drawable);
static void        build_beta_table       (gdouble         **beta_table_ptr,
                                           gint              count);
static gboolean    dialog_constrain       (gint32            image_id,
                                           gint32            drawable_id,
                                           gpointer          data);
static void        dialog_usemap_callback (GimpIntComboBox  *combo,
                                           GimpPreview      *preview);
static void        dialog_response        (GtkWidget        *widget,
                                           gint              response_id,
                                           gpointer          data);

/************************************ Guts ***********************************/

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Input drawable" },
    { GIMP_PDB_DRAWABLE, "map_drawable", "Intensity map drawable" },
    { GIMP_PDB_FLOAT,    "blur_rcount",  "Repeat count (1 - 1000)" },
  };

  const gchar *author = "William Skaggs";
  const gchar *copyrights = "Miles O'Neal, Spencer Kimball, Peter Mattis, Torsten Martinsen, Brian Degenhardt, Federico Mena Quintero, Stephen Norris, Daniel Cotting, William Skaggs";
  const gchar *copyright_date = "1995-1998,2004";

  gimp_install_procedure (PLUG_IN_NAME,
                          (gchar *) blurb,
                          (gchar *) help,
                          (gchar *) author,
                          (gchar *) copyrights,
                          (gchar *) copyright_date,
                          N_("_Variable Blur..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/Filters/Blur");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;        /* assume the best! */
  static GimpParam   values[1];

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  /*
   *  Get the specified drawable, do standard initialization.
   */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  pivals.drawable_id = drawable->drawable_id;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals = values;

  gimp_tile_cache_ntiles (4 * drawable->ntile_cols);

  /*
   *  Make sure the drawable type is appropriate.
   */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      switch (run_mode)
        {
          /*
           *  If we're running interactively, pop up the dialog box.
           */
        case GIMP_RUN_INTERACTIVE:
          gimp_get_data (PLUG_IN_NAME, &pivals);
          blur_dialog (drawable);
          break;

          /*
           *  If we're not interactive (probably scripting), we
           *  get the parameters from the param[] array, since
           *  we don't use the dialog box.  Make sure all
           *  parameters have legitimate values.
           */
        case GIMP_RUN_NONINTERACTIVE:
          if (nparams == 5)
            {
              pivals.map_id      = param[3].data.d_drawable;
              pivals.blur_rcount = CLAMP ((gdouble) param[4].data.d_float, 1.0, 100.0);
	      pivals.run	 = TRUE;
            }
          else
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          break;

          /*
           *  If we're running with the last set of values, get those values.
           */
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_get_data (PLUG_IN_NAME, &pivals);
	  pivals.run = TRUE;
          break;

          /*
           *  Hopefully we never get here!
           */
        default:
          break;
        }

      if (pivals.run && status == GIMP_PDB_SUCCESS)
        {
          /*
           *  JUST DO IT!
           */
          gimp_progress_init (_("Blurring..."));

          blur (drawable, NULL);
          /*
           *  If we ran interactively, update the display.
           */
          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            {
              gimp_displays_flush ();
            }
          /*
           *  If we use the dialog popup, set the data for future use.
           */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            {
              gimp_set_data (PLUG_IN_NAME, &pivals, sizeof (BlurVals));
            }
        }
    }
  else
    {
      /*
       *  If we got the wrong drawable type, we need to complain.
       */
      status = GIMP_PDB_EXECUTION_ERROR;
    }
  /*
   *  DONE!
   *  Set the status where the GIMP can see it, and let go
   *  of the drawable.
   */
  values[0].data.d_status = status;
  gimp_drawable_detach (drawable);

}

/*********************************
 *
 *  blur_prepare_row()
 *
 *  Get a row of pixels.  If the requested row
 *  is off the edge, clone the edge row.
 *
 ********************************/

static inline void
blur_prepare_row (GimpPixelRgn *pixel_rgn,
                  guchar       *data,
                  gint          x,
                  gint          y,
                  gint          w)
{
  int b;

  y = CLAMP (y, 1, pixel_rgn->y + pixel_rgn->h - 1);
  gimp_pixel_rgn_get_row (pixel_rgn, data, x, y, w);

  /*
   *  Fill in edge pixels
   */
  for (b = 0; b < pixel_rgn->bpp; b++)
    {
      data[- (gint) pixel_rgn->bpp + b] = data[b];
      data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

/*********************************
 *
 *  blur()
 *
 *  Actually mess with the image.
 *
 ********************************/

static void
blur (GimpDrawable *drawable,
      GimpPreview  *preview)
{
  GimpDrawable *map_drawable      = NULL;
  GimpPixelRgn  srcPR, destPR;
  GimpPixelRgn  destPR2, mapPR;
  GimpPixelRgn *sp, *dp, *tp, *mp;
  gint          width, height;
  gint          bytes;
  gint		map_bytes         = 0;
  guchar       *dest, *d;
  guchar       *prev_row, *pr;
  guchar       *cur_row, *cr;
  guchar       *next_row, *nr;
  guchar       *map_row           = NULL;
  guchar       *mr                = NULL;
  guchar       *tmp;
  gint          row, col;
  gint          x1, y1, x2, y2;
  gint          cnt, ind;
  gint          repeat_count;
  gboolean      has_alpha;
  gdouble	A, B, C, D, X, Y;
  gdouble	beta              = 0;
  gdouble	*beta_table       = NULL;
  gchar		msg[40];

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
      width     = (x2 - x1);
      height    = (y2 - y1);
    }

  map_drawable = gimp_drawable_get (pivals.map_id);
  if (! map_drawable)
    return;

  bytes = drawable->bpp;
  map_bytes = map_drawable->bpp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  /*
   *  allocate row buffers
   */
  prev_row = g_new (guchar, (x2 - x1 + 2) * bytes);
  cur_row  = g_new (guchar, (x2 - x1 + 2) * bytes);
  next_row = g_new (guchar, (x2 - x1 + 2) * bytes);
  map_row  = g_new (guchar, (x2 - x1 + 2) * map_bytes);
  dest     = g_new (guchar, (x2 - x1) * bytes);

  /*
   *  initialize the pixel regions
   */
  gimp_pixel_rgn_init (&srcPR,   drawable, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR,  drawable, x1, y1, width, height, TRUE,  TRUE);
  gimp_pixel_rgn_init (&destPR2, drawable, x1, y1, width, height, TRUE,  TRUE);
  gimp_pixel_rgn_init (&mapPR,   map_drawable, x1, y1, width, height, FALSE, FALSE);

  sp = &srcPR;
  dp = &destPR;
  tp = NULL;
  mp = &mapPR;

  pr = prev_row + bytes;
  cr = cur_row + bytes;
  nr = next_row + bytes;
  mr = map_row + map_bytes;

  repeat_count = (gint) pivals.blur_rcount;

  build_beta_table (&beta_table, repeat_count);

  for (cnt = 1; cnt <= repeat_count; cnt++)
    {
      sprintf (msg, "%s %d", _("Blurring, count ="), cnt);
      if (! preview)
        {
          gimp_progress_init (msg);
          /*
           * process any pending events, such as Cancel for example
           */
          while (gtk_events_pending ())
            gtk_main_iteration ();
        }

      /*
       *  prepare the first row and previous row
       */
      blur_prepare_row (sp, pr, x1, y1 - 1, (x2 - x1));
      blur_prepare_row (dp, cr, x1, y1,     (x2 - x1));
      /*
       *  loop through the rows, applying the selected convolution
       */
      for (row = y1; row < y2; row++)
        {
          /*  prepare the next row  */
          blur_prepare_row (sp, nr, x1, row + 1, (x2 - x1));
	  blur_prepare_row (mp, mr, x1, row,     (x2 - x1));

          d = dest;
          ind = 0;

          for (col = 0; col < (x2 - x1) * bytes; col++)
            {
	      /*
	       * calculate relative weights of center pixel
	       * and surrounding pixels.
	       */
	      if (ind == 0)
		{
		  guchar value;
                  gint   c      = (col / bytes) * map_bytes;

		  if (pivals.map_is_rgb)
		    value = ((gint) mr[c] + (gint) mr[c + 1] + (gint) mr[c + 2]) / 3;
		  else
		    value = mr[c];

		  beta = beta_table[value];
		}

	      ind++;

	      if (ind == bytes || !has_alpha)
		{
		  /*
		   *  If no alpha channel,
		   *   or if there is one and this is it...
		   */
		  *d++ = (1 - beta) * cr[col] + (beta *
						 ((gint) pr[col - bytes] + (gint) pr[col] +
						  (gint) pr[col + bytes] +
						  (gint) cr[col - bytes] +
						  (gint) cr[col + bytes] +
						  (gint) nr[col - bytes] + (gint) nr[col] +
						  (gint) nr[col + bytes])) / 8.0 + 0.5;
		  ind = 0;
		}
	      else
		{
		  /*
		   *  otherwise we have an alpha channel,
		   *   but this is a color channel
		   */
		  A = (gdouble) (cr[col] * cr[col + bytes - ind]);
		  B = (gdouble) cr[col + bytes - ind];

		  X = ((gdouble) (pr[col - bytes] * pr[col - ind])
                       + (gdouble) (pr[col] * pr[col + bytes - ind])
                       + (gdouble) (pr[col + bytes] * pr[col + 2 * bytes - ind])
                       + (gdouble) (cr[col - bytes] * cr[col - ind])
                       + (gdouble) (cr[col + bytes] * cr[col + 2 * bytes - ind])
                       + (gdouble) (nr[col - bytes] * nr[col - ind])
                       + (gdouble) (nr[col] * nr[col + bytes - ind])
                       + (gdouble) (nr[col + bytes] * nr[col + 2 * bytes - ind]));

		  Y = ((gdouble) pr[col - ind]
		       + (gdouble) pr[col + bytes - ind]
		       + (gdouble) pr[col + 2 * bytes - ind]
		       + (gdouble) cr[col - ind]
		       + (gdouble) cr[col + 2 * bytes - ind]
		       + (gdouble) nr[col - ind]
		       + (gdouble) nr[col + bytes - ind]
		       + (gdouble) nr[col + 2 * bytes - ind]);

		  D = (1 - beta) * B + beta * Y;

		  if (B < 1.0)
		    C = 0;
		  else
		    C = A / B;

		  if (Y < 1)
		    D = 0;
		  else
		    D = X / Y;

		  *d++ = (gint) ((1 - beta) * C + beta * D + 0.5);
		}
	    }
          /*
           *  Save the modified row, shuffle the row pointers, and every
           *  so often, update the progress meter.
           */
          gimp_pixel_rgn_set_row (dp, dest, x1, row, (x2 - x1));

          tmp = pr;
          pr = cr;
          cr = nr;
          nr = tmp;

          if (! preview && PROG_UPDATE_TIME)
            {
              gdouble base = (gdouble) cnt / repeat_count;
              gdouble inc  = (gdouble) row / ((y2 - y1) * repeat_count);

              gimp_progress_update (base + inc);
            }
        }

      /*
       *  if we have more cycles to perform, swap the src and dest Pixel Regions
       */
      if (cnt < repeat_count)
        {
          if (tp != NULL)
            {
              tp = dp;
              dp = sp;
              sp = tp;
            }
          else
            {
              tp = &srcPR;
              sp = &destPR;
              dp = &destPR2;
            }
        }
    }

  /*
   *  update the blurred region
   */
  if (preview)
    {
      gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                         dp);
    }
  else
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
    }

  /*
   *  clean up after ourselves.
   */
  g_free (prev_row);
  g_free (cur_row);
  g_free (next_row);
  g_free (map_row);
  g_free (dest);
}

/*********************************
 *  GUI ROUTINES
 ********************************/

static void
blur_dialog (GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *table;
  GtkWidget *hbox;
  GtkWidget *tmpw;
  GtkObject *adj;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *combo;

  gimp_ui_init ("variable blur", FALSE);

  /*
   * sanity check
   */
  if (pivals.map_id == -1)
    pivals.map_id = pivals.drawable_id;

  dlg = gimp_dialog_new (_("Variable Blur"), "blur",
                         NULL, 0,
                         gimp_standard_help_func, HELP_ID,

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  g_signal_connect (dlg, "response",
                    G_CALLBACK (dialog_response),
                    NULL);
  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (blur),
                            drawable);

  /*
   *  Parameter settings
   *
   */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);

  table = gtk_table_new (1, 5, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 15);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 5);
  gtk_widget_show (table);

  /*
   *  Repeat count label & scale (1 to 100)
   */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Repeat:"), SCALE_WIDTH, 0,
                              pivals.blur_rcount, 1.0, 1000.0, 1.0, 10.0, 0,
                              TRUE, 0, 0,
                              _("Number of times to apply filter"),
			      "plug-in-variable-blur-repeat");
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pivals.blur_rcount);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*
   * drawable menu for blur intensity map
   */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  tmpw = gtk_label_new (_("Intensity map:"));
  gtk_box_pack_start (GTK_BOX (hbox), tmpw, FALSE, FALSE, 5);
  gtk_widget_show (tmpw);
  gimp_help_set_help_data
    (tmpw, _("Image that determines the intensity of blurring at each location"),
     "plug-in-variable-blur-map");

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  combo = gimp_drawable_combo_box_new (dialog_constrain,
                                       GINT_TO_POINTER (pivals.drawable_id));
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              pivals.map_id,
                              (GCallback) dialog_usemap_callback,
                              preview);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 10);
  gtk_widget_show (combo);

  gtk_widget_show (dlg);
  gtk_main();
  return;
}


/* The intensity map can be rgb or grayscale, must have the same shape as the base image */
static gboolean
dialog_constrain (gint32   image_id,
                  gint32   drawable_id,
                  gpointer data)
{
  gint32 src_drawable_id = GPOINTER_TO_INT (data);

  if (drawable_id == -1) /* checking an image, not a drawable */
    return FALSE;

  if(gimp_drawable_width (drawable_id) == gimp_drawable_width (src_drawable_id)
     && gimp_drawable_height (drawable_id) == gimp_drawable_height (src_drawable_id)
     && (gimp_drawable_is_rgb (drawable_id) || gimp_drawable_is_gray (drawable_id)) )
    return TRUE;
  else
    return FALSE;
}



static void
dialog_usemap_callback (GimpIntComboBox *combo,
                        GimpPreview     *preview)
{
  gint id;

  if (! gimp_int_combo_box_get_active (combo, &id))
    {
      id = pivals.drawable_id;
      gimp_int_combo_box_set_active (combo, id);
    }

  pivals.map_id = id;
  pivals.map_is_rgb = gimp_drawable_is_rgb (id);

  gimp_preview_invalidate (preview);
}


static void
dialog_response (GtkWidget *widget,
                 gint       response_id,
                 gpointer   data)
{
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      pivals.run = TRUE;
      gtk_widget_destroy (widget);
      gtk_main_quit ();
      break;

    default:
      pivals.run = FALSE;
      gtk_widget_destroy (widget);
      gtk_main_quit ();
      break;
    }
}


/*
 * The beta table encodes the coefficient corresponding to each possible
 * map value.
 */
static void
build_beta_table (gdouble **beta_table_ptr,
                  gint      count)
{
  int      k;
  gdouble *beta_table;

  *beta_table_ptr = (gdouble *) g_malloc (256 * sizeof (gdouble));
  beta_table = *beta_table_ptr;

  for(k = 0; k < 256; k++)
    beta_table[k] = SQR (k / 255.);
}
