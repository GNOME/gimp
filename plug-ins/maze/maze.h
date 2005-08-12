
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
