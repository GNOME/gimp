#include "FractalExplorer.h"

/**********************************************************************
  Global variables  
 *********************************************************************/

double              xmin = -2,
                    xmax = 1,
                    ymin = -1.5,
                    ymax = 1.5;
double              xbild,
                    ybild,
                    xdiff,
                    ydiff;
double              x_press = -1.0,
                    y_press = -1.0;
double              x_release = -1.0,
                    y_release = -1.0;
float               cx = -0.75;
float               cy = -0.2;
GDrawable          *drawable;
gint                tile_width,
                    tile_height;
gint                img_width,
                    img_height,
                    img_bpp;
gint                sel_x1,
                    sel_y1,
                    sel_x2,
                    sel_y2;
gint                sel_width,
                    sel_height;
gint                preview_width,
                    preview_height;
GTile              *the_tile = NULL;
double              cen_x,
                    cen_y;
double              xpos,
                    ypos,
                    oldxpos = -1,
                    oldypos = -1;
GtkWidget          *maindlg;
GtkWidget          *logodlg;
GtkWidget          *cmap_preview;
GtkWidget          *delete_frame_to_freeze;
GtkWidget          *fractalexplorer_gtk_list;
GtkWidget          *save_menu_item;
GtkWidget          *fractalexplorer_op_menu;
GdkCursor          *MyCursor;
int                 ready_now = FALSE;
explorer_vals_t     zooms[100];
DialogElements     *elements = NULL;
int                 zoomindex = 1;
int                 zoommax = 1;
gdouble            *gg;
int                 line_no;
gchar              *filename;
clrmap              colormap;
GList		   *fractalexplorer_path_list = NULL;
GList		   *fractalexplorer_list = NULL;
GList		   *gradient_list = NULL;
gchar 		   *tpath = NULL;
fractalexplorerOBJ *fractalexplorer_obj_for_menu;
GList              *rescan_list = NULL;


explorer_interface_t wint =
{
    NULL,			/* preview */
    NULL,			/* wimage */
    FALSE			/* run */
};				/* wint */

explorer_vals_t wvals =
{
  0,
  -2.0,
  2.0,
  -1.5,
  1.5,
  50.0,
  -0.75,
  -0.2,
  0,
  128.0,
  128.0,
  128.0,
  1,
  1,
  0,
  0,
  0,
  0,
  1,
  256,
  0
};				/* wvals */

explorer_vals_t standardvals =
{
  0,
  -2.0,
  2.0,
  -1.5,
  1.5,
  50.0,
  -0.75,
  -0.2,
  0,
  128.0,
  128.0,
  128.0,
  1,
  1,
  0,
  0,
  0,
  0,
  1,
  256,
  0
};				/* standardvals */


fractalexplorerOBJ *current_obj;
fractalexplorerOBJ *pic_obj;
GtkWidget *delete_dialog = NULL;
