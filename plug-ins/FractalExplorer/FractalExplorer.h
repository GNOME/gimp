/**********************************************************************
 Magic numbers  
 *********************************************************************/

#define PREVIEW_SIZE 128
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  60
#define MAX_LOAD_LINE 256
#define GR_WIDTH 325

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

typedef int         clrmap[256][3];

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

static fractalexplorerOBJ *current_obj;
static fractalexplorerOBJ *pic_obj;
static GtkWidget *delete_dialog = NULL;



/**********************************************************************
 Declare local functions
 *********************************************************************/

/* Gimp interface functions */
static void         query(void);
static void         run(char *name, int nparams, GParam * param, int *nreturn_vals,
			GParam ** return_vals);

/* Dialog and fractal functions */
void                explorer(GDrawable * drawable);
void                explorer_render_row(const guchar * src_row, guchar * dest_row, gint row,
					gint row_width, gint bytes);
void                transform(short int *, short int *, short int *, double, double, double);
gint                explorer_dialog(void);
void                dialog_update_preview(void);

/* Functions for dialog widgets */
void                dialog_create_value(char *title, GtkTable * table, int row, gdouble * value,
	      int left, int right, const char *desc, scaledata * scalevalues);
void                dialog_scale_update(GtkAdjustment * adjustment, gdouble * value);
void                dialog_create_int_value(char *title, GtkTable * table, int row, gdouble * value,
	      int left, int right, const char *desc, scaledata * scalevalues);
void                dialog_scale_int_update(GtkAdjustment * adjustment, gdouble * value);
void                dialog_entry_update(GtkWidget * widget, gdouble * value);
void                dialog_close_callback(GtkWidget * widget, gpointer data);
void                dialog_ok_callback(GtkWidget * widget, gpointer data);
void                dialog_cancel_callback(GtkWidget * widget, gpointer data);
void                dialog_step_out_callback(GtkWidget * widget, gpointer data);
void                dialog_step_in_callback(GtkWidget * widget, gpointer data);
void                dialog_undo_zoom_callback(GtkWidget * widget, gpointer data);
void                dialog_redo_zoom_callback(GtkWidget * widget, gpointer data);
void                dialog_redraw_callback(GtkWidget * widget, gpointer data);
void                dialog_reset_callback(GtkWidget * widget, gpointer data);
GtkWidget          *explorer_logo_dialog();
GtkWidget          *explorer_load_dialog();
void                explorer_toggle_update(GtkWidget * widget, gpointer data);
void                set_tooltip(GtkTooltips * tooltips, GtkWidget * widget, const char *desc);
void         dialog_change_scale(void);
void         set_cmap_preview(void);
void         make_color_map(void);
void                create_file_selection();
void                create_load_file_selection();
void                explorer_load();
void                load_button_press(GtkWidget * widget, gpointer data);

/* Preview events */
gint                preview_button_press_event(GtkWidget * widget, GdkEventButton * event);
gint                preview_button_release_event(GtkWidget * widget, GdkEventButton * event);
gint                preview_motion_notify_event(GtkWidget * widget, GdkEventButton * event);
gint                preview_leave_notify_event(GtkWidget * widget, GdkEventButton * event);
gint                preview_enter_notify_event(GtkWidget * widget, GdkEventButton * event);



static gint      list_button_press(GtkWidget *widget,GdkEventButton *event,gpointer   data);
static gint      new_button_press(GtkWidget *widget,GdkEventButton *bevent,gpointer   data);
static gint      fractalexplorer_delete_fractalexplorer_callback(GtkWidget *widget,GdkEventButton *bevent,gpointer   data);
static gint      delete_button_press_ok(GtkWidget *widget,gpointer   data);
static gint      rescan_button_press(GtkWidget *widget,GdkEventButton *bevent,gpointer   data);
static void      fractalexplorer_list_ok_callback (GtkWidget *w,  gpointer   client_data);
static void      fractalexplorer_list_cancel_callback (GtkWidget *w, gpointer   client_data);
static void      fractalexplorer_dialog_edit_list (GtkWidget *lwidget,fractalexplorerOBJ *obj,gint created);
static GtkWidget *new_fractalexplorer_obj(gchar * name);
static void      fractalexplorer_rescan_cancel_callback (GtkWidget *w, gpointer   client_data);
void             clear_list_items(GtkList *list);
gint             fractalexplorer_list_pos(fractalexplorerOBJ *fractalexplorer);
gint             fractalexplorer_list_insert (fractalexplorerOBJ *fractalexplorer);
GtkWidget*       fractalexplorer_list_item_new_with_label_and_pixmap (fractalexplorerOBJ *obj, gchar *label, GtkWidget *pix_widget);
GtkWidget*       fractalexplorer_new_pixmap(GtkWidget *list, char **pixdata);
static GtkWidget *fractalexplorer_list_add(fractalexplorerOBJ *obj);
void             list_button_update(fractalexplorerOBJ *obj);
fractalexplorerOBJ *fractalexplorer_new(void);
void             build_list_items(GtkWidget *list);
/*
static void      fractalexplorer_op_menu_popup(gint button, guint32 activate_time,fractalexplorerOBJ *obj);
*/
void             plug_in_parse_fractalexplorer_path();
void             fractalexplorer_free(fractalexplorerOBJ * fractalexplorer);
void             fractalexplorer_free_everything(fractalexplorerOBJ * fractalexplorer);
void             fractalexplorer_list_free_all ();
fractalexplorerOBJ *fractalexplorer_load (gchar *filename, gchar *name);
static void      fractalexplorer_rescan_file_selection_ok(GtkWidget *w, GtkFileSelection *fs, gpointer data);
void             fractalexplorer_list_load_all(GList *plist);
static GtkWidget *add_objects_list ();
static GtkWidget *add_gradients_list ();
static void      fractalexplorer_rescan_ok_callback (GtkWidget *w, gpointer   client_data);
static void      fractalexplorer_rescan_add_entry_callback (GtkWidget *w, gpointer   client_data);
static void      fractalexplorer_rescan_list (void);
/*
static void      fractalexplorer_op_menu_create(GtkWidget *window);
*/


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
static GList *rescan_list = NULL;
int 		 lng=LNG_GERMAN;


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

static explorer_vals_t wvals =
{
    0, -2.0, 2.0, -1.5, 1.5, 50.0, -0.75, -0.2, 0, 128.0, 128.0, 128.0, 1, 1, 0, 0, 0, 0, 1, 0,
};				/* wvals */

static explorer_vals_t standardvals =
{
    0, -2.0, 2.0, -1.5, 1.5, 50.0, -0.75, -0.2, 0, 128.0, 128.0, 128.0, 1, 1, 0, 0, 0, 0, 1, 0,
};				/* standardvals */
