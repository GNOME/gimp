/*
 * "$Id$"
 *
 *   Sharpen filters for The GIMP -- an image manipulation program
 *
 *   Copyright 1997-1998 Michael Sweet (mike@easysw.com)
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
 *   main()                    - Main entry - just call gimp_main()...
 *   query()                   - Respond to a plug-in query...
 *   run()                     - Run the filter...
 *   sharpen()                 - Sharpen an image using a median filter.
 *   sharpen_dialog()          - Popup a dialog window for the filter box size...
 *   preview_init()            - Initialize the preview window...
 *   preview_scroll_callback() - Update the preview when a scrollbar is moved.
 *   preview_update()          - Update the preview window.
 *   preview_exit()            - Free all memory used by the preview window...
 *   dialog_iscale_update()    - Update the value field using the scale.
 *   dialog_ok_callback()      - Start the filter...
 *   gray_filter()             - Sharpen grayscale pixels.
 *   graya_filter()            - Sharpen grayscale+alpha pixels.
 *   rgb_filter()              - Sharpen RGB pixels.
 *   rgba_filter()             - Sharpen RGBA pixels.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/*
 * Constants...
 */

#define PLUG_IN_NAME		"plug_in_sharpen"
#define PLUG_IN_VERSION		"1.4.2 - 3 June 1998"
#define PREVIEW_SIZE		128
#define SCALE_WIDTH		100

/*
 * Local functions...
 */

static void	query (void);
static void	run   (gchar   *name,
		       gint     nparams,
		       GParam  *param,
		       gint    *nreturn_vals,
		       GParam **returm_vals);

static void	compute_luts   (void);
static void	sharpen        (void);

static gint	sharpen_dialog (void);

static void	dialog_iscale_update (GtkAdjustment *, gint *);
static void	dialog_ok_callback   (GtkWidget *, gpointer);

static void	preview_init            (void);
static void	preview_exit            (void);
static void	preview_update          (void);
static void	preview_scroll_callback (void);

typedef gint32 intneg;
typedef gint32 intpos;

static void	gray_filter  (int width, guchar *src, guchar *dst, intneg *neg0,
			      intneg *neg1, intneg *neg2);
static void	graya_filter (int width, guchar *src, guchar *dst, intneg *neg0,
			      intneg *neg1, intneg *neg2);
static void	rgb_filter   (int width, guchar *src, guchar *dst, intneg *neg0,
			      intneg *neg1, intneg *neg2);
static void	rgba_filter  (int width, guchar *src, guchar *dst, intneg *neg0,
			      intneg *neg1, intneg *neg2);


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

static GtkWidget *preview;		/* Preview widget */
static gint       preview_width;	/* Width of preview widget */
static gint       preview_height;	/* Height of preview widget */
static gint       preview_x1;		/* Upper-left X of preview */
static gint       preview_y1;		/* Upper-left Y of preview */
static gint       preview_x2;		/* Lower-right X of preview */
static gint       preview_y2;		/* Lower-right Y of preview */
static guchar    *preview_src;		/* Source pixel image */
static intneg    *preview_neg;		/* Negative coefficient pixels */
static guchar    *preview_dst;		/* Destination pixel image */
static guchar    *preview_image;	/* Preview RGB image */
static GtkObject *hscroll_data;		/* Horizontal scrollbar data */
static GtkObject *vscroll_data;		/* Vertical scrollbar data */

static GDrawable *drawable = NULL;	/* Current image */
static gint       sel_x1;      		/* Selection bounds */
static gint       sel_y1;
static gint       sel_x2;
static gint       sel_y2;
static gint       sel_width;		/* Selection width */
static gint       sel_height;		/* Selection height */
static gint       img_bpp;		/* Bytes-per-pixel in image */
static gint       sharpen_percent = 10;	/* Percent of sharpening */
static gint       run_filter = FALSE;	/* True if we should run the filter */

static intneg     neg_lut[256];		/* Negative coefficient LUT */
static intpos     pos_lut[256];		/* Positive coefficient LUT */


MAIN ()

static void
query (void)
{
  static GParamDef	args[] =
  {
    { PARAM_INT32,	"run_mode",	"Interactive, non-interactive" },
    { PARAM_IMAGE,	"image",	"Input image" },
    { PARAM_DRAWABLE,	"drawable",	"Input drawable" },
    { PARAM_INT32,	"percent",	"Percent sharpening (default = 10)" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure (PLUG_IN_NAME,
			  "Sharpen filter, typically used to 'sharpen' a photographic image.",
			  "This plug-in selectively performs a convolution filter on an image.",
			  "Michael Sweet <mike@easysw.com>",
			  "Copyright 1997-1998 by Michael Sweet",
			  PLUG_IN_VERSION,
			  N_("<Image>/Filters/Enhance/Sharpen..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  GRunModeType	run_mode;	/* Current run mode */
  GStatusType	status;		/* Return status */
  GParam	*values;	/* Return values */

  /*
   * Initialize parameter data...
   */

  status   = STATUS_SUCCESS;
  run_mode = param[0].data.d_int32;

  values = g_new (GParam, 1);

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  /*
   * Get drawable information...
   */

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  gimp_drawable_mask_bounds (drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width     = sel_x2 - sel_x1;
  sel_height    = sel_y2 - sel_y1;
  img_bpp       = gimp_drawable_bpp (drawable->id);

  /*
   * See how we will run
   */

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_NAME, &sharpen_percent);

      /*
       * Get information from the dialog...
       */
      if (!sharpen_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*
       * Make sure all the arguments are present...
       */
      if (nparams != 4)
	status = STATUS_CALLING_ERROR;
      else
	sharpen_percent = param[3].data.d_int32;
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_NAME, &sharpen_percent);
      break;

    default:
      status = STATUS_CALLING_ERROR;
      break;;
    };

  /*
   * Sharpen the image...
   */

  if (status == STATUS_SUCCESS)
    {
      if ((gimp_drawable_is_rgb (drawable->id) ||
	   gimp_drawable_is_gray (drawable->id)))
	{
	  /*
	   * Set the tile cache size...
	   */
	  gimp_tile_cache_ntiles (2 * (drawable->width + gimp_tile_width() - 1) /
				  gimp_tile_width() + 1);

	  /*
	   * Run!
	   */
	  sharpen ();

	  /*
	   * If run mode is interactive, flush displays...
	   */
	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /*
	   * Store data...
	   */
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data (PLUG_IN_NAME,
			   &sharpen_percent, sizeof (sharpen_percent));
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


static void
compute_luts (void)
{
  gint i;	/* Looping var */
  gint fact;	/* 1 - sharpness */

  fact = 100 - sharpen_percent;
  if (fact < 1)
    fact = 1;

  for (i = 0; i < 256; i ++)
    {
      pos_lut[i] = 800 * i / fact;
      neg_lut[i] = (4 + pos_lut[i] - (i << 3)) >> 3;
    };
}

/*
 * 'sharpen()' - Sharpen an image using a convolution filter.
 */

static void
sharpen (void)
{
  GPixelRgn	src_rgn,	/* Source image region */
		dst_rgn;	/* Destination image region */
  guchar	*src_rows[4],	/* Source pixel rows */
		*src_ptr,	/* Current source pixel */
		*dst_row;	/* Destination pixel row */
  intneg	*neg_rows[4],	/* Negative coefficient rows */
		*neg_ptr;	/* Current negative coefficient */
  gint		i,		/* Looping vars */
		y,		/* Current location in image */
		row,		/* Current row in src_rows */
		count,		/* Current number of filled src_rows */
		width;		/* Byte width of the image */
  void		(*filter)(int, guchar *, guchar *, intneg *, intneg *, intneg *);

  filter = NULL;

  /*
   * Let the user know what we're doing...
   */
  gimp_progress_init( _("Sharpening..."));

  /*
   * Setup for filter...
   */

  gimp_pixel_rgn_init (&src_rgn, drawable,
		       sel_x1, sel_y1, sel_width, sel_height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dst_rgn, drawable,
		       sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);

  compute_luts ();

  width = sel_width * img_bpp;

  for (row = 0; row < 4; row ++)
    {
      src_rows[row] = g_new (guchar, width);
      neg_rows[row] = g_new (intneg, width);
    };

  dst_row = g_new (guchar, width);

  /*
   * Pre-load the first row for the filter...
   */

  gimp_pixel_rgn_get_row (&src_rgn, src_rows[0], sel_x1, sel_y1, sel_width);
  for (i = width, src_ptr = src_rows[0], neg_ptr = neg_rows[0];
       i > 0;
       i --, src_ptr ++, neg_ptr ++)
    *neg_ptr = neg_lut[*src_ptr];

  row   = 1;
  count = 1;

  /*
   * Select the filter...
   */

  switch (img_bpp)
    {
    case 1 :
      filter = gray_filter;
      break;
    case 2 :
      filter = graya_filter;
      break;
    case 3 :
      filter = rgb_filter;
      break;
    case 4 :
      filter = rgba_filter;
      break;
    };

  /*
   * Sharpen...
   */

  for (y = sel_y1; y < sel_y2; y ++)
    {
      /*
       * Load the next pixel row...
       */

      if ((y + 1) < sel_y2)
	{
	  /*
	   * Check to see if our src_rows[] array is overflowing yet...
	   */

	  if (count >= 3)
	    count --;

	  /*
	   * Grab the next row...
	   */

	  gimp_pixel_rgn_get_row (&src_rgn, src_rows[row],
				  sel_x1, y + 1, sel_width);
	  for (i = width, src_ptr = src_rows[row], neg_ptr = neg_rows[row];
	       i > 0;
	       i --, src_ptr ++, neg_ptr ++)
	    *neg_ptr = neg_lut[*src_ptr];

	  count ++;
	  row = (row + 1) & 3;
	}
      else
	{
	  /*
	   * No more pixels at the bottom...  Drop the oldest samples...
	   */

	  count --;
	};

      /*
       * Now sharpen pixels and save the results...
       */

      if (count == 3)
	{
	  (* filter) (sel_width, src_rows[(row + 2) & 3], dst_row,
		      neg_rows[(row + 1) & 3] + img_bpp,
		      neg_rows[(row + 2) & 3] + img_bpp,
		      neg_rows[(row + 3) & 3] + img_bpp);

	  /*
	   * Set the row...
	   */

	  gimp_pixel_rgn_set_row (&dst_rgn, dst_row, sel_x1, y, sel_width);
	}
      else if (count == 2)
	{
	  if (y == sel_y1)
	    gimp_pixel_rgn_set_row (&dst_rgn, src_rows[0], sel_x1, y, sel_width);
	  else
	    gimp_pixel_rgn_set_row (&dst_rgn, src_rows[2], sel_x1, y, sel_width);
	};

      if ((y & 15) == 0)
	gimp_progress_update ((gdouble) (y - sel_y1) / (gdouble) sel_height);
    };

  /*
   * OK, we're done.  Free all memory used...
   */

  for (row = 0; row < 4; row ++)
    {
      g_free (src_rows[row]);
      g_free (neg_rows[row]);
    };

  g_free (dst_row);

  /*
   * Update the screen...
   */

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, sel_x1, sel_y1, sel_width, sel_height);
}


/*
 * 'sharpen_dialog()' - Popup a dialog window for the filter box size...
 */

static gint
sharpen_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *abox;
  GtkWidget *ptable;
  GtkWidget *frame;
  GtkWidget *scrollbar;
  GtkObject *adj;
  gchar *title;

  gimp_ui_init ("sharpen", TRUE);

  title = g_strdup_printf (_("Sharpen - %s"), PLUG_IN_VERSION);

  dialog = gimp_dialog_new (title, "sharpen",
			    gimp_plugin_help_func, "filters/sharpen.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("OK"), dialog_ok_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  g_free (title);

  gtk_signal_connect (GTK_OBJECT(dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*
   * Top-level table for dialog...
   */

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /*
   * Preview window...
   */

  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  ptable = gtk_table_new (2, 2, FALSE);
  gtk_container_add (GTK_CONTAINER (abox), ptable);
  gtk_widget_show (ptable);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (ptable), frame, 0, 1, 0, 1, 0, 0, 0, 0);
  gtk_widget_show (frame);

  preview_width  = MIN (sel_width, PREVIEW_SIZE);
  preview_height = MIN (sel_height, PREVIEW_SIZE);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  hscroll_data = gtk_adjustment_new (0, 0, sel_width - 1, 1.0,
				     MIN (preview_width, sel_width),
				     MIN (preview_width, sel_width));

  gtk_signal_connect (hscroll_data, "value_changed",
		      (GtkSignalFunc) preview_scroll_callback,
		      NULL);

  scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (hscroll_data));
  gtk_range_set_update_policy (GTK_RANGE (scrollbar), GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (ptable), scrollbar, 0, 1, 1, 2,
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (scrollbar);

  vscroll_data = gtk_adjustment_new (0, 0, sel_height - 1, 1.0,
				     MIN (preview_height, sel_height),
				     MIN (preview_height, sel_height));

  gtk_signal_connect (vscroll_data, "value_changed",
		      GTK_SIGNAL_FUNC (preview_scroll_callback),
		      NULL);

  scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (vscroll_data));
  gtk_range_set_update_policy (GTK_RANGE (scrollbar), GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (ptable), scrollbar, 1, 2, 0, 1,
		    0, GTK_FILL, 0, 0);
  gtk_widget_show (scrollbar);

  preview_init ();

  /*
   * Sharpness control...
   */

  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Sharpness:"), SCALE_WIDTH, 0,
			      sharpen_percent, 1, 99, 1, 10, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (dialog_iscale_update),
		      &sharpen_percent);

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

  compute_luts();

  width = preview_width * img_bpp;

  preview_src   = g_new (guchar, width * preview_height);
  preview_neg   = g_new (intneg, width * preview_height);
  preview_dst   = g_new (guchar, width * preview_height);
  preview_image = g_new (guchar, preview_width * preview_height * 3);

  preview_x1 = sel_x1;
  preview_y1 = sel_y1;
  preview_x2 = preview_x1 + preview_width;
  preview_y2 = preview_y1 + preview_height;
}

static void
preview_scroll_callback (void)
{
  preview_x1 = sel_x1 + GTK_ADJUSTMENT (hscroll_data)->value;
  preview_y1 = sel_y1 + GTK_ADJUSTMENT (vscroll_data)->value;
  preview_x2 = preview_x1 + MIN (preview_width, sel_width);
  preview_y2 = preview_y1 + MIN (preview_height, sel_height);

  preview_update ();
}

static void
preview_update (void)
{
  GPixelRgn	src_rgn;	/* Source image region */
  guchar	*src_ptr,	/* Current source pixel */
		*dst_ptr,	/* Current destination pixel */
  		*image_ptr;	/* Current image pixel */
  intneg	*neg_ptr;	/* Current negative pixel */
  guchar	check;		/* Current check mark pixel */
  gint		i,	  	/* Looping var */
		x, y,		/* Current location in image */
		width;		/* Byte width of the image */
  void		(*filter)(int, guchar *, guchar *, intneg *, intneg *, intneg *);

  filter = NULL;

  /*
   * Setup for filter...
   */

  gimp_pixel_rgn_init (&src_rgn, drawable,
		       preview_x1, preview_y1, preview_width, preview_height,
		       FALSE, FALSE);

  width = preview_width * img_bpp;

  /*
   * Load the preview area...
   */

  gimp_pixel_rgn_get_rect (&src_rgn, preview_src, preview_x1, preview_y1,
			   preview_width, preview_height);

  for (i = width * preview_height, src_ptr = preview_src, neg_ptr = preview_neg;
       i > 0;
       i --)
    *neg_ptr++ = neg_lut[*src_ptr++];

  /*
   * Select the filter...
   */

  switch (img_bpp)
    {
    case 1:
      filter = gray_filter;
      break;
    case 2:
      filter = graya_filter;
      break;
    case 3:
      filter = rgb_filter;
      break;
    case 4:
      filter = rgba_filter;
      break;
    };

  /*
   * Sharpen...
   */

  memcpy (preview_dst, preview_src, width);
  memcpy (preview_dst + width * (preview_height - 1),
	  preview_src + width * (preview_height - 1),
	  width);

  for (y = preview_height - 2, src_ptr = preview_src + width,
	   neg_ptr = preview_neg + width + img_bpp,
	   dst_ptr = preview_dst + width;
       y > 0;
       y --, src_ptr += width, neg_ptr += width, dst_ptr += width)
    (*filter)(preview_width, src_ptr, dst_ptr, neg_ptr - width,
              neg_ptr, neg_ptr + width);

  /*
   * Fill the preview image buffer...
   */

  switch (img_bpp)
    {
    case 1:
      for (x = preview_width * preview_height, dst_ptr = preview_dst,
	     image_ptr = preview_image;
	   x > 0;
	   x --, dst_ptr ++, image_ptr += 3)
	image_ptr[0] = image_ptr[1] = image_ptr[2] = *dst_ptr;
      break;

    case 2:
      for (y = preview_height, dst_ptr = preview_dst,
	     image_ptr = preview_image;
	   y > 0;
	   y --)
	for (x = preview_width;
	     x > 0;
	     x --, dst_ptr += 2, image_ptr += 3)
	  if (dst_ptr[1] == 255)
	    image_ptr[0] = image_ptr[1] = image_ptr[2] = *dst_ptr;
	  else
	    {
              if ((y & GIMP_CHECK_SIZE) ^ (x & GIMP_CHECK_SIZE))
                check = GIMP_CHECK_LIGHT * 255;
              else
                check = GIMP_CHECK_DARK * 255;

              if (dst_ptr[1] == 0)
                image_ptr[0] = image_ptr[1] = image_ptr[2] = check;
              else
                image_ptr[0] = image_ptr[1] = image_ptr[2] =
		  check + ((dst_ptr[0] - check) * dst_ptr[1]) / 255;
	    };
      break;

    case 3:
      memcpy (preview_image, preview_dst, preview_width * preview_height * 3);
      break;

    case 4:
      for (y = preview_height, dst_ptr = preview_dst,
	     image_ptr = preview_image;
	   y > 0;
	   y --)
	for (x = preview_width;
	     x > 0;
	     x --, dst_ptr += 4, image_ptr += 3)
	  if (dst_ptr[3] == 255)
	    {
	      image_ptr[0] = dst_ptr[0];
	      image_ptr[1] = dst_ptr[1];
	      image_ptr[2] = dst_ptr[2];
	    }
	  else
	    {
              if ((y & GIMP_CHECK_SIZE) ^ (x & GIMP_CHECK_SIZE))
                check = GIMP_CHECK_LIGHT * 255;
              else
                check = GIMP_CHECK_DARK * 255;

              if (dst_ptr[3] == 0)
                image_ptr[0] = image_ptr[1] = image_ptr[2] = check;
              else
		{
		  image_ptr[0] =
		    check + ((dst_ptr[0] - check) * dst_ptr[3]) / 255;
		  image_ptr[1] =
		    check + ((dst_ptr[1] - check) * dst_ptr[3]) / 255;
		  image_ptr[2] =
		    check + ((dst_ptr[2] - check) * dst_ptr[3]) / 255;
		};
	    };
      break;
    };

  /*
   * Draw the preview image on the screen...
   */

  for (y = 0, image_ptr = preview_image;
       y < preview_height;
       y ++, image_ptr += preview_width * 3)
    gtk_preview_draw_row (GTK_PREVIEW (preview), image_ptr, 0, y,
			  preview_width);

  gtk_widget_draw (preview, NULL);
  gdk_flush ();
}

static void
preview_exit (void)
{
  g_free (preview_src);
  g_free (preview_neg);
  g_free (preview_dst);
  g_free (preview_image);
}

/*  dialog callbacks  */

static void
dialog_iscale_update (GtkAdjustment *adjustment,
		      gint          *value)
{
  gimp_int_adjustment_update (adjustment, value);

  compute_luts ();
  preview_update ();
}

static void
dialog_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  run_filter = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}


/*
 * 'gray_filter()' - Sharpen grayscale pixels.
 */

static void
gray_filter (gint    width,	/* I - Width of line in pixels */
	     guchar *src,	/* I - Source line */
	     guchar *dst,	/* O - Destination line */
	     intneg *neg0,	/* I - Top negative coefficient line */
	     intneg *neg1,	/* I - Middle negative coefficient line */
	     intneg *neg2)	/* I - Bottom negative coefficient line */
{
  intpos pixel;		/* New pixel value */

  *dst++ = *src++;
  width -= 2;

  while (width > 0)
    {
      pixel = (pos_lut[*src++] - neg0[-1] - neg0[0] - neg0[1] -
	       neg1[-1] - neg1[1] -
	       neg2[-1] - neg2[0] - neg2[1]);
      pixel = (pixel + 4) >> 3;
      if (pixel < 0)
	*dst++ = 0;
      else if (pixel < 255)
	*dst++ = pixel;
      else
	*dst++ = 255;

      neg0 ++;
      neg1 ++;
      neg2 ++;
      width --;
    };

  *dst++ = *src++;
}

/*
 * 'graya_filter()' - Sharpen grayscale+alpha pixels.
 */

static void
graya_filter (gint   width,	/* I - Width of line in pixels */
	      guchar *src,	/* I - Source line */
	      guchar *dst,	/* O - Destination line */
	      intneg *neg0,	/* I - Top negative coefficient line */
	      intneg *neg1,	/* I - Middle negative coefficient line */
	      intneg *neg2)	/* I - Bottom negative coefficient line */
{
  intpos pixel;		/* New pixel value */

  *dst++ = *src++;
  *dst++ = *src++;
  width -= 2;

  while (width > 0)
    {
      pixel = (pos_lut[*src++] - neg0[-2] - neg0[0] - neg0[2] -
	       neg1[-2] - neg1[2] -
	       neg2[-2] - neg2[0] - neg2[2]);
      pixel = (pixel + 4) >> 3;
      if (pixel < 0)
	*dst++ = 0;
      else if (pixel < 255)
	*dst++ = pixel;
      else
	*dst++ = 255;

      *dst++ = *src++;
      neg0 += 2;
      neg1 += 2;
      neg2 += 2;
      width --;
    };

  *dst++ = *src++;
  *dst++ = *src++;
}

/*
 * 'rgb_filter()' - Sharpen RGB pixels.
 */

static void
rgb_filter (gint    width,	/* I - Width of line in pixels */
	    guchar *src,	/* I - Source line */
	    guchar *dst,	/* O - Destination line */
	    intneg *neg0,	/* I - Top negative coefficient line */
	    intneg *neg1,	/* I - Middle negative coefficient line */
	    intneg *neg2)	/* I - Bottom negative coefficient line */
{
  intpos pixel;		/* New pixel value */

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  width -= 2;

  while (width > 0)
    {
      pixel = (pos_lut[*src++] - neg0[-3] - neg0[0] - neg0[3] -
	       neg1[-3] - neg1[3] -
	       neg2[-3] - neg2[0] - neg2[3]);
      pixel = (pixel + 4) >> 3;
      if (pixel < 0)
	*dst++ = 0;
      else if (pixel < 255)
	*dst++ = pixel;
      else
	*dst++ = 255;

      pixel = (pos_lut[*src++] - neg0[-2] - neg0[1] - neg0[4] -
	       neg1[-2] - neg1[4] -
	       neg2[-2] - neg2[1] - neg2[4]);
      pixel = (pixel + 4) >> 3;
      if (pixel < 0)
	*dst++ = 0;
      else if (pixel < 255)
	*dst++ = pixel;
      else
	*dst++ = 255;

      pixel = (pos_lut[*src++] - neg0[-1] - neg0[2] - neg0[5] -
	       neg1[-1] - neg1[5] -
	       neg2[-1] - neg2[2] - neg2[5]);
      pixel = (pixel + 4) >> 3;
      if (pixel < 0)
	*dst++ = 0;
      else if (pixel < 255)
	*dst++ = pixel;
      else
	*dst++ = 255;

      neg0 += 3;
      neg1 += 3;
      neg2 += 3;
      width --;
    };

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
}

/*
 * 'rgba_filter()' - Sharpen RGBA pixels.
 */

static void
rgba_filter (gint   width,	/* I - Width of line in pixels */
	     guchar *src,	/* I - Source line */
	     guchar *dst,	/* O - Destination line */
	     intneg *neg0,	/* I - Top negative coefficient line */
	     intneg *neg1,	/* I - Middle negative coefficient line */
	     intneg *neg2)	/* I - Bottom negative coefficient line */
{
  intpos pixel;		/* New pixel value */

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  width -= 2;

  while (width > 0)
    {
      pixel = (pos_lut[*src++] - neg0[-4] - neg0[0] - neg0[4] -
	       neg1[-4] - neg1[4] -
	       neg2[-4] - neg2[0] - neg2[4]);
      pixel = (pixel + 4) >> 3;
      if (pixel < 0)
	*dst++ = 0;
      else if (pixel < 255)
	*dst++ = pixel;
      else
	*dst++ = 255;

      pixel = (pos_lut[*src++] - neg0[-3] - neg0[1] - neg0[5] -
	       neg1[-3] - neg1[5] -
	       neg2[-3] - neg2[1] - neg2[5]);
      pixel = (pixel + 4) >> 3;
      if (pixel < 0)
	*dst++ = 0;
      else if (pixel < 255)
	*dst++ = pixel;
      else
	*dst++ = 255;

      pixel = (pos_lut[*src++] - neg0[-2] - neg0[2] - neg0[6] -
	       neg1[-2] - neg1[6] -
	       neg2[-2] - neg2[2] - neg2[6]);
      pixel = (pixel + 4) >> 3;
      if (pixel < 0)
	*dst++ = 0;
      else if (pixel < 255)
	*dst++ = pixel;
      else
	*dst++ = 255;

      *dst++ = *src++;

      neg0 += 4;
      neg1 += 4;
      neg2 += 4;
      width --;
    };

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
}
