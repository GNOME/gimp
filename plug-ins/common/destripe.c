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
 *   dialog_create_ivalue()      - Create an integer value control...
 *   dialog_iscale_update()      - Update the value field using the scale.
 *   dialog_ientry_update()      - Update the value field using the text entry.
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

#define PLUG_IN_NAME		"plug_in_destripe"
#define PLUG_IN_VERSION		"0.2"
#define PREVIEW_SIZE		200
#define SCALE_WIDTH		140
#define ENTRY_WIDTH		40
#define MAX_AVG			100

/*
 * Local functions...
 */

static void	query (void);
static void	run   (char *,
		       int,
		       GParam *,
		       int *,
		       GParam **);

static void	destripe (void);

static gint	destripe_dialog      (void);
static void	dialog_create_ivalue (char *, GtkTable *, int, gint *, int, int);
static void	dialog_iscale_update (GtkAdjustment *, gint *);
static void	dialog_ientry_update (GtkWidget *, gint *);
static void	dialog_ok_callback   (GtkWidget *, gpointer);

static void	preview_init            (void);
static void	preview_exit            (void);
static void	preview_update          (void);
static void	preview_scroll_callback (void);


/*
 * Globals...
 */

GPlugInInfo	PLUG_IN_INFO =
		{
		  NULL,		/* init_proc */
		  NULL,		/* quit_proc */
		  query,	/* query_proc */
		  run		/* run_proc */
		};

GtkWidget	*preview;		/* Preview widget */
int		preview_width,		/* Width of preview widget */
		preview_height,		/* Height of preview widget */
		preview_x1,		/* Upper-left X of preview */
		preview_y1,		/* Upper-left Y of preview */
		preview_x2,		/* Lower-right X of preview */
		preview_y2;		/* Lower-right Y of preview */
GtkObject	*hscroll_data,		/* Horizontal scrollbar data */
		*vscroll_data;		/* Vertical scrollbar data */

GDrawable	*drawable = NULL;	/* Current image */
int		sel_x1,			/* Selection bounds */
		sel_y1,
		sel_x2,
		sel_y2;
int		histogram = FALSE;
int		img_bpp;		/* Bytes-per-pixel in image */
gint		run_filter = FALSE;	/* True if we should run the filter */

int		avg_width = 36;


/*
 * 'main()' - Main entry - just call gimp_main()...
 */

MAIN()

/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query(void)
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

  gimp_install_procedure(PLUG_IN_NAME,
      _("Destripe filter, used to remove vertical stripes caused by cheap scanners."),
      _("This plug-in tries to remove vertical stripes from an image."),
      "Marc Lehmann <pcg@goof.com>", "Marc Lehmann <pcg@goof.com>",
      PLUG_IN_VERSION,
      N_("<Image>/Filters/Enhance/Destripe..."),
      "RGB*, GRAY*",
      PROC_PLUG_IN, nargs, nreturn_vals, args, return_vals);
}


/*
 * 'run()' - Run the filter...
 */

static void
run(char   *name,		/* I - Name of filter program. */
    int    nparams,		/* I - Number of parameters passed in */
    GParam *param,		/* I - Parameter values */
    int    *nreturn_vals,	/* O - Number of return values */
    GParam **return_vals)	/* O - Return values */
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

  drawable = gimp_drawable_get(param[2].data.d_drawable);

  gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  img_bpp       = gimp_drawable_bpp(drawable->id);

 /*
  * See how we will run
  */

  switch (run_mode)
  {
    case RUN_INTERACTIVE :
       /*
        * Possibly retrieve data...
        */

        gimp_get_data(PLUG_IN_NAME, &avg_width);

       /*
        * Get information from the dialog...
        */

	if (!destripe_dialog())
          return;
        break;

    case RUN_NONINTERACTIVE :
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

	gimp_get_data(PLUG_IN_NAME, &avg_width);
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
    if ((gimp_drawable_is_rgb(drawable->id) ||
	 gimp_drawable_is_gray(drawable->id)))
    {
     /*
      * Set the tile cache size...
      */

      gimp_tile_cache_ntiles((drawable->width + gimp_tile_width() - 1) /
                             gimp_tile_width());

     /*
      * Run!
      */

      destripe();

     /*
      * If run mode is interactive, flush displays...
      */

      if (run_mode != RUN_NONINTERACTIVE)
        gimp_displays_flush();

     /*
      * Store data...
      */

      if (run_mode == RUN_INTERACTIVE)
        gimp_set_data(PLUG_IN_NAME, &avg_width, sizeof(avg_width));
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

  gimp_drawable_detach(drawable);
}

static inline void
preview_draw_row (int x, int y, int w, guchar *row)
{
  guchar *rgb = g_new(guchar, w * 3);
  guchar *rgb_ptr;
  int i;
  
  switch (img_bpp)
  {
    case 1:
    case 2:
        for (i = 0, rgb_ptr = rgb; i < w; i++, row += img_bpp, rgb_ptr += 3)
          rgb_ptr[0] = rgb_ptr[1] = rgb_ptr[2] = *row;

	gtk_preview_draw_row(GTK_PREVIEW(preview), rgb, x, y, w);
        break;

    case 3:
	gtk_preview_draw_row(GTK_PREVIEW(preview), row, x, y, w);
        break;

    case 4:
        for (i = 0, rgb_ptr = rgb; i < w; i++, row += 4, rgb_ptr += 3)
          rgb_ptr[0] = row[0],
          rgb_ptr[1] = row[1],
          rgb_ptr[2] = row[2];

	gtk_preview_draw_row(GTK_PREVIEW(preview), rgb, x, y, w);
        break;
  }
  g_free(rgb);
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

static void
dialog_histogram_callback (GtkWidget *widget,	/* I - Toggle button */
			   gpointer  data)	/* I - Data */
{
  histogram = !histogram;
  preview_update ();
}

/*
 * 'destripe_dialog()' - Popup a dialog window for the filter box size...
 */

static gint
destripe_dialog (void)
{
  GtkWidget *dialog;     /* Dialog window */
  GtkWidget *table;      /* Table "container" for controls */
  GtkWidget *ptable;     /* Preview table */
  GtkWidget *ftable;     /* Filter table */
  GtkWidget *frame;      /* Frame for preview */
  GtkWidget *scrollbar;  /* Horizontal + vertical scroller */
  GtkWidget *button;
  gint     argc;         /* Fake argc for GUI */
  gchar  **argv;         /* Fake argv for GUI */
  guchar  *color_cube;	 /* Preview color cube... */

  /*
   * Initialize the program's display...
   */

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

  /*
   * Dialog window...
   */

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
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
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

  dialog_create_ivalue (_("Width:"), GTK_TABLE (table), 2,
			&avg_width, 2, MAX_AVG);

  /*
   * Show it and wait for the user to do something...
   */

  gtk_widget_show (dialog);

  preview_update ();

  gtk_main ();
  gdk_flush ();

  /*
   * Free the preview data...
   */

  preview_exit ();

  /*
   * Return ok/cancel...
   */

  return run_filter;
}


/*
 * 'preview_init()' - Initialize the preview window...
 */

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


/*
 * 'preview_scroll_callback()' - Update the preview when a scrollbar is moved.
 */

static void
preview_scroll_callback (void)
{
  preview_x1 = sel_x1 + GTK_ADJUSTMENT (hscroll_data)->value;
  preview_y1 = sel_y1 + GTK_ADJUSTMENT (vscroll_data)->value;
  preview_x2 = preview_x1 + MIN (preview_width, sel_x2 - sel_x1);
  preview_y2 = preview_y1 + MIN (preview_height, sel_y2 - sel_y1);

  preview_update ();
}


/*
 * 'preview_update()' - Update the preview window.
 */

static void
preview_update (void)
{
  destripe_rect (preview_x1, preview_y1, preview_x2, preview_y2, TRUE);
}


/*
 * 'preview_exit()' - Free all memory used by the preview window...
 */

static void
preview_exit (void)
{
}


/*
 * 'dialog_create_ivalue()' - Create an integer value control...
 */

static void
dialog_create_ivalue (char     *title,	/* I - Label for control */
		      GtkTable *table,	/* I - Table container to use */
		      int      row,	/* I - Row # for container */
		      gint     *value,	/* I - Value holder */
		      int      left,	/* I - Minimum value for slider */
		      int      right)	/* I - Maximum value for slider */
{
  GtkWidget	*label,		/* Control label */
		*scale,		/* Scale widget */
		*entry;		/* Text widget */
  GtkObject	*scale_data;	/* Scale data */
  gchar		buf[256];	/* String buffer */


  /*
   * Label...
   */

  label = gtk_label_new(title);
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(table, label, 0, 1, row, row + 1,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  /*
   * Scale...
   */

  scale_data = gtk_adjustment_new(*value, left, right, 1.0, 1.0, 1.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc) dialog_iscale_update,
		     value);

  scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
  gtk_table_attach(table, scale, 1, 2, row, row + 1,
		   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

 /*
  * Text entry...
  */

  entry = gtk_entry_new();
  gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
  gtk_object_set_user_data(scale_data, entry);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  g_snprintf(buf, sizeof (buf), "%d", *value);
  gtk_entry_set_text(GTK_ENTRY(entry), buf);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc) dialog_ientry_update,
		     value);
  gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(entry);
}


/*
 * 'dialog_iscale_update()' - Update the value field using the scale.
 */

static void
dialog_iscale_update (GtkAdjustment *adjustment, /* I - New value */
                      gint          *value)      /* I - Current value */
{
  GtkWidget *entry;     /* Text entry widget */
  gchar      buf[256];  /* Text buffer */

  if (*value != adjustment->value)
    {
      *value = adjustment->value;

      entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
      g_snprintf(buf, sizeof (buf), "%d", *value);

      gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
      gtk_entry_set_text(GTK_ENTRY(entry), buf);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);

      preview_update();
    };
}


/*
 * 'dialog_ientry_update()' - Update the value field using the text entry.
 */

static void
dialog_ientry_update(GtkWidget *widget,	/* I - Entry widget */
                     gint      *value)	/* I - Current value */
{
  GtkAdjustment	*adjustment;
  gint		new_value;

  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (*value != new_value)
    {
      adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));

      if ((new_value >= adjustment->lower) &&
	  (new_value <= adjustment->upper))
	{
	  *value            = new_value;
	  adjustment->value = new_value;

	  gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");

	  preview_update();
	};
    };
}

/*
 * 'dialog_ok_callback()' - Start the filter...
 */

static void
dialog_ok_callback (GtkWidget *widget,	/* I - OK button widget */
		    gpointer   data)	/* I - Dialog window */
{
  run_filter = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
