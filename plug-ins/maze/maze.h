#define MAZE_TITLE "Maze 0.6.2"
#define MAZE_URL "http://www.poboxes.com/kevint/gimp/maze.html"

#define HELP_OPENS_NEW_WINDOW FALSE

/* The "divbox" really should look and act more like a spinbutton.
  This flag is a small step in the direction toward the former, the
  latter leaves much to be desired. */
#define DIVBOX_LOOKS_LIKE_SPINBUTTON FALSE

#include "gtk/gtk.h"

typedef struct {
     gint width;
     gint height;
     gint seed;
     gboolean tile;
     gint multiple;
     gint offset;
     /* Interface options. */
     gboolean timeseed;
} MazeValues;
