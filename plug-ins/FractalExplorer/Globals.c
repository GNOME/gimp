#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "FractalExplorer.h"

/**********************************************************************
  Global variables  
 *********************************************************************/

gdouble             xmin = -2;
gdouble             xmax = 1;
gdouble             ymin = -1.5;
gdouble             ymax = 1.5;
gdouble             xbild;
gdouble             ybild;
gdouble             xdiff;
gdouble             ydiff;
gdouble             x_press = -1.0;
gdouble             y_press = -1.0;
gdouble             x_release = -1.0;
gdouble             y_release = -1.0;
gfloat              cx = -0.75;
gfloat              cy = -0.2;
GimpDrawable          *drawable;
gint                tile_width;
gint                tile_height;
gint                img_width;
gint                img_height;
gint                img_bpp;
gint                sel_x1;
gint                sel_y1;
gint                sel_x2;
gint                sel_y2;
gint                sel_width;
gint                sel_height;
gint                preview_width;
gint                preview_height;
GimpTile           *the_tile = NULL;
gdouble             cen_x;
gdouble             cen_y;
gdouble             xpos;
gdouble             ypos;
gdouble             oldxpos = -1;
gdouble             oldypos = -1;
GtkWidget          *maindlg;
GtkWidget          *logodlg;
GtkWidget          *cmap_preview;
GtkWidget          *delete_frame_to_freeze;
GtkWidget          *fractalexplorer_gtk_list;
GtkWidget          *save_menu_item;
GtkWidget          *fractalexplorer_op_menu;
GdkCursor          *MyCursor;
gboolean            ready_now = FALSE;
explorer_vals_t     zooms[100];
DialogElements     *elements = NULL;
gint                zoomindex = 1;
gint                zoommax = 1;
gdouble            *gg;
gint                line_no;
gchar              *filename;
clrmap              colormap;
gchar		   *fractalexplorer_path = NULL;
GList		   *fractalexplorer_list = NULL;
GList		   *gradient_list        = NULL;
gchar 		   *tpath                = NULL;
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
  1.0,
  1.0,
  1.0,
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
  1.0,
  1.0,
  1.0,
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
