/**********************************************************************
 *  Julia Chaos Fractal Explorer Plug-in (Version 1.01)
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "logo.h"

/***** Macros *****/

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


/***** Magic numbers *****/

#define PREVIEW_SIZE 128
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  60

#define SINUS 0
#define COSINUS 1
#define NONE 2

/***** Types *****/
typedef struct {
        gdouble xmin;
        gdouble xmax;
        gdouble ymin;
        gdouble ymax;
        gdouble iter;
	gdouble cx;
	gdouble cy;
	gint colormode;
        gdouble redstretch;
        gdouble greenstretch;
        gdouble bluestretch;
        gint redmode;
        gint greenmode;
        gint bluemode;
} Julia_vals_t;

typedef struct {
        GtkWidget *preview;
        guchar    *image;
        guchar    *wimage;
        gint run;
} Julia_interface_t;



/* Declare local functions. */

static void      query  (void);
static void      run    (char      *name,
        		 int        nparams,
        		 GParam    *param,
        		 int       *nreturn_vals,
        		 GParam   **return_vals);

static void      Julia 	    (GDrawable  *drawable);
static void      Julia_render_row  (const guchar *src_row,
        			     guchar *dest_row,
        			     gint row,
        			     gint row_width,gint bytes);
static void      Julia_get_pixel(int x, int y, guchar *pixel);
void    	 transform           (short int *, short int *, short int *,double, double, double);


static void      build_preview_source_image(void);

static gint      Julia_dialog(void);
static void      dialog_update_preview(void);
static void      dialog_create_value(char *title, GtkTable *table, int row, gdouble *value,
        			     int left, int right, const char *desc);
static void      dialog_scale_update(GtkAdjustment *adjustment, gdouble *value);
static void      dialog_create_int_value(char *title, GtkTable *table, int row, gdouble *value,
        			     int left, int right, const char *desc);
static void      dialog_scale_int_update(GtkAdjustment *adjustment, gdouble *value);
static void      dialog_entry_update(GtkWidget *widget, gdouble *value);
static void      dialog_close_callback(GtkWidget *widget, gpointer data);
static void      dialog_ok_callback(GtkWidget *widget, gpointer data);
/* static void      dialog_reset_callback(GtkWidget *widget, gpointer data); */
static void      dialog_cancel_callback(GtkWidget *widget, gpointer data);
static void      Julia_toggle_update    (GtkWidget *widget,
        				    gpointer   data);
static float xmin=-2.0;
static float xmax=2.0;

static float ymin=-2.0;
static float ymax=2.0;
static float cx=-1.0;
static float cy=-0.2;
static float xbild;
static float ybild;
static float xdiff;
static float ydiff;

/***** Variables *****/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static Julia_interface_t wint = {
        NULL,  /* preview */
        NULL,  /* image */
        NULL,  /* wimage */
        FALSE  /* run */
}; /* wint */

static Julia_vals_t wvals = {
        -2.0,2.0,-2.0,2.0,50.0,-1.0,-0.2,0,128,128,128,1,1,0,
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

gint do_colormode1;
gint do_colormode2;

static GParam *ExternalParam=NULL;
static int     ExternalInt;



GtkWidget * Julia_logo_dialog(void);

GtkWidget *maindlg;
GtkWidget *logodlg;
GtkTooltips *tips;
GdkColor tips_fg,tips_bg;	




MAIN ();

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32,    "run_mode",     "Interactive, non-interactive" },
    { PARAM_IMAGE,    "image",        "Input image" },
    { PARAM_DRAWABLE, "drawable",     "Input drawable" },
    { PARAM_FLOAT,    "xmin",  "xmin fractal image delimiter" },
    { PARAM_FLOAT,    "xmax",  "xmax fractal image delimiter" },
    { PARAM_FLOAT,    "ymin",  "ymin fractal image delimiter" },
    { PARAM_FLOAT,    "ymax",  "ymax fractal image delimiter" },
    { PARAM_FLOAT,    "iter",  "Iteration value" },
    { PARAM_FLOAT,    "cx",  "cx value" },
    { PARAM_FLOAT,    "cy",  "cy value" },
    { PARAM_INT8,    "colormode",   "0: Apply colormap as specified by the parameters below; 1: Apply active gradient to final image"},
    { PARAM_FLOAT,    "redstretch",  "Red stretching factor" },
    { PARAM_FLOAT,    "greenstretch","Green stretching factor" },
    { PARAM_FLOAT,    "bluestretch", "Blue stretching factor" },
    { PARAM_INT8,     "redmode",      "Red application mode (0:SIN;1:COS;2:NONE)" },
    { PARAM_INT8,     "greenmode",    "Green application mode (0:SIN;1:COS;2:NONE)" },
    { PARAM_INT8,     "bluemode",     "Blue application mode (0:SIN;1:COS;2:NONE)" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_julia",
        		  "Julia Chaos Fractal Explorer Plug-In",
        		  "Fills the active painting area with a Julia fractal.",
        		  "Daniel Cotting (cotting@mygale.org, www.mygale.org/~cotting)",
        		  "Daniel Cotting (cotting@mygale.org, www.mygale.org/~cotting)",
        		  "1th May 1997",
        		  "<Image>/Filters/Render/Julia Fractal",
        		  "RGB*",
        		  PROC_PLUG_IN,
        		  nargs, nreturn_vals,
        		  args, return_vals);
}

static void
set_tooltip (GtkTooltips *tooltips, GtkWidget *widget, const char *desc)
{
  if (desc && desc[0])
    gtk_tooltips_set_tips (tooltips, widget, (char *) desc);
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
  gint32 image_ID;
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
  image_ID = param[1].data.d_image;
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
        	pwidth  = MIN(sel_width, PREVIEW_SIZE);
        	pheight = sel_height * pwidth / sel_width;
        } else {
        	pheight = MIN(sel_height, PREVIEW_SIZE);
        	pwidth  = sel_width * pheight / sel_height;
        } /* else */

        preview_width  = MAX(pwidth, 2);  /* Min size is 2 */
        preview_height = MAX(pheight, 2);

        /* See how we will run */
        switch (run_mode) {
        	case RUN_INTERACTIVE:
        		/* Possibly retrieve data */

        		gimp_get_data("plug_in_julia", &wvals);

        		/* Get information from the dialog */

        		if (!Julia_dialog())
        			return;

        		break;

        	case RUN_NONINTERACTIVE:
        		/* Make sure all the arguments are present */

        		if (nparams != 17)
        			status = STATUS_CALLING_ERROR;

        		if (status == STATUS_SUCCESS)

        			wvals.xmin = param[3].data.d_float;
        			wvals.xmax = param[4].data.d_float;
        			wvals.ymin = param[5].data.d_float;
        			wvals.ymax = param[6].data.d_float;
        			wvals.iter = param[7].data.d_float;
        			wvals.cx = param[8].data.d_float;
        			wvals.cy = param[9].data.d_float;
        			wvals.colormode = param[10].data.d_int8;
        			wvals.redstretch = param[11].data.d_float;
        			wvals.greenstretch = param[12].data.d_float;
        			wvals.bluestretch = param[13].data.d_float;
        			wvals.redmode = param[14].data.d_int8;
        			wvals.greenmode = param[15].data.d_int8;
        			wvals.bluemode = param[16].data.d_int8;

        		break;

        	case RUN_WITH_LAST_VALS:
        		/* Possibly retrieve data */

        		gimp_get_data("plug_in_julia", &wvals);
        		break;

        	default:
        		break;
        } /* switch */


  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is indexed or RGB color  */
      if (gimp_drawable_color (drawable->id))
        {
          gimp_progress_init ("Rendering Julia Fractal...");

        	/* Set the tile cache size */

        	gimp_tile_cache_ntiles(2*(drawable->width / gimp_tile_width()+1));

        	/* Run! */


/*          gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width ()
        			       + 1));*/
          Julia (drawable);
	  if (wvals.colormode==1)  ExternalParam = gimp_run_procedure("plug_in_gradmap",&ExternalInt,
	                                     PARAM_INT32, run_mode,
					     PARAM_IMAGE, image_ID,
					     PARAM_DRAWABLE, drawable->id,
					     PARAM_END);
	  
        	if (run_mode != RUN_NONINTERACTIVE)
        		gimp_displays_flush();

        	/* Store data */

        	if (run_mode == RUN_INTERACTIVE)
        		gimp_set_data("plug_in_julia", &wvals, sizeof(Julia_vals_t));
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
Julia_get_pixel(int x, int y, guchar *pixel)
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

} /* Julia_get_pixel */



static void
Julia_render_row (const guchar *src_row,
        	  guchar *dest_row,
        	  gint row,
        	  gint row_width,
        	  gint bytes)
{
  gint col, bytenum;
  double redstretch, greenstretch, bluestretch;
  for (col = 0; col < row_width ; col++)
    {
      float a,tb,x,y,xx;
      int zaehler,color,r,g,b;
      float pi=3.1415926;
      cx=wvals.cx;
      cy=wvals.cy;
      redstretch = wvals.redstretch;
      greenstretch = wvals.greenstretch;
      bluestretch = wvals.bluestretch;
      a=xmin+col*xdiff;
      tb=ymin+row*ydiff;
      x=a;
      y=tb;
      zaehler=0;
      for (zaehler=0; (zaehler <= wvals.iter) && ((x*x+y*y)<4); zaehler++)
         {
           xx=x*x-y*y+cx;
           y=2*x*y+cy;
           x=xx;
         }
      color=zaehler*256/wvals.iter;
      r=g=b=color;
  switch (wvals.redmode)
  {
    case SINUS:
       r    = (int) redstretch*(1.0+sin((r/128.0-1)*pi));
       break;
    case COSINUS:
       r    = (int) redstretch*(1.0+cos((r/128.0-1)*pi));
       break;
    default:
    break;
   }

  switch (wvals.greenmode)
  {
    case SINUS:
       g    = (int) greenstretch*(1.0+sin((g/128.0-1)*pi));
       break;
    case COSINUS:
       g    = (int) greenstretch*(1.0+cos((g/128.0-1)*pi));
       break;
    default:
    break;
   }
  switch (wvals.bluemode)
  {
    case SINUS:
       b   = (int) bluestretch*(1.0+sin((b/128.0-1)*pi));
       break;
    case COSINUS:
       b   = (int) bluestretch*(1.0+cos((b/128.0-1)*pi));
       break;
    default:
    break;
   }
   if (r== 256) {r= 255;}
   if (g== 256) {g= 255;}
   if (b== 256) {b= 255;}


      dest_row[col*bytes] =    (int)r;
      dest_row[col*bytes +1] = (int)g;
      dest_row[col*bytes +2] = (int)b;

      if (bytes>3)
        for (bytenum = 3; bytenum<bytes; bytenum++)
          {
            dest_row[col*bytes+bytenum] = src_row[col*bytes+bytenum];
          }
    }
}


static void
Julia_logo_ok_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, TRUE);
  gtk_widget_destroy(logodlg);
}


static void
Julia_about_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, FALSE);
  Julia_logo_dialog();
}



static void
Julia (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  guchar *src_row;
  guchar *dest_row;
  gint row;
  gint x1, y1, x2, y2;

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

  xbild=width;
  ybild=height;
  xdiff=(xmax-xmin)/xbild;
  ydiff=(ymax-ymin)/ybild;

  for (row = y1; row < y2; row++)

    {
      gimp_pixel_rgn_get_row (&srcPR, src_row, x1, row, (x2 - x1));

      Julia_render_row (src_row,
        		dest_row,
        		row,
        		(x2 - x1),
        		bytes);

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
        		Julia_get_pixel((int) px, (int) py, pixel);

        		*p++ = pixel[0];
        		*p++ = pixel[1];
        		*p++ = pixel[2];

        		px += dx;
        	} /* for */

        	py += dy;
        } /* for */
} /* build_preview_source_image */


/*****/

static gint
Julia_dialog(void)
{
        GtkWidget  *dialog;
        GtkWidget  *top_table;
        GtkWidget  *top_table2;
        GtkWidget  *frame;
        GtkWidget  *frame2;
        GtkWidget  *frame3;
        GtkWidget  *toggle;
        GtkWidget  *toggle_vbox;
        GtkWidget  *toggle_vbox2;
        GtkWidget  *toggle_vbox3;
        GtkWidget  *table, *table6;
        GtkWidget  *button;
        gint        argc;
        gchar     **argv;
        guchar     *color_cube;
        GSList *redmode_group = NULL;
        GSList *greenmode_group = NULL;
        GSList *bluemode_group = NULL;
        GSList *colormode_group = NULL;
        
        do_redsinus = (wvals.redmode == SINUS);
        do_redcosinus = (wvals.redmode == COSINUS);
        do_rednone = (wvals.redmode == NONE);
        do_greensinus = (wvals.greenmode == SINUS);
        do_greencosinus = (wvals.greenmode == COSINUS);
        do_greennone = (wvals.greenmode == NONE);
        do_bluesinus = (wvals.bluemode == SINUS);
        do_bluecosinus = (wvals.bluemode == COSINUS);
        do_bluenone = (wvals.bluemode == NONE);
        do_colormode1 = (wvals.colormode == 0);
        do_colormode2 = (wvals.colormode == 1);

        

        argc    = 1;
        argv    = g_new(gchar *, 1);
        argv[0] = g_strdup("Julia");

        gtk_init(&argc, &argv);

        gtk_preview_set_gamma(gimp_gamma());
        gtk_preview_set_install_cmap(gimp_install_cmap());
        color_cube = gimp_color_cube();
        gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);

        gtk_widget_set_default_visual(gtk_preview_get_visual());
        gtk_widget_set_default_colormap(gtk_preview_get_cmap());

        build_preview_source_image();
        dialog = maindlg = gtk_dialog_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "Julia Fractal Explorer (cotting@mygale.org)");
        gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
        gtk_container_border_width(GTK_CONTAINER(dialog), 0);
        gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
        		   (GtkSignalFunc) dialog_close_callback,
        		   NULL);

        top_table = gtk_table_new(4, 4, FALSE);
            gtk_container_border_width(GTK_CONTAINER(top_table), 0);
            gtk_table_set_row_spacings(GTK_TABLE(top_table), 0);
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
               gtk_table_attach(GTK_TABLE(top_table), frame, 0, 1, 0, 1,0, 0, 0, 0);
               gtk_widget_show(frame);

               wint.preview = gtk_preview_new(GTK_PREVIEW_COLOR);
               gtk_preview_size(GTK_PREVIEW(wint.preview), preview_width, preview_height);
               gtk_container_add(GTK_CONTAINER(frame), wint.preview);
               gtk_widget_show(wint.preview);

            /* Controls */

	    frame = gtk_frame_new ("Fractal options:");
               gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
               gtk_table_attach (GTK_TABLE (top_table), frame, 1, 4, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
               gtk_widget_show(frame);

               toggle_vbox = gtk_vbox_new (FALSE, 0);
                  gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 0);
                  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

             	  table = gtk_table_new(7, 7, FALSE);
                     gtk_container_border_width(GTK_CONTAINER(table), 5);
                     gtk_table_set_row_spacings(GTK_TABLE(table), 0);
                     gtk_box_pack_start(GTK_BOX(toggle_vbox),table, FALSE, FALSE, 0);
                     gtk_widget_show(table);
                     dialog_create_value("XMIN", GTK_TABLE(table), 0, &wvals.xmin,-3,3, "Change the first (minimal) x-coordinate delimitation");
                     dialog_create_value("XMAX", GTK_TABLE(table), 1, &wvals.xmax,-3,3, "Change the second (maximal) x-coordinate delimitation");
                     dialog_create_value("YMIN", GTK_TABLE(table), 2, &wvals.ymin,-3,3, "Change the first (minimal) y-coordinate delimitation");
                     dialog_create_value("YMAX", GTK_TABLE(table), 3, &wvals.ymax,-3,3, "Change the second (maximal) y-coordinate delimitation");
                     dialog_create_value("ITER", GTK_TABLE(table), 4, &wvals.iter,0,1000, "Change the iteration value. The higher it is, the more details will be calculated, which will take more time.");
                     dialog_create_value("CX", GTK_TABLE(table), 5, &wvals.cx,-5,5, "Change the CX value (changes aspect of fractal)");
                     dialog_create_value("CY", GTK_TABLE(table), 6, &wvals.cy,-5,5, "Change the CY value (changes aspect of fractal)");
                  gtk_widget_show(table);
               gtk_widget_show(toggle_vbox);
            gtk_widget_show(frame);
		

	    frame2 = gtk_frame_new ("Color options:");
               gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_ETCHED_IN);
               gtk_table_attach (GTK_TABLE (top_table), frame2, 0, 4, 1, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
               gtk_widget_show(frame2);

	       toggle_vbox2 = gtk_vbox_new (FALSE, 0);
                  gtk_container_border_width (GTK_CONTAINER (toggle_vbox2), 0);
                  gtk_container_add (GTK_CONTAINER (frame2), toggle_vbox2);
                  gtk_widget_show(toggle_vbox2);

          	  top_table2 = gtk_table_new(5, 5, FALSE);
                     gtk_container_border_width(GTK_CONTAINER(top_table2), 10);
                     gtk_table_set_row_spacings(GTK_TABLE(top_table2), 0);
                     gtk_box_pack_start(GTK_BOX(toggle_vbox2), top_table2, FALSE, FALSE, 0);
                     gtk_widget_show(top_table2);


	             frame = gtk_frame_new ("Color density:");
                        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
                        gtk_table_attach (GTK_TABLE (top_table2), frame, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
                        gtk_widget_show(frame);

	                toggle_vbox = gtk_vbox_new (FALSE, 0);
                           gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 0);
                           gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);
                           gtk_widget_show(toggle_vbox);

	                   table6 = gtk_table_new(3, 3, FALSE);
                              gtk_container_border_width(GTK_CONTAINER(table6), 0);
                              gtk_table_set_row_spacings(GTK_TABLE(table6), 0);
                              gtk_box_pack_start(GTK_BOX(toggle_vbox), table6, FALSE, FALSE, 0);
                              gtk_widget_show(table6);
                              dialog_create_int_value("Red", GTK_TABLE(table6), 0, &wvals.redstretch,0,128,"Change the intensity of the red channel");
                              dialog_create_int_value("Green", GTK_TABLE(table6), 1, &wvals.greenstretch,0,128,"Change the intensity of the green channel");
                              dialog_create_int_value("Blue", GTK_TABLE(table6), 2, &wvals.bluestretch,0,128,"Change the intensity of the blue channel");
                        gtk_widget_show (toggle_vbox);
                     gtk_widget_show (frame);

                     frame3 = gtk_frame_new ("Color function:");
                        gtk_frame_set_shadow_type (GTK_FRAME (frame3), GTK_SHADOW_ETCHED_IN);
                        gtk_table_attach (GTK_TABLE (top_table2), frame3, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
                        gtk_widget_show (frame3);

                     	toggle_vbox3 = gtk_vbox_new (FALSE, 0);
                           gtk_container_border_width (GTK_CONTAINER (toggle_vbox3), 0);
                           gtk_container_add (GTK_CONTAINER (frame3), toggle_vbox3);
                           gtk_widget_show (toggle_vbox3);
			   
                           table6 = gtk_table_new(4, 4, FALSE);
                              gtk_container_border_width(GTK_CONTAINER(table6), 0);
                              gtk_table_set_row_spacings(GTK_TABLE(table6), 0);
                              gtk_box_pack_start(GTK_BOX(toggle_vbox3), table6, FALSE, FALSE, 0);
                              gtk_widget_show(table6);

                              frame = gtk_frame_new ("Red:");
                                 gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
                                 gtk_table_attach (GTK_TABLE (table6), frame, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
                                 gtk_widget_show (frame);
				 
                                 toggle_vbox = gtk_vbox_new (FALSE, 0);
                                    gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 0);
                                    gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);
                                    gtk_widget_show (toggle_vbox);
/*   <-------------------------------/ */
    toggle = gtk_radio_button_new_with_label (redmode_group, "Sine");
    redmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) Julia_toggle_update,
        		&do_redsinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_redsinus);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Use sine-function for red component");

    toggle = gtk_radio_button_new_with_label (redmode_group, "Cosine");
    redmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) Julia_toggle_update,
        		&do_redcosinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_redcosinus);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Use cosine-function for red component");

    toggle = gtk_radio_button_new_with_label (redmode_group, "None");
    redmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) Julia_toggle_update,
        		&do_rednone);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_rednone);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Red channel: Use linear mapping instead of any trigonometrical function");

    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);


/*  Greenmode toggle box  */
    frame = gtk_frame_new ("Green:");
       gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
       gtk_table_attach (GTK_TABLE (table6), frame, 1, 2, 0,1 , GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
       gtk_widget_show (frame);
       toggle_vbox = gtk_vbox_new (FALSE, 0);
          gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 0);
          gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);
	  gtk_widget_show (toggle_vbox);

/*   <-----/ */
    toggle = gtk_radio_button_new_with_label (greenmode_group, "Sine");
    greenmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) Julia_toggle_update,
        		&do_greensinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_greensinus);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Use sine-function for green component");

    toggle = gtk_radio_button_new_with_label (greenmode_group, "Cosine");
    greenmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) Julia_toggle_update,
        		&do_greencosinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_greencosinus);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Use cosine-function for green component");

    toggle = gtk_radio_button_new_with_label (greenmode_group, "None");
    greenmode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) Julia_toggle_update,
        		&do_greennone);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_greennone);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Green channel: Use linear mapping instead of any trigonometrical function");

    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);


/*  Bluemode toggle box  */
    frame = gtk_frame_new ("Blue:");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach (GTK_TABLE (table6), frame, 2, 3,0,1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    gtk_widget_show (frame);
    toggle_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 0);
    gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);
    gtk_widget_show (toggle_vbox);

    toggle = gtk_radio_button_new_with_label (bluemode_group, "Sine");
    bluemode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) Julia_toggle_update,
        		&do_bluesinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_bluesinus);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Use sine-function for blue component");

    toggle = gtk_radio_button_new_with_label (bluemode_group, "Cosine");
    bluemode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) Julia_toggle_update,
        		&do_bluecosinus);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_bluecosinus);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Use cosine-function for blue component");

    toggle = gtk_radio_button_new_with_label (bluemode_group, "None");
    bluemode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) Julia_toggle_update,
        		&do_bluenone);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_bluenone);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Blue channel: Use linear mapping instead of any trigonometrical function");

    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);
    gtk_widget_show (toggle_vbox3);
    gtk_widget_show (frame3);

/*  Colormode toggle box  */
    frame = gtk_frame_new ("Color Mode:");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach (GTK_TABLE (top_table2), frame, 0, 1, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    toggle_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 0);
    gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

    toggle = gtk_radio_button_new_with_label (colormode_group, "As specified above");
    bluemode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) Julia_toggle_update,
        		&do_colormode1);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_colormode1);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Create a color-map with the options you specified above (color density/function). The result is visible in the preview image");

    toggle = gtk_radio_button_new_with_label (bluemode_group, "Apply active gradient to final image");
    bluemode_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
        		(GtkSignalFunc) Julia_toggle_update,
        		&do_colormode2);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), do_colormode2);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"Create a color-map using a gradient from the gradient editor. The result is only visible in the final image. The preview will be calculated with the color-map derived from the color options above (color density/function).");

    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);
/*     gtk_widget_show (table); */


        /* Buttons */


gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 5);

        button = gtk_button_new_with_label("OK");
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
        		   (GtkSignalFunc) dialog_ok_callback,
        		   dialog);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
        gtk_widget_grab_default(button);
        gtk_widget_show(button);
        set_tooltip(tips,button,"Accept settings and start the calculation of the fractal");

        button = gtk_button_new_with_label("Cancel");
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
        		   (GtkSignalFunc) dialog_cancel_callback,
        		   dialog);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
        gtk_widget_show(button);
        set_tooltip(tips,button,"Discard any changes and close dialog box");
	
	button = gtk_button_new_with_label("About...");
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)Julia_about_callback,button);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     button, TRUE, TRUE, 0);
        gtk_widget_show(button);
        set_tooltip(tips,button,"Show information about the plug-in and the author");

/*
	button = gtk_button_new_with_label("Reset");
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
        		   (GtkSignalFunc) dialog_reset_callback,
        		   dialog);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
        gtk_widget_show(button);
*/
        /* Done */

        gtk_widget_show(dialog);
        dialog_update_preview();

        gtk_main();
        gdk_flush();
        if (the_tile != NULL) {
        	gimp_tile_unref(the_tile, FALSE);
        	the_tile = NULL;
        } /* if */

        g_free(wint.image);
        g_free(wint.wimage);

        return wint.run;
} /* Julia_dialog */


/*****/

static void
dialog_update_preview(void)
{
        double  left, right, bottom, top;
        double  dx, dy;
        int  px, py;
        int     x, y;
        double  redstretch, greenstretch, bluestretch;
        int r,g,b;
        double  scale_x, scale_y;
        guchar *p_ul, *i, *p;
        float ta,tb,tx,ty,txx;
        int zaehler,color;
        float pi=3.1415926;

        left   = sel_x1;
        right  = sel_x2 - 1;
        bottom = sel_y2 - 1;
        top    = sel_y1;
        dx = (right - left) / (preview_width - 1);
        dy = (bottom - top) / (preview_height - 1);

        xmin = wvals.xmin;
        xmax = wvals.xmax;
        ymin = wvals.ymin;
        ymax = wvals.ymax;
	cx=wvals.cx;
        cy=wvals.cy;

        redstretch = wvals.redstretch;
        greenstretch = wvals.greenstretch;
        bluestretch = wvals.bluestretch;
        xbild=preview_width;
        ybild=preview_height;
        xdiff=(xmax-xmin)/xbild;
        ydiff=(ymax-ymin)/ybild;

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
        	       ta=(float)xmin+(float)x*xdiff;
        	       tb=(float)ymin+(float)y*ydiff;
        	       tx=ta;
        	       ty=tb;
        	       zaehler=0;
        	       for (zaehler=0; (zaehler <= wvals.iter) && ((tx*tx+ty*ty)<4); zaehler++)
        		   {
        		      txx=tx*tx-ty*ty+cx;
        		      ty=2.0*tx*ty+cy;
        		      tx=txx;
        		    }
        	       r=g=b=color=zaehler*256/wvals.iter;
  switch (wvals.redmode)
  {
    case SINUS:
       r    = (int) redstretch*(1.0+sin((r/128.0-1)*pi));
       break;
    case COSINUS:
       r    = (int) redstretch*(1.0+cos((r/128.0-1)*pi));
       break;
    default:
    break;
   }

  switch (wvals.greenmode)
  {
    case SINUS:
       g    = (int) greenstretch*(1.0+sin((g/128.0-1)*pi));
       break;
    case COSINUS:
       g    = (int) greenstretch*(1.0+cos((g/128.0-1)*pi));
       break;
    default:
    break;
   }
  switch (wvals.bluemode)
  {
    case SINUS:
       b   = (int) bluestretch*(1.0+sin((b/128.0-1)*pi));
       break;
    case COSINUS:
       b   = (int) bluestretch*(1.0+cos((b/128.0-1)*pi));
       break;
    default:
    break;
   }
   if (r== 256) {r= 255;}
   if (g== 256) {g= 255;}
   if (b== 256) {b= 255;}


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
        				(right - left) / 1000,
        				(right - left) / 1000,
        				0);

        gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
        		   (GtkSignalFunc) dialog_scale_update,
        		   value);

        scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
        gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
        gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
        gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
        gtk_scale_set_digits(GTK_SCALE(scale), 3);
        gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
        gtk_widget_show(scale);
        set_tooltip(tips,scale,desc);

        entry = gtk_entry_new();
        gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
        gtk_object_set_user_data(scale_data, entry);
        gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
        sprintf(buf, "%0.4f", *value);
        gtk_entry_set_text(GTK_ENTRY(entry), buf);
        gtk_signal_connect(GTK_OBJECT(entry), "changed",
        		   (GtkSignalFunc) dialog_entry_update,
        		   value);
        gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, 0,0,4, 0);
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
        	sprintf(buf, "%0.4f", *value);

        	gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
        	gtk_entry_set_text(GTK_ENTRY(entry), buf);
        	gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);

        	dialog_update_preview();
        } /* if */
} /* dialog_scale_update */
/*****/

static void
dialog_create_int_value(char *title, GtkTable *table, int row, gdouble *value,
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
        				(right - left) / 200,
        				(right - left) / 200,
        				0);

        gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
        		   (GtkSignalFunc) dialog_scale_int_update,
        		   value);

        scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
        gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
        gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
        gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
        gtk_scale_set_digits(GTK_SCALE(scale), 3);
        gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
        gtk_widget_show(scale);
        set_tooltip(tips,scale,desc);

        entry = gtk_entry_new();
        gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
        gtk_object_set_user_data(scale_data, entry);
        gtk_widget_set_usize(entry, ENTRY_WIDTH-20, 0);
        sprintf(buf, "%i", (int)*value);
        gtk_entry_set_text(GTK_ENTRY(entry), buf);
        gtk_signal_connect(GTK_OBJECT(entry), "changed",
        		   (GtkSignalFunc) dialog_entry_update,
        		   value);
        gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, 0,0,4, 0);
        gtk_widget_show(entry);
	set_tooltip(tips,entry,desc);

} /* dialog_create_int_value */

/*****/

static void
dialog_scale_int_update(GtkAdjustment *adjustment, gdouble *value)
{
        GtkWidget *entry;
        char       buf[256];

        if (*value != adjustment->value) {
        	*value = adjustment->value;

        	entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
        	sprintf(buf, "%i", (int) *value);

        	gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
        	gtk_entry_set_text(GTK_ENTRY(entry), buf);
        	gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);

        	dialog_update_preview();
        } /* if */
} /* dialog_scale_int_update */
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
/*
static void
dialog_reset_callback(GtkWidget *widget, gpointer data)
{
static Julia_vals_t wvals = {
        -2,1,-1.5,1.5,100,0,128,128,128,0,0,0,
}; 
        gtk_widget_destroy(GTK_WIDGET(data));
        Julia_dialog();
}
*/

/*****/

static void
dialog_cancel_callback(GtkWidget *widget, gpointer data)
{
        gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_cancel_callback */


static void
Julia_toggle_update (GtkWidget *widget,
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

 if (do_colormode1)
    wvals.colormode = 0;
  else if (do_colormode2)
    wvals.colormode = 1;
  dialog_update_preview();

}

GtkWidget * 
Julia_logo_dialog()
{
  GtkWidget *xdlg;
  GtkWidget *xlabel;
  GtkWidget *xbutton;
  GtkWidget *xlogo_box;
  GtkWidget *xpreview;
  GtkWidget *xframe,*xframe2;
  GtkWidget *xvbox;
  GtkWidget *xhbox;
  char *text;
  gchar *temp,*temp2;
  char *datapointer;
  gint y,x;
  xdlg = logodlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(xdlg), "About");
  gtk_window_position(GTK_WINDOW(xdlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(xdlg), "destroy",
                     (GtkSignalFunc)dialog_close_callback,
		     NULL);

  xbutton = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(xbutton, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(xbutton), "clicked",
                     (GtkSignalFunc)Julia_logo_ok_callback,
		     xdlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(xdlg)->action_area),
		     xbutton, TRUE, TRUE, 0);
  gtk_widget_grab_default(xbutton);
  gtk_widget_show(xbutton);
  set_tooltip(tips,xbutton,"This will close the information box");

  xframe = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(xframe), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(xframe), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(xdlg)->vbox), xframe, TRUE, TRUE, 0);
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
          "Julia Fractal Chaos Explorer\nPlug-In for the GIMP\n"
          "Version 1.01\n";
  xlabel = gtk_label_new(text);
  gtk_box_pack_start(GTK_BOX(xhbox), xlabel, TRUE, FALSE, 0);
  gtk_widget_show(xlabel);

  gtk_widget_show(xhbox);

  gtk_widget_show(xvbox);
  gtk_widget_show(xframe);
  gtk_widget_show(xdlg);

  gtk_main();
  gdk_flush();
  return xdlg;
}
