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

#ifndef __MAZE_H__
#define __MAZE_H__


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

#define PLUG_IN_PROC   "plug-in-maze"
#define PLUG_IN_BINARY "maze"
#define PLUG_IN_ROLE   "gimp-maze"

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

extern MazeValues mvals;
extern guint      sel_w;
extern guint      sel_h;
extern GRand     *gr;


#endif  /* __MAZE_H__ */
