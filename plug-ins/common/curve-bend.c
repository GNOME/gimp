/* curve_bend plugin for GIMP */

/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Revision history
 *  (2004/02/08)  v1.3.18 hof: #133244 exit with execution error if there is
 *                             an empty selection
 *  (2003/08/24)  v1.3.18 hof: #119937 show busy cursor when recalculating
 *                             preview
 *  (2002/09/xx)  v1.1.18 mitch and gsr: clean interface
 *  (2000/02/16)  v1.1.17b hof: added spinbuttons for rotate entry
 *  (2000/02/16)  v1.1.17 hof: undo bugfix (#6012)
 *                             don't call gimp_undo_push_group_end
 *                             after gimp_displays_flush
 *  (1999/09/13)  v1.01  hof: PDB-calls updated for gimp 1.1.9
 *  (1999/05/10)  v1.0   hof: first public release
 *  (1999/04/23)  v0.0   hof: coding started,
 *                            splines and dialog parts are similar to curves.c
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Defines */
#define PLUG_IN_PROC        "plug-in-curve-bend"
#define PLUG_IN_BINARY      "curve-bend"
#define PLUG_IN_ROLE        "gimp-curve-bend"
#define PLUG_IN_VERSION     "v1.3.18 (2003/08/26)"
#define PLUG_IN_IMAGE_TYPES "RGB*, GRAY*"
#define PLUG_IN_AUTHOR      "Wolfgang Hofer (hof@hotbot.com)"
#define PLUG_IN_COPYRIGHT   "Wolfgang Hofer"

#define PLUG_IN_ITER_NAME       "plug_in_curve_bend_Iterator"
#define PLUG_IN_DATA_ITER_FROM  "plug_in_curve_bend_ITER_FROM"
#define PLUG_IN_DATA_ITER_TO    "plug_in_curve_bend_ITER_TO"

#define KEY_POINTFILE "POINTFILE_CURVE_BEND"
#define KEY_POINTS    "POINTS"
#define KEY_VAL_Y     "VAL_Y"

#define MIDDLE 127

#define MIX_CHANNEL(a, b, m)  (((a * m) + (b * (255 - m))) / 255)

#define UP_GRAPH              0x1
#define UP_PREVIEW_EXPOSE     0x4
#define UP_PREVIEW            0x8
#define UP_DRAW               0x10
#define UP_ALL                0xFF

#define GRAPH_WIDTH      256
#define GRAPH_HEIGHT     256
#define PREVIEW_SIZE_X   256
#define PREVIEW_SIZE_Y   256
#define PV_IMG_WIDTH     128
#define PV_IMG_HEIGHT    128
#define RADIUS           3
#define MIN_DISTANCE     8
#define PREVIEW_BPP      4

#define SMOOTH       0
#define GFREE        1

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
  guchar  curve[2][256];    /* for curve_type freehand mode   0   <= curve  <= 255 */
  gdouble points[2][17][2]; /* for curve_type smooth mode     0.0 <= points <= 1.0 */

  gint    curve_type;

  gint    smoothing;
  gint    antialias;
  gint    work_on_copy;
  gdouble rotation;

  gint32  total_steps;
  gdouble current_step;
};

typedef struct _Curves Curves;

struct _Curves
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _BenderDialog BenderDialog;

struct _BenderDialog
{
  GtkWidget *shell;
  GtkWidget *outline_menu;
  GtkWidget *pv_widget;
  GtkWidget *graph;
  GtkAdjustment *rotate_data;
  GdkPixmap *pixmap;
  GtkWidget *filechooser;

  GdkCursor *cursor_busy;

  GimpDrawable *drawable;
  int        color;
  int        outline;
  gint       preview;

  int        grab_point;
  int        last;
  int        leftmost;
  int        rightmost;
  int        curve_type;
  gdouble    points[2][17][2];    /*  0.0 <= points    <= 1.0 */
  guchar     curve[2][256];       /*  0   <= curve     <= 255 */
  gint32    *curve_ptr[2];        /*  0   <= curve_ptr <= src_drawable_width
                                   *  both arrays are allocated dynamic,
                                   *  depending on drawable width
                                   */
  gint32     min2[2];
  gint32     max2[2];
  gint32     zero2[2];

  gboolean   show_progress;
  gboolean   smoothing;
  gboolean   antialias;
  gboolean   work_on_copy;
  gdouble    rotation;

  gint32     preview_image_id;
  gint32     preview_layer_id1;
  gint32     preview_layer_id2;

  BenderValues  *bval_from;
  BenderValues  *bval_to;
  BenderValues  *bval_curr;

  gboolean   run;
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
 *      |    .    |    |    |  ---   max                   |    .    |    |    |  ---   max
 *      +--.-+--.-+----+----+                              +--.-+--.-+----+----+
 *      .    |    .    |    |                    zero ___  .    |    .    |    |
 *      +----+----+-.--+----+                              +----+----+-.--+----+
 *      |    |    |    .    .  ---   zero                  |    |    |    .    .
 *      +----+----+----+-.-.+  ___   min                   +----+----+----+-.-.+  ___   min
 *      |    |    |    |    |                              |    |    |    |    |
 *    0 +----+----+----+----+                            0 +----+----+----+----+
 *      0                   255                            0                   255
 */

typedef double CRMatrix[4][4];

typedef struct
{
  GimpDrawable *drawable;
  gint       x1;
  gint       y1;
  gint       x2;
  gint       y2;
  gint       index_alpha;   /* 0 == no alpha, 1 == GREYA, 3 == RGBA */
  gint       bpp;
  GimpPixelFetcher *pft;
  gint       tile_width;
  gint       tile_height;
} t_GDRW;

typedef struct
{
  gint32 y;
  guchar color[4];
} t_Last;


/*  curves action functions  */
static void  query (void);
static void  run   (const gchar      *name,
                    gint              nparams,
                    const GimpParam  *param,
                    gint             *nreturn_vals,
                    GimpParam       **return_vals);

static BenderDialog *  bender_new_dialog              (GimpDrawable *);
static void            bender_update                  (BenderDialog *, int);
static void            bender_plot_curve              (BenderDialog *,
                                                       int, int, int, int,
                                                       gint32, gint32, gint);
static void            bender_calculate_curve         (BenderDialog *, gint32,
                                                       gint32, gint);
static void            bender_rotate_adj_callback     (GtkAdjustment *, gpointer);
static void            bender_border_callback         (GtkWidget *, gpointer);
static void            bender_type_callback           (GtkWidget *, gpointer);
static void            bender_reset_callback          (GtkWidget *, gpointer);
static void            bender_copy_callback           (GtkWidget *, gpointer);
static void            bender_copy_inv_callback       (GtkWidget *, gpointer);
static void            bender_swap_callback           (GtkWidget *, gpointer);
static void            bender_response                (GtkWidget *, gint,
                                                       BenderDialog *);
static void            bender_smoothing_callback      (GtkWidget *, gpointer);
static void            bender_antialias_callback      (GtkWidget *, gpointer);
static void            bender_work_on_copy_callback   (GtkWidget *, gpointer);
static void            bender_preview_update          (GtkWidget *, gpointer);
static void            bender_preview_update_once     (GtkWidget *, gpointer);
static void            bender_load_callback           (GtkWidget *,
                                                       BenderDialog *);
static void            bender_save_callback           (GtkWidget *,
                                                       BenderDialog *);
static gint            bender_graph_events            (GtkWidget *, GdkEvent *,
                                                       BenderDialog *);
static void            bender_CR_compose              (CRMatrix, CRMatrix,
                                                       CRMatrix);
static void            bender_init_min_max            (BenderDialog *, gint32);
static BenderDialog *  do_dialog                      (GimpDrawable *);
static void            p_init_gdrw                    (t_GDRW *gdrw, GimpDrawable *drawable,
                                                       int dirty, int shadow);
static void            p_end_gdrw                     (t_GDRW *gdrw);
static gint32          p_main_bend                    (BenderDialog *, GimpDrawable *, gint);
static gint32          p_create_pv_image              (GimpDrawable *src_drawable, gint32 *layer_id);
static void            p_render_preview               (BenderDialog *cd, gint32 layer_id);
static void            p_get_pixel                    (t_GDRW *gdrw,
                                                       gint32 x, gint32 y, guchar *pixel);
static void            p_put_pixel                    (t_GDRW *gdrw,
                                                       gint32 x, gint32 y, guchar *pixel);
static void            p_put_mix_pixel                (t_GDRW *gdrw,
                                                       gint32 x, gint32 y, guchar *color,
                                                       gint32 nb_curvy, gint32 nb2_curvy,
                                                       gint32 curvy);
static void            p_stretch_curves               (BenderDialog *cd, gint32 xmax, gint32 ymax);
static void            p_cd_to_bval                   (BenderDialog *cd, BenderValues *bval);
static void            p_cd_from_bval                 (BenderDialog *cd, BenderValues *bval);
static void            p_store_values                 (BenderDialog *cd);
static void            p_retrieve_values              (BenderDialog *cd);
static void            p_bender_calculate_iter_curve  (BenderDialog *cd, gint32 xmax, gint32 ymax);
static void            p_delta_gdouble                (double *val, double val_from, double val_to,
                                                       gint32 total_steps, gdouble current_step);
static void            p_delta_gint32                 (gint32 *val, gint32 val_from, gint32 val_to,
                                                       gint32 total_steps, gdouble current_step);
static void            p_copy_points                  (BenderDialog *cd, int outline, int xy,
                                                       int argc, gdouble *floatarray);
static void            p_copy_yval                    (BenderDialog *cd, int outline,
                                                       int argc, guint8 *int8array);
static int             p_save_pointfile               (BenderDialog *cd, const gchar *filename);


/* Global Variables */
const GimpPlugInInfo PLUG_IN_INFO =
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

static int gb_debug = FALSE;

/* Functions */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX PDB_STUFF XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

#ifdef ROTATE_OPTIMIZE

/* ============================================================================
 * p_pdb_procedure_available
 *   if requested procedure is available in the PDB return the number of args
 *      (0 upto n) that are needed to call the procedure.
 *   if not available return -1
 * ============================================================================
 */

static gint
p_pdb_procedure_available (const gchar *proc_name)
{
  gint             l_nparams;
  gint             l_nreturn_vals;
  GimpPDBProcType  l_proc_type;
  gchar           *l_proc_blurb;
  gchar           *l_proc_help;
  gchar           *l_proc_author;
  gchar           *l_proc_copyright;
  gchar           *l_proc_date;
  GimpParamDef    *l_params;
  GimpParamDef    *l_return_vals;
  gint             l_rc;

  l_rc = 0;

  /* Query the gimp application's procedural database
   *  regarding a particular procedure.
   */
  if (gimp_procedural_db_proc_info (proc_name,
                                    &l_proc_blurb,
                                    &l_proc_help,
                                    &l_proc_author,
                                    &l_proc_copyright,
                                    &l_proc_date,
                                    &l_proc_type,
                                    &l_nparams, &l_nreturn_vals,
                                    &l_params, &l_return_vals))
    {
      /* procedure found in PDB */
      return l_nparams;
    }

  g_printerr ("Warning: Procedure %s not found.\n", proc_name);
  return -1;
}

#endif /* ROTATE_OPTIMIZE */

static gint
p_gimp_rotate (gint32  image_id,
               gint32  drawable_id,
               gint32  interpolation,
               gdouble angle_deg)
{
  gdouble       l_angle_rad;
  gint          l_rc;

#ifdef ROTATE_OPTIMIZE
  static gchar *l_rotate_proc = "plug-in-rotate";
  GimpParam    *return_vals;
  gint          nreturn_vals;
  gint32        l_angle_step;
  gint          l_nparams;

  if     (angle_deg == 90.0)  { l_angle_step = 1; }
  else if(angle_deg == 180.0) { l_angle_step = 2; }
  else if(angle_deg == 270.0) { l_angle_step = 3; }
  else                        { l_angle_step = 0; }

  if (l_angle_step != 0)
    {
      l_nparams = p_pdb_procedure_available (l_rotate_proc);
      if (l_nparams == 5)
        {
          /* use faster rotate plugin on multiples of 90 degrees */
          return_vals = gimp_run_procedure (l_rotate_proc,
                                            &nreturn_vals,
                                            GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,
                                            GIMP_PDB_IMAGE, image_id,
                                            GIMP_PDB_DRAWABLE, drawable_id,
                                            GIMP_PDB_INT32, l_angle_step,
                                            GIMP_PDB_INT32, FALSE,         /* don't rotate the whole image */
                                            GIMP_PDB_END);

          if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
            {
              return 0;
            }
        }
    }
#endif /* ROTATE_OPTIMIZE */

  l_angle_rad = (angle_deg * G_PI) / 180.0;

  gimp_context_push ();
  if (! interpolation)
    gimp_context_set_interpolation (GIMP_INTERPOLATION_NONE);
  gimp_context_set_transform_resize (GIMP_TRANSFORM_RESIZE_ADJUST);
  l_rc = gimp_item_transform_rotate (drawable_id,
                                     l_angle_rad,
                                     TRUE /*auto_center*/,
                                     -1.0 /*center_x*/,
                                     -1.0 /*center_y*/);
  gimp_context_pop ();

  if (l_rc == -1)
    g_printerr ("Error: gimp_drawable_transform_rotate_default call failed\n");

  return l_rc;
}

/* ============================================================================
 * p_if_selection_float_it
 * ============================================================================
 */

static gint32
p_if_selection_float_it (gint32 image_id,
                         gint32 layer_id)
{
  if (! gimp_layer_is_floating_sel (layer_id))
    {
      gint32   l_sel_channel_id;
      gint32   l_x1, l_x2, l_y1, l_y2;
      gint32   non_empty;

      /* check and see if we have a selection mask */
      l_sel_channel_id  = gimp_image_get_selection (image_id);

      gimp_selection_bounds (image_id, &non_empty, &l_x1, &l_y1, &l_x2, &l_y2);

      if (non_empty && l_sel_channel_id >= 0)
        {
          /* selection is TRUE, make a layer (floating selection) from
             the selection  */
          if (gimp_edit_copy (layer_id))
            {
              layer_id = gimp_edit_paste (layer_id, FALSE);
            }
          else
            {
              return -1;
            }
        }
    }

  return layer_id;
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

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,      "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"},
    { GIMP_PDB_IMAGE,      "image", "Input image" },
    { GIMP_PDB_DRAWABLE,   "drawable", "Input drawable (must be a layer without layermask)"},
    { GIMP_PDB_FLOAT,      "rotation", "Direction {angle 0 to 360 degree } of the bend effect"},
    { GIMP_PDB_INT32,      "smoothing", "Smoothing { TRUE, FALSE }"},
    { GIMP_PDB_INT32,      "antialias", "Antialias { TRUE, FALSE }"},
    { GIMP_PDB_INT32,      "work-on-copy", "{ TRUE, FALSE } TRUE: copy the drawable and bend the copy"},
    { GIMP_PDB_INT32,      "curve-type", " { 0, 1 } 0 == smooth (use 17 points), 1 == freehand (use 256 val_y) "},
    { GIMP_PDB_INT32,      "argc-upper-point-x", "{2 <= argc <= 17} "},
    { GIMP_PDB_FLOATARRAY, "upper-point-x", "array of 17 x point_koords { 0.0 <= x <= 1.0 or -1 for unused point }"},
    { GIMP_PDB_INT32,      "argc-upper-point-y", "{2 <= argc <= 17} "},
    { GIMP_PDB_FLOATARRAY, "upper-point-y", "array of 17 y point_koords { 0.0 <= y <= 1.0 or -1 for unused point }"},
    { GIMP_PDB_INT32,      "argc-lower_point-x", "{2 <= argc <= 17} "},
    { GIMP_PDB_FLOATARRAY, "lower-point-x", "array of 17 x point_koords { 0.0 <= x <= 1.0 or -1 for unused point }"},
    { GIMP_PDB_INT32,      "argc-lower-point-y", "{2 <= argc <= 17} "},
    { GIMP_PDB_FLOATARRAY, "lower_point_y", "array of 17 y point_koords { 0.0 <= y <= 1.0 or -1 for unused point }"},
    { GIMP_PDB_INT32,      "argc-upper-val-y", "{ 256 } "},
    { GIMP_PDB_INT8ARRAY,  "upper-val-y",   "array of 256 y freehand koord { 0 <= y <= 255 }"},
    { GIMP_PDB_INT32,      "argc-lower-val-y", "{ 256 } "},
    { GIMP_PDB_INT8ARRAY,  "lower-val-y",   "array of 256 y freehand koord { 0 <= y <= 255 }"}
  };

  static const GimpParamDef return_vals[] =
  {
    { GIMP_PDB_LAYER, "bent-layer", "the handled layer" }
  };

  static const GimpParamDef args_iter[] =
  {
    { GIMP_PDB_INT32, "run-mode", "The run mode { RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_INT32, "total-steps", "total number of steps (# of layers-1 to apply the related plug-in)" },
    { GIMP_PDB_FLOAT, "current-step", "current (for linear iterations this is the layerstack position, otherwise some value in between)" },
    { GIMP_PDB_INT32, "len-struct", "length of stored data structure with id is equal to the plug_in  proc_name" },
  };

  /* the actual installation of the bend plugin */
  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Bend the image using two control curves"),
                          "This plug-in does bend the active layer "
                          "If there is a current selection it is copied to "
                          "floating selection and the curve_bend distortion "
                          "is done on the floating selection. If "
                          "work_on_copy parameter is TRUE, the curve_bend "
                          "distortion is done on a copy of the active layer "
                          "(or floating selection). The upper and lower edges "
                          "are bent in shape of 2 spline curves. both (upper "
                          "and lower) curves are determined by upto 17 points "
                          "or by 256 Y-Values if curve_type == 1 (freehand "
                          "mode) If rotation is not 0, the layer is rotated "
                          "before and rotated back after the bend operation. "
                          "This enables bending in other directions than "
                          "vertical. bending usually changes the size of "
                          "the handled layer. this plug-in sets the offsets "
                          "of the handled layer to keep its center at the "
                          "same position",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          PLUG_IN_VERSION,
                          N_("_Curve Bend..."),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
                          args,
                          return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Distorts");

   /* the installation of the Iterator procedure for the bend plugin */
  gimp_install_procedure (PLUG_IN_ITER_NAME,
                          "This procedure calculates the modified values "
                          "for one iterationstep for the call of "
                          "plug_in_curve_bend",
                          "",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          PLUG_IN_VERSION,
                          NULL,    /* do not appear in menus */
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args_iter), 0,
                          args_iter, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  const gchar  *l_env;
  BenderDialog *cd;
  GimpDrawable *l_active_drawable    = NULL;
  gint32        l_active_drawable_id = -1;
  gint32        l_image_id           = -1;
  gint32        l_layer_id           = -1;
  gint32        l_layer_mask_id      = -1;
  GError       *error                = NULL;

  /* Get the runmode from the in-parameters */
  GimpRunMode run_mode = param[0].data.d_int32;

  /* status variable, use it to check for errors in invocation usually only
     during non-interactive calling */
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  /*always return at least the status to the caller. */
  static GimpParam values[2];

  INIT_I18N ();

  cd = NULL;

  l_env = g_getenv ("BEND_DEBUG");
  if (l_env != NULL)
    {
      if((*l_env != 'n') && (*l_env != 'N')) gb_debug = 1;
    }

  if (gb_debug) g_printerr ("\n\nDEBUG: run %s\n", name);

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  values[1].type         = GIMP_PDB_LAYER;
  values[1].data.d_int32 = -1;

  *nreturn_vals = 2;
  *return_vals  = values;

  if (strcmp (name, PLUG_IN_ITER_NAME) == 0)
    {
      gint32       len_struct;
      gint32       total_steps;
      gdouble      current_step;
      BenderValues bval;                  /* current values while iterating */
      BenderValues bval_from, bval_to;    /* start and end values */

      /* Iterator procedure for animated calls is usually called from
       * "plug_in_gap_layers_run_animfilter"
       * (always run noninteractive)
       */
      if ((run_mode == GIMP_RUN_NONINTERACTIVE) && (nparams == 4))
        {
          total_steps  =  param[1].data.d_int32;
          current_step =  param[2].data.d_float;
          len_struct   =  param[3].data.d_int32;

          if (len_struct == sizeof (bval))
            {
              /* get _FROM and _TO data,
               * This data was stored by plug_in_gap_layers_run_animfilter
               */
              gimp_get_data (PLUG_IN_DATA_ITER_FROM, &bval_from);
              gimp_get_data (PLUG_IN_DATA_ITER_TO,   &bval_to);
              bval = bval_from;

              p_delta_gdouble (&bval.rotation, bval_from.rotation,
                               bval_to.rotation, total_steps, current_step);
              /* note: iteration of curve and points arrays would not
               *       give useful results.  (there might be different
               *       number of points in the from/to bender values )
               *       the iteration is done later, (see
               *       p_bender_calculate_iter_curve) when the curve
               *       is calculated.
               */

              bval.total_steps = total_steps;
              bval.current_step = current_step;

              gimp_set_data (PLUG_IN_PROC, &bval, sizeof (bval));
            }
          else
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
        }
      else
        {
          status = GIMP_PDB_CALLING_ERROR;
        }

      values[0].data.d_status = status;
      return;
    }

  /* get image and drawable */
  l_image_id = param[1].data.d_int32;
  l_layer_id = param[2].data.d_drawable;

  if (! gimp_item_is_layer (l_layer_id))
    {
      g_set_error (&error, 0, 0, "%s",
                   _("Can operate on layers only "
                     "(but was called on channel or mask)."));
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  /* check for layermask */
  l_layer_mask_id = gimp_layer_get_mask (l_layer_id);
  if (l_layer_mask_id >= 0)
    {
      g_set_error (&error, 0, 0, "%s",
                   _("Cannot operate on layers with masks."));
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  /* if there is a selection, make it the floating selection layer */
  l_active_drawable_id = p_if_selection_float_it (l_image_id, l_layer_id);
  if (l_active_drawable_id < 0)
    {
      /* could not float the selection because selection rectangle
       * is completely empty return GIMP_PDB_EXECUTION_ERROR
       */
      g_set_error (&error, 0, 0, "%s",
                   _("Cannot operate on empty selections."));
       status = GIMP_PDB_EXECUTION_ERROR;
    }
  else
    {
      l_active_drawable = gimp_drawable_get (l_active_drawable_id);
    }

  /* how are we running today? */
  if (status == GIMP_PDB_SUCCESS)
    {
      /* how are we running today? */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /* Possibly retrieve data from a previous run */
          /* gimp_get_data (PLUG_IN_PROC, &g_bndvals); */

          /* Get information from the dialog */
          cd = do_dialog (l_active_drawable);
          cd->show_progress = TRUE;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /* check to see if invoked with the correct number of parameters */
          if (nparams >= 20)
            {
              cd = g_new (BenderDialog, 1);
              cd->run = TRUE;
              cd->show_progress = TRUE;
              cd->drawable = l_active_drawable;

              cd->rotation      = param[3].data.d_float;
              cd->smoothing     = param[4].data.d_int32 != 0;
              cd->antialias     = param[5].data.d_int32 != 0;
              cd->work_on_copy  = param[6].data.d_int32 != 0;
              cd->curve_type    = param[7].data.d_int32 != 0;

              p_copy_points (cd, OUTLINE_UPPER, 0,
                             param[8].data.d_int32,
                             param[9].data.d_floatarray);
              p_copy_points (cd, OUTLINE_UPPER, 1,
                             param[10].data.d_int32,
                             param[11].data.d_floatarray);
              p_copy_points (cd, OUTLINE_LOWER, 0,
                             param[12].data.d_int32,
                             param[13].data.d_floatarray);
              p_copy_points (cd, OUTLINE_LOWER, 1,
                             param[14].data.d_int32,
                             param[15].data.d_floatarray);

              p_copy_yval (cd, OUTLINE_UPPER,
                           param[16].data.d_int32,
                           param[17].data.d_int8array);
              p_copy_yval (cd, OUTLINE_LOWER,
                           param[18].data.d_int32,
                           param[19].data.d_int8array);
            }
          else
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          cd = g_new (BenderDialog, 1);
          cd->run = TRUE;
          cd->show_progress = TRUE;
          cd->drawable = l_active_drawable;
          p_retrieve_values (cd);  /* Possibly retrieve data from a previous run */
          break;

        default:
          break;
        }
    }

  if (! cd)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /* Run the main function */

      if (cd->run)
        {
          gint32 l_bent_layer_id;

          gimp_image_undo_group_start (l_image_id);

          l_bent_layer_id = p_main_bend (cd, cd->drawable, cd->work_on_copy);

          gimp_image_undo_group_end (l_image_id);

          /* Store variable states for next run */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            {
              p_store_values (cd);
            }

          /* return the id of handled layer */
          values[1].data.d_int32 = l_bent_layer_id;
        }
      else
        {
          status = GIMP_PDB_CANCEL;
        }

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}

static int
p_save_pointfile (BenderDialog *cd,
                  const gchar  *filename)
{
  gint j;
  FILE *l_fp;

  l_fp = g_fopen(filename, "wb+");
  if (!l_fp)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  fprintf(l_fp, "%s\n", KEY_POINTFILE);
  fprintf(l_fp, "VERSION 1.0\n\n");

  fprintf(l_fp, "# points for upper and lower smooth curve (0.0 <= pt <= 1.0)\n");
  fprintf(l_fp, "# there are upto 17 points where unused points are set to -1\n");
  fprintf(l_fp, "#       UPPERX     UPPERY      LOWERX    LOWERY\n");
  fprintf(l_fp, "\n");

  for(j = 0; j < 17; j++)
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

  for (j = 0; j < 256; j++)
  {
    fprintf(l_fp, "%s %3d  %3d\n", KEY_VAL_Y,
               (int)cd->curve[OUTLINE_UPPER][j],
               (int)cd->curve[OUTLINE_LOWER][j]);
  }

  fclose(l_fp);
  return 0;
}

static int
p_load_pointfile (BenderDialog *cd,
                  const gchar  *filename)
{
  gint  l_pi, l_ci, l_n, l_len;
  FILE *l_fp;
  char  l_buff[2000];
  float l_fux, l_fuy, l_flx, l_fly;
  gint  l_iuy, l_ily ;

  l_fp = g_fopen(filename, "rb");
  if (!l_fp)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  l_pi = 0;
  l_ci = 0;

  if (! fgets (l_buff, 2000 - 1, l_fp))
    {
      g_message (_("Error while reading '%s': %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      fclose (l_fp);
      return -1;
    }

  if (strncmp(l_buff, KEY_POINTFILE, strlen(KEY_POINTFILE)) == 0)
  {
     while (NULL != fgets (l_buff, 2000-1, l_fp))
     {
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
             l_pi++;
           }
           else
           {
              printf("warning: BAD points[%d] in file %s are ignored\n", l_pi, filename);
           }
        }
        l_len = strlen(KEY_VAL_Y);
        if (strncmp(l_buff, KEY_VAL_Y, l_len) == 0)
        {
           l_n = sscanf(&l_buff[l_len], "%d %d", &l_iuy, &l_ily);
           if ((l_n == 2) && (l_ci < 256))
           {
             cd->curve[OUTLINE_UPPER][l_ci] = l_iuy;
             cd->curve[OUTLINE_LOWER][l_ci] = l_ily;
             l_ci++;
           }
           else
           {
              printf("warning: BAD y_vals[%d] in file %s are ignored\n", l_ci, filename);
           }
        }

     }
  }
  fclose(l_fp);
  return 0;
}

static void
p_cd_to_bval (BenderDialog *cd,
              BenderValues *bval)
{
  gint i,j;

  for (i = 0; i < 2; i++)
  {
    for(j = 0; j < 256; j++)
    {
      bval->curve[i][j] = cd->curve[i][j];
    }

    for(j = 0; j < 17; j++)
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
  bval->current_step = 0.0;
}

static void
p_cd_from_bval(BenderDialog *cd, BenderValues *bval)
{
  gint i,j;

  for(i = 0; i < 2; i++)
  {
    for(j = 0; j < 256; j++)
    {
      cd->curve[i][j] = bval->curve[i][j];
    }

    for(j = 0; j < 17; j++)
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

static void
p_store_values (BenderDialog *cd)
{
  BenderValues l_bval;

  p_cd_to_bval(cd, &l_bval);
  gimp_set_data(PLUG_IN_PROC, &l_bval, sizeof(l_bval));
}

static void
p_retrieve_values (BenderDialog *cd)
{
  BenderValues l_bval;

  l_bval.total_steps = 0;
  l_bval.current_step = -444.4;  /* init with an invalid  dummy value */

  gimp_get_data (PLUG_IN_PROC, &l_bval);

  if (l_bval.total_steps == 0)
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
    cd->bval_from = g_new (BenderValues, 1);
    cd->bval_to   = g_new (BenderValues, 1);
    cd->bval_curr = g_new (BenderValues, 1);
    *cd->bval_curr = l_bval;

    /* it seems that we are called from GAP with "Varying Values" */
    gimp_get_data(PLUG_IN_DATA_ITER_FROM, cd->bval_from);
    gimp_get_data(PLUG_IN_DATA_ITER_TO,   cd->bval_to);
    *cd->bval_curr = l_bval;
    p_cd_from_bval(cd, cd->bval_curr);
    cd->work_on_copy = FALSE;
  }
}

static void
p_delta_gdouble (double  *val,
                 double   val_from,
                 double   val_to,
                 gint32   total_steps,
                 gdouble  current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
}

static void
p_delta_gint32 (gint32  *val,
                gint32   val_from,
                gint32   val_to,
                gint32   total_steps,
                gdouble  current_step)
{
    double     delta;

    if (total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
}

static void
p_copy_points (BenderDialog *cd,
               int           outline,
               int           xy,
               int           argc,
               gdouble      *floatarray)
{
   int j;

   for (j = 0; j < 17; j++)
   {
     cd->points[outline][j][xy] = -1;
   }
   for(j = 0; j < argc; j++)
   {
     cd->points[outline][j][xy] = floatarray[j];
   }
}

static void
p_copy_yval (BenderDialog *cd,
             int           outline,
             int           argc,
             guint8       *int8array)
{
   int j;
   guchar fill;

   fill = MIDDLE;
   for (j = 0; j < 256; j++)
   {
     if (j < argc)
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

static BenderDialog *
do_dialog (GimpDrawable *drawable)
{
  BenderDialog *cd;

  /* Init GTK  */
  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  /*  The curve_bend dialog  */
  cd = bender_new_dialog (drawable);

  /* create temporary image (with a small copy of drawable) for the preview */
  cd->preview_image_id = p_create_pv_image(drawable, &cd->preview_layer_id1);
  cd->preview_layer_id2 = -1;

  if (! gtk_widget_get_visible (cd->shell))
    gtk_widget_show (cd->shell);

  bender_update (cd, UP_GRAPH | UP_DRAW | UP_PREVIEW_EXPOSE);

  gtk_main ();
  gdk_display_flush (gtk_widget_get_display (cd->shell));

  gimp_image_delete(cd->preview_image_id);
  cd->preview_image_id = -1;
  cd->preview_layer_id1 = -1;
  cd->preview_layer_id2 = -1;

  return cd;
}

/**************************/
/*  Select Curves dialog  */
/**************************/

static BenderDialog *
bender_new_dialog (GimpDrawable *drawable)
{
  BenderDialog *cd;
  GtkWidget    *main_hbox;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *vbox2;
  GtkWidget    *abox;
  GtkWidget    *frame;
  GtkWidget    *upper, *lower;
  GtkWidget    *smooth, *freew;
  GtkWidget    *toggle;
  GtkWidget    *button;
  GtkWidget    *spinbutton;
  GtkWidget    *label;
  GdkDisplay   *display;
  gint          i, j;

  cd = g_new (BenderDialog, 1);

  cd->preview = FALSE;
  cd->curve_type = SMOOTH;
  cd->pixmap = NULL;
  cd->filechooser = NULL;
  cd->outline = OUTLINE_UPPER;
  cd->show_progress = FALSE;
  cd->smoothing = TRUE;
  cd->antialias = TRUE;
  cd->work_on_copy = FALSE;
  cd->rotation = 0.0;       /* vertical bend */

  cd->drawable = drawable;
  cd->color = gimp_drawable_is_rgb (cd->drawable->drawable_id);

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

  /*  The shell and main vbox  */
  cd->shell = gimp_dialog_new (_("Curve Bend"), PLUG_IN_ROLE,
                               NULL, 0,
                               gimp_standard_help_func, PLUG_IN_PROC,

                               _("_Cancel"), GTK_RESPONSE_CANCEL,
                               _("_OK"),     GTK_RESPONSE_OK,

                               NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (cd->shell),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (cd->shell));

  g_signal_connect (cd->shell, "response",
                    G_CALLBACK (bender_response),
                    cd);

  /*  busy cursor  */
  display = gtk_widget_get_display (cd->shell);
  cd->cursor_busy = gdk_cursor_new_for_display (display, GDK_WATCH);

  /*  The main hbox  */
  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (cd->shell))),
                      main_hbox, TRUE, TRUE, 0);
  gtk_widget_show (main_hbox);

  /* Left side column */
  vbox =  gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* Preview area, top of column */
  frame = gimp_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox2), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  /*  The range drawing area  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);

  cd->pv_widget = gimp_preview_area_new ();
  gtk_widget_set_size_request (cd->pv_widget,
                               PREVIEW_SIZE_X, PREVIEW_SIZE_Y);
  gtk_container_add (GTK_CONTAINER (frame), cd->pv_widget);
  gtk_widget_show (cd->pv_widget);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_end (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  The preview button  */
  button = gtk_button_new_with_mnemonic (_("_Preview Once"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_preview_update_once),
                    cd);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_mnemonic (_("Automatic pre_view"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cd->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (bender_preview_update),
                    cd);

  /* Options area, bottom of column */
  frame = gimp_frame_new (_("Options"));
  gtk_box_pack_end (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* Render Options  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  Rotate spinbutton  */
  label = gtk_label_new_with_mnemonic (_("Rotat_e:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  cd->rotate_data = gtk_adjustment_new (0, 0.0, 360.0, 1, 45, 0);
  gtk_adjustment_set_value (cd->rotate_data, cd->rotation);

  spinbutton = gtk_spin_button_new (cd->rotate_data, 0.5, 1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  g_signal_connect (cd->rotate_data, "value-changed",
                    G_CALLBACK (bender_rotate_adj_callback),
                    cd);

  /*  The smoothing toggle  */
  toggle = gtk_check_button_new_with_mnemonic (_("Smoo_thing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cd->smoothing);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (bender_smoothing_callback),
                    cd);

  /*  The antialiasing toggle  */
  toggle = gtk_check_button_new_with_mnemonic (_("_Antialiasing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cd->antialias);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (bender_antialias_callback),
                    cd);

  /*  The work_on_copy toggle  */
  toggle = gtk_check_button_new_with_mnemonic (_("Work on cop_y"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cd->work_on_copy);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (bender_work_on_copy_callback),
                    cd);

  /*  The curves graph  */
  frame = gimp_frame_new (_("Modify Curves"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  cd->graph = gtk_drawing_area_new ();
  gtk_widget_set_size_request (cd->graph,
                               GRAPH_WIDTH + RADIUS * 2,
                               GRAPH_HEIGHT + RADIUS * 2);
  gtk_widget_set_events (cd->graph, GRAPH_MASK);
  gtk_container_add (GTK_CONTAINER (abox), cd->graph);
  gtk_widget_show (cd->graph);

  g_signal_connect (cd->graph, "event",
                    G_CALLBACK (bender_graph_events),
                    cd);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gimp_int_radio_group_new (TRUE, _("Curve for Border"),
                                    G_CALLBACK (bender_border_callback),
                                    &cd->outline, cd->outline,

                                    C_("curve-border", "_Upper"), OUTLINE_UPPER, &upper,
                                    C_("curve-border", "_Lower"), OUTLINE_LOWER, &lower,

                                    NULL);

  g_object_set_data (G_OBJECT (upper), "cd", cd);
  g_object_set_data (G_OBJECT (lower), "cd", cd);

  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  frame = gimp_int_radio_group_new (TRUE, _("Curve Type"),
                                    G_CALLBACK (bender_type_callback),
                                    &cd->curve_type, cd->curve_type,

                                    _("Smoot_h"), SMOOTH, &smooth,
                                    _("_Free"),   GFREE,  &freew,

                                    NULL);
  g_object_set_data (G_OBJECT (smooth), "cd", cd);
  g_object_set_data (G_OBJECT (freew), "cd", cd);

  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /*  hbox for curve options  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  The Copy button  */
  button = gtk_button_new_with_mnemonic (_("_Copy"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Copy the active curve to the other border"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_copy_callback),
                    cd);

  /*  The CopyInv button  */
  button = gtk_button_new_with_mnemonic (_("_Mirror"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Mirror the active curve to the other border"),
                           NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_copy_inv_callback),
                    cd);

  /*  The Swap button  */
  button = gtk_button_new_with_mnemonic (_("S_wap"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Swap the two curves"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_swap_callback),
                    cd);

  /*  The Reset button  */
  button = gtk_button_new_with_mnemonic (_("_Reset"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Reset the active curve"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_reset_callback),
                    cd);

  /*  hbox for curve load and save  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  The Load button  */
  button = gtk_button_new_with_mnemonic (_("_Open"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Load the curves from a file"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_load_callback),
                    cd);

  /*  The Save button  */
  button = gtk_button_new_with_mnemonic (_("_Save"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Save the curves to a file"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_save_callback),
                    cd);

  gtk_widget_show (main_hbox);

  return cd;
}

static void
bender_update (BenderDialog *cd,
               int           update)
{
  GtkStyle *graph_style = gtk_widget_get_style (cd->graph);
  gint      i;
  gint      other;

  if (update & UP_PREVIEW)
    {
      gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (cd->shell)),
                             cd->cursor_busy);
      gdk_display_flush (gtk_widget_get_display (cd->shell));

      if (cd->preview_layer_id2 >= 0)
         gimp_image_remove_layer(cd->preview_image_id, cd->preview_layer_id2);

      cd->preview_layer_id2 = p_main_bend(cd, gimp_drawable_get (cd->preview_layer_id1), TRUE /* work_on_copy*/ );
      p_render_preview(cd, cd->preview_layer_id2);

      if (update & UP_DRAW)
        gtk_widget_queue_draw (cd->pv_widget);

      gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (cd->shell)),
                             NULL);
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
        gtk_widget_queue_draw (cd->pv_widget);
    }
  if ((update & UP_GRAPH) && (update & UP_DRAW) && cd->pixmap != NULL)
    {
      cairo_t *cr;

      cr = gdk_cairo_create (gtk_widget_get_window (cd->graph));

      cairo_set_line_width (cr, 1.0);
      cairo_translate (cr, 0.5, 0.5);

      /*  Clear the background  */
      gdk_cairo_set_source_color (cr, &graph_style->bg[GTK_STATE_NORMAL]);
      cairo_paint (cr);

      /*  Draw the grid lines  */
      for (i = 0; i < 5; i++)
        {
          cairo_move_to (cr, RADIUS, i * (GRAPH_HEIGHT / 4) + RADIUS);
          cairo_line_to (cr, GRAPH_WIDTH + RADIUS, i * (GRAPH_HEIGHT / 4) + RADIUS);

          cairo_move_to (cr, i * (GRAPH_WIDTH / 4) + RADIUS, RADIUS);
          cairo_line_to (cr, i * (GRAPH_WIDTH / 4) + RADIUS, GRAPH_HEIGHT + RADIUS);
        }

      gdk_cairo_set_source_color (cr, &graph_style->dark[GTK_STATE_NORMAL]);
      cairo_stroke (cr);

      /*  Draw the other curve  */
      other = (cd->outline == 0) ? 1 : 0;

      cairo_move_to (cr, RADIUS, 255 - cd->curve[other][0] + RADIUS);

      for (i = 1; i < 256; i++)
        {
          cairo_line_to (cr, i + RADIUS, 255 - cd->curve[other][i] + RADIUS);
        }

      gdk_cairo_set_source_color (cr, &graph_style->dark[GTK_STATE_NORMAL]);
      cairo_stroke (cr);

      /*  Draw the active curve  */
      cairo_move_to (cr, RADIUS, 255 - cd->curve[cd->outline][0] + RADIUS);

      for (i = 1; i < 256; i++)
        {
          cairo_line_to (cr, i + RADIUS, 255 - cd->curve[cd->outline][i] + RADIUS);
        }

      /*  Draw the points  */
      if (cd->curve_type == SMOOTH)
        {
          for (i = 0; i < 17; i++)
            {
              if (cd->points[cd->outline][i][0] != -1)
                {
                  cairo_new_sub_path (cr);
                  cairo_arc (cr,
                             (cd->points[cd->outline][i][0] * 255.0) + RADIUS,
                             255 - (cd->points[cd->outline][i][1] * 255.0) + RADIUS,
                             RADIUS,
                             0, 2 * G_PI);
                }
            }
        }

      gdk_cairo_set_source_color (cr, &graph_style->black);
      cairo_stroke (cr);

      cairo_destroy (cr);
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


  if (fix255)
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
bender_calculate_curve (BenderDialog *cd,
                        gint32        xmax,
                        gint32        ymax,
                        gint          fix255)
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
bender_rotate_adj_callback (GtkAdjustment *adjustment,
                            gpointer       client_data)
{
  BenderDialog *cd = client_data;

  if (gtk_adjustment_get_value (adjustment) != cd->rotation)
    {
      cd->rotation = gtk_adjustment_get_value (adjustment);
      if (cd->preview)
        bender_update (cd, UP_PREVIEW | UP_DRAW);
    }
}

static void
bender_border_callback (GtkWidget *widget,
                        gpointer   data)
{
  BenderDialog *cd;

  gimp_radio_button_update (widget, data);
  cd = g_object_get_data (G_OBJECT (widget), "cd");
  bender_update (cd, UP_GRAPH | UP_DRAW);
}

static void
bender_type_callback (GtkWidget *widget,
                      gpointer   data)
{
  BenderDialog *cd;

  gimp_radio_button_update (widget, data);

  cd = g_object_get_data (G_OBJECT (widget), "cd");
  if (! cd)
    return;

  if (cd->curve_type == SMOOTH)
    {
      gint i;

      /*  pick representative points from the curve and make them control points  */
      for (i = 0; i <= 8; i++)
        {
          gint index = CLAMP ((i * 32), 0, 255);
          cd->points[cd->outline][i * 2][0] = (gdouble)index / 255.0;
          cd->points[cd->outline][i * 2][1] = (gdouble)cd->curve[cd->outline][index] / 255.0;
        }

      bender_calculate_curve (cd, 255, 255, TRUE);
      bender_update (cd, UP_GRAPH | UP_DRAW);

      if (cd->preview)
        bender_update (cd, UP_PREVIEW | UP_DRAW);
    }
  else
    {
      bender_update (cd, UP_GRAPH | UP_DRAW);
    }
}

static void
bender_reset_callback (GtkWidget *widget,
                       gpointer   client_data)
{
  BenderDialog *cd = (BenderDialog *) client_data;
  gint          i;

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
  BenderDialog *cd = (BenderDialog *) client_data;
  int i;
  int other;

  other = (cd->outline) ? 0 : 1;

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
  BenderDialog *cd = (BenderDialog*) client_data;
  int i;
  int other;

  other = (cd->outline) ? 0 : 1;

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
  BenderDialog *cd = (BenderDialog*) client_data;
  int i;
  int other;
  gdouble hd;
  guchar  hu;

  other = (cd->outline) ? 0 : 1;

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
bender_response (GtkWidget    *widget,
                 gint          response_id,
                 BenderDialog *cd)
{
  if (response_id == GTK_RESPONSE_OK)
    cd->run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (cd->shell));
  gtk_main_quit ();
}

static void
bender_preview_update (GtkWidget *widget,
                       gpointer   data)
{
  BenderDialog *cd = (BenderDialog*) data;

  cd->preview = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if(cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_preview_update_once (GtkWidget *widget,
                            gpointer   data)
{
  BenderDialog *cd = (BenderDialog*) data;

  bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
p_points_save_to_file_response (GtkWidget    *dialog,
                                gint          response_id,
                                BenderDialog *cd)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      p_save_pointfile (cd, filename);

      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}

static void
p_points_load_from_file_response (GtkWidget    *dialog,
                                  gint          response_id,
                                  BenderDialog *cd)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      p_load_pointfile (cd, filename);
      bender_update (cd, UP_ALL);

      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}

static void
bender_load_callback (GtkWidget    *w,
                      BenderDialog *cd)
{
  if (! cd->filechooser)
    {
      cd->filechooser =
        gtk_file_chooser_dialog_new (_("Load Curve Points from File"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (w)),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Open"),   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (cd->filechooser),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      gtk_dialog_set_default_response (GTK_DIALOG (cd->filechooser),
                                       GTK_RESPONSE_OK);

      g_signal_connect (cd->filechooser, "response",
                        G_CALLBACK (p_points_load_from_file_response),
                        cd);
      g_signal_connect (cd->filechooser, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &cd->filechooser);
    }

  gtk_window_present (GTK_WINDOW (cd->filechooser));
}

static void
bender_save_callback (GtkWidget    *w,
                      BenderDialog *cd)
{
  if (! cd->filechooser)
    {
      cd->filechooser =
        gtk_file_chooser_dialog_new (_("Save Curve Points to File"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (w)),
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Save"),   GTK_RESPONSE_OK,

                                     NULL);

      g_signal_connect (cd->filechooser, "response",
                        G_CALLBACK (p_points_save_to_file_response),
                        cd);
      g_signal_connect (cd->filechooser, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &cd->filechooser);

      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (cd->filechooser),
                                         "curve_bend.points");
    }

  gtk_window_present (GTK_WINDOW (cd->filechooser));
}

static void
bender_smoothing_callback (GtkWidget *w,
                           gpointer   data)
{
  BenderDialog *cd = (BenderDialog*) data;

  cd->smoothing = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

  if(cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_antialias_callback (GtkWidget *w,
                           gpointer   data)
{
  BenderDialog *cd = (BenderDialog*) data;

  cd->antialias = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

  if (cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_work_on_copy_callback (GtkWidget *w,
                              gpointer   data)
{
  BenderDialog *cd = (BenderDialog*) data;

  cd->work_on_copy = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
}

static gboolean
bender_graph_events (GtkWidget    *widget,
                     GdkEvent     *event,
                     BenderDialog *cd)
{
  static GdkCursorType cursor_type = GDK_TOP_LEFT_ARROW;
  GdkCursorType new_type;
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
  gdk_window_get_pointer (gtk_widget_get_window (cd->graph), &tx, &ty, NULL);
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
        cd->pixmap = gdk_pixmap_new (gtk_widget_get_window (cd->graph),
                                     GRAPH_WIDTH + RADIUS * 2,
                                     GRAPH_HEIGHT + RADIUS * 2, -1);

      bender_update (cd, UP_GRAPH | UP_DRAW);
      break;

    case GDK_BUTTON_PRESS:
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
          /* change_win_cursor (gtk_widget_get_window (cd->graph), cursor_type); */
        }
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
  gint i, j;

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

static void
p_render_preview (BenderDialog *cd,
                  gint32        layer_id)
{
   guchar        l_pixel[4];
   guchar       *l_buf, *l_ptr;
   GimpDrawable *l_pv_drawable;
   gint          l_x, l_y;
   gint          l_ofx, l_ofy;
   t_GDRW        l_gdrw;
   t_GDRW       *gdrw;

   l_pv_drawable = gimp_drawable_get (layer_id);

   l_ptr = l_buf = g_new (guchar, PREVIEW_BPP * PREVIEW_SIZE_X * PREVIEW_SIZE_Y);
   gdrw = &l_gdrw;
   p_init_gdrw(gdrw, l_pv_drawable, FALSE, FALSE);

  /* offsets to set bend layer to preview center */
  l_ofx = (l_pv_drawable->width / 2) - (PREVIEW_SIZE_X / 2);
  l_ofy = (l_pv_drawable->height / 2) - (PREVIEW_SIZE_Y / 2);

  /* render preview */
  for (l_y = 0; l_y < PREVIEW_SIZE_Y; l_y++)
  {
     for (l_x = 0; l_x < PREVIEW_SIZE_X; l_x++)
     {
        p_get_pixel(gdrw, l_x + l_ofx, l_y + l_ofy, &l_pixel[0]);

        if (cd->color)
        {
           l_ptr[0] = l_pixel[0];
           l_ptr[1] = l_pixel[1];
           l_ptr[2] = l_pixel[2];
        }
        else
        {
           l_ptr[0] = l_pixel[0];
           l_ptr[1] = l_pixel[0];
           l_ptr[2] = l_pixel[0];
        }
        l_ptr[3] = l_pixel[gdrw->index_alpha];

        l_ptr += PREVIEW_BPP;
     }
  }
  gimp_preview_area_draw (GIMP_PREVIEW_AREA (cd->pv_widget),
                          0, 0, PREVIEW_SIZE_X, PREVIEW_SIZE_Y,
                          GIMP_RGBA_IMAGE,
                          l_buf,
                          PREVIEW_BPP * PREVIEW_SIZE_X);
  g_free (l_buf);

  p_end_gdrw(gdrw);
}       /* end p_render_preview */

/* ===================================================== */
/* curve_bend worker procedures                          */
/* ===================================================== */

static void
p_stretch_curves (BenderDialog *cd,
                  gint32        xmax,
                  gint32        ymax)
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
    }
  }
}

static void
bender_init_min_max (BenderDialog *cd,
                     gint32        xmax)
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
}

static gint32
p_curve_get_dy (BenderDialog *cd,
                gint32        x,
                gint32        drawable_width,
                gint32        total_steps,
                gdouble       current_step)
{
  /* get y values of both upper and lower curve,
   * and return the iterated value in between
   */
  gdouble     l_y1,  l_y2;
  gdouble     delta;

  l_y1 = cd->zero2[OUTLINE_UPPER] - cd->curve_ptr[OUTLINE_UPPER][x];
  l_y2 = cd->zero2[OUTLINE_LOWER] - cd->curve_ptr[OUTLINE_LOWER][x];

  delta = ((double)(l_y2 - l_y1) / (double)(total_steps -1)) * current_step;
  return SIGNED_ROUND(l_y1 + delta);
}

static gint32
p_upper_curve_extend (BenderDialog *cd,
                      gint32        drawable_width,
                      gint32        drawable_height)
{
   gint32  l_y1,  l_y2;

   l_y1 = cd->max2[OUTLINE_UPPER] - cd->zero2[OUTLINE_UPPER];
   l_y2 = (cd->max2[OUTLINE_LOWER] - cd->zero2[OUTLINE_LOWER]) - drawable_height;

   return MAX(l_y1, l_y2);
}

static gint32
p_lower_curve_extend (BenderDialog *cd,
                      gint32        drawable_width,
                      gint32        drawable_height)
{
   gint32  l_y1,  l_y2;

   l_y1 = cd->zero2[OUTLINE_LOWER] - cd->min2[OUTLINE_LOWER];
   l_y2 = (cd->zero2[OUTLINE_UPPER] - cd->min2[OUTLINE_UPPER]) - drawable_height;

   return MAX(l_y1, l_y2);
}

static void
p_end_gdrw (t_GDRW *gdrw)
{
  gimp_pixel_fetcher_destroy (gdrw->pft);
}

static void
p_init_gdrw (t_GDRW       *gdrw,
             GimpDrawable *drawable,
             int           dirty,
             int           shadow)
{
  gint w, h;

  gdrw->drawable = drawable;
  gdrw->pft = gimp_pixel_fetcher_new (drawable, FALSE);
  gimp_pixel_fetcher_set_edge_mode (gdrw->pft, GIMP_PIXEL_FETCHER_EDGE_BLACK);
  gdrw->tile_width = gimp_tile_width ();
  gdrw->tile_height = gimp_tile_height ();

  if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                      &gdrw->x1, &gdrw->y1, &w, &h))
    {
      w = 0;
      h = 0;
    }

  gdrw->x2 = gdrw->x1 + w;
  gdrw->y2 = gdrw->y1 + h;

  gdrw->bpp = drawable->bpp;
  if (gimp_drawable_has_alpha (drawable->drawable_id))
    {
      /* index of the alpha channelbyte {1|3} */
      gdrw->index_alpha = gdrw->bpp - 1;
    }
  else
    {
      gdrw->index_alpha = 0;      /* there is no alpha channel */
    }
}

/* get pixel value
 *   return light transparent black gray pixel if out of bounds
 *   (should occur in the previews only)
 */
static void
p_get_pixel (t_GDRW *gdrw,
             gint32  x,
             gint32  y,
             guchar *pixel)
{
  pixel[1] = 255;
  pixel[3] = 255;  /* simulate full visible alpha channel */
  gimp_pixel_fetcher_get_pixel (gdrw->pft, x, y, pixel);
}

static void
p_put_pixel (t_GDRW *gdrw,
             gint32  x,
             gint32  y,
             guchar *pixel)
{
  gimp_pixel_fetcher_put_pixel (gdrw->pft, x, y, pixel);
}

static void
p_put_mix_pixel (t_GDRW *gdrw,
                 gint32  x,
                 gint32  y,
                 guchar *color,
                 gint32  nb_curvy,
                 gint32  nb2_curvy,
                 gint32  curvy)
{
   guchar  l_pixel[4];
   guchar  l_mixmask;
   gint    l_idx;
   gint    l_diff;

   l_mixmask = 255 - 96;
   l_diff = abs(nb_curvy - curvy);
   if (l_diff == 0)
   {
     l_mixmask = 255 - 48;
     l_diff = abs(nb2_curvy - curvy);

     if (l_diff == 0)
     {
       /* last 2 neighbours were not shifted against current pixel, do not mix */
       p_put_pixel(gdrw, x, y, color);
       return;
     }
   }

   /* get left neighbour pixel */
   p_get_pixel(gdrw, x-1, y, &l_pixel[0]);

   if (l_pixel[gdrw->index_alpha] < 10)
   {
     /* neighbour is (nearly or full) transparent, do not mix */
     p_put_pixel(gdrw, x, y, color);
     return;
   }

   for (l_idx = 0; l_idx < gdrw->index_alpha ; l_idx++)
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

static void
p_clear_drawable (GimpDrawable *drawable)
{
   GimpPixelRgn  pixel_rgn;
   gpointer      pr;
   guint         l_row;
   guchar       *l_ptr;

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
}       /* end  p_clear_drawable */

/* ============================================================================
 * p_create_pv_image
 * ============================================================================
 */
static gint32
p_create_pv_image (GimpDrawable *src_drawable,
                   gint32    *layer_id)
{
  gint32        l_new_image_id;
  guint         l_new_width;
  guint         l_new_height;
  GimpImageType l_type;
  guint         l_x, l_y;
  double        l_scale;
  guchar        l_pixel[4];
  GimpDrawable *dst_drawable;
  t_GDRW        l_src_gdrw;
  t_GDRW        l_dst_gdrw;

  l_new_image_id = gimp_image_new (PREVIEW_SIZE_X, PREVIEW_SIZE_Y,
                   gimp_image_base_type (gimp_item_get_image (src_drawable->drawable_id)));
  gimp_image_undo_disable (l_new_image_id);

  l_type = gimp_drawable_type(src_drawable->drawable_id);
  if (src_drawable->height > src_drawable->width)
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
                             0);       /* mode NORMAL */
  if (!gimp_drawable_has_alpha(*layer_id))
  {
    /* always add alpha channel */
    gimp_layer_add_alpha(*layer_id);
  }

  gimp_image_insert_layer(l_new_image_id, *layer_id, -1, 0);

  dst_drawable = gimp_drawable_get (*layer_id);
  p_init_gdrw(&l_src_gdrw, src_drawable, FALSE, FALSE);
  p_init_gdrw(&l_dst_gdrw, dst_drawable,  TRUE,  FALSE);

  for (l_y = 0; l_y < l_new_height; l_y++)
  {
     for (l_x = 0; l_x < l_new_width; l_x++)
     {
       p_get_pixel(&l_src_gdrw, l_x * l_scale, l_y * l_scale, &l_pixel[0]);
       p_put_pixel(&l_dst_gdrw, l_x, l_y, &l_pixel[0]);
     }
  }

  p_end_gdrw(&l_src_gdrw);
  p_end_gdrw(&l_dst_gdrw);

  /* gimp_display_new(l_new_image_id); */
  return l_new_image_id;
}

/* ============================================================================
 * p_add_layer
 * ============================================================================
 */
static GimpDrawable*
p_add_layer (gint       width,
             gint       height,
             GimpDrawable *src_drawable)
{
  GimpImageType  l_type;
  static GimpDrawable  *l_new_drawable;
  gint32     l_new_layer_id;
  char      *l_name;
  char      *l_name2;
  gdouble    l_opacity;
  GimpLayerMode l_mode;
  gint       l_visible;
  gint32     image_id;
  gint       stack_position;

  image_id = gimp_item_get_image (src_drawable->drawable_id);
  stack_position = 0;                                  /* TODO:  should be same as src_layer */

  /* copy type, name, opacity and mode from src_drawable */
  l_type     = gimp_drawable_type (src_drawable->drawable_id);
  l_visible  = gimp_item_get_visible (src_drawable->drawable_id);

  l_name2 = gimp_item_get_name (src_drawable->drawable_id);
  l_name = g_strdup_printf ("%s_b", l_name2);
  g_free (l_name2);

  l_mode = gimp_layer_get_mode (src_drawable->drawable_id);
  l_opacity = gimp_layer_get_opacity (src_drawable->drawable_id);  /* full opacity */

  l_new_layer_id = gimp_layer_new (image_id, l_name,
                                   width, height,
                                   l_type,
                                   l_opacity,
                                   l_mode);

  g_free (l_name);
  if (!gimp_drawable_has_alpha (l_new_layer_id))
    {
      /* always add alpha channel */
      gimp_layer_add_alpha (l_new_layer_id);
    }

  l_new_drawable = gimp_drawable_get (l_new_layer_id);
  if (!l_new_drawable)
    {
      g_printerr ("p_add_layer: can't get new_drawable\n");
      return NULL;
    }

  /* add the copied layer to the temp. working image */
  gimp_image_insert_layer (image_id, l_new_layer_id, -1, stack_position);

  /* copy visibility state */
  gimp_item_set_visible (l_new_layer_id, l_visible);

  return l_new_drawable;
}

/* ============================================================================
 * p_bender_calculate_iter_curve
 * ============================================================================
 */

static void
p_bender_calculate_iter_curve (BenderDialog *cd,
                               gint32        xmax,
                               gint32        ymax)
{
   int l_x;
   gint      l_outline;
   BenderDialog *cd_from;
   BenderDialog *cd_to;

   l_outline = cd->outline;

   if ((cd->bval_from == NULL) ||
       (cd->bval_to == NULL) ||
       (cd->bval_curr == NULL))
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
       if(gb_debug)  printf ("p_bender_calculate_iter_curve ITERmode 1\n");

       /* init FROM curves */
       cd_from = g_new (BenderDialog, 1);
       p_cd_from_bval (cd_from, cd->bval_from);
       cd_from->curve_ptr[OUTLINE_UPPER] = g_new (gint32, 1+xmax);
       cd_from->curve_ptr[OUTLINE_LOWER] = g_new (gint32, 1+xmax);

       /* init TO curves */
       cd_to = g_new (BenderDialog, 1);
       p_cd_from_bval (cd_to, cd->bval_to);
       cd_to->curve_ptr[OUTLINE_UPPER] = g_new (gint32, 1+xmax);
       cd_to->curve_ptr[OUTLINE_LOWER] = g_new (gint32, 1+xmax);

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
           p_stretch_curves (cd_from, xmax, ymax);
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
           p_stretch_curves (cd_to, xmax, ymax);
         }

       /* MIX Y-koords of the curves according to current iteration step */
       for (l_x = 0; l_x <= xmax; l_x++)
         {
           p_delta_gint32 (&cd->curve_ptr[OUTLINE_UPPER][l_x],
                           cd_from->curve_ptr[OUTLINE_UPPER][l_x],
                           cd_to->curve_ptr[OUTLINE_UPPER][l_x],
                           cd->bval_curr->total_steps,
                           cd->bval_curr->current_step);

           p_delta_gint32 (&cd->curve_ptr[OUTLINE_LOWER][l_x],
                           cd_from->curve_ptr[OUTLINE_LOWER][l_x],
                           cd_to->curve_ptr[OUTLINE_LOWER][l_x],
                           cd->bval_curr->total_steps,
                           cd->bval_curr->current_step);
         }

       g_free (cd_from->curve_ptr[OUTLINE_UPPER]);
       g_free (cd_from->curve_ptr[OUTLINE_LOWER]);

       g_free (cd_from);
       g_free (cd_to);
     }

   cd->outline = l_outline;
}

/* ============================================================================
 * p_vertical_bend
 * ============================================================================
 */

static void
p_vertical_bend (BenderDialog *cd,
                 t_GDRW       *src_gdrw,
                 t_GDRW       *dst_gdrw)
{
  gint32   l_row, l_col;
  gint32   l_first_row, l_first_col, l_last_row, l_last_col;
  gint32   l_x, l_y;
  gint32   l_x2, l_y2;
  gint32   l_curvy, l_nb_curvy, l_nb2_curvy;
  gint32   l_desty, l_othery;
  gint32   l_miny, l_maxy;
  gint32   l_sign, l_dy, l_diff;
  gint32   l_topshift;
  float    l_progress_step;
  float    l_progress_max;
  float    l_progress;

  t_Last  *last_arr;
  t_Last  *first_arr;
  guchar   color[4];
  guchar   mixcolor[4];
  gint     l_alias_dir;
  guchar   l_mixmask;

  l_topshift = p_upper_curve_extend (cd, src_gdrw->drawable->width,
                                     src_gdrw->drawable->height);
  l_diff = l_curvy = l_nb_curvy = l_nb2_curvy= l_miny = l_maxy = 0;

  /* allocate array of last values (one element foreach x coordinate) */
  last_arr  = g_new (t_Last, src_gdrw->x2);
  first_arr = g_new (t_Last, src_gdrw->x2);

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
  if (cd->show_progress)
    gimp_progress_init ( _("Curve Bend"));

  for (l_row = l_first_row; l_row <= l_last_row; l_row++)
    {
      for (l_col = l_first_col; l_col <= l_last_col; l_col++)
        {
          if (l_col == l_first_col)
            l_x = src_gdrw->x1;
          else
            l_x = l_col * src_gdrw->tile_width;
          if (l_col == l_last_col)
            l_x2 = src_gdrw->x2;
          else
            l_x2 = (l_col +1) * src_gdrw->tile_width;

          if (cd->show_progress)
            gimp_progress_update (l_progress += l_progress_step);

          for( ; l_x < l_x2; l_x++)
            {
              if (l_row == l_first_row)
                l_y = src_gdrw->y1;
              else
                l_y = l_row * src_gdrw->tile_height;
              if(l_row == l_last_row)
                l_y2 = src_gdrw->y2;
              else
                l_y2 = (l_row +1) * src_gdrw->tile_height ;

              for( ; l_y < l_y2; l_y++)
                {
                  /* ---------- copy SRC_PIXEL to curve position ------ */

                  p_get_pixel(src_gdrw, l_x, l_y, color);

                  l_curvy = p_curve_get_dy(cd, l_x,
                                           (gint32)src_gdrw->drawable->width,
                                           (gint32)src_gdrw->drawable->height, (gdouble)l_y);
                  l_desty = l_y + l_topshift + l_curvy;

                  /* ----------- SMOOTHING ------------------ */
                  if (cd->smoothing && (l_x > 0))
                    {
                      l_nb_curvy = p_curve_get_dy(cd, l_x -1,
                                                  (gint32)src_gdrw->drawable->width,
                                                  (gint32)src_gdrw->drawable->height, (gdouble)l_y);
                      if ((l_nb_curvy == l_curvy) && (l_x > 1))
                        {
                          l_nb2_curvy = p_curve_get_dy(cd, l_x -2,
                                                       (gint32)src_gdrw->drawable->width,
                                                       (gint32)src_gdrw->drawable->height, (gdouble)l_y);
                        }
                      else
                        {
                          l_nb2_curvy = l_nb_curvy;
                        }
                      p_put_mix_pixel(dst_gdrw, l_x, l_desty, color, l_nb_curvy, l_nb2_curvy, l_curvy );
                    }
                  else
                    {
                      p_put_pixel(dst_gdrw, l_x, l_desty, color);
                    }

                  /* ----------- render ANTIALIAS ------------------ */

                  if(cd->antialias)
                    {
                      l_othery = l_desty;

                      if(l_y == src_gdrw->y1)             /* Upper outline */
                        {
                          first_arr[l_x].y = l_curvy;
                          memcpy(first_arr[l_x].color, color,
                                 dst_gdrw->drawable->bpp);

                          if (l_x > 0)
                            {
                              memcpy(mixcolor, first_arr[l_x-1].color,
                                     dst_gdrw->drawable->bpp);

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
                      if (l_y == src_gdrw->y2 - 1)      /* Lower outline */
                        {
                          if (l_x > 0)
                            {
                              memcpy(mixcolor, last_arr[l_x-1].color,
                                     dst_gdrw->drawable->bpp);

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

                      if (l_alias_dir != 0)
                        {
                          guchar l_alpha_lo;

                          l_alpha_lo = 20;
                          if (gimp_drawable_has_alpha(src_gdrw->drawable->drawable_id))
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
                                  p_put_pixel(dst_gdrw, l_x -1, l_y + l_topshift  + l_miny + l_dy, mixcolor);
                                }
                              else
                                {
                                  p_put_pixel(dst_gdrw, l_x -1, l_y + l_topshift  + (l_maxy - l_dy), mixcolor);
                                }

                            }
                        }
                    }

                  /* ------------------ FILL HOLES ------------------ */

                  if (l_y == src_gdrw->y1)
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

                      memcpy(mixcolor, color, dst_gdrw->drawable->bpp);
                    }

                  for (l_dy = 1; l_dy <= l_diff; l_dy++)
                    {
                      /* y differs more than 1 pixel from last y in the
                       * destination drawable. So we have to fill the empty
                       * space between using a mixed color
                       */

                      if (cd->smoothing)
                        {
                          /* smoothing is on, so we are using a mixed color */
                          gulong alpha1 = last_arr[l_x].color[3];
                          gulong alpha2 = color[3];
                          gulong alpha;
                          l_mixmask =  255 * ((gdouble)(l_dy) / (gdouble)(l_diff+1));
                          alpha = alpha1 * l_mixmask + alpha2 * (255 - l_mixmask);
                          mixcolor[3] = alpha/255;
                          if (mixcolor[3])
                            {
                              mixcolor[0] = (alpha1 * l_mixmask * last_arr[l_x].color[0]
                                             + alpha2 * (255 - l_mixmask) * color[0])/alpha;
                              mixcolor[1] = (alpha1 * l_mixmask * last_arr[l_x].color[1]
                                             + alpha2 * (255 - l_mixmask) * color[1])/alpha;
                              mixcolor[2] = (alpha1 * l_mixmask * last_arr[l_x].color[2]
                                             + alpha2 * (255 - l_mixmask) * color[2])/alpha;
                              /*mixcolor[2] =  MIX_CHANNEL(last_arr[l_x].color[2], color[2], l_mixmask);*/
                            }
                        }
                      else
                        {
                          /* smoothing is off, so we are using this color or
                             the last color */
                          if (l_dy < l_diff / 2)
                            {
                              memcpy(mixcolor, color,
                                     dst_gdrw->drawable->bpp);
                            }
                          else
                            {
                              memcpy(mixcolor, last_arr[l_x].color,
                                     dst_gdrw->drawable->bpp);
                            }
                        }

                      if (cd->smoothing)
                        {
                          p_put_mix_pixel(dst_gdrw, l_x,
                                          l_desty + (l_dy * l_sign),
                                          mixcolor,
                                          l_nb_curvy, l_nb2_curvy, l_curvy );
                        }
                      else
                        {
                          p_put_pixel(dst_gdrw, l_x,
                                      l_desty + (l_dy * l_sign), mixcolor);
                        }
                    }

                  /* store y and color */
                  last_arr[l_x].y = l_curvy;
                  memcpy(last_arr[l_x].color, color, dst_gdrw->drawable->bpp);
                }
            }
        }
    }
  gimp_progress_update (1.0);

  g_free (last_arr);
  g_free (first_arr);
}

/* ============================================================================
 * p_main_bend
 * ============================================================================
 */

static gint32
p_main_bend (BenderDialog *cd,
             GimpDrawable    *original_drawable,
             gint          work_on_copy)
{
   t_GDRW  l_src_gdrw;
   t_GDRW  l_dst_gdrw;
   GimpDrawable *dst_drawable;
   GimpDrawable *src_drawable;
   gint32    l_dst_height;
   gint32    l_image_id;
   gint32    l_tmp_layer_id;
   gint32    l_interpolation;
   gint      l_offset_x, l_offset_y;
   gint      l_center_x, l_center_y;
   gint32    xmax, ymax;

   l_interpolation = cd->smoothing;
   l_image_id = gimp_item_get_image (original_drawable->drawable_id);
   gimp_drawable_offsets(original_drawable->drawable_id, &l_offset_x, &l_offset_y);

   l_center_x = l_offset_x + (gimp_drawable_width  (original_drawable->drawable_id) / 2 );
   l_center_y = l_offset_y + (gimp_drawable_height (original_drawable->drawable_id) / 2 );

   /* always copy original_drawable to a tmp src_layer */
   l_tmp_layer_id = gimp_layer_copy(original_drawable->drawable_id);
   /* set layer invisible and dummyname and
    * add at top of the image while working
    * (for the case of undo GIMP must know,
    *  that the layer was part of the image)
    */
   gimp_image_insert_layer (l_image_id, l_tmp_layer_id, -1, 0);
   gimp_item_set_visible (l_tmp_layer_id, FALSE);
   gimp_item_set_name (l_tmp_layer_id, "curve_bend_dummylayer");

   if(gb_debug) printf("p_main_bend  l_tmp_layer_id %d\n", (int)l_tmp_layer_id);

   if (cd->rotation != 0.0)
     {
       if(gb_debug) printf("p_main_bend rotate: %f\n", (float)cd->rotation);
       p_gimp_rotate(l_image_id, l_tmp_layer_id, l_interpolation, cd->rotation);
     }
   src_drawable = gimp_drawable_get (l_tmp_layer_id);

   xmax = ymax = src_drawable->width -1;
   cd->curve_ptr[OUTLINE_UPPER] = g_new (gint32, 1+xmax);
   cd->curve_ptr[OUTLINE_LOWER] = g_new (gint32, 1+xmax);

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
       gimp_layer_resize(original_drawable->drawable_id,
                         src_drawable->width,
                         l_dst_height,
                         l_offset_x, l_offset_y);
       if(gb_debug) printf("p_main_bend: DONE layer resize\n");
       if(!gimp_drawable_has_alpha(original_drawable->drawable_id))
         {
           /* always add alpha channel */
           gimp_layer_add_alpha(original_drawable->drawable_id);
         }
       dst_drawable = gimp_drawable_get (original_drawable->drawable_id);
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
       p_gimp_rotate (l_image_id, dst_drawable->drawable_id,
                      l_interpolation, (gdouble)(360.0 - cd->rotation));

       /* TODO: here we should crop dst_drawable to cut off full transparent borderpixels */

   }

   /* set offsets of the resulting new layer
    *(center == center of original_drawable)
    */
   l_offset_x = l_center_x - (gimp_drawable_width  (dst_drawable->drawable_id) / 2 );
   l_offset_y = l_center_y - (gimp_drawable_height  (dst_drawable->drawable_id) / 2 );
   gimp_layer_set_offsets (dst_drawable->drawable_id, l_offset_x, l_offset_y);

   /* delete the temp layer */
   gimp_image_remove_layer (l_image_id, l_tmp_layer_id);

   g_free (cd->curve_ptr[OUTLINE_UPPER]);
   g_free (cd->curve_ptr[OUTLINE_LOWER]);

   if (gb_debug) printf("p_main_bend: DONE bend main\n");

   return dst_drawable->drawable_id;
}
