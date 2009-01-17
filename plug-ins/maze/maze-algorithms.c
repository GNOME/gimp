/* $Id$
 * Contains routines for generating mazes, somewhat intertwined with
 * Gimp plug-in-maze specific stuff.
 *
 * Kevin Turner <acapnotic@users.sourceforge.net>
 * http://gimp-plug-ins.sourceforge.net/maze/
 */

/* mazegen code from rec.games.programmer's maze-faq:
 * * maz.c - generate a maze
 * *
 * * algorithm posted to rec.games.programmer by jallen@ic.sunysb.edu
 * * program cleaned and reorganized by mzraly@ldbvax.dnet.lotus.com
 * *
 * * don't make people pay for this, or I'll jump up and down and
 * * yell and scream and embarass you in front of your friends...
 */

/* I've put a HTMLized version of the FAQ up at
 * http://www.poboxes.com/kevint/gimp/maze-faq/maze-faq.html
 */

/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <stdlib.h>

#include "libgimp/gimp.h"

#include "maze.h"
#include "maze-algorithms.h"

#include "libgimp/stdplugins-intl.h"


#define ABSMOD(A,B) (((A) < 0) ? (((B) + (A)) % (B)) : ((A) % (B)))

/* Since we are using a 1D array on 2D space, we need to do our own
   calculations.  (Ok, so there are ways of doing dynamically allocated
   2D arrays, but we started this way, so let's stick with it. */

/* The difference between CELL_* and WALL_* is that cell moves two spaces,
   while wall moves one. */

/* Macros assume that x and y will be defined where they are used. */
/* A return of -1 means "no such place, don't go there". */
#define CELL_UP(POS) ((POS) < (x*2) ? -1 : (POS) - x - x)
#define CELL_DOWN(POS) ((POS) >= x*(y-2) ? -1 : (POS) + x + x)
#define CELL_LEFT(POS) (((POS) % x) <= 1 ? -1 : (POS) - 2)
#define CELL_RIGHT(POS) (((POS) % x) >= (x - 2) ? -1 : (POS) + 2)

/* With walls, we don't need to check for boundaries, since we are
   assured that there *is* a valid cell on the other side of the
   wall. */
#define WALL_UP(POS) ((POS) - x)
#define WALL_DOWN(POS) ((POS) + x)
#define WALL_LEFT(POS) ((POS) - 1)
#define WALL_RIGHT(POS) ((POS) + 1)

/***** For tileable mazes *****/

#define CELL_UP_TILEABLE(POS) ((POS) < (x*2) ? x*(y-2)+(POS) : (POS) - x - x)
#define CELL_DOWN_TILEABLE(POS) ((POS) >= x*(y-2) ? (POS) - x*(y-2) : (POS) + x + x)
#define CELL_LEFT_TILEABLE(POS) (((POS) % x) <= 1 ? (POS) + x - 2 : (POS) - 2)
#define CELL_RIGHT_TILEABLE(POS) (((POS) % x) >= (x - 2) ? (POS) + 2 - x : (POS) + 2)
/* Up and left need checks, but down and right should never have to
   wrap on an even sized maze. */
#define WALL_UP_TILEABLE(POS) ((POS) < x ? x*(y-1)+(POS) : (POS) - x)
#define WALL_DOWN_TILEABLE(POS) ((POS) + x)
#define WALL_LEFT_TILEABLE(POS) (((POS) % x) == 0 ? (POS) + x - 1 : (POS) - 1)
#define WALL_RIGHT_TILEABLE(POS) ((POS) + 1)

/* Down and right with checks.
#define WALL_DOWN_TILEABLE(POS) ((POS) >= x*(y-1) ? (POS) - x * (y-1) : (POS) + x)
#define WALL_RIGHT_TILEABLE(POS) (((POS) % x) == (x - 1) ? (POS) + 1 - x : (POS) + 1)
*/

/* The Incredible Recursive Maze Generation Routine */
/* Ripped from rec.programmers.games maze-faq       */
/* Modified and commented by me, Kevin Turner. */
void
mazegen (gint    pos,
         guchar *maz,
         gint    x,
         gint    y,
         gint    rnd)
{
  gchar d, i;
  gint  c = 0;
  gint  j = 1;

  /* Punch a hole here...  */
  maz[pos] = IN;

  /* If there is a wall two rows above us, bit 1 is 1. */
  while ((d= (pos <= (x * 2) ? 0 : (maz[pos - x - x ] ? 0 : 1))
          /* If there is a wall two rows below us, bit 2 is 1. */
          | (pos >= x * (y - 2) ? 0 : (maz[pos + x + x] ? 0 : 2))
          /* If there is a wall two columns to the right, bit 3 is 1. */
          | (pos % x == x - 2 ? 0 : (maz[pos + 2] ? 0 : 4))
          /* If there is a wall two colums to the left, bit 4 is 1.  */
          | ((pos % x == 1 ) ? 0 : (maz[pos-2] ? 0 : 8))))
    {
      /* Note if all bits are 0, d is false, we don't do this
         while loop, we don't call ourselves again, so this branch
         is done.  */

      /* I see what this loop does (more or less), but I don't know
         _why_ it does it this way...  I also haven't figured out exactly
         which values of multiple will work and which won't.  */
      do
        {
          rnd = (rnd * mvals.multiple + mvals.offset);
          i = 3 & (rnd / d);
          if (++c > 100)
            {  /* Break and try to salvage something */
              i=99;         /* if it looks like we're going to be */
              break;        /* here forever...                    */
            }
        }
      while (!(d & (1 << i)));
      /* ...While there's *not* a wall in direction i. */
      /* (stop looping when there is) */

      switch (i)
        {
        case 0:       /* Go in the direction we just figured . . . */
          j = -x;
          break;
        case 1:
          j = x;
          break;
        case 2:
          j = 1;
          break;
        case 3:
          j = -1;
          break;
        case 99:
          return;  /* Hey neat, broken mazes! */
          break;     /* (Umm... Wow... Yeah, neat.) */
        default:
          g_warning ("maze: mazegen: Going in unknown direction.\n"
                     "i: %d, d: %d, seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     i, d,mvals.seed, x, y, mvals.multiple, mvals.offset);
          break;
        }

      /* And punch a hole there. */
      maz[pos + j] = 1;

      /* Now, start again just past where we punched the hole... */
      mazegen (pos + 2 * j, maz, x, y, rnd);

    }

  return;
}

/* Tileable mazes are my creation, based on the routine above. */
void
mazegen_tileable (gint    pos,
                  guchar *maz,
                  gint    x,
                  gint    y,
                  gint    rnd)
{
  gchar d, i;
  gint  c = 0;
  gint  npos = 2;

  /* Punch a hole here...  */
  maz[pos] = IN;

  /* If there is a wall two rows above us, bit 1 is 1. */
  while ((d= (maz[CELL_UP_TILEABLE(pos)] ? 0 : 1)
	  /* If there is a wall two rows below us, bit 2 is 1. */
	  | (maz[CELL_DOWN_TILEABLE(pos)] ? 0 : 2)
	  /* If there is a wall two columns to the right, bit 3 is 1. */
	  | (maz[CELL_RIGHT_TILEABLE(pos)] ? 0 : 4)
	  /* If there is a wall two colums to the left, bit 4 is 1.  */
	  | (maz[CELL_LEFT_TILEABLE(pos)] ? 0 : 8)))
    {
      /* Note if all bits are 0, d is false, we don't do this
         while loop, we don't call ourselves again, so this branch
         is done.  */

      /* I see what this loop does (more or less), but I don't know
         _why_ it does it this way...  I also haven't figured out exactly
         which values of multiple will work and which won't.  */
      do
        {
          rnd = (rnd * mvals.multiple + mvals.offset);
          i = 3 & (rnd / d);
          if (++c > 100)
            {  /* Break and try to salvage something */
              i=99;         /* if it looks like we're going to be */
              break;        /* here forever...                    */
	    }
	}
      while (!(d & (1 << i)));
      /* ...While there's *not* a wall in direction i. */
      /* (stop looping when there is) */

      switch (i)
        {
	case 0:       /* Go in the direction we just figured . . . */
          maz[WALL_UP_TILEABLE (pos)] = IN;
          npos = CELL_UP_TILEABLE (pos);
          break;
	case 1:
          maz[WALL_DOWN_TILEABLE (pos)] = IN;
          npos = CELL_DOWN_TILEABLE (pos);
          break;
	case 2:
          maz[WALL_RIGHT_TILEABLE (pos)] = IN;
          npos = CELL_RIGHT_TILEABLE (pos);
          break;
	case 3:
          maz[WALL_LEFT_TILEABLE (pos)] = IN;
          npos = CELL_LEFT_TILEABLE (pos);
          break;
	case 99:
          return;  /* Hey neat, broken mazes! */
          break;     /* (Umm... Wow... Yeah, neat.) */
	default:
          g_warning ("maze: mazegen_tileable: Going in unknown direction.\n"
                     "i: %d, d: %d, seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     i, d,mvals.seed, x, y, mvals.multiple, mvals.offset);
          break;
	}

      /* Now, start again just past where we punched the hole... */
      mazegen_tileable (npos, maz, x, y, rnd);
    }

  return;
}

/* This function (as well as prim_tileable) make use of the somewhat
   unclean practice of storing ints as pointers.  I've been informed
   that this may cause problems with 64-bit stuff.  However, hopefully
   it will be okay, since the only values stored are positive.  If it
   does break, let me know, and I'll go cry in a corner for a while
   before I get up the strength to re-code it. */
void
prim (gint    pos,
      guchar *maz,
      guint   x,
      guint   y)
{
  GSList *front_cells = NULL;
  guint   current;
  gint    up, down, left, right; /* Not unsigned, because macros return -1. */
  guint   progress = 0;
  guint   max_progress;
  char    d, i;
  guint   c = 0;
  gint    rnd = mvals.seed;

  g_rand_set_seed (gr, rnd);

  gimp_progress_init (_("Constructing maze using Prim's Algorithm"));

  /* OUT is zero, so we should be already initalized. */

  max_progress = x * y / 4;

  /* Starting position has already been determined by the calling function. */

  maz[pos] = IN;

  /* For now, repeating everything four times seems manageable.  But when
     Gimp is extended to drawings in n-dimensional space instead of 2D,
     this will require a bit of a re-write. */
  /* Add frontier. */
  up    = CELL_UP (pos);
  down  = CELL_DOWN (pos);
  left  = CELL_LEFT (pos);
  right = CELL_RIGHT (pos);

  if (up >= 0)
    {
      maz[up] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (up));
    }

  if (down >= 0)
    {
      maz[down] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (down));
    }

  if (left >= 0)
    {
      maz[left] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (left));
    }

  if (right >= 0)
    {
      maz[right] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (right));
    }

  /* While frontier is not empty do the following... */
  while (g_slist_length (front_cells) > 0)
    {
      /* Remove one cell at random from frontier and place it in IN. */
      current = g_rand_int_range (gr, 0, g_slist_length (front_cells));
      pos = GPOINTER_TO_INT (g_slist_nth (front_cells, current)->data);

      front_cells = g_slist_remove (front_cells, GINT_TO_POINTER (pos));
      maz[pos] = IN;

      /* If the cell has any neighbors in OUT, remove them from
         OUT and place them in FRONTIER. */

      up    = CELL_UP (pos);
      down  = CELL_DOWN (pos);
      left  = CELL_LEFT (pos);
      right = CELL_RIGHT (pos);

      d = 0;
      if (up >= 0)
        {
          switch (maz[up])
            {
            case OUT:
              maz[up] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (up));
              break;

            case IN:
              d = 1;
              break;

            default:
              break;
            }
        }

      if (down >= 0)
        {
          switch (maz[down])
            {
            case OUT:
              maz[down] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (down));
              break;

            case IN:
              d = d | 2;
              break;

            default:
              break;
            }
        }

      if (left >= 0)
        {
          switch (maz[left])
            {
            case OUT:
              maz[left] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (left));
              break;

            case IN:
              d = d | 4;
              break;

            default:
              break;
            }
        }

      if (right >= 0)
        {
          switch (maz[right])
            {
            case OUT:
              maz[right] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (right));
              break;

            case IN:
              d = d | 8;
              break;

            default:
              break;
            }
        }

      /* The cell is guaranteed to have at least one neighbor in
         IN (otherwise it would not have been in FRONTIER); pick
         one such neighbor at random and connect it to the new
         cell (ie knock out a wall).  */

      if (!d)
        {
          g_warning ("maze: prim: Lack of neighbors.\n"
                     "seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     mvals.seed, x, y, mvals.multiple, mvals.offset);
          break;
        }

      c = 0;
      do
        {
          rnd = (rnd * mvals.multiple + mvals.offset);
          i = 3 & (rnd / d);
          if (++c > 100)
            {  /* Break and try to salvage something */
              i = 99;         /* if it looks like we're going to be */
              break;        /* here forever...                    */
            }
        }
      while (!(d & (1 << i)));

      switch (i)
        {
        case 0:
          maz[WALL_UP (pos)] = IN;
          break;
        case 1:
          maz[WALL_DOWN (pos)] = IN;
          break;
        case 2:
          maz[WALL_LEFT (pos)] = IN;
          break;
        case 3:
          maz[WALL_RIGHT (pos)] = IN;
          break;
        case 99:
          break;
        default:
          g_warning ("maze: prim: Going in unknown direction.\n"
                     "i: %d, d: %d, seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     i, d, mvals.seed, x, y, mvals.multiple, mvals.offset);
        }

      if (progress++ % PRIMS_PROGRESS_UPDATE)
        gimp_progress_update ((double) progress / (double) max_progress);
    }

  g_slist_free (front_cells);
}

void
prim_tileable (guchar *maz,
               guint   x,
               guint   y)
{
  GSList *front_cells=NULL;
  guint   current, pos;
  guint   up, down, left, right;
  guint   progress = 0;
  guint   max_progress;
  char    d, i;
  guint   c = 0;
  gint    rnd = mvals.seed;

  g_rand_set_seed (gr, rnd);

  gimp_progress_init (_("Constructing tileable maze using Prim's Algorithm"));

  /* OUT is zero, so we should be already initalized. */

  max_progress = x * y / 4;

  /* Pick someplace to start. */

  pos = x * 2 * g_rand_int_range (gr, 0, y/2) + 2 * g_rand_int_range(gr, 0, x/2);

  maz[pos] = IN;

  /* Add frontier. */
  up    = CELL_UP_TILEABLE (pos);
  down  = CELL_DOWN_TILEABLE (pos);
  left  = CELL_LEFT_TILEABLE (pos);
  right = CELL_RIGHT_TILEABLE (pos);

  maz[up] = maz[down] = maz[left] = maz[right] = FRONTIER;

  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (up));
  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (down));
  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (left));
  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (right));

  /* While frontier is not empty do the following... */
  while (g_slist_length (front_cells) > 0)
    {
      /* Remove one cell at random from frontier and place it in IN. */
      current = g_rand_int_range (gr, 0, g_slist_length (front_cells));
      pos = GPOINTER_TO_UINT (g_slist_nth (front_cells, current)->data);

      front_cells = g_slist_remove (front_cells, GUINT_TO_POINTER (pos));
      maz[pos] = IN;

      /* If the cell has any neighbors in OUT, remove them from
         OUT and place them in FRONTIER. */

      up    = CELL_UP_TILEABLE (pos);
      down  = CELL_DOWN_TILEABLE (pos);
      left  = CELL_LEFT_TILEABLE (pos);
      right = CELL_RIGHT_TILEABLE (pos);

      d = 0;
      switch (maz[up])
        {
        case OUT:
          maz[up] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (up));
          break;

        case IN:
          d = 1;
          break;

        default:
          break;
        }

      switch (maz[down])
        {
        case OUT:
          maz[down] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (down));
          break;

        case IN:
          d = d | 2;
          break;

        default:
          break;
        }

      switch (maz[left])
        {
        case OUT:
          maz[left] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (left));
          break;

        case IN:
          d = d | 4;
          break;

        default:
          break;
        }

      switch (maz[right])
        {
        case OUT:
          maz[right] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (right));
          break;

        case IN:
          d = d | 8;
          break;

        default:
          break;
        }

      /* The cell is guaranteed to have at least one neighbor in
         IN (otherwise it would not have been in FRONTIER); pick
         one such neighbor at random and connect it to the new
         cell (ie knock out a wall).  */

      if (!d)
        {
          g_warning ("maze: prim's tileable: Lack of neighbors.\n"
                     "seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     mvals.seed, x, y, mvals.multiple, mvals.offset);
          break;
        }

      c = 0;
      do
        {
          rnd = (rnd * mvals.multiple + mvals.offset);
          i = 3 & (rnd / d);
          if (++c > 100)
            {  /* Break and try to salvage something */
              i = 99;         /* if it looks like we're going to be */
              break;        /* here forever...                    */
            }
        }
      while (!(d & (1 << i)));

      switch (i)
        {
        case 0:
          maz[WALL_UP_TILEABLE (pos)] = IN;
          break;
        case 1:
          maz[WALL_DOWN_TILEABLE (pos)] = IN;
          break;
        case 2:
          maz[WALL_LEFT_TILEABLE (pos)] = IN;
          break;
        case 3:
          maz[WALL_RIGHT_TILEABLE (pos)] = IN;
          break;
        case 99:
          break;
        default:
          g_warning ("maze: prim's tileable: Going in unknown direction.\n"
                     "i: %d, d: %d, seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     i, d, mvals.seed, x, y, mvals.multiple, mvals.offset);
        }

      if (progress++ % PRIMS_PROGRESS_UPDATE)
        gimp_progress_update ((double) progress / (double) max_progress);
    }

  g_slist_free (front_cells);
}
