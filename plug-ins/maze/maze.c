/* maze.c, version 0.6.2, March 7, 1998.
 * This is a plug-in for the GIMP.
 * It draws mazes...
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
 * 0.6.2  - drawbox rewritten with memcpy and a row buffer.  By the way, Maze is now
 *          faster than Checkerboard.
 *        - Added Help button using extension_web_browser.
 *        - Added DIVBOX_LOOKS_LIKE_SPINBUTTON flag.  Doesn't look too hot, set to
 *          FALSE by default.
 * 0.6.1  - Made use-time-for-random-seed a toggle button that remembers
 *          its state, and moved seed to the second notebook page.
 * 0.6.0  - Width and height are now seperate options.  
 *          ^^ Note this changed the PDB interface. ^^
 *        
 *        - Added interface for selecting sizes by "divisions".
 *        - Added "Time" button for random seed.
 *        - Turned out that GParam shouldn't have been "fixed".
 * 0.5.0  - Added the long-awaited "tileable" option.
 *           Required a change to PDB parameters.
 *        - fixed some stuff with GParam values in run();
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
 *   Rework the divboxes to be more like spinbuttons.
 *
 *   Add an option to kill the outer border.
 * 
 *   Fix that stray line down there between maze wall and dead space border...
 *
 *   Make get_colors() work with indexed.  * HELP! *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef MAZE_DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif

#include <time.h>  /* For random seeding */
#include "libgimp/gimp.h"
#include "maze.h"

extern gint      maze_dialog (void);
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
static gint      mazegen_tileable(gint     pos,
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

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MazeValues mvals = 
{
    /* Calling parameters */
    5,      /* Passage width */
    5,      /* Passage height */
    0,     /* seed */
    FALSE, /* Tileable? */
    57,    /* multiple * These two had "Experiment with this?" comments */
    1,     /* offset   * in the maz.c source, so, lets expiriment.  :) */
    /* Interface options */
    TRUE /* Time seed? */
};

guint sel_w, sel_h;

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
    { PARAM_INT32, "mazep_width", "Width of the passages" },
    { PARAM_INT32, "mazep_height", "Height of the passages"},
    { PARAM_INT8, "maze_tile", "Tileable maze?"},
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
  gint x1, y1, x2, y2;

#ifdef MAZE_DEBUG
  g_print("maze PID: %d\n",getpid());
#endif
  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_maze", &mvals);
      
      /* The interface needs to know the dimensions of the image... */
      gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
      sel_w=x2-x1; sel_h=y2-y1;

      /* Acquire info with a dialog */
      if (! maze_dialog ()) {
	gimp_drawable_detach (drawable);
	return;
      }
      break;
      
    case RUN_NONINTERACTIVE:
      if (nparams != 9)
	{
	  status = STATUS_CALLING_ERROR;
	}
      if (status == STATUS_SUCCESS)
	{
	  mvals.width = (gint) param[3].data.d_int32;
	  mvals.height = (gint) param[4].data.d_int32;
	  mvals.seed = (gint) param[5].data.d_int32;
	  mvals.tile = (gboolean) param[6].data.d_int32;
	  mvals.multiple = (gint) param[7].data.d_int32;
	  mvals.offset = (gint) param[8].data.d_int32;
	}
      break;
    case RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_maze", &mvals);
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
      
      if (run_mode == RUN_INTERACTIVE || 
	  (mvals.timeseed && run_mode == RUN_WITH_LAST_VALS))
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
  mh = (y2-y1) / mvals.height;

  if (!mvals.tile) {
       mw -= !(mw & 1); /* mazegen doesn't work with even-sized mazes. */
       mh -= !(mh & 1); /* Note I don't warn the user about this... */
  } else { /* On the other hand, tileable mazes must be even. */
       mw -= (mw & 1);
       mh -= (mh & 1);
  };

  /* It will really suck if your tileable maze ends up with this dead
     space around it.  Oh well, life is hard. */
  deadx = ((x2-x1) - mw * mvals.width)/2;
  deady = ((y2-y1) - mh * mvals.height)/2;

  maz = g_malloc0(mw * mh);

#ifdef MAZE_DEBUG
  printf("x:  %d\ty:  %d\nmw: %d\tmh: %d\ndx: %d\tdy: %d\nwidth:%d\theight: %d\n",
	 (x2-x1),(y2-y1),mw,mh,deadx,deady,mvals.width, mvals.height);
#endif

  if (mvals.timeseed)
       mvals.seed = time(NULL);

  if (mvals.tile) {
       (void) mazegen_tileable((mw+1), maz, mw, mh, mvals.seed);
  } else {
       (void) mazegen((mw+1), maz, mw, mh, mvals.seed);
       /* (void) mazegen(((x2-x1)+1), maz, (x2-x1), (y2-y1), rnd); */
  }
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
	  dy = mvals.height - (y % mvals.height);
	  foo = x/mvals.width;
	  bar = mw * (y/mvals.height);

	  /* Draws the upper-left [split] box */
	  drawbox(&dest_rgn,0,0,dx,dy,
		  maz[foo+bar] ? fg : bg);

	  baz=foo+1;
	  /* Draw the top row [split] boxes */
	  for(xx=dx; xx < dest_rgn.w; xx+=mvals.width)
	      { drawbox(&dest_rgn,xx,0,mvals.width,dy,
			maz[bar + baz++] ? fg : bg ); }

	  baz=bar+mw;
	  /* Left column */
	  for(yy=dy; yy < dest_rgn.h; yy+=mvals.height) {
	      drawbox(&dest_rgn,0,yy,dx,mvals.height,
		      maz[foo + baz] ? fg : bg );
	      baz += mw;
	  }
	  
	  foo++;
	  /* Everything else */
	  for(yy=dy; yy < dest_rgn.h; yy+=mvals.height) {
	      baz = foo; bar+=mw;
	      for(xx=dx; xx < dest_rgn.w; xx+=mvals.width)
		  { 
		      drawbox(&dest_rgn,xx,yy,mvals.width,mvals.height,
			    maz[bar + baz++] ? fg : bg ); }
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

/* Draws a solid color box in a GPixelRgn. */
/* Optimization assumptions:
 * (Or, "Why Maze is Faster Than Checkerboard.")
 * 
 * Assuming calling memcpy is faster than using loops.
 * Row buffers are nice...
 *
 * Assume allocating memory for row buffers takes a significant amount 
 * of time.  Assume drawbox will be called many times.
 * Only allocate memory once.
 *
 * Do not assume the row buffer will always be the same size.  Allow
 * for reallocating to make it bigger if needed.  However, I don't see 
 * reason to bother ever shrinking it again.
 * (Under further investigation, assuming the row buffer never grows
 * may be a safe assumption in this case.)
 *
 * Also assume that the program calling drawbox is short-lived, so
 * memory leaks aren't of particular concern-- the memory allocated to 
 * the row buffer is never set free.
 */

/* Further optimizations that could be made...
 *  Currently, the row buffer is re-filled with every call.  However,
 *  plug-ins such as maze and checkerboard only use two colors, and
 *  for the most part, have rows of the same size with every call.
 *  We could keep a row of each color on hand so we wouldn't have to
 *  re-fill it every time...  */


static void 
drawbox( GPixelRgn *dest_rgn, 
	 guint x, guint y, guint w, guint h, 
	 guint8 clr[4])
{
     guint xx, yy, y_max, x_min /*, x_max */;
     static guint8 *rowbuf;
     guint rowsize;
     static guint high_size=0;
  
     /* The maximum [xy] value is that of the far end of the box, or
      * the edge of the region, whichever comes first. */
     
     y_max = dest_rgn->rowstride * MIN(dest_rgn->h, (y + h));
/*   x_max = dest_rgn->bpp * MIN(dest_rgn->w, (x + w)); */
     
     x_min = x * dest_rgn->bpp;
     
     /* rowsize = x_max - x_min */
     rowsize = dest_rgn->bpp * MIN(dest_rgn->w, (x + w)) - x_min;
     
     /* Does the row buffer need to be (re)allocated? */
     if (high_size == 0) {
	  rowbuf = g_new(guint8, rowsize);
     } else if (rowsize > high_size) {
	  g_realloc(rowbuf, rowsize * sizeof(guint8) );
     }
     
     high_size = MAX(high_size, rowsize);
     
     /* Fill the row buffer with the color. */
     for (xx= 0;
	  xx < rowsize;
	  xx+= dest_rgn->bpp) {
	  memcpy (&rowbuf[xx], clr, dest_rgn->bpp);
     } /* next xx */
     
     /* Fill in the box in the region with rows... */
     for (yy = dest_rgn->rowstride * y; 
	  yy < y_max; 
	  yy += dest_rgn->rowstride ) {
	  memcpy (&dest_rgn->data[yy+x_min], rowbuf, rowsize);
     } /* next yy */
}

/* The old drawbox code, preserved here 'just in case' something
   doesn't go right. */

#if 0
	for (xx= x * dest_rgn->bpp;
	     xx < bar;
	     xx+= dest_rgn->bpp) {
#if 0
	    for (bp=0; bp < dest_rgn->bpp; bp++) {
		dest_rgn->data[yy+xx+bp]=clr[bp];
	    } /* next bp */
#else
		memcpy (&dest_rgn->data[yy+xx], clr, dest_rgn->bpp);
#endif
	} /* next xx */
    } /* next yy */
}
#endif 

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

#define ABSMOD(A,B) ( ((A) < 0) ? (((B) + (A)) % (B)) : ((A) % (B)) )

/* Tileable mazes are my creation, based on the routine above. */
static gint mazegen_tileable(gint pos, gchar *maz, gint x, gint y, gint rnd)
{
    gchar d, i;
    gint c=0, j=1, npos=2;

    /* Punch a hole here...  */
    maz[pos] = 1;

    /* If there is a wall two rows above us, bit 1 is 1. */
    while((d= (pos < (x*2) ? (maz[x*(y-2)+pos] ? 0 : 1) : (maz[pos - x - x ] ? 0 : 1))
	  /* If there is a wall two rows below us, bit 2 is 1. */
	  | (pos >= x * (y-2) ? (maz[pos - x*(y-2)] ? 0 : 2) : (maz[pos +x+x] ? 0 : 2))
	  /* If there is a wall two columns to the right, bit 3 is 1. */
	  | (pos % x >= x - 2 ? (maz[pos + 2 - x] ? 0 : 4) : (maz[pos + 2] ? 0 : 4))
	  /* If there is a wall two colums to the left, bit 4 is 1.  */
	  | ((pos % x <= 1 ) ? (maz[pos + x - 2] ? 0 : 8) : (maz[pos-2] ? 0 : 8)))) {

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
	     j = pos < x ? x*(y-1)+pos : pos - x;
	     npos = pos < (x*2) ? x*(y-2)+pos : pos - x - x;
	     break;
	case 1:
	     j = pos >= x*(y-1) ? pos - x * (y-1) : pos + x;
	     npos = pos >= x*(y-2) ? pos - x*(y-2) : pos + x + x; 
	     break;
	case 2:
	     j = (pos % x) == (x - 1) ? pos + 1 - x : pos + 1;
	     npos = (pos % x) >= (x - 2) ? pos + 2 - x : pos + 2;
	     break;
	case 3:
	     j= (pos % x) == 0 ? pos + x - 1 : pos - 1;
	     npos = (pos % x) <= 1 ? pos + x - 2 : pos - 2;
	     break;
	case 99:
	     return 1;  /* Hey neat, broken mazes! */
	     break;     /* (Umm... Wow... Yeah, neat.) */
	default:
	     break;
	}

	/* And punch a hole there. */
	maz[j] = 1;

        /* Now, start again just past where we punched the hole... */
	mazegen_tileable(npos, maz, x, y, rnd);

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
	g_warning("maze: get_colors: Indexed image.  Using colors 15 and 0.\n");
	fg[0] = 15;        /* As a plugin, I protest.  *I* shouldn't be the */
	bg[0] = 0;       /* one who has to deal with this colormapcrap.   */
	break;
    default:
      break;
    }
}
