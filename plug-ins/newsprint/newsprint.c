/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * newsprint plug-in
 * Copyright (C) 1997-1998 Austin Donnelly <austin@greenend.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Portions of this plug-in are copyright 1991-1992 Adobe Systems
 * Incorporated.  See the spot_PS*() functions for details.
 */

/*
 * version 0.01
 * This version requires gtk-1.0.4 or above.
 *
 * This plug-in puts an image through a screen at a particular angle
 * and lines per inch, to arrive at a newspaper-style effect.
 *
 * Austin Donnelly <austin@greenend.org.uk>
 * http://www.cl.cam.ac.uk/~and1000/newsprint/
 *
 * Richard Mortier <rmm1002@cam.ac.uk> did the spot_round() function
 * with correct tonal balance.
 *
 * Tim Harris <tim.harris@acm.org> provided valuable feedback on
 * pre-press issues.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>
#include "libgimp/gimp.h"

#ifdef RCSID
static char rcsid[] = "$Id$";
#endif

#define VERSION "v0.01"

/* Some useful macros */
#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x)
#endif

/* HACK so we compile with old gtks.  Won't work on machines where the
 * size of an int is different from the size of a void*, eg Alphas */
#ifndef GINT_TO_POINTER
#warning glib did not define GINT_TO_POINTER, assuming same size as int
#define GINT_TO_POINTER(x) ((gpointer)(x))
#endif

#ifndef GPOINTER_TO_INT
#warning glib did not define GPOINTER_TO_INT, assuming same size as int
#define GPOINTER_TO_INT(x) ((int)(x))
#endif


#define TIMINGS

#ifdef TIMINGS
#include <sys/time.h>
#include <unistd.h>
#define TM_INIT()	struct timeval tm_start, tm_end
#define TM_START()	gettimeofday(&tm_start, NULL)
#define TM_END()							 \
do {									 \
    gettimeofday(&tm_end, NULL);					 \
    tm_end.tv_sec  -= tm_start.tv_sec;					 \
    tm_end.tv_usec -= tm_start.tv_usec;					 \
    if (tm_end.tv_usec < 0)						 \
    {									 \
	tm_end.tv_usec += 1000000;					 \
	tm_end.tv_sec  -= 1;						 \
    }									 \
    printf("operation took %ld.%06ld\n", tm_end.tv_sec, tm_end.tv_usec); \
} while(0)
#else
#define TM_INIT()
#define TM_START()
#define TM_END()
#endif

#define TILE_CACHE_SIZE 16
#define ENTSCALE_SCALE_WIDTH 125
#define ENTSCALE_ENTRY_WIDTH 40
#define DEF_OVERSAMPLE	1	/* default for how much to oversample by */
#define SPOT_PREVIEW_SZ	16
#define PREVIEW_OVERSAMPLE 3


#define BOUNDS(a,x,y)	((a < x) ? x : ((a > y) ? y : a))
#define ISNEG(x)	(((x) < 0)? 1 : 0)
#define DEG2RAD(d)	((d) * M_PI / 180)
#define VALID_BOOL(x)	((x) == TRUE || (x) == FALSE)
#define CLAMPED_ADD(a, b) (((a)+(b) > 0xff)? 0xff : (a) + (b))

/* Ideally, this would be in a common header file somewhere.  This was
 * nicked from app/convert.c */
#define INTENSITY(r,g,b) (r * 0.30 + g * 0.59 + b * 0.11 + 0.001)

/* Bartlett window supersampling weight function.  See table 4.1, page
 * 123 of Alan Watt and Mark Watt, Advanced Animation and Rendering
 * Techniques, theory and practice. Addison-Wesley, 1992. ISBN
 * 0-201-54412-1 */
#define BARTLETT(x,y)	(((oversample/2)+1-ABS(x)) * ((oversample/2)+1-ABS(y)))
#define WGT(x,y)	wgt[((y+oversample/2)*oversample) + x+oversample/2]

/* colourspaces we can separate to: */
#define CS_GREY		0
#define CS_RGB		1
#define CS_CMYK		2
#define CS_INTENSITY	3
#define NUM_CS		4
#define VALID_CS(x)	((x) >= 0 && (x) <= NUM_CS-1)

/* Spot function related types and definitions */

typedef gdouble spotfn_t (gdouble x, gdouble y);

/* forward declaration of the functions themselves */
static spotfn_t spot_round;
static spotfn_t spot_line;
static spotfn_t spot_diamond;
static spotfn_t spot_PSsquare;
static spotfn_t spot_PSdiamond;

typedef struct {
    const char	*name;		/* function's name */
    spotfn_t	*fn;		/* function itself */
    guchar	*thresh;	/* cached threshold matrix */
    gdouble	prev_lvl[3];	/* intensities to preview */
    guchar	*prev_thresh;	/* preview-sized threshold matrix */
    gint	balanced;	/* TRUE if spot fn is already balanced */
} spot_info_t;


/* This is all the info needed per spot function.  Functions are refered to
 * by their index into this array. */
static spot_info_t spotfn_list[] = {
#define SPOTFN_DOT 0
    {"round",
     spot_round,
     NULL,
     {.22, .50, .90},
     NULL,
     FALSE},

    {"line",
     spot_line,
     NULL,
     {.15, .50, .80},
     NULL,
     FALSE},

    {"diamond",
     spot_diamond,
     NULL,
     {.15, .50, .80},
     NULL,
     TRUE},

    {"PS square (Euclidean dot)",
     spot_PSsquare,
     NULL,
     {.15, .50, .90},
     NULL,
     FALSE},

    {"PS diamond",
     spot_PSdiamond,
     NULL,
     {.15, .50, .90},
     NULL,
     FALSE},

    /* NULL-name terminates */
    {NULL,
     NULL,
     NULL,
     {0.0, 0.0, 0.0},
     NULL,
     FALSE}
};

#define NUM_SPOTFN	((sizeof(spotfn_list) / sizeof(spot_info_t)) - 1)
#define VALID_SPOTFN(x)	((x) >= 0 && (x) < NUM_SPOTFN)
#define THRESH(x,y)	(thresh[(y)*width + (x)])
#define THRESHn(n,x,y)	((thresh[n])[(y)*width + (x)])
    


/* Arguments to filter */

/* Some of these are here merely to save them across calls.  They are
 * marked as "UI use".  Everything else is a valid arg. */
typedef struct {
    /* resolution section: */
    gint	cell_width;

    /* screening section: */
    gint	colourspace;	/* 0: RGB, 1: CMYK, 2: Intensity */
    gint	k_pullout;	/* percentage of black to pull out */

    /* grey screen (only used if greyscale drawable) */
    gdouble	gry_ang;
    gint	gry_spotfn;
    /* red / cyan screen */
    gdouble	red_ang;
    gint	red_spotfn;
    /* green / magenta screen */
    gdouble	grn_ang;
    gint	grn_spotfn;
    /* blue / yellow screen */
    gdouble	blu_ang;
    gint	blu_spotfn;

    /* anti-alias section */
    gint	oversample;	/* 1 == no anti-aliasing, else small odd int */
} NewsprintValues;

/* bits of state used by the UI, but not visible from the PDB */
typedef struct {
    gint	input_spi;	/* input samples per inch */
    gdouble	output_lpi;	/* desired output lines per inch */
    gint	lock_channels;	/* changes to one channel affect all */
} NewsprintUIValues;

typedef struct {
    gint	run;
} NewsprintInterface;


typedef void (*EntscaleCallbackFunc) (gdouble new_val, gpointer data);


typedef enum {
  ENTSCALE_INT,
  ENTSCALE_DOUBLE
} EntscaleType;


typedef struct {
  GtkObject		*adjustment;
  GtkWidget		*entry;
  GtkWidget		*label;
  GtkWidget		*hbox;
  EntscaleType		type;
  gchar			fmt_string[16];
  gint			constraint;
  EntscaleCallbackFunc	callback;
  gpointer		call_data;
} Entscale;




/* state for the preview widgets */
typedef struct {
    GtkWidget	*widget;	/* preview widget itself */
    GtkWidget	*label;		/* the label below it */
} preview_st;

/* state for the channel frames */
typedef struct _channel_st {
    GtkWidget	*frame;		/* frame around this channel */
    gint	*spotfn_num;	/* which spotfn the menu is controlling */
    preview_st	prev[3];	/* state for 3 preview widgets */
    Entscale	*entscale;	/* angle entscale widget */
    GtkWidget	*option_menu;	/* popup for spot function */
    GtkWidget	*menuitem[NUM_SPOTFN];	/* menuitems for each spot function */
    struct _channel_st *next;	/* circular list of channels in locked group */
} channel_st;


/* State associated with the configuration dialog and passed to its
 * callback functions */
typedef struct {
    GtkWidget	*dlg;	/* main dialog itself */
    Entscale	*pull;	/* black pullout percentage */
    GtkWidget	*vbox;	/* container for screen info */    
    /* room for up to 4 channels per colourspace */
    channel_st	*chst[NUM_CS][4];
} NewsprintDialog_st;




/***** Local vars *****/


/* defaults */
/* fixed copy so we can reset them */
static const NewsprintValues factory_defaults =
{
    /* resolution stuff */
    10,		/* cell width */

    /* screen setup (default is the classic rosette pattern) */
    CS_RGB,	/* use RGB, not CMYK or Intensity */
    100,	/* max pullout */
    /* grey/black */
    45.0,
    SPOTFN_DOT,
    /* red/cyan */
    15.0,
    SPOTFN_DOT,
    /* green/magenta */
    75.0,
    SPOTFN_DOT,
    /* blue/yellow */
    0.0,
    SPOTFN_DOT,

    /* anti-alias control */
    DEF_OVERSAMPLE
};

static const NewsprintUIValues factory_defaults_ui = {
    75,		/* input spi */
     7.5,	/* output lpi */
    FALSE	/* lock channels */
};

/* Mutable copy for normal use.  Initialised in run(). */
static NewsprintValues pvals;
static NewsprintUIValues pvals_ui;

static NewsprintInterface pint =
{
  FALSE	    /* run */
};


/* channel templates */
typedef struct {
    const gchar	*name;
    /* pointers to the variables this channel updates */
    gdouble	*angle;
    gint	*spotfn;
    /* factory defaults for the angle and spot function (as pointers so
     * the silly compiler can see they're really constants) */
    const gdouble *factory_angle;
    const gint	*factory_spotfn;
} chan_tmpl;

static const chan_tmpl grey_tmpl[] = {
    {"Grey",
     &pvals.gry_ang,
     &pvals.gry_spotfn,
     &factory_defaults.gry_ang,
     &factory_defaults.gry_spotfn},

    {NULL, NULL, NULL, NULL, NULL}
};

static const chan_tmpl rgb_tmpl[] = {
    {"Red",
     &pvals.red_ang,
     &pvals.red_spotfn,
     &factory_defaults.red_ang,
     &factory_defaults.red_spotfn},

    {"Green",
     &pvals.grn_ang,
     &pvals.grn_spotfn,
     &factory_defaults.grn_ang,
     &factory_defaults.grn_spotfn},

    {"Blue",
     &pvals.blu_ang,
     &pvals.blu_spotfn,
     &factory_defaults.blu_ang,
     &factory_defaults.blu_spotfn},

    {NULL, NULL, NULL, NULL, NULL}
};

static const chan_tmpl cmyk_tmpl[] = {
    {"Cyan",
     &pvals.red_ang,
     &pvals.red_spotfn,
     &factory_defaults.red_ang,
     &factory_defaults.red_spotfn},


    {"Magenta",
     &pvals.grn_ang,
     &pvals.grn_spotfn,
     &factory_defaults.grn_ang,
     &factory_defaults.grn_spotfn},

    {"Yellow",
     &pvals.blu_ang,
     &pvals.blu_spotfn,
     &factory_defaults.blu_ang,
     &factory_defaults.blu_spotfn},

    {"Black",
     &pvals.gry_ang,
     &pvals.gry_spotfn,
     &factory_defaults.gry_ang,
     &factory_defaults.gry_spotfn},

    {NULL, NULL, NULL, NULL, NULL}
};

static const chan_tmpl intensity_tmpl[] = {
    {"Intensity",
     &pvals.gry_ang,
     &pvals.gry_spotfn,
     &factory_defaults.gry_ang,
     &factory_defaults.gry_spotfn},

    {NULL, NULL, NULL, NULL, NULL}
};

/* cspace_chan_tmpl is indexed by colourspace, and gives an array of
 * channel templates for that colourspace */
static const chan_tmpl *cspace_chan_tmpl[] = {
    grey_tmpl,
    rgb_tmpl,
    cmyk_tmpl,
    intensity_tmpl
};

#define NCHANS(x) ((sizeof(x) / sizeof(chan_tmpl)) - 1)

/* cspace_nchans gives a quick way of finding the number of channels
 * in a colourspace.  Alternatively, if you're walking the channel
 * template, you can use the NULL entry at the end to stop. */
static const gint cspace_nchans[] = {
    NCHANS(grey_tmpl),
    NCHANS(rgb_tmpl),
    NCHANS(cmyk_tmpl),
    NCHANS(intensity_tmpl)
};


/* Declare local functions.  */
static void	query	(void);
static void	run	(gchar	 *name,
			 gint	 nparams,
			 GParam	 *param,
			 gint	 *nreturn_vals,
			 GParam	 **return_vals);

static gint	newsprint_dialog (GDrawable *drawable);
static void	newsprint_close_callback	 (GtkWidget *widget,
					  gpointer   data);
static void	newsprint_ok_callback	 (GtkWidget *widget,
					  gpointer   data);

static void	newsprint_toggle_update (GtkWidget *widget,
					 gpointer   data);

static void	newsprint_cspace_update (GtkWidget *widget,
					 gpointer   data);

static void	newsprint	(GDrawable *drawable);

static guchar *	spot2thresh	(gint type, gint width);

static Entscale * entscale_new (GtkWidget *table, gint x, gint y,
			  gchar *caption, EntscaleType type, gpointer variable,
			  gdouble min, gdouble max, gdouble step,
			  gint constraint, EntscaleCallbackFunc callback,
			  gpointer call_data);


static int entscale_get_precision (gdouble step);
static void entscale_destroy_callback (GtkWidget *widget, gpointer data);
static void entscale_scale_update (GtkAdjustment *adjustment, gpointer data);
static void entscale_entry_update (GtkWidget *widget, gpointer data);

static void entscale_set_sensitive (Entscale *ent, gint sensitive);
static void entscale_set_value(Entscale *ent, gfloat value);



GPlugInInfo PLUG_IN_INFO =
{
  NULL,	   /* init_proc */
  NULL,	   /* quit_proc */
  query,   /* query_proc */
  run	   /* run_proc */
};



/***** Functions *****/

MAIN ()

static void
query()
{
  static GParamDef args[]=
    {
      { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
      { PARAM_IMAGE, "image", "Input image (unused)" },
      { PARAM_DRAWABLE, "drawable", "Input drawable" },

      { PARAM_INT32, "cell_width", "screen cell width, in pixels" },

      { PARAM_INT32, "colourspace", "separate to 0:RGB, 1:CMYK, 2:Intensity" },
      { PARAM_INT32, "k_pullout", "Percentage of black to pullout (CMYK only)" },

      { PARAM_FLOAT, "gry_ang", "Grey/black screen angle (degrees)" },
      { PARAM_INT32, "gry_spotfn", "Grey/black spot function (0=dots, 1=lines, 2=diamonds, 3=euclidean dot, 4=PS diamond)" },
      { PARAM_FLOAT, "red_ang", "Red/cyan screen angle (degrees)" },
      { PARAM_INT32, "red_spotfn", "Red/cyan spot function (values as gry_spotfn)" },
      { PARAM_FLOAT, "grn_ang", "Green/magenta screen angle (degrees)" },
      { PARAM_INT32, "grn_spotfn", "Green/magenta spot function (values as gry_spotfn)" },
      { PARAM_FLOAT, "blu_ang", "Blue/yellow screen angle (degrees)" },
      { PARAM_INT32, "blu_spotfn", "Blue/yellow spot function (values as gry_spotfn)" },
      
      { PARAM_INT32, "oversample", "how many times to oversample spot fn" }
      /* 15 args */
   };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_newsprint",
			  "Re-sample the image to give a newspaper-like "
			  "effect",
			  "Halftone the image, trading off "
			  "resolution to represent colours or grey levels "
			  "using the process described both in the PostScript "
			  "language definition, and also by Robert Ulichney, "
			  "\"Digital halftoning\", MIT Press, 1987.",
			  "Austin Donnelly",
			  "Austin Donnelly",
			  "1998 (" VERSION ")",
			  "<Image>/Filters/Misc/Newsprint",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (gchar   *name,
     gint    nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam  **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  /* basic defaults */
  pvals    = factory_defaults;
  pvals_ui = factory_defaults_ui;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
  {
  case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_newsprint", &pvals);
      gimp_get_data ("plug_in_newsprint_ui", &pvals_ui);

      /*  First acquire information with a dialog  */
      if (! newsprint_dialog (drawable))
      {
	  gimp_drawable_detach (drawable);
	  return;
      }
      break;

    case RUN_NONINTERACTIVE:
	/*  Make sure all the arguments are there!  */
	if (nparams != 15)
	{
	    status = STATUS_CALLING_ERROR;
	    break;
	}

	pvals.cell_width = param[3].data.d_int32;
	pvals.colourspace = param[4].data.d_int32;
	pvals.k_pullout  = param[5].data.d_int32;
	pvals.gry_ang	 = param[6].data.d_float;
	pvals.gry_spotfn = param[7].data.d_int32;
	pvals.red_ang    = param[8].data.d_float;
	pvals.red_spotfn = param[9].data.d_int32;
	pvals.grn_ang    = param[10].data.d_float;
	pvals.grn_spotfn = param[11].data.d_int32;
	pvals.blu_ang    = param[12].data.d_float;
	pvals.blu_spotfn = param[13].data.d_int32;
	pvals.oversample = param[14].data.d_int32;

	/* check values are within permitted ranges */
	if (!VALID_SPOTFN(pvals.gry_spotfn) ||
	    !VALID_SPOTFN(pvals.red_spotfn) ||
	    !VALID_SPOTFN(pvals.grn_spotfn) ||
	    !VALID_SPOTFN(pvals.blu_spotfn) ||
	    !VALID_CS(pvals.colourspace) ||
	    pvals.k_pullout < 0 || pvals.k_pullout > 100)
	    status = STATUS_CALLING_ERROR;

	break;

  case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_newsprint", &pvals);
      break;

  default:
      break;
  }

  if (status == STATUS_SUCCESS)
  {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_color (drawable->id) ||
	  gimp_drawable_gray (drawable->id))
      {
	  gimp_progress_init("Newsprintifing...");

	  /*  set the tile cache size  */
	  gimp_tile_cache_ntiles(TILE_CACHE_SIZE);

	  /*  run the newsprint effect  */
	  newsprint(drawable);

	  if (run_mode != RUN_NONINTERACTIVE)
	      gimp_displays_flush();

	  /*  Store data  */
	  if (run_mode == RUN_INTERACTIVE)
	  {
	      gimp_set_data ("plug_in_newsprint",
			     &pvals, sizeof (NewsprintValues));
	      gimp_set_data ("plug_in_newsprint_ui",
			     &pvals_ui, sizeof (NewsprintUIValues));
	  }
      }
      else
      {
	  /*gimp_message ("newsprint: cannot operate on indexed images");*/
	  status = STATUS_EXECUTION_ERROR;
      }
  }

  values[0].data.d_status = status;
  
  gimp_drawable_detach(drawable);
}




/* create new menu state, and the preview widgets for it */
static channel_st *
new_preview(gint *sfn)
{
    channel_st	*st;
    GtkWidget	*preview;
    GtkWidget	*label;
    gint	i;

    st = g_new(channel_st, 1);

    st->spotfn_num = sfn;

    /* make the preview widgets */
    for(i=0; i < 3; i++)
    {
	preview = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_size(GTK_PREVIEW(preview),
			 SPOT_PREVIEW_SZ*2 + 1, SPOT_PREVIEW_SZ*2 + 1);
	gtk_widget_show(preview);

	label = gtk_label_new("");
	gtk_widget_show(label);
	
	st->prev[i].widget = preview;
	st->prev[i].label  = label;
	/* st->prev[i].value changed by preview_update */
    }

    return st;
}


/* the popup menu "st" has changed, so the previews associated with it 
 * need re-calculation */
static void
preview_update(channel_st *st)
{
    gint	sfn = *(st->spotfn_num);
    preview_st	*prev;
    gint	i;
    gint	x;
    gint	y;
    gint	width = SPOT_PREVIEW_SZ * PREVIEW_OVERSAMPLE;
    gint	oversample = PREVIEW_OVERSAMPLE;
    guchar	*thresh;
    gchar	pct[12];
    guchar	value;
    guchar	row[3 * (2 * SPOT_PREVIEW_SZ + 1)];

    /* If we don't yet have a preview threshold matrix for this spot
     * function, generate one now. */
    if (!spotfn_list[sfn].prev_thresh)
    {
	spotfn_list[sfn].prev_thresh =
	    spot2thresh(sfn, SPOT_PREVIEW_SZ*PREVIEW_OVERSAMPLE);
    }


    thresh = spotfn_list[sfn].prev_thresh;

    for(i=0; i < 3; i++)
    {
	prev = &st->prev[i];

	value = spotfn_list[sfn].prev_lvl[i] * 0xff;

	for (y=0; y <= SPOT_PREVIEW_SZ*2; y++)
	{
	    for (x=0; x <= SPOT_PREVIEW_SZ*2; x++)
	    {
		guint32 sum = 0;
		gint sx, sy;
		gint tx, ty;
		gint rx, ry;

		rx = x * PREVIEW_OVERSAMPLE;
		ry = y * PREVIEW_OVERSAMPLE;

		for(sy=-oversample/2; sy<=oversample/2; sy++)
		    for(sx=-oversample/2; sx<=oversample/2; sx++)
		    {
			tx = rx+sx;
			ty = ry+sy;
			while(tx < 0)  tx += width;
			while(ty < 0)  ty += width;
			while(tx >= width)  tx -= width;
			while(ty >= width)  ty -= width;
			
			if (value > THRESH(tx, ty))
			    sum += 0xff * BARTLETT(sx, sy);
		    }
		sum /= BARTLETT(0,0) * BARTLETT(0,0);

		/* blue lines on cell boundaries */
		if ((x % SPOT_PREVIEW_SZ) == 0 ||
		    (y % SPOT_PREVIEW_SZ) == 0)
		{
		    row[x*3+0] = 0;
		    row[x*3+1] = 0;
		    row[x*3+2] = 0xff;
		}
		else
		{
		    row[x*3+0] = sum;
		    row[x*3+1] = sum;
		    row[x*3+2] = sum;
		}
	    }

	    gtk_preview_draw_row(GTK_PREVIEW(prev->widget),
				 row, 0, y, SPOT_PREVIEW_SZ*2+1);
	}

	/* redraw preview widget */
	gtk_widget_draw(prev->widget, NULL);

	sprintf(pct, "%2d%%", (int)rint(spotfn_list[sfn].prev_lvl[i] * 100));
	gtk_label_set(GTK_LABEL(prev->label), pct);
    }

    gdk_flush ();
}

    
static void
newsprint_menu_callback(GtkWidget *widget,
			gpointer   data)
{
    channel_st *st = data;
    gpointer *ud;
    gint menufn;
    static gint in_progress = FALSE;

    /* we shouldn't need recursion protection, but if lock_channels is
     * set, and gtk_option_menu_set_history ever generates an
     * "activated" signmal, then we'll get back here.  So we've defensive. */
    if (in_progress)
    {
	printf("newsprint_menu_callback: unexpected recursion: "
	       "can't happen\n");
	return;
    }	

    in_progress = TRUE;

    ud = gtk_object_get_user_data(GTK_OBJECT(widget));
    menufn = (gint)ud;

    *(st->spotfn_num) = menufn;

    preview_update(st);

    /* ripple the change to the other popups if the channels are
     * locked together. */
    if (pvals_ui.lock_channels)
    {
	channel_st *c = st->next;
	gint	oldfn;

	while(c != st)
	{
	    gtk_option_menu_set_history(GTK_OPTION_MENU(c->option_menu),
					menufn);
	    oldfn = *(c->spotfn_num);
	    *(c->spotfn_num) = menufn;
	    if (oldfn != menufn)
		preview_update(c);
	    c = c->next;
	}
    }

    in_progress = FALSE;
}


static void
angle_callback(gdouble new_val, gpointer data)
{
    channel_st *st = data;
    channel_st *c;
    static gint in_progress = FALSE;

    if (pvals_ui.lock_channels && !in_progress)
    {
	in_progress = TRUE;

	c = st->next;

	while(c != st)
	{
	    entscale_set_value(c->entscale, new_val);
	    c = c->next;
	}

	in_progress = FALSE;
    }
}



static void
newsprint_defaults_callback(GtkWidget *widget,
			    gpointer   data)
{
    NewsprintDialog_st *st = data;
    gint	saved_lock;
    gint	cspace;
    channel_st	**chst;
    const chan_tmpl *ct;
    gint	spotfn;
    gint	c;

    /* temporarily turn off channel lock */
    saved_lock = pvals_ui.lock_channels;
    pvals_ui.lock_channels = FALSE;

    /* for each colourspace, reset its channel info */
    for(cspace=0; cspace < NUM_CS; cspace++)
    {
	chst = st->chst[cspace];
	ct = cspace_chan_tmpl[cspace];

	/* skip this colourspace if we haven't used it yet */
	if (!chst[0])
	    continue;

	c = 0;
	while(ct->name)
	{
	    entscale_set_value(chst[c]->entscale, *(ct->factory_angle));

	    /* change the popup menu and also activate the menuitem in
	     * question, in order to run the handler that re-computes
	     * the preview area */
	    spotfn = *(ct->factory_spotfn);
	    gtk_option_menu_set_history(GTK_OPTION_MENU(chst[c]->option_menu),
					spotfn);
	    gtk_menu_item_activate(GTK_MENU_ITEM(chst[c]->menuitem[spotfn]));

	    c++;
	    ct++;
	}
    }

    pvals_ui.lock_channels = saved_lock;
}

/* Create (but don't yet show) a new frame for a channel 'widget'.
 * Return the channel state, so caller can find the frame and place it
 * in a container. */
static channel_st *
new_channel(const chan_tmpl *ct)
{
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *menu;
    spot_info_t *sf;
    channel_st *chst;
    gint      i;    

    /* create the channel state record */
    chst = new_preview(ct->spotfn);

    chst->frame = gtk_frame_new (ct->name);
    gtk_frame_set_shadow_type (GTK_FRAME (chst->frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_border_width (GTK_CONTAINER (chst->frame), 4);

    table = gtk_table_new (3, 2, FALSE);
    gtk_container_border_width (GTK_CONTAINER (table), 4);
    gtk_container_add (GTK_CONTAINER (chst->frame), table);

    /* angle slider */
    chst->entscale = entscale_new(table, 0, 0, "angle",
				  ENTSCALE_DOUBLE, ct->angle,
				  -90.0, 90.0, 1.0, TRUE/*constrain*/,
				  angle_callback, chst);

    /* spot function popup */
    label = gtk_label_new("spot function");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
		     GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(label);

    chst->option_menu = gtk_option_menu_new();
    gtk_table_attach(GTK_TABLE(table), chst->option_menu, 0, 1, 2, 3,
		     GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_widget_show(chst->option_menu);

    menu = gtk_menu_new();

    sf = spotfn_list;
    i = 0;
    while(sf->name)
    {
	chst->menuitem[i] = gtk_menu_item_new_with_label(sf->name);
	gtk_signal_connect(GTK_OBJECT(chst->menuitem[i]), "activate",
			   (GtkSignalFunc) newsprint_menu_callback,
			   chst);
	gtk_object_set_user_data(GTK_OBJECT(chst->menuitem[i]), (gpointer*)i);
	gtk_widget_show(chst->menuitem[i]);
	gtk_menu_append(GTK_MENU(menu), GTK_WIDGET(chst->menuitem[i]));
	sf++;
	i++;
    }

    gtk_menu_set_active(GTK_MENU(menu), *ct->spotfn);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(chst->option_menu), menu);
    gtk_widget_show(chst->option_menu);

    /* spot function previews go in table slots 1-2, 1-3 (double
     * height) */
    {
	GtkWidget *sub;
	GtkWidget *align;

	sub = gtk_table_new(2, 3, FALSE);
	gtk_container_border_width(GTK_CONTAINER(sub), 1);

	align = gtk_alignment_new(0.5, 1.0, 1.0, 0.0);
	gtk_container_add(GTK_CONTAINER(align), sub);
	gtk_widget_show(align);

	gtk_table_attach(GTK_TABLE(table), align, 1, 2, 1, 3,
			 GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	for(i=0; i < 3; i++)
	{
	    gtk_table_attach(GTK_TABLE(sub), chst->prev[i].widget,
			     i, i+1, 0, 1,
			     GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	    gtk_table_attach(GTK_TABLE(sub), chst->prev[i].label,
			     i, i+1, 1, 2,
			     GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	}

	gtk_widget_show(sub);
    }

    /* fire the update once to make sure we start with something
     * in the preview windows */
    preview_update(chst);

    gtk_widget_show(table);

    /* deliberately don't show the chst->frame, leave that up to
     * the caller */

    return chst;
}


/* Make all the channels needed for "colourspace", and fill in
 * the respective channel state fields in "st". */
static void
gen_channels(NewsprintDialog_st *st, gint colourspace)
{
    const chan_tmpl *ct;
    channel_st	**chst;
    channel_st	*base = NULL;
    gint	i;

    chst = st->chst[colourspace];
    ct   = cspace_chan_tmpl[colourspace];
    i    = 0;

    while(ct->name)
    {
	chst[i] = new_channel(ct);

	if (i)
	    chst[i-1]->next = chst[i];
	else
	    base = chst[i];

	gtk_box_pack_start(GTK_BOX(st->vbox), chst[i]->frame,
			   TRUE, FALSE, 0);
	gtk_box_reorder_child(GTK_BOX(st->vbox), chst[i]->frame, 5+i);

	i++;
	ct++;
    }

    /* make the list circular */
    chst[i-1]->next = base;
}


static gint
newsprint_dialog (GDrawable *drawable)
{
    gchar	**argv;
    gint	argc;
    /* widgets we need from callbacks stored here */
    NewsprintDialog_st	st;
    GtkWidget	*frame;
    GtkWidget	*table;
    GtkWidget	*align;
    GtkWidget	*button;
    GtkWidget	*hbox;
    GtkWidget	*toggle;
    GtkWidget	*label;
    GSList	*group = NULL;
    gint	bpp;
    guchar	*color_cube;
    gint	i;

    argc = 1;
    argv = g_new(gchar *, 1);
    argv[0] = g_strdup("newsprint");

    gtk_init (&argc, &argv);
    gtk_rc_parse(gimp_gtkrc());

    gtk_preview_set_gamma (gimp_gamma ());
    gtk_preview_set_install_cmap (gimp_install_cmap ());
    color_cube = gimp_color_cube ();
    gtk_preview_set_color_cube (color_cube[0], color_cube[1],
				color_cube[2], color_cube[3]);

    gtk_widget_set_default_visual (gtk_preview_get_visual ());
    gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

#if 0
    printf("newsprint: waiting... (pid %d)\n", getpid());
    kill(getpid(), 19);
#endif

    /* flag values to say we haven't filled these channel
     * states in yet */
    for(i=0; i<NUM_CS; i++)
	st.chst[i][0] = NULL;

    /* need to know the bpp, so we can tell if we're doing 
     * RGB/CMYK or grey style of dialog box */
    bpp = gimp_drawable_bpp(drawable->id);
    if (gimp_drawable_has_alpha(drawable->id))
	bpp--;	

    /* force greyscale if it's the only thing we can do */
    if (bpp == 1) {
	pvals.colourspace = CS_GREY;
    } else {
	if (pvals.colourspace == CS_GREY)
	    pvals.colourspace = CS_RGB;
    }

    st.dlg = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (st.dlg), "Newsprint");
    gtk_window_position (GTK_WINDOW (st.dlg), GTK_WIN_POS_MOUSE);
    gtk_signal_connect (GTK_OBJECT (st.dlg), "destroy",
			(GtkSignalFunc) newsprint_close_callback,
			NULL);

    /*  Action area  */
    button = gtk_button_new_with_label ("OK");
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			(GtkSignalFunc) newsprint_ok_callback,
			st.dlg);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (st.dlg)->action_area), button,
			TRUE, TRUE, 0);
    gtk_widget_grab_default (button);
    gtk_widget_show (button);

    button = gtk_button_new_with_label ("Cancel");
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       (GtkSignalFunc) gtk_widget_destroy,
			       GTK_OBJECT (st.dlg));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (st.dlg)->action_area), button,
			TRUE, TRUE, 0);
    gtk_widget_show (button);

    /* resolution settings  */
    frame = gtk_frame_new ("Resolution");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_border_width (GTK_CONTAINER (frame), 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (st.dlg)->vbox), frame,
			TRUE, TRUE, 0);

    table = gtk_table_new (3, 2, FALSE);
    gtk_container_border_width (GTK_CONTAINER (table), 10);
    gtk_container_add (GTK_CONTAINER (frame), table);

    entscale_new(table, 0, 0, "Input SPI ",
		 ENTSCALE_INT, &pvals_ui.input_spi,
		 1.0, 1200.0, 5.0, FALSE/*constrain*/,
		 NULL, NULL);
  
    entscale_new(table, 0, 1, "Output LPI ",
		 ENTSCALE_DOUBLE, &pvals_ui.output_lpi,
		 1.0, 1200.0, 5.0, FALSE/*constrain*/,
		 NULL, NULL);

    entscale_new(table, 0, 2, "Cell size ",
		 ENTSCALE_INT, &pvals.cell_width,
		 3.0, 100.0, 1.0, FALSE/*constrain*/,
		 NULL, NULL);
    /* XXX still need interlocks between these. Maybe use callback fns? */

    gtk_widget_show(table);
    gtk_widget_show(frame);


    /* screen settings */
    frame = gtk_frame_new ("Screen");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_border_width (GTK_CONTAINER (frame), 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (st.dlg)->vbox), frame,
			TRUE, TRUE, 0);

    st.vbox = gtk_vbox_new(FALSE, 1);
    gtk_container_add (GTK_CONTAINER (frame), st.vbox);

    /* optional portion begins */
    if (bpp != 1)
    {
	table = gtk_table_new (2, 2, FALSE);
	gtk_container_border_width (GTK_CONTAINER (table), 10);
	gtk_box_pack_start (GTK_BOX (st.vbox), table, TRUE, TRUE, 0);

	/* black pullout */
	st.pull = entscale_new(table, 0, 1, "Black pullout (%)",
			       ENTSCALE_INT, &pvals.k_pullout,
			       0.0, 100.0, 1.0, TRUE/*constrain*/,
			       NULL, NULL);
	entscale_set_sensitive(st.pull, (pvals.colourspace == CS_CMYK));

	/* RGB / CMYK / Intensity select */
	label = gtk_label_new("Separate to");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show(label);

	hbox = gtk_hbox_new(FALSE, 5);

	toggle = gtk_radio_button_new_with_label(group, "RGB");
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
	gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
	gtk_object_set_user_data(GTK_OBJECT(toggle), &st);
	gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			    (GtkSignalFunc) newsprint_cspace_update,
			    GINT_TO_POINTER(CS_RGB));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
				     (pvals.colourspace == CS_RGB));
	gtk_widget_show (toggle);

	toggle = gtk_radio_button_new_with_label (group, "CMYK");
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
	gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
	gtk_object_set_user_data(GTK_OBJECT(toggle), &st);
	gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			    (GtkSignalFunc) newsprint_cspace_update,
			    GINT_TO_POINTER(CS_CMYK));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
				     (pvals.colourspace == CS_CMYK));
	gtk_widget_show (toggle);

	toggle = gtk_radio_button_new_with_label (group, "Intensity");
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
	gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
	gtk_object_set_user_data(GTK_OBJECT(toggle), &st);
	gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			    (GtkSignalFunc) newsprint_cspace_update,
			    GINT_TO_POINTER(CS_INTENSITY));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
				     (pvals.colourspace == CS_INTENSITY));
	gtk_widget_show (toggle);

	gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 0, 1,
			 GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show(hbox);

	gtk_widget_show(table);

	/* channel lock & factory defaults button */
	align = gtk_alignment_new(0.0, 1.0, 0.1, 1.0);
	hbox = gtk_hbox_new(TRUE, 10);
	gtk_container_add(GTK_CONTAINER(align), hbox);
	gtk_box_pack_start(GTK_BOX(st.vbox), align, TRUE, FALSE, 0);
	/* make sure it went in the right place, since colourspace
	 * radio button callbacks may have already inserted the
	 * channel frames */
	gtk_box_reorder_child(GTK_BOX(st.vbox), align, 1);
	gtk_widget_show(align);
	gtk_widget_show(hbox);

	toggle = gtk_check_button_new_with_label("Lock channels");
	gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
			   (GtkSignalFunc) newsprint_toggle_update,
			   &pvals_ui.lock_channels);
	gtk_object_set_user_data(GTK_OBJECT(toggle), NULL);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON (toggle),
				    pvals_ui.lock_channels);
	gtk_box_pack_start(GTK_BOX(hbox), toggle, TRUE, TRUE, 0);
	gtk_widget_show(toggle);

	button = gtk_button_new_with_label("Factory defaults");
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc) newsprint_defaults_callback,
			   &st);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);
    }

    /* Make the channels appropriate for this colourspace and
     * currently selected defaults.  They may have already been
     * created as a result of callbacks to cspace_update from
     * gtk_toggle_button_set_state().  Other channel frames are
     * created lazily the first time they are required. */
    if (!st.chst[pvals.colourspace][0])
    {
	channel_st **chst;

	gen_channels(&st, pvals.colourspace);

	chst = st.chst[pvals.colourspace];

	for(i=0; i < cspace_nchans[pvals.colourspace]; i++)
	    gtk_widget_show(GTK_WIDGET(chst[i]->frame));
    }

    gtk_widget_show(st.vbox);
    gtk_widget_show(frame);



    /* anti-alias control */
    frame = gtk_frame_new ("Anti-alias");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_border_width (GTK_CONTAINER (frame), 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (st.dlg)->vbox), frame,
			TRUE, TRUE, 0);
    
    table = gtk_table_new (1, 2, FALSE);
    gtk_container_border_width (GTK_CONTAINER (table), 10);
    gtk_container_add (GTK_CONTAINER (frame), table);

    entscale_new(table, 1, 0, "oversample ",
		 ENTSCALE_INT, &pvals.oversample,
		 1.0, 15.0, 1.0, TRUE/*constrain*/,
		 NULL, NULL);

    gtk_widget_show (table);
    gtk_widget_show (frame);

    gtk_widget_show (st.dlg);

    gtk_main ();
    gdk_flush ();

    return pint.run;
}

/*  Newsprint interface functions  */

static void
newsprint_close_callback (GtkWidget *widget,
			 gpointer   data)
{
    gtk_main_quit ();
}

static void
newsprint_ok_callback (GtkWidget *widget,
		      gpointer	 data)
{
    pint.run = TRUE;
    gtk_widget_destroy (GTK_WIDGET (data));
}

static void
newsprint_toggle_update (GtkWidget *widget,
			 gpointer   data)
{
    int *toggle_val;

    toggle_val = (int *) data;

    *toggle_val = GTK_TOGGLE_BUTTON(widget)->active;
}


static void
newsprint_cspace_update (GtkWidget *widget,
			 gpointer   data)
{
    NewsprintDialog_st	*st;
    gint		new_cs = GPOINTER_TO_INT(data);
    gint		old_cs = pvals.colourspace;
    channel_st		**chst;
    gint		i;

    st = gtk_object_get_user_data(GTK_OBJECT(widget));

    if (!st)
	printf("newsprint: cspace_update: no state, can't happen!\n");

    if (st)
    {
	/* the CMYK widget looks after the black pullout widget */
	if (new_cs == CS_CMYK)
	{
	    entscale_set_sensitive(st->pull,
				   GTK_TOGGLE_BUTTON(widget)->active);
	}

	/* if we're not activate, then there's nothing more to do */
	if (!GTK_TOGGLE_BUTTON(widget)->active)
	    return;

	pvals.colourspace = new_cs;

	/* make sure we have the necessary channels for the new
         * colourspace */
	if (!st->chst[new_cs][0])
	    gen_channels(st, new_cs);

	/* hide the old channels (if any) */
	if (st->chst[old_cs][0])
	{
	    chst = st->chst[old_cs];

	    for(i=0; i < cspace_nchans[old_cs]; i++)
		gtk_widget_hide(GTK_WIDGET(chst[i]->frame));
	}

	/* show the new channels */
	chst = st->chst[new_cs];
	for(i=0; i < cspace_nchans[new_cs]; i++)
	    gtk_widget_show(GTK_WIDGET(chst[i]->frame));
    }
}




/*
 * Newsprint Effect
 */


/*************************************************************/
/* Spot functions */


/* Spot functions define the order in which pixels should be whitened
 * as a cell lightened in colour.  They are defined over the entire
 * cell, and are called over each pixel in the cell.  The cell
 * co-ordinate space ranges from -1.0 .. +1.0 inclusive, in both x- and
 * y-axes.
 *
 * This means the spot function f(x, y) must be defined for:
 *     -1 <= x <= +1, where x is a real number,   and
 *     -1 <= y <= +1, where y is a real number.
 *
 * The function f's range is -1.0 .. +1.0 inclusive, but it is
 * permissible for f to return values outside this range: the nearest
 * valid value will be used instead.  NOTE: this is in contrast with
 * PostScript spot functions, where it is a RangeError for the spot
 * function to go outside these limits.
 *
 * An initially black cell is filled from lowest spot function value
 * to highest.  The actual values returned do not matter - it is their
 * relative orderings that count.  This means that spot functions do
 * not need to be tonally balanced.  A tonally balanced spot function
 * is one which for all slices though the function (eg say at z), the
 * area of the slice = 4z.  In particular, almost all PostScript spot
 * functions are _not_ tonally balanced.
 */


/* The classic growing dot spot function.  This one isn't tonally
 * balanced.  It can be made so, but it's _really_ ugly.  Thanks to
 * Richard Mortier for this one:
 *
 * #define a(r) \
 *     ((r<=1)? M_PI * (r*r) : \
 *	4 * sqrt(r*r - 1)  +  M_PI*r*r  -  4*r*r*acos(1/r))
 *
 *   radius = sqrt(x*x + y*y);
 *
 * return a(radius) / 4; */
static gdouble
spot_round(gdouble x, gdouble y)
{
    return 1 - (x*x + y*y);
}


/* Another commonly seen spot function is the v-shaped wedge. Tonally
 * balanced. */
static gdouble
spot_line(gdouble x, gdouble y)
{
    return ABS(y);
}


/* Square/Diamond dot that never becomes round.  Tonally balanced. */
static gdouble
spot_diamond(gdouble x, gdouble y)
{
    gdouble xy = ABS(x) + ABS(y);
    /* spot only valid for 0 <= xy <= 2 */
    return ((xy <= 1)? 2*xy*xy : 2*xy*xy - 4*(xy-1)*(xy-1)) / 4;
}



/* The following functions were derived from a peice of PostScript by
 * Peter Fink and published in his book, "PostScript Screening: Adobe
 * Accurate Screens" (Adobe Press, 1992).  Adobe Systems Incorporated
 * allow its use, provided the following copyright message is present:
 *
 *  % Film Test Pages for Screenset Development
 *  % Copyright (c) 1991 and 1992 Adobe Systems Incorporated
 *  % All rights reserved.
 *  %
 *  % NOTICE: This code is copyrighted by Adobe Systems Incorporated, and
 *  % may not be reproduced for sale except by permission of Adobe Systems
 *  % Incorporated. Adobe Systems Incorporated grants permission to use
 *  % this code for the development of screen sets for use with Adobe
 *  % Accurate Screens software, as long as the copyright notice remains
 *  % intact.
 *  %
 *  % By Peter Fink 1991/1992
 */

/* Square (or Euclidean) dot.  Also very common. */
static gdouble
spot_PSsquare(gdouble x, gdouble y)
{
    gdouble ax = ABS(x);
    gdouble ay = ABS(y);

    return (ax+ay)>1? ((ay-1)*(ay-1) + (ax-1)*(ax-1)) - 1 : 1-(ay*ay + ax*ax);
}

/* Diamond spot function, again from Peter Fink's PostScript
 * original.  Copyright as for previous function. */
static gdouble
spot_PSdiamond(gdouble x, gdouble y)
{
    gdouble ax = ABS(x);
    gdouble ay = ABS(y);

    return (ax+ay)<=0.75? 1-(ax*ax + ay*ay) :	/* dot */
	( (ax+ay)<=1.23?  1-((ay*0.76) + ax) :	/* to diamond (0.76 distort) */
	  ((ay-1)*(ay-1) + (ax-1)*(ax-1)) -1);	/* back to dot */
}

/* end of Adobe Systems Incorporated copyrighted functions */





/*************************************************************/
/* Spot function to threshold matrix conversion */


/* Each call of the spot function results in one of these */
typedef struct {
    gint	index;	/* (y * width) + x */
    gdouble	value;	/* return value of the spot function */
} order_t;

/* qsort(3) compare function */
static int
order_cmp(const void *va, const void *vb)
{
    const order_t *a = va;
    const order_t *b = vb;

    return (a->value < b->value)? -1 : ((a->value > b->value)? +1 : 0);
}

/* Convert spot function "type" to a threshold matrix of size "width"
 * times "width".  Returns newly allocated threshold matrix.  The
 * reason for qsort()ing the results rather than just using the spot
 * function's value directly as the threshold value is that we want to
 * ensure that the threshold matrix is tonally balanced - that is, for
 * a threshold value of x%, x% of the values in the matrix are < x%.
 *
 * Actually, it turns out that qsort()ing a function which is already
 * balanced can quite significantly detract from the quality of the
 * final result.  This is particularly noticable with the line or
 * diamond spot functions at 45 degrees.  This is because if the spot
 * function has multiple locations with the same value, qsort may use
 * them in any order.  Often, there is quite clearly an optimal order
 * however.  By marking functions as pre-balanced, this random
 * shuffling is avoided.  WARNING: a non-balanced spot function marked
 * as pre-balanced is bad: you'll end up with dark areas becoming too
 * dark or too light, and vice versa for light areas.  This is most
 * easily checked by halftoning an area, then bluring it back - you
 * should get the same colour back again.  The only way of getting a
 * correctly balanced function is by getting a formula for the spot's
 * area as a function of x and y - this can be fairly tough (ie
 * possiblly an integral in two dimensions that must be solved
 * analytically).
 *
 * The threshold matrix is used to compare against image values.  If
 * the image value is greater than the threshold value, then the
 * output pixel is illuminated.  This means that a threshold matrix
 * entry of 0 never causes output pixels to be illuminated.  */
static guchar *
spot2thresh(gint type, gint width)
{
    gdouble	sx, sy;
    gdouble	val;
    spotfn_t	*spotfn;
    guchar	*thresh;
    order_t	*order;
    gint	x, y;
    gint	i;
    gint	wid2 = width*width;
    gint	balanced = spotfn_list[type].balanced;

    thresh = g_new(guchar, wid2);
    spotfn = spotfn_list[type].fn;

    order = g_new(order_t, wid2);

    i = 0;
    for(y=0; y<width; y++)
    {
	for(x=0; x<width; x++)
	{
	    /* scale x & y to -1 ... +1 inclusive */
	    sx = (((gdouble)x) / (width-1) - 0.5) * 2;
	    sy = (((gdouble)y) / (width-1) - 0.5) * 2;
	    val = spotfn(sx, sy);
	    val = BOUNDS(val, -1, 1);	/* interval is inclusive */

	    order[i].index = i;
	    order[i].value = val;
	    i++;
	}
    }

    if (!balanced)
    {
	/* now sort array of (point, value) pairs in ascending order */
	qsort(order, wid2, sizeof(order_t), order_cmp);
    }

    /* compile threshold matrix in order from darkest to lightest */
    for(i=0; i < wid2; i++)
    {
	/* thresh[] contains values from 0 .. 254.  The reason for not
	 * including 255 is so that an image value of 255 remains
	 * unmolested.  It would be bad to filter a completely white
	 * image and end up with black speckles.  */
	if (balanced)
	    thresh[order[i].index] = order[i].value * 0xfe;
	else
	    thresh[order[i].index] = i * 0xff / wid2;
    }

    g_free(order);

    /* TODO: this is where the code to apply a transfer or dot gain
     * function to the threshold matrix would go. */

    return thresh;
}




/**************************************************************/
/* Main loop */


/* This function operates on the image, striding across it in tiles. */
static void
newsprint(GDrawable *drawable)
{
    GPixelRgn	src_rgn, dest_rgn;
    guchar	*src_row, *dest_row;
    guchar	*src, *dest;
    guchar	*thresh[4];
    gdouble	r;
    gdouble	theta;
    gdouble	rot[4];
    gdouble	k_pullout;
    gint	bpp, colour_bpp;
    gint	has_alpha;
    gint	b;
    gint	tile_width;
    gint	width;
    gint	row, col;
    gint	x, y, x_step, y_step;
    gint	x1, y1, x2, y2;
    gint	rx, ry;
    gint	progress, max_progress;
    gint	oversample;
    gint	colourspace;
    gpointer	pr;
    gint	w002;
    TM_INIT();

    width = pvals.cell_width;

    if (width < 0)
	width = -width;
    if (width < 1)
	width = 1;

    oversample = pvals.oversample;
    k_pullout = ((gdouble)pvals.k_pullout) / 100.0;

    width *= oversample;

    tile_width = gimp_tile_width();

    gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);

    bpp        = gimp_drawable_bpp(drawable->id);
    has_alpha  = gimp_drawable_has_alpha(drawable->id);
    colour_bpp = has_alpha? bpp-1 : bpp;
    colourspace= pvals.colourspace;
    if (bpp == 1) {
	colourspace = CS_GREY;
    } else {
	if (colourspace == CS_GREY)
	    colourspace = CS_RGB;
    }

    /* Bartlett window matrix optimisation */
    w002 = BARTLETT(0,0) * BARTLETT(0,0);
#if 0
    /* It turns out to be slightly slower to cache a pre-computed
     * bartlett matrix!   I put it down to d-cache pollution *shrug* */
    wgt = g_new(gint, oversample*oversample);
    for(y=-oversample/2; y<=oversample/2; y++)
	for(x=-oversample/2; x<=oversample/2; x++)
	    WGT(x,y) = BARTLETT(x,y);
#endif /* 0 */

#define ASRT(_x)						\
do {								\
    if (!VALID_SPOTFN(_x))					\
    {								\
	printf("newsprint: %d is not a valid spot type\n", _x);	\
	_x = SPOTFN_DOT;					\
    }								\
} while(0)

    /* calculate the RGB / CMYK rotations and threshold matrices */
    if (bpp == 1 || colourspace == CS_INTENSITY)
    {
	rot[0]    = DEG2RAD(pvals.gry_ang);	
	thresh[0] = spot2thresh(pvals.gry_spotfn, width);
    }
    else
    {
	gint rf = pvals.red_spotfn;
	gint gf = pvals.grn_spotfn;
	gint bf = pvals.blu_spotfn;

	rot[0] = DEG2RAD(pvals.red_ang);
	rot[1] = DEG2RAD(pvals.grn_ang);
	rot[2] = DEG2RAD(pvals.blu_ang);

	/* always need at least one threshold matrix */
	ASRT(rf);
	spotfn_list[rf].thresh = spot2thresh(rf, width);
	thresh[0] = spotfn_list[rf].thresh;

	/* optimisation: only use separate threshold matrices if the
	 * spot functions are actually different */
	ASRT(gf);
	if (!spotfn_list[gf].thresh)
	    spotfn_list[gf].thresh = spot2thresh(gf, width);
	thresh[1] = spotfn_list[gf].thresh;

	ASRT(bf);
	if (!spotfn_list[bf].thresh)
	    spotfn_list[bf].thresh = spot2thresh(bf, width);
	thresh[2] = spotfn_list[bf].thresh;

	if (colourspace == CS_CMYK)
	{
	    rot[3] = DEG2RAD(pvals.gry_ang);
	    gf = pvals.gry_spotfn;
	    ASRT(gf);
	    if (!spotfn_list[gf].thresh)
		spotfn_list[gf].thresh = spot2thresh(gf, width);	
	    thresh[3] = spotfn_list[gf].thresh;
	}
    }



    /* Initialize progress */
    progress     = 0;
    max_progress = (x2 - x1) * (y2 - y1);

    TM_START();

    for( y = y1; y < y2; y += tile_width - ( y % tile_width ) )
    {
	for( x = x1; x < x2; x += tile_width - ( x % tile_width ) )
	{
	    /* snap to tile boundary */
	    x_step = tile_width - ( x % tile_width );
	    y_step = tile_width - ( y % tile_width );
	    /* don't step off the end of the image */
	    x_step = MIN( x_step, x2-x );
	    y_step = MIN( y_step, y2-y );

	    /* set up the source and dest regions */
	    gimp_pixel_rgn_init (&src_rgn, drawable, x, y, x_step, y_step,
				 FALSE/*dirty*/, FALSE/*shadow*/);

	    gimp_pixel_rgn_init (&dest_rgn, drawable, x, y, x_step, y_step,
				 TRUE/*dirty*/, TRUE/*shadow*/);

	    /* page in the image, one tile at a time */
	    for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
		 pr != NULL;
		 pr = gimp_pixel_rgns_process (pr))
	    {
		src_row  = src_rgn.data;
		dest_row = dest_rgn.data;

		for (row = 0; row < src_rgn.h; row++)
		{
		    src  = src_row;
		    dest = dest_row;
		    for (col = 0; col < src_rgn.w; col++)
		    {
			guchar data[4];

			rx = (x + col) * oversample;
			ry = (y + row) * oversample;

			/* convert rx and ry to polar (r, theta) */
			r = sqrt(((double)rx)*rx + ((double)ry)*ry);
			theta = atan2(((gdouble)ry), ((gdouble)rx));

			for(b=0; b<colour_bpp; b++)
			    data[b] = src[b];

			/* do colour space conversion */
			switch(colourspace) {
			case CS_CMYK:
			    data[3] = 0xff;

			    data[0] = 0xff - data[0];
			    data[3] = MIN(data[3], data[0]);
			    data[1] = 0xff - data[1];
			    data[3] = MIN(data[3], data[1]);
			    data[2] = 0xff - data[2];
			    data[3] = MIN(data[3], data[2]);

			    data[3] = ((gdouble)data[3]) * k_pullout;

			    data[0] -= data[3];
			    data[1] -= data[3];
			    data[2] -= data[3];
			    break;

			case CS_INTENSITY:
			    data[3] = data[0]; /* save orig for later */
			    data[0] = INTENSITY(data[0], data[1], data[2]);
			    break;

			default:
			    break;
			}
			

			for(b=0; b<cspace_nchans[colourspace]; b++)
			{
			    rx = rint(r * cos(theta + rot[b]));
			    ry = rint(r * sin(theta + rot[b]));

			    /* Make sure rx and ry are positive and within
			     * the range 0 .. width-1 (incl).  Can't use %
			     * operator, since its definition on negative
			     * numbers is not helpful.  Can't use ABS(),
			     * since that would cause reflection about the
			     * x- and y-axes.  Relies on integer division
			     * rounding towards zero. */
			    rx -= ((rx - ISNEG(rx)*(width-1)) / width) * width;
			    ry -= ((ry - ISNEG(ry)*(width-1)) / width) * width;

			    {
				guint32 sum = 0;
				gint sx, sy;
				gint tx, ty;
				for(sy=-oversample/2; sy<=oversample/2; sy++)
				    for(sx=-oversample/2; sx<=oversample/2; sx++)
				    {
					tx = rx+sx;
					ty = ry+sy;
					while (tx < 0)  tx += width;
					while (ty < 0)  ty += width;
					while (tx >= width)  tx -= width;
					while (ty >= width)  ty -= width;
					if (data[b] > THRESHn(b, tx, ty))
					    sum += 0xff * BARTLETT(sx, sy);
				    }
				sum /= w002;
				data[b] = sum;
			    }
			}
			if (has_alpha)
			    dest[colour_bpp] = src[colour_bpp];

			/* re-pack the colours into RGB */
			switch(colourspace) {
			case CS_CMYK:
			    data[0] = CLAMPED_ADD(data[0], data[3]);
			    data[1] = CLAMPED_ADD(data[1], data[3]);
			    data[2] = CLAMPED_ADD(data[2], data[3]);
			    data[0] = 0xff - data[0];
			    data[1] = 0xff - data[1];
			    data[2] = 0xff - data[2];
			    break;

			case CS_INTENSITY:
			    if (has_alpha)
			    {
				dest[colour_bpp] = data[0];
				data[0] = 0xff;
			    }
			    data[1] = data[1] * data[0] / 0xff;
			    data[2] = data[2] * data[0] / 0xff;
			    data[0] = data[3] * data[0] / 0xff;
			    break;

			default:
			    /* no other special cases */
			    break;
			}

			for(b=0; b<colour_bpp; b++)
			    dest[b] = data[b];

			src  += src_rgn.bpp;
			dest += dest_rgn.bpp;
		    }
		    src_row  += src_rgn.rowstride;
		    dest_row += dest_rgn.rowstride;
		}

		/* Update progress */
		progress += src_rgn.w * src_rgn.h;
		gimp_progress_update (
		    (double) progress / (double) max_progress);
	    }
	}
    }

    TM_END();

    /* We don't free the threshold matrices, since we're about to
     * exit, and the OS should clean up after us. */

    /* update the affected region */
    gimp_drawable_flush (drawable);
    gimp_drawable_merge_shadow (drawable->id, TRUE);
    gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}




/**************************************************************/
/* Support routines */

/* Lightly modified entscale:
 *  o Added entscale_set_sensitive() method */


/*************************************************************************/
/**									**/
/**			+++ Entscale					**/
/**									**/
/*************************************************************************/

/* these routines are taken from gflare.c */

/* -*- mode:c -*- */
/*
  Entry and Scale pair (int and double integrated)

  This is an attempt to combine entscale_int and double.
  Never compiled yet.

  TODO:
  - Do the proper thing when the user changes value in entry,
  so that callback should not be called when value is actually not changed.
  - Update delay
 */


static void   entscale_destroy_callback (GtkWidget *widget,
					 gpointer data);
static void   entscale_scale_update (GtkAdjustment *adjustment,
				     gpointer	   data);
static void   entscale_entry_update (GtkWidget *widget,
				     gpointer	data);
/*
 *  Create an entry, a scale and a label, then attach them to
 *  specified table.
 *  1 row and 2 cols of table are needed.
 *
 *  Input:
 *    table:	  table which entscale is attached to
 *    x, y:	  starting row and col in table
 *    caption:	  label string
 *    type:	  type of variable (ENTSCALE_INT or ENTSCALE_DOUBLE)
 *    variable:	  pointer to variable
 *    min, max:	  boundary of scale
 *    step:	  step of scale (ignored when (type == ENTSCALE_INT))
 *    constraint: (bool) true iff the value of *variable should be
 *		  constraint by min and max
 *    callback:	  called when the value is actually changed
 *		  (*variable is automatically changed, so there's no
 *		  need of callback func usually.)
 *    call_data:  data for callback func
 */

Entscale *
entscale_new (GtkWidget *table, gint x, gint y, gchar *caption,
	      EntscaleType type, gpointer variable,
	      gdouble min, gdouble max, gdouble step,
	      gint constraint,
	      EntscaleCallbackFunc callback,
	      gpointer call_data)
{
  Entscale	*entscale;
  GtkWidget	*entry;
  GtkWidget	*scale;
  GtkObject	*adjustment;
  gchar		buffer[256];
  gdouble	val;
  gdouble	constraint_val;

  entscale = g_new0 (Entscale, 1 );
  entscale->type	= type;
  entscale->constraint	= constraint;
  entscale->callback	= callback;
  entscale->call_data	= call_data;

  switch (type)
    {
    case ENTSCALE_INT:
      step = 1.0;
      strcpy (entscale->fmt_string, "%.0f");
      val = *(gint *)variable;
      break;
    case ENTSCALE_DOUBLE:
      sprintf (entscale->fmt_string, "%%.%df", entscale_get_precision (step) + 1);
      val = *(gdouble *)variable;
      break;
    default:
      g_error ("TYPE must be either ENTSCALE_INT or ENTSCALE_DOUBLE");
      val = 0;
      break;
    }

  entscale->label = gtk_label_new (caption);
  gtk_misc_set_alignment (GTK_MISC (entscale->label), 0.0, 0.5);

  /*
    If the first arg of gtk_adjustment_new() isn't between min and
    max, it is automatically corrected by gtk later with
    "value_changed" signal. I don't like this, since I want to leave
    *variable untouched when `constraint' is false.
    The lines below might look oppositely, but this is OK.
   */
  if (constraint)
    constraint_val = val;
  else
    constraint_val = BOUNDS (val, min, max);

  adjustment = entscale->adjustment =
    gtk_adjustment_new (constraint_val, min, max, step, step, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, ENTSCALE_SCALE_WIDTH, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  entry = entscale->entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTSCALE_ENTRY_WIDTH, 0);
  sprintf (buffer, entscale->fmt_string, val);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);

  /* entscale is done */
  gtk_object_set_user_data (GTK_OBJECT(adjustment), entscale);
  gtk_object_set_user_data (GTK_OBJECT(entry), entscale);

  /* now ready for signals */
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entscale_entry_update,
		      variable);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) entscale_scale_update,
		      variable);
  gtk_signal_connect (GTK_OBJECT (entry), "destroy",
		      (GtkSignalFunc) entscale_destroy_callback,
		      NULL );

  /* start packing */
  entscale->hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (entscale->hbox), scale, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (entscale->hbox), entry, FALSE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), entscale->label, x, x+1, y, y+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entscale->hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (entscale->label);
  gtk_widget_show (entry);
  gtk_widget_show (scale);
  gtk_widget_show (entscale->hbox);

  return entscale;
}

static int
entscale_get_precision (gdouble step)
{
  int precision;
  if (step <= 0)
    return 0;
  for (precision = 0; pow (0.1, precision) > step; precision++) ;
  return precision;
}

static void
entscale_destroy_callback (GtkWidget *widget,
			   gpointer data)
{
  Entscale *entscale;

  entscale = gtk_object_get_user_data (GTK_OBJECT (widget));
  g_free (entscale);
}
static void
entscale_scale_update (GtkAdjustment *adjustment,
		       gpointer	     data)
{
  Entscale	*entscale;
  GtkEntry	*entry;
  gchar		buffer[256];
  gdouble	old_val, new_val;

  entscale = gtk_object_get_user_data (GTK_OBJECT (adjustment));

  new_val = adjustment->value;
  /* adjustmet->value is always constrainted */

  switch (entscale->type)
    {
    case ENTSCALE_INT:
      old_val = *(gint*) data;
      *(gint*) data = (gint) new_val;
      DEBUG_PRINT (("entscale_scale_update(int): fmt=\"%s\" old=%g new=%g\n",
		    entscale->fmt_string, old_val, new_val));
      break;
    case ENTSCALE_DOUBLE:
      old_val = *(gdouble*) data;
      *(gdouble*) data = new_val;
      DEBUG_PRINT (("entscale_scale_update(double): fmt=\"%s\" old=%g new=%g\n",
		    entscale->fmt_string, old_val, new_val));
      break;
    default:
      g_warning ("entscale_scale_update: invalid type");
      return;
    }

  entry = GTK_ENTRY (entscale->entry);
  sprintf (buffer, entscale->fmt_string, new_val);

  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data (GTK_OBJECT(entry), data);
  gtk_entry_set_text (entry, buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT(entry), data);

  if (entscale->callback && (old_val != new_val))
    (*entscale->callback) (new_val, entscale->call_data);
}

static void
entscale_entry_update (GtkWidget *widget,
		       gpointer	  data)
{
  Entscale	*entscale;
  GtkAdjustment *adjustment;
  gdouble	old_val, val, new_val, constraint_val;

  entscale = gtk_object_get_user_data (GTK_OBJECT (widget));
  adjustment = GTK_ADJUSTMENT (entscale->adjustment);

  val = atof (gtk_entry_get_text (GTK_ENTRY (widget)));

  constraint_val = BOUNDS (val, adjustment->lower, adjustment->upper);

  if (entscale->constraint)
    new_val = constraint_val;
  else
    new_val = val;

  switch (entscale->type)
    {
    case ENTSCALE_INT:
      old_val = *(gint*) data;
      *(gint*) data = (gint) new_val;
      DEBUG_PRINT (("entscale_entry_update(int): old=%g new=%g const=%g\n",
		    old_val, new_val, constraint_val));
      break;
    case ENTSCALE_DOUBLE:
      old_val = *(gdouble*) data;
      *(gdouble*) data = new_val;
      DEBUG_PRINT (("entscale_entry_update(double): old=%g new=%g const=%g\n",
		    old_val, new_val, constraint_val));
      break;
    default:
      g_warning ("entscale_entry_update: invalid type");
      return;
    }

  adjustment->value = constraint_val;
  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data (GTK_OBJECT(adjustment), data );
  gtk_signal_emit_by_name (GTK_OBJECT(adjustment), "value_changed");
  gtk_signal_handler_unblock_by_data (GTK_OBJECT(adjustment), data );

  if (entscale->callback && (old_val != new_val))
    (*entscale->callback) (new_val, entscale->call_data);
}


static void entscale_set_sensitive(Entscale *ent, gint sensitive)
{
    gtk_widget_set_sensitive(GTK_WIDGET(ent->label), sensitive);
    gtk_widget_set_sensitive(GTK_WIDGET(ent->hbox), sensitive);
}


static void entscale_set_value(Entscale *ent, gfloat value)
{
    gtk_adjustment_set_value(GTK_ADJUSTMENT(ent->adjustment), value);
}
