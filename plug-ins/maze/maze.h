#define MAZE_TITLE "Maze 0.6.1"

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
