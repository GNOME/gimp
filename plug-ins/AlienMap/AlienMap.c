/**********************************************************************
 *  AlienMap (Co-)sine color transformation plug-in (Version 1.01)
 *  Daniel Cotting (cotting@mygale.org)
 **********************************************************************
 *  Official Homepage: http://www.mygale.org/~cotting
 **********************************************************************
 *  Homepages under construction: http://www.chez.com/cotting
 *                                http://www.cyberbrain.com/cotting
 *  You won't be able to see anything yet, as I don't really have the 
 *  time to build up these two sites :-( 
 *  Have a look at www.mygale.org/~cotting instead!
 **********************************************************************    
 */

/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "logo.h"

/***** Macros *****/

#define ALIEN_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ALIEN_MAX(a, b) (((a) > (b)) ? (a) : (b))


/***** Magic numbers *****/

#define PREVIEW_SIZE 128
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  45

#define SINUS 0
#define COSINUS 1
#define NONE 2

/***** Types *****/
typedef struct {
        gdouble redstretch;
        gdouble greenstretch;
        gdouble bluestretch;
        gint    redmode;
        gint    greenmode;
        gint    bluemode;
} alienmap_vals_t;

typedef struct {
        GtkWidget *preview;
        guchar    *image;
        guchar    *wimage;
        gint run;
} alienmap_interface_t;



/* Declare local functions. */

static void      query  (void);
static void      run    (char      *name,
        		 int        nparams,
        		 GParam    *param,
        		 int       *nreturn_vals,
        		 GParam   **return_vals);

static void      alienmap 	    (GDrawable  *drawable);
static void      alienmap_render_row  (const guchar *src_row,
        			     guchar *dest_row,
        			     gint row,
        			     gint row_width,
        			     gint bytes, double, double, double);
static void      alienmap_get_pixel(int x, int y, guchar *pixel);
void    	 transform           (short int *, short int *, short int *,double, double, double);


static void      build_preview_source_image(void);

static gint      alienmap_dialog(void);
static void      dialog_update_preview(void);
static void      dialog_create_value(char *title, GtkTable *table, int row, gdouble *value,
        			     int left, int right, const char *desc);
static void      dialog_scale_update(GtkAdjustment *adjustment, gdouble *value);
static void      dialog_entry_update(GtkWidget *widget, gdouble *value);
static void      dialog_close_callback(GtkWidget *widget, gpointer data);
static void      dialog_ok_callback(GtkWidget *widget, gpointer data);
static void      dialog_cancel_callback(GtkWidget *widget, gpointer data);
static void      alienmap_toggle_update    (GtkWidget *widget,
        				    gpointer   data);
void alienmap_logo_dialog (void);



					    
					    
/***** Variables *****/

GtkWidget *maindlg;
GtkWidget *logodlg;
GtkTooltips *tips;
GdkColor tips_fg,tips_bg;	

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static alienmap_interface_t wint = {
        NULL,  /* preview */
        NULL,  /* image */
        NULL,  /* wimage */
        FALSE  /* run */
}; /* wint */

static alienmap_vals_t wvals = {
        128,128,128,COSINUS,SINUS,SINUS,
}; /* wvals */

static GDrawable *drawable;
static gint   tile_width, tile_height;
static gint   img_width, img_height, img_bpp;
static gint   sel_x1, sel_y1, sel_x2, sel_y2;
static gint   sel_width, sel_height;
static gint   preview_width, preview_height;
static GTile *the_tile = NULL;
static double cen_x, cen_y;
static double scale_x, scale_y;

gint do_redsinus;
gint do_redcosinus;
gint do_rednone;

gint do_greensinus;
gint do_greencosinus;
gint do_greennone;

gint do_bluesinus;
gint do_bluecosinus;
gint do_bluenone;
/***** Functions *****/


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32,    "run_mode",     "Interactive, non-interactive" },
    { PARAM_IMAGE,    "image",        "Input image" },
    { PARAM_DRAWABLE, "drawable",     "Input drawable" },
    { PARAM_INT8,    "redstretch",   "Red component stretching factor (0-128)" },
    { PARAM_INT8,    "greenstretch", "Green component stretching factor (0-128)" },
    { PARAM_INT8,    "bluestretch",  "Blue component stretching factor (0-128)" },
    { PARAM_INT8,    "redmode",      "Red application mode (0:SIN;1:COS;2:NONE)" },
    { PARAM_INT8,    "greenmode",    "Green application mode (0:SIN;1:COS;2:NONE)" },
    { PARAM_INT8,    "bluemode",     "Blue application mode (0:SIN;1:COS;2:NONE)" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_alienmap",
        		  "AlienMap Color Transformation Plug-In",
        		  "No help yet. Just try it and you'll see!",
        		  "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
        		  "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
        		  "1th May 1997",
        		  "<Image>/Filters/Colors/Alien Map",
        		  "RGB*",
        		  PROC_PLUG_IN,
        		  nargs, nreturn_vals,
        		  args, return_vals);
}



void
transform  (short int *r,
            short int *g,
            short int *b, double redstretch, double greenstretch, double bluestretch)
{
  int red, green, blue;
  double pi=atan(1)*4;
  red = *r;
  green = *g;
  blue = *b;
  switch (wvals.redmode)
  {
    case SINUS:
       red    = (int) redstretch*(1.0+sin((red/128.0-1)*pi));
       break;
    case COSINUS:
       red    = (int) redstretch*(1.0+cos((red/128.0-1)*pi));
       break;
    default:
    break;
   }

  switch (wvals.greenmode)
  {
    case SINUS:
       green    = (int) greenstretch*(1.0+sin((green/128.0-1)*pi));
       break;
    case COSINUS:
       green    = (int) greenstretch*(1.0+cos((green/128.0-1)*pi));
       break;
    default:
    break;
   }

  switch (wvals.bluemode)
  {
    case SINUS:
       blue    = (int) bluestretch*(1.0+sin((blue/128.0-1)*pi));
       break;
    case COSINUS:
       blue    = (int) bluestretch*(1.0+cos((blue/128.0-1)*pi));
       break;
    default:
    break;
   }
   
   if (red== 256) {
               red= 255;}
   if (green== 256) {
          green= 255;}
   if (blue== 256) {blue= 255;}
  *r = red;
  *g = green;
  *b = blue;
}


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
/*   GDrawable *drawable; */
/*   gint32 image_ID; */
  GRunModeType  run_mode;
  double        xhsiz, yhsiz;
  int   	pwidth, pheight;
  GStatusType status = STATUS_SUCCESS;


  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;



  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
/*   image_ID = param[1].data.d_image; */
  tile_width  = gimp_tile_width();
  tile_height = gimp_tile_height();

  img_width  = gimp_drawable_width(drawable->id);
  img_height = gimp_drawable_height(drawable->id);
  img_bpp    = gimp_drawable_bpp(drawable->id);

  gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width  = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  cen_x = (double) (sel_x2 - 1 + sel_x1) / 2.0;
  cen_y = (double) (sel_y2 - 1 + sel_y1) / 2.0;

  xhsiz = (double) (sel_width - 1) / 2.0;
  yhsiz = (double) (sel_height - 1) / 2.0;

        if (xhsiz < yhsiz) {
        	scale_x = yhsiz / xhsiz;
        	scale_y = 1.0;
        } else if (xhsiz > yhsiz) {
        	scale_x = 1.0;
        	scale_y = xhsiz / yhsiz;
        } else {
        	scale_x = 1.0;
        	scale_y = 1.0;
        } /* else */

        /* Calculate preview size */
        if (sel_width > sel_height) {
        	pwidth  = ALIEN_MIN(sel_width, PREVIEW_SIZE);
        	pheight = sel_height * pwidth / sel_width;
        } else {
        	pheight = ALIEN_MIN(sel_height, PREVIEW_SIZE);
        	pwidth  = sel_width * pheight / sel_height;
        } /* else */

        preview_width  = ALIEN_MAX(pwidth, 2);  /* Min size is 2 */
        preview_height = ALIEN_MAX(pheight, 2);

        /* See how we will run */
        switch (run_mode) {
        	case RUN_INTERACTIVE:
        		/* Possibly retrieve data */

        		gimp_get_data("plug_in_alienmap", &wvals);

        		/* Get information from the dialog */

        		if (!alienmap_dialog())
        			return;

        		break;

        	case RUN_NONINTERACTIVE:
        		/* Make sure all the arguments are present */

        		if (nparams != 9)
        			status = STATUS_CALLING_ERROR;

        		if (status == STATUS_SUCCESS)

        			wvals.redstretch = param[3].data.d_int8;
        			wvals.greenstretch = param[4].data.d_int8;
        			wvals.bluestretch = param[5].data.d_int8;
        			wvals.redmode = param[6].data.d_int8;
        			wvals.greenmode = param[7].data.d_int8;
        			wvals.bluemode = param[8].data.d_int8;


        		break;

        	case RUN_WITH_LAST_VALS:
        		/* Possibly retrieve data */

        		gimp_get_data("plug_in_alienmap", &wvals);
        		break;

        	default:
        		break;
        } /* switch */


  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is indexed or RGB color  */
      if (gimp_drawable_color (drawable->id))
        {
          gimp_progress_init ("AlienMap: Transforming ...");

        	/* Set the tile cache size */

        	gimp_tile_cache_ntiles(2*(drawable->width / gimp_tile_width()+1));

        	/* Run! */


/*          gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width ()
        			       + 1));*/
          alienmap (drawable);
        	if (run_mode != RUN_NONINTERACTIVE)
        		gimp_displays_flush();

        	/* Store data */

        	if (run_mode == RUN_INTERACTIVE)
        		gimp_set_data("plug_in_alienmap", &wvals, sizeof(alienmap_vals_t));

        }
      else
        {
/*           gimp_message("This filter only applies on RGB-images"); */
          status = STATUS_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/*****/

static void
alienmap_get_pixel(int x, int y, guchar *pixel)
{
        static gint row  = -1;
        static gint col  = -1;

        gint    newcol, newrow;
        gint    newcoloff, newrowoff;
        guchar *p;
        int     i;

        if ((x < 0) || (x >= img_width) || (y < 0) || (y >= img_height)) {
        	pixel[0] = 0;
        	pixel[1] = 0;
        	pixel[2] = 0;
        	pixel[3] = 0;

        	return;
        } /* if */

        newcol    = x / tile_width; /* The compiler should optimize this */
        newcoloff = x % tile_width;
        newrow    = y / tile_height;
        newrowoff = y % tile_height;

        if ((col != newcol) || (row != newrow) || (the_tile == NULL)) {

        	if (the_tile != NULL)
        		gimp_tile_unref(the_tile, FALSE);

        	the_tile = gimp_drawable_get_tile(drawable, FALSE, newrow, newcol);
        	gimp_tile_ref(the_tile);
        	col = newcol;
        	row = newrow;
        } /* if */
        p = the_tile->data + the_tile->bpp * (the_tile->ewidth * newrowoff + newcoloff);
        for (i = img_bpp; i; i--)
        	*pixel++ = *p++;

} /* alienmap_get_pixel */



static void
alienmap_render_row (const guchar *src_row,
        	  guchar *dest_row,
        	  gint row,
        	  gint row_width,
        	  gint bytes, double redstretch, double greenstretch, double bluestretch)




{
  gint col, bytenum;

  for (col = 0; col < row_width ; col++)
    {
      short int v1, v2, v3;

      v1 = (short int)src_row[col*bytes];
      v2 = (short int)src_row[col*bytes +1];
      v3 = (short int)src_row[col*bytes +2];

      transform(&v1, &v2, &v3, redstretch, greenstretch, bluestretch);

      dest_row[col*bytes] = (int)v1;
      dest_row[col*bytes +1] = (int)v2;
      dest_row[col*bytes +2] = (int)v3;

      if (bytes>3)
        for (bytenum = 3; bytenum<bytes; bytenum++)
          {
            dest_row[col*bytes+bytenum] = src_row[col*bytes+bytenum];
          }
    }
}





static void
alienmap (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  guchar *src_row;
  guchar *dest_row;
  gint row;
  gint x1, y1, x2, y2;
  double redstretch,greenstretch,bluestretch;

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  /*  allocate row buffers  */
  src_row = (guchar *) malloc ((x2 - x1) * bytes);
  dest_row = (guchar *) malloc ((x2 - x1) * bytes);


  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);


  redstretch = wvals.redstretch;
  greenstretch = wvals.greenstretch;
  bluestretch = wvals.bluestretch;

  for (row = y1; row < y2; row++)

    {
      gimp_pixel_rgn_get_row (&srcPR, src_row, x1, row, (x2 - x1));

      alienmap_render_row (src_row,
        		dest_row,
        		row,
        		(x2 - x1),
        		bytes,
        		redstretch, greenstretch, bluestretch);

      /*  store the dest  */
      gimp_pixel_rgn_set_row (&destPR, dest_row, x1, row, (x2 - x1));

      if ((row % 10) == 0)
        gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  /*  update the processed region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  free (src_row);
  free (dest_row);
}

/*****/

static void
build_preview_source_image(void)
{
        double  left, right, bottom, top;
        double  px, py;
        double  dx, dy;
        int     x, y;
        guchar *p;
        guchar  pixel[4];

        wint.image  = g_malloc(preview_width * preview_height * 3 * sizeof(guchar));
        wint.wimage = g_malloc(preview_width * preview_height * 3 * sizeof(guchar));

        left   = sel_x1;
        right  = sel_x2 - 1;
        bottom = sel_y2 - 1;
        top    = sel_y1;

        dx = (right - left) / (preview_width - 1);
        dy = (bottom - top) / (preview_height - 1);

        py = top;

        p = wint.image;

        for (y = 0; y < preview_height; y++) {
        	px = left;
        	for (x = 0; x < preview_width; x++) {
        		alienmap_get_pixel((int) px, (int) py, pixel);

        		*p++ = pixel[0];
        		*p++ = pixel[1];
        		*p++ = pixel[2];

        		px += dx;
        	} /* for */

        	py += dy;
        } /* for */
} /* build_preview_source_image */


static void
set_tooltip (GtkTooltips *tooltips, GtkWidget *widget, const char *desc)
{
  if (desc && desc[0])
    gtk_tooltips_set_tip (tooltips, widget, (char *) desc, NULL);
}


/*****/

static gint
alienmap_dialog(void)
{
        GtkWidget  *dialog;
        GtkWidget  *top_table;
        GtkWidget  *frame;
        GtkWidget  *toggle;
        GtkWidget  *toggle_vbox;
        GtkWidget  *table, *table2, *table3;
        GtkWidget  *button;
        gint        argc;
        gchar     **argv;
        guchar     *color_cube;
        GSList *redmode_group = NULL;
        GSList *greenmode_group = NULL;
        GSList *bluemode_group = NULL;
        do_redsinus = (wvals.redmode == SINUS);
        do_redcosinus = (wvals.redmode == COSINUS);
        do_rednone = (wvals.redmode == NONE);
        do_greensinus = (wvals.greenmode == SINUS);
        do_greencosinus = (wvals.greenmode == COSINUS);
        do_greennone = (wvals.greenmode == NONE);
        do_bluesinus = (wvals.bluemode == SINUS);
        do_bluecosinus = (wvals.bluemode == COSINUS);
        do_bluenone = (wvals.bluemode == NONE);
        /*
        printf("Waiting... (pid %d)\n", getpid());
        kill(getpid(), SIGSTOP);
        */

        argc    = 1;
        argv    = g_new(gchar *, 1);
        argv[0] = g_strdup("alienmap");

        gtk_init(&argc, &argv);
        gtk_rc_parse(gimp_gtkrc());

        gtk_preview_set_gamma(gimp_gamma());
        gtk_preview_set_install_cmap(gimp_install_cmap());
        color_cube = gimp_color_cube();
        gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);

        gtk_widget_set_default_visual(gtk_preview_get_visual());
        gtk_widget_set_default_colormap(gtk_preview_get_cmap());

        build_preview_source_image();
        dialog = maindlg = gtk_dialog_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "AlienMap");
        gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
        gtk_container_border_width(GTK_CONTAINER(dialog), 0);
        gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
        		   (GtkSignalFunc) dialog_close_callback,
        		   NULL);

        top_table = gtk_table_new(4, 4, FALSE);
        gtk_container_border_width(GTK_CONTAINER(top_table), 6);
        gtk_table_set_row_spacings(GTK_TABLE(top_table), 4);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), top_table, FALSE, FALSE, 0);
        gtk_widget_show(top_table);

	/* use black as foreground: */
        tips = gtk_tooltips_new ();
        tips_fg.red   = 0;
        tips_fg.green = 0;
        tips_fg.blue  = 0;
       /* postit yellow (khaki) as background: */
        gdk_color_alloc (gtk_widget_get_colormap (top_table), &tips_fg);
        tips_bg.red   = 61669;
        tips_bg.green = 59113;
        tips_bg.blue  = 35979;
        gdk_color_alloc (gtk_widget_get_colormap (top_table), &tips_bg);
        gtk_tooltips_set_colors (tips,&tips_bg,&tips_fg);

        /* Preview */

        frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
        gtk_table_attach(GTK_TABLE(top_table), frame, 0, 1, 0, 1, 0, 0, 0, 0);
        gtk_widget_show(frame);

        wint.preview = gtk_preview_new(GTK_PREVIEW_COLOR);
        gtk_preview_size(GTK_PREVIEW(wint.preview), preview_width, preview_height);
        gtk_container_add(GTK_CONTAINER(frame), wint.preview);
        gtk_widget_show(wint.preview);
        /* Controls */

        table = gtk_table_new(1, 3, FALSE);
        gtk_container_border_width(GTK_CONTAINER(table), 0);
        gtk_table_attach(GTK_TABLE(top_table), table, 0, 4, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
        gtk_widget_show(table);

        dialog_create_value("R", GTK_TABLE(table), 0, &wvals.redstretch,0,128.00000000000, "Change intensity of the red channel");


        table2 = gtk_table_new(1, 3, FALSE);
        gtk_container_border_width(GTK_CONTAINER(table2), 0);
        gtk_table_attach(GTK_TABLE(top_table), table2, 0, 4, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
        gtk_widget_show(table2);

        dialog_create_value("G", GTK_TABLE(table2), 0, &wvals.greenstretch,0,128.0000000000000, "Change intensity of the green channel");


        table3 = gtk_table_new(1, 3, FALSE);
        gtk_container_border_width(GTK_CONTAINER(table3), 0);
        gtk_table_attach(GTK_TABLE(top_table), table3, 0, 4, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 0);
        gtk_widget_show(table3);

        dialog_create_value("B", GTK_TABLE(table3), 0, &wvals.bluestretch,0,128.00000000000000, "Change intensity of the blue channel");

/*  Redmode toggle box  */
    frame = gtk_frame_new ("Red:");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach (GTK_TABLE (top_table), frame, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    toggle_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
    gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

    toggle = gtk_radio_button_new_with_label (redmode_group, "Sine");
    redmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) alienmap_toggle_update,
        		&do_redsinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_redsinus);
    gtk_widget_show (toggle);
   
    set_tooltip(tips,toggle,"Use sine-function for red component");

    toggle = gtk_radio_button_new_with_label (redmode_group, "Cosine");
    redmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) alienmap_toggle_update,
        		&do_redcosinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_redcosinus);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Use cosine-function for red component");

    toggle = gtk_radio_button_new_with_label (redmode_group, "None");
    redmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) alienmap_toggle_update,
        		&do_rednone);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_rednone);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Red channel: use linear mapping instead of any trigonometrical function");

    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);


/*  Greenmode toggle box  */
    frame = gtk_frame_new ("Green:");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach (GTK_TABLE (top_table), frame, 2, 3, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    toggle_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
    gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

    toggle = gtk_radio_button_new_with_label (greenmode_group, "Sine");
    greenmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) alienmap_toggle_update,
        		&do_greensinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_greensinus);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Use sine-function for green component");

    toggle = gtk_radio_button_new_with_label (greenmode_group, "Cosine");
    greenmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) alienmap_toggle_update,
        		&do_greencosinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_greencosinus);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Use cosine-function for green component");

    toggle = gtk_radio_button_new_with_label (greenmode_group, "None");
    greenmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) alienmap_toggle_update,
        		&do_greennone);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_greennone);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Green channel: use linear mapping instead of any trigonometrical function");

    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);


/*  Bluemode toggle box  */
    frame = gtk_frame_new ("Blue:");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach (GTK_TABLE (top_table), frame, 3, 4, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    toggle_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
    gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

    toggle = gtk_radio_button_new_with_label (bluemode_group, "Sine");
    bluemode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) alienmap_toggle_update,
        		&do_bluesinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_bluesinus);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Use sine-function for blue component");

    toggle = gtk_radio_button_new_with_label (bluemode_group, "Cosine");
    bluemode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) alienmap_toggle_update,
        		&do_bluecosinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_bluecosinus);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Use cosine-function for blue component");

    toggle = gtk_radio_button_new_with_label (bluemode_group, "None");
    bluemode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) alienmap_toggle_update,
        		&do_bluenone);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_bluenone);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Blue channel: use linear mapping instead of any trigonometrical function");

    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);
/*     gtk_widget_show (table); */


        /* Buttons */


gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 6);

        button = gtk_button_new_with_label("OK");
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
        		   (GtkSignalFunc) dialog_ok_callback,
        		   dialog);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
        gtk_widget_grab_default(button);
        gtk_widget_show(button);
        set_tooltip(tips,button,"Accept settings and apply filter on image");

        button = gtk_button_new_with_label("Cancel");
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
        		   (GtkSignalFunc) dialog_cancel_callback,
        		   dialog);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
        gtk_widget_show(button);
        set_tooltip(tips,button,"Reject any changes and close plug-in");

	button = gtk_button_new_with_label("About...");
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC (alienmap_logo_dialog),
			   NULL);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     button, TRUE, TRUE, 0);
        gtk_widget_show(button);
	set_tooltip(tips,button,"Show information about this plug-in and the author");


        /* Done */

        gtk_widget_show(dialog);
        dialog_update_preview();

        gtk_main();
	gtk_object_unref (GTK_OBJECT (tips));
        gdk_flush();
        if (the_tile != NULL) {
        	gimp_tile_unref(the_tile, FALSE);
        	the_tile = NULL;
        } /* if */

        g_free(wint.image);
        g_free(wint.wimage);

        return wint.run;
} /* alienmap_dialog */


/*****/

static void
dialog_update_preview(void)
{
        double  left, right, bottom, top;
        double  dx, dy;
        int  px, py;
        int     x, y;
        double  redstretch, greenstretch, bluestretch;
        short int r,g,b;
        double  scale_x, scale_y;
        guchar *p_ul, *i, *p;

        left   = sel_x1;
        right  = sel_x2 - 1;
        bottom = sel_y2 - 1;
        top    = sel_y1;
        dx = (right - left) / (preview_width - 1);
        dy = (bottom - top) / (preview_height - 1);

        redstretch = wvals.redstretch;
        greenstretch = wvals.greenstretch;
        bluestretch = wvals.bluestretch;

        scale_x = (double) (preview_width - 1) / (right - left);
        scale_y = (double) (preview_height - 1) / (bottom - top);

        py = 0;

        p_ul = wint.wimage;

        for (y = 0; y < preview_height; y++) {
        	px = 0;

        	for (x = 0; x < preview_width; x++) {
        	       i = wint.image + 3 * (preview_width * py + px);
        	       r = *i++;
        	       g = *i++;
        	       b = *i;
        	       transform(&r,&g,&b,redstretch, greenstretch, bluestretch);
        	       p_ul[0] = r;
        	       p_ul[1] = g;
        	       p_ul[2] = b;
        	       p_ul += 3;
        	       px += 1; /* dx; */
        	} /* for */
        	py +=1; /* dy; */
        } /* for */

        p = wint.wimage;

        for (y = 0; y < preview_height; y++) {
        	gtk_preview_draw_row(GTK_PREVIEW(wint.preview), p, 0, y, preview_width);
        	p += preview_width * 3;
        } /* for */
        gtk_widget_draw(wint.preview, NULL);
        gdk_flush();
} /* dialog_update_preview */


/*****/

static void
dialog_create_value(char *title, GtkTable *table, int row, gdouble *value,
        	    int left, int right, const char *desc)
{
        GtkWidget *label;
        GtkWidget *scale;
        GtkWidget *entry;
        GtkObject *scale_data;
        char       buf[256];

        label = gtk_label_new(title);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
        gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
        gtk_widget_show(label);


        scale_data = gtk_adjustment_new(*value, left, right,
        				(right - left) / 128,
        				(right - left) / 128,
        				0);

        gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
        		   (GtkSignalFunc) dialog_scale_update,
        		   value);

        scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
        gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
        gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
        gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
        gtk_scale_set_digits(GTK_SCALE(scale), 3);
        gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
        gtk_widget_show(scale);
        set_tooltip(tips,scale,desc);

        entry = gtk_entry_new();
        gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
        gtk_object_set_user_data(scale_data, entry);
        gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
        sprintf(buf, "%0.2f", *value);
        gtk_entry_set_text(GTK_ENTRY(entry), buf);
        gtk_signal_connect(GTK_OBJECT(entry), "changed",
        		   (GtkSignalFunc) dialog_entry_update,
        		   value);
        gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
        gtk_widget_show(entry);
	set_tooltip(tips,entry,desc);

} /* dialog_create_value */

/*****/

static void
dialog_scale_update(GtkAdjustment *adjustment, gdouble *value)
{
        GtkWidget *entry;
        char       buf[256];

        if (*value != adjustment->value) {
        	*value = adjustment->value;

        	entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
        	sprintf(buf, "%0.2f", *value);

        	gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
        	gtk_entry_set_text(GTK_ENTRY(entry), buf);
        	gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);

        	dialog_update_preview();
        } /* if */
} /* dialog_scale_update */
/*****/

static void
dialog_entry_update(GtkWidget *widget, gdouble *value)
{
        GtkAdjustment *adjustment;
        gdouble        new_value;

        new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));

        if (*value != new_value) {
        	adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));

        	if ((new_value >= adjustment->lower) &&
        	    (new_value <= adjustment->upper)) {
        		*value  	  = new_value;
        		adjustment->value = new_value;

        		gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");

        		dialog_update_preview();
        	} /* if */
        } /* if */
} /* dialog_entry_update */


static void
dialog_close_callback(GtkWidget *widget, gpointer data)
{
        gtk_main_quit();
} /* dialog_close_callback */


/*****/

static void
dialog_ok_callback(GtkWidget *widget, gpointer data)
{
        wint.run = TRUE;
        gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_ok_callback */


/*****/

static void
dialog_cancel_callback(GtkWidget *widget, gpointer data)
{
        gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_cancel_callback */


static void
alienmap_toggle_update (GtkWidget *widget,
        		gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;

  if (do_redsinus)
    wvals.redmode = SINUS;
  else if (do_redcosinus)
    wvals.redmode = COSINUS;
  else if (do_rednone)
    wvals.redmode = NONE;

  if (do_greensinus)
    wvals.greenmode = SINUS;
  else if (do_greencosinus)
    wvals.greenmode = COSINUS;
  else if (do_greennone)
    wvals.greenmode = NONE;

  if (do_bluesinus)
    wvals.bluemode = SINUS;
  else if (do_bluecosinus)
    wvals.bluemode = COSINUS;
  else if (do_bluenone)
    wvals.bluemode = NONE;
  dialog_update_preview();

}

void
alienmap_logo_dialog()
{
  GtkWidget *xlabel;
  GtkWidget *xbutton;
  GtkWidget *xlogo_box;
  GtkWidget *xpreview;
  GtkWidget *xframe,*xframe2;
  GtkWidget *xvbox;
  GtkWidget *xhbox;
  char *text;
  guchar *temp,*temp2;
  guchar *datapointer;
  gint y,x;

  if (!logodlg)
    {
      logodlg = gtk_dialog_new();
      gtk_window_set_title(GTK_WINDOW(logodlg), "About Alien Map");
      gtk_window_position(GTK_WINDOW(logodlg), GTK_WIN_POS_MOUSE);
      gtk_signal_connect(GTK_OBJECT(logodlg),
			 "destroy",
			 GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			 &logodlg);
      gtk_quit_add_destroy (1, GTK_OBJECT (logodlg));
      gtk_signal_connect(GTK_OBJECT(logodlg),
			 "delete_event",
			 GTK_SIGNAL_FUNC (gtk_widget_hide_on_delete),
			 &logodlg);
      
      xbutton = gtk_button_new_with_label("OK");
      GTK_WIDGET_SET_FLAGS(xbutton, GTK_CAN_DEFAULT);
      gtk_signal_connect_object (GTK_OBJECT(xbutton), "clicked",
				 GTK_SIGNAL_FUNC (gtk_widget_hide),
				 GTK_OBJECT(logodlg));
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(logodlg)->action_area),
			 xbutton, TRUE, TRUE, 0);
      gtk_widget_grab_default(xbutton);
      gtk_widget_show(xbutton);
      set_tooltip(tips,xbutton,"This closes the information box");
      
      xframe = gtk_frame_new(NULL);
      gtk_frame_set_shadow_type(GTK_FRAME(xframe), GTK_SHADOW_ETCHED_IN);
      gtk_container_border_width(GTK_CONTAINER(xframe), 10);
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(logodlg)->vbox), xframe, TRUE, TRUE, 0);
      xvbox = gtk_vbox_new(FALSE, 5);
      gtk_container_border_width(GTK_CONTAINER(xvbox), 10);
      gtk_container_add(GTK_CONTAINER(xframe), xvbox);
      
      /*  The logo frame & drawing area  */
      xhbox = gtk_hbox_new (FALSE, 5);
      gtk_box_pack_start (GTK_BOX (xvbox), xhbox, FALSE, TRUE, 0);
      
      xlogo_box = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (xhbox), xlogo_box, FALSE, FALSE, 0);
      
      xframe2 = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (xframe2), GTK_SHADOW_IN);
      gtk_box_pack_start (GTK_BOX (xlogo_box), xframe2, FALSE, FALSE, 0);
      
      xpreview = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_preview_size (GTK_PREVIEW (xpreview), logo_width, logo_height);
      temp = g_malloc((logo_width+10)*3);
      datapointer=header_data+logo_width*logo_height-1;
      for (y = 0; y < logo_height; y++){
	temp2=temp;
	for (x = 0; x< logo_width; x++) {
	  HEADER_PIXEL(datapointer,temp2); temp2+=3;}
	gtk_preview_draw_row (GTK_PREVIEW (xpreview),
			      temp,
			      0, y, logo_width); 
      }			  
      g_free(temp);
      gtk_container_add (GTK_CONTAINER (xframe2), xpreview);
      gtk_widget_show (xpreview);
      gtk_widget_show (xframe2);
      gtk_widget_show (xlogo_box);
      gtk_widget_show (xhbox);
      
      xhbox = gtk_hbox_new(FALSE, 5);
      gtk_box_pack_start(GTK_BOX(xvbox), xhbox, TRUE, TRUE, 0);
      text = "\nCotting Software Productions\n"
	"Bahnhofstrasse 31\n"
	"CH-3066 Stettlen (Switzerland)\n\n"
	"cotting@mygale.org\n"
	"http://www.mygale.org/~cotting\n\n"
	"AlienMap Plug-In for the GIMP\n"
	"Version 1.01\n";
      xlabel = gtk_label_new(text);
      gtk_box_pack_start(GTK_BOX(xhbox), xlabel, TRUE, FALSE, 0);
      gtk_widget_show(xlabel);
      
      gtk_widget_show(xhbox);
      
      gtk_widget_show(xvbox);
      gtk_widget_show(xframe);
      gtk_widget_show(logodlg);
    }
  else
    {
      gtk_widget_show (logodlg);
      gdk_window_raise (logodlg->window);
    }
}
