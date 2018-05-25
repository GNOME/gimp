#ifndef __FRACTALEXPLORER_H__
#define __FRACTALEXPLORER_H__


/**********************************************************************
 Magic numbers
 *********************************************************************/

#define PREVIEW_SIZE 256
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  60
#define MAX_LOAD_LINE 256
#define GR_WIDTH 325

#define MAXNCOLORS 8192
#define MAXSTRLEN 256

#define PLUG_IN_PROC   "plug-in-fractalexplorer"
#define PLUG_IN_BINARY "fractal-explorer"
#define PLUG_IN_ROLE   "gimp-fractal-explorer"

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
  gboolean redinvert;
  gboolean greeninvert;
  gboolean blueinvert;
  gboolean alwayspreview;
  gint     ncolors;
  gboolean gradinvert;
  gboolean useloglog;
} explorer_vals_t;

typedef struct
{
  GtkWidget *preview;
  guchar    *wimage;
  gint       run;
} explorer_interface_t;

/* typedef gint       colorvalue[3]; */
typedef struct
  {
    guchar r, g, b;
  } gucharRGB;

typedef gucharRGB  clrmap[MAXNCOLORS];

typedef guchar     vlumap[MAXNCOLORS];

typedef struct
{
  GtkWidget     *text;
  GtkAdjustment *data;
} scaledata;

typedef struct _DialogElements DialogElements;

struct _DialogElements
{
  GtkWidget     *type[NUM_TYPES];
  GtkAdjustment *xmin;
  GtkAdjustment *xmax;
  GtkAdjustment *ymin;
  GtkAdjustment *ymax;
  GtkAdjustment *iter;
  GtkAdjustment *cx;
  GtkAdjustment *cy;

  GtkAdjustment *ncol;
  GtkWidget     *useloglog;

  GtkAdjustment *red;
  GtkAdjustment *green;
  GtkAdjustment *blue;

  GtkWidget     *redmode[3];
  GtkWidget     *redinvert;

  GtkWidget     *greenmode[3];
  GtkWidget     *greeninvert;

  GtkWidget     *bluemode[3];
  GtkWidget     *blueinvert;

  GtkWidget     *colormode[2];
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

#define gradient_GRADIENTEDITOR  0x2

extern fractalexplorerOBJ *current_obj;

GtkWidget * add_objects_list (void);

/**********************************************************************
  Global variables
 *********************************************************************/

extern gdouble      xmin;
extern gdouble      xmax;
extern gdouble      ymin;
extern gdouble      ymax;
extern gdouble      xbild;
extern gdouble      ybild;
extern gdouble      xdiff;
extern gdouble      ydiff;
extern gint         sel_x1,
                    sel_y1,
                    sel_x2,
                    sel_y2;
extern gint         preview_width,
                    preview_height;
extern gdouble     *gg;
extern int          line_no;
extern gchar       *filename;
extern clrmap       colormap;
extern gchar       *fractalexplorer_path;


extern explorer_interface_t wint;

extern explorer_vals_t wvals;
extern GimpDrawable   *drawable;


/**********************************************************************
  Global functions
 *********************************************************************/

void explorer_render_row (const guchar *src_row,
                          guchar       *dest_row,
                          gint          row,
                          gint          row_width,
                          gint          bpp);
#endif
