/* refract.c, version 0.1.0-alpha, 20 October 1997 
 * A plug-in for the GIMP 0.99.
 * Uses a height field as a lens of specified refraction index.
 *
 * by Kevin Turner <kevint@poboxes.com>
 * http://www.poboxes.com/kevint/gimp/refract.html
 *
 * Check that web page for a more complete description of what the
 * plug-in does and does _not_ do. */

/* I require megawidgets to compile!  A copy was probably compiled in
   the plug-ins directory of your GIMP source distribution, it will
   work nicely.  Just move me or it somewhere I can see it...  */

/* THIS IS AN ALPHA RELEASE.

   The code is ugly, and it the plug-in is NOT full featured and I
   know I still have work left to do before it is.  Hopefully I'll
   have much done by the end of October...  But I thought I'd go with
   "release early, release often" in case anyone cares to improve an
   alogrythm (or my spelling) or something...

   But it's fun enough that I thought I'd let people play with it...  
   So enjoy, and keep on hackin'.  */


/*
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
 *
 */

/* I'm not a very expirenced C programmer, so questions, comments, and
 * reservations on code and style are more than welcome. */

/* Pixel fetcher routines are from Quartic's whirlpinch plug-in.
   Thanks, Quartic! */

/* TO DO:
 * Preview
 * ToolTips
 * Variable IOR information in some [alpha?] channel.
 * Reflections
 * Dispersion or diffraction or whatever that thing that makes rainbows is called.
 * Lighting
 * */

/* Refresher course in optics:
   Incident ray is the light ray hitting the surface.
   Angles are measured from the perpendicular to the surface.
   Angle of reflection is equal to angle of incidence.
   
   Snell's law: index[a] * sin(a) = index[b] * sin(b)

   If second index is smaller than first, light is bent toward normal.
   Otherwise, away.
   */

/* Common indexes of refraction:
   (for yellow sodium light, 589 nm)

   Air: 1.0003 (call it 1)

         Ice: 1.309   
    Flourite: 1.434
   Rock Salt: 1.544
      Quartz: 1.544
      Zircon: 1.923
     Diamond: 2.417
     
   Crown glass: 1.52
   
           Water: 1.333
   Ethyl alcohol: 1.36
      Turpentine: 1.472
       Glycerine: 1.473
       */

/* Comment out the #define REFRACT_DEBUG for distribution releases, or
 * if you get unwanted diagnostic noise from refract on stdout or
 * stderr... */

#define REFRACT_DEBUG

#ifdef REFRACT_DEBUG
#include <stdio.h>
#include <signal.h>
#endif /* DEBUG */

#include <stdlib.h>
#include <math.h>      /* It's not clear to me if this needs be here or no... */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h" 

#include "../megawidget.h" /* For entry/scale pairs. */

#ifndef REFRACT_DEBUG
#define REFRACT_TITLE "Refract 0.1.0-Alpha"
#else
#define REFRACT_TITLE "Refract 0.1.0-Alpha (debug)"
#endif

typedef struct {
    gint32 lensmap; /* lens map id */
    gint32 depth; /* lens depth */
    gint32 dist;  /* distance */
    gdouble na;   /* index a */
    gdouble nb;   /* index b */
    gint wrap;    /* wrap/transparent */
    gint newl;    /* new layer? */
    gint xofs;    /* offset x */
    gint yofs;    /* offset y */
} RefractValues;

typedef struct {
    gint x;
    gint y;
} EksWhy;

typedef struct {
    gint run;
} RefractInterface;

typedef struct { /* Quartic's pixelfetcher thing */
	gint       col, row;
	gint       img_width, img_height, img_bpp, img_has_alpha;
	gint       tile_width, tile_height;
	guchar     bg_color[4];
	GDrawable *drawable;
	GTile     *tile;
} pixel_fetcher_t;



static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);

static void      refract (GDrawable * drawable);
static gint      delta (gdouble *offset, gdouble slope, gint height);
static gint      refract_dialog();
static gint      map_constrain(gint32 image_id, 
			       gint32 drawable_id,
			       gpointer data);
static void      refract_close_callback(GtkWidget *widget, 
					gpointer data);
static void      refract_ok_callback(GtkWidget *widget, 
				     gpointer data);
static void      map_menu_callback (gint32 id, 
				    gpointer data);

/* More pixelfetcher things */
static pixel_fetcher_t *pixel_fetcher_new(GDrawable *drawable);
static void             pixel_fetcher_set_bg_color(pixel_fetcher_t *pf, guchar r, guchar g, guchar b, guchar a);
static void             pixel_fetcher_get_pixel(pixel_fetcher_t *pf, int x, int y, guchar *pixel);
static void             pixel_fetcher_destroy(pixel_fetcher_t *pf);

/* This bilinear interpolation function also borrowed. */
static guchar    bilinear        (gdouble    x,
				  gdouble    y,
				  guchar *   v);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static RefractValues refractvals = 
{
    -1,     /* Lens map ID */
    20,     /* lens depth */
    10,     /* distance */
    1.0003, /* index a */
    1.333,  /* index b */
    0,      /* 0 = wrap, 1 = transparent */
    FALSE,  /* new layer? */
    0,      /* offset x */
    0,      /* offset y */
};

static RefractInterface refractint =
{
    FALSE  /* run */
};

gint sel_x1=-1,sel_x2=-1,sel_y1=-1,sel_y2=-1; /* pixel_fetcher uses these as globals and I'm too lazy to make it do otherwise. */

MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    /* If we did have parameters, these be them: */
    { PARAM_DRAWABLE, "lensmap", "Lens map drawable" },
    { PARAM_INT32, "depth", "Lens depth" },
    { PARAM_INT32, "dist", "Lens distance from image" },
    { PARAM_FLOAT, "na", "Index of Refraction A" },
    { PARAM_FLOAT, "nb", "Index of Refraction B" },
    { PARAM_INT32, "wrap", "Wrap (0), Background (1)" },
    { PARAM_INT32, "newl", "New layer?" },
    { PARAM_INT32, "xofs", "X offset" },
    { PARAM_INT32, "yofs", "Y offset" }
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_refract",
			  "Uses a height field as a lens.",
			  "Distorts the image by refracting it through a height field 'lens' with a specified index of refraction.",
			  "Kevin Turner <kevint@poboxes.com>",
			  "Kevin Turner",
			  "1997",
			  "<Image>/Filters/Distorts/Refract&Reflect...",
			  "RGB*, GRAY*, INDEXED*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
} /* query */

static void
run    (gchar    *name,
	gint      nparams,
	GParam   *param,
	gint     *nreturn_vals,
	GParam  **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

#ifdef REFRACT_DEBUG
  printf("refract: pid %d\n", getpid());
#endif

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode) {
  case RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_refract", &refractvals);
      
      /* Acquire info with a dialog */
      if (! refract_dialog ()) {
	  gimp_drawable_detach (drawable);
	  return;
      }
      break;

  case RUN_NONINTERACTIVE:
      if (status == STATUS_SUCCESS) {
	  refractvals.lensmap = param[3].data.d_drawable;
	  refractvals.depth = param[4].data.d_int32;
	  refractvals.dist = param[5].data.d_int32;
	  refractvals.na = param[6].data.d_float;
	  refractvals.nb = param[7].data.d_float;
	  refractvals.wrap = param[8].data.d_int32;
	  refractvals.newl = param[9].data.d_int32;
	  refractvals.xofs = param[10].data.d_int32;
	  refractvals.yofs = param[11].data.d_int32;
      } /* if */

      break;

  case RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_refract", &refractvals);
      break;

  default:
      break;
  } /* switch run_mode */
  
  if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id) || gimp_drawable_indexed (drawable->id)) {
      gimp_progress_init ("Doing optics homework...");

      gimp_tile_cache_ntiles(2 * (drawable->width + gimp_tile_width() - 1) / gimp_tile_width()); /* Maybe need this to not segsev?  What's it do? */

      refract (drawable);
      
      if (run_mode != RUN_NONINTERACTIVE)
	  gimp_displays_flush ();
      
      if (run_mode == RUN_INTERACTIVE /*|| run_mode == RUN_WITH_LAST_VALS*/)         
	  gimp_set_data ("plug_in_refract", &refractvals, sizeof (RefractValues));
  } else {
      status = STATUS_EXECUTION_ERROR;
  }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
} /* run */


static void
refract( GDrawable * drawable)
{

    GPixelRgn src_rgn, dest_rgn;
    GPixelRgn lens_rgn;

    guchar *lm_rowm2, *lm_rowm1, *lm_row0;
    guchar *lm_rowp1, *lm_rowp2, *lm_rowfoo;
    guchar *dest, *dest_row;

    GDrawable *lensmap;
    gint lm_width, lm_row_width, lm_height; /* Lensmap stuff */
    gint lm_bpp, lm_has_alpha;

    gint x1, y1, x2, y2; /* Bounds of the selection */
    gint x, y, i;

    gdouble depths; /* Depth scalar */
    gdouble dx, dy;
    gint xf, yf;

    const gint h=1; /* The delta value for the slope interpolation equation. */
    /* FIXME: Give option of changing h for large maps.  */

    gint img_width, img_height, img_has_alpha, img_bpp;
    pixel_fetcher_t *pf;
    guchar bg_color[4], fg_color[4];

    guchar           pixel[4][4];
    guchar           values[4];

    /* Set up pixel fetcher...  */
    pf = pixel_fetcher_new(drawable);

    gimp_palette_get_background(&bg_color[0], &bg_color[1], &bg_color[2]);
    gimp_palette_get_foreground(&fg_color[0], &fg_color[1], &fg_color[2]);
    fg_color[3] = 255;

    img_width = gimp_drawable_width(drawable->id);
    img_height = gimp_drawable_height(drawable->id);
    img_has_alpha = gimp_drawable_has_alpha(drawable->id);
    img_bpp = gimp_drawable_bpp(drawable->id);

    pixel_fetcher_set_bg_color(pf,
			       bg_color[0],
			       bg_color[1],
			       bg_color[2],
			       (img_has_alpha ? 0 : 255));

    gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

    sel_x1=x1,sel_y1=y1,sel_x2=x2,sel_y2=y2;
    /* Source region: */
    gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2-x1), (y2-y1), FALSE, FALSE);

    /* Initialize destion region: */
    gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2-x1), (y2-y1), TRUE, TRUE);
    /* FIXME: "drawable" should be a new layer if so desired. */

    /* Get the lens map... */
    lensmap = gimp_drawable_get (refractvals.lensmap);

    lm_width     = gimp_drawable_width(refractvals.lensmap);
    lm_height    = gimp_drawable_height(refractvals.lensmap);
    lm_bpp       = gimp_drawable_bpp(refractvals.lensmap);
    lm_has_alpha = gimp_drawable_has_alpha(refractvals.lensmap);

    gimp_pixel_rgn_init (&lens_rgn, lensmap, x1, y1, lm_width, lm_height, FALSE, FALSE);

    dest_row  = g_malloc((x2-x1) * img_bpp * sizeof(guchar));

    /* FIXME: X and Y offsets not yet used. */

    if ((x2-x1) >= lm_width) { /* If we need the entire lens map... */
	lm_row_width = lm_width;
    } else if ((x1+lm_width) < x2) { /* Image is smaller than lens map, and doesn't */
	                            /* require wrapping over the edge... */
	lm_row_width = x2-x1;
    } else {
	puts("refract: Please don't make life more complicated than it needs to be.\n");
    }

    if (lm_row_width > 0) {
	lm_rowm2 = g_malloc(lm_row_width * lm_bpp * sizeof(guchar));
	lm_rowm1 = g_malloc(lm_row_width * lm_bpp * sizeof(guchar));
	lm_row0 = g_malloc(lm_row_width * lm_bpp * sizeof(guchar));
	lm_rowp1 = g_malloc(lm_row_width * lm_bpp * sizeof(guchar));
	lm_rowp2 = g_malloc(lm_row_width * lm_bpp * sizeof(guchar));
	
	y = ((y1-2) < 0) ? (lm_height - 2) : ((y1-2) % lm_height);
	gimp_pixel_rgn_get_row(&lens_rgn, lm_rowm2, 0, y, lm_row_width);
	
	y = ((y1-1) < 0) ? (lm_height - 1) : ((y1-1) % lm_height);
	gimp_pixel_rgn_get_row(&lens_rgn, lm_rowm1, 0, y, lm_row_width);
	
	gimp_pixel_rgn_get_row(&lens_rgn, lm_row0, 0, (y1 % lm_height), lm_row_width);
	
	gimp_pixel_rgn_get_row(&lens_rgn, lm_rowp1, 0, (y1 +1) % lm_height, lm_row_width);
	gimp_pixel_rgn_get_row(&lens_rgn, lm_rowp2, 0, (y1 +2) % lm_height, lm_row_width);
    } else { 
	puts("refract: Row buffers not initalized.\n");
    }
	

    depths = (gdouble) refractvals.depth/ (gdouble) 256.0;

    for (y=y1; y < y2; y++) {

	gimp_pixel_rgn_get_row(&dest_rgn, dest_row, x1, y, (x2-x1));
	dest = dest_row;

	for (x=x1; x < x2; x++) {

	    /* So on a scale of 1 to 100, how far below zero does this
               rank for coding style?  */

#define ROWM2 (lm_rowm2[ x % lm_width * lm_bpp ])
#define ROWM1 (lm_rowm1[ x % lm_width * lm_bpp ])
#define ROW0(X)  (lm_row0[  (x + (X)) % lm_width * lm_bpp ]) /* FIXME: If x + X < 0  . . . */
#define ROWP1 (lm_rowp1[ x % lm_width * lm_bpp ])
#define ROWP2 (lm_rowp2[ x % lm_width * lm_bpp ])

#define SLOPE_X ((gdouble) 1.0 / (12 * h) * ( ROW0(-2) - 8 * ROW0(-1) + 8 * ROW0(1) - ROW0(2) ) * depths )
#define SLOPE_Y ((gdouble) 1.0 / (12 * h) * ( ROWM2    - 8 * ROWM1    + 8 * ROWP1   - ROWP2   ) * depths )

#if 0 /* The old slope equations. */
#define SLOPE_X	(gdouble) ((lm_row2[ ( (x+1) % lm_width) * lm_bpp ] - \
                           lm_row2[ (((x-1) < 0) ? \
				     lm_width : \
				     ((x-1) % lm_width)) * lm_bpp ]) \
	                   / 2.0) * depths

#define SLOPE_Y (gdouble) ((lm_row3[ (  x    % lm_width) * lm_bpp ] - \
                            lm_row1[ (  x    % lm_width) * lm_bpp ]) \
                           / 2.0) * depths
#endif
			      
		if (delta(&dx, SLOPE_X, ROW0(0) * depths) &&
		    delta(&dy, SLOPE_Y, ROW0(0) * depths)) {

		    if (refractvals.wrap) {
			xf = (x + (gint) dx) % img_width;
			yf = (y + (gint) dy) % img_height;
		    } else {
			xf = x + (gint) dx;
			yf = y + (gint) dy;
		    }

		    pixel_fetcher_get_pixel(pf, xf,     yf,     pixel[0]);
		    pixel_fetcher_get_pixel(pf, xf + 1, yf,     pixel[1]);
		    pixel_fetcher_get_pixel(pf, xf,     yf + 1, pixel[2]);
		    pixel_fetcher_get_pixel(pf, xf + 1, yf + 1, pixel[3]);
		    
		    for (i = 0; i < img_bpp; i++) {
			values[0] = pixel[0][i];
			values[1] = pixel[1][i];
			values[2] = pixel[2][i];
			values[3] = pixel[3][i];
			
			*dest++ = bilinear(dx, dy, values);
		    } /* for */

		} else { /* if a delta() call returns FALSE */
		      for (i = 0; i < img_bpp; i++) {
			  *dest++ = fg_color[i];
		      } /* for */
		}/* if */
		
	} /* next x */

	gimp_pixel_rgn_set_row(&dest_rgn, dest_row, x1, y, (x2-x1));

	if (!(y % 16)) /* On the theory that a % takes less time than an update... */
	    gimp_progress_update((double) (y-y1) / (double) (y2-y1));

	lm_rowfoo=lm_rowm2;
	 lm_rowm2=lm_rowm1;
	 lm_rowm1=lm_row0;
	 lm_row0= lm_rowp1;
	 lm_rowp1=lm_rowp2;
	 lm_rowp2=lm_rowfoo;

	gimp_pixel_rgn_get_row(&lens_rgn, lm_rowp2, x1, ((y+3) % lm_height), lm_row_width);

    } /* next y */

    pixel_fetcher_destroy(pf);

    g_free(dest_row);

    g_free(lm_rowm2);g_free(lm_rowm1);
    g_free(lm_row0);
    g_free(lm_rowp1);g_free(lm_rowp2);

    gimp_drawable_flush(drawable);
    gimp_drawable_merge_shadow(drawable->id, TRUE);
    gimp_drawable_update(drawable->id, x1, y1, (x2-x1), (y2-y1));

} /* refract */    

static gint delta(gdouble *offset, gdouble slope, gint height)
{
    gdouble alpha, beta;
    
    alpha = atan(slope);

    if( alpha > asin( refractvals.nb / refractvals.na )) {
	return FALSE; /* Total Internal Refraction.  Aiee! */
    }
    
    beta = asin(refractvals.na * sin(alpha)/refractvals.nb);
    *offset = -(refractvals.dist + height) * tan(beta - alpha); 

    return TRUE;
}

static gint refract_dialog()
{
    gint        argc;
    gchar     **argv;

    GtkWidget *menu, *option_menu;
    GtkWidget *button;
    GtkWidget *dlg;
    GtkWidget *table;
    GtkWidget *label;

#ifdef REFRACT_DEBUG
#if 0
    printf("refract: waiting... (pid %d)\n", getpid());
    kill(getpid(), SIGSTOP);
#endif
#endif

    /* Standard GTK startup sequence */
    argc = 1;
    argv = g_new (gchar *, 1);
    argv[0] = g_strdup ("refract");

    gtk_init (&argc, &argv);

    /* If you find that you *do* need this uncommented, please let me know:
       gdk_set_use_xshm(gimp_use_xshm()); */

    /* end standard GTK startup */

    /* I guess we need a window... */
    dlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dlg), REFRACT_TITLE);
    gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
		       (GtkSignalFunc) refract_close_callback,
		       NULL);

    /* Action area: */

    /* OK */
    button = gtk_button_new_with_label ("OK");
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			(GtkSignalFunc) refract_ok_callback, dlg);
    gtk_widget_grab_default (button);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
			TRUE, TRUE, 0);
    gtk_widget_show (button);

    /* Cancel */
    button = gtk_button_new_with_label ("Cancel");
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       (GtkSignalFunc) refract_close_callback,
			       NULL);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
			TRUE, TRUE, 0);
    gtk_widget_show (button);

    /* Paramater settings: */
    table = gtk_table_new(7, 3, FALSE);
    gtk_container_add(GTK_CONTAINER (GTK_DIALOG (dlg)->vbox),table);
    gtk_widget_show (table);

    /* FIXME: add preview box */

    /* drop box for lens map */
    label = gtk_label_new("Lens map");

    option_menu = gtk_option_menu_new();

    menu = gimp_drawable_menu_new(map_constrain, map_menu_callback,
				      NULL, refractvals.lensmap);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu),menu);

    gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,0,1);
    gtk_table_attach_defaults(GTK_TABLE(table),option_menu,1,3,0,1);
    gtk_widget_show(label);
    gtk_widget_show(option_menu);

    /* entry/scale for depth of lens */

    mw_iscale_entry_new(table, "Depth", 
			-256, 256, 
			1, 10, 0,
			0, 2, 1, 2,
			&refractvals.depth);

    /* entry/scale pair for distance */
    mw_iscale_entry_new(table, "Distance", 
			0, 1000, 
			1, 10, 0/*what's this do?*/,
			0, 2, 2, 3,
			&refractvals.dist);

    /* a entry/scale/drop-box for each index */
    mw_fscale_entry_new(table, "Index A", 
			0.0, 5.0, 
			1.0, 0.1, 0,
			0,2, 3, 4,
			&refractvals.na);

    mw_fscale_entry_new(table, "Index B", 
			0.0, 5.0, 
			1.0, 0.1, 0,
			0,2, 4, 5,
			&refractvals.nb);

    /* FIXME: Add drop-boxes with common indicies of refraction. */

    /* entry/scale pairs for x and y offsets */

    mw_iscale_entry_new(table, "X Offset", 
			-1000, 1000, 
			1, 20, 0,
			0,2, 5, 6,
			&refractvals.xofs);

    mw_iscale_entry_new(table, "Y Offset", 
			-1000, 1000, 
			1, 20, 0,
			0,2, 6, 7,
			&refractvals.yofs);

    /* radio buttons for wrap/transparent (or bg, if image isn't layered) */

    /*    button = gtk_check_button_new_with_label ("Wrap?");
    toggle_button_callback (button, gpointer   data);
    gtk_toggle_button_set_state (GtkToggleButton button, refractvals.wrap); */

    gtk_widget_show (dlg);

    gtk_main ();
    gdk_flush ();

    return refractint.run;
} /* refract_dialog */

static gint
map_constrain(gint32 image_id, gint32 drawable_id, gpointer data)
{
	if (drawable_id == -1)
		return TRUE;

	return (gimp_drawable_color(drawable_id) || gimp_drawable_gray(drawable_id));
} /* map_constrain */

/* Callbacks */
static void
refract_close_callback (GtkWidget *widget,
                        gpointer   data)
{
  gtk_main_quit ();
}

static void
refract_ok_callback (GtkWidget *widget, gpointer data)
{
    refractint.run = TRUE;
    gtk_widget_destroy (GTK_WIDGET (data));
}

static void
map_menu_callback (gint32 id, gpointer data)
{
    refractvals.lensmap = id;
}

/**********************************************
 * A borrowed bilinear interpolation function.
 */

static guchar
bilinear(double x, double y, guchar *values)
{
	double m0, m1;

	x = fmod(x, 1.0);
	y = fmod(y, 1.0);

	if (x < 0.0)
		x += 1.0;

	if (y < 0.0)
		y += 1.0;

	m0 = (double) values[0] + x * ((double) values[1] - values[0]);
	m1 = (double) values[2] + x * ((double) values[3] - values[2]);

	return (guchar) (m0 + y * (m1 - m0));
} /* bilinear */


/************************************************************************
 *
 *   Fun pixel fetching stuff...  Quartic's code from whirlpinch.c
 *
 */ 

static pixel_fetcher_t *
pixel_fetcher_new(GDrawable *drawable)
{
	pixel_fetcher_t *pf;

	pf = g_malloc(sizeof(pixel_fetcher_t));

	pf->col           = -1;
	pf->row           = -1;
	pf->img_width     = gimp_drawable_width(drawable->id);
	pf->img_height    = gimp_drawable_height(drawable->id);
	pf->img_bpp       = gimp_drawable_bpp(drawable->id);
	pf->img_has_alpha = gimp_drawable_has_alpha(drawable->id);
	pf->tile_width    = gimp_tile_width();
	pf->tile_height   = gimp_tile_height();
	pf->bg_color[0]   = 0;
	pf->bg_color[1]   = 0;
	pf->bg_color[2]   = 0;
	pf->bg_color[3]   = 0;

	pf->drawable    = drawable;
	pf->tile        = NULL;

	return pf;
} /* pixel_fetcher_new */


/*****/

static void
pixel_fetcher_set_bg_color(pixel_fetcher_t *pf, guchar r, guchar g, guchar b, guchar a)
{
	pf->bg_color[0] = r;
	pf->bg_color[1] = g;
	pf->bg_color[2] = b;

	if (pf->img_has_alpha)
		pf->bg_color[pf->img_bpp - 1] = a;
} /* pixel_fetcher_set_bg_color */


/*****/

static void
pixel_fetcher_get_pixel(pixel_fetcher_t *pf, int x, int y, guchar *pixel)
{
	gint    col, row;
	gint    coloff, rowoff;
	guchar *p;
	int     i;

	if ((x < sel_x1) || (x >= sel_x2) ||
	    (y < sel_y1) || (y >= sel_y2)) {
		for (i = 0; i < pf->img_bpp; i++)
			pixel[i] = pf->bg_color[i];

		return;
	} /* if */

	col    = x / pf->tile_width;
	coloff = x % pf->tile_width;
	row    = y / pf->tile_height;
	rowoff = y % pf->tile_height;

	if ((col != pf->col) ||
	    (row != pf->row) ||
	    (pf->tile == NULL)) {
		if (pf->tile != NULL)
			gimp_tile_unref(pf->tile, FALSE);

		pf->tile = gimp_drawable_get_tile(pf->drawable, FALSE, row, col);
		gimp_tile_ref(pf->tile);

		pf->col = col;
		pf->row = row;
	} /* if */

	p = pf->tile->data + pf->img_bpp * (pf->tile->ewidth * rowoff + coloff);

	for (i = pf->img_bpp; i; i--)
		*pixel++ = *p++;
} /* pixel_fetcher_get_pixel */


/*****/

static void
pixel_fetcher_destroy(pixel_fetcher_t *pf)
{
	if (pf->tile != NULL)
		gimp_tile_unref(pf->tile, FALSE);

	g_free(pf);
} /* pixel_fetcher_destroy */
