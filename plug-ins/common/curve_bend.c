/* curve_bend plugin for the GIMP (tested with GIMP 1.1.9, requires gtk+ 1.2) */

/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Revision history
 *  (1999/09/13)  v1.01  hof: PDB-calls updated for gimp 1.1.9
 *  (1999/05/10)  v1.0   hof: first public release
 *  (1999/04/23)  v0.0   hof: coding started,
 *                            splines and dialog parts are similar to curves.c
 */

#include "config.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/* Defines */
#define PLUG_IN_NAME        "plug_in_curve_bend"
#define PLUG_IN_PRINT_NAME  "CurveBend"
#define PLUG_IN_VERSION     "v1.01 (1999/09/13)"
#define PLUG_IN_IMAGE_TYPES "RGB*, GRAY*"
#define PLUG_IN_AUTHOR      "Wolfgang Hofer (hof@hotbot.com)"
#define PLUG_IN_COPYRIGHT   "Wolfgang Hofer"
#define PLUG_IN_DESCRIBTION "Bends a layer using 2 spline-curves"

#define PLUG_IN_ITER_NAME       "plug_in_curve_bend_Iterator"
#define PLUG_IN_DATA_ITER_FROM  "plug_in_curve_bend_ITER_FROM"
#define PLUG_IN_DATA_ITER_TO    "plug_in_curve_bend_ITER_TO"

#define KEY_POINTFILE "POINTFILE_CURVE_BEND"
#define KEY_POINTS    "POINTS"
#define KEY_VAL_Y     "VAL_Y"

#define MIDDLE 127

#define ROUND(x)  ((int) ((x) + 0.5))
#define SIGNED_ROUND(x)  ((int) (((x < 0) ? (x) - 0.5 : (x) + 0.5)  ))
#define MIX_CHANNEL(a, b, m)  (((a * m) + (b * (255 - m))) / 255)

#define UP_GRAPH              0x1
#define UP_XRANGE_TOP         0x2
#define UP_PREVIEW_EXPOSE     0x4
#define UP_PREVIEW            0x8
#define UP_DRAW               0x10
#define UP_ALL                0xFF

#define ENTRY_WIDTH      50
#define GRAPH_WIDTH      256
#define GRAPH_HEIGHT     256
#define PREVIEW_SIZE_X   256
#define PREVIEW_SIZE_Y   256
#define PV_IMG_WIDTH     128
#define PV_IMG_HEIGHT    128
#define RADIUS           3
#define MIN_DISTANCE     8
#define PREVIEW_BPP      3
#define PREVIEW_BG_GRAY1 108
#define PREVIEW_BG_GRAY2 156

#define SMOOTH       0
#define GFREE        1

#define RANGE_MASK  GDK_EXPOSURE_MASK | \
                    GDK_ENTER_NOTIFY_MASK

#define GRAPH_MASK  GDK_EXPOSURE_MASK | \
		    GDK_POINTER_MOTION_MASK | \
		    GDK_POINTER_MOTION_HINT_MASK | \
                    GDK_ENTER_NOTIFY_MASK | \
		    GDK_BUTTON_PRESS_MASK | \
		    GDK_BUTTON_RELEASE_MASK | \
		    GDK_BUTTON1_MOTION_MASK


#define OUTLINE_UPPER    0
#define OUTLINE_LOWER    1

typedef struct _BenderValues BenderValues;
struct _BenderValues
{
  unsigned char  curve[2][256];            /* for curve_type freehand mode   0   <= curve  <= 255 */
  gdouble        points[2][17][2];         /* for curve_type smooth mode     0.0 <= points <= 1.0 */

  int            curve_type;

  gint           smoothing;
  gint           antialias;
  gint           work_on_copy;
  gdouble        rotation;
  
  
  gint32         total_steps;
  gdouble        current_step;
};


typedef struct _Curves Curves;

struct _Curves
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _BenderDialog BenderDialog;

struct _BenderDialog
{
  GtkWidget *    shell;
  GtkWidget *    outline_menu;
  GtkWidget *    pv_widget;
  GtkWidget *    graph;
  GtkWidget *    rotate_entry;
  GdkPixmap *    pixmap;
  GtkWidget *    filesel;

  GDrawable *    drawable;
  int            color;
  int            outline;
  gint           preview;

  int            grab_point;
  int            last;
  int            leftmost;
  int            rightmost;
  int            curve_type;
  gdouble        points[2][17][2];    /* 0.0 <= points    <= 1.0 */
  unsigned char  curve[2][256];       /* 0   <= curve     <= 255 */
  gint32        *curve_ptr[2];        /* 0   <= curve_ptr <= src_drawable_width 
                                       * both arrays are allocated dynamic, depending on drawable width
                                       */

  gint32  min2[2];
  gint32  max2[2];
  gint32  zero2[2];
    
  int            show_progress;
  gint           smoothing;
  gint           antialias;
  gint           work_on_copy;
  gdouble        rotation;
  
  
  gint32         preview_image_id;
  gint32         preview_layer_id1;
  gint32         preview_layer_id2;
  
  BenderValues  *bval_from;
  BenderValues  *bval_to;
  BenderValues  *bval_curr;
  
  gint           run;
};

/* points Coords:
 *
 *  1.0 +----+----+----+----+ 
 *      |    .    |    |    | 
 *      +--.-+--.-+----+----+ 
 *      .    |    .    |    |  
 *  0.5 +----+----+-.--+----+ 
 *      |    |    |    .    . 
 *      +----+----+----+-.-.+ 
 *      |    |    |    |    | 
 *  0.0 +----+----+----+----+ 
 *      0.0      0.5       1.0
 *
 * curve Coords:
 *
 *      OUTLINE_UPPER                                       OUTLINE_LOWER
 *
 *  255 +----+----+----+----+                          255 +----+----+----+----+
 *      |    .    |    |    |  ---   max	           |	.    |    |    |  ---	max
 *      +--.-+--.-+----+----+			           +--.-+--.-+----+----+
 *      .    |    .    |    |			 zero ___  .	|    .    |    |                 
 *      +----+----+-.--+----+   		           +----+----+-.--+----+
 *      |    |    |    .    .  ---   zero      	           |	|    |    .    .  
 *      +----+----+----+-.-.+  ___   min	           +----+----+----+-.-.+  ___	min
 *      |    |    |    |    |			           |	|    |    |    |
 *    0 +----+----+----+----+			         0 +----+----+----+----+
 *      0                   255 		           0		       255
 */

typedef double CRMatrix[4][4];


/* p_buildmenu Structures */

typedef struct _MenuItem   MenuItem;

typedef void (*MenuItemCallback) (GtkWidget *widget,
				  gpointer   user_data);
struct _MenuItem
{
  char *label;
  char  unused_accelerator_key;
  int   unused_accelerator_mods;
  MenuItemCallback callback;
  gpointer user_data;
  MenuItem *unused_subitems;
  GtkWidget *widget;
};

typedef void (*ActionCallback) (GtkWidget *, gpointer);

typedef struct {
  char *label;
  ActionCallback callback;
  gpointer user_data;
  GtkWidget *widget;
} ActionAreaItem;

typedef struct {
   GDrawable *drawable;
   GPixelRgn  pr;
   gint       x1;
   gint       y1;
   gint       x2;
   gint       y2;
   gint       index_alpha;   /* 0 == no alpha, 1 == GREYA, 3 == RGBA */
   gint       bpp;
   GTile     *tile;
   gint       tile_row;
   gint       tile_col;
   gint       tile_width;
   gint       tile_height;
   gint       tile_dirty;
   gint       shadow;
   gint32     seldeltax;
   gint32     seldeltay;
   gint32     tile_swapcount;   
} t_GDRW;

typedef struct {
  gint32 y;
  guchar color[4];
} t_Last;


/*  curves action functions  */
static void  query (void);
static void  run (char *name,
                  int nparams,            /* number of parameters passed in */
                  GParam * param,         /* parameters passed in */
                  int *nreturn_vals,      /* number of parameters returned */
                  GParam ** return_vals); /* parameters to be returned */

static BenderDialog *  bender_new_dialog              (GDrawable *);
static void            bender_update                  (BenderDialog *, int);
static void            bender_plot_curve              (BenderDialog *, int, int, int, int, gint32, gint32, gint);
static void            bender_calculate_curve         (BenderDialog *, gint32, gint32, gint);
static void            bender_rotate_entry_callback   (GtkWidget *, gpointer);
static void            bender_upper_callback          (GtkWidget *, gpointer);
static void            bender_lower_callback          (GtkWidget *, gpointer);
static void            bender_smooth_callback         (GtkWidget *, gpointer);
static void            bender_free_callback           (GtkWidget *, gpointer);
static void            bender_reset_callback          (GtkWidget *, gpointer);
static void            bender_copy_callback           (GtkWidget *, gpointer);
static void            bender_copy_inv_callback       (GtkWidget *, gpointer);
static void            bender_swap_callback           (GtkWidget *, gpointer);
static void            bender_ok_callback             (GtkWidget *, gpointer);
static void            bender_cancel_callback         (GtkWidget *, gpointer);
static void            bender_smoothing_callback      (GtkWidget *, gpointer);
static void            bender_antialias_callback      (GtkWidget *, gpointer);
static void            bender_work_on_copy_callback   (GtkWidget *, gpointer);
static void            bender_preview_update          (GtkWidget *, gpointer);
static void            bender_preview_update_once     (GtkWidget *, gpointer);
static void            bender_load_callback           (GtkWidget *, gpointer);
static void            bender_save_callback           (GtkWidget *, gpointer);
static gint            bender_pv_widget_events        (GtkWidget *, GdkEvent *, BenderDialog *);
static gint            bender_graph_events            (GtkWidget *, GdkEvent *, BenderDialog *);
static void            bender_CR_compose              (CRMatrix, CRMatrix, CRMatrix);
static void            bender_init_min_max            (BenderDialog *, gint32);
static BenderDialog *  do_dialog                      (GDrawable *);
GtkWidget           *  p_buildmenu (MenuItem *);
static void      p_init_gdrw(t_GDRW *gdrw, GDrawable *drawable, int dirty, int shadow);
static void      p_end_gdrw(t_GDRW *gdrw);
static gint32    p_main_bend(BenderDialog *, GDrawable *, gint);
static gint32    p_create_pv_image( GDrawable *src_drawable, gint32 *layer_id);
static void      p_render_preview(BenderDialog *cd, gint32 layer_id);
static void      p_get_pixel( t_GDRW *gdrw, gint32 x, gint32 y, guchar *pixel);
static void      p_put_pixel(t_GDRW  *gdrw, gint32 x, gint32 y, guchar *pixel);
static void      p_put_mix_pixel(t_GDRW  *gdrw, gint32 x, gint32 y, guchar *color, gint32 nb_curvy, gint32 nb2_curvy, gint32 curvy);
static void      p_set_rotate_entry_text(BenderDialog *cd);
static void      p_stretch_curves(BenderDialog *cd, gint32 xmax, gint32 ymax);
static void      p_cd_to_bval(BenderDialog *cd, BenderValues *bval);
static void      p_cd_from_bval(BenderDialog *cd, BenderValues *bval);
static void      p_store_values(BenderDialog *cd);
static void      p_retrieve_values(BenderDialog *cd);
static void      p_bender_calculate_iter_curve (BenderDialog *cd, gint32 xmax, gint32 ymax);
static void      p_delta_gdouble(double *val, double val_from, double val_to, gint32 total_steps, gdouble current_step);
static void      p_delta_gint32(gint32 *val, gint32 val_from, gint32 val_to, gint32 total_steps, gdouble current_step);
static void      p_copy_points(BenderDialog *cd, int outline, int xy,  int argc, gdouble *floatarray);
static void      p_copy_yval(BenderDialog *cd, int outline,  int argc, gint8 *int8array);
static int       p_save_pointfile(BenderDialog *cd, char *filename);



/* Global Variables */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run     /* run_proc   */
};

static CRMatrix CR_basis =
{
  { -0.5,  1.5, -1.5,  0.5 },
  {  1.0, -2.5,  2.0, -0.5 },
  { -0.5,  0.0,  0.5,  0.0 },
  {  0.0,  1.0,  0.0,  0.0 },
};

static MenuItem outline_items[] =
{
  { N_("Upper"), 0, 0, bender_upper_callback, NULL, NULL, NULL },
  { N_("Lower"), 0, 0, bender_lower_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

static MenuItem curve_type_items[] =
{
  { N_("Smooth"), 0, 0, bender_smooth_callback, NULL, NULL, NULL },
  { N_("Free"), 0, 0, bender_free_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

int gb_debug = FALSE;

/* Functions */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX PDB_STUFF XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

/* ============================================================================
 * p_pdb_procedure_available
 *   if requested procedure is available in the PDB return the number of args
 *      (0 upto n) that are needed to call the procedure.
 *   if not available return -1
 * ============================================================================
 */

gint p_pdb_procedure_available(char *proc_name)
{    
  int             l_nparams;
  int             l_nreturn_vals;
  int             l_proc_type;
  char            *l_proc_blurb;
  char            *l_proc_help;
  char            *l_proc_author;
  char            *l_proc_copyright;
  char            *l_proc_date;
  GParamDef       *l_params;
  GParamDef       *l_return_vals;
  gint             l_rc;

  l_rc = 0;
  
  /* Query the gimp application's procedural database
   *  regarding a particular procedure.
   */
  if(gimp_query_procedure  (proc_name,
			        &l_proc_blurb,
			        &l_proc_help,
			        &l_proc_author,
			        &l_proc_copyright,
			        &l_proc_date,
			        &l_proc_type,
			        &l_nparams,
			        &l_nreturn_vals,
			        &l_params,
			        &l_return_vals))
  {
     /* procedure found in PDB */
     return (l_nparams);
  }

  printf("Warning: Procedure %s not found.\n", proc_name);
  return -1;
}	/* end p_pdb_procedure_available */


/* ============================================================================
 * p_gimp_rotate
 *  PDB call of 'gimp_rotate'
 * ============================================================================
 */

gint p_gimp_rotate(gint32 image_id, gint32 drawable_id, gint32 interpolation, gdouble angle_deg)
{
   static char     *l_rotate_proc = "gimp_rotate";
   GParam          *return_vals;
   int              nreturn_vals;
   gdouble          l_angle_rad;
   int              l_nparams;
   int              l_rc;

#ifdef ROTATE_OPTIMIZE
   static char     *l_rotate_proc2 = "plug_in_rotate";
   gint32 l_angle_step;

   if     (angle_deg == 90.0)  { l_angle_step = 1; }
   else if(angle_deg == 180.0) { l_angle_step = 2; }
   else if(angle_deg == 270.0) { l_angle_step = 3; }   
   else                        { l_angle_step = 0; }

   if(l_angle_step != 0)
   {
      l_nparams = p_pdb_procedure_available(l_rotate_proc2);
      if (l_nparams == 5)
      {
        /* use faster rotate plugin on multiples of 90 degrees */
        return_vals = gimp_run_procedure (l_rotate_proc2,
                                          &nreturn_vals,
			                  PARAM_INT32, RUN_NONINTERACTIVE,
			                  PARAM_IMAGE, image_id,
                                          PARAM_DRAWABLE, drawable_id,
			                  PARAM_INT32, l_angle_step,
			                  PARAM_INT32, FALSE,         /* dont rotate the whole image */
                                          PARAM_END);
        if (return_vals[0].data.d_status == STATUS_SUCCESS)
        {
          return 0;
        }
      
      }
   
   }
#endif

   l_rc = -1;
   l_angle_rad = (angle_deg * 3.14159) / 180.0;

   l_nparams = p_pdb_procedure_available(l_rotate_proc);
   if (l_nparams >= 0)
   {
         /* use the new Interface (Gimp 1.1 style)
          * (1.1 knows the image_id where the drawable belongs to)
          */
        return_vals = gimp_run_procedure (l_rotate_proc,
                                          &nreturn_vals,
                                          PARAM_DRAWABLE, drawable_id,
			                  PARAM_INT32, interpolation,
			                  PARAM_FLOAT, l_angle_rad,
                                          PARAM_END);
     if (return_vals[0].data.d_status == STATUS_SUCCESS)
     {
        l_rc = 0;
     }
     else
     {
       printf("Error: %s call failed %d\n", l_rotate_proc, (int)return_vals[0].data.d_status);
     }

     gimp_destroy_params (return_vals, nreturn_vals);
   }
   else
   {
      printf("Error: Procedure %s not found.\n",l_rotate_proc);
   }
   return (l_rc);

}  /* end p_gimp_rotate */

/* ============================================================================
 * p_gimp_edit_copy
 *   
 * ============================================================================
 */

gint32  p_gimp_edit_copy(gint32 image_id, gint32 drawable_id)
{
   static char     *l_procname = "gimp_edit_copy";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_procname) >= 0)
   {
      return_vals = gimp_run_procedure (l_procname,
                                    &nreturn_vals,
                                    PARAM_DRAWABLE,  drawable_id,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         gimp_destroy_params (return_vals, nreturn_vals);
         return 0;
      }
      printf("Error: PDB call of %s failed status:%d\n", l_procname, (int)return_vals[0].data.d_status);
   }

   return(-1);
}	/* end p_gimp_edit_copy */

/* ============================================================================
 * p_gimp_edit_paste
 *   
 * ============================================================================
 */

gint32  p_gimp_edit_paste(gint32 image_id, gint32 drawable_id, gint32 paste_into)
{
   static char     *l_procname = "gimp_edit_paste";
   GParam          *return_vals;
   int              nreturn_vals;
   gint32           fsel_layer_id;

   if (p_pdb_procedure_available(l_procname) >= 0)
   {
      return_vals = gimp_run_procedure (l_procname,
                                    &nreturn_vals,
                                    PARAM_DRAWABLE,  drawable_id,
                                    PARAM_INT32,     paste_into,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         fsel_layer_id = return_vals[1].data.d_layer;
         gimp_destroy_params (return_vals, nreturn_vals);
         return (fsel_layer_id);
      }
      printf("Error: PDB call of %s failed status:%d\n", l_procname, (int)return_vals[0].data.d_status);
   }

   return(-1);
}	/* end p_gimp_edit_paste */

/* ============================================================================
 * p_get_gimp_selection_bounds
 *   
 * ============================================================================
 */
gint
p_get_gimp_selection_bounds (gint32 image_id, gint32 *x1, gint32 *y1, gint32 *x2, gint32 *y2)
{
   static char     *l_get_sel_bounds_proc = "gimp_selection_bounds";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_get_sel_bounds_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_END);
                                 
   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      *x1 = return_vals[2].data.d_int32;
      *y1 = return_vals[3].data.d_int32;
      *x2 = return_vals[4].data.d_int32;
      *y2 = return_vals[5].data.d_int32;
      return(return_vals[1].data.d_int32);
   }
   printf("Error: PDB call of %s failed staus=%d\n", 
          l_get_sel_bounds_proc, (int)return_vals[0].data.d_status);
   return(FALSE);
}	/* end p_get_gimp_selection_bounds */

/* ============================================================================
 * p_if_selection_float_it
 * ============================================================================
 */

gint32
p_if_selection_float_it(gint32 image_id, gint32 layer_id)
{
  gint32   l_sel_channel_id;
  gint32   l_layer_id;
  gint32  l_x1, l_x2, l_y1, l_y2;

  l_layer_id = layer_id;
  if(!gimp_layer_is_floating_selection (layer_id)  )
  {
    /* check and see if we have a selection mask */
    l_sel_channel_id  = gimp_image_get_selection(image_id);     

    if((p_get_gimp_selection_bounds(image_id, &l_x1, &l_y1, &l_x2, &l_y2))
    && (l_sel_channel_id >= 0))
    {
	/* selection is TRUE, make a layer (floating selection) from the selection  */
        p_gimp_edit_copy(image_id, layer_id);
	l_layer_id = p_gimp_edit_paste(image_id, layer_id, FALSE);

    }
  }

  return(l_layer_id);
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX END_PDB_STUFF XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

/*
 *    M      M    AAAAA    IIIIII    N     N
 *    M M  M M   A     A     II      NN    N
 *    M  M   M   AAAAAAA     II      N  N  N 
 *    M      M   A     A     II      N    NN
 *    M      M   A     A   IIIIII    N     N
 */

MAIN ()

static void query (void)
{
  static GParamDef args[] = {
                  { PARAM_INT32,      "run_mode", "Interactive, non-interactive"},
                  { PARAM_IMAGE,      "image", "Input image" },
                  { PARAM_DRAWABLE,   "drawable", "Input drawable (must be a layer without layermask)"},
                  { PARAM_FLOAT,      "rotation", "Direction {angle 0 to 360 degree } of the bend effect"},
                  { PARAM_INT32,      "smoothing", "Smoothing { TRUE, FALSE }"},
                  { PARAM_INT32,      "antialias", "Antialias { TRUE, FALSE }"},
                  { PARAM_INT32,      "work_on_copy", "{ TRUE, FALSE } TRUE: copy the drawable and bend the copy"},
                  { PARAM_INT32,      "curve_type", " { 0, 1 } 0 == smooth (use 17 points), 1 == freehand (use 256 val_y) "},
                  { PARAM_INT32,      "argc_upper_point_x", "{2 <= argc <= 17} "},
                  { PARAM_FLOATARRAY, "upper_point_x", "array of 17 x point_koords { 0.0 <= x <= 1.0 or -1 for unused point }"},
                  { PARAM_INT32,      "argc_upper_point_y", "{2 <= argc <= 17} "},
                  { PARAM_FLOATARRAY, "upper_point_y", "array of 17 y point_koords { 0.0 <= y <= 1.0 or -1 for unused point }"},
                  { PARAM_INT32,      "argc_lower_point_x", "{2 <= argc <= 17} "},
                  { PARAM_FLOATARRAY, "lower_point_x", "array of 17 x point_koords { 0.0 <= x <= 1.0 or -1 for unused point }"},
                  { PARAM_INT32,      "argc_lower_point_y", "{2 <= argc <= 17} "},
                  { PARAM_FLOATARRAY, "lower_point_y", "array of 17 y point_koords { 0.0 <= y <= 1.0 or -1 for unused point }"},
                  { PARAM_INT32,      "argc_upper_val_y", "{ 256 } "},
                  { PARAM_INT8ARRAY,  "upper_val_y",   "array of 256 y freehand koord { 0 <= y <= 255 }"},
                  { PARAM_INT32,      "argc_lower_val_y", "{ 256 } "},
                  { PARAM_INT8ARRAY,  "lower_val_y",   "array of 256 y freehand koord { 0 <= y <= 255 }"}
  };
  static int nargs = sizeof(args) / sizeof(args[0]);
  
  static GParamDef return_vals[] =
  {
    { PARAM_LAYER, "bent_layer", "the handled layer" }
  };
  static int nreturn_vals = sizeof(return_vals) / sizeof(return_vals[0]);


  static GParamDef args_iter[] =
  {
    {PARAM_INT32, "run_mode", "non-interactive"},
    {PARAM_INT32, "total_steps", "total number of steps (# of layers-1 to apply the related plug-in)"},
    {PARAM_FLOAT, "current_step", "current (for linear iterations this is the layerstack position, otherwise some value inbetween)"},
    {PARAM_INT32, "len_struct", "length of stored data structure with id is equal to the plug_in  proc_name"},
  };
  static int nargs_iter = sizeof(args_iter) / sizeof(args_iter[0]);

  static GParamDef *return_iter = NULL;
  static int nreturn_iter = 0;

  /* the actual installation of the bend plugin */
  gimp_install_procedure (PLUG_IN_NAME,
                          PLUG_IN_DESCRIBTION,
                          "This plug-in does bend the active layer "
			  "If there is a current selection it is copied to floating selection "
			  "and the curve_bend distortion is done on the floating selection. "
                          "If work_on_copy parameter is TRUE, the curve_bend distortion is done "
			  "on a copy of the active layer (or floating selection). "
                          "The upper and lower edges are bent in shape of 2 spline curves. "
                          "both (upper and lower) curves are determined by upto 17 points "
                          "or by 256 Y-Values if curve_type == 1 (freehand mode) "
                          "If rotation is not 0, the layer is rotated before "
                          "and rotated back after the bend operation. This enables "
                          "bending in other directions than vertical."
                          "bending usually changes the size of the handled layer."
                          "this plugin sets the offsets of the handled layer to keep its center at the same position"
                          ,
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          PLUG_IN_VERSION,
                          N_("<Image>/Filters/Distorts/CurveBend..."),
                          PLUG_IN_IMAGE_TYPES,
                          PROC_PLUG_IN,
                          nargs,
                          nreturn_vals,
                          args,
                          return_vals);
 
 
  /* the installation of the Iterator extension for the bend plugin */
  gimp_install_procedure (PLUG_IN_ITER_NAME,
                          "This extension calculates the modified values for one iterationstep for the call of plug_in_curve_bend",
                          "",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          PLUG_IN_VERSION,
                          NULL,    /* do not appear in menus */
                          NULL,
                          PROC_EXTENSION,
                          nargs_iter, nreturn_iter,
                          args_iter, return_iter);
}

static void
run (char *name,                /* name of plugin */
     int nparams,               /* number of in-paramters */
     GParam * param,            /* in-parameters */
     int *nreturn_vals,         /* number of out-parameters */
     GParam ** return_vals)     /* out-parameters */
{
  char       *l_env;
  BenderDialog *cd;
  gint      l_nreturn_vals;
  
   GDrawable *l_active_drawable = NULL;
   gint32    l_image_id = -1;
   gint32    l_layer_id = -1;
   gint32    l_layer_mask_id = -1;
   gint32    l_bent_layer_id = -1;

  /* Get the runmode from the in-parameters */
  GRunModeType run_mode = param[0].data.d_int32;

  /* status variable, use it to check for errors in invocation usualy only
     during non-interactive calling */
  GStatusType status = STATUS_SUCCESS;

  /*always return at least the status to the caller. */
  static GParam values[2];


  if (run_mode == RUN_INTERACTIVE) {
    INIT_I18N_UI();
  } else {
    INIT_I18N();
  }

   cd = NULL;

  l_env = g_getenv("BEND_DEBUG");
  if(l_env != NULL)
  {
    if((*l_env != 'n') && (*l_env != 'N')) gb_debug = 1;
  }
  
  if(gb_debug) fprintf(stderr, "\n\nDEBUG: run %s\n", name);

  /* initialize the return of the status */
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  values[1].type = PARAM_LAYER;
  values[1].data.d_int32 = -1;
  *nreturn_vals = 2;
  *return_vals = values;

  if (strcmp (name, PLUG_IN_ITER_NAME) == 0)
  {
     gint32  len_struct;
     gint32  total_steps;
     gdouble current_step;
     BenderValues   bval;                  /* current values while iterating */
     BenderValues   bval_from, bval_to;    /* start and end values */

        /* Iterator procedure for animated calls is usually called from
         * "plug_in_gap_layers_run_animfilter"
         * (always run noninteractive)
         */
        if ((run_mode == RUN_NONINTERACTIVE) && (nparams == 4))
        {
          total_steps  =  param[1].data.d_int32;
          current_step =  param[2].data.d_float;
          len_struct   =  param[3].data.d_int32;

          if(len_struct == sizeof(bval))
          {
            /* get _FROM and _TO data, 
             * This data was stored by plug_in_gap_layers_run_animfilter
             */


               gimp_get_data(PLUG_IN_DATA_ITER_FROM, &bval_from); 
               gimp_get_data(PLUG_IN_DATA_ITER_TO,   &bval_to); 
               memcpy(&bval, &bval_from, sizeof(bval));
    
               p_delta_gdouble(&bval.rotation, bval_from.rotation, bval_to.rotation, total_steps, current_step);
               /* note: iteration of curve and points arrays would not give useful results.
                *       (there might be different number of points in the from/to bender values )
                *       the iteration is done later, (see p_bender_calculate_iter_curve)
                *       when the curve is calculated.
                */

               bval.total_steps = total_steps;
               bval.current_step = current_step;


               gimp_set_data(PLUG_IN_NAME, &bval, sizeof(bval));
          }
          else status = STATUS_CALLING_ERROR;
        }
        else status = STATUS_CALLING_ERROR;

        values[0].data.d_status = status;
        return;
  }




  /* get image and drawable */
  l_image_id = param[1].data.d_int32;
  l_layer_id = param[2].data.d_drawable;

  gimp_run_procedure ("gimp_undo_push_group_start", &l_nreturn_vals,
                      PARAM_IMAGE, l_image_id, PARAM_END);
  
  if(!gimp_drawable_is_layer(l_layer_id))
  {
     gimp_message(PLUG_IN_PRINT_NAME " operates on layers only (but was called on channel or mask)");
     printf("Passed drawable is no Layer\n");
     status = STATUS_EXECUTION_ERROR; 
  }
  /* check for layermask */
  l_layer_mask_id = gimp_layer_get_mask_id(l_layer_id);
  if(l_layer_mask_id >= 0)
  {
     /* apply the layermask
      *   some transitions (especially rotate) cant operate proper on
      *   layers with masks !
      */
      gimp_image_remove_layer_mask(l_image_id, l_layer_id, 0 /* 0==APPLY */ );
  }

  /* if there is a selection, make it the floating selection layer */
  l_active_drawable = gimp_drawable_get (p_if_selection_float_it(l_image_id, l_layer_id));


  /* how are we running today? */
  if(status == STATUS_SUCCESS)
  {
    /* how are we running today? */
    switch (run_mode)
     {
      case RUN_INTERACTIVE:
        /* Possibly retrieve data from a previous run */
        /* gimp_get_data (PLUG_IN_NAME, &g_bndvals); */

        /* Get information from the dialog */
        cd = do_dialog(l_active_drawable);
        cd->show_progress = TRUE;
        break;

      case RUN_NONINTERACTIVE:
        /* check to see if invoked with the correct number of parameters */
        if (nparams >= 20)
        {
           cd = g_malloc (sizeof (BenderDialog));
           cd->run = TRUE;
           cd->show_progress = FALSE;
           cd->drawable = l_active_drawable;

           cd->rotation      = (gdouble) param[3].data.d_float;
           cd->smoothing     = (gint) param[4].data.d_int32;
           cd->antialias     = (gint) param[5].data.d_int32;
           cd->work_on_copy  = (gint) param[6].data.d_int32;
           cd->curve_type    = (int)  param[7].data.d_int32;

           p_copy_points(cd, OUTLINE_UPPER, 0, (int)param[8].data.d_int32,  param[9].data.d_floatarray);
           p_copy_points(cd, OUTLINE_UPPER, 1, (int)param[10].data.d_int32,  param[11].data.d_floatarray);
           p_copy_points(cd, OUTLINE_LOWER, 0, (int)param[12].data.d_int32,  param[13].data.d_floatarray);
           p_copy_points(cd, OUTLINE_LOWER, 1, (int)param[14].data.d_int32,  param[15].data.d_floatarray);

           p_copy_yval(cd, OUTLINE_UPPER, (int)param[16].data.d_int32, param[17].data.d_int8array);
           p_copy_yval(cd, OUTLINE_UPPER, (int)param[18].data.d_int32, param[19].data.d_int8array);

        }
        else
        {
          status = STATUS_CALLING_ERROR;
        }

        break;

      case RUN_WITH_LAST_VALS:
        cd = g_malloc (sizeof (BenderDialog));
        cd->run = TRUE;
        cd->show_progress = TRUE;
        cd->drawable = l_active_drawable;
        p_retrieve_values(cd);  /* Possibly retrieve data from a previous run */
        break;

      default:
        break;
    }
  }

  if (cd == NULL)
  {
    status = STATUS_EXECUTION_ERROR;
  }

  if (status == STATUS_SUCCESS)
  {
    /* Run the main function */

     if(cd->run)
     {
        l_bent_layer_id = p_main_bend(cd, cd->drawable, cd->work_on_copy);

	/* Store variable states for next run */
	if (run_mode == RUN_INTERACTIVE)
	{
            p_store_values(cd);
	}     
    }
    else
    {
      status = STATUS_EXECUTION_ERROR;       /* dialog ended with cancel button */
    }

    /* If run mode is interactive, flush displays, else (script) don't
       do it, as the screen updates would make the scripts slow */
    if (run_mode != RUN_NONINTERACTIVE)
      gimp_displays_flush ();

  }
  values[0].data.d_status = status;
  values[1].data.d_int32 = l_bent_layer_id;   /* return the id of handled layer */

  gimp_run_procedure ("gimp_undo_push_group_end", &l_nreturn_vals,
                      PARAM_IMAGE, l_image_id, PARAM_END);

  if(gb_debug) printf("end run curve_bend plugin\n");
  
}	/* end run */


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/*  Last_VALUEs and ITERATOR stuff   */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
int
p_save_pointfile(BenderDialog *cd, char *filename)
{
  int j;
  FILE *l_fp;

  if(gb_debug) printf("Saving curve to file:%s\n", filename);

  l_fp = fopen(filename, "w+");
  if(l_fp == NULL)
  {
    return -1;
  }

  fprintf(l_fp, "%s\n", KEY_POINTFILE);
  fprintf(l_fp, "VERSION 1.0\n\n");

  fprintf(l_fp, "# points for upper and lower smooth curve (0.0 <= pt <= 1.0)\n");
  fprintf(l_fp, "# there are upto 17 points where unused points are set to -1\n");
  fprintf(l_fp, "#       UPPERX     UPPERY      LOWERX    LOWERY\n");
  fprintf(l_fp, "\n");

  for(j=0; j < 17; j++)
  {
      fprintf(l_fp, "%s %+.6f  %+.6f   %+.6f  %+.6f\n", KEY_POINTS,
                  (float)cd->points[OUTLINE_UPPER][j][0], 
                  (float)cd->points[OUTLINE_UPPER][j][1],
                  (float)cd->points[OUTLINE_LOWER][j][0], 
                  (float)cd->points[OUTLINE_LOWER][j][1] );
  }

  fprintf(l_fp, "\n");
  fprintf(l_fp, "# y values for upper/lower freehand curve (0 <= y <= 255) \n");
  fprintf(l_fp, "# there must be exactly 256 y values \n");
  fprintf(l_fp, "#     UPPER_Y  LOWER_Y\n");
  fprintf(l_fp, "\n");

  for(j=0; j < 256; j++)
  {
    fprintf(l_fp, "%s %3d  %3d\n", KEY_VAL_Y, 
               (int)cd->curve[OUTLINE_UPPER][j],
               (int)cd->curve[OUTLINE_LOWER][j]);
  }

  fclose(l_fp);
  return 0; /* OK */
}	/* end p_save_pointfile */


int
p_load_pointfile(BenderDialog *cd, char *filename)
{
  int   l_pi, l_ci, l_n, l_len;
  FILE *l_fp;
  char  l_buff[2000];
  float l_fux, l_fuy, l_flx, l_fly;
  int   l_iuy, l_ily ;
  
  if(gb_debug) printf("Loading curve from file:%s\n", filename);

  l_fp = fopen(filename, "r");
  if(l_fp == NULL)
  {
    return -1;
  }

  l_pi= 0;
  l_ci= 0;

  fgets (l_buff, 2000-1, l_fp);
  if(strncmp(l_buff, KEY_POINTFILE, strlen(KEY_POINTFILE)) == 0)
  {
     while (NULL != fgets (l_buff, 2000-1, l_fp))
     {
        if(gb_debug) printf("FGETS: %s\n", l_buff);
        l_len = strlen(KEY_POINTS);
        if(strncmp(l_buff, KEY_POINTS, l_len) == 0)
        {
           l_n = sscanf(&l_buff[l_len], "%f %f %f %f", &l_fux, &l_fuy, &l_flx, &l_fly);
           if((l_n == 4) && (l_pi < 17))
           {
             cd->points[OUTLINE_UPPER][l_pi][0] = l_fux;
             cd->points[OUTLINE_UPPER][l_pi][1] = l_fuy;
             cd->points[OUTLINE_LOWER][l_pi][0] = l_flx;
             cd->points[OUTLINE_LOWER][l_pi][1] = l_fly;
             if(gb_debug) printf("OK points[%d]\n", l_pi);     
             l_pi++;
           }
	   else
	   {
	      printf("warnig: BAD points[%d] in file %s are ignored\n", l_pi, filename);
	   }
        }
        l_len = strlen(KEY_VAL_Y);
        if(strncmp(l_buff, KEY_VAL_Y, l_len) == 0)
        {
           l_n = sscanf(&l_buff[l_len], "%d %d", &l_iuy, &l_ily);
           if((l_n == 2) && (l_ci < 256))
           {
             cd->curve[OUTLINE_UPPER][l_ci] = l_iuy;
             cd->curve[OUTLINE_LOWER][l_ci] = l_ily;
             l_ci++;
             if(gb_debug) printf("OK y_val[%d]\n", l_pi);     
           }
	   else
	   {
	      printf("warnig: BAD y_vals[%d] in file %s are ignored\n", l_ci, filename);
	   }
        }

     }
  }
  fclose(l_fp);
  return 0; /* OK */
}	/* end p_load_pointfile */



void
p_cd_to_bval(BenderDialog *cd, BenderValues *bval)
{
  int i,j;

  for(i=0; i<2; i++)
  {
    for(j=0; j<256; j++)
    {
      bval->curve[i][j] = cd->curve[i][j];
    }

    for(j=0; j<17; j++)
    {
      bval->points[i][j][0] = cd->points[i][j][0];  /* x */
      bval->points[i][j][1] = cd->points[i][j][1];  /* y */
    }
  
  }
  
  bval->curve_type = cd->curve_type;
  bval->smoothing = cd->smoothing;
  bval->antialias = cd->antialias;
  bval->work_on_copy = cd->work_on_copy;
  bval->rotation = cd->rotation;

  bval->total_steps = 0;
  bval->current_step =0.0;
}

void
p_cd_from_bval(BenderDialog *cd, BenderValues *bval)
{
  int i,j;

  for(i=0; i<2; i++)
  {
    for(j=0; j<256; j++)
    {
      cd->curve[i][j] = bval->curve[i][j];
    }

    for(j=0; j<17; j++)
    {
      cd->points[i][j][0] = bval->points[i][j][0];  /* x */
      cd->points[i][j][1] = bval->points[i][j][1];  /* y */
    }
  
  }
  
  cd->curve_type = bval->curve_type;
  cd->smoothing = bval->smoothing;
  cd->antialias = bval->antialias;
  cd->work_on_copy = bval->work_on_copy;
  cd->rotation = bval->rotation;

}

void
p_store_values(BenderDialog *cd)
{
  BenderValues l_bval;
  
  p_cd_to_bval(cd, &l_bval);
  gimp_set_data(PLUG_IN_NAME, &l_bval, sizeof(l_bval));
}

void
p_retrieve_values(BenderDialog *cd)
{
  BenderValues l_bval;

  l_bval.total_steps = 0;
  l_bval.current_step = -444.4;  /* init with an invalid  dummy value */
  
  gimp_get_data (PLUG_IN_NAME, &l_bval);

  if(l_bval.total_steps == 0)
  {
    cd->bval_from = NULL;
    cd->bval_to = NULL;
    if(l_bval.current_step != -444.4)
    {
       /* last_value data was retrieved (and dummy value was overwritten) */
       p_cd_from_bval(cd, &l_bval);
    }
  }
  else
  {
    cd->bval_from = g_malloc(sizeof(BenderValues));
    cd->bval_to   = g_malloc(sizeof(BenderValues));
    cd->bval_curr  = g_malloc(sizeof(BenderValues));
    memcpy(cd->bval_curr, &l_bval, sizeof(l_bval));
   
    /* it seems that we are called from GAP with "Varying Values" */
    gimp_get_data(PLUG_IN_DATA_ITER_FROM, cd->bval_from);
    gimp_get_data(PLUG_IN_DATA_ITER_TO,   cd->bval_to);
    memcpy(cd->bval_curr, &l_bval, sizeof(l_bval));
    p_cd_from_bval(cd, cd->bval_curr);
    cd->work_on_copy = FALSE;
  }
  
}


static void p_delta_gdouble(double *val, double val_from, double val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
}

static void p_delta_gint32(gint32 *val, gint32 val_from, gint32 val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
}


void
p_copy_points(BenderDialog *cd, int outline, int xy, int argc, gdouble *floatarray)
{
   int j;
   
   for(j=0; j < 17; j++)
   {
     cd->points[outline][j][xy] = -1;
   }
   for(j=0; j < argc; j++)
   {
     cd->points[outline][j][xy] = floatarray[j];
   }
}

void
p_copy_yval(BenderDialog *cd, int outline, int argc, gint8 *int8array)
{
   int j;
   guchar fill;
   
   fill = MIDDLE;
   for(j=0; j < 256; j++)
   {
     if(j < argc) 
     {
        fill = cd->curve[outline][j] = int8array[j];
     }
     else
     {
        cd->curve[outline][j] = fill;
     }
   }
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/*  curves machinery  */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

BenderDialog *
do_dialog (GDrawable *drawable)
{
  BenderDialog *cd;
  int i;
  gint argc = 1;
  guchar     *color_cube;
  gchar **argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("CurveBend");


  /* Init GTK  */
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma(gimp_gamma());
  gtk_preview_set_install_cmap(gimp_install_cmap());
  color_cube = gimp_color_cube();
  gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);
  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap(gtk_preview_get_cmap());


  /*  The curve_bend dialog  */
  cd = bender_new_dialog (drawable);

  /* create temporary image (with a small copy of drawable) for the preview */
  cd->preview_image_id = p_create_pv_image(drawable, &cd->preview_layer_id1);
  cd->preview_layer_id2 = -1;
  
  /* show the outline menu   */
  for (i = 0; i < 2; i++)
  {
       gtk_widget_set_sensitive( outline_items[i].widget, TRUE);
  }
  
  /* set the current selection */
  gtk_option_menu_set_history ( GTK_OPTION_MENU (cd->outline_menu), 0);

  if (!GTK_WIDGET_VISIBLE (cd->shell))
    gtk_widget_show (cd->shell);


  bender_update (cd, UP_GRAPH | UP_DRAW);

  gtk_main ();
  gdk_flush ();

  gimp_image_delete(cd->preview_image_id);
  cd->preview_image_id = -1;
  cd->preview_layer_id1 = -1;
  cd->preview_layer_id2 = -1;
  
  if(gb_debug) printf("do_dialog END\n");  
  return cd;
}


/**************************/
/*  Select Curves dialog  */
/**************************/

static BenderDialog *
bender_new_dialog (GDrawable *drawable)
{
  BenderDialog *cd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *option_menu;
  GtkWidget *outline_hbox;
  GtkWidget *menu;
  GtkWidget *table;
  GtkWidget *button;
  int i, j;

  cd = g_malloc (sizeof (BenderDialog));

  cd->preview = FALSE;
  cd->curve_type = SMOOTH;
  cd->pixmap = NULL;
  cd->filesel = NULL;
  cd->outline = OUTLINE_UPPER;
  cd->show_progress = FALSE;
  cd->smoothing = TRUE;
  cd->antialias = TRUE;
  cd->work_on_copy = FALSE;
  cd->rotation = 0.0;       /* vertical bend */

  cd->drawable = drawable;
  cd->color = gimp_drawable_is_rgb (cd->drawable->id);

  cd->run = FALSE;
  cd->bval_from = NULL;
  cd->bval_to = NULL;
  cd->bval_curr = NULL;

  for (i = 0; i < 2; i++)
    for (j = 0; j < 256; j++)
      cd->curve[i][j] = MIDDLE;

  cd->grab_point = -1;
  for (i = 0; i < 2; i++)
  {
      for (j = 0; j < 17; j++)
      {
	  cd->points[i][j][0] = -1;
	  cd->points[i][j][1] = -1;
      }
      cd->points[i][0][0] = 0.0;        /* x */
      cd->points[i][0][1] = 0.5;        /* y */
      cd->points[i][16][0] = 1.0;       /* x */
      cd->points[i][16][1] = 0.5;       /* y */
  }


  p_retrieve_values(cd);       /* Possibly retrieve data from a previous run */

  for (i = 0; i < 2; i++)
    outline_items [i].user_data = (gpointer) cd;
  for (i = 0; i < 2; i++)
    curve_type_items [i].user_data = (gpointer) cd;

  /*  The shell and main vbox  */
  cd->shell = gimp_dialog_new (_("Curve Bend"), "curve_bend",
			       gimp_plugin_help_func, "filters/curve_bend.html",
			       GTK_WIN_POS_MOUSE,
			       FALSE, TRUE, FALSE,

			       _("Reset"), bender_reset_callback,
			       cd, NULL, NULL, FALSE, FALSE,
			       _("Copy"), bender_copy_callback,
			       cd, NULL, NULL, FALSE, FALSE,
			       _("CopyInv"), bender_copy_inv_callback,
			       cd, NULL, NULL, FALSE, FALSE,
			       _("Swap"), bender_swap_callback,
			       cd, NULL, NULL, FALSE, FALSE,
			       _("OK"), bender_ok_callback,
			       cd, NULL, NULL, TRUE, FALSE,
			       _("Cancel"), bender_cancel_callback,
			       cd, NULL, NULL, FALSE, TRUE,

			       NULL);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cd->shell)->vbox), vbox,
		      TRUE, TRUE, 0);

  /*  The option menu for selecting outlines  */
  outline_hbox = gtk_hbox_new (FALSE, 2+4);
  gtk_box_pack_start (GTK_BOX (vbox), outline_hbox, FALSE, FALSE, 0);

  /*  The Load button  */
  button = gtk_button_new_with_label (_("LoadCurve"));
  gtk_box_pack_start (GTK_BOX (outline_hbox), button, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (bender_load_callback),
		      cd);
  gtk_widget_show (button);

  /*  The Save button  */
  button = gtk_button_new_with_label (_("SaveCurve"));
  gtk_box_pack_start (GTK_BOX (outline_hbox), button, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (bender_save_callback),
		      cd);
  gtk_widget_show (button);

  label = gtk_label_new (_("Rotate: "));
  gtk_box_pack_start (GTK_BOX (outline_hbox), label, FALSE, FALSE, 0);

  cd->rotate_entry = gtk_entry_new ();
  gtk_signal_connect (GTK_OBJECT (cd->rotate_entry), "changed",
		      GTK_SIGNAL_FUNC (bender_rotate_entry_callback),
		      cd);
  gtk_widget_set_usize (GTK_WIDGET (cd->rotate_entry), ENTRY_WIDTH,0 );
  gtk_box_pack_start (GTK_BOX (outline_hbox), cd->rotate_entry, FALSE, FALSE, 2);
  gtk_widget_show (label);
  gtk_widget_show (cd->rotate_entry);
  p_set_rotate_entry_text (cd);

  label = gtk_label_new (_("Curve for Border: "));
  gtk_box_pack_start (GTK_BOX (outline_hbox), label, FALSE, FALSE, 0);

  menu = p_buildmenu (outline_items);
  cd->outline_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (outline_hbox), cd->outline_menu, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (cd->outline_menu);
  gtk_widget_show (outline_hbox);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (cd->outline_menu), menu);

  /*  The option menu for selecting the drawing method  */
  label = gtk_label_new (_("Curve Type: "));
  gtk_box_pack_start (GTK_BOX (outline_hbox), label, FALSE, FALSE, 0);

  menu = p_buildmenu (curve_type_items);
  option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (outline_hbox), option_menu, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (option_menu);
  gtk_widget_show (outline_hbox);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);


  /*  The table for the pv_widget preview  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The range drawing area  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);

  cd->pv_widget = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (cd->pv_widget), PREVIEW_SIZE_X, PREVIEW_SIZE_Y);
  gtk_widget_set_events (cd->pv_widget, RANGE_MASK);
  gtk_signal_connect (GTK_OBJECT (cd->pv_widget), "event",
		      (GtkSignalFunc) bender_pv_widget_events,
		      cd);
  gtk_container_add (GTK_CONTAINER (frame), cd->pv_widget);
  gtk_widget_show (cd->pv_widget);
  gtk_widget_show (frame);

  /*  The curves graph  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_FILL, 0, 0);

  cd->graph = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (cd->graph),
			 GRAPH_WIDTH + RADIUS * 2,
			 GRAPH_HEIGHT + RADIUS * 2);
  gtk_widget_set_events (cd->graph, GRAPH_MASK);
  gtk_signal_connect (GTK_OBJECT (cd->graph), "event",
		      (GtkSignalFunc) bender_graph_events,
		      cd);
  gtk_container_add (GTK_CONTAINER (frame), cd->graph);
  gtk_widget_show (cd->graph);
  gtk_widget_show (frame);

  gtk_widget_show (table);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (FALSE, 2+3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);


  /*  The preview button  */
  button = gtk_button_new_with_label (_("PreviewOnce"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc) bender_preview_update_once,
			    cd);
  gtk_widget_show (button);


  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cd->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) bender_preview_update,
		      cd);

  gtk_widget_show (toggle);

  /*  The smoothing toggle  */
  toggle = gtk_check_button_new_with_label (_("Smoothing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cd->smoothing);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) bender_smoothing_callback,
		      cd);

  gtk_widget_show (toggle);

  /*  The antialias toggle  */
  toggle = gtk_check_button_new_with_label (_("Antialias"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cd->antialias);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) bender_antialias_callback,
		      cd);

  gtk_widget_show (toggle);

  /*  The wor_on_copy toggle  */
  toggle = gtk_check_button_new_with_label (_("Work on Copy"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cd->work_on_copy);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) bender_work_on_copy_callback,
		      cd);

  gtk_widget_show (toggle);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);

  return cd;
}

static void
bender_update (BenderDialog *cd,
	       int           update)
{
  int i;
  int other;

  if (update & UP_PREVIEW)
    {
      if (cd->preview_layer_id2 >= 0)
         gimp_image_remove_layer(cd->preview_image_id, cd->preview_layer_id2);

      cd->preview_layer_id2 = p_main_bend(cd, gimp_drawable_get (cd->preview_layer_id1), TRUE /* work_on_copy*/ ); 
      p_render_preview(cd, cd->preview_layer_id2);
      
      if (update & UP_DRAW)
	gtk_widget_draw (cd->pv_widget, NULL);
    }
  if (update & UP_PREVIEW_EXPOSE)
    {
      /* on expose just redraw cd->preview_layer_id2 
       * that holds the bent version of the preview (if there is one) 
       */
      if (cd->preview_layer_id2 < 0)
         cd->preview_layer_id2 = p_main_bend(cd, gimp_drawable_get (cd->preview_layer_id1), TRUE /* work_on_copy*/ ); 
      p_render_preview(cd, cd->preview_layer_id2);
      
      if (update & UP_DRAW)
	gtk_widget_draw (cd->pv_widget, NULL);
    }
  if ((update & UP_GRAPH) && (update & UP_DRAW) && cd->pixmap != NULL)
    {
      GdkPoint points[256];

      /*  Clear the pixmap  */
      gdk_draw_rectangle (cd->pixmap, cd->graph->style->bg_gc[GTK_STATE_NORMAL],
			  TRUE, 0, 0, GRAPH_WIDTH + RADIUS * 2, GRAPH_HEIGHT + RADIUS * 2);

      /*  Draw the grid lines  */
      for (i = 0; i < 5; i++)
	{
	  gdk_draw_line (cd->pixmap, cd->graph->style->dark_gc[GTK_STATE_NORMAL],
			 RADIUS, i * (GRAPH_HEIGHT / 4) + RADIUS,
			 GRAPH_WIDTH + RADIUS, i * (GRAPH_HEIGHT / 4) + RADIUS);
	  gdk_draw_line (cd->pixmap, cd->graph->style->dark_gc[GTK_STATE_NORMAL],
			 i * (GRAPH_WIDTH / 4) + RADIUS, RADIUS,
			 i * (GRAPH_WIDTH / 4) + RADIUS, GRAPH_HEIGHT + RADIUS);
	}

      /*  Draw the other curve  */
      if(cd->outline == 0) { other = 1; }
      else                 { other = 0; }
      
      for (i = 0; i < 256; i++)
	{
	  points[i].x = i + RADIUS;
	  points[i].y = 255 - cd->curve[other][i] + RADIUS;
	}
      gdk_draw_points (cd->pixmap, cd->graph->style->dark_gc[GTK_STATE_NORMAL], points, 256);


      /*  Draw the active curve  */
      for (i = 0; i < 256; i++)
	{
	  points[i].x = i + RADIUS;
	  points[i].y = 255 - cd->curve[cd->outline][i] + RADIUS;
	}
      gdk_draw_points (cd->pixmap, cd->graph->style->black_gc, points, 256);

      /*  Draw the points  */
      if (cd->curve_type == SMOOTH)
	for (i = 0; i < 17; i++)
	  {
	    if (cd->points[cd->outline][i][0] != -1)
	      gdk_draw_arc (cd->pixmap, cd->graph->style->black_gc, TRUE,
			    (cd->points[cd->outline][i][0] * 255.0),
			    255 - (cd->points[cd->outline][i][1] * 255.0),
			    RADIUS * 2, RADIUS * 2, 0, 23040);
	  }


      gdk_draw_pixmap (cd->graph->window, cd->graph->style->black_gc, cd->pixmap,
		       0, 0, 0, 0, GRAPH_WIDTH + RADIUS * 2, GRAPH_HEIGHT + RADIUS * 2);
    }
}


static void
bender_plot_curve (BenderDialog *cd,
		   int           p1,
		   int           p2,
		   int           p3,
		   int           p4,
		   gint32        xmax,
		   gint32        ymax,
		   gint          fix255)
{
  CRMatrix geometry;
  CRMatrix tmp1, tmp2;
  CRMatrix deltas;
  double x, dx, dx2, dx3;
  double y, dy, dy2, dy3;
  double d, d2, d3;
  int lastx, lasty;
  gint32 newx, newy;
  gint32 ntimes; 
  gint32 i;

  /* construct the geometry matrix from the segment */
  for (i = 0; i < 4; i++)
  {
      geometry[i][2] = 0;
      geometry[i][3] = 0;
  }

  geometry[0][0] = (cd->points[cd->outline][p1][0] * xmax);
  geometry[1][0] = (cd->points[cd->outline][p2][0] * xmax);
  geometry[2][0] = (cd->points[cd->outline][p3][0] * xmax);
  geometry[3][0] = (cd->points[cd->outline][p4][0] * xmax);

  geometry[0][1] = (cd->points[cd->outline][p1][1] * ymax);
  geometry[1][1] = (cd->points[cd->outline][p2][1] * ymax);
  geometry[2][1] = (cd->points[cd->outline][p3][1] * ymax);
  geometry[3][1] = (cd->points[cd->outline][p4][1] * ymax);

  /* subdivide the curve ntimes (1000) times */
  ntimes = 4 * xmax;
  /* ntimes can be adjusted to give a finer or coarser curve */
  d = 1.0 / ntimes;
  d2 = d * d;
  d3 = d * d * d;

  /* construct a temporary matrix for determining the forward differencing deltas */
  tmp2[0][0] = 0;     tmp2[0][1] = 0;     tmp2[0][2] = 0;    tmp2[0][3] = 1;
  tmp2[1][0] = d3;    tmp2[1][1] = d2;    tmp2[1][2] = d;    tmp2[1][3] = 0;
  tmp2[2][0] = 6*d3;  tmp2[2][1] = 2*d2;  tmp2[2][2] = 0;    tmp2[2][3] = 0;
  tmp2[3][0] = 6*d3;  tmp2[3][1] = 0;     tmp2[3][2] = 0;    tmp2[3][3] = 0;

  /* compose the basis and geometry matrices */
  bender_CR_compose (CR_basis, geometry, tmp1);

  /* compose the above results to get the deltas matrix */
  bender_CR_compose (tmp2, tmp1, deltas);

  /* extract the x deltas */
  x = deltas[0][0];
  dx = deltas[1][0];
  dx2 = deltas[2][0];
  dx3 = deltas[3][0];

  /* extract the y deltas */
  y = deltas[0][1];
  dy = deltas[1][1];
  dy2 = deltas[2][1];
  dy3 = deltas[3][1];

  lastx = CLAMP (x, 0, xmax);
  lasty = CLAMP (y, 0, ymax);


  if(fix255)
  {
    cd->curve[cd->outline][lastx] = lasty;
  }
  else  
  {
    cd->curve_ptr[cd->outline][lastx] = lasty;
    if(gb_debug) printf("bender_plot_curve xmax:%d ymax:%d\n", (int)xmax, (int)ymax);
  }

  /* loop over the curve */
  for (i = 0; i < ntimes; i++)
    {
      /* increment the x values */
      x += dx;
      dx += dx2;
      dx2 += dx3;

      /* increment the y values */
      y += dy;
      dy += dy2;
      dy2 += dy3;

      newx = CLAMP ((ROUND (x)), 0, xmax);
      newy = CLAMP ((ROUND (y)), 0, ymax);

      /* if this point is different than the last one...then draw it */
      if ((lastx != newx) || (lasty != newy))
      {
        if(fix255)
        {
          /* use fixed array size (for the curve graph) */
          cd->curve[cd->outline][newx] = newy;
	}
	else
	{
	  /* use dynamic allocated curve_ptr (for the real curve) */
	  cd->curve_ptr[cd->outline][newx] = newy;
	  
	  if(gb_debug) printf("outline: %d  cX: %d cY: %d\n", (int)cd->outline, (int)newx, (int)newy);	  
	}
      }

      lastx = newx;
      lasty = newy;
    }
}

static void
bender_calculate_curve (BenderDialog *cd, gint32 xmax, gint32 ymax, gint fix255)
{
  int i;
  int points[17];
  int num_pts;
  int p1, p2, p3, p4;
  int xmid;
  int yfirst, ylast;

  switch (cd->curve_type)
  {
    case GFREE:
      break;
    case SMOOTH:
      /*  cycle through the curves  */
      num_pts = 0;
      for (i = 0; i < 17; i++)
	if (cd->points[cd->outline][i][0] != -1)
	  points[num_pts++] = i;

      xmid = xmax / 2;
      /*  Initialize boundary curve points */
      if (num_pts != 0)
      {
        
        if(fix255)
        {
          for (i = 0; i < (cd->points[cd->outline][points[0]][0] * 255); i++)
	    cd->curve[cd->outline][i] = (cd->points[cd->outline][points[0]][1] * 255);
          for (i = (cd->points[cd->outline][points[num_pts - 1]][0] * 255); i < 256; i++)
	    cd->curve[cd->outline][i] = (cd->points[cd->outline][points[num_pts - 1]][1] * 255);
        }
        else
        {
	    yfirst  = cd->points[cd->outline][points[0]][1] * ymax;
	    ylast   = cd->points[cd->outline][points[num_pts - 1]][1] * ymax;
	    
	    for (i = 0; i < xmid; i++)
	    {
	       cd->curve_ptr[cd->outline][i] = yfirst;
	    }
	    for (i = xmid; i <= xmax; i++)
	    {
	       cd->curve_ptr[cd->outline][i] = ylast;
	    }
	      
        }
      }

      for (i = 0; i < num_pts - 1; i++)
      {
	  p1 = (i == 0) ? points[i] : points[(i - 1)];
	  p2 = points[i];
	  p3 = points[(i + 1)];
	  p4 = (i == (num_pts - 2)) ? points[(num_pts - 1)] : points[(i + 2)];

	  bender_plot_curve (cd, p1, p2, p3, p4, xmax, ymax, fix255);
      }
      break;
  }

}

static void
p_set_rotate_entry_text(BenderDialog *cd)
{
  gchar l_buffer[30];

  sprintf (l_buffer, "%.1f", (float)cd->rotation );
  gtk_entry_set_text (GTK_ENTRY(cd->rotate_entry), l_buffer);
 
}

static void
bender_rotate_entry_callback (GtkWidget *w,
		       gpointer   client_data)
{
  BenderDialog *cd;
  gdouble new_val;
 
  cd = (BenderDialog *) client_data;
  new_val = atof ( gtk_entry_get_text( GTK_ENTRY(w) ) );

  if (new_val != cd->rotation)
  {
    cd->rotation = new_val;
    
    /* it would be nice to update the preview image
     * on each digit you type in, but you would need a real
     * fast machine to do so.
     */ 
    /* if(cd->preview) bender_update (cd, UP_PREVIEW | UP_DRAW); */ 
  }
}


static void
bender_upper_callback (GtkWidget *w,
		       gpointer   client_data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) client_data;

  if (cd->outline != OUTLINE_UPPER)
    {
      cd->outline = OUTLINE_UPPER;
      bender_update (cd, UP_GRAPH | UP_DRAW);
    }
}

static void
bender_lower_callback (GtkWidget *w,
		     gpointer   client_data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) client_data;

  if (cd->outline != OUTLINE_LOWER)
    {
      cd->outline = OUTLINE_LOWER;
      bender_update (cd, UP_GRAPH | UP_DRAW);
    }
}


static void
bender_smooth_callback (GtkWidget *w,
			gpointer   client_data)
{
  BenderDialog *cd;
  int i;
  gint32 index;

  cd = (BenderDialog *) client_data;

  if (cd->curve_type != SMOOTH)
    {
      cd->curve_type = SMOOTH;

      /*  pick representative points from the curve and make them control points  */
      for (i = 0; i <= 8; i++)
	{
	  index = CLAMP ((i * 32), 0, 255);
	  cd->points[cd->outline][i * 2][0] = (gdouble)index / 255.0;
	  cd->points[cd->outline][i * 2][1] = (gdouble)cd->curve[cd->outline][index] / 255.0;
	}

      bender_calculate_curve (cd, 255, 255, TRUE);
      bender_update (cd, UP_GRAPH | UP_DRAW);

      if (cd->preview)
	bender_update (cd, UP_PREVIEW | UP_DRAW);
    }
}

static void
bender_free_callback (GtkWidget *w,
		      gpointer   client_data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) client_data;

  if (cd->curve_type != GFREE)
    {
      cd->curve_type = GFREE;
      bender_update (cd, UP_GRAPH | UP_DRAW);
    }
}

static void
bender_reset_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  BenderDialog *cd;
  int i;

  cd = (BenderDialog *) client_data;

  /*  Initialize the values  */
  for (i = 0; i < 256; i++)
    cd->curve[cd->outline][i] = MIDDLE;

  cd->grab_point = -1;
  for (i = 0; i < 17; i++)
    {
      cd->points[cd->outline][i][0] = -1;
      cd->points[cd->outline][i][1] = -1;
    }
  cd->points[cd->outline][0][0] = 0.0;       /* x */
  cd->points[cd->outline][0][1] = 0.5;       /* y */
  cd->points[cd->outline][16][0] = 1.0;      /* x */
  cd->points[cd->outline][16][1] = 0.5;      /* y */

  bender_update (cd, UP_GRAPH | UP_DRAW);
  if (cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_copy_callback (GtkWidget *widget,
		      gpointer   client_data)
{
  BenderDialog *cd;
  int i;
  int other;

  cd = (BenderDialog *) client_data;

  if(cd->outline == 0)
     other = 1;
  else
     other = 0;


  for (i = 0; i < 17; i++)
    {
      cd->points[other][i][0] = cd->points[cd->outline][i][0];
      cd->points[other][i][1] = cd->points[cd->outline][i][1];
    }

  for (i= 0; i < 256; i++)
    {
      cd->curve[other][i] = cd->curve[cd->outline][i];
    }

  bender_update (cd, UP_GRAPH | UP_DRAW);
  if (cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_copy_inv_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  BenderDialog *cd;
  int i;
  int other;

  cd = (BenderDialog *) client_data;

  if(cd->outline == 0)
     other = 1;
  else
     other = 0;


  for (i = 0; i < 17; i++)
    {
      cd->points[other][i][0] = cd->points[cd->outline][i][0];        /* x */
      cd->points[other][i][1] = 1.0 - cd->points[cd->outline][i][1];  /* y */
    }

  for (i= 0; i < 256; i++)
    {
      cd->curve[other][i] = 255 - cd->curve[cd->outline][i];
    }

  bender_update (cd, UP_GRAPH | UP_DRAW);
  if (cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}


static void
bender_swap_callback (GtkWidget *widget,
		      gpointer   client_data)
{
#define SWAP_VALUE(a, b, h) { h=a; a=b; b=h; }
  BenderDialog *cd;
  int i;
  int other;
  gdouble hd;
  guchar  hu;

  cd = (BenderDialog *) client_data;

  if(cd->outline == 0)
     other = 1;
  else
     other = 0;


  for (i = 0; i < 17; i++)
    {
      SWAP_VALUE(cd->points[other][i][0], cd->points[cd->outline][i][0], hd);  /* x */    
      SWAP_VALUE(cd->points[other][i][1], cd->points[cd->outline][i][1], hd);  /* y */
    }

  for (i= 0; i < 256; i++)
    {
      SWAP_VALUE(cd->curve[other][i], cd->curve[cd->outline][i], hu);
    }

  bender_update (cd, UP_GRAPH | UP_DRAW);
  if (cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) data;

  if (GTK_WIDGET_VISIBLE (cd->shell))
    gtk_widget_hide (cd->shell);

  cd->run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (cd->shell));
  gtk_main_quit ();
}

static void
bender_cancel_callback (GtkWidget *widget,
			gpointer   data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) data;
  gtk_main_quit ();
}

static void
bender_preview_update (GtkWidget *widget,
		       gpointer   data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) data;
  
  if (GTK_TOGGLE_BUTTON (widget)->active)
  {
      cd->preview = TRUE;
      bender_update (cd, UP_PREVIEW | UP_DRAW);
  }
  else
  {
    cd->preview = FALSE;
  }
}

static void
bender_preview_update_once (GtkWidget *widget,
			    gpointer   data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) data;  
  bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
p_filesel_close_cb (GtkWidget *widget,
		    gpointer   data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) data;  

  if(gb_debug) printf("p_filesel_close_cb\n");

  if(cd->filesel == NULL) return;

  gtk_widget_destroy(GTK_WIDGET(cd->filesel));
  cd->filesel = NULL;   /* now filesel is closed */
}

static void
p_points_save_to_file (GtkWidget *widget,
		      gpointer	 data)
{
  BenderDialog *cd;
  char	    *filename;
 
  if(gb_debug) printf("p_points_save_to_file\n");
  cd = (BenderDialog *) data;  
  
  if(cd->filesel == NULL) return;
  
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (cd->filesel));
  p_save_pointfile(cd, filename);
  gtk_widget_destroy(GTK_WIDGET(cd->filesel));
  cd->filesel = NULL;
}

static void
p_points_load_from_file (GtkWidget *widget,
		      gpointer	 data)
{
  BenderDialog *cd;
  char	    *filename;
 
  if(gb_debug) printf("p_points_load_from_file\n");
  cd = (BenderDialog *) data;  
  
  if(cd->filesel == NULL) return;
  
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (cd->filesel));
  p_load_pointfile(cd, filename);
  gtk_widget_destroy(GTK_WIDGET(cd->filesel));
  cd->filesel = NULL;
  bender_update (cd, UP_ALL);
}

static void
bender_load_callback (GtkWidget *w,
		       gpointer   data)
{
  BenderDialog *cd;
  GtkWidget *filesel;

  cd = (BenderDialog *) data;
  if(cd->filesel != NULL)
     return;   /* filesel is already open */
  
  filesel = gtk_file_selection_new ( _("Load Curve Points from file"));
  cd->filesel = filesel;
  
  gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", (GtkSignalFunc) p_points_load_from_file,
		      cd);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		      "clicked", (GtkSignalFunc) p_filesel_close_cb,
		      cd);
  gtk_signal_connect (GTK_OBJECT (filesel), "destroy",
		      (GtkSignalFunc) p_filesel_close_cb,
		      cd);
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				   "curve_bend.points");
  gtk_widget_show (filesel);

}

static void
bender_save_callback (GtkWidget *w,
		       gpointer   data)
{
  BenderDialog *cd;
  GtkWidget *filesel;

  cd = (BenderDialog *) data;
  if(cd->filesel != NULL)
     return;   /* filesel is already open */
  
  filesel = gtk_file_selection_new ( _("Save Curve Points to file"));
  cd->filesel = filesel;
  
  gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", (GtkSignalFunc) p_points_save_to_file,
		      cd);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		      "clicked", (GtkSignalFunc) p_filesel_close_cb,
		      cd);
  gtk_signal_connect (GTK_OBJECT (filesel), "destroy",
		      (GtkSignalFunc) p_filesel_close_cb,
		      cd);
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				   "curve_bend.points");
  gtk_widget_show (filesel);
}

static void
bender_smoothing_callback (GtkWidget *w,
		       gpointer   data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) data;
  
  if (GTK_TOGGLE_BUTTON (w)->active) { cd->smoothing = TRUE;  }
  else                               { cd->smoothing = FALSE; }

  if(cd->preview) bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_antialias_callback (GtkWidget *w,
		       gpointer   data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) data;
  
  if (GTK_TOGGLE_BUTTON (w)->active) { cd->antialias = TRUE; }
  else                               { cd->antialias = FALSE; }

  if(cd->preview) bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_work_on_copy_callback (GtkWidget *w,
		       gpointer   data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) data;
  
  if (GTK_TOGGLE_BUTTON (w)->active) { cd->work_on_copy = TRUE;  }
  else                               { cd->work_on_copy = FALSE; }
}

static gint
bender_graph_events (GtkWidget    *widget,
		     GdkEvent     *event,
		     BenderDialog *cd)
{
  static GdkCursorType cursor_type = GDK_TOP_LEFT_ARROW;
  GdkCursorType new_type;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int i;
  int tx, ty;
  int x, y;
  int closest_point;
  int distance;
  int x1, x2, y1, y2;

  new_type      = GDK_X_CURSOR;
  closest_point = 0;

  /*  get the pointer position  */
  gdk_window_get_pointer (cd->graph->window, &tx, &ty, NULL);
  x = CLAMP ((tx - RADIUS), 0, 255);
  y = CLAMP ((ty - RADIUS), 0, 255);

  distance = G_MAXINT;
  for (i = 0; i < 17; i++)
    {
      if (cd->points[cd->outline][i][0] != -1)
	if (abs (x - (cd->points[cd->outline][i][0] * 255.0)) < distance)
	  {
	    distance = abs (x - (cd->points[cd->outline][i][0] * 255.0));
	    closest_point = i;
	  }
    }
  if (distance > MIN_DISTANCE)
    closest_point = (x + 8) / 16;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (cd->pixmap == NULL)
	cd->pixmap = gdk_pixmap_new (cd->graph->window,
				     GRAPH_WIDTH + RADIUS * 2,
				     GRAPH_HEIGHT + RADIUS * 2, -1);

      bender_update (cd, UP_GRAPH | UP_DRAW);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      new_type = GDK_TCROSS;

      switch (cd->curve_type)
	{
	case SMOOTH:
	  /*  determine the leftmost and rightmost points  */
	  cd->leftmost = -1;
	  for (i = closest_point - 1; i >= 0; i--)
	    if (cd->points[cd->outline][i][0] != -1)
	      {
		cd->leftmost = (cd->points[cd->outline][i][0] * 255.0);
		break;
	      }
	  cd->rightmost = 256;
	  for (i = closest_point + 1; i < 17; i++)
	    if (cd->points[cd->outline][i][0] != -1)
	      {
		cd->rightmost = (cd->points[cd->outline][i][0] * 255.0);
		break;
	      }

	  cd->grab_point = closest_point;
	  cd->points[cd->outline][cd->grab_point][0] = (gdouble)x / 255.0;
	  cd->points[cd->outline][cd->grab_point][1] = (gdouble)(255 - y) / 255.0;

	  bender_calculate_curve (cd, 255, 255, TRUE);
	  break;

	case GFREE:
	  cd->curve[cd->outline][x] = 255 - y;
	  cd->grab_point = x;
	  cd->last = y;
	  break;
	}

      bender_update (cd, UP_GRAPH | UP_DRAW);
      break;

    case GDK_BUTTON_RELEASE:
      new_type = GDK_FLEUR;
      cd->grab_point = -1;

      if (cd->preview)
	bender_update (cd, UP_PREVIEW | UP_DRAW);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;

      if (mevent->is_hint)
	{
	  mevent->x = tx;
	  mevent->y = ty;
	}

      switch (cd->curve_type)
	{
	case SMOOTH:
	  /*  If no point is grabbed...  */
	  if (cd->grab_point == -1)
	    {
	      if (cd->points[cd->outline][closest_point][0] != -1)
		new_type = GDK_FLEUR;
	      else
		new_type = GDK_TCROSS;
	    }
	  /*  Else, drag the grabbed point  */
	  else
	    {
	      new_type = GDK_TCROSS;

	      cd->points[cd->outline][cd->grab_point][0] = -1;

	      if (x > cd->leftmost && x < cd->rightmost)
		{
		  closest_point = (x + 8) / 16;
		  if (cd->points[cd->outline][closest_point][0] == -1)
		    cd->grab_point = closest_point;
		  cd->points[cd->outline][cd->grab_point][0] = (gdouble)x / 255.0;
		  cd->points[cd->outline][cd->grab_point][1] = (gdouble)(255 - y) / 255.0;
		}

	      bender_calculate_curve (cd, 255, 255, TRUE);
	      bender_update (cd, UP_GRAPH | UP_DRAW);
	    }
	  break;

	case GFREE:
	  if (cd->grab_point != -1)
	    {
	      if (cd->grab_point > x)
		{
		  x1 = x;
		  x2 = cd->grab_point;
		  y1 = y;
		  y2 = cd->last;
		}
	      else
		{
		  x1 = cd->grab_point;
		  x2 = x;
		  y1 = cd->last;
		  y2 = y;
		}

	      if (x2 != x1)
		for (i = x1; i <= x2; i++)
		  cd->curve[cd->outline][i] = 255 - (y1 + ((y2 - y1) * (i - x1)) / (x2 - x1));
	      else
		cd->curve[cd->outline][x] = 255 - y;

	      cd->grab_point = x;
	      cd->last = y;

	      bender_update (cd, UP_GRAPH | UP_DRAW);
	    }

	  if (mevent->state & GDK_BUTTON1_MASK)
	    new_type = GDK_TCROSS;
	  else
	    new_type = GDK_PENCIL;
	  break;
	}

      if (new_type != cursor_type)
	{
	  cursor_type = new_type;
	  /* change_win_cursor (cd->graph->window, cursor_type); */
	}
      break;

    default:
      break;
    }

  return FALSE;
}


static gint
bender_pv_widget_events (GtkWidget    *widget,
		      GdkEvent     *event,
		      BenderDialog *cd)
{
  switch (event->type)
    {
    case GDK_EXPOSE:
      bender_update (cd, UP_PREVIEW_EXPOSE);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
bender_CR_compose (CRMatrix a,
		   CRMatrix b,
		   CRMatrix ab)
{
  int i, j;

  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 4; j++)
        {
          ab[i][j] = (a[i][0] * b[0][j] +
                      a[i][1] * b[1][j] +
                      a[i][2] * b[2][j] +
                      a[i][3] * b[3][j]);
        }
    }
}


/* ============================================================================
 * p_buildmenu
 *    build menu widget for all Items passed in the MenuItems Parameter
 *    MenuItems is an array of Pointers to Structure MenuItem.
 *    The End is marked by a Structure Member where the label is a NULL pointer
 *    (simplifyed version of GIMP 1.0.2 bulid_menu procedur)
 * ============================================================================
 */

GtkWidget *
p_buildmenu (MenuItem *items)
{
  GtkWidget *menu;
  GtkWidget *menu_item;

  menu = gtk_menu_new ();

  while (items->label)
  {
      menu_item = gtk_menu_item_new_with_label ( gettext(items->label));
      gtk_container_add (GTK_CONTAINER (menu), menu_item);

      if (items->callback)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			    (GtkSignalFunc) items->callback,
			    items->user_data);

      gtk_widget_show (menu_item);
      items->widget = menu_item;

      items++;
  }

  return menu;
}	/* end p_buildmenu */

void
p_render_preview (BenderDialog *cd,
		  gint32        layer_id)
{
   guchar  l_rowbuf[PREVIEW_BPP * PREVIEW_SIZE_X];
   guchar  l_pixel[4];
   guchar *l_ptr;
   GDrawable *l_pv_drawable;
   gint    l_x, l_y;
   gint    l_ofx, l_ofy;
   gint    l_idx;
   guchar  l_bg_gray;
   t_GDRW  l_gdrw;
   t_GDRW  *gdrw;

   l_pv_drawable = gimp_drawable_get (layer_id);

   gdrw = &l_gdrw;
   p_init_gdrw(gdrw, l_pv_drawable, FALSE, FALSE);

  /* offsets to set bend layer to preview center */
  l_ofx = (l_pv_drawable->width / 2) - (PREVIEW_SIZE_X / 2);
  l_ofy = (l_pv_drawable->height / 2) - (PREVIEW_SIZE_Y / 2);
  
  /* render preview */
  for(l_y = 0; l_y < PREVIEW_SIZE_Y; l_y++)
  {
     l_ptr = &l_rowbuf[0];

     for(l_x = 0; l_x < PREVIEW_SIZE_X; l_x++)
     {
	p_get_pixel(gdrw, l_x + l_ofx, l_y + l_ofy, &l_pixel[0]);
	if(l_pixel[gdrw->index_alpha] < 255)
	{
	   /* for transparent pixels: mix with preview background color */
	   if((l_x % 32) < 16)
	   {
	      l_bg_gray = PREVIEW_BG_GRAY1;
	      if ((l_y % 32) < 16)   l_bg_gray = PREVIEW_BG_GRAY2;
	   }
	   else
	   {
	      l_bg_gray = PREVIEW_BG_GRAY2;
	      if ((l_y % 32) < 16)   l_bg_gray = PREVIEW_BG_GRAY1;
	   }
	   
	   for(l_idx = 0; l_idx < gdrw->index_alpha ; l_idx++)
	   {
	      l_pixel[l_idx] = MIX_CHANNEL(l_pixel[l_idx], l_bg_gray, l_pixel[gdrw->index_alpha]);
	   }
	}
	  
        
	if(cd->color)
	{
	  *l_ptr    = l_pixel[0];
	   l_ptr[1] = l_pixel[1];
	   l_ptr[2] = l_pixel[2];
	}
	else
	{
	  *l_ptr    = l_pixel[0];
	   l_ptr[1] = l_pixel[0];
	   l_ptr[2] = l_pixel[0];
        }

        l_ptr += PREVIEW_BPP;
     }

     gtk_preview_draw_row(GTK_PREVIEW(cd->pv_widget), &l_rowbuf[0], 0, l_y, PREVIEW_SIZE_X);
  }

   p_end_gdrw(gdrw);
}	/* end p_render_preview */

/* ===================================================== */
/* curve_bend worker procedures                          */
/* ===================================================== */

void
p_stretch_curves(BenderDialog *cd, gint32 xmax, gint32 ymax)
{
  gint32   l_x1, l_x2;
  gdouble  l_ya, l_yb;
  gdouble  l_rest;
  int      l_outline;

  for(l_outline = 0; l_outline < 2; l_outline++)
  {
    for(l_x1 = 0; l_x1 <= xmax; l_x1++)
    {
       l_x2 = (l_x1 * 255) / xmax;
       if((xmax <= 255) && (l_x2 < 255))
       {
         cd->curve_ptr[l_outline][l_x1] = ROUND((cd->curve[l_outline][l_x2] * ymax) / 255);
       }
       else
       {
         /* interpolate */
         l_rest = (((gdouble)l_x1 * 255.0) / (gdouble)xmax) - l_x2;
         l_ya = cd->curve[l_outline][l_x2];        /* y of this point */
         l_yb = cd->curve[l_outline][l_x2 +1];     /* y of next point */
         cd->curve_ptr[l_outline][l_x1] = ROUND (((l_ya + ((l_yb -l_ya) * l_rest)) * ymax) / 255);
       }

       {
        int      l_debugY;
        l_debugY = ROUND((cd->curve[l_outline][l_x2] * ymax) / 255);
/*
 *            printf("_X: %d  _X2: %d Y2: %d _Y: %d debugY:%d\n",
 *                   (int)l_x1, (int)l_x2,
 *                   (int)cd->curve[l_outline][l_x2],
 *                   (int)cd->curve_ptr[l_outline][l_x1], 
 *                   l_debugY);
 */
       }	  

    }
  }
}


void
bender_init_min_max (BenderDialog *cd, gint32 xmax)
{
  int i, j;

  for (i = 0; i < 2; i++)
  {
    cd->min2[i] = 65000;
    cd->max2[i] = 0;
    for (j = 0; j <= xmax; j++)
    {
       if(cd->curve_ptr[i][j] > cd->max2[i])
       {
          cd->max2[i] = cd->curve_ptr[i][j];
       }
       if(cd->curve_ptr[i][j] < cd->min2[i])
       {
          cd->min2[i] = cd->curve_ptr[i][j];
       }
    }
  }
  
  /* for UPPER outline : y-zero line is assumed at the min leftmost or rightmost point */
  cd->zero2[OUTLINE_UPPER] = MIN(cd->curve_ptr[OUTLINE_UPPER][0], cd->curve_ptr[OUTLINE_UPPER][xmax]);
 
  /* for LOWER outline : y-zero line is assumed at the min leftmost or rightmost point */
  cd->zero2[OUTLINE_LOWER] = MAX(cd->curve_ptr[OUTLINE_LOWER][0], cd->curve_ptr[OUTLINE_LOWER][xmax]);
  
  if(gb_debug)
  {
    printf("bender_init_min_max: zero2[0]: %d min2[0]:%d max2[0]:%d\n",
        (int)cd->zero2[0],
	(int)cd->min2[0],
	(int)cd->max2[0]  );
    printf("bender_init_min_max: zero2[1]: %d min2[1]:%d max2[1]:%d\n",
        (int)cd->zero2[1],
	(int)cd->min2[1],
	(int)cd->max2[1]  );
  }
	
}


gint32
p_curve_get_dy(BenderDialog *cd, gint32 x, gint32 drawable_width, gint32 total_steps, gdouble current_step)
{
  /* get y values of both upper and lower curve,
   * and return the iterated value inbetween
   */
  gint32     l_y; 
  double     l_y1,  l_y2;
  double     delta;

  l_y1 = cd->zero2[OUTLINE_UPPER] - cd->curve_ptr[OUTLINE_UPPER][x];
  l_y2 = cd->zero2[OUTLINE_LOWER] - cd->curve_ptr[OUTLINE_LOWER][x];
  
  delta = ((double)(l_y2 - l_y1) / (double)(total_steps -1)) * current_step;
  l_y = SIGNED_ROUND(l_y1 + delta);
  
  return l_y;
}


gint32
p_upper_curve_extend(BenderDialog *cd, gint32 drawable_width, gint32 drawable_height)
{
   gint32  l_y1,  l_y2;
   
   l_y1 = cd->max2[OUTLINE_UPPER] - cd->zero2[OUTLINE_UPPER];
   l_y2 = (cd->max2[OUTLINE_LOWER] - cd->zero2[OUTLINE_LOWER]) - drawable_height;
   
   return (MAX(l_y1, l_y2));  
}

gint32
p_lower_curve_extend(BenderDialog *cd, gint32 drawable_width, gint32 drawable_height)
{
   gint32  l_y1,  l_y2;

   l_y1 = cd->zero2[OUTLINE_LOWER] - cd->min2[OUTLINE_LOWER];
   l_y2 = (cd->zero2[OUTLINE_UPPER] - cd->min2[OUTLINE_UPPER]) - drawable_height;

   return (MAX(l_y1, l_y2));  
}

void
p_end_gdrw(t_GDRW *gdrw)
{
  if(gb_debug)  printf("\np_end_gdrw: drawable %x  ID: %d\n", (int)gdrw->drawable, (int)gdrw->drawable->id);

  if(gdrw->tile)
  {
     if(gb_debug)  printf("p_end_gdrw: tile unref\n");
     gimp_tile_unref( gdrw->tile, gdrw->tile_dirty);
     gdrw->tile = NULL;
  }

  if(gb_debug)  printf("p_end_gdrw:TILE_SWAPCOUNT: %d\n", (int)gdrw->tile_swapcount);

}	/* end p_end_gdrw */


void
p_init_gdrw(t_GDRW *gdrw, GDrawable *drawable, int dirty, int shadow)
{
  gint32 l_image_id;
  gint32 l_sel_channel_id;
  gint    l_offsetx, l_offsety;

  if(gb_debug)  printf("\np_init_gdrw: drawable %x  ID: %d\n", (int)drawable, (int)drawable->id);

  gdrw->drawable = drawable;
  gdrw->tile = NULL;
  gdrw->tile_dirty = FALSE;
  gdrw->tile_width = gimp_tile_width ();
  gdrw->tile_height = gimp_tile_height ();
  gdrw->shadow = shadow;
  gdrw->tile_swapcount = 0;
  gdrw->seldeltax = 0;
  gdrw->seldeltay = 0;
  gimp_drawable_offsets (drawable->id, &l_offsetx, &l_offsety);  /* get offsets within the image */
  
  gimp_drawable_mask_bounds (drawable->id, &gdrw->x1, &gdrw->y1, &gdrw->x2, &gdrw->y2);

/*
 *   gimp_pixel_rgn_init (&gdrw->pr, drawable,
 *                       gdrw->x1, gdrw->y1, gdrw->x2 - gdrw->x1, gdrw->y2 - gdrw->y1,
 *                       dirty, shadow);
 */


  gdrw->bpp = drawable->bpp;
  if (gimp_drawable_has_alpha(drawable->id))
  {
     /* index of the alpha channelbyte {1|3} */
     gdrw->index_alpha = gdrw->bpp -1;
  }
  else
  {
     gdrw->index_alpha = 0;      /* there is no alpha channel */
  }
  
  if(gb_debug)  printf("\np_init_gdrw: bpp %d  index_alpha: %d\n", (int)gdrw->bpp, (int)gdrw->index_alpha);
  
  l_image_id = gimp_layer_get_image_id(drawable->id);

  /* check and see if we have a selection mask */
  l_sel_channel_id  = gimp_image_get_selection(l_image_id);     

  if(gb_debug)  
  {
     printf("p_init_gdrw: image_id %d sel_channel_id: %d\n", (int)l_image_id, (int)l_sel_channel_id);
     printf("p_init_gdrw: BOUNDS     x1: %d y1: %d x2:%d y2: %d\n",
        (int)gdrw->x1,  (int)gdrw->y1, (int)gdrw->x2,(int)gdrw->y2);
     printf("p_init_gdrw: OFFS       x: %d y: %d\n", (int)l_offsetx, (int)l_offsety );
  }
}	/* end p_init_gdrw */


static void
p_provide_tile(t_GDRW *gdrw, gint col, gint row, gint shadow )
{
    if ( col != gdrw->tile_col || row != gdrw->tile_row || !gdrw->tile )
    {
       if( gdrw->tile ) 
       {
	 gimp_tile_unref( gdrw->tile, gdrw->tile_dirty );
       }
       gdrw->tile_col = col;
       gdrw->tile_row = row;
       gdrw->tile = gimp_drawable_get_tile( gdrw->drawable, shadow, gdrw->tile_row, gdrw->tile_col );
       gdrw->tile_dirty = FALSE;
       gimp_tile_ref( gdrw->tile );

       gdrw->tile_swapcount++;

       return;
    }
}	/* end p_provide_tile */

/* get pixel value
 *   return light transparent black gray pixel if out of bounds
 *   (should occur in the previews only)
 */
static void
p_get_pixel( t_GDRW *gdrw, gint32 x, gint32 y, guchar *pixel )
{
    gint   row, col;
    gint   offx, offy;
    guchar  *ptr;

    if((x < 0)
    || (x > gdrw->drawable->width   -1)
    || (y < 0)
    || (y > gdrw->drawable->height - 1))
    {
      pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
      return;
    }

    col = x / gdrw->tile_width;
    row = y / gdrw->tile_height;
    offx = x % gdrw->tile_width;
    offy = y % gdrw->tile_height;

    p_provide_tile(  gdrw, col, row, gdrw->shadow);

    pixel[1] = 255;  pixel[3] = 255;  /* simulate full visible alpha channel */
    ptr = gdrw->tile->data + ((( offy * gdrw->tile->ewidth) + offx ) * gdrw->bpp);
    memcpy(pixel, ptr, gdrw->bpp);

    /* printf("p_get_pixel: x: %d  y: %d bpp:%d RGBA:%d %d %d %d\n", (int)x, (int)y, (int)gdrw->bpp, (int)pixel[0], (int)pixel[1], (int)pixel[2], (int)pixel[3]);
     */
}	/* end p_get_pixel */

void
p_put_pixel(t_GDRW  *gdrw, gint32 x, gint32 y, guchar *pixel)
{
    gint   row, col;
    gint   offx, offy;
    guchar  *ptr;

    if (x < gdrw->x1)       return;
    if (x > gdrw->x2 - 1)   return;
    if (y < gdrw->y1)       return;
    if (y > gdrw->y2 - 1)   return;

    col = x / gdrw->tile_width;
    row = y / gdrw->tile_height;
    offx = x % gdrw->tile_width;
    offy = y % gdrw->tile_height;

    p_provide_tile( gdrw, col, row, gdrw->shadow );

    ptr = gdrw->tile->data + ((( offy * gdrw->tile->ewidth ) + offx ) * gdrw->bpp);
    memcpy(ptr, pixel, gdrw->bpp);

    gdrw->tile_dirty = TRUE;

   /* printf("p_put_pixel: x: %d  y: %d shadow: %d bpp:%d RGBA:%d %d %d %d\n", (int)x, (int)y, (int)gdrw->shadow, (int)gdrw->bpp, (int)ptr[0], (int)ptr[1], (int)ptr[2], (int)ptr[3]);
    */
}	/* end p_put_pixel */

void
p_put_mix_pixel(t_GDRW  *gdrw, gint32 x, gint32 y, guchar *color, gint32 nb_curvy, gint32 nb2_curvy, gint32 curvy)
{
   guchar  l_pixel[4];
   guchar  l_mixmask;
   gint    l_idx;
   gint    l_diff;
   
   l_mixmask = 255 - 96;
   l_diff = abs(nb_curvy - curvy);
   if(l_diff == 0)
   {
     l_mixmask = 255 - 48;
     l_diff = abs(nb2_curvy - curvy);
     
     if(l_diff == 0)
     {
       /* last 2 neighbours were not shifted against current pixel, do not mix */     
       p_put_pixel(gdrw, x, y, color);
       return;
     }
   }

   /* get left neighbour pixel */ 
   p_get_pixel(gdrw, x-1, y, &l_pixel[0]);

   if(l_pixel[gdrw->index_alpha] < 10)
   {
     /* neighbour is (nearly or full) transparent, do not mix */     
     p_put_pixel(gdrw, x, y, color);
     return;
   }

   for(l_idx = 0; l_idx < gdrw->index_alpha ; l_idx++)
   {
      /* mix in left neighbour color */
      l_pixel[l_idx] = MIX_CHANNEL(color[l_idx], l_pixel[l_idx], l_mixmask);
   }

   l_pixel[gdrw->index_alpha] = color[gdrw->index_alpha];
   p_put_pixel(gdrw, x, y, &l_pixel[0]);
}

void
p_put_mix_pixel_FIX(t_GDRW  *gdrw, gint32 x, gint32 y, guchar *color, gint32 nb_curvy, gint32 nb2_curvy, gint32 curvy)
{
   guchar  l_pixel[4];
   guchar  l_mixmask;
   gint    l_idx;
   
   l_mixmask = 256 - 64;

   /* get left neighbour pixel */ 
   p_get_pixel(gdrw, x-1, y, &l_pixel[0]);

   if(l_pixel[gdrw->index_alpha] < 20)
   {
     /* neighbour is (nearly or full) transparent, do not mix */     
     p_put_pixel(gdrw, x, y, color);
     return;
   }

   for(l_idx = 0; l_idx < gdrw->index_alpha ; l_idx++)
   {
      /* mix in left neighbour color */
      l_pixel[l_idx] = MIX_CHANNEL(color[l_idx], l_pixel[l_idx], l_mixmask);
   }

   l_pixel[gdrw->index_alpha] = color[gdrw->index_alpha];
   p_put_pixel(gdrw, x, y, &l_pixel[0]);
}

/* ============================================================================
 * p_clear_drawable
 * ============================================================================
 */

void
p_clear_drawable(GDrawable *drawable)
{
   GPixelRgn  pixel_rgn;
   gpointer	pr;
   gint         l_row;
   guchar      *l_ptr;
   

   gimp_pixel_rgn_init (&pixel_rgn, drawable,
			     0, 0, drawable->width, drawable->height,
			     TRUE,  /* dirty */
			     FALSE  /* shadow */
			     );

   /* clear the drawable with 0 Bytes (black full-transparent pixels) */
   for (pr = gimp_pixel_rgns_register (1, &pixel_rgn);
	pr != NULL; pr = gimp_pixel_rgns_process (pr))
   {
      l_ptr = pixel_rgn.data;
      for ( l_row = 0; l_row < pixel_rgn.h; l_row++ )
      {
	memset(l_ptr, 0, pixel_rgn.w * drawable->bpp);
	l_ptr += pixel_rgn.rowstride;
      }
   }
}	/* end  p_clear_drawable */

/* ============================================================================
 * p_create_pv_image
 * ============================================================================
 */
gint32
p_create_pv_image( GDrawable *src_drawable, gint32 *layer_id)
{
  gint32     l_new_image_id;
  guint      l_new_width;
  guint      l_new_height;
  GDrawableType l_type;

  gint    l_x, l_y;
  double  l_scale;
  guchar  l_pixel[4];
  GDrawable *dst_drawable;
  t_GDRW  l_src_gdrw;
  t_GDRW  l_dst_gdrw;

  l_new_image_id = gimp_image_new(PREVIEW_SIZE_X, PREVIEW_SIZE_Y, 
                   gimp_image_base_type(gimp_layer_get_image_id(src_drawable->id)));
  l_type = gimp_drawable_type(src_drawable->id);  
  if(src_drawable->height > src_drawable->width)
  {
    l_new_height = PV_IMG_HEIGHT;
    l_new_width = (src_drawable->width * l_new_height) / src_drawable->height;
    l_scale = (float)src_drawable->height / PV_IMG_HEIGHT;
  }
  else
  {
    l_new_width = PV_IMG_WIDTH;
    l_new_height = (src_drawable->height * l_new_width) / src_drawable->width;
    l_scale = (float)src_drawable->width / PV_IMG_WIDTH;
  }

  *layer_id = gimp_layer_new(l_new_image_id, "preview_original",
                             l_new_width, l_new_height,
                             l_type, 
                             100.0,    /* opacity */
                             0         /* mode NORMAL */
                             );
  if(!gimp_drawable_has_alpha(*layer_id))
  {
    /* always add alpha channel */
    gimp_layer_add_alpha(*layer_id);
  }
                                 
  gimp_image_add_layer(l_new_image_id, *layer_id, 0);

  dst_drawable = gimp_drawable_get (*layer_id);
  p_init_gdrw(&l_src_gdrw, src_drawable, FALSE, FALSE);
  p_init_gdrw(&l_dst_gdrw, dst_drawable,  TRUE,  FALSE);

  for(l_y = 0; l_y < l_new_height; l_y++)
  {
     for(l_x = 0; l_x < l_new_width; l_x++)
     {
       p_get_pixel(&l_src_gdrw, l_x * l_scale, l_y * l_scale, &l_pixel[0]);
       p_put_pixel(&l_dst_gdrw, l_x, l_y, &l_pixel[0]);
     }
  }
  
  p_end_gdrw(&l_src_gdrw);
  p_end_gdrw(&l_dst_gdrw);

  /* gimp_display_new(l_new_image_id); */
  return (l_new_image_id);
}

/* ============================================================================
 * p_add_layer
 * ============================================================================
 */
GDrawable* p_add_layer(gint width, gint height, GDrawable * src_drawable)
{
  GDrawableType  l_type;
  static GDrawable  *l_new_drawable;
  gint32 l_new_layer_id;
  char      *l_name;
  char      *l_name2;
  gdouble    l_opacity;
  GLayerMode l_mode;
  gint       l_visible;
  gint32     image_id;
  gint       stack_position;


  image_id = gimp_layer_get_image_id(src_drawable->id);
  stack_position = 0;                                  /* TODO:  should be same as src_layer */
  
  /* copy type, name, opacity and mode from src_drawable */ 
  l_type     = gimp_layer_type(src_drawable->id);
  l_visible  = gimp_layer_get_visible(src_drawable->id);

  if (TRUE != TRUE)
  {
    l_name = gimp_layer_get_name(src_drawable->id);
  }
  else
  {
    l_name2 = gimp_layer_get_name(src_drawable->id);
    l_name  = g_malloc(strlen(l_name2) + 10);
    if(l_name == NULL) 
       return (NULL);
    sprintf(l_name, "%s_b", l_name2);
    g_free(l_name2);
  }
  l_mode = gimp_layer_get_mode(src_drawable->id);
  l_opacity = gimp_layer_get_opacity(src_drawable->id);  /* full opacity */
  
  l_new_layer_id = gimp_layer_new(image_id, l_name,
                                  width, height,
                                  l_type, 
                                  l_opacity,
                                  l_mode
                                  );

  if(l_name != NULL)         g_free (l_name);
  if(!gimp_drawable_has_alpha(l_new_layer_id))
  {
    /* always add alpha channel */
    gimp_layer_add_alpha(l_new_layer_id);
  }
  
  l_new_drawable = gimp_drawable_get (l_new_layer_id);
  if(l_new_drawable == NULL)
  {
     fprintf(stderr, "p_ad_layer: cant get new_drawable\n");
     return (NULL);
  }

  /* add the copied layer to the temp. working image */
  gimp_image_add_layer(image_id, l_new_layer_id, stack_position);

  /* copy visiblity state */
  gimp_layer_set_visible(l_new_layer_id, l_visible);
    
  return (l_new_drawable);
}

/* ============================================================================
 * p_bender_calculate_iter_curve
 * ============================================================================
 */


void
p_bender_calculate_iter_curve (BenderDialog *cd, gint32 xmax, gint32 ymax)
{
   int l_x;
   gint      l_outline;
   BenderDialog *cd_from;
   BenderDialog *cd_to;
 
   l_outline = cd->outline;
   
   if((cd->bval_from == NULL) || (cd->bval_to == NULL) || (cd->bval_curr == NULL))
   {
     if(gb_debug)  printf("p_bender_calculate_iter_curve NORMAL1\n");
     if (cd->curve_type == SMOOTH)
     {
       cd->outline = OUTLINE_UPPER;
       bender_calculate_curve (cd, xmax, ymax, FALSE);
       cd->outline = OUTLINE_LOWER;
       bender_calculate_curve (cd, xmax, ymax, FALSE);
     }
     else
     {
       p_stretch_curves(cd, xmax, ymax);
     }
   }
   else
   {
     /* compose curves by iterating between FROM/TO values */
     if(gb_debug)  printf("p_bender_calculate_iter_curve ITERmode 1\n");
   
     /* init FROM curves */
     cd_from = g_malloc (sizeof (BenderDialog));
     p_cd_from_bval(cd_from, cd->bval_from);
     cd_from->curve_ptr[OUTLINE_UPPER] = g_malloc(sizeof(gint32) * (1+xmax));
     cd_from->curve_ptr[OUTLINE_LOWER] = g_malloc(sizeof(gint32) * (1+xmax));
     
     /* init TO curves */
     cd_to = g_malloc (sizeof (BenderDialog));
     p_cd_from_bval(cd_to, cd->bval_to);
     cd_to->curve_ptr[OUTLINE_UPPER] = g_malloc(sizeof(gint32) * (1+xmax));
     cd_to->curve_ptr[OUTLINE_LOWER] = g_malloc(sizeof(gint32) * (1+xmax));

     if (cd_from->curve_type == SMOOTH)
     {
       /* calculate FROM curves */
       cd_from->outline = OUTLINE_UPPER;
       bender_calculate_curve (cd_from, xmax, ymax, FALSE);
       cd_from->outline = OUTLINE_LOWER;
       bender_calculate_curve (cd_from, xmax, ymax, FALSE);
     }
     else
     {
       p_stretch_curves(cd_from, xmax, ymax);
     }

     if (cd_to->curve_type == SMOOTH)
     {
       /* calculate TO curves */
       cd_to->outline = OUTLINE_UPPER;
       bender_calculate_curve (cd_to, xmax, ymax, FALSE);
       cd_to->outline = OUTLINE_LOWER;
       bender_calculate_curve (cd_to, xmax, ymax, FALSE);
     }
     else
     {
       p_stretch_curves(cd_to, xmax, ymax);
     }
   
     
     /* MIX Y-koords of the curves according to current iteration step */
     for(l_x = 0; l_x <= xmax; l_x++)
     {
        p_delta_gint32(&cd->curve_ptr[OUTLINE_UPPER][l_x],
                        cd_from->curve_ptr[OUTLINE_UPPER][l_x],
                        cd_to->curve_ptr[OUTLINE_UPPER][l_x],
                        cd->bval_curr->total_steps,
                        cd->bval_curr->current_step);
                        
        p_delta_gint32(&cd->curve_ptr[OUTLINE_LOWER][l_x],
                        cd_from->curve_ptr[OUTLINE_LOWER][l_x],
                        cd_to->curve_ptr[OUTLINE_LOWER][l_x],
                        cd->bval_curr->total_steps,
                        cd->bval_curr->current_step);
     }
     

     g_free(cd_from->curve_ptr[OUTLINE_UPPER]);
     g_free(cd_from->curve_ptr[OUTLINE_LOWER]);
     
     g_free(cd_from);
     g_free(cd_to);
     
   }

   cd->outline = l_outline;
}	/* end p_bender_calculate_iter_curve */

/* ============================================================================
 * p_vertical_bend
 * ============================================================================
 */

int
p_vertical_bend(BenderDialog *cd, t_GDRW *src_gdrw, t_GDRW *dst_gdrw)
{
   gint32             l_row, l_col;
   gint32             l_first_row, l_first_col, l_last_row, l_last_col;
   gint32             l_x, l_y;
   gint32             l_x2, l_y2;
   gint32             l_curvy, l_nb_curvy, l_nb2_curvy;
   gint32             l_desty, l_othery;
   gint32             l_miny, l_maxy;
   gint32             l_sign, l_dy, l_diff;
   gint32             l_topshift;
   float l_progress_step;
   float l_progress_max;
   float l_progress;

   t_Last            *last_arr;
   t_Last            *first_arr;
   guchar             color[4];
   guchar             mixcolor[4];
   guchar             l_alpha_lo;
   gint               l_alias_dir;
   guchar             l_mixmask;


   l_topshift = p_upper_curve_extend(cd, src_gdrw->drawable->width, src_gdrw->drawable->height);
   l_diff = l_curvy = l_nb_curvy = l_nb2_curvy= l_miny = l_maxy = 0;
   l_alpha_lo = 20;

   /* allocate array of last values (one element foreach x koordinate) */
   last_arr = g_malloc(sizeof(t_Last) * src_gdrw->x2);
   first_arr = g_malloc(sizeof(t_Last) * src_gdrw->x2);

   /* ------------------------------------------------
    * foreach pixel in the SAMPLE_drawable:
    * ------------------------------------------------
    * the inner loops (l_x/l_y) are designed to process
    * all pixels of one tile in the sample drawable, the outer loops (row/col) do step
    * to the next tiles. (this was done to reduce tile swapping)
    */

   l_first_row = src_gdrw->y1 / src_gdrw->tile_height;
   l_last_row  = (src_gdrw->y2 / src_gdrw->tile_height);
   l_first_col = src_gdrw->x1 / src_gdrw->tile_width;
   l_last_col  = (src_gdrw->x2 / src_gdrw->tile_width);
 
   /* init progress */
   l_progress_max = (1 + l_last_row - l_first_row) * (1 + l_last_col - l_first_col);
   l_progress_step = 1.0 / l_progress_max;
   l_progress = 0.0;
   if(cd->show_progress) gimp_progress_init ( _("Curve Bend ..."));

   for(l_row = l_first_row; l_row <= l_last_row; l_row++)
   {
     for(l_col = l_first_col; l_col <= l_last_col; l_col++)
     {
       if(l_col == l_first_col)    l_x = src_gdrw->x1;
       else                        l_x = l_col * src_gdrw->tile_width;
       if(l_col == l_last_col)     l_x2 = src_gdrw->x2;
       else                        l_x2 = (l_col +1) * src_gdrw->tile_width;

       if(cd->show_progress) gimp_progress_update (l_progress += l_progress_step);    
       
       for( ; l_x < l_x2; l_x++)
       {
         if(l_row == l_first_row)    l_y = src_gdrw->y1;
         else                        l_y = l_row * src_gdrw->tile_height;
         if(l_row == l_last_row)     l_y2 = src_gdrw->y2;
         else                        l_y2 = (l_row +1) * src_gdrw->tile_height ;

         /* printf("X: %4d Y:%4d Y2:%4d\n", (int)l_x, (int)l_y, (int)l_y2); */
       
         for( ; l_y < l_y2; l_y++)
         {
            /* ---------- copy SRC_PIXEL to curve position ------ */
            
	    p_get_pixel(src_gdrw, l_x, l_y, &color[0]);            

            l_curvy = p_curve_get_dy(cd, l_x, 
                                    (gint32)src_gdrw->drawable->width, 
                                    (gint32)src_gdrw->drawable->height, (gdouble)l_y);
            l_desty = l_y + l_topshift + l_curvy;
            
            /* ----------- SMOOTING ------------------ */
            if((cd->smoothing == TRUE) && (l_x > 0))
            {
               l_nb_curvy = p_curve_get_dy(cd, l_x -1, 
                                    (gint32)src_gdrw->drawable->width, 
                                    (gint32)src_gdrw->drawable->height, (gdouble)l_y);
               if((l_nb_curvy == l_curvy) && (l_x > 1))
               {
                    l_nb2_curvy = p_curve_get_dy(cd, l_x -2, 
                                    (gint32)src_gdrw->drawable->width, 
                                    (gint32)src_gdrw->drawable->height, (gdouble)l_y);
               }
               else
               {
                    l_nb2_curvy = l_nb_curvy;
               }
               p_put_mix_pixel(dst_gdrw, l_x, l_desty, &color[0], l_nb_curvy, l_nb2_curvy, l_curvy );
            }
            else
            {
               p_put_pixel(dst_gdrw, l_x, l_desty, &color[0]);
            }

            /* ----------- render ANTIALIAS ------------------ */

            if(cd->antialias)
            {
               l_othery = l_desty;
               
               if(l_y == src_gdrw->y1)             /* Upper outline */
               {
                  first_arr[l_x].y = l_curvy;
                  memcpy(&first_arr[l_x].color[0], &color[0], dst_gdrw->drawable->bpp);
                    
                  if(l_x > 0)
                  {
                    memcpy(&mixcolor[0], &first_arr[l_x-1].color[0], dst_gdrw->drawable->bpp);
                     
                    l_diff = abs(first_arr[l_x - 1].y - l_curvy) +1;
                    l_miny = MIN(first_arr[l_x - 1].y, l_curvy) -1;
                    l_maxy = MAX(first_arr[l_x - 1].y, l_curvy) +1;

		    l_othery = (src_gdrw->y2 -1) 
		             + l_topshift 
		             + p_curve_get_dy(cd, l_x, 
                                              (gint32)src_gdrw->drawable->width, 
                                              (gint32)src_gdrw->drawable->height,
				              (gdouble)(src_gdrw->y2 -1));
                   }
               }
               if(l_y == src_gdrw->y2 -1)      /* Lower outline */
               {
                  if(l_x > 0)
                  {
                    memcpy(&mixcolor[0], &last_arr[l_x-1].color[0], dst_gdrw->drawable->bpp);
                     
                    l_diff = abs(last_arr[l_x - 1].y - l_curvy) +1;
                    l_maxy = MAX(last_arr[l_x - 1].y, l_curvy) +1;
                    l_miny = MIN(last_arr[l_x - 1].y, l_curvy) -1;
                   }

		    l_othery = (src_gdrw->y1) 
		             + l_topshift 
		             + p_curve_get_dy(cd, l_x, 
                                              (gint32)src_gdrw->drawable->width, 
                                              (gint32)src_gdrw->drawable->height,
				              (gdouble)(src_gdrw->y1));
               }

	       if(l_desty < l_othery)        { l_alias_dir =  1; }  /* fade to transp. with descending dy */
	       else if(l_desty > l_othery)   { l_alias_dir = -1; }  /* fade to transp. with ascending dy */
	       else                          { l_alias_dir =  0; }  /* no antialias at curve crossing point(s) */
	       
	       if(l_alias_dir != 0)
	       {
	           l_alpha_lo = 20;
                   if (gimp_drawable_has_alpha(src_gdrw->drawable->id))
                   {
                     l_alpha_lo = MIN(20, mixcolor[src_gdrw->index_alpha]);
                   }
                     
	       
                   for(l_dy = 0; l_dy < l_diff; l_dy++)
                   {
                        /* iterate for fading alpha channel */
                        l_mixmask =  255 * ((gdouble)(l_dy+1) / (gdouble)(l_diff+1));
                        mixcolor[dst_gdrw->index_alpha] = MIX_CHANNEL(color[dst_gdrw->index_alpha], l_alpha_lo, l_mixmask);
			if(l_alias_dir > 0)
			{
                          p_put_pixel(dst_gdrw, l_x -1, l_y + l_topshift  + l_miny + l_dy, &mixcolor[0]);
			}
			else
			{
                          p_put_pixel(dst_gdrw, l_x -1, l_y + l_topshift  + (l_maxy - l_dy), &mixcolor[0]);
			}

                   }
	       }
            }

            /* ------------------ FILL HOLES ------------------ */

            if(l_y == src_gdrw->y1)
            {
               l_diff = 0;
               l_sign = 1;
            }
            else
            {
               l_diff = last_arr[l_x].y - l_curvy;
               if (l_diff < 0)
               {
                  l_diff = 0 - l_diff;
                  l_sign = -1;
               }
               else
               {
                  l_sign = 1;
               }
               
               memcpy(&mixcolor[0], &color[0], dst_gdrw->drawable->bpp);
            }
            
            for(l_dy = 1; l_dy <= l_diff; l_dy++)
            {
               /* y differs more than 1 pixel from last y in the destination
                * drawable. So we have to fill the empty space between
                * using a mixed color
                */

               if(cd->smoothing == TRUE)
               {
                 /* smooting is on, so we are using a mixed color */
                 l_mixmask =  255 * ((gdouble)(l_dy) / (gdouble)(l_diff+1));
                 mixcolor[0] =  MIX_CHANNEL(last_arr[l_x].color[0], color[0], l_mixmask);
                 mixcolor[1] =  MIX_CHANNEL(last_arr[l_x].color[1], color[1], l_mixmask);
                 mixcolor[2] =  MIX_CHANNEL(last_arr[l_x].color[2], color[2], l_mixmask);
                 mixcolor[3] =  MIX_CHANNEL(last_arr[l_x].color[3], color[3], l_mixmask);
               }
               else
               {
                  /* smooting is off, so we are using this color or the last color */
                  if(l_dy < l_diff / 2)
                  {
                    memcpy(&mixcolor[0], &color[0], dst_gdrw->drawable->bpp);
                  }
                  else
                  {
                    memcpy(&mixcolor[0], &last_arr[l_x].color[0], dst_gdrw->drawable->bpp);
                  }
               }
               
               if(cd->smoothing == TRUE)
               {
                  p_put_mix_pixel(dst_gdrw, l_x, l_desty + (l_dy * l_sign), &mixcolor[0],
                                  l_nb_curvy, l_nb2_curvy, l_curvy );
               }
               else
               {
                  p_put_pixel(dst_gdrw, l_x, l_desty + (l_dy * l_sign), &mixcolor[0]);               
               }
            }
            
            /* store y and color */ 
            last_arr[l_x].y = l_curvy;
            memcpy(&last_arr[l_x].color[0], &color[0], dst_gdrw->drawable->bpp);
            

         }
       }
     }
   }

   if(gb_debug) printf("ROWS: %d - %d  COLS: %d - %d\n", (int)l_first_row, (int)l_last_row, (int)l_first_col, (int)l_last_col);

   return 0;

}

/* ============================================================================
 * p_main_bend
 * ============================================================================
 */

gint32
p_main_bend(BenderDialog *cd, GDrawable *original_drawable, gint work_on_copy)
{
   t_GDRW  l_src_gdrw;
   t_GDRW  l_dst_gdrw;
   GDrawable *dst_drawable;
   GDrawable *src_drawable;
   gint32             l_dst_height;
   gint32    l_image_id;
   gint32    l_tmp_layer_id;
   gint32    l_interpolation;
   gint      l_offset_x, l_offset_y;
   gint      l_center_x, l_center_y;
   gint32    xmax, ymax;
  
   l_interpolation = cd->smoothing;
   l_image_id = gimp_layer_get_image_id(original_drawable->id);
   gimp_drawable_offsets(original_drawable->id, &l_offset_x, &l_offset_y);

   l_center_x = l_offset_x + (gimp_drawable_width  (original_drawable->id) / 2 );
   l_center_y = l_offset_y + (gimp_drawable_height (original_drawable->id) / 2 );
   
   /* always copy original_drawable to a tmp src_layer */  
   l_tmp_layer_id = gimp_layer_copy(original_drawable->id);
   
   if(gb_debug) printf("p_main_bend  l_tmp_layer_id %d\n", (int)l_tmp_layer_id);   
   
   if(cd->rotation != 0.0)
   {
      if(gb_debug) printf("p_main_bend rotate: %f\n", (float)cd->rotation);   
      p_gimp_rotate(l_image_id, l_tmp_layer_id, l_interpolation, cd->rotation);
   }
   src_drawable = gimp_drawable_get (l_tmp_layer_id);

   xmax = ymax = src_drawable->width -1;
   cd->curve_ptr[OUTLINE_UPPER] = g_malloc(sizeof(gint32) * (1+xmax));
   cd->curve_ptr[OUTLINE_LOWER] = g_malloc(sizeof(gint32) * (1+xmax));

   p_bender_calculate_iter_curve(cd, xmax, ymax);
   bender_init_min_max(cd, xmax);
   
   l_dst_height = src_drawable->height 
                + p_upper_curve_extend(cd, src_drawable->width, src_drawable->height)
                + p_lower_curve_extend(cd, src_drawable->width, src_drawable->height);

   if(gb_debug) printf("p_main_bend: l_dst_height:%d\n", (int)l_dst_height);   

   if(work_on_copy)
   {
     dst_drawable = p_add_layer(src_drawable->width, l_dst_height, src_drawable);
     if(gb_debug) printf("p_main_bend: DONE add layer\n");   
   }
   else
   {
     /* work on the original */
     gimp_layer_resize(original_drawable->id, 
                       src_drawable->width,
		       l_dst_height,
		       l_offset_x, l_offset_y);
     if(gb_debug) printf("p_main_bend: DONE layer resize\n");   
     if(!gimp_drawable_has_alpha(original_drawable->id))
     {
       /* always add alpha channel */
       gimp_layer_add_alpha(original_drawable->id);
     }
     dst_drawable = gimp_drawable_get (original_drawable->id);
   }
   p_clear_drawable(dst_drawable);

   p_init_gdrw(&l_src_gdrw, src_drawable, FALSE, FALSE);
   p_init_gdrw(&l_dst_gdrw, dst_drawable,  TRUE,  FALSE);

   p_vertical_bend(cd, &l_src_gdrw, &l_dst_gdrw);

   if(gb_debug) printf("p_main_bend: DONE vertical bend\n");   

   p_end_gdrw(&l_src_gdrw);
   p_end_gdrw(&l_dst_gdrw);

   if(cd->rotation != 0.0)
   {
      p_gimp_rotate(l_image_id, dst_drawable->id, l_interpolation, (gdouble)(360.0 - cd->rotation));
      
      /* TODO: here we should crop dst_drawable to cut off full transparent borderpixels */
      
   }

   /* set offsets of the resulting new layer 
    *(center == center of original_drawable)
    */
   l_offset_x = l_center_x - (gimp_drawable_width  (dst_drawable->id) / 2 );
   l_offset_y = l_center_y - (gimp_drawable_height  (dst_drawable->id) / 2 );
   gimp_layer_set_offsets (dst_drawable->id, l_offset_x, l_offset_y);

   /* delete the temp layer */
   gimp_image_remove_layer(l_image_id, l_tmp_layer_id);

   g_free(cd->curve_ptr[OUTLINE_UPPER]);
   g_free(cd->curve_ptr[OUTLINE_LOWER]);

   if(gb_debug) printf("p_main_bend: DONE bend main\n");   

   return dst_drawable->id;
}	/* end p_main_bend */

