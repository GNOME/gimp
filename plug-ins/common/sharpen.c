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
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the filter...
 *   sharpen()                   - Sharpen an image using a median filter.
 *   sharpen_dialog()            - Popup a dialog window for the filter box size...
 *   preview_init()              - Initialize the preview window...
 *   preview_scroll_callback()   - Update the preview when a scrollbar is moved.
 *   preview_update()            - Update the preview window.
 *   preview_exit()              - Free all memory used by the preview window...
 *   dialog_create_ivalue()      - Create an integer value control...
 *   dialog_iscale_update()      - Update the value field using the scale.
 *   dialog_ientry_update()      - Update the value field using the text entry.
 *   dialog_ok_callback()        - Start the filter...
 *   dialog_cancel_callback()    - Cancel the filter...
 *   dialog_close_callback()     - Exit the filter dialog application.
 *   gray_filter()               - Sharpen grayscale pixels.
 *   graya_filter()              - Sharpen grayscale+alpha pixels.
 *   rgb_filter()                - Sharpen RGB pixels.
 *   rgba_filter()               - Sharpen RGBA pixels.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.11  1999/10/17 00:07:40  pcg
 *   API PATCH #2 or so
 *
 *   Revision 1.10  1999/04/22 14:05:58  asbjoer
 *   use MAIN macro
 *
 *   Revision 1.9  1999/03/28 22:03:12  raph
 *   Fixed a silly bug causing sharpen to not work in the non-rgb cases.
 *
 *   Revision 1.8  1999/03/15 22:38:36  raph
 *   Improved the quality of the algorithm in the sharpen plugin.
 *
 *   Revision 1.7  1998/06/06 23:22:22  yosh
 *   * adding Lighting plugin
 *
 *   * updated despeckle, png, sgi, and sharpen
 *
 *   -Yosh
 *
 *   Revision 1.14  1998/05/17 16:01:33  mike
 *   Removed signal handler stuff used for debugging.
 *   Added gtk_rc_parse().
 *   Removed extra variables.
 *
 *   Revision 1.13  1998/04/27  15:55:38  mike
 *   Sharpen would shift the image down one pixel; was using the wrong "source"
 *   row...
 *
 *   Revision 1.12  1998/04/27  15:45:27  mike
 *   OK, put the shadow buffer stuff back in - without shadowing the undo stuff
 *   will *not* work...  sigh...
 *   Doubled tile cache to avoid cache thrashing with shadow buffer.
 *
 *   Revision 1.11  1998/04/27  15:33:45  mike
 *   Updated to use LUTs for coefficients.
 *   Broke out filter code for GRAY, GRAYA, RGB, RGBA modes.
 *   Fixed destination region code - was using a shadow buffer when it wasn't
 *   needed.
 *   Now add 1 to the number of tiles needed in the cache to avoid possible
 *   rounding error and resulting cache thrashing.
 *
 *   Revision 1.10  1998/04/23  14:39:47  mike
 *   Whoops - wasn't copying the preview image over for RGB mode...
 *
 *   Revision 1.9  1998/04/23  13:56:02  mike
 *   Updated preview to do checkerboard pattern for transparency (thanks Yosh!)
 *   Added gtk_window_set_wmclass() call to make sure this plug-in gets to use
 *   the standard GIMP icon if none is otherwise created...
 *
 *   Revision 1.8  1998/04/22  16:25:45  mike
 *   Fixed RGBA preview problems...
 *
 *   Revision 1.7  1998/03/12  18:48:52  mike
 *   Fixed pixel errors around the edge of the bounding rectangle - the
 *   original pixels weren't being written back to the image...
 *
 *   Revision 1.6  1997/11/14  17:17:59  mike
 *   Updated to dynamically allocate return params in the run() function.
 *
 *   Revision 1.5  1997/10/17  13:56:54  mike
 *   Updated author/contact information.
 *
 *   Revision 1.4  1997/09/29  17:16:29  mike
 *   To average 8 numbers you do *not* divide by 9!  This caused the brightening
 *   problem when sharpening was "turned up".
 *
 *   Revision 1.2  1997/06/08  22:27:35  mike
 *   Updated sharpen code for hard-coded 3x3 convolution matrix.
 *
 *   Revision 1.1  1997/06/08  16:46:07  mike
 *   Initial revision
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>


/*
 * Macros...
 */

#define MIN(a,b)		(((a) < (b)) ? (a) : (b))
#define MAX(a,b)		(((a) > (b)) ? (a) : (b))


/*
 * Constants...
 */

#define PLUG_IN_NAME		"plug_in_sharpen"
#define PLUG_IN_VERSION		"1.4.2 - 3 June 1998"
#define PREVIEW_SIZE		128
#define SCALE_WIDTH		64
#define ENTRY_WIDTH		64

#define CHECK_SIZE		8
#define CHECK_DARK		85
#define CHECK_LIGHT		170


/*
 * Local functions...
 */

static void	query(void);
static void	run(char *, int, GParam *, int *, GParam **);
static void	compute_luts(void);
static void	sharpen(void);
static gint	sharpen_dialog(void);
static void	dialog_create_ivalue(char *, GtkTable *, int, gint *, int, int);
static void	dialog_iscale_update(GtkAdjustment *, gint *);
static void	dialog_ientry_update(GtkWidget *, gint *);
static void	dialog_ok_callback(GtkWidget *, gpointer);
static void	dialog_cancel_callback(GtkWidget *, gpointer);
static void	dialog_close_callback(GtkWidget *, gpointer);

static void	preview_init(void);
static void	preview_exit(void);
static void	preview_update(void);
static void	preview_scroll_callback(void);

typedef gint32 intneg;
typedef gint32 intpos;

static void	gray_filter(int width, guchar *src, guchar *dst, intneg *neg0,
		            intneg *neg1, intneg *neg2);
static void	graya_filter(int width, guchar *src, guchar *dst, intneg *neg0,
		             intneg *neg1, intneg *neg2);
static void	rgb_filter(int width, guchar *src, guchar *dst, intneg *neg0,
		           intneg *neg1, intneg *neg2);
static void	rgba_filter(int width, guchar *src, guchar *dst, intneg *neg0,
		            intneg *neg1, intneg *neg2);


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
guchar		*preview_src;		/* Source pixel image */
intneg		*preview_neg;		/* Negative coefficient pixels */
guchar		*preview_dst,		/* Destination pixel image */
		*preview_image;		/* Preview RGB image */
GtkObject	*hscroll_data,		/* Horizontal scrollbar data */
		*vscroll_data;		/* Vertical scrollbar data */

GDrawable	*drawable = NULL;	/* Current image */
int		sel_x1,			/* Selection bounds */
		sel_y1,
		sel_x2,
		sel_y2;
int		sel_width,		/* Selection width */
		sel_height;		/* Selection height */
int		img_bpp;		/* Bytes-per-pixel in image */
int		sharpen_percent = 10;	/* Percent of sharpening */
gint		run_filter = FALSE;	/* True if we should run the filter */

intneg		neg_lut[256];		/* Negative coefficient LUT */
intpos		pos_lut[256];		/* Positive coefficient LUT */


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
    { PARAM_INT32,	"percent",	"Percent sharpening (default = 10)" }
  };
  static GParamDef	*return_vals = NULL;
  static int		nargs        = sizeof(args) / sizeof(args[0]),
			nreturn_vals = 0;


  gimp_install_procedure(PLUG_IN_NAME,
      "Sharpen filter, typically used to \'sharpen\' a photographic image.",
      "This plug-in selectively performs a convolution filter on an image.",
      "Michael Sweet <mike@easysw.com>",
      "Copyright 1997-1998 by Michael Sweet",
      PLUG_IN_VERSION,
      "<Image>/Filters/Enhance/Sharpen", "RGB*, GRAY*",
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
  GParam	*values;	/* Return values */


 /*
  * Initialize parameter data...
  */

  status   = STATUS_SUCCESS;
  run_mode = param[0].data.d_int32;

  values = g_new(GParam, 1);

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

 /*
  * Get drawable information...
  */

  drawable = gimp_drawable_get(param[2].data.d_drawable);

  gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width     = sel_x2 - sel_x1;
  sel_height    = sel_y2 - sel_y1;
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

        gimp_get_data(PLUG_IN_NAME, &sharpen_percent);

       /*
        * Get information from the dialog...
        */

	if (!sharpen_dialog())
          return;
        break;

    case RUN_NONINTERACTIVE :
       /*
        * Make sure all the arguments are present...
        */

        if (nparams != 4)
	  status = STATUS_CALLING_ERROR;
	else
	  sharpen_percent = param[3].data.d_int32;
        break;

    case RUN_WITH_LAST_VALS :
       /*
        * Possibly retrieve data...
        */

	gimp_get_data(PLUG_IN_NAME, &sharpen_percent);
	break;

    default :
        status = STATUS_CALLING_ERROR;
        break;;
  };

 /*
  * Sharpen the image...
  */

  if (status == STATUS_SUCCESS)
  {
    if ((gimp_drawable_color(drawable->id) ||
	 gimp_drawable_is_gray(drawable->id)))
    {
     /*
      * Set the tile cache size...
      */

      gimp_tile_cache_ntiles(2 * (drawable->width + gimp_tile_width() - 1) /
                             gimp_tile_width() + 1);

     /*
      * Run!
      */

      sharpen();

     /*
      * If run mode is interactive, flush displays...
      */

      if (run_mode != RUN_NONINTERACTIVE)
        gimp_displays_flush();

     /*
      * Store data...
      */

      if (run_mode == RUN_INTERACTIVE)
        gimp_set_data(PLUG_IN_NAME, &sharpen_percent, sizeof(sharpen_percent));
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


static void
compute_luts(void)
{
  int	i,	/* Looping var */
	fact;	/* 1 - sharpness */


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
sharpen(void)
{
  GPixelRgn	src_rgn,	/* Source image region */
		dst_rgn;	/* Destination image region */
  guchar	*src_rows[4],	/* Source pixel rows */
		*src_ptr,	/* Current source pixel */
		*dst_row;	/* Destination pixel row */
  intneg	*neg_rows[4],	/* Negative coefficient rows */
		*neg_ptr;	/* Current negative coefficient */
  int		i,		/* Looping vars */
		y,		/* Current location in image */
		row,		/* Current row in src_rows */
		count,		/* Current number of filled src_rows */
		width;		/* Byte width of the image */
  void		(*filter)(int, guchar *, guchar *, intneg *, intneg *, intneg *);


 /*
  * Let the user know what we're doing...
  */

  gimp_progress_init("Sharpening...");

 /*
  * Setup for filter...
  */

  gimp_pixel_rgn_init(&src_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height,
                      FALSE, FALSE);
  gimp_pixel_rgn_init(&dst_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height,
                      TRUE, TRUE);

  compute_luts();

  width = sel_width * img_bpp;

  for (row = 0; row < 4; row ++)
  {
    src_rows[row] = g_malloc(width * sizeof(guchar));
    neg_rows[row] = g_malloc(width * sizeof(intneg));
  };

  dst_row = g_malloc(width * sizeof(guchar));

 /*
  * Pre-load the first row for the filter...
  */

  gimp_pixel_rgn_get_row(&src_rgn, src_rows[0], sel_x1, sel_y1, sel_width);
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

      gimp_pixel_rgn_get_row(&src_rgn, src_rows[row], sel_x1, y + 1, sel_width);
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
      (*filter)(sel_width, src_rows[(row + 2) & 3], dst_row,
		neg_rows[(row + 1) & 3] + img_bpp,
		neg_rows[(row + 2) & 3] + img_bpp,
		neg_rows[(row + 3) & 3] + img_bpp);

     /*
      * Set the row...
      */

      gimp_pixel_rgn_set_row(&dst_rgn, dst_row, sel_x1, y, sel_width);
    }
    else if (count == 2)
    {
      if (y == sel_y1)
        gimp_pixel_rgn_set_row(&dst_rgn, src_rows[0], sel_x1, y, sel_width);
      else
        gimp_pixel_rgn_set_row(&dst_rgn, src_rows[2], sel_x1, y, sel_width);
    };

    if ((y & 15) == 0)
      gimp_progress_update((double)(y - sel_y1) / (double)sel_height);
  };

 /*
  * OK, we're done.  Free all memory used...
  */

  for (row = 0; row < 4; row ++)
  {
    g_free(src_rows[row]);
    g_free(neg_rows[row]);
  };

  g_free(dst_row);

 /*
  * Update the screen...
  */

  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, sel_x1, sel_y1, sel_width, sel_height);
}


/*
 * 'sharpen_dialog()' - Popup a dialog window for the filter box size...
 */

static gint
sharpen_dialog(void)
{
  GtkWidget	*dialog,	/* Dialog window */
		*table,		/* Table "container" for controls */
		*ptable,	/* Preview table */
		*frame,		/* Frame for preview */
		*scrollbar,	/* Horizontal + vertical scroller */
		*button;	/* OK/Cancel buttons */
  gint		argc;		/* Fake argc for GUI */
  gchar		**argv;		/* Fake argv for GUI */
  guchar	*color_cube;	/* Preview color cube... */


 /*
  * Initialize the program's display...
  */

  argc    = 1;
  argv    = g_new(gchar *, 1);
  argv[0] = g_strdup("sharpen");

  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());
  gdk_set_use_xshm(gimp_use_xshm());
  gtk_preview_set_gamma(gimp_gamma());
  gtk_preview_set_install_cmap(gimp_install_cmap());
  color_cube = gimp_color_cube();
  gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual(gtk_preview_get_visual());
  gtk_widget_set_default_colormap(gtk_preview_get_cmap());

 /*
  * Dialog window...
  */

  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Sharpen - " PLUG_IN_VERSION);
  gtk_window_set_wmclass(GTK_WINDOW(dialog), "sharpen", "Gimp");
  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width(GTK_CONTAINER(dialog), 0);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		     (GtkSignalFunc) dialog_close_callback,
		     NULL);

 /*
  * Top-level table for dialog...
  */

  table = gtk_table_new(3, 3, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 6);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, FALSE, FALSE, 0);
  gtk_widget_show(table);

 /*
  * Preview window...
  */

  ptable = gtk_table_new(2, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER(ptable), 0);
  gtk_table_attach(GTK_TABLE(table), ptable, 0, 3, 0, 1, 0, 0, 0, 0);
  gtk_widget_show(ptable);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(ptable), frame, 0, 1, 0, 1, 0, 0, 0, 0);
  gtk_widget_show(frame);

  preview_width  = MIN(sel_width, PREVIEW_SIZE);
  preview_height = MIN(sel_height, PREVIEW_SIZE);

  preview = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(preview), preview_width, preview_height);
  gtk_container_add(GTK_CONTAINER(frame), preview);
  gtk_widget_show(preview);

  hscroll_data = gtk_adjustment_new(0, 0, sel_width - 1, 1.0,
				    MIN(preview_width, sel_width),
				    MIN(preview_width, sel_width));

  gtk_signal_connect(hscroll_data, "value_changed",
		     (GtkSignalFunc)preview_scroll_callback, NULL);

  scrollbar = gtk_hscrollbar_new(GTK_ADJUSTMENT(hscroll_data));
  gtk_range_set_update_policy(GTK_RANGE(scrollbar), GTK_UPDATE_CONTINUOUS);
  gtk_table_attach(GTK_TABLE(ptable), scrollbar, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show(scrollbar);

  vscroll_data = gtk_adjustment_new(0, 0, sel_height - 1, 1.0,
				    MIN(preview_height, sel_height),
				    MIN(preview_height, sel_height));

  gtk_signal_connect(vscroll_data, "value_changed",
		     (GtkSignalFunc)preview_scroll_callback, NULL);

  scrollbar = gtk_vscrollbar_new(GTK_ADJUSTMENT(vscroll_data));
  gtk_range_set_update_policy(GTK_RANGE(scrollbar), GTK_UPDATE_CONTINUOUS);
  gtk_table_attach(GTK_TABLE(ptable), scrollbar, 1, 2, 0, 1, 0, GTK_FILL, 0, 0);
  gtk_widget_show(scrollbar);

  preview_init();

 /*
  * Sharpness control...
  */

  dialog_create_ivalue("Sharpness", GTK_TABLE(table), 2, &sharpen_percent, 1, 99);

 /*
  * OK, cancel buttons...
  */

  gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 6);

  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) dialog_ok_callback,
		     dialog);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) dialog_cancel_callback,
		     dialog);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show(button);

 /*
  * Show it and wait for the user to do something...
  */

  gtk_widget_show(dialog);

  preview_update();

  gtk_main();
  gdk_flush();

 /*
  * Free the preview data...
  */

  preview_exit();

 /*
  * Return ok/cancel...
  */

  return (run_filter);
}


/*
 * 'preview_init()' - Initialize the preview window...
 */

static void
preview_init(void)
{
  int	width;		/* Byte width of the image */

 /*
  * Setup for preview filter...
  */

  compute_luts();

  width = preview_width * img_bpp;

  preview_src   = g_malloc(width * preview_height * sizeof(guchar));
  preview_neg   = g_malloc(width * preview_height * sizeof(intneg));
  preview_dst   = g_malloc(width * preview_height * sizeof(guchar));
  preview_image = g_malloc(preview_width * preview_height * 3 * sizeof(guchar));

  preview_x1 = sel_x1;
  preview_y1 = sel_y1;
  preview_x2 = preview_x1 + preview_width;
  preview_y2 = preview_y1 + preview_height;
}


/*
 * 'preview_scroll_callback()' - Update the preview when a scrollbar is moved.
 */

static void
preview_scroll_callback(void)
{
  preview_x1 = sel_x1 + GTK_ADJUSTMENT(hscroll_data)->value;
  preview_y1 = sel_y1 + GTK_ADJUSTMENT(vscroll_data)->value;
  preview_x2 = preview_x1 + MIN(preview_width, sel_width);
  preview_y2 = preview_y1 + MIN(preview_height, sel_height);

  preview_update();
}


/*
 * 'preview_update()' - Update the preview window.
 */

static void
preview_update(void)
{
  GPixelRgn	src_rgn;	/* Source image region */
  guchar	*src_ptr,	/* Current source pixel */
		*dst_ptr,	/* Current destination pixel */
  		*image_ptr;	/* Current image pixel */
  intneg	*neg_ptr;	/* Current negative pixel */
  guchar	check;		/* Current check mark pixel */
  int		i,	  	/* Looping var */
		x, y,		/* Current location in image */
		width;		/* Byte width of the image */
  void		(*filter)(int, guchar *, guchar *, intneg *, intneg *, intneg *);


 /*
  * Setup for filter...
  */

  gimp_pixel_rgn_init(&src_rgn, drawable, preview_x1, preview_y1, preview_width, preview_height, FALSE, FALSE);

  width = preview_width * img_bpp;

 /*
  * Load the preview area...
  */

  gimp_pixel_rgn_get_rect(&src_rgn, preview_src, preview_x1, preview_y1,
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

  memcpy(preview_dst, preview_src, width);
  memcpy(preview_dst + width * (preview_height - 1),
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
    case 1 :
        for (x = preview_width * preview_height, dst_ptr = preview_dst,
                 image_ptr = preview_image;
             x > 0;
             x --, dst_ptr ++, image_ptr += 3)
          image_ptr[0] = image_ptr[1] = image_ptr[2] = *dst_ptr;
        break;

    case 2 :
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
              if ((y & CHECK_SIZE) ^ (x & CHECK_SIZE))
                check = CHECK_LIGHT;
              else
                check = CHECK_DARK;

              if (dst_ptr[1] == 0)
                image_ptr[0] = image_ptr[1] = image_ptr[2] = check;
              else
                image_ptr[0] = image_ptr[1] = image_ptr[2] =
                    check + ((dst_ptr[0] - check) * dst_ptr[1]) / 255;
	    };
        break;

    case 3 :
        memcpy(preview_image, preview_dst, preview_width * preview_height * 3);
        break;

    case 4 :
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
              if ((y & CHECK_SIZE) ^ (x & CHECK_SIZE))
                check = CHECK_LIGHT;
              else
                check = CHECK_DARK;

              if (dst_ptr[3] == 0)
                image_ptr[0] = image_ptr[1] = image_ptr[2] = check;
              else
              {
                image_ptr[0] = check + ((dst_ptr[0] - check) * dst_ptr[3]) / 255;
                image_ptr[1] = check + ((dst_ptr[1] - check) * dst_ptr[3]) / 255;
                image_ptr[2] = check + ((dst_ptr[2] - check) * dst_ptr[3]) / 255;
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
    gtk_preview_draw_row(GTK_PREVIEW(preview), image_ptr, 0, y,
  	                 preview_width);

  gtk_widget_draw(preview, NULL);
  gdk_flush();
}


/*
 * 'preview_exit()' - Free all memory used by the preview window...
 */

static void
preview_exit(void)
{
  g_free(preview_src);
  g_free(preview_neg);
  g_free(preview_dst);
  g_free(preview_image);
}


/*
 * 'dialog_create_ivalue()' - Create an integer value control...
 */

static void
dialog_create_ivalue(char     *title,	/* I - Label for control */
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
  char		buf[256];	/* String buffer */


 /*
  * Label...
  */

  label = gtk_label_new(title);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
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
  gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
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
  sprintf(buf, "%d", *value);
  gtk_entry_set_text(GTK_ENTRY(entry), buf);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc) dialog_ientry_update,
		     value);
  gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(entry);
}


/*
 * 'dialog_iscale_update()' - Update the value field using the scale.
 */

static void
dialog_iscale_update(GtkAdjustment *adjustment,	/* I - New value */
                     gint          *value)	/* I - Current value */
{
  GtkWidget	*entry;		/* Text entry widget */
  char		buf[256];	/* Text buffer */


  if (*value != adjustment->value)
  {
    *value = adjustment->value;

    entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
    sprintf(buf, "%d", *value);

    gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);

    compute_luts();

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

      compute_luts();

      preview_update();
    };
  };
}


/*
 * 'dialog_ok_callback()' - Start the filter...
 */

static void
dialog_ok_callback(GtkWidget *widget,	/* I - OK button widget */
                   gpointer  data)	/* I - Dialog window */
{
  run_filter = TRUE;
  gtk_widget_destroy(GTK_WIDGET(data));
}


/*
 * 'dialog_cancel_callback()' - Cancel the filter...
 */

static void
dialog_cancel_callback(GtkWidget *widget,	/* I - Cancel button widget */
                       gpointer  data)		/* I - Dialog window */
{
  gtk_widget_destroy(GTK_WIDGET(data));
}


/*
 * 'dialog_close_callback()' - Exit the filter dialog application.
 */

static void
dialog_close_callback(GtkWidget *widget,	/* I - Dialog window */
                      gpointer  data)		/* I - Dialog window */
{
  gtk_main_quit();
}


/*
 * 'gray_filter()' - Sharpen grayscale pixels.
 */

static void
gray_filter(int    width,	/* I - Width of line in pixels */
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
    pixel = pos_lut[*src++] - neg0[-1] - neg0[0] - neg0[1] -
	    neg1[-1] - neg1[1] -
	    neg2[-1] - neg2[0] - neg2[1];
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
graya_filter(int    width,	/* I - Width of line in pixels */
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
    pixel = pos_lut[*src++] - neg0[-2] - neg0[0] - neg0[2] -
	    neg1[-2] - neg1[2] -
	    neg2[-2] - neg2[0] - neg2[2];
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
rgb_filter(int    width,	/* I - Width of line in pixels */
           guchar *src,		/* I - Source line */
           guchar *dst,		/* O - Destination line */
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
    pixel = pos_lut[*src++] - neg0[-3] - neg0[0] - neg0[3] -
	    neg1[-3] - neg1[3] -
	    neg2[-3] - neg2[0] - neg2[3];
    pixel = (pixel + 4) >> 3;
    if (pixel < 0)
      *dst++ = 0;
    else if (pixel < 255)
      *dst++ = pixel;
    else
      *dst++ = 255;

    pixel = pos_lut[*src++] - neg0[-2] - neg0[1] - neg0[4] -
	    neg1[-2] - neg1[4] -
	    neg2[-2] - neg2[1] - neg2[4];
    pixel = (pixel + 4) >> 3;
    if (pixel < 0)
      *dst++ = 0;
    else if (pixel < 255)
      *dst++ = pixel;
    else
      *dst++ = 255;

    pixel = pos_lut[*src++] - neg0[-1] - neg0[2] - neg0[5] -
	    neg1[-1] - neg1[5] -
	    neg2[-1] - neg2[2] - neg2[5];
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
rgba_filter(int    width,	/* I - Width of line in pixels */
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
    pixel = pos_lut[*src++] - neg0[-4] - neg0[0] - neg0[4] -
	    neg1[-4] - neg1[4] -
	    neg2[-4] - neg2[0] - neg2[4];
    pixel = (pixel + 4) >> 3;
    if (pixel < 0)
      *dst++ = 0;
    else if (pixel < 255)
      *dst++ = pixel;
    else
      *dst++ = 255;

    pixel = pos_lut[*src++] - neg0[-3] - neg0[1] - neg0[5] -
	    neg1[-3] - neg1[5] -
	    neg2[-3] - neg2[1] - neg2[5];
    pixel = (pixel + 4) >> 3;
    if (pixel < 0)
      *dst++ = 0;
    else if (pixel < 255)
      *dst++ = pixel;
    else
      *dst++ = 255;

    pixel = pos_lut[*src++] - neg0[-2] - neg0[2] - neg0[6] -
	    neg1[-2] - neg1[6] -
	    neg2[-2] - neg2[2] - neg2[6];
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


/*
 * End of "$Id$".
 */
