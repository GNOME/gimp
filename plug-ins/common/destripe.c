/*
 *   Destripe filter for The GIMP -- an image manipulation
 *   program
 *
 *   Copyright 1997 Marc Lehmann, heavily modified from a filter by
 *   Michael Sweet.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contents:
 *
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the filter...
 *   destripe()                  - Destripe an image.
 *   destripe_dialog()           - Popup a dialog window...
 *   preview_init()              - Initialize the preview window...
 *   preview_scroll_callback()   - Update the preview when a scrollbar is moved.
 *   preview_update()            - Update the preview window.
 *   preview_exit()              - Free all memory used by the preview window...
 *   dialog_iscale_update()      - Update the value field using the scale.
 *   dialog_histogram_callback()
 *   dialog_ok_callback()        - Start the filter...
 *
 *   1997/08/16 * Initial Revision.
 *   1998/02/06 * Minor changes.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/*
 * Constants...
 */

#define PLUG_IN_NAME     "plug_in_destripe"
#define PLUG_IN_VERSION  "0.2"
#define PREVIEW_SIZE     200
#define SCALE_WIDTH      140
#define MAX_AVG          100

/*
 * Local functions...
 */

static void	query (void);
static void	run   (gchar   *name,
		       gint     nparams,
		       GParam  *param,
		       gint    *nreturn_vals,
		       GParam **return_vals);

static void	destripe (void);

static gint	destripe_dialog           (void);
static void     dialog_histogram_callback (GtkWidget *, gpointer);
static void	dialog_iscale_update      (GtkAdjustment *, gint *);
static void	dialog_ok_callback        (GtkWidget *, gpointer);

static void	preview_init            (void);
static void	preview_exit            (void);
static void	preview_update          (void);
static void	preview_scroll_callback (void);


/*
 * Globals...
 */

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

GtkWidget      *preview;		/* Preview widget */
gint		preview_width,		/* Width of preview widget */
		preview_height,		/* Height of preview widget */
		preview_x1,		/* Upper-left X of preview */
		preview_y1,		/* Upper-left Y of preview */
		preview_x2,		/* Lower-right X of preview */
		preview_y2;		/* Lower-right Y of preview */
GtkObject      *hscroll_data,		/* Horizontal scrollbar data */
	       *vscroll_data;		/* Vertical scrollbar data */

GDrawable      *drawable = NULL;	/* Current image */
gint		sel_x1,			/* Selection bounds */
		sel_y1,
		sel_x2,
		sel_y2;
gint		histogram = FALSE;
gint		img_bpp;		/* Bytes-per-pixel in image */
gint		run_filter = FALSE;	/* True if we should run the filter */

gint		avg_width = 36;


MAIN ()

static void
query (void)
{
  static GParamDef	args[] =
  {
    { PARAM_INT32,	"run_mode",	"Interactive, non-interactive" },
    { PARAM_IMAGE,	"image",	"Input image" },
    { PARAM_DRAWABLE,	"drawable",	"Input drawable" },
    { PARAM_INT32,	"avg_width",	"Averaging filter width (default = 36)" }
  };
  static GParamDef	*return_vals = NULL;
  static int		nargs        = sizeof(args) / sizeof(args[0]),
			nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure (PLUG_IN_NAME,
			  _("Destripe filter, used to remove vertical stripes caused by cheap scanners."),
			  _("This plug-in tries to remove vertical stripes from an image."),
			  "Marc Lehmann <pcg@goof.com>", "Marc Lehmann <pcg@goof.com>",
			  PLUG_IN_VERSION,
			  N_("<Image>/Filters/Enhance/Destripe..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (gchar  *name,
     gint   nparams,
     GParam *param,
     gint   *nreturn_vals,
     GParam **return_vals)
{
  GRunModeType	run_mode;	/* Current run mode */
  GStatusType	status;		/* Return status */
  static GParam	values[1];	/* Return values */

  INIT_I18N_UI();

  /*
   * Initialize parameter data...
   */

  status   = STATUS_SUCCESS;
  run_mode = param[0].data.d_int32;

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  /*
   * Get drawable information...
   */

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  gimp_drawable_mask_bounds (drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  img_bpp = gimp_drawable_bpp (drawable->id);

  /*
   * See how we will run
   */

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_NAME, &avg_width);

      /*
       * Get information from the dialog...
       */
      if (!destripe_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*
       * Make sure all the arguments are present...
       */
      if (nparams != 4)
	status = STATUS_CALLING_ERROR;
      else
	avg_width = param[3].data.d_int32;
      break;

    case RUN_WITH_LAST_VALS :
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_NAME, &avg_width);
      break;

    default :
      status = STATUS_CALLING_ERROR;
      break;
    };

  /*
   * Destripe the image...
   */

  if (status == STATUS_SUCCESS)
    {
      if ((gimp_drawable_is_rgb (drawable->id) ||
	   gimp_drawable_is_gray (drawable->id)))
	{
	  /*
	   * Set the tile cache size...
	   */
	  gimp_tile_cache_ntiles ((drawable->width + gimp_tile_width() - 1) /
				  gimp_tile_width());

	  /*
	   * Run!
	   */
	  destripe ();

	  /*
	   * If run mode is interactive, flush displays...
	   */
	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush();

	  /*
	   * Store data...
	   */
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data (PLUG_IN_NAME, &avg_width, sizeof(avg_width));
	}
      else
	status = STATUS_EXECUTION_ERROR;
    };

  /*
   * Reset the current run status...
   */
  values[0].data.d_status = status;

  /*
   * Detach from the drawable...
   */
  gimp_drawable_detach (drawable);
}

static inline void
preview_draw_row (gint    x,
		  gint    y,
		  gint    w,
		  guchar *row)
{
  guchar *rgb = g_new (guchar, w * 3);
  guchar *rgb_ptr;
  gint i;

  switch (img_bpp)
    {
    case 1:
    case 2:
      for (i = 0, rgb_ptr = rgb; i < w; i++, row += img_bpp, rgb_ptr += 3)
	rgb_ptr[0] = rgb_ptr[1] = rgb_ptr[2] = *row;

      gtk_preview_draw_row (GTK_PREVIEW (preview), rgb, x, y, w);
      break;

    case 3:
      gtk_preview_draw_row (GTK_PREVIEW (preview), row, x, y, w);
      break;

    case 4:
      for (i = 0, rgb_ptr = rgb; i < w; i++, row += 4, rgb_ptr += 3)
	{
	  rgb_ptr[0] = row[0];
          rgb_ptr[1] = row[1];
          rgb_ptr[2] = row[2];
	}

      gtk_preview_draw_row (GTK_PREVIEW (preview), rgb, x, y, w);
      break;
    }
  g_free (rgb);
}


static void
destripe_rect (gint      sel_x1,
	       gint      sel_y1,
	       gint      sel_x2,
	       gint      sel_y2,
	       gboolean  do_preview)
{
  GPixelRgn src_rgn;	/* source image region */
  GPixelRgn dst_rgn;	/* destination image region */
  guchar *src_rows;	/* image data */
  double progress, progress_inc;
  int sel_width = sel_x2 - sel_x1;
  int sel_height = sel_y2 - sel_y1;
  long *hist, *corr;	/* "histogram" data */
  int tile_width = gimp_tile_width ();
  int i, x, y, ox, cols;
  
  /* initialize */

  progress = 0.0;
  progress_inc = 0.0;

  /*
   * Let the user know what we're doing...
   */

  if (!do_preview)
    {
      gimp_progress_init (_("Destriping..."));

      progress = 0;
      progress_inc = 0.5 * tile_width / sel_width;
    }

  /*
   * Setup for filter...
   */

  gimp_pixel_rgn_init (&src_rgn, drawable,
		       sel_x1, sel_y1, sel_width, sel_height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dst_rgn, drawable,
		       sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);
  
  hist = g_new (long, sel_width * img_bpp);
  corr = g_new (long, sel_width * img_bpp);
  src_rows = g_new (guchar, tile_width * sel_height * img_bpp);

  memset (hist, 0, sel_width * img_bpp * sizeof (long));
  
  /*
   * collect "histogram" data.
   */

  for (ox = sel_x1; ox < sel_x2; ox += tile_width)
    {
      guchar *rows = src_rows;

      cols = sel_x2 - ox;
      if (cols > tile_width)
        cols = tile_width;

      gimp_pixel_rgn_get_rect (&src_rgn, rows, ox, sel_y1, cols, sel_height);

      for (y = 0; y < sel_height; y++)
        {
          long *h = hist + (ox - sel_x1) * img_bpp;
          guchar *row_end = rows + cols * img_bpp;

          while (rows < row_end)
            *h++ += *rows++;
        }

      if (!do_preview)
        gimp_progress_update (progress += progress_inc);
    }
  
  /*
   * average out histogram
   */
  
  if (1)
    {
      gint extend = (avg_width >> 1) * img_bpp;

      for (i = 0; i < MIN (3, img_bpp); i++)
        {
          long *h = hist - extend + i;
          long *c = corr - extend + i;
          long sum = 0;
          gint cnt = 0;

          for (x = -extend; x < sel_width * img_bpp; x += img_bpp)
            {
              if (x + extend < sel_width * img_bpp)
		{
		  sum += h[ extend]; cnt++;
		}
              if (x - extend >= 0)
		{
		  sum -= h[-extend]; cnt--;
		}
              if (x >= 0)
		{
		  *c = ((sum / cnt - *h) << 10) / *h;
		}

              h += img_bpp;
              c += img_bpp;
            }
        }
    }
  else
    {
      for (i = 0; i < MIN (3, img_bpp); i++)
        {
          long *h = hist + i + sel_width * img_bpp - img_bpp;
          long *c = corr + i + sel_width * img_bpp - img_bpp;
          long i = *h;
          *c = 0;

          do
            {
              h -= img_bpp;
              c -= img_bpp;

              if (*h - i > avg_width && i - *h > avg_width)
                i = *h;

              *c = (i-128) << 10 / *h;
            }
          while (h > hist);
        }
    }

  /*
   * remove stripes.
   */

  for (ox = sel_x1; ox < sel_x2; ox += tile_width)
    {
      guchar *rows = src_rows;

      cols = sel_x2 - ox;
      if (cols > tile_width)
        cols = tile_width;

      gimp_pixel_rgn_get_rect (&src_rgn, rows, ox, sel_y1, cols, sel_height);

      if (!do_preview)
        gimp_progress_update (progress += progress_inc);

      for (y = 0; y < sel_height; y++)
        {
          long *c = corr + (ox - sel_x1) * img_bpp;
          guchar *row_end = rows + cols * img_bpp;

          if (histogram)
            while (rows < row_end)
              {
                *rows = MIN (255, MAX (0, 128 + (*rows * *c >> 10)));
                c++; rows++;
              }
          else
            while (rows < row_end)
              {
                *rows = MIN (255, MAX (0, *rows + (*rows * *c >> 10) ));
                c++; rows++;
              }

          if (do_preview)
            preview_draw_row (ox - sel_x1, y, cols, rows - cols * img_bpp);
        }

      if (!do_preview)
        {
          gimp_pixel_rgn_set_rect (&dst_rgn, src_rows,
				   ox, sel_y1, cols, sel_height);
          gimp_progress_update (progress += progress_inc);
        }
    }

  g_free (src_rows);

  /*
   * Update the screen...
   */

  if (do_preview)
    {
      gtk_widget_draw (preview, NULL);
      gdk_flush ();
    }
  else
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->id, TRUE);
      gimp_drawable_update (drawable->id, sel_x1, sel_y1, sel_width, sel_height);
    }
  g_free (hist);
  g_free (corr);
}

/*
 * 'destripe()' - Destripe an image.
 *
 */

static void
destripe (void)
{
  destripe_rect (sel_x1, sel_y1, sel_x2, sel_y2, FALSE);
}

static gint
destripe_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *ptable;
  GtkWidget *ftable;
  GtkWidget *frame;
  GtkWidget *scrollbar;
  GtkWidget *button;
  GtkObject *adj;
  gint     argc;
  gchar  **argv;
  guchar  *color_cube;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("destripe");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  gdk_set_use_xshm (gimp_use_xshm ());

#ifdef SIGBUS
  signal (SIGBUS, SIG_DFL);
#endif
  signal (SIGSEGV, SIG_DFL);

  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  dialog = gimp_dialog_new (_("Destripe"), "destripe",
			    gimp_plugin_help_func, "filters/destripe.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("OK"), dialog_ok_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*
   * Top-level table for dialog...
   */

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table,
		      FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Preview window...
   */

  ptable = gtk_table_new (2, 2, FALSE);
  gtk_table_attach (GTK_TABLE (table), ptable, 0, 2, 0, 1,
		    0, 0, 0, 0);
  gtk_widget_show (ptable);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(ptable), frame, 0, 1, 0, 1,
		   0, 0, 0, 0);
  gtk_widget_show (frame);

  preview_width  = MIN (sel_x2 - sel_x1, PREVIEW_SIZE);
  preview_height = MIN (sel_y2 - sel_y1, PREVIEW_SIZE);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  hscroll_data = gtk_adjustment_new (0, 0, sel_x2 - sel_x1 - 1, 1.0,
				     MIN (preview_width, sel_x2 - sel_x1),
				     MIN (preview_width, sel_x2 - sel_x1));

  gtk_signal_connect (hscroll_data, "value_changed",
		      GTK_SIGNAL_FUNC (preview_scroll_callback),
		      NULL);

  scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (hscroll_data));
  gtk_range_set_update_policy (GTK_RANGE (scrollbar), GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (ptable), scrollbar, 0, 1, 1, 2,
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (scrollbar);

  vscroll_data = gtk_adjustment_new (0, 0, sel_y2 - sel_y1 - 1, 1.0,
				     MIN (preview_height, sel_y2 - sel_y1),
				     MIN (preview_height, sel_y2 - sel_y1));

  gtk_signal_connect (vscroll_data, "value_changed",
		      GTK_SIGNAL_FUNC (preview_scroll_callback),
		      NULL);

  scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (vscroll_data));
  gtk_range_set_update_policy (GTK_RANGE (scrollbar), GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (ptable), scrollbar, 1, 2, 0, 1, 0,
		    GTK_FILL, 0, 0);
  gtk_widget_show (scrollbar);

  preview_init ();

  /*
   * Filter type controls...
   */

  ftable = gtk_table_new (4, 1, FALSE);
  gtk_table_attach (GTK_TABLE (table), ftable, 2, 3, 0, 1,
		    0, 0, 0, 0);
  gtk_widget_show (ftable);

  button = gtk_check_button_new_with_label (_("Histogram"));
  gtk_table_attach (GTK_TABLE (ftable), button, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				histogram ? TRUE : FALSE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (dialog_histogram_callback),
		      NULL);
  gtk_widget_show (button);

/*  button = gtk_check_button_new_with_label("Recursive");
  gtk_table_attach(GTK_TABLE(ftable), button, 0, 1, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                               (filter_type & FILTER_RECURSIVE) ? TRUE : FALSE);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     (GtkSignalFunc)dialog_recursive_callback,
		     NULL);
  gtk_widget_show(button);*/

  /*
   * Box size (radius) control...
   */

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Width:"), SCALE_WIDTH, 0,
			      avg_width, 2, MAX_AVG, 1, 10, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (dialog_iscale_update),
		      &avg_width);

  gtk_widget_show (dialog);

  preview_update ();

  gtk_main ();
  gdk_flush ();

  preview_exit ();

  return run_filter;
}

/*  preview functions  */

static void
preview_init (void)
{
  gint width;  /* Byte width of the image */

  /*
   * Setup for preview filter...
   */
  width = preview_width * img_bpp;

  preview_x1 = sel_x1;
  preview_y1 = sel_y1;
  preview_x2 = preview_x1 + MIN (preview_width, sel_x2 - sel_x1);
  preview_y2 = preview_y1 + MIN (preview_height, sel_y2 -sel_y1);
}

static void
preview_scroll_callback (void)
{
  preview_x1 = sel_x1 + GTK_ADJUSTMENT (hscroll_data)->value;
  preview_y1 = sel_y1 + GTK_ADJUSTMENT (vscroll_data)->value;
  preview_x2 = preview_x1 + MIN (preview_width, sel_x2 - sel_x1);
  preview_y2 = preview_y1 + MIN (preview_height, sel_y2 - sel_y1);

  preview_update ();
}

static void
preview_update (void)
{
  destripe_rect (preview_x1, preview_y1, preview_x2, preview_y2, TRUE);
}

static void
preview_exit (void)
{
}

/*  dialog callbacks  */

static void
dialog_histogram_callback (GtkWidget *widget,
			   gpointer  data)
{
  histogram = !histogram;
  preview_update ();
}

static void
dialog_iscale_update (GtkAdjustment *adjustment,
                      gint          *value)
{
  gimp_int_adjustment_update (adjustment, value);

  preview_update ();
}

static void
dialog_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  run_filter = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
