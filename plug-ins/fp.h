#define MAX_PREVIEW_SIZE   125
#define MAX_ROUGHNESS      128
#define RANGE_HEIGHT       15
#define PR_BX_BRDR         10
#define ALL                255

#define RANGE_ADJUST_MASK GDK_EXPOSURE_MASK | \
                        GDK_ENTER_NOTIFY_MASK | \
                        GDK_BUTTON_PRESS_MASK | \
                        GDK_BUTTON_RELEASE_MASK | \
                        GDK_BUTTON1_MOTION_MASK | \
                        GDK_POINTER_MOTION_HINT_MASK


typedef struct {
  gint run;
} fpInterface;

typedef double hsv;

typedef struct {
  gint      width;
  gint      height;
  guchar    *rgb;
  hsv       *hsv;
  guchar    *mask;
} ReducedImage;

typedef enum {
  SHADOWS,
  MIDTONES,
  HIGHLIGHTS,
  INTENSITIES
}FP_Intensity;

enum {
  NONEATALL  =0,
  CURRENT    =1,
  HUE        =2, 
  SATURATION =4,
  VALUE      =8
};


enum {
  RED,
  GREEN,
  BLUE,
  CYAN,
  YELLOW,
  MAGENTA,
  ALL_PRIMARY
};

enum {
  DOWN = -1,
  UP   =  1
};

enum {
  BY_HUE, 
  BY_SAT, 
  BY_VAL, 
  JUDGE_BY
};


typedef struct {
  GtkWidget *window;
  GtkWidget *shadowsEntry;
  GtkWidget *midtonesEntry;
  GtkWidget *rangePreview;
  GtkWidget *aliasingPreview;
  GtkObject *aliasingData;
  GtkWidget *aliasingGraph;
} AdvancedWindow;


typedef struct {
  int Color;
  float Rough;
  GtkWidget *roughnessScale;
  float Alias;
  GtkWidget *aliasingScale;
  float PreviewSize;
  GtkWidget *previewSizeScale;
  FP_Intensity Range;
  gint ValueBy;
  gint SlctnOnly;
  gint  RealTime;
  guchar Offset;
  guchar VisibleFrames;
  guchar Cutoffs[INTENSITIES];
  gint Touched[JUDGE_BY];
  gint redAdj[JUDGE_BY][256];
  gint blueAdj[JUDGE_BY][256];
  gint greenAdj[JUDGE_BY][256];
  gint satAdj[JUDGE_BY][256];
  GtkWidget *rangeLabels[12];
}FP_Params;

GtkWidget *fp_create_bna(void);
GtkWidget *fp_create_rough(void);
GtkWidget *fp_create_range(void);
GtkWidget *fp_create_circle_palette(void);
GtkWidget *fp_create_lnd(void);
GtkWidget *fp_create_show(void);
GtkWidget *fp_create_msnls();
GtkWidget *fp_create_frame_select();  
GtkWidget *fp_create_pixels_select_by();

void  fp_show_hide_frame(GtkWidget *button,
			 GtkWidget *frame);
 

ReducedImage  *Reduce_The_Image   (GDrawable *,
				   GDrawable *,
				   gint,
				   gint);

void      fp_render_preview  (GtkWidget *,
			      gint,
			      gint         );

void      Update_Current_FP  (gint,
			      gint         );

void      fp_Create_Nudge    (gint*        );

gint      fp_dialog          (void);
gint      fp_advanced_dialog (void);

void      fp_advanced_call   (void);

void      fp_entire_image                (GtkWidget *,
					  gpointer     );
void      fp_selection_only              (GtkWidget *,
					  gpointer     );

void      fp_selection_in_context        (GtkWidget *,
					  gpointer     );

void      selectionMade                  (GtkWidget *,
					  gpointer     );
void      fp_close_callback              (GtkWidget *,
					  gpointer     );
void      fp_ok_callback                 (GtkWidget *,
					  gpointer     );
void      fp_scale_update                (GtkAdjustment *,
					  float*       );
void      fp_change_current_range        (GtkAdjustment*,
					  gint*        );
void      fp_change_current_pixels_by    (GtkWidget *,
					  gint      *);
void      resetFilterPacks               (void);

void      update_range_labels            (void);

void      fp_create_smoothness_graph     (GtkWidget*    );

void      fp_range_preview_spill         (GtkWidget*, 
					  gint           );

void      Create_A_Preview        (GtkWidget  **,
				   GtkWidget  **, 
				   int,
				   int           );

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

void      Adjust_Preview_Sizes     (int width, 
				    int height   );
void      refreshPreviews          (int);
void      initializeFilterPacks    (void);

void      fp                       (GDrawable  *drawable);
void      fp_row                   (const guchar *src_row,
				    guchar *dest_row,
				    gint row,
				    gint row_width,
				    gint bytes);

void      draw_slider              (GdkWindow *window,
				    GdkGC     *border_gc,
				    GdkGC     *fill_gc,
				    int        xpos);
gint     FP_Range_Change_Events    (GtkWidget *, 
				    GdkEvent *, 
				    FP_Params *);

void     As_You_Drag               (GtkWidget *button);
void     preview_size_scale_update (GtkAdjustment *adjustment,
				    float         *scale_val);
void     ErrorMessage              (guchar *);


void     fp_advanced_ok();
