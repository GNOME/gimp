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

#ifndef __MAZE_H__
#define __MAZE_H__


#define MAZE_TITLE N_("Maze")

#define HELP_OPENS_NEW_WINDOW FALSE

/* The "divbox" really should look and act more like a spinbutton.
  This flag is a small step in the direction toward the former, the
  latter leaves much to be desired. */
#define DIVBOX_LOOKS_LIKE_SPINBUTTON FALSE

/* Don't update the progress for every cell when creating a maze.
   Instead, update every . . . */
#define PRIMS_PROGRESS_UPDATE 256

/* Don't draw in anything that has less than
   this value in the selection channel. */
#define MAZE_ALPHA_THRESHOLD 127

#include "glib.h"

#define PLUG_IN_PROC   "plug-in-maze"
#define PLUG_IN_BINARY "maze"

typedef enum {
     DEPTH_FIRST,
     PRIMS_ALGORITHM
} MazeAlgoType;

typedef struct {
     gint width;
     gint height;
     guint32 seed;
     gboolean tile;
     gint multiple;
     gint offset;
     MazeAlgoType algorithm;
     gboolean random_seed;
     /* Interface options. */
} MazeValues;

enum CellTypes {
     OUT,
     IN,
     FRONTIER,
     MASKED
};


void  get_colors (GimpDrawable *drawable,
                  guint8       *fg,
                  guint8       *bg);
void  drawbox    (GimpPixelRgn *dest_rgn,
                  guint         x,
                  guint         y,
                  guint         w,
                  guint         h,
                  guint8        clr[4]);


#endif  /* __MAZE_H__ */
