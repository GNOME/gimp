#ifndef __FRACTALEXPLORER_H__
#define __FRACTALEXPLORER_H__

#include "config.h"
#include <glib.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/**********************************************************************
 Magic numbers  
 *********************************************************************/

#define PREVIEW_SIZE 128
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  60
#define MAX_LOAD_LINE 256
#define GR_WIDTH 325

#define NCOLORS 256
#define MAXSTRLEN 256

#define SINUS 0
#define COSINUS 1
#define NONE 2

#define FRACTAL_HEADER "Fractal Explorer Plug-In Version 2 - (c) 1997 <cotting@mygale.org>\n"
#define fractalexplorer_HEADER "Fractal Explorer Plug-In Version 2 - (c) 1997 <cotting@mygale.org>\n"

/**********************************************************************
 Types  
 *********************************************************************/

typedef struct {
    gint                fractaltype;
    gdouble             xmin;
    gdouble             xmax;
    gdouble             ymin;
    gdouble             ymax;
    gdouble             iter;
    gdouble             cx;
    gdouble             cy;
    gint                colormode;
    gdouble             redstretch;
    gdouble             greenstretch;
    gdouble             bluestretch;
    gint                redmode;
    gint                greenmode;
    gint                bluemode;
    gint 		redinvert;
    gint 		greeninvert;
    gint 		blueinvert;
    gint                alwayspreview;
    gint 		language;
    
} explorer_vals_t;

typedef struct {
    GtkWidget          *preview;
    guchar             *wimage;
    gint                run;
} explorer_interface_t;

typedef int         colorvalue[3];

typedef colorvalue  clrmap[NCOLORS];

typedef struct {
    GtkWidget          *text;
    GtkAdjustment      *data;
} scaledata;

typedef struct _DialogElements DialogElements;


struct _DialogElements {
    GtkWidget          *type_mandelbrot;
    GtkWidget          *type_julia;
    GtkWidget          *type_barnsley1;
    GtkWidget          *type_barnsley2;
    GtkWidget          *type_barnsley3;
    GtkWidget          *type_spider;
    GtkWidget          *type_manowar;
    GtkWidget          *type_lambda;
    GtkWidget          *type_sierpinski;
    scaledata           xmin;
    scaledata           xmax;
    scaledata           ymin;
    scaledata           ymax;
    scaledata           iter;
    scaledata           cx;
    scaledata           cy;
    scaledata           red;
    scaledata           green;
    scaledata           blue;
    GtkWidget          *redmodecos;
    GtkWidget          *redmodesin;
    GtkWidget          *redmodenone;
    GtkWidget          *greenmodecos;
    GtkWidget          *greenmodesin;
    GtkWidget          *greenmodenone;
    GtkWidget          *bluemodecos;
    GtkWidget          *bluemodesin;
    GtkWidget          *bluemodenone;
    GtkWidget          *redinvert;
    GtkWidget          *greeninvert;
    GtkWidget          *blueinvert;
    GtkWidget          *colormode0;
    GtkWidget          *colormode1;
};


typedef struct DFigObj {
  gchar * name;     /* Trailing name of file  */
  gchar * filename; /* Filename itself */
  gchar * draw_name;/* Name of the drawing */
  explorer_vals_t opts;    /* Options enforced when fig saved */
  GtkWidget *list_item;
  GtkWidget *label_widget;
  GtkWidget *pixmap_widget;
  gint obj_status;
} fractalexplorerOBJ;  


typedef struct GigObj {
  gchar * name;     /* Trailing name of file  */
  gchar * filename; /* Filename itself */
  gchar * draw_name;/* Name of the drawing */
  gint  typus;
  GtkWidget *list_item;
  GtkWidget *label_widget;
  GtkWidget *pixmap_widget;
  gint obj_status;
} gradientOBJ;  

typedef struct _fractalexplorerListOptions {
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GtkWidget *list_entry;
  fractalexplorerOBJ * obj;
  gint created;
} fractalexplorerListOptions;

/* States of the object */
#define fractalexplorer_OK       0x0
#define fractalexplorer_MODIFIED 0x1
#define fractalexplorer_READONLY 0x2

#define gradient_GRADIENTEDITOR 0x2

extern fractalexplorerOBJ *current_obj;
extern fractalexplorerOBJ *pic_obj;
extern GtkWidget *delete_dialog;

/**********************************************************************
 Declare local functions
 *********************************************************************/

/* Gimp interface functions */
void         query(void);
void         run(char *name, int nparams, GParam * param, int *nreturn_vals,
			GParam ** return_vals);

/* Dialog and fractal functions */
void                explorer(GDrawable * drawable);
void                explorer_render_row(const guchar * src_row, guchar * dest_row, gint row,
					gint row_width, gint bytes);
void                transform(short int *, short int *, short int *, double, double, double);

/* Functions for dialog widgets */

gint      list_button_press(GtkWidget *widget,GdkEventButton *event,gpointer   data);
gint      new_button_press(GtkWidget *widget,GdkEventButton *bevent,gpointer   data);
gint      fractalexplorer_delete_fractalexplorer_callback(GtkWidget *widget,GdkEventButton *bevent,gpointer   data);
gint      delete_button_press_ok(GtkWidget *widget,gpointer   data);
gint      rescan_button_press(GtkWidget *widget,GdkEventButton *bevent,gpointer   data);
void      fractalexplorer_list_ok_callback (GtkWidget *w,  gpointer   client_data);
void      fractalexplorer_list_cancel_callback (GtkWidget *w, gpointer   client_data);
void      fractalexplorer_dialog_edit_list (GtkWidget *lwidget,fractalexplorerOBJ *obj,gint created);
GtkWidget *new_fractalexplorer_obj(gchar * name);
void      fractalexplorer_rescan_cancel_callback (GtkWidget *w, gpointer   client_data);
void             clear_list_items(GtkList *list);
gint             fractalexplorer_list_pos(fractalexplorerOBJ *fractalexplorer);
gint             fractalexplorer_list_insert (fractalexplorerOBJ *fractalexplorer);
GtkWidget*       fractalexplorer_list_item_new_with_label_and_pixmap (fractalexplorerOBJ *obj, gchar *label, GtkWidget *pix_widget);
GtkWidget*       fractalexplorer_new_pixmap(GtkWidget *list, char **pixdata);
GtkWidget *fractalexplorer_list_add(fractalexplorerOBJ *obj);
void             list_button_update(fractalexplorerOBJ *obj);
fractalexplorerOBJ *fractalexplorer_new(void);
void             build_list_items(GtkWidget *list);

void             plug_in_parse_fractalexplorer_path();
void             fractalexplorer_free(fractalexplorerOBJ * fractalexplorer);
void             fractalexplorer_free_everything(fractalexplorerOBJ * fractalexplorer);
void             fractalexplorer_list_free_all ();
fractalexplorerOBJ *fractalexplorer_load (gchar *filename, gchar *name);
void      fractalexplorer_rescan_file_selection_ok(GtkWidget *w, GtkFileSelection *fs, gpointer data);
void             fractalexplorer_list_load_all(GList *plist);
GtkWidget *add_objects_list ();
GtkWidget *add_gradients_list ();
void      fractalexplorer_rescan_ok_callback (GtkWidget *w, gpointer   client_data);
void      fractalexplorer_rescan_add_entry_callback (GtkWidget *w, gpointer   client_data);
void      fractalexplorer_rescan_list (void);


/**********************************************************************
  Global variables  
 *********************************************************************/

extern double       xmin,
                    xmax,
                    ymin,
                    ymax;
extern double       xbild,
                    ybild,
                    xdiff,
                    ydiff;
extern double       x_press,
                    y_press;
extern double       x_release,
                    y_release;
extern float        cx;
extern float        cy;
extern GDrawable   *drawable;
extern gint         tile_width,
                    tile_height;
extern gint         img_width,
                    img_height,
                    img_bpp;
extern gint         sel_x1,
                    sel_y1,
                    sel_x2,
                    sel_y2;
extern gint         sel_width,
                    sel_height;
extern gint         preview_width,
                    preview_height;
extern GTile       *the_tile;
extern double       cen_x,
                    cen_y;
extern double       xpos,
                    ypos,
                    oldxpos,
                    oldypos;
extern gint         do_redsinus,
                    do_redcosinus,
                    do_rednone;
extern gint         do_greensinus,
                    do_greencosinus,
                    do_greennone;
extern gint         do_bluesinus,
                    do_bluecosinus,
                    do_bluenone;
extern gint         do_redinvert,
                    do_greeninvert,
		    do_blueinvert;
extern gint         do_colormode1,
                    do_colormode2;
extern gint         do_type0,
                    do_type1,
		    do_type2,
                    do_type3,
                    do_type4,
                    do_type5,
                    do_type6,
                    do_type7,
                    do_type8,
                    do_english,
                    do_french,
                    do_german;
extern GtkWidget   *maindlg;
extern GtkWidget   *logodlg;
extern GtkWidget   *loaddlg;
extern GtkWidget   *cmap_preview;
extern GtkWidget   *cmap_preview_long;
extern GtkWidget   *cmap_preview_long2;
extern GtkWidget   *delete_frame_to_freeze;
extern GtkWidget   *fractalexplorer_gtk_list;
extern GtkWidget   *save_menu_item;
extern GtkWidget   *fractalexplorer_op_menu;
extern GtkTooltips *tips;
extern GdkColor     tips_fg,
                    tips_bg;
extern GdkCursor   *MyCursor;
extern int          ready_now;
extern explorer_vals_t     
                    zooms[100];
extern DialogElements
                    *elements;
extern int          zoomindex;
extern int          zoommax;
extern gdouble     *gg;
extern int          line_no;
extern gchar       *filename;
extern clrmap       colormap;
extern GList	   *fractalexplorer_path_list;
extern GList	   *fractalexplorer_list;
extern GList	   *gradient_list;
extern gchar 	   *tpath;
extern fractalexplorerOBJ 
                   *fractalexplorer_obj_for_menu;
extern GList       *rescan_list;
extern int 	    lng;

extern GPlugInInfo  PLUG_IN_INFO;

extern explorer_interface_t wint;

extern explorer_vals_t wvals;

extern explorer_vals_t standardvals;

#endif
