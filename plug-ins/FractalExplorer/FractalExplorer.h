#ifndef __FRACTALEXPLORER_H__
#define __FRACTALEXPLORER_H__

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

/**********************************************************************
 Magic numbers  
 *********************************************************************/

#define PREVIEW_SIZE 128
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  60
#define MAX_LOAD_LINE 256
#define GR_WIDTH 325

#define MAXNCOLORS 8192
#define MAXSTRLEN 256

#define FRACTAL_HEADER "Fractal Explorer Plug-In Version 2 - (c) 1997 <cotting@mygale.org>\n"
#define fractalexplorer_HEADER "Fractal Explorer Plug-In Version 2 - (c) 1997 <cotting@mygale.org>\n"

enum
{
  SINUS,
  COSINUS,
  NONE
};

enum
{
  TYPE_MANDELBROT,
  TYPE_JULIA,
  TYPE_BARNSLEY_1,
  TYPE_BARNSLEY_2,
  TYPE_BARNSLEY_3,
  TYPE_SPIDER,
  TYPE_MAN_O_WAR,
  TYPE_LAMBDA,
  TYPE_SIERPINSKI,
  NUM_TYPES
};

/**********************************************************************
 Types  
 *********************************************************************/

typedef struct
{
  gint     fractaltype;
  gdouble  xmin;
  gdouble  xmax;
  gdouble  ymin;
  gdouble  ymax;
  gdouble  iter;
  gdouble  cx;
  gdouble  cy;
  gint     colormode;
  gdouble  redstretch;
  gdouble  greenstretch;
  gdouble  bluestretch;
  gint     redmode;
  gint     greenmode;
  gint     bluemode;
  gint     redinvert;
  gint 	   greeninvert;
  gint 	   blueinvert;
  gint     alwayspreview;
  gint     ncolors;
  gint     useloglog;
} explorer_vals_t;

typedef struct
{
  GtkWidget *preview;
  guchar    *wimage;
  gint       run;
} explorer_interface_t;

typedef gint       colorvalue[3];

typedef colorvalue clrmap[MAXNCOLORS];

typedef struct
{
  GtkWidget     *text;
  GtkAdjustment *data;
} scaledata;

typedef struct _DialogElements DialogElements;

struct _DialogElements
{
  GtkWidget  *type[NUM_TYPES];
  GtkObject  *xmin;
  GtkObject  *xmax;
  GtkObject  *ymin;
  GtkObject  *ymax;
  GtkObject  *iter;
  GtkObject  *cx;
  GtkObject  *cy;

  GtkObject  *ncol;
  GtkWidget  *useloglog;

  GtkObject  *red;
  GtkObject  *green;
  GtkObject  *blue;

  GtkWidget  *redmode[3];
  GtkWidget  *redinvert;

  GtkWidget  *greenmode[3];
  GtkWidget  *greeninvert;

  GtkWidget  *bluemode[3];
  GtkWidget  *blueinvert;

  GtkWidget  *colormode[2];
};


typedef struct DFigObj
{
  gchar           *name;      /* Trailing name of file  */
  gchar           *filename;  /* Filename itself */
  gchar           *draw_name; /* Name of the drawing */
  explorer_vals_t  opts;      /* Options enforced when fig saved */
  GtkWidget       *list_item;
  GtkWidget       *label_widget;
  GtkWidget       *pixmap_widget;
  gint             obj_status;
} fractalexplorerOBJ;  


typedef struct GigObj
{
  gchar     *name;      /* Trailing name of file  */
  gchar     *filename;  /* Filename itself */
  gchar     *draw_name; /* Name of the drawing */
  gint       typus;
  GtkWidget *list_item;
  GtkWidget *label_widget;
  GtkWidget *pixmap_widget;
  gint       obj_status;
} gradientOBJ;  

typedef struct _fractalexplorerListOptions
{
  GtkWidget          *query_box;
  GtkWidget          *name_entry;
  GtkWidget          *list_entry;
  fractalexplorerOBJ *obj;
  gint                created;
} fractalexplorerListOptions;

/* States of the object */
#define fractalexplorer_OK       0x0
#define fractalexplorer_MODIFIED 0x1
#define fractalexplorer_READONLY 0x2

#define gradient_GRADIENTEDITOR  0x2

extern fractalexplorerOBJ *current_obj;
extern fractalexplorerOBJ *pic_obj;
extern GtkWidget          *delete_dialog;

GtkWidget * add_objects_list                   (void);
void        plug_in_parse_fractalexplorer_path (void);

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
extern GimpDrawable   *drawable;
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
extern GimpTile       *the_tile;
extern double       cen_x,
                    cen_y;
extern double       xpos,
                    ypos,
                    oldxpos,
                    oldypos;
extern GtkWidget   *maindlg;
extern GtkWidget   *logodlg;
extern GtkWidget   *cmap_preview;
extern GtkWidget   *delete_frame_to_freeze;
extern GtkWidget   *fractalexplorer_gtk_list;
extern GtkWidget   *save_menu_item;
extern GtkWidget   *fractalexplorer_op_menu;
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

extern explorer_interface_t wint;

extern explorer_vals_t wvals;

extern explorer_vals_t standardvals;

#endif
