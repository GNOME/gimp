/*
 * "$Id$"
 *
 *   Despeckle (adaptive median) filter for The GIMP -- an image manipulation
 *   program
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contents:
 *
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the filter...
 *   despeckle()                 - Despeckle an image using a median filter.
 *   despeckle_dialog()          -  Popup a dialog window for the filter box size...
 *   preview_init()              - Initialize the preview window...
 *   preview_scroll_callback()   - Update the preview when a scrollbar is moved.
 *   preview_update()            - Update the preview window.
 *   preview_exit()              - Free all memory used by the preview window...
 *   dialog_create_ivalue()      - Create an integer value control...
 *   dialog_iscale_update()      - Update the value field using the scale.
 *   dialog_ientry_update()      - Update the value field using the text entry.
 *   dialog_adaptive_callback()  - Update the filter type...
 *   dialog_recursive_callback() - Update the filter type...
 *   dialog_ok_callback()        - Start the filter...
 *   dialog_cancel_callback()    - Cancel the filter...
 *   dialog_close_callback()     - Exit the filter dialog application.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.5  1998/01/25 09:29:23  yosh
 *   Plugin updates
 *   Properly generated aa Makefile (still not built by default)
 *   Sven's no args script patch
 *
 *   -Yosh
 *
 *   Revision 1.16  1998/01/22  14:35:03  mike
 *   Added black & white level controls.
 *   Fixed bug in despeckle code that caused the borders to darken.
 *
 *   Revision 1.15  1998/01/21  21:33:47  mike
 *   Fixed malloc buffer overflow bug - wasn't realloc'ing buffers
 *   when the filter radius changed.
 *
 *   Revision 1.14  1997/11/14  17:17:59  mike
 *   Updated to dynamically allocate return params in the run() function.
 *
 *   Revision 1.13  1997/11/12  15:53:34  mike
 *   Added <string.h> header file for Digital UNIX...
 *
 *   Revision 1.12  1997/10/17  13:56:54  mike
 *   Updated author/contact information.
 *
 *   Revision 1.11  1997/06/12  16:58:11  mike
 *   Optimized final despeckle - now grab gimp_tile_height() rows at a time
 *   for faster filtering.
 *
 *   Revision 1.10  1997/06/08  23:30:29  mike
 *   Improved the preview update speed significantly by loading the entire
 *   source (preview) image first.
 *
 *   Revision 1.9  1997/06/08  16:48:21  mike
 *   Renamed "adaptive" argument to "type" (filter type).
 *
 *   Revision 1.8  1997/06/08  12:45:09  mike
 *   Added recursive filter option.
 *   Cleaned up UI.
 *
 *   Revision 1.7  1997/06/08  04:27:19  mike
 *   Updated documentation.
 *   Moved plug-in back to original location in menu tree.
 *
 *   Revision 1.6  1997/06/08  04:24:56  mike
 *   Added filter type argument & control.
 *
 *   Revision 1.5  1997/06/08  04:12:36  mike
 *   Added preview window.
 *
 *   Revision 1.4  1997/06/08  02:18:22  mike
 *   Updated to adjust the despeckling radius based upon the window's
 *   histogram.  This improves filter quality significantly as surface
 *   details are preserved and not blurred...
 *
 *   Revision 1.3  1997/06/07  01:29:47  mike
 *   Added some minor optimizations.
 *   Updated version to 1.01.
 *   Fixed minor bug in dialog_ientry_update() - was using gdouble instead
 *   of gint for new_value...
 *
 *   Revision 1.2  1997/06/07  01:03:07  mike
 *   Updated docos, changed maximum radius to 20.
 *
 *   Revision 1.1  1997/06/07  00:01:15  mike
 *   Initial Revision.
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

#define PLUG_IN_NAME		"plug_in_despeckle"
#define PLUG_IN_VERSION		"1.2 - 22 January 1998"
#define PREVIEW_SIZE		128
#define SCALE_WIDTH		64
#define ENTRY_WIDTH		64
#define MAX_RADIUS		20

#define FILTER_ADAPTIVE		0x01
#define FILTER_RECURSIVE	0x02

#define despeckle_radius	(despeckle_vals[0])	/* Radius of filter */
#define filter_type		(despeckle_vals[1])	/* Type of filter */
#define black_level		(despeckle_vals[2])	/* Black level */
#define white_level		(despeckle_vals[3])	/* White level */

/*
 * Local functions...
 */

static void	query(void);
static void	run(char *, int, GParam *, int *, GParam **);
static void	despeckle(void);
static gint	despeckle_dialog(void);
static void	dialog_create_ivalue(char *, GtkTable *, int, gint *, int, int);
static void	dialog_iscale_update(GtkAdjustment *, gint *);
static void	dialog_ientry_update(GtkWidget *, gint *);
static void	dialog_adaptive_callback(GtkWidget *, gpointer);
static void	dialog_recursive_callback(GtkWidget *, gpointer);
static void	dialog_ok_callback(GtkWidget *, gpointer);
static void	dialog_cancel_callback(GtkWidget *, gpointer);
static void	dialog_close_callback(GtkWidget *, gpointer);

static void	preview_init(void);
static void	preview_exit(void);
static void	preview_update(void);
static void	preview_scroll_callback(void);


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
guchar		*preview_src = NULL,	/* Source pixel rows */
		*preview_dst,		/* Destination pixel row */
		*preview_sort;		/* Pixel value sort array */
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
gint		run_filter = FALSE;	/* True if we should run the filter */
int		despeckle_vals[4] = { 3, FILTER_ADAPTIVE, 7, 248 };


/*
 * 'main()' - Main entry - just call gimp_main()...
 */

int
main(int  argc,		/* I - Number of command-line args */
     char *argv[])	/* I - Command-line args */
{
  return (gimp_main(argc, argv));
}


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
    { PARAM_INT32,	"radius",	"Filter box radius (default = 3)" },
    { PARAM_INT32,	"type",		"Filter type (0 = median, 1 = adaptive, 2 = recursive-median, 3 = recursive-adaptive)" },
    { PARAM_INT32,	"black",	"Black level (0 to 255)" },
    { PARAM_INT32,	"white",	"White level (0 to 255)" }
  };
  static GParamDef	*return_vals = NULL;
  static int		nargs        = sizeof(args) / sizeof(args[0]),
			nreturn_vals = 0;


  gimp_install_procedure(PLUG_IN_NAME,
      "Despeckle filter, typically used to \'despeckle\' a photographic image.",
      "This plug-in selectively performs a median or adaptive box filter on an image.",
      "Michael Sweet <mike@easysw.com>",
      "Michael Sweet <mike@easysw.com>",
      PLUG_IN_VERSION,
      "<Image>/Filters/Enhance/Despeckle...", "RGB*, GRAY*",
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

        gimp_get_data(PLUG_IN_NAME, &despeckle_radius);

       /*
        * Get information from the dialog...
        */

	if (!despeckle_dialog())
          return;
        break;

    case RUN_NONINTERACTIVE :
       /*
        * Make sure all the arguments are present...
        */

        if (nparams < 4 || nparams > 7)
	  status = STATUS_CALLING_ERROR;
	else if (nparams == 4)
	{
	  despeckle_radius = param[3].data.d_int32;
	  filter_type      = FILTER_ADAPTIVE;
	  black_level      = 7;
	  white_level      = 248;
	}
	else if (nparams == 5)
	{
	  despeckle_radius = param[3].data.d_int32;
	  filter_type      = param[4].data.d_int32;
	  black_level      = 7;
	  white_level      = 248;
	}
	else if (nparams == 6)
	{
	  despeckle_radius = param[3].data.d_int32;
	  filter_type      = param[4].data.d_int32;
	  black_level      = param[5].data.d_int32;
	  white_level      = 248;
	}
	else
	{
	  despeckle_radius = param[3].data.d_int32;
	  filter_type      = param[4].data.d_int32;
	  black_level      = param[5].data.d_int32;
	  white_level      = param[6].data.d_int32;
	};
        break;

    case RUN_WITH_LAST_VALS :
       /*
        * Possibly retrieve data...
        */

	gimp_get_data(PLUG_IN_NAME, despeckle_vals);
	break;

    default :
        status = STATUS_CALLING_ERROR;
        break;;
  };

 /*
  * Despeckle the image...
  */

  if (status == STATUS_SUCCESS)
  {
    if ((gimp_drawable_color(drawable->id) ||
	 gimp_drawable_gray(drawable->id)))
    {
     /*
      * Set the tile cache size...
      */

      gimp_tile_cache_ntiles((drawable->width + gimp_tile_width() - 1) /
                             gimp_tile_width());

     /*
      * Run!
      */

      despeckle();

     /*
      * If run mode is interactive, flush displays...
      */

      if (run_mode != RUN_NONINTERACTIVE)
        gimp_displays_flush();

     /*
      * Store data...
      */

      if (run_mode == RUN_INTERACTIVE)
        gimp_set_data(PLUG_IN_NAME, despeckle_vals, sizeof(despeckle_vals));
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


/*
 * 'despeckle()' - Despeckle an image using a median filter.
 *
 * A median filter basically collects pixel values in a region around the
 * target pixel, sorts them, and uses the median value. This code uses a
 * circular row buffer to improve performance.
 *
 * The adaptive filter is based on the median filter but analizes the histogram
 * of the region around the target pixel and adjusts the despeckle radius
 * accordingly.
 */

static void
despeckle(void)
{
  GPixelRgn	src_rgn,	/* Source image region */
		dst_rgn;	/* Destination image region */
  guchar	**src_rows,	/* Source pixel rows */
		*dst_row,	/* Destination pixel row */
		*src_ptr,	/* Source pixel pointer */
		*sort,		/* Pixel value sort array */
		*sort_ptr;	/* Current sort value */
  int		sort_count,	/* Number of soft values */
		i, j, t, d,	/* Looping vars */
		x, y,		/* Current location in image */
		row,		/* Current row in src_rows */
		rowcount,	/* Number of rows loaded */
		lasty,		/* Last row loaded in src_rows */
		trow,		/* Looping var */
		startrow,	/* Starting row for loop */
		endrow,		/* Ending row for loop */
		max_row,	/* Maximum number of filled src_rows */
		size,		/* Width/height of the filter box */
		width,		/* Byte width of the image */
		xmin, xmax, tx,	/* Looping vars */
		radius,		/* Current radius */
		hist0,		/* Histogram count for 0 values */
		hist255;	/* Histogram count for 255 values */


 /*
  * Let the user know what we're doing...
  */

  gimp_progress_init("Despeckling...");

 /*
  * Setup for filter...
  */

  gimp_pixel_rgn_init(&src_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height, FALSE, FALSE);
  gimp_pixel_rgn_init(&dst_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);

  size     = despeckle_radius * 2 + 1;
  max_row  = 2 * gimp_tile_height();
  width    = sel_width * img_bpp;

  src_rows    = g_malloc(max_row * sizeof(guchar *));
  src_rows[0] = g_malloc(max_row * width * sizeof(guchar));

  for (row = 1; row < max_row; row ++)
    src_rows[row] = src_rows[0] + row * width;

  dst_row = g_malloc(width * sizeof(guchar));
  sort    = g_malloc(size * size * sizeof(guchar));

 /*
  * Pre-load the first "size" rows for the filter...
  */

  if (sel_height < gimp_tile_height())
    rowcount = sel_height;
  else
    rowcount = gimp_tile_height();

  gimp_pixel_rgn_get_rect(&src_rgn, src_rows[0], sel_x1, sel_y1, sel_width,
                          rowcount);

  row   = rowcount;
  lasty = sel_y1 + rowcount;

 /*
  * Despeckle...
  */

  for (y = sel_y1 ; y < sel_y2; y ++)
  {
    if ((y + despeckle_radius) >= lasty &&
        lasty < sel_y2)
    {
     /*
      * Load the next block of rows...
      */

      rowcount -= gimp_tile_height();
      if ((i = sel_y2 - lasty) > gimp_tile_height())
        i = gimp_tile_height();

      gimp_pixel_rgn_get_rect(&src_rgn, src_rows[row], sel_x1, lasty, sel_width, i);

      rowcount += i;
      lasty    += i;
      row      = (row + i) % max_row;
    };

   /*
    * Now find the median pixels and save the results...
    */

    radius = despeckle_radius;

    memcpy(dst_row, src_rows[(row + y - lasty + max_row) % max_row], width);

    if (y >= (sel_y1 + radius) && y < (sel_y2 - radius))
    {
      for (x = 0; x < width; x ++)
      {
	hist0   = 0;
	hist255 = 0;
	xmin    = x - radius * img_bpp;
	xmax    = x + (radius + 1) * img_bpp;

	if (xmin < 0)
          xmin = x % img_bpp;

	if (xmax > width)
          xmax = width;

	startrow = (row + y - lasty - radius + max_row) % max_row;
	endrow   = (row + y - lasty + radius + 1 + max_row) % max_row;

	for (sort_ptr = sort, trow = startrow;
             trow != endrow;
             trow = (trow + 1) % max_row)
          for (tx = xmin, src_ptr = src_rows[trow] + xmin;
               tx < xmax;
               tx += img_bpp, src_ptr += img_bpp)
          {
            if ((*sort_ptr = *src_ptr) <= black_level)
              hist0 ++;
            else if (*sort_ptr >= white_level)
              hist255 ++;

            if (*sort_ptr < white_level)
              sort_ptr ++;
          };

       /*
	* Shell sort the color values...
	*/

	sort_count = sort_ptr - sort;

        if (sort_count > 1)
        {
	  for (d = sort_count / 2; d > 0; d = d / 2)
	    for (i = d; i < sort_count; i ++)
	      for (j = i - d, sort_ptr = sort + j;
		   j >= 0 && sort_ptr[0] > sort_ptr[d];
		   j -= d, sort_ptr -= d)
	      {
		t           = sort_ptr[0];
		sort_ptr[0] = sort_ptr[d];
		sort_ptr[d] = t;
	      };

	 /*
	  * Assign the median value...
	  */

	  t = sort_count / 2;

	  if (sort_count & 1)
            dst_row[x] = (sort[t] + sort[t + 1]) / 2;
	  else
            dst_row[x] = sort[t];

	 /*
	  * Save the change to the source image too if the user wants the
	  * recursive method...
	  */

	  if (filter_type & FILTER_RECURSIVE)
            src_rows[(row + y - lasty + max_row) % max_row][x] = dst_row[x];
        };

       /*
	* Check the histogram and adjust the radius accordingly...
	*/

	if (filter_type & FILTER_ADAPTIVE)
	{
	  if (hist0 >= radius || hist255 >= radius)
	  {
            if (radius < despeckle_radius)
              radius ++;
	  }
	  else if (radius > 1)
            radius --;
	};
      };
    };

    gimp_pixel_rgn_set_row(&dst_rgn, dst_row, sel_x1, y, sel_width);

    if ((y & 15) == 0)
      gimp_progress_update((double)(y - sel_y1) / (double)sel_height);
  };

 /*
  * OK, we're done.  Free all memory used...
  */

  g_free(src_rows[0]);
  g_free(src_rows);
  g_free(dst_row);
  g_free(sort);

 /*
  * Update the screen...
  */

  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, sel_x1, sel_y1, sel_width, sel_height);
}


/*
 * 'despeckle_dialog()' - Popup a dialog window for the filter box size...
 */

static gint
despeckle_dialog(void)
{
  GtkWidget	*dialog,	/* Dialog window */
		*table,		/* Table "container" for controls */
		*ptable,	/* Preview table */
		*ftable,	/* Filter table */
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
  argv[0] = g_strdup("despeckle");

  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());
  gdk_set_use_xshm(gimp_use_xshm());

  signal(SIGBUS, SIG_DFL);
  signal(SIGSEGV, SIG_DFL);
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
  gtk_window_set_title(GTK_WINDOW(dialog), "Despeckle " PLUG_IN_VERSION);
  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width(GTK_CONTAINER(dialog), 0);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		     (GtkSignalFunc) dialog_close_callback,
		     NULL);

 /*
  * Top-level table for dialog...
  */

  table = gtk_table_new(5, 3, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 6);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, FALSE, FALSE, 0);
  gtk_widget_show(table);

 /*
  * Preview window...
  */

  ptable = gtk_table_new(2, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER(ptable), 0);
  gtk_table_attach(GTK_TABLE(table), ptable, 0, 2, 0, 1, 0, 0, 0, 0);
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

  preview_x1 = sel_x1;
  preview_y1 = sel_y1;
  preview_x2 = preview_x1 + MIN(preview_width, sel_width);
  preview_y2 = preview_y1 + MIN(preview_height, sel_height);

 /*
  * Filter type controls...
  */

  ftable = gtk_table_new(6, 1, FALSE);
  gtk_container_border_width(GTK_CONTAINER(ftable), 4);
  gtk_table_attach(GTK_TABLE(table), ftable, 2, 3, 0, 1, 0, 0, 0, 0);
  gtk_widget_show(ftable);

  button = gtk_check_button_new_with_label("Adaptive");
  gtk_table_attach(GTK_TABLE(ftable), button, 0, 1, 0, 1,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button),
                              (filter_type & FILTER_ADAPTIVE) ? TRUE : FALSE);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     (GtkSignalFunc)dialog_adaptive_callback,
		     NULL);
  gtk_widget_show(button);

  button = gtk_check_button_new_with_label("Recursive");
  gtk_table_attach(GTK_TABLE(ftable), button, 0, 1, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button),
                              (filter_type & FILTER_RECURSIVE) ? TRUE : FALSE);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     (GtkSignalFunc)dialog_recursive_callback,
		     NULL);
  gtk_widget_show(button);

 /*
  * Box size (radius) control...
  */

  dialog_create_ivalue("Radius", GTK_TABLE(table), 2, &despeckle_radius, 1, MAX_RADIUS);

 /*
  * Black level control...
  */

  dialog_create_ivalue("Black Level", GTK_TABLE(table), 3, &black_level, 0, 256);

 /*
  * White level control...
  */

  dialog_create_ivalue("White Level", GTK_TABLE(table), 4, &white_level, 0, 256);

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
  int	size,		/* Size of filter box */
	width;		/* Byte width of the image */


 /*
  * Setup for preview filter...
  */

  size  = despeckle_radius * 2 + 1;
  width = preview_width * img_bpp;

  if (preview_src != NULL)
  {
    g_free(preview_src);
    g_free(preview_dst);
    g_free(preview_sort);
  };

  preview_src  = g_malloc(width * preview_height * sizeof(guchar *));
  preview_dst  = g_malloc(width * sizeof(guchar));
  preview_sort = g_malloc(size * size * sizeof(guchar));
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
  guchar	*sort_ptr,	/* Current preview_sort value */
		*src_ptr,	/* Current source pixel */
		*dst_ptr;	/* Current destination pixel */
  int		sort_count,	/* Number of soft values */
		i, j, t, d,	/* Looping vars */
		x, y,		/* Current location in image */
		size,		/* Width/height of the filter box */
		width,		/* Byte width of the image */
		xmin, xmax, tx,	/* Looping vars */
		radius,		/* Current radius */
		hist0,		/* Histogram count for 0 values */
		hist255;	/* Histogram count for 255 values */
  guchar	rgb[PREVIEW_SIZE * 3],	/* Output image */
		*rgb_ptr;	/* Pixel pointer for output */


 /*
  * Setup for filter...
  */

  gimp_pixel_rgn_init(&src_rgn, drawable, preview_x1, preview_y1, preview_width, preview_height, FALSE, FALSE);

 /*
  * Pre-load the preview rectangle...
  */

  size  = despeckle_radius * 2 + 1;
  width = preview_width * img_bpp;

  gimp_pixel_rgn_get_rect(&src_rgn, preview_src, preview_x1, preview_y1,
                          preview_width, preview_height);

 /*
  * Despeckle...
  */

  for (y = 0; y < preview_height; y ++)
  {
   /*
    * Now find the median pixels and save the results...
    */

    radius = despeckle_radius;

    memcpy(preview_dst, preview_src + y * width, width);

    if (y >= radius && y < (preview_height - radius))
    {
      for (x = 0, dst_ptr = preview_dst; x < width; x ++, dst_ptr ++)
      {
	hist0   = 0;
	hist255 = 0;
	xmin    = x - radius * img_bpp;
	xmax    = x + (radius + 1) * img_bpp;

	if (xmin < 0)
          xmin = x % img_bpp;

	if (xmax > width)
          xmax = width;

	for (i = -radius, sort_ptr = preview_sort,
	         src_ptr = preview_src + width * (y - radius);
             i <= radius;
             i ++, src_ptr += width)
          for (tx = xmin; tx < xmax; tx += img_bpp)
          {
            if ((*sort_ptr = src_ptr[tx]) <= black_level)
              hist0 ++;
            else if (*sort_ptr >= white_level)
              hist255 ++;

            if (*sort_ptr < white_level)
              sort_ptr ++;
          };

       /*
	* Shell preview_sort the color values...
	*/

	sort_count = sort_ptr - preview_sort;

        if (sort_count > 1)
        {
	  for (d = sort_count / 2; d > 0; d = d / 2)
	    for (i = d; i < sort_count; i ++)
	      for (j = i - d, sort_ptr = preview_sort + j;
		   j >= 0 && sort_ptr[0] > sort_ptr[d];
		   j -= d, sort_ptr -= d)
	      {
		t           = sort_ptr[0];
		sort_ptr[0] = sort_ptr[d];
		sort_ptr[d] = t;
	      };

	 /*
	  * Assign the median value...
	  */

	  t = sort_count / 2;

	  if (sort_count & 1)
            *dst_ptr = (preview_sort[t] + preview_sort[t + 1]) / 2;
	  else
            *dst_ptr = preview_sort[t];

	 /*
	  * Save the change to the source image too if the user wants the
	  * recursive method...
	  */

	  if (filter_type & FILTER_RECURSIVE)
            preview_src[y * width + x] = *dst_ptr;
        };

       /*
	* Check the histogram and adjust the radius accordingly...
	*/

	if (filter_type & FILTER_ADAPTIVE)
	{
	  if (hist0 >= radius || hist255 >= radius)
	  {
            if (radius < despeckle_radius)
              radius ++;
	  }
	  else if (radius > 1)
            radius --;
	};
      };
    };

   /*
    * Draw this row...
    */

    switch (img_bpp)
    {
      case 1 :
      case 2 :
          for (x = 0, dst_ptr = preview_dst, rgb_ptr = rgb;
               x < preview_width;
               x ++, dst_ptr += img_bpp, rgb_ptr += 3)
            rgb_ptr[0] = rgb_ptr[1] = rgb_ptr[2] = *dst_ptr;

  	  gtk_preview_draw_row(GTK_PREVIEW(preview), rgb, 0, y, preview_width);
          break;

      case 3 :
  	  gtk_preview_draw_row(GTK_PREVIEW(preview), preview_dst, 0, y,
  	                       preview_width);
          break;

      case 4 :
          for (x = 0, dst_ptr = preview_dst, rgb_ptr = rgb;
               x < preview_width;
               x ++, dst_ptr += 4, rgb_ptr += 3)
          {
            rgb_ptr[0] = dst_ptr[0];
            rgb_ptr[1] = dst_ptr[1];
            rgb_ptr[2] = dst_ptr[2];
          };

  	  gtk_preview_draw_row(GTK_PREVIEW(preview), rgb, 0, y, preview_width);
          break;
    };
  };

  gtk_widget_draw(preview, NULL);
  gdk_flush();
}


/*
 * 'preview_exit()' - Free all memory used by the preview window...
 */

static void
preview_exit(void)
{
  int	row,	/* Looping var */
	size;	/* Size of row buffer */


  size = MAX_RADIUS * 2 + 1;

  g_free(preview_src);
  g_free(preview_dst);
  g_free(preview_sort);
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

    if (value == &despeckle_radius)
      preview_init();

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

      if (value == &despeckle_radius)
	preview_init();

      preview_update();
    };
  };
}


/*
 * 'dialog_adaptive_callback()' - Update the filter type...
 */

static void
dialog_adaptive_callback(GtkWidget *widget,	/* I - Toggle button */
                         gpointer  data)	/* I - Data */
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
    filter_type |= FILTER_ADAPTIVE;
  else
    filter_type &= ~FILTER_ADAPTIVE;

  preview_update();
}


/*
 * 'dialog_recursive_callback()' - Update the filter type...
 */

static void
dialog_recursive_callback(GtkWidget *widget,	/* I - Toggle button */
                          gpointer  data)	/* I - Data */
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
    filter_type |= FILTER_RECURSIVE;
  else
    filter_type &= ~FILTER_RECURSIVE;

  preview_update();
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
 * End of "$Id$".
 */
