#include <stdio.h>
#include <stdlib.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "rcm_hsv.h"
#include "pmg_gtk_objects.h"

#define RCM_XPM_DIR "./"
#define MAX_PREVIEW_SIZE   150
#define ALL                255
#define INITIAL_ALPHA      0
#define INITIAL_BETA       (PI/2.0)
#define INITIAL_GRAY_SAT   0.0
#define RADIUS 60
#define MARGIN 4
#define SUM (2*RADIUS+2*MARGIN)
#define CENTER (SUM/2)
#define TP (2*PI)

#define GRAY_RADIUS 60
#define GRAY_MARGIN 3
#define GRAY_SUM (2*GRAY_RADIUS+2*GRAY_MARGIN)
#define GRAY_CENTER (GRAY_SUM/2)

#define LITTLE_RADIUS 3
#define EACH_OR_BOTH .3

#define sqr(X)  ((X)*(X))
#define SWAP(X,Y) {float t=X; X=Y; Y=t;}

#define RANGE_ADJUST_MASK GDK_EXPOSURE_MASK | \
                          GDK_ENTER_NOTIFY_MASK | \
                          GDK_BUTTON_PRESS_MASK | \
                          GDK_BUTTON_RELEASE_MASK | \
                          GDK_BUTTON1_MOTION_MASK | \
                          GDK_POINTER_MOTION_HINT_MASK

typedef struct {
  float alpha;
  float beta;
  int   cw_ccw;
} RcmAngle;


typedef enum {
  VIRGIN,
  DRAG_START,
  DRAGING,
  DO_NOTHING
} RcmOp;

typedef struct {
  GtkWidget         *preview;
  GtkWidget         *frame;
  GtkWidget         *table;
  PmgButtonLabelXpm *cw_ccw_xpm_button;
  GtkWidget         *alpha_entry;
  GtkWidget         *alpha_units_label;
  GtkWidget         *beta_entry;
  GtkWidget         *beta_units_label;
  gfloat            *target;
  gint               mode;
  RcmAngle          *angle;
  RcmOp              action_flag;
  gfloat             prev_clicked;
} RcmCircle;

typedef struct {
  GtkWidget *dlg;
  GtkWidget *bna_frame;
  GtkWidget *before;
  GtkWidget *after;
} RcmBna;

typedef struct {
  GtkWidget *preview;
  GtkWidget *frame;
  float gray_sat;
  float hue;
  float satur;
  GtkWidget *gray_sat_entry;
  GtkWidget *hue_entry;
  GtkWidget *hue_units_label;
  GtkWidget *satur_entry;
  RcmOp action_flag;
} RcmGray;

typedef struct {
  gint      width;
  gint      height;
  guchar    *rgb;
  hsv       *hsv;
  guchar    *mask;
} ReducedImage;

typedef struct {
  gint         Slctn;
  gint         RealTime;
  gint         Success;
  gint         Units;
  gint         Gray_to_from;
  GDrawable    *drawable;
  GDrawable    *mask;
  ReducedImage *reduced;
  RcmCircle    *To;
  RcmCircle    *From;
  RcmGray      *Gray;
  RcmBna       *Bna;
} RcmParams;


enum {
  ENTIRE_IMAGE,
  SELECTION,
  SELECTION_IN_CONTEXT,
  PREVIEW_OPTIONS
};

enum {
 EACH,
 BOTH,
 DEGREES,
 RADIANS,
 RADIANS_OVER_PI,
 GRAY_FROM,
 GRAY_TO,
 CURRENT,
 ORIGINAL
};

gint rcm_dialog();

GtkWidget *rcm_create_bna(void);
GtkWidget *rcm_create_from_to(void);

float arctg(float y, float x);

float min_prox(float alpha, 
	       float beta, 
	       float angle);

float *closest(float *alpha, 
	       float *beta, 
	       float  angle);

float beta_from_alpha_and_delta(float alpha, 
				float delta);

float angle_mod_2PI(float angle);

float angle_inside_slice(float angle, RcmAngle *slice);

void rcm_draw_arrows(GdkWindow  *window,
		     GdkGC      *color,
		     RcmAngle   *angle);

gint rcm_expose_event  (GtkWidget *widget, 
			GdkEvent  *event, 
			RcmCircle *circle);

gint rcm_button_press_event  (GtkWidget *widget, 
			      GdkEvent  *event, 
			      RcmCircle *circle);

gint rcm_release_event  (GtkWidget *widget, 
			 GdkEvent  *event, 
			 RcmCircle *circle);

gint rcm_motion_notify_event  (GtkWidget *widget, 
			       GdkEvent  *event, 
			       RcmCircle *circle);

/***************** GRAY ***********************/
void rcm_draw_little_circle(GdkWindow  *window,
			   GdkGC      *color,
			   float      hue,
			   float      satur);

void rcm_draw_large_circle(GdkWindow  *window,
			   GdkGC      *color,
			   float      gray_sat);

gint rcm_gray_expose_event  (GtkWidget *widget, 
			     GdkEvent  *event, 
			     RcmGray   *circle);

gint rcm_gray_button_press_event  (GtkWidget *widget, 
				   GdkEvent  *event, 
				   RcmGray   *circle);

gint rcm_gray_release_event  (GtkWidget *widget, 
			      GdkEvent  *event, 
			      RcmGray   *circle);

gint rcm_gray_motion_notify_event  (GtkWidget *widget, 
				    GdkEvent  *event, 
				    RcmGray   *circle);

/***************** END GRAY ***********************/

float rcm_units_factor(gint units);
gchar *rcm_units_string(gint units);
void rcm_set_alpha(GtkWidget *entry,
		   gpointer  data);

void rcm_set_beta(GtkWidget *entry,
		   gpointer  data);

void rcm_set_hue(GtkWidget *entry,
		 gpointer  data);

void rcm_set_satur(GtkWidget *entry,
		   gpointer  data);

void rcm_set_gray_sat(GtkWidget *entry,
		      gpointer  data);

void  rcm_show_hide_frame(GtkWidget *button,
			  GtkWidget *frame);

RcmCircle     *rcm_create_square_preview(gint, gchar *);

ReducedImage  *Reduce_The_Image   (GDrawable *,
				   GDrawable *,
				   gint,
				   gint);

GtkWidget *rcm_widget_in_a_table(GtkWidget     *table,
				 guchar        *string,
				 gint          x_spot,	
				 gint          y_spot,
				 GtkSignalFunc *function,
				 gpointer      *data);

GtkWidget *rcm_label_in_a_table(GtkWidget *table,
				guchar     *string,
				int        x_spot,		   
				int        y_spot);

GtkWidget *rcm_entry_in_a_table(GtkWidget *table,
				float     value,
				gint      x_spot,		   
				gint      y_spot,
				GtkSignalFunc func,
				gpointer  data);


void      rcm_render_preview  (GtkWidget *,
			       gint);

void      rcm_render_circle_preview    (GtkWidget *preview,
					int       sum,
					int       margin);

void      rcm_preview_what             (GtkWidget *widget,
					gpointer  data);

void      rcm_cw_ccw                   (GtkWidget *,
					RcmCircle *);

void      rcm_a_to_b                   (GtkWidget *,
					RcmCircle *);

void      rcm_360_degrees              (GtkWidget *,
					RcmCircle *);

void      rcm_close_callback              (GtkWidget *,
					  gpointer     );
void      rcm_ok_callback                 (GtkWidget *,
					  gpointer     );

void      Create_A_Preview        (GtkWidget  **,
				   GtkWidget  **, 
				   int,
				   int,
				   int);

void      Create_A_Table_Entry    (GtkWidget  **,
				   GtkWidget  *,
				   char       *);

GSList*   Button_In_A_Box        (GtkWidget  *,
				  GSList     *,
				  guchar     *,
				  GtkSignalFunc,
				  gpointer,
				  int           );

void Check_Button_In_A_Box       (GtkWidget     *,
				  guchar        *label,
				  GtkSignalFunc func,
				  gpointer      data,
				  int           clicked);

void      rcm                       (GDrawable  *drawable);
void      rcm_row                   (const guchar *src_row,
				    guchar *dest_row,
				    gint row,
				    gint row_width,
				    gint bytes);

void     As_You_Drag               (GtkWidget *button);

void     preview_size_scale_update (GtkAdjustment *adjustment,
				    float         *scale_val);

void     ErrorMessage              (guchar *);


void     rcm_advanced_ok();
gint     rcm_fake_transparency(gint i, gint j);

void     rcm_float_angle_in_an_entry(GtkWidget *entry,
				     float     value,
				     gint      units);

void     rcm_float_in_an_entry(GtkWidget *entry,
			       float     value);

void     rcm_update_entries(gint units);
float    rcm_unit_factor(gint units);
gint     rcm_is_gray(float s);
float    rcm_linear(float, float, float, float, float);
float    rcm_left_end(RcmAngle *angle);
float    rcm_right_end(RcmAngle *angle);
GtkWidget *rcm_xpm_label_box (GtkWidget *parent,
			      gchar *xpm_filename, 
			      gchar *label_text);

gint rcm_fake_transparency(gint i, gint j);
