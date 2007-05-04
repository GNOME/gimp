/* $Id$
 * This is a plug-in for the GIMP.
 * It draws mazes...
 *
 * Implemented as a GIMP 0.99 Plugin by
 * Kevin Turner <acapnotic@users.sourceforge.net>
 * http://gimp-plug-ins.sourceforge.net/maze/
 *
 * Code generously borrowed from assorted GIMP plugins
 * and used as a template to get me started on this one.  :)
 *
 * TO DO:
 *   maze_face.c: Rework the divboxes to be more like spinbuttons.
 *
 *   Maybe add an option to kill the outer border.
 *
 *   Fix that stray line down there between maze wall and dead space border...
 *
 *   handy.c: Make get_colors() work with indexed.  * HELP! *
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

#include "config.h"

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "maze.h"

#include "libgimp/stdplugins-intl.h"


static void      query     (void);
static void      run       (const gchar      *name,
                            gint              nparams,
                            const GimpParam  *param,
                            gint             *nreturn_vals,
                            GimpParam       **return_vals);

static void      maze      (GimpDrawable     *drawable);

static void      mask_maze (gint32  selection_ID,
			    guchar *maz,
			    guint   mw,
			    guint   mh,
			    gint    x1,
			    gint    x2,
			    gint    y1,
			    gint    y2,
			    gint    deadx,
			    gint    deady);

/* In maze_face.c */
extern gint      maze_dialog      (void);

/* In algorithms.c */
extern void      mazegen          (gint    pos,
				   guchar *maz,
				   gint    x,
				   gint    y,
				   gint    rnd);
extern void      mazegen_tileable (gint    pos,
				   guchar *maz,
				   gint    x,
				   gint    y,
				   gint    rnd);
extern void      prim             (guint   pos,
				   guchar *maz,
				   guint   x,
				   guint   y);
extern void      prim_tileable    (guchar *maz,
				   guint   x,
				   guint   y);

/* In handy.c */
extern void      get_colors (GimpDrawable *drawable,
			     guint8       *fg,
			     guint8       *bg);

extern void      drawbox    (GimpPixelRgn *dest_rgn,
			     guint         x,
			     guint         y,
			     guint         w,
			     guint         h,
			     guint8        clr[4]);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MazeValues mvals =
{
  5,           /* Passage width */
  5,           /* Passage height */
  0,           /* seed */
  FALSE,       /* Tileable? */
  57,          /* multiple * These two had "Experiment with this?" comments */
  1,           /* offset   * in the maz.c source, so, lets expiriment.  :) */
  DEPTH_FIRST, /* Algorithm */
  TRUE,        /* random_seed */
};

GRand *gr;

guint sel_w, sel_h;


MAIN ()


static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "(unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",  "ID of drawable" },
    /* If we did have parameters, these be them: */
    { GIMP_PDB_INT16,    "width",     "Width of the passages" },
    { GIMP_PDB_INT16,    "height",    "Height of the passages"},
    { GIMP_PDB_INT8,     "tileable",  "Tileable maze?"},
    { GIMP_PDB_INT8,     "algorithm", "Generation algorithm"
                                      "(0=DEPTH FIRST, 1=PRIM'S ALGORITHM)" },
    { GIMP_PDB_INT32,    "seed",      "Random Seed"},
    { GIMP_PDB_INT16,    "multiple",  "Multiple (use 57)" },
    { GIMP_PDB_INT16,    "offset",    "Offset (use 1)" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
			  N_("Draw a labyrinth"),
			  "Generates a maze using either the depth-first "
                          "search method or Prim's algorithm.  Can make "
                          "tileable mazes too.",
			  "Kevin Turner <kevint@poboxes.com>",
			  "Kevin Turner",
			  "1997, 1998",
			  N_("_Maze..."),
			  "RGB*, GRAY*, INDEXED*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC,
                             "<Image>/Filters/Render/Pattern");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint               x1, y1, x2, y2;

#ifdef MAZE_DEBUG
  g_print("maze PID: %d\n",getpid());
#endif
  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N ();

  gr = g_rand_new ();

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_PROC, &mvals);

      /* The interface needs to know the dimensions of the image... */
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
      sel_w = x2 - x1;
      sel_h = y2 - y1;

      /* Acquire info with a dialog */
      if (! maze_dialog ())
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 10)
        status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
	{
	  mvals.width     = (gint16)  param[3].data.d_int16;
	  mvals.height    = (gint16)  param[4].data.d_int16;
	  mvals.tile      = (gint8)   param[5].data.d_int8;
          mvals.algorithm = (gint8)   param[6].data.d_int8;
	  mvals.seed      = (guint32) param[7].data.d_int32;
	  mvals.multiple  = (gint16)  param[8].data.d_int16;
	  mvals.offset    = (gint16)  param[9].data.d_int16;

          if (mvals.random_seed)
            mvals.seed = g_random_int ();
	}
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_PROC, &mvals);

      if (mvals.random_seed)
        mvals.seed = g_random_int ();
      break;

    default:
      break;
  }

  /* color, gray, or indexed... hmm, miss anything?  ;)  */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id) ||
      gimp_drawable_is_indexed (drawable->drawable_id))
    {
      maze (drawable);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE ||
	  (run_mode == GIMP_RUN_WITH_LAST_VALS))
	gimp_set_data (PLUG_IN_PROC, &mvals, sizeof (MazeValues));
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  g_rand_free (gr);
  gimp_drawable_detach (drawable);
}

#ifdef MAZE_DEBUG
void
maze_dump (guchar *maz, gint mw, gint mh)
{
  short xx, yy;
  int   foo = 0;

  for (yy = 0; yy < mh; yy++)
    {
      for (xx = 0; xx < mw; xx++)
        g_print ("%3d ", maz[foo++]);
      g_print ("\n");
    }
}

void
maze_dumpX (guchar *maz, gint mw, gint mh)
{
  short xx, yy;
  int   foo = 0;

  for (yy = 0; yy < mh; yy++)
    {
      for (xx = 0; xx < mw; xx++)
        g_print ("%c", maz[foo++] ? 'X' : '.');
      g_print ("\n");
    }
}
#endif

static void
maze (GimpDrawable * drawable)
{
  GimpPixelRgn dest_rgn;
  guint        mw, mh;
  gint         deadx, deady;
  guint        cur_progress, max_progress;
  gdouble      per_progress;
  gint         x1, y1, x2, y2, x, y;
  gint         dx, dy, xx, yy;
  gint         maz_x, maz_xx, maz_row, maz_yy;
  guint8       fg[4], bg[4];
  gpointer     pr;
  gboolean     active_selection;

  guchar      *maz;
  guint        pos;

  /* Gets the input area... */
  active_selection = gimp_drawable_mask_bounds (drawable->drawable_id,
						&x1, &y1, &x2, &y2);

  /***************** Maze Stuff Happens Here ***************/

  mw = (x2-x1) / mvals.width;
  mh = (y2-y1) / mvals.height;

  if (!mvals.tile)
    {
      mw -= !(mw & 1); /* mazegen doesn't work with even-sized mazes. */
      mh -= !(mh & 1); /* Note I don't warn the user about this... */
    }
  else
    {
      /* On the other hand, tileable mazes must be even. */
      mw -= (mw & 1);
      mh -= (mh & 1);
    };

  /* It will really suck if your tileable maze ends up with this dead
     space around it.  Oh well, life is hard. */
  deadx = ((x2-x1) - mw * mvals.width)  / 2;
  deady = ((y2-y1) - mh * mvals.height) / 2;

  maz = g_new0 (guchar, mw * mh);

#ifdef MAZE_DEBUG
  printf("x:  %d\ty:  %d\nmw: %d\tmh: %d\ndx: %d\tdy: %d\nwidth:%d\theight: %d\n",
	 (x2 - x1), (y2 - y1), mw, mh, deadx, deady, mvals.width, mvals.height);
#endif

  /* Sanity check: */
  switch (mvals.algorithm)
    {
    case DEPTH_FIRST:
    case PRIMS_ALGORITHM:
      break;
    default:
      g_warning ("maze: Invalid algorithm choice %d", mvals.algorithm);
    }

  if (mvals.tile)
    {
      switch (mvals.algorithm)
        {
        case DEPTH_FIRST:
          mazegen_tileable (0, maz, mw, mh, mvals.seed);
          break;

        case PRIMS_ALGORITHM:
          prim_tileable (maz, mw, mh);
          break;

        default:
          break;
        }
    }
  else
    {
      /* not tileable */
      if (active_selection)
        {
          /* Mask and draw mazes until there's no
           * more room left. */
          mask_maze (drawable->drawable_id,
                     maz, mw, mh, x1, x2, y1, y2, deadx, deady);

          for (maz_yy = mw; maz_yy < (mh * mw); maz_yy += 2 * mw)
            {
              for (maz_xx = 1; maz_xx < mw; maz_xx += 2)
                {
                  if (maz[maz_yy + maz_xx] == 0)
                    {
                      switch (mvals.algorithm)
                        {
                        case DEPTH_FIRST:
                          mazegen (maz_yy+maz_xx, maz, mw, mh, mvals.seed);
                          break;

                        case PRIMS_ALGORITHM:
                          prim (maz_yy+maz_xx, maz, mw, mh);
                          break;

                        default:
                          break;
                        }
                    }
                }
	    }
        }
      else
        {
          /* No active selection. */
          pos = mw + 1;

          switch (mvals.algorithm)
            {
	    case DEPTH_FIRST:
              mazegen (pos, maz, mw, mh, mvals.seed);
              break;

	    case PRIMS_ALGORITHM:
              prim (pos, maz, mw, mh);
              break;

	    default:
              break;
	    }
        }
    }

  /************** Begin Drawing *********************/

  /* Initialize pixel region (?) */
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1),
                       TRUE, TRUE);

  cur_progress = 0;
  per_progress = 0.0;
  max_progress = (x2 - x1) * (y2 - y1) / 100;

  /* Get the foreground and background colors */
  get_colors (drawable, fg, bg);

  gimp_progress_init (_("Drawing maze"));

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
      maz_x = x/mvals.width;
      maz_row = mw * (y/mvals.height);

      /* Draws the upper-left [split] box */
      drawbox (&dest_rgn, 0, 0, dx, dy,
               (maz[maz_row + maz_x] == IN) ? fg : bg);

      maz_xx=maz_x + 1;
      /* Draw the top row [split] boxes */
      for (xx=dx; xx < dest_rgn.w; xx+=mvals.width)
        {
          drawbox (&dest_rgn, xx, 0, mvals.width, dy,
                   (maz[maz_row + maz_xx++] == IN) ? fg : bg);
        }

      maz_yy = maz_row + mw;
      /* Left column */
      for (yy = dy; yy < dest_rgn.h; yy += mvals.height)
        {
          drawbox (&dest_rgn, 0, yy, dx, mvals.height,
                   (maz[maz_yy + maz_x] == IN) ? fg : bg);
          maz_yy += mw;
        }

      maz_x++;
      /* Everything else */
      for (yy = dy; yy < dest_rgn.h; yy += mvals.height)
        {
          maz_xx = maz_x; maz_row+=mw;

          for (xx = dx; xx < dest_rgn.w; xx += mvals.width)
            {
              drawbox (&dest_rgn, xx, yy, mvals.width, mvals.height,
                       (maz[maz_row + maz_xx++] == IN) ? fg : bg);
            }
        }

      cur_progress += dest_rgn.w * dest_rgn.h;

      if (cur_progress > max_progress)
        {
          cur_progress = cur_progress - max_progress;
          per_progress = per_progress + 0.01;
          gimp_progress_update (per_progress);
        }
    }

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
}

/* Shaped mazes: */
/* With
 * Depth first: Nonzero cells will not be connected to or visited.
 * Prim's Algorithm:
 *  Cells that are not IN will not be connected to.
 *  Cells that are not OUT will not be converted to FRONTIER.
 *
 * So we'll put unavailable cells in a non-zero non-in non-out class
 * called MASKED.
 */

/* But first...  A little discussion about cells. */

/* In the eyes of the generation algorithms, the world is made up of
 * two sorts of things: Cells, and the walls between them.  Walls can
 * be knocked out, and then you have a passage between cells.  The
 * drawing routine has a simpler view of life...  Everything is a
 * pixel.  Or a block of pixels.  It makes no distinction between
 * passages, walls, and cells.
 *
 *  We may also make the distinction between two different types of
 * passages: horizontal and vertical.  With that in mind, a
 * part of the world looks something like this:
 *
 * @-@-@-@-  Where @ is a cell, | is a vertical passage, and - is a
 * | | | |   horizontal passsage.
 * @-@-@-@-
 * | | | |   Remember, the maze generation routines will not rest
 *          until the maze is full, that is, every cell is connected
 * to another.  Already, we can determine a few things about the final
 * maze.  We know which blocks will be cells, which blocks may become
 * passages (and we know what sort), and we also notice that there are
 * some blocks that will never be either cells or passages.
 *
 * Now, back to our masking routine...  To save a little time, lets
 * just take sample points from the block.  We'll sample a point from
 * the top and the bottom of vertical passages, left/right for
 * horizontal, and, hmm, left/right/top/bottom for cells.  And of
 * course, we needn't concern ourselves with the others.  We could
 * also sample the midpoint of each...
 * Then what we'll do is see if the average is higher than some magic
 * threshold number, and if so, we let maze happen there.  Otherwise
 * we mask it out.
 */

/* And, uh, that's on the TODO list.  Looks like I spent so much time
 * writing comments I haven't left enough to implement the code.  :)
 * Right now we only sample one point. */
static void
mask_maze (gint32 drawable_ID, guchar *maz, guint mw, guint mh,
           gint x1, gint x2, gint y1, gint y2, gint deadx, gint deady)
{
  gint32 selection_ID;
  GimpPixelRgn sel_rgn;
  gint xx0=0, yy0=0, xoff, yoff;
  guint xx=0, yy=0;
  guint foo=0;

  gint cur_row, cur_col;
  gint x1half, x2half, y1half, y2half;
  guchar *linebuf;

  if ((selection_ID =
       gimp_image_get_selection (gimp_drawable_get_image (drawable_ID))) == -1)
    return;

  gimp_pixel_rgn_init (&sel_rgn, gimp_drawable_get (selection_ID),
                       x1, y1, (x2-x1), (y2-y1),
                       FALSE, FALSE);
  gimp_drawable_offsets (drawable_ID, &xoff, &yoff);

  /* Special cases:  If mw or mh < 3 */
  /* FIXME (Currently works, but inefficiently.) */

  /* mw && mh => 3 */

  linebuf = g_new (guchar, sel_rgn.w * sel_rgn.bpp);

  xx0 = x1 + deadx + mvals.width  + xoff;
  yy0 = y1 + deady + mvals.height + yoff;

  x1half = mvals.width / 2;
  x2half = mvals.width - 1;

  y1half = mvals.height / 2;
  y2half = mvals.height - 1;

  /* Here, yy is with respect to the drawable (or something),
     whereas xx is with respect to the row buffer. */

  yy = yy0 + y1half;

  for (cur_row=1; cur_row < mh; cur_row += 2)
    {
      gimp_pixel_rgn_get_row (&sel_rgn, linebuf, x1+xoff, yy, (x2 - x1));

      cur_col = 1; xx = mvals.width;

      while (cur_col < mw)
        {
          /* Cell: */
          maz[cur_row * mw + cur_col] =
            (linebuf[xx] + linebuf[xx + x1half] + linebuf[xx+x2half]) / 5;

          cur_col += 1;
          xx += mvals.width;

          /* Passage: */
          if (cur_col < mw)
            maz[cur_row * mw + cur_col] =
              (linebuf[xx] + linebuf[xx + x1half] + linebuf[xx+x2half]) / 3;

          cur_col += 1;
          xx += mvals.width;

        } /* next col */

      yy += 2 * mvals.height;

    } /* next cur_row += 2 */

  /* Done doing horizontal scans, change this from a row buffer to
     a column buffer. */
  g_free (linebuf);
  linebuf = g_new (guchar, sel_rgn.h * sel_rgn.bpp);

  /* Now xx is with respect to the drawable (or whatever),
     and yy is with respect to the row buffer. */

  xx=xx0 + x1half;
  for (cur_col = 1; cur_col < mw; cur_col += 2)
    {
      gimp_pixel_rgn_get_col (&sel_rgn, linebuf, xx, y1, (y2-y1));

      cur_row = 1; yy = mvals.height;

      while (cur_row < mh)
        {
          /* Cell: */
          maz[cur_row * mw + cur_col] +=
            (linebuf[yy] + linebuf[yy+y2half]) / 5;

          cur_row += 1;
          yy += mvals.height;

          /* Passage: */
          if (cur_row < mh)
            maz[cur_row * mw + cur_col] =
              (linebuf[yy] + linebuf[yy + y1half] + linebuf[yy+y2half]) / 3;

          cur_row += 1;
          yy += mvals.height;
        } /* next cur_row */

      xx += 2 * mvals.width;

    } /* next cur_col */

  g_free (linebuf);

  /* Do the alpha -> masked conversion. */

  for (yy = 0; yy < mh; yy++)
    {
      for (xx = 0; xx < mw; xx++)
        {
          maz[foo] = ( maz[foo] < MAZE_ALPHA_THRESHOLD ) ? MASKED : OUT;
          foo++;
        }
    }
}


/* The attempt to implement this with tiles: (it wasn't fun) */

#if 0

{
  /* Tiles make my life decidedly difficult here.  There are too
   * many special cases...  "What if a tile starts less/more than
   * halfway through a block?  What if we get a narrow edge-tile
   * that..." etc, etc.  I shall investigate other options.
   * Possibly a row buffer, or can we use something other than this
   * black-magic gimp_pixel_rgns_register call to get tiles of
   * different sizes?  Now that'd be nice...  */

  for (pr = gimp_pixel_rgns_register (1, &sel_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      /* This gives us coordinates relative to the starting point
       * of the maze grid.  Negative values happen here if there
       * is dead space. */
      x = sel_rgn.x - x1 - deadx;
      y = sel_rgn.y - y1 - deady;

      /* These coordinates are relative to the current tile. */
      /* This starts us off at the first block boundary in the
       * tile. */

      /* e.g. with x=16 and width=10.
       * 16 % 10 = 6
       * 10 - 6 = 4

  x: 6789!123456789!123456789!12
     ....|.........|.........|..
 xx: 0123456789!123456789!123456

 So to start on the boundary, begin at 4.

 For the case x=0, 10-0=10. So xx0 will always between 1 and width. */

      xx0 = mvals.width  - (x % mvals.width);
      yy0 = mvals.height - (y % mvals.height);

      /* Find the corresponding row and column in the maze. */
      maz_x = (x+xx0)/mvals.width;
      maz_row = mw * ((y + yy0)/mvals.height);

      for (yy = yy0 * sel_rgn.rowstride;
           yy < sel_rgn.h * sel_rgn.rowstride;
           yy += (mvals.height * sel_rgn.rowstride))
        {
          maz_xx = maz_x;
          for (xx = xx0 * sel_rgn.bpp;
               xx < sel_rgn.w;
               xx += mvals.width * sel_rgn.bpp)
            {
              if (sel_rgn.data[yy+xx] < MAZE_ALPHA_THRESHOLD)
                maz[maz_row+maz_xx]=MASKED;
              maz_xx++;
            }

          maz_row += mw;
        }

    } /* next pr sel_rgn tile thing */
#ifdef MAZE_DEBUG
  /* maze_dump(maz,mw,mh); */
#endif
}

#endif /* 0 */
