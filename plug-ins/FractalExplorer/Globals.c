#include "FractalExplorer.h"
#include "Languages.h"

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
gint                do_redsinus,
                    do_redcosinus,
                    do_rednone;
gint                do_greensinus,
                    do_greencosinus,
                    do_greennone;
gint                do_bluesinus,
                    do_bluecosinus,
                    do_bluenone;
gint                do_redinvert,
                    do_greeninvert,
		    do_blueinvert;
gint                do_colormode1 = FALSE,
                    do_colormode2 = FALSE;
gint                do_type0 = FALSE,
                    do_type1 = FALSE,
		    do_type2 = FALSE,
                    do_type3 = FALSE,
                    do_type4 = FALSE,
                    do_type5 = FALSE,
                    do_type6 = FALSE,
                    do_type7 = FALSE,
                    do_type8 = FALSE,
                    do_english = TRUE,
                    do_french = FALSE,
                    do_german = FALSE;
GtkWidget          *maindlg;
GtkWidget          *logodlg;
GtkWidget          *loaddlg;
GtkWidget          *cmap_preview;
GtkWidget          *cmap_preview_long;
GtkWidget          *cmap_preview_long2;
GtkWidget          *delete_frame_to_freeze;
GtkWidget          *fractalexplorer_gtk_list;
GtkWidget          *save_menu_item;
GtkWidget          *fractalexplorer_op_menu;
GtkTooltips        *tips;
GdkColor            tips_fg,
                    tips_bg;
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
int 		    lng=LNG_GERMAN;


GPlugInInfo         PLUG_IN_INFO =
{
    NULL,			/* init_proc */
    NULL,			/* quit_proc */
    query,			/* query_proc */
    run,			/* run_proc */
};

explorer_interface_t wint =
{
    NULL,			/* preview */
    NULL,			/* wimage */
    FALSE			/* run */
};				/* wint */

explorer_vals_t wvals =
{
    0, -2.0, 2.0, -1.5, 1.5, 50.0, -0.75, -0.2, 0, 128.0, 128.0, 128.0, 1, 1, 0, 0, 0, 0, 1, 0,
};				/* wvals */

explorer_vals_t standardvals =
{
    0, -2.0, 2.0, -1.5, 1.5, 50.0, -0.75, -0.2, 0, 128.0, 128.0, 128.0, 1, 1, 0, 0, 0, 0, 1, 0,
};				/* standardvals */


fractalexplorerOBJ *current_obj;
fractalexplorerOBJ *pic_obj;
GtkWidget *delete_dialog = NULL;
