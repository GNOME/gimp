/* maze.c, version 0.4.2, 17 October 1997
 * This is a plug-in for the GIMP.
 * It draws mazes...  walls and passages are 1 pixel wide.
 * 
 * Implemented as a GIMP 0.99 Plugin by 
 * Kevin Turner <kevint@poboxes.com>
 * http://www.poboxes.com/kevint/gimp/maze.html
 * 
 * mazegen code from rec.games.programmer's maze-faq:
 * * maz.c - generate a maze
 * *
 * * algorithm posted to rec.games.programmer by jallen@ic.sunysb.edu
 * * program cleaned and reorganized by mzraly@ldbvax.dnet.lotus.com
 * *
 * * don't make people pay for this, or I'll jump up and down and
 * * yell and scream and embarass you in front of your friends...
 *
 * Code generously borrowed from assorted GIMP plugins
 * and used as a template to get me started on this one.  :)
 * 
 * Revision history:
 * 0.4.2  - Applied Adrian Likins' patch to fix non-interactive stuff.
 *        - -ansi and -pedantic-errors clean.  Woo-hoo?
 * 0.4.1  - get_colors() now works properly for grayscale images.
 *          I'd still like it to do indexed too, but I don't know
 *          if that's worth breaking a sweat over.
 *        - We're -Wall clean now.  Woohoo!
 * 0.4.0  - Code for the painting of the maze has been almost completely rebuilt.
 *            Hopefully it's a more sane and speedier approach.
 *            Utilizes a new function, drawbox, which colors a solid rectangle.
 *            (Good excercise, in any case.)
 *        - Order of paramaters changed, defaults are used if not given.
 *        - Discovery made that that was an utterly useless thing to do.
 * 0.3.0  - Maze is centered with dead space around outside
 *        - Width slider works...  And does stuff!  
 *        - Allows partial mazes to be generated with "broken" multiple
 *             and offset values.
 * 0.1.99 - Has dialog box with seed, multiple, and offset.
 * 0.1.0  - First release.  It works!  :)
 *
 * TO DO:
 *   Add an option to kill the outer border.
 * 
 *   Fix that stray line down there between maze wall and dead space border...
 *
 *   Make get_colors() work with indexed.  * HELP! *
 *
 *   Tileable mazes are fun :) 
 *   
 *   If we add many more paramaters, we'll need a preview box.
 *
 * Also someday:
 * Maybe make it work with irregularly shaped selections?
 * Add different generation algorythms.
 *
 */

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

#include <time.h>  /* For random seeding */
#include <stdio.h>
#include <stdlib.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#define ENTRY_WIDTH 75
#define MAZE_TITLE "Maze 0.4.2"
/* entscale stuff begin */
#define ENTSCALE_INT_SCALE_WIDTH 125
#define ENTSCALE_INT_ENTRY_WIDTH 40
/* entscale stuff end */

typedef struct {
    gint width;
    gint seed;
    gint multiple;
    gint offset;
} MazeValues;

typedef struct {
    gint run;
} MazeInterface;

/* entscale stuff begin */
typedef void (*EntscaleIntCallbackFunc) (gint value, gpointer data);

typedef struct {
  GtkObject     *adjustment;
  GtkWidget     *entry;
  gint          constraint;
  EntscaleIntCallbackFunc	callback;
  gpointer	call_data;
} EntscaleIntData;
/* entscale stuff end */

static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);
static void      maze   (GDrawable * drawable);
static gint      mazegen(gint     pos,
			 gchar   *maz,
			 gint     x,
                         gint     y,
			 gint     rnd);
static void      get_colors (GDrawable * drawable,
			     guint8 *fg,
			     guint8 *bg);

static void      drawbox (GPixelRgn *dest_rgn, 
			  guint x, 
			  guint y,
			  guint w,
			  guint h, 
			  guint8 clr[4]);

static gint maze_dialog (void);
static void maze_close_callback (GtkWidget *widget, gpointer data);
static void maze_ok_callback  (GtkWidget *widget, gpointer data);
static void maze_entry_callback  (GtkWidget *widget, gpointer data);

/* entscale stuff begin */
void   entscale_int_new ( GtkWidget *table, gint x, gint y,
			  gchar *caption, gint *intvar, 
			  gint min, gint max, gint constraint,
			  EntscaleIntCallbackFunc callback,
			  gpointer data );

static void   entscale_int_destroy_callback (GtkWidget *widget,
					     gpointer data);
static void   entscale_int_scale_update (GtkAdjustment *adjustment,
					 gpointer      data);
static void   entscale_int_entry_update (GtkWidget *widget,
					 gpointer   data);
/* entscale stuff end */
/* message box stuff begin */
GtkWidget *   message_box (char *, GtkCallback, gpointer);
/* message box stuff end */


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static MazeValues mvals = 
{
    1,      /* Passage width */
    0,     /* seed */
    57,    /* multiple * These two had "Experiment with this?" comments */
    1      /* offset   * in the maz.c source, so, lets expiriment.  :) */
};

static MazeInterface mint =
{
    FALSE  /* run */
};

MAIN () /*;*/

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    /* If we did have parameters, these be them: */
    { PARAM_INT32, "mazep_size", "Size of the passages" },
    { PARAM_INT32, "maze_rseed", "Random Seed"},
    { PARAM_INT32, "maze_multiple", "Multiple (use 57)" },
    { PARAM_INT32, "maze_offset", "Offset (use 1)" }
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_maze",
			  "Generates a maze.",
			  "Generates a maze using the depth-first search method.",
			  "Kevin Turner <kevint@poboxes.com>",
			  "Kevin Turner",
			  "1997",
			  "<Image>/Filters/Render/Maze",
			  "RGB*, GRAY*, INDEXED*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

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

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

#ifdef MAZE_DEBUG
  fprintf(stderr, "%d", param[2].data.d_drawable);
#endif

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_maze", &mvals);
      
      /* Acquire info with a dialog */
      if (! maze_dialog ()) {
	gimp_drawable_detach (drawable);
	return;
      }
      break;
      
    case RUN_NONINTERACTIVE:
      /* WARNING: Stupidity Follows */
      if (nparams != 7)
	{
	  status = STATUS_CALLING_ERROR;
	}
      if (status == STATUS_SUCCESS)
	{
	  mvals.width = (gint) param[3].data.d_int32;
	  mvals.seed = (gint) param[4].data.d_int32;
	  mvals.multiple = (gint) param[5].data.d_int32;
	  mvals.offset = (gint) param[6].data.d_int32;
	}
      break;
      /* #define MAZE_DEBUG */
#ifdef MAZE_DEBUG
      fprintf(stderr,"nparams: %d width: %d seed: %d multiple: %d offset: %d\n",
	      nparams, mvals.width, mvals.seed, mvals.multiple, mvals.offset);
#endif
    case RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_maze", &mvals);
      mvals.seed=time(NULL);   /* ** USES NEW SEED when reruning with "last" */
      /*          values.  Maybe not the Right Thing, but I find it handy. */
      break;
      
    default:
      break;
  }
  
  /* color, gray, or indexed... hmm, miss anything?  ;)  */
  if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id) || gimp_drawable_indexed (drawable->id)) {
      gimp_progress_init ("Drawing Maze...");

      maze (drawable);
      
      if (run_mode != RUN_NONINTERACTIVE)
	  gimp_displays_flush ();
      
      if (run_mode == RUN_INTERACTIVE /*|| run_mode == RUN_WITH_LAST_VALS*/)         
	  gimp_set_data ("plug_in_maze", &mvals, sizeof (MazeValues));
  } else {
      status = STATUS_EXECUTION_ERROR;
  }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
maze( GDrawable * drawable)
{
  GPixelRgn dest_rgn;
  gint mw, mh;
  gint deadx, deady;
  gint progress, max_progress;
  gint x1, y1, x2, y2, x, y;
  gint dx, dy, xx, yy;
  gint foo, bar, baz;
  guint8 fg[4],bg[4];
  gpointer pr;

  gchar *maz;
  gint i;

  /* Gets the input area... */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /* Initialize pixel region (?) */
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  /* Get the foreground and background colors */
  get_colors(drawable,fg,bg);

  /* Maze Stuff Happens Here */
  mw = (x2-x1) / mvals.width;
  mh = (y2-y1) / mvals.width;

  mw -= !(mw & 1); /* mazegen doesn't work with even-sized mazes. */
  mh -= !(mh & 1); /* Note I don't warn the user about this... */

  deadx = ((x2-x1) - mw * mvals.width)/2;
  deady = ((y2-y1) - mh * mvals.width)/2;

  maz = g_malloc(mw * mh);

  for (i = 0; i < (mw * mh); ++i)
      maz[i] = 0;

#ifdef MAZE_DEBUG
  printf("x:  %d\ty:  %d\nmw: %d\tmh: %d\ndx: %d\tdy: %d\nwidth:%d\n",
	 (x2-x1),(y2-y1),mw,mh,deadx,deady,mvals.width);
#endif

  (void) mazegen((mw+1), maz, mw, mh, mvals.seed);
  /* (void) mazegen(((x2-x1)+1), maz, (x2-x1), (y2-y1), rnd); */
  /* It's done happening.  Now go through and color dem pixels...  */

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn); 
       pr != NULL; 
       pr = gimp_pixel_rgns_process (pr))
      {
	  x = dest_rgn.x - x1 - deadx;
	  y = dest_rgn.y - y1 - deady;

	  /* First boxes by edge of tile must be handled specially 
	     because they may have started on a previous tile,
	     unbeknownst to us. */

	  dx = mvals.width - (x % mvals.width);
	  dy = mvals.width - (y % mvals.width);
	  foo = x/mvals.width;
	  bar = mw * (y/mvals.width);

	  /* Draws the upper-left [split] box */
	  drawbox(&dest_rgn,0,0,dx,dy,
		  maz[foo+bar] ? fg : bg);

	  baz=foo+1;
	  /* Draw the top row [split] boxes */
	  for(xx=dx/*1*/; xx < dest_rgn.w; xx+=mvals.width/*+1*/)
	      { drawbox(&dest_rgn,xx,0,mvals.width,dy,
			maz[bar + baz++] ? fg : bg ); }

	  baz=bar+mw;
	  /* Left column */
	  for(yy=dy/*+1*/; yy < dest_rgn.h; yy+=mvals.width/*+1*/) {
	      drawbox(&dest_rgn,0,yy,dx,mvals.width,
		      maz[foo + baz] ? fg : bg );
	      baz += mw;
	  }
	  
	  foo++;
	  /* Everything else */
	  for(yy=dy/*+1*/; yy < dest_rgn.h; yy+=mvals.width/*+1*/) {
	      baz = foo; bar+=mw;
	      for(xx=dx/*+1*/; xx < dest_rgn.w; xx+=mvals.width/*+1*/)
		  { 
#ifdef MAZE_DEBUG
		      putchar(maz[bar+baz] ? '#' : '.');
#endif
		      drawbox(&dest_rgn,xx,yy,mvals.width,mvals.width,
			    maz[bar + baz++] ? fg : bg ); }
#ifdef MAZE_DEBUG
	      putchar('\n');
#endif
	  }

	  progress += dest_rgn.w * dest_rgn.h;
	  gimp_progress_update ((double) progress / (double) max_progress);
	  /* Note the progess indicator doesn't indicate how much of the maze
	     has been built...  It just indicates how much has been drawn
	     *after* building...  Thing is, that's what takes longer.  */
      }
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

void drawbox( GPixelRgn *dest_rgn, 
	      guint x, guint y, guint w, guint h, 
	      guint8 clr[4])
{
    guint xx,yy, foo, bar;
    guint8 bp;

    /* This looks ugly, but it's just finding the lower number... */
    foo	= dest_rgn->h * dest_rgn->rowstride
	< y * dest_rgn->rowstride + h * dest_rgn->rowstride
	? dest_rgn->h * dest_rgn->rowstride 
	: y * dest_rgn->rowstride + h * dest_rgn->rowstride;

    bar	= dest_rgn->w * dest_rgn->bpp
	< x * dest_rgn->bpp + w * dest_rgn->bpp
	? dest_rgn->w * dest_rgn->bpp
	: x * dest_rgn->bpp + w * dest_rgn->bpp;

    for (yy = dest_rgn->rowstride * y; 
	 yy < foo; 
	 yy += dest_rgn->rowstride ) {
	for (xx= x * dest_rgn->bpp;
	     xx < bar;
	     xx+= dest_rgn->bpp) {
	    for (bp=0; bp < dest_rgn->bpp; bp++) {
		dest_rgn->data[yy+xx+bp]=clr[bp];
	    } /* next bp */
	} /* next xx */
    } /* next yy */
}

/* The Incredible Recursive Maze Generation Routine */
/* Ripped from rec.programmers.games maze-faq       */
/* Modified and commented by me, Kevin Turner. */
gint mazegen(pos, maz, x, y, rnd)
gint pos, x, y, rnd;
gchar *maz;
{
    gchar d, i;
    gint c=0, j=1;

    /* Punch a hole here...  */
    maz[pos] = 1;

    /* If there is a wall two rows above us, bit 1 is 1. */
    while((d= (pos <= (x * 2) ? 0 : (maz[pos - x - x ] ? 0 : 1))
	  /* If there is a wall two rows below us, bit 2 is 1. */
	  | (pos >= x * (y - 2) ? 0 : (maz[pos + x + x] ? 0 : 2))
	  /* If there is a wall two columns to the right, bit 3 is 1. */
	  | (pos % x == x - 2 ? 0 : (maz[pos + 2] ? 0 : 4))
	  /* If there is a wall two colums to the left, bit 4 is 1.  */
	  | ((pos % x == 1 ) ? 0 : (maz[pos-2] ? 0 : 8)))) {

	/* Note if all bits are 0, d is false, we don't do this
	   while loop, we don't call ourselves again, so this branch
           is done.  */

	/* I see what this loop does (more or less), but I don't know
	   _why_ it does it this way...  I also haven't figured out exactly
	   which values of multiple will work and which won't.  */
	do {
	    rnd = (rnd * mvals.multiple + mvals.offset);
	    i = 3 & (rnd / d);
	    if (++c > 100) {  /* Break and try to salvage something */
		i=99;         /* if it looks like we're going to be */
		break;        /* here forever...                    */
	    }
	} while ( !(d & ( 1 << i) ) );
	/* ...While there's *not* a wall in direction i. */
        /* (stop looping when there is) */

	switch (i) {  /* This is simple enough. */
	case 0:       /* Go in the direction we just figured . . . */
	    j= -x;   
	    break;
	case 1:
	    j = x;
	    break;
	case 2:
	    j=1;
	    break;
	case 3:
	    j= -1;
	    break;
	case 99:
	    return 1;  /* Hey neat, broken mazes! */
	    break;     /* (Umm... Wow... Yeah, neat.) */
	default:
	    break;
	}

	/* And punch a hole there. */
	maz[pos + j] = 1;

        /* Now, start again just past where we punched the hole... */
	mazegen(pos + 2 * j, maz, x, y, rnd);

    } /* End while(d=...) Loop */

    return 0;
}

static void 
get_colors (GDrawable *drawable, guint8 *fg, guint8 *bg) 
{
  GParam *return_vals;
  gint nreturn_vals;

  switch ( gimp_drawable_type (drawable->id) )
    {
    case RGBA_IMAGE:   /* ASSUMPTION: Assuming the user wants entire */
      fg[3] = 255;                 /* area to be fully opaque.       */
      bg[3] = 255;
    case RGB_IMAGE:

      return_vals = gimp_run_procedure ("gimp_palette_get_foreground",
					&nreturn_vals,
					PARAM_END);

      if (return_vals[0].data.d_status == STATUS_SUCCESS)
	{
	  fg[0] = return_vals[1].data.d_color.red;
	  fg[1] = return_vals[1].data.d_color.green;
	  fg[2] = return_vals[1].data.d_color.blue;
	}
      else
	{
	  fg[0] = 255;
	  fg[1] = 255;
	  fg[2] = 255;
	}
      return_vals = gimp_run_procedure ("gimp_palette_get_background",
					&nreturn_vals,
					PARAM_END);

      if (return_vals[0].data.d_status == STATUS_SUCCESS)
	{
	  bg[0] = return_vals[1].data.d_color.red;
	  bg[1] = return_vals[1].data.d_color.green;
	  bg[2] = return_vals[1].data.d_color.blue;
	}
      else
	{
	  bg[0] = 0;
	  bg[1] = 0;
	  bg[2] = 0;
	}
      break;
    case GRAYA_IMAGE:       /* and again */
      fg[1] = 255;
      bg[1] = 255;
    case GRAY_IMAGE:       

	return_vals = gimp_run_procedure ("gimp_palette_get_foreground",
					&nreturn_vals,
					PARAM_END);

	if (return_vals[0].data.d_status == STATUS_SUCCESS)
	    {
		fg[0] = 0.30 * return_vals[1].data.d_color.red + 
		    0.59 * return_vals[1].data.d_color.green + 
		    0.11 * return_vals[1].data.d_color.blue;
	    }
	else
	    {
		fg[0] = 255;
	    }

	return_vals = gimp_run_procedure ("gimp_palette_get_background",
					  &nreturn_vals,
					  PARAM_END);
	
	if (return_vals[0].data.d_status == STATUS_SUCCESS)
	    {
		bg[0] = 0.30 * return_vals[1].data.d_color.red + 
		    0.59 * return_vals[1].data.d_color.green + 
		    0.11 * return_vals[1].data.d_color.blue;
	    }
	else
	    {
		bg[0] = 0;
	    }
	
	break;
    case INDEXEDA_IMAGE:
    case INDEXED_IMAGE:     /* FIXME: Should use current fg/bg colors.  */
	fputs("Maze: Using indexed colors 15 and 0 to draw maze.",stderr);
	fg[0] = 15;        /* As a plugin, I protest.  *I* shouldn't be the */
	bg[0] = 0;       /* one who has to deal with this colormapcrap.   */
	break;
    default:
      break;
    }
}

static gint maze_dialog() 
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *notebook;
  gchar **argv;
  gint  argc;
  gchar buffer[32];

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("maze");

  gtk_init (&argc, &argv);

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), MAZE_TITLE);
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) maze_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) maze_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /* Create notebook */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook);

  /*  Set up Options page  */
  frame = gtk_frame_new ("Maze Options");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  /* gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0); */
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* Seed input box */
  label = gtk_label_new ("Seed");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0 );
  gtk_widget_show (label);
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0 );
  gtk_widget_set_usize( entry, ENTRY_WIDTH, 0 );
  sprintf( buffer, "%d", mvals.seed );
  gtk_entry_set_text (GTK_ENTRY (entry), buffer );
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) maze_entry_callback,
		      &mvals.seed);
  gtk_widget_show (entry);

  /* entscale == Entry and Scale pair function found in pixelize.c */
  entscale_int_new (table, 0, 1, "Width:", &mvals.width, 
		    1, 64, FALSE, 
		    NULL, NULL);

  /* Add Options page to notebook */
  gtk_widget_show (frame);
  gtk_widget_show (table);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, 
			    gtk_label_new ("Options"));

  /* Set up other page */
  frame = gtk_frame_new ("Don't change these");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  /* gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0); */
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* Multiple input box */
  label = gtk_label_new ("Multiple (57)");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0 );
  gtk_widget_show (label);
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0 );
  gtk_widget_set_usize( entry, ENTRY_WIDTH, 0 );
  sprintf( buffer, "%d", mvals.multiple );
  gtk_entry_set_text (GTK_ENTRY (entry), buffer );
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) maze_entry_callback,
		      &mvals.multiple);
  gtk_widget_show (entry);

  /* Offset input box */
  label = gtk_label_new ("Offset (1)");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0 );
  gtk_widget_show (label);
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0 );
  gtk_widget_set_usize( entry, ENTRY_WIDTH, 0 );
  sprintf( buffer, "%d", mvals.offset );
  gtk_entry_set_text (GTK_ENTRY (entry), buffer );
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) maze_entry_callback,
		      &mvals.offset);
  gtk_widget_show (entry);

   /* Add Advanced page to notebook */
  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, 
			    gtk_label_new ("Advanced"));

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return mint.run;
}

/* Maze Interface Functions */
static void 
maze_close_callback (GtkWidget *widget, 
		     gpointer data)
{
    gtk_main_quit ();
}

static void
maze_ok_callback (GtkWidget *widget,
		  gpointer data)
{
    mint.run = TRUE;
    gtk_widget_destroy (GTK_WIDGET (data));
}

static void 
maze_entry_callback (GtkWidget *widget,
		       gpointer data)
{
    gint *text_val;

    text_val = (gint *) data;

    *text_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}

/* ==================================================================== */
/* As found in pixelize.c */

/*
  Entry and Scale pair 1.03

  TODO:
  - Do the proper thing when the user changes value in entry,
  so that callback should not be called when value is actually not changed.
  - Update delay
 */

/*
 *  entscale: create new entscale with label. (int)
 *  1 row and 2 cols of table are needed.
 *  Input:
 *    x, y:       starting row and col in table
 *    caption:    label string
 *    intvar:     pointer to variable
 *    min, max:   the boundary of scale
 *    constraint: (bool) true iff the value of *intvar should be constraint
 *                by min and max
 *    callback:	  called when the value is actually changed
 *    call_data:  data for callback func
 */
void
entscale_int_new ( GtkWidget *table, gint x, gint y,
		   gchar *caption, gint *intvar,
		   gint min, gint max, gint constraint,
		   EntscaleIntCallbackFunc callback,
		   gpointer call_data)
{
  EntscaleIntData *userdata;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *scale;
  GtkObject *adjustment;
  gchar    buffer[256];
  gint	    constraint_val;

  userdata = g_new ( EntscaleIntData, 1 );

  label = gtk_label_new (caption);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  /*
    If the first arg of gtk_adjustment_new() isn't between min and
    max, it is automatically corrected by gtk later with
    "value_changed" signal. I don't like this, since I want to leave
    *intvar untouched when `constraint' is false.
    The lines below might look oppositely, but this is OK.
   */
  userdata->constraint = constraint;
  if( constraint )
    constraint_val = *intvar;
  else
    constraint_val = ( *intvar < min ? min : *intvar > max ? max : *intvar );

  userdata->adjustment = adjustment = 
    gtk_adjustment_new ( constraint_val, min, max, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new ( GTK_ADJUSTMENT(adjustment) );
  gtk_widget_set_usize (scale, ENTSCALE_INT_SCALE_WIDTH, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  userdata->entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTSCALE_INT_ENTRY_WIDTH, 0);
  sprintf( buffer, "%d", *intvar );
  gtk_entry_set_text( GTK_ENTRY (entry), buffer );

  userdata->callback = callback;
  userdata->call_data = call_data;

  /* userdata is done */
  gtk_object_set_user_data (GTK_OBJECT(adjustment), userdata);
  gtk_object_set_user_data (GTK_OBJECT(entry), userdata);

  /* now ready for signals */
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entscale_int_entry_update,
		      intvar);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) entscale_int_scale_update,
		      intvar);
  gtk_signal_connect (GTK_OBJECT (entry), "destroy",
		      (GtkSignalFunc) entscale_int_destroy_callback,
		      userdata );

  /* start packing */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), label, x, x+1, y, y+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (label);
  gtk_widget_show (entry);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);
}  


/* when destroyed, userdata is destroyed too */
static void
entscale_int_destroy_callback (GtkWidget *widget,
			       gpointer data)
{
  EntscaleIntData *userdata;

  userdata = data;
  g_free ( userdata );
}

static void
entscale_int_scale_update (GtkAdjustment *adjustment,
			   gpointer      data)
{
  EntscaleIntData *userdata;
  GtkEntry	*entry;
  gchar		buffer[256];
  gint		*intvar = data;
  gint		new_val;

  userdata = gtk_object_get_user_data (GTK_OBJECT (adjustment));

  new_val = (gint) adjustment->value;

  *intvar = new_val;

  entry = GTK_ENTRY( userdata->entry );
  sprintf (buffer, "%d", (int) new_val );
  
  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data ( GTK_OBJECT(entry), data );
  gtk_entry_set_text ( entry, buffer);
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(entry), data );

  if (userdata->callback)
    (*userdata->callback) (*intvar, userdata->call_data);
}

static void
entscale_int_entry_update (GtkWidget *widget,
			   gpointer   data)
{
  EntscaleIntData *userdata;
  GtkAdjustment	*adjustment;
  int		new_val, constraint_val;
  int		*intvar = data;

  userdata = gtk_object_get_user_data (GTK_OBJECT (widget));
  adjustment = GTK_ADJUSTMENT( userdata->adjustment );

  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  constraint_val = new_val;
  if ( constraint_val < adjustment->lower )
    constraint_val = adjustment->lower;
  if ( constraint_val > adjustment->upper )
    constraint_val = adjustment->upper;

  if ( userdata->constraint )
    *intvar = constraint_val;
  else
    *intvar = new_val;

  adjustment->value = constraint_val;
  gtk_signal_handler_block_by_data ( GTK_OBJECT(adjustment), data );
  gtk_signal_emit_by_name ( GTK_OBJECT(adjustment), "value_changed");
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(adjustment), data );
  
  if (userdata->callback)
    (*userdata->callback) (*intvar, userdata->call_data);
}
