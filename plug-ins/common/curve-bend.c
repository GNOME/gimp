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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

  gint     curve_type;
  gboolean work_on_copy;
};

typedef struct _Curves Curves;

struct _Curves
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _BenderDialog BenderDialog;

struct _BenderDialog
{
  GtkWidget           *shell;
  GtkWidget           *outline_menu;
  GtkWidget           *pv_widget;
  GtkWidget           *graph;
  GtkWidget           *filechooser;

  GdkCursor           *cursor_busy;

  GimpProcedureConfig *config;

  GimpDrawable        *drawable;
  int                  color;
  int                  outline;
  gint                 preview;

  int                  grab_point;
  int                  last;
  int                  leftmost;
  int                  rightmost;
  int                  curve_type;
  gdouble              points[2][17][2];    /*  0.0 <= points    <= 1.0 */
  guchar               curve[2][256];       /*  0   <= curve     <= 255 */
  gint32              *curve_ptr[2];        /*  0   <= curve_ptr <= src_drawable_width
                                             *  both arrays are allocated dynamic,
                                             *  depending on drawable width
                                             */
  gint32     min2[2];
  gint32     max2[2];
  gint32     zero2[2];

  gboolean   show_progress;

  GimpImage *preview_image;
  GimpLayer *preview_layer1;
  GimpLayer *preview_layer2;

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
  gint          width;
  gint          height;
  GeglBuffer   *buffer;
  const Babl   *format;
  gint          x1;
  gint          y1;
  gint          x2;
  gint          y2;
  gint          index_alpha;   /* 0 == no alpha, 1 == GREYA, 3 == RGBA */
  gint          bpp;
  gint          tile_width;
  gint          tile_height;
} t_GDRW;

typedef struct
{
  gint32 y;
  guchar color[4];
} t_Last;


typedef struct _Bender      Bender;
typedef struct _BenderClass BenderClass;

struct _Bender
{
  GimpPlugIn parent_instance;
};

struct _BenderClass
{
  GimpPlugInClass parent_class;
};


#define BENDER_TYPE  (bender_get_type ())
#define BENDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BENDER_TYPE, Bender))

GType                   bender_get_type         (void) G_GNUC_CONST;

static GList          * bender_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * bender_create_procedure (GimpPlugIn           *plug_in,
                                                 const gchar          *name);

static GimpValueArray * bender_run              (GimpProcedure        *procedure,
                                                 GimpRunMode           run_mode,
                                                 GimpImage            *image,
                                                 GimpDrawable        **drawables,
                                                 GimpProcedureConfig  *config,
                                                 gpointer              run_data);

static BenderDialog *   bender_new_dialog             (GimpProcedure       *procedure,
                                                       GimpProcedureConfig *config,
                                                       GimpDrawable        *drawable);
static void             bender_update                 (BenderDialog *,
                                                       int);
static void             bender_plot_curve             (BenderDialog *,
                                                       int,
                                                       int,
                                                       int,
                                                       int,
                                                       gint32,
                                                       gint32,
                                                       gint);
static void             bender_calculate_curve        (BenderDialog *,
                                                       gint32,
                                                       gint32,
                                                       gint);
static void             bender_global_notify          (GtkWidget *,
                                                       gpointer);
static void             bender_type_callback          (GtkWidget *,
                                                       gpointer);
static void             bender_reset_callback         (GtkWidget *,
                                                       gpointer);
static void             bender_copy_callback          (GtkWidget *,
                                                       gpointer);
static void             bender_copy_inv_callback      (GtkWidget *,
                                                       gpointer);
static void             bender_swap_callback          (GtkWidget *,
                                                       gpointer);
static void             bender_preview_update         (GtkWidget *,
                                                       gpointer);
static void             bender_preview_update_once    (GtkWidget *,
                                                       gpointer);
static void             bender_load_callback          (GtkWidget *,
                                                       BenderDialog *);
static void             bender_save_callback          (GtkWidget *,
                                                       BenderDialog *);
static gint             bender_graph_events           (GtkWidget *,
                                                       GdkEvent *,
                                                       BenderDialog *);
static gint             bender_graph_draw             (GtkWidget *,
                                                       cairo_t *,
                                                       BenderDialog *);
static void             bender_CR_compose             (CRMatrix,
                                                       CRMatrix,
                                                       CRMatrix);
static void             bender_init_min_max           (BenderDialog *,
                                                       gint32);
static BenderDialog   * do_dialog                     (GimpProcedure       *procedure,
                                                       GimpProcedureConfig *config,
                                                       GimpDrawable        *drawable);
static void             p_init_gdrw                   (t_GDRW              *gdrw,
                                                       GimpDrawable        *drawable,
                                                       int                  shadow);
static void             p_end_gdrw                    (t_GDRW              *gdrw);
static GimpLayer      * p_main_bend                   (BenderDialog        *cd,
                                                       GimpDrawable        *original_drawable,
                                                       GimpProcedureConfig *config,
                                                       gint                 work_on_copy);
static GimpImage      * p_create_pv_image             (GimpDrawable        *src_drawable,
                                                       GimpLayer          **layer);
static void             p_render_preview              (BenderDialog        *cd,
                                                       GimpLayer           *layer);
static void             p_get_pixel                   (t_GDRW              *gdrw,
                                                       gint32               x,
                                                       gint32               y,
                                                       guchar              *pixel);
static void             p_put_pixel                   (t_GDRW              *gdrw,
                                                       gint32               x,
                                                       gint32               y,
                                                       guchar              *pixel);
static void             p_put_mix_pixel               (t_GDRW              *gdrw,
                                                       gint32               x,
                                                       gint32               y,
                                                       guchar              *color,
                                                       gint32               nb_curvy,
                                                       gint32               nb2_curvy,
                                                       gint32               curvy);
static void             p_stretch_curves              (BenderDialog        *cd,
                                                       gint32               xmax,
                                                       gint32               ymax);
static void             p_cd_to_bval                  (BenderDialog        *cd,
                                                       BenderValues        *bval);
static void             p_cd_from_bval                (BenderDialog        *cd,
                                                       BenderValues        *bval);
static void             p_store_values                (GimpProcedureConfig *config,
                                                       BenderDialog        *cd);
static void             p_retrieve_values             (GimpProcedureConfig *config,
                                                       BenderDialog        *cd);
static void             p_bender_calculate_iter_curve (BenderDialog        *cd,
                                                       GimpProcedureConfig *config,
                                                       gint32               xmax,
                                                       gint32               ymax);
static int              p_save_pointfile              (BenderDialog        *cd,
                                                       const gchar         *filename);


G_DEFINE_TYPE (Bender, bender, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (BENDER_TYPE)
DEFINE_STD_SET_I18N


static CRMatrix CR_basis =
{
  { -0.5,  1.5, -1.5,  0.5 },
  {  1.0, -2.5,  2.0, -0.5 },
  { -0.5,  0.0,  0.5,  0.0 },
  {  0.0,  1.0,  0.0,  0.0 },
};

static int gb_debug = FALSE;


static void
bender_class_init (BenderClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = bender_query_procedures;
  plug_in_class->create_procedure = bender_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
bender_init (Bender *bender)
{
}

static GList *
bender_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
bender_create_procedure (GimpPlugIn  *plug_in,
                         const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            bender_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, PLUG_IN_IMAGE_TYPES);
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Curve Bend..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Distorts");

      gimp_procedure_set_documentation
        (procedure,
         _("Bend the image using two control curves"),
         _("This plug-in bends the active layer. "
           "If there is a current selection it is copied to "
           "floating selection and the curve_bend distortion "
           "is done on the floating selection. If "
           "work_on_copy parameter is TRUE, the curve_bend "
           "distortion is done on a copy of the active layer "
           "(or floating selection). The upper and lower edges "
           "are bent in shape of 2 spline curves. Both (upper "
           "and lower) curves are determined by up to 17 points "
           "or by 256 Y-Values if curve_type == 1 (freehand "
           "mode). If rotation is not 0, the layer is rotated "
           "before and rotated back after the bend operation. "
           "This enables bending in other directions than "
           "vertical. Bending usually changes the size of "
           "the handled layer. This plug-in sets the offsets "
           "of the handled layer to keep its center at the "
           "same position."),
         name);
      gimp_procedure_set_attribution (procedure,
                                      PLUG_IN_AUTHOR,
                                      PLUG_IN_COPYRIGHT,
                                      PLUG_IN_VERSION);

      gimp_procedure_add_double_argument (procedure, "rotation",
                                          _("Rotat_e"),
                                          _("Direction {angle 0 to 360 degree } "
                                            "of the bend effect"),
                                          0, 360, 0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "smoothing",
                                           _("Smoo_thing"),
                                           _("Smoothing"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "antialias",
                                           _("_Antialiasing"),
                                           _("Antialias"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "work-on-copy",
                                           _("Work on cop_y"),
                                           _("Copy the drawable and bend the copy"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "curve-type",
                                          _("Cur_ve Type"),
                                          _("Whether to use Smooth (17 points) or "
                                            "Freehand (256 points) to draw the curve"),
                                          gimp_choice_new_with_values ("smooth",   SMOOTH, _("Smooth"),   NULL,
                                                                       "freehand", GFREE,  _("Freehand"), NULL,
                                                                       NULL),
                                          "smooth",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "curve-border",
                                          _("Curve for _Border"),
                                          _("Choose the active border line to edit"),
                                          gimp_choice_new_with_values ("upper", OUTLINE_UPPER, _("Upper"),               NULL,
                                                                       "lower", OUTLINE_LOWER, C_("adjective", "Lower"), NULL,
                                                                       NULL),
                                          "upper",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_array_argument (procedure, "upper-point-x",
                                                _("Upper point X"),
                                                _("Array of 17 x point coords "
                                                  "{ 0.0 <= x <= 1.0 or -1 for unused point }"),
                                                G_PARAM_READWRITE);

      gimp_procedure_add_double_array_argument (procedure, "upper-point-y",
                                                _("Upper point Y"),
                                                _("Array of 17 y point coords "
                                                  "{ 0.0 <= y <= 1.0 or -1 for unused point }"),
                                                G_PARAM_READWRITE);

      gimp_procedure_add_double_array_argument (procedure, "lower-point-x",
                                                _("Lower point X"),
                                                _("Array of 17 x point coords "
                                                  "{ 0.0 <= x <= 1.0 or -1 for unused point }"),
                                                G_PARAM_READWRITE);

      gimp_procedure_add_double_array_argument (procedure, "lower-point-y",
                                                _("Lower point Y"),
                                                _("Array of 17 y point coords "
                                                  "{ 0.0 <= y <= 1.0 or -1 for unused point }"),
                                                G_PARAM_READWRITE);

      gimp_procedure_add_bytes_argument (procedure, "upper-val-y",
                                         _("Upper val Y"),
                                         _("Array of 256 y freehand coords "
                                           "{ 0 <= y <= 255 }"),
                                         G_PARAM_READWRITE);

      gimp_procedure_add_bytes_argument (procedure, "lower-val-y",
                                         _("Lower val Y"),
                                         _("Array of 256 y freehand coords "
                                           "{ 0 <= y <= 255 }"),
                                         G_PARAM_READWRITE);

      gimp_procedure_add_layer_return_value (procedure, "bent-layer",
                                             _("Bent layer"),
                                             _("The transformed layer"),
                                             FALSE,
                                             G_PARAM_READWRITE);

      gimp_procedure_add_bytes_aux_argument (procedure, "settings-data",
                                             "Settings data",
                                             "TODO: eventually we must implement proper args for every settings",
                                             GIMP_PARAM_READWRITE);
    }

  return procedure;
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX PDB_STUFF XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

static GimpItem *
p_gimp_rotate (GimpImage    *image,
               GimpDrawable *drawable,
               gint32        interpolation,
               gdouble       angle_deg)
{
  gdouble   angle_rad;
  GimpItem *rc;

  angle_rad = (angle_deg * G_PI) / 180.0;

  gimp_context_push ();
  if (! interpolation)
    gimp_context_set_interpolation (GIMP_INTERPOLATION_NONE);
  gimp_context_set_transform_resize (GIMP_TRANSFORM_RESIZE_ADJUST);
  rc = gimp_item_transform_rotate (GIMP_ITEM (drawable),
                                   angle_rad,
                                   TRUE /*auto_center*/,
                                   -1.0 /*center_x*/,
                                   -1.0 /*center_y*/);
  gimp_context_pop ();

  if (! rc)
    g_printerr ("Error: gimp_drawable_transform_rotate_default call failed\n");

  return rc;
}

/* ============================================================================
 * p_if_selection_float_it
 * ============================================================================
 */

static GimpLayer *
p_if_selection_float_it (GimpImage *image,
                         GimpLayer *layer)
{
  if (! gimp_layer_is_floating_sel (layer))
    {
      GimpSelection *sel_channel;
      gint32         x1, x2, y1, y2;
      gint32         non_empty;

      /* check and see if we have a selection mask */
      sel_channel  = gimp_image_get_selection (image);

      gimp_selection_bounds (image, &non_empty, &x1, &y1, &x2, &y2);

      if (non_empty && sel_channel)
        {
          const GimpDrawable *drawables[2] = { (GimpDrawable *) layer, NULL };

          /* selection is TRUE, make a layer (floating selection) from
             the selection  */
          if (gimp_edit_copy (drawables))
            {
              GimpDrawable **layers;

              layers = gimp_edit_paste (GIMP_DRAWABLE (layer), FALSE);
              /* Since we explicitly copied a single layer, there should
               * be no more than a single layer in the paste as well.
               */
              layer = (GimpLayer *) (layers && layers[0] ? layers[0] : NULL);

              g_free (layers);
            }
          else
            {
              return NULL;
            }
        }
    }

  return layer;
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX END_PDB_STUFF XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

static GimpValueArray *
bender_run (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            GimpDrawable        **drawables,
            GimpProcedureConfig  *config,
            gpointer              run_data)
{
  const gchar    *env;
  GimpValueArray *return_vals;
  BenderDialog   *cd              = NULL;
  GimpDrawable   *active_drawable = NULL;
  GimpDrawable   *drawable;
  GimpLayer      *layer           = NULL;
  GimpLayer      *bent_layer      = NULL;
  GError         *error           = NULL;

  gegl_init (NULL, NULL);

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  env = g_getenv ("BEND_DEBUG");
  if (env != NULL)
    if ((*env != 'n') && (*env != 'N')) gb_debug = 1;

  if (! GIMP_IS_LAYER (drawable))
    {
      g_set_error (&error, 0, 0, "%s",
                   _("Can operate on layers only "
                     "(but was called on channel or mask)."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

  layer = GIMP_LAYER (drawable);

  /* check for layermask */
  if (gimp_layer_get_mask (layer))
    {
      g_set_error (&error, 0, 0, "%s",
                   _("Cannot operate on layers with masks."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

  /* if there is a selection, make it the floating selection layer */
  active_drawable = GIMP_DRAWABLE (p_if_selection_float_it (image, layer));
  if (! active_drawable)
    {
      /* could not float the selection because selection rectangle
       * is completely empty return GIMP_PDB_EXECUTION_ERROR
       */
      g_set_error (&error, 0, 0, "%s",
                   _("Cannot operate on empty selections."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Get information from the dialog */
      cd = do_dialog (procedure, config, active_drawable);
      cd->show_progress = TRUE;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      cd = g_new (BenderDialog, 1);

      cd->run           = TRUE;
      cd->show_progress = TRUE;
      cd->drawable      = active_drawable;
      cd->config        = config;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      cd = g_new (BenderDialog, 1);

      cd->run           = TRUE;
      cd->show_progress = TRUE;
      cd->drawable      = active_drawable;
      cd->config        = config;

      p_retrieve_values (config, cd);  /* Possibly retrieve data from a previous run */
      break;
    }

  if (! cd)
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

  if (cd->run)
    {
      gboolean work_on_copy;

      g_object_get (config,
                    "work-on-copy", &work_on_copy,
                    NULL);

      gimp_image_undo_group_start (image);

      bent_layer = p_main_bend (cd, cd->drawable,
                                config, work_on_copy);

      gimp_image_undo_group_end (image);

      /* Store variable states for next run */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        p_store_values (config, cd);
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CANCEL,
                                               NULL);
    }

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_LAYER (return_vals, 1, bent_layer);

  return return_vals;
}

static int
p_save_pointfile (BenderDialog *cd,
                  const gchar  *filename)
{
  FILE *fp;
  gint  j;

  fp = g_fopen(filename, "w+b");
  if (! fp)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  fprintf (fp, "%s\n", KEY_POINTFILE);
  fprintf (fp, "VERSION 1.0\n\n");

  fprintf (fp, "# points for upper and lower smooth curve (0.0 <= pt <= 1.0)\n");
  fprintf (fp, "# there are upto 17 points where unused points are set to -1\n");
  fprintf (fp, "#       UPPERX     UPPERY      LOWERX    LOWERY\n");
  fprintf (fp, "\n");

  for(j = 0; j < 17; j++)
  {
      fprintf (fp, "%s %+.6f  %+.6f   %+.6f  %+.6f\n", KEY_POINTS,
               (float)cd->points[OUTLINE_UPPER][j][0],
               (float)cd->points[OUTLINE_UPPER][j][1],
               (float)cd->points[OUTLINE_LOWER][j][0],
               (float)cd->points[OUTLINE_LOWER][j][1] );
  }

  fprintf (fp, "\n");
  fprintf (fp, "# y values for upper/lower freehand curve (0 <= y <= 255) \n");
  fprintf (fp, "# there must be exactly 256 y values \n");
  fprintf (fp, "#     UPPER_Y  LOWER_Y\n");
  fprintf (fp, "\n");

  for (j = 0; j < 256; j++)
  {
    fprintf (fp, "%s %3d  %3d\n", KEY_VAL_Y,
             (int)cd->curve[OUTLINE_UPPER][j],
             (int)cd->curve[OUTLINE_LOWER][j]);
  }

  fclose (fp);

  return 0;
}

static int
p_load_pointfile (BenderDialog *cd,
                  const gchar  *filename)
{
  gint  pi, ci, n, len;
  FILE *fp;
  char  buff[2000];
  float fux, fuy, flx, fly;
  gint  iuy, ily ;

  fp = g_fopen(filename, "rb");
  if (! fp)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  pi = 0;
  ci = 0;

  if (! fgets (buff, 2000 - 1, fp))
    {
      g_message (_("Error while reading '%s': %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      fclose (fp);
      return -1;
    }

  if (strncmp (buff, KEY_POINTFILE, strlen(KEY_POINTFILE)) == 0)
    {
      while (NULL != fgets (buff, 2000-1, fp))
        {
          len = strlen(KEY_POINTS);

          if (strncmp (buff, KEY_POINTS, len) == 0)
            {
              n = sscanf (&buff[len],
                          "%f %f %f %f", &fux, &fuy, &flx, &fly);

              if ((n == 4) && (pi < 17))
                {
                  cd->points[OUTLINE_UPPER][pi][0] = fux;
                  cd->points[OUTLINE_UPPER][pi][1] = fuy;
                  cd->points[OUTLINE_LOWER][pi][0] = flx;
                  cd->points[OUTLINE_LOWER][pi][1] = fly;
                  pi++;
                }
              else
                {
                  g_printf("warning: BAD points[%d] in file %s are ignored\n", pi, filename);
                }
            }

          len = strlen (KEY_VAL_Y);

          if (strncmp (buff, KEY_VAL_Y, len) == 0)
            {
              n = sscanf (&buff[len], "%d %d", &iuy, &ily);

              if ((n == 2) && (ci < 256))
                {
                  cd->curve[OUTLINE_UPPER][ci] = iuy;
                  cd->curve[OUTLINE_LOWER][ci] = ily;
                  ci++;
                }
              else
                {
                  g_printf("warning: BAD y_vals[%d] in file %s are ignored\n", ci, filename);
                }
            }
        }
    }

  fclose (fp);

  return 0;
}

static void
p_cd_to_bval (BenderDialog *cd,
              BenderValues *bval)
{
  gint i, j;

  for (i = 0; i < 2; i++)
    {
      for (j = 0; j < 256; j++)
        {
          bval->curve[i][j] = cd->curve[i][j];
        }

      for (j = 0; j < 17; j++)
        {
          bval->points[i][j][0] = cd->points[i][j][0];  /* x */
          bval->points[i][j][1] = cd->points[i][j][1];  /* y */
        }
    }
}

static void
p_cd_from_bval (BenderDialog *cd,
                BenderValues *bval)
{
  gint i, j;

  for (i = 0; i < 2; i++)
    {
      for (j = 0; j < 256; j++)
        {
          cd->curve[i][j] = bval->curve[i][j];
        }

      for (j = 0; j < 17; j++)
        {
          cd->points[i][j][0] = bval->points[i][j][0];  /* x */
          cd->points[i][j][1] = bval->points[i][j][1];  /* y */
        }
    }
}

static void
p_store_values (GimpProcedureConfig *config,
                BenderDialog        *cd)
{
  BenderValues  bval;
  GBytes       *settings_bytes;

  p_cd_to_bval (cd, &bval);

  settings_bytes = g_bytes_new (&bval, sizeof (BenderValues));
  g_object_set (config, "settings-data", settings_bytes, NULL);
  g_bytes_unref (settings_bytes);
}

static void
p_retrieve_values (GimpProcedureConfig *config,
                   BenderDialog        *cd)
{
  GBytes *settings_bytes;

  g_object_get (config, "settings-data", &settings_bytes, NULL);
  if (settings_bytes != NULL && g_bytes_get_size (settings_bytes) == sizeof (BenderValues))
    {
      BenderValues bval;

      bval = *((BenderValues *) g_bytes_get_data (settings_bytes, NULL));
      p_cd_from_bval(cd, &bval);
    }
  g_bytes_unref (settings_bytes);
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/*  curves machinery  */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

static BenderDialog *
do_dialog (GimpProcedure       *procedure,
           GimpProcedureConfig *config,
           GimpDrawable        *drawable)
{
  BenderDialog *cd;

  /* Init GTK  */
  gimp_ui_init (PLUG_IN_BINARY);

  /*  The curve_bend dialog  */
  cd = bender_new_dialog (procedure, config, drawable);

  /* create temporary image (with a small copy of drawable) for the preview */
  cd->preview_image = p_create_pv_image (drawable,
                                         &cd->preview_layer1);
  cd->preview_layer2 = NULL;

  if (! gtk_widget_get_visible (cd->shell))
    gtk_widget_show (cd->shell);

  bender_update (cd, UP_GRAPH | UP_DRAW | UP_PREVIEW_EXPOSE);

  cd->run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (cd->shell));
  gtk_widget_destroy (cd->shell);

  gimp_image_delete (cd->preview_image);
  cd->preview_image  = NULL;
  cd->preview_layer1 = NULL;
  cd->preview_layer2 = NULL;

  return cd;
}

/**************************/
/*  Select Curves dialog  */
/**************************/

static BenderDialog *
bender_new_dialog (GimpProcedure       *procedure,
                   GimpProcedureConfig *config,
                   GimpDrawable        *drawable)
{
  BenderDialog *cd;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *vbox2;
  GtkWidget    *frame;
  GtkWidget    *toggle;
  GtkWidget    *button;
  GdkDisplay   *display;
  gint          i, j;

  cd = g_new (BenderDialog, 1);

  cd->config = config;
  cd->preview = FALSE;
  cd->filechooser = NULL;
  cd->show_progress = FALSE;

  cd->drawable = drawable;

  cd->color = gimp_drawable_is_rgb (drawable);

  cd->run = FALSE;

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

  p_retrieve_values (config, cd);       /* Possibly retrieve data from a previous run */

  /*  The shell and main vbox  */
  cd->shell = gimp_procedure_dialog_new (procedure,
                                         GIMP_PROCEDURE_CONFIG (config),
                                         _("Curve Bend"));

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (cd->shell),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gimp_window_set_transient (GTK_WINDOW (cd->shell));

  /*  busy cursor  */
  display = gtk_widget_get_display (cd->shell);
  cd->cursor_busy = gdk_cursor_new_for_display (display, GDK_WATCH);

  /* Left side column */
  /* Config-connected widgets first */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (cd->shell),
                                   "option-label", _("Options"), FALSE,
                                   FALSE);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (cd->shell),
                                  "options-hbox", "rotation", "smoothing",
                                  "antialias", "work-on-copy", NULL);
  toggle = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (cd->shell),
                                             "rotation", G_TYPE_NONE);
  g_signal_connect (toggle, "value-changed",
                    G_CALLBACK (bender_global_notify),
                    cd);
  toggle = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (cd->shell),
                                             "smoothing", G_TYPE_NONE);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (bender_global_notify),
                    cd);
  toggle = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (cd->shell),
                                             "antialias", G_TYPE_NONE);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (bender_global_notify),
                    cd);

  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (cd->shell),
                                    "option-frame", "option-label", FALSE,
                                    "options-hbox");

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (cd->shell),
                                         "curve-left-col", "option-frame",
                                         NULL);

  /* GUI-only widgets */
  /* Preview area, top of column */
  frame = gimp_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);
  gtk_box_reorder_child (GTK_BOX (vbox), frame, 0);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_set_visible (vbox2, TRUE);

  /*  The range drawing area  */
  frame = gtk_frame_new (NULL);
  gtk_widget_set_halign (frame, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (frame, GTK_ALIGN_CENTER);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  cd->pv_widget = gimp_preview_area_new ();
  gtk_widget_set_size_request (cd->pv_widget,
                               PREVIEW_SIZE_X, PREVIEW_SIZE_Y);
  gtk_container_add (GTK_CONTAINER (frame), cd->pv_widget);
  gtk_widget_set_visible (cd->pv_widget, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_end (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /*  The preview button  */
  button = gtk_button_new_with_mnemonic (_("_Preview Once"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_preview_update_once),
                    cd);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_mnemonic (_("Automatic pre_view"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cd->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_set_visible (toggle, TRUE);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (bender_preview_update),
                    cd);

  /* The curves graph  */
  /* Config-connected widgets first */
  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (cd->shell),
                                    "curve-type", GIMP_TYPE_INT_RADIO_FRAME);
  g_signal_connect (config, "notify::curve-type",
                    G_CALLBACK (bender_type_callback), cd);

  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (cd->shell),
                                    "curve-border",
                                    GIMP_TYPE_INT_RADIO_FRAME);

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (cd->shell),
                                         "curve-hbox", "curve-border",
                                         "curve-type", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (cd->shell),
                                         "curve-right-col", "curve-hbox",
                                         NULL);

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (cd->shell),
                                   "modify-label", _("Modify Curves"), FALSE,
                                   FALSE);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (cd->shell),
                                    "curve-right-frame", "modify-label", FALSE,
                                    "curve-right-col");

  /* GUI-only widgets */
  cd->graph = gtk_drawing_area_new ();
  gtk_widget_set_halign (cd->graph, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (cd->graph, GTK_ALIGN_CENTER);
  gtk_widget_set_size_request (cd->graph,
                               GRAPH_WIDTH + RADIUS * 2,
                               GRAPH_HEIGHT + RADIUS * 2);
  gtk_widget_set_events (cd->graph, GRAPH_MASK);
  gtk_box_pack_start (GTK_BOX (vbox), cd->graph, FALSE, FALSE, 0);
  gtk_widget_set_visible (cd->graph, TRUE);

  g_signal_connect (cd->graph, "event",
                    G_CALLBACK (bender_graph_events),
                    cd);
  g_signal_connect (cd->graph, "draw",
                    G_CALLBACK (bender_graph_draw),
                    cd);

  gtk_box_reorder_child (GTK_BOX (vbox), cd->graph, 0);

  /*  hbox for curve options  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /*  The Copy button  */
  button = gtk_button_new_with_mnemonic (_("_Copy"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  gimp_help_set_help_data (button,
                           _("Copy the active curve to the other border"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_copy_callback),
                    cd);

  /*  The CopyInv button  */
  button = gtk_button_new_with_mnemonic (_("_Mirror"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  gimp_help_set_help_data (button,
                           _("Mirror the active curve to the other border"),
                           NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_copy_inv_callback),
                    cd);

  /*  The Swap button  */
  button = gtk_button_new_with_mnemonic (_("S_wap"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  gimp_help_set_help_data (button,
                           _("Swap the two curves"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_swap_callback),
                    cd);

  /*  The Reset button  */
  button = gtk_button_new_with_mnemonic (_("_Reset"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  gimp_help_set_help_data (button,
                           _("Reset the active curve"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_reset_callback),
                    cd);

  /*  hbox for curve load and save  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /*  The Load button  */
  button = gtk_button_new_with_mnemonic (_("_Open"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  gimp_help_set_help_data (button,
                           _("Load the curves from a file"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_load_callback),
                    cd);

  /*  The Save button  */
  button = gtk_button_new_with_mnemonic (_("_Save"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  gimp_help_set_help_data (button,
                           _("Save the curves to a file"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (bender_save_callback),
                    cd);

  /* Combine the two columns */
  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (cd->shell),
                                         "curve-columns", "curve-left-col",
                                         "curve-right-frame", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (cd->shell),
                              "curve-columns", NULL);

  return cd;
}

static void
bender_update (BenderDialog *cd,
               int           update)
{
  if (update & UP_PREVIEW)
    {
      gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (cd->shell)),
                             cd->cursor_busy);
      gdk_display_flush (gtk_widget_get_display (cd->shell));

      if (cd->preview_layer2)
         gimp_image_remove_layer (cd->preview_image, cd->preview_layer2);

      cd->preview_layer2 = p_main_bend (cd, GIMP_DRAWABLE (cd->preview_layer1),
                                        cd->config, TRUE /* work_on_copy*/ );
      p_render_preview (cd, cd->preview_layer2);

      if (update & UP_DRAW)
        gtk_widget_queue_draw (cd->pv_widget);

      gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (cd->shell)),
                             NULL);
    }
  if (update & UP_PREVIEW_EXPOSE)
    {
      /* on expose just redraw cd->preview_layer2
       * that holds the bent version of the preview (if there is one)
       */
      if (! cd->preview_layer2)
        cd->preview_layer2 = p_main_bend (cd, GIMP_DRAWABLE (cd->preview_layer1),
                                          cd->config, TRUE /* work_on_copy*/ );
      p_render_preview (cd, cd->preview_layer2);

      if (update & UP_DRAW)
        gtk_widget_queue_draw (cd->pv_widget);
    }

  if ((update & UP_GRAPH) && (update & UP_DRAW))
    gtk_widget_queue_draw (cd->graph);
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
  double   x, dx, dx2, dx3;
  double   y, dy, dy2, dy3;
  double   d, d2, d3;
  int      lastx, lasty;
  gint32   newx, newy;
  gint32   ntimes;
  gint32   i;
  gint     outline;

  outline = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                 "curve-border");

  /* construct the geometry matrix from the segment */
  for (i = 0; i < 4; i++)
    {
      geometry[i][2] = 0;
      geometry[i][3] = 0;
    }

  geometry[0][0] = (cd->points[outline][p1][0] * xmax);
  geometry[1][0] = (cd->points[outline][p2][0] * xmax);
  geometry[2][0] = (cd->points[outline][p3][0] * xmax);
  geometry[3][0] = (cd->points[outline][p4][0] * xmax);

  geometry[0][1] = (cd->points[outline][p1][1] * ymax);
  geometry[1][1] = (cd->points[outline][p2][1] * ymax);
  geometry[2][1] = (cd->points[outline][p3][1] * ymax);
  geometry[3][1] = (cd->points[outline][p4][1] * ymax);

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
      cd->curve[outline][lastx] = lasty;
    }
  else
    {
      cd->curve_ptr[outline][lastx] = lasty;
      if(gb_debug) g_printf("bender_plot_curve xmax:%d ymax:%d\n",
                            (gint) xmax, (gint) ymax);
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
          if (fix255)
            {
              /* use fixed array size (for the curve graph) */
              cd->curve[outline][newx] = newy;
            }
          else
            {
              /* use dynamic allocated curve_ptr (for the real curve) */
              cd->curve_ptr[outline][newx] = newy;

              if (gb_debug) g_printf("outline: %d  cX: %d cY: %d\n",
                                     (gint) outline, (int) newx, (int) newy);
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
  gint i;
  gint points[17];
  gint num_pts;
  gint p1, p2, p3, p4;
  gint xmid;
  gint yfirst, ylast;
  gint curve_type;
  gint outline;

  curve_type = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                    "curve-type");
  outline = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                 "curve-border");

  switch (curve_type)
    {
    case GFREE:
      break;

    case SMOOTH:
      /*  cycle through the curves  */
      num_pts = 0;
      for (i = 0; i < 17; i++)
        if (cd->points[outline][i][0] != -1)
          points[num_pts++] = i;

      xmid = xmax / 2;
      /*  Initialize boundary curve points */
      if (num_pts != 0)
        {
          if (fix255)
            {
              for (i = 0; i < (cd->points[outline][points[0]][0] * 255); i++)
                cd->curve[outline][i] =
                  (cd->points[outline][points[0]][1] * 255);

              for (i = (cd->points[outline][points[num_pts - 1]][0] * 255); i < 256; i++)
                cd->curve[outline][i] =
                  (cd->points[outline][points[num_pts - 1]][1] * 255);
            }
          else
            {
              yfirst  = cd->points[outline][points[0]][1] * ymax;
              ylast   = cd->points[outline][points[num_pts - 1]][1] * ymax;

              for (i = 0; i < xmid; i++)
                {
                  cd->curve_ptr[outline][i] = yfirst;
                }

              for (i = xmid; i <= xmax; i++)
                {
                  cd->curve_ptr[outline][i] = ylast;
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
bender_global_notify (GtkWidget *widget,
                      gpointer   data)
{
  BenderDialog *cd;

  cd = (BenderDialog *) data;

  if (! cd)
    return;

  if (cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_type_callback (GtkWidget *widget,
                      gpointer   data)
{
  BenderDialog *cd;
  gint          curve_type;
  gint          outline;

  cd = g_object_get_data (G_OBJECT (widget), "cd");
  if (! cd)
    return;

  curve_type = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                    "curve-type");
  outline = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                 "curve-border");

  if (curve_type == SMOOTH)
    {
      gint i;

      /*  pick representative points from the curve and make them control points  */
      for (i = 0; i <= 8; i++)
        {
          gint index = CLAMP ((i * 32), 0, 255);
          cd->points[outline][i * 2][0] = (gdouble)index / 255.0;
          cd->points[outline][i * 2][1] = (gdouble)cd->curve[outline][index] / 255.0;
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
  gint          outline;

  outline = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                 "curve-border");

  /*  Initialize the values  */
  for (i = 0; i < 256; i++)
    cd->curve[outline][i] = MIDDLE;

  cd->grab_point = -1;
  for (i = 0; i < 17; i++)
    {
      cd->points[outline][i][0] = -1;
      cd->points[outline][i][1] = -1;
    }
  cd->points[outline][0][0] = 0.0;       /* x */
  cd->points[outline][0][1] = 0.5;       /* y */
  cd->points[outline][16][0] = 1.0;      /* x */
  cd->points[outline][16][1] = 0.5;      /* y */

  bender_update (cd, UP_GRAPH | UP_DRAW);
  if (cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_copy_callback (GtkWidget *widget,
                      gpointer   client_data)
{
  BenderDialog *cd = (BenderDialog *) client_data;
  gint          i;
  gint          other;
  gint          outline;

  outline = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                 "curve-border");

  other = (outline) ? 0 : 1;

  for (i = 0; i < 17; i++)
    {
      cd->points[other][i][0] = cd->points[outline][i][0];
      cd->points[other][i][1] = cd->points[outline][i][1];
    }

  for (i= 0; i < 256; i++)
    {
      cd->curve[other][i] = cd->curve[outline][i];
    }

  bender_update (cd, UP_GRAPH | UP_DRAW);
  if (cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_copy_inv_callback (GtkWidget *widget,
                          gpointer   client_data)
{
  BenderDialog *cd = (BenderDialog *) client_data;
  gint          i;
  gint          other;
  gint          outline;

  outline = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                 "curve-border");

  other = (outline) ? 0 : 1;

  for (i = 0; i < 17; i++)
    {
      cd->points[other][i][0] = cd->points[outline][i][0];        /* x */
      cd->points[other][i][1] = 1.0 - cd->points[outline][i][1];  /* y */
    }

  for (i= 0; i < 256; i++)
    {
      cd->curve[other][i] = 255 - cd->curve[outline][i];
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
  BenderDialog *cd = (BenderDialog *) client_data;
  gint          i;
  gint          other;
  gint          outline;
  gdouble       hd;
  guchar        hu;

  outline = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                 "curve-border");

  other = (outline) ? 0 : 1;

  for (i = 0; i < 17; i++)
    {
      SWAP_VALUE(cd->points[other][i][0], cd->points[outline][i][0], hd);  /* x */
      SWAP_VALUE(cd->points[other][i][1], cd->points[outline][i][1], hd);  /* y */
    }

  for (i= 0; i < 256; i++)
    {
      SWAP_VALUE(cd->curve[other][i], cd->curve[outline][i], hu);
    }

  bender_update (cd, UP_GRAPH | UP_DRAW);
  if (cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_preview_update (GtkWidget *widget,
                       gpointer   data)
{
  BenderDialog *cd = (BenderDialog*) data;

  if (! cd)
    return;

  cd->preview = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if(cd->preview)
    bender_update (cd, UP_PREVIEW | UP_DRAW);
}

static void
bender_preview_update_once (GtkWidget *widget,
                            gpointer   data)
{
  BenderDialog *cd = (BenderDialog*) data;

  if (! cd)
    return;

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

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (cd->filechooser),
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

static gboolean
bender_graph_events (GtkWidget    *widget,
                     GdkEvent     *event,
                     BenderDialog *cd)
{
  static GdkCursorType cursor_type = GDK_TOP_LEFT_ARROW;
  GdkCursorType new_type;
  GdkEventMotion *mevent;
  gint i;
  gint tx, ty;
  gint x, y;
  gint closest_point;
  gint distance;
  gint x1, x2, y1, y2;
  gint curve_type;
  gint outline;

  curve_type = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                    "curve-type");
  outline = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                 "curve-border");

  new_type      = GDK_X_CURSOR;
  closest_point = 0;

  if (! gdk_event_get_device (event))
    return FALSE;

  /*  get the pointer position  */
  gdk_window_get_device_position (gtk_widget_get_window (cd->graph),
                                  gdk_event_get_device (event),
                                  &tx, &ty, NULL);
  x = CLAMP ((tx - RADIUS), 0, 255);
  y = CLAMP ((ty - RADIUS), 0, 255);

  distance = G_MAXINT;
  for (i = 0; i < 17; i++)
    {
      if (cd->points[outline][i][0] != -1)
        if (abs ((int) (x - (cd->points[outline][i][0] * 255.0))) < distance)
          {
            distance = abs ((int) (x - (cd->points[outline][i][0] * 255.0)));
            closest_point = i;
          }
    }
  if (distance > MIN_DISTANCE)
    closest_point = (x + 8) / 16;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      new_type = GDK_TCROSS;

      switch (curve_type)
        {
        case SMOOTH:
          /*  determine the leftmost and rightmost points  */
          cd->leftmost = -1;
          for (i = closest_point - 1; i >= 0; i--)
            if (cd->points[outline][i][0] != -1)
              {
                cd->leftmost = (cd->points[outline][i][0] * 255.0);
                break;
              }
          cd->rightmost = 256;
          for (i = closest_point + 1; i < 17; i++)
            if (cd->points[outline][i][0] != -1)
              {
                cd->rightmost = (cd->points[outline][i][0] * 255.0);
                break;
              }

          cd->grab_point = closest_point;
          cd->points[outline][cd->grab_point][0] = (gdouble)x / 255.0;
          cd->points[outline][cd->grab_point][1] = (gdouble)(255 - y) / 255.0;

          bender_calculate_curve (cd, 255, 255, TRUE);
          break;

        case GFREE:
          cd->curve[outline][x] = 255 - y;
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

      switch (curve_type)
        {
        case SMOOTH:
          /*  If no point is grabbed...  */
          if (cd->grab_point == -1)
            {
              if (cd->points[outline][closest_point][0] != -1)
                new_type = GDK_FLEUR;
              else
                new_type = GDK_TCROSS;
            }
          /*  Else, drag the grabbed point  */
          else
            {
              new_type = GDK_TCROSS;

              cd->points[outline][cd->grab_point][0] = -1;

              if (x > cd->leftmost && x < cd->rightmost)
                {
                  closest_point = (x + 8) / 16;
                  if (cd->points[outline][closest_point][0] == -1)
                    cd->grab_point = closest_point;
                  cd->points[outline][cd->grab_point][0] = (gdouble)x / 255.0;
                  cd->points[outline][cd->grab_point][1] = (gdouble)(255 - y) / 255.0;
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
                  cd->curve[outline][i] = 255 - (y1 + ((y2 - y1) * (i - x1)) / (x2 - x1));
              else
                cd->curve[outline][x] = 255 - y;

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

static gboolean
bender_graph_draw (GtkWidget    *widget,
                   cairo_t      *cr,
                   BenderDialog *cd)
{
  GdkRGBA black = { 0.0, 0.0, 0.0, 1.0 };
  GdkRGBA white = { 1.0, 1.0, 1.0, 1.0 };
  GdkRGBA gray  = { 0.5, 0.5, 0.5, 1.0 };
  gint    i;
  gint    other;
  gint    curve_type;
  gint    outline;

  curve_type = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                    "curve-type");
  outline = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                 "curve-border");

  cairo_set_line_width (cr, 1.0);
  cairo_translate (cr, 0.5, 0.5);

  /*  Clear the background  */
  gdk_cairo_set_source_rgba (cr, &white);
  cairo_paint (cr);

  /*  Draw the grid lines  */
  for (i = 0; i < 5; i++)
    {
      cairo_move_to (cr, RADIUS, i * (GRAPH_HEIGHT / 4) + RADIUS);
      cairo_line_to (cr, GRAPH_WIDTH + RADIUS, i * (GRAPH_HEIGHT / 4) + RADIUS);

      cairo_move_to (cr, i * (GRAPH_WIDTH / 4) + RADIUS, RADIUS);
      cairo_line_to (cr, i * (GRAPH_WIDTH / 4) + RADIUS, GRAPH_HEIGHT + RADIUS);
    }

  gdk_cairo_set_source_rgba (cr, &gray);
  cairo_stroke (cr);

  /*  Draw the other curve  */
  other = (outline == 0) ? 1 : 0;

  cairo_move_to (cr, RADIUS, 255 - cd->curve[other][0] + RADIUS);

  for (i = 1; i < 256; i++)
    {
      cairo_line_to (cr, i + RADIUS, 255 - cd->curve[other][i] + RADIUS);
    }

  gdk_cairo_set_source_rgba (cr, &gray);
  cairo_stroke (cr);

  /*  Draw the active curve  */
  cairo_move_to (cr, RADIUS, 255 - cd->curve[outline][0] + RADIUS);

  for (i = 1; i < 256; i++)
    {
      cairo_line_to (cr, i + RADIUS, 255 - cd->curve[outline][i] + RADIUS);
    }

  /*  Draw the points  */
  if (curve_type == SMOOTH)
    {
      for (i = 0; i < 17; i++)
        {
          if (cd->points[outline][i][0] != -1)
            {
              cairo_new_sub_path (cr);
              cairo_arc (cr,
                         (cd->points[outline][i][0] * 255.0) + RADIUS,
                         255 - (cd->points[outline][i][1] * 255.0) + RADIUS,
                         RADIUS,
                         0, 2 * G_PI);
            }
        }
    }

  gdk_cairo_set_source_rgba (cr, &black);
  cairo_stroke (cr);

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
                  GimpLayer    *layer)
{
   guchar  pixel[4];
   guchar *buf, *ptr;
   gint    x, y;
   gint    ofx, ofy;
   gint    width, height;
   t_GDRW  l_gdrw;
   t_GDRW *gdrw;

   width  = gimp_drawable_get_width  (GIMP_DRAWABLE (layer));
   height = gimp_drawable_get_height (GIMP_DRAWABLE (layer));

   ptr = buf = g_new (guchar, PREVIEW_BPP * PREVIEW_SIZE_X * PREVIEW_SIZE_Y);
   gdrw = &l_gdrw;
   p_init_gdrw (gdrw, GIMP_DRAWABLE (layer), FALSE);

  /* offsets to set bend layer to preview center */
  ofx = (width / 2) - (PREVIEW_SIZE_X / 2);
  ofy = (height / 2) - (PREVIEW_SIZE_Y / 2);

  /* render preview */
  for (y = 0; y < PREVIEW_SIZE_Y; y++)
    {
      for (x = 0; x < PREVIEW_SIZE_X; x++)
        {
          p_get_pixel (gdrw, x + ofx, y + ofy, &pixel[0]);

          if (cd->color)
            {
              ptr[0] = pixel[0];
              ptr[1] = pixel[1];
              ptr[2] = pixel[2];
            }
          else
            {
              ptr[0] = pixel[0];
              ptr[1] = pixel[0];
              ptr[2] = pixel[0];
            }

          ptr[3] = pixel[gdrw->index_alpha];

          ptr += PREVIEW_BPP;
        }
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (cd->pv_widget),
                          0, 0, PREVIEW_SIZE_X, PREVIEW_SIZE_Y,
                          GIMP_RGBA_IMAGE,
                          buf,
                          PREVIEW_BPP * PREVIEW_SIZE_X);
  g_free (buf);

  p_end_gdrw(gdrw);
}

/* ===================================================== */
/* curve_bend worker procedures                          */
/* ===================================================== */

static void
p_stretch_curves (BenderDialog *cd,
                  gint32        xmax,
                  gint32        ymax)
{
  gint32   x1, x2;
  gdouble  ya, yb;
  gdouble  rest;
  int      outline;

  for (outline = 0; outline < 2; outline++)
    {
      for(x1 = 0; x1 <= xmax; x1++)
        {
          x2 = (x1 * 255) / xmax;

          if ((xmax <= 255) && (x2 < 255))
            {
              cd->curve_ptr[outline][x1] =
                ROUND ((cd->curve[outline][x2] * ymax) / 255);
            }
          else
            {
              /* interpolate */
              rest = (((gdouble)x1 * 255.0) / (gdouble)xmax) - x2;
              ya = cd->curve[outline][x2];        /* y of this point */
              yb = cd->curve[outline][x2 +1];     /* y of next point */

              cd->curve_ptr[outline][x1] =
                ROUND (((ya + ((yb -ya) * rest)) * ymax) / 255);
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

  /* for UPPER outline : y-zero line is assumed at the min leftmost or
   * rightmost point
   */
  cd->zero2[OUTLINE_UPPER] = MIN (cd->curve_ptr[OUTLINE_UPPER][0],
                                  cd->curve_ptr[OUTLINE_UPPER][xmax]);

  /* for LOWER outline : y-zero line is assumed at the min leftmost or
   * rightmost point
   */
  cd->zero2[OUTLINE_LOWER] = MAX (cd->curve_ptr[OUTLINE_LOWER][0],
                                  cd->curve_ptr[OUTLINE_LOWER][xmax]);
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
  gdouble y1,  y2;
  gdouble delta;

  y1 = cd->zero2[OUTLINE_UPPER] - cd->curve_ptr[OUTLINE_UPPER][x];
  y2 = cd->zero2[OUTLINE_LOWER] - cd->curve_ptr[OUTLINE_LOWER][x];

  delta = ((double)(y2 - y1) / (double)(total_steps - 1)) * current_step;

  return SIGNED_ROUND (y1 + delta);
}

static gint32
p_upper_curve_extend (BenderDialog *cd,
                      gint32        drawable_width,
                      gint32        drawable_height)
{
   gint32 y1, y2;

   y1 = cd->max2[OUTLINE_UPPER] - cd->zero2[OUTLINE_UPPER];
   y2 = (cd->max2[OUTLINE_LOWER] - cd->zero2[OUTLINE_LOWER]) - drawable_height;

   return MAX (y1, y2);
}

static gint32
p_lower_curve_extend (BenderDialog *cd,
                      gint32        drawable_width,
                      gint32        drawable_height)
{
   gint32  y1,  y2;

   y1 = cd->zero2[OUTLINE_LOWER] - cd->min2[OUTLINE_LOWER];
   y2 = (cd->zero2[OUTLINE_UPPER] - cd->min2[OUTLINE_UPPER]) - drawable_height;

   return MAX (y1, y2);
}

static void
p_end_gdrw (t_GDRW *gdrw)
{
  g_object_unref (gdrw->buffer);
}

static void
p_init_gdrw (t_GDRW       *gdrw,
             GimpDrawable *drawable,
             int           shadow)
{
  gint w, h;

  gdrw->drawable = drawable;

  if (shadow)
    gdrw->buffer = gimp_drawable_get_shadow_buffer (drawable);
  else
    gdrw->buffer = gimp_drawable_get_buffer (drawable);

  gdrw->width  = gimp_drawable_get_width  (gdrw->drawable);
  gdrw->height = gimp_drawable_get_height (gdrw->drawable);

  gdrw->tile_width  = gimp_tile_width ();
  gdrw->tile_height = gimp_tile_height ();

  if (! gimp_drawable_mask_intersect (gdrw->drawable,
                                      &gdrw->x1, &gdrw->y1, &w, &h))
    {
      w = 0;
      h = 0;
    }

  gdrw->x2 = gdrw->x1 + w;
  gdrw->y2 = gdrw->y1 + h;

  if (gimp_drawable_has_alpha (drawable))
    gdrw->format = babl_format ("R'G'B'A u8");
  else
    gdrw->format = babl_format ("R'G'B' u8");

  gdrw->bpp = babl_format_get_bytes_per_pixel (gdrw->format);

  if (gimp_drawable_has_alpha (drawable))
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

  gegl_buffer_sample (gdrw->buffer, x, y, NULL, pixel, gdrw->format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
}

static void
p_put_pixel (t_GDRW *gdrw,
             gint32  x,
             gint32  y,
             guchar *pixel)
{
  gegl_buffer_set (gdrw->buffer, GEGL_RECTANGLE (x, y, 1, 1), 0,
                   gdrw->format, pixel, GEGL_AUTO_ROWSTRIDE);
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
   guchar  pixel[4];
   guchar  mixmask;
   gint    idx;
   gint    diff;

   mixmask = 255 - 96;
   diff = abs(nb_curvy - curvy);

   if (diff == 0)
     {
       mixmask = 255 - 48;
       diff = abs(nb2_curvy - curvy);

       if (diff == 0)
         {
           /* last 2 neighbours were not shifted against current pixel, do not mix */
           p_put_pixel(gdrw, x, y, color);
           return;
         }
     }

   /* get left neighbour pixel */
   p_get_pixel(gdrw, x-1, y, &pixel[0]);

   if (pixel[gdrw->index_alpha] < 10)
     {
       /* neighbour is (nearly or full) transparent, do not mix */
       p_put_pixel(gdrw, x, y, color);
       return;
     }

   for (idx = 0; idx < gdrw->index_alpha ; idx++)
     {
       /* mix in left neighbour color */
       pixel[idx] = MIX_CHANNEL(color[idx], pixel[idx], mixmask);
     }

   pixel[gdrw->index_alpha] = color[gdrw->index_alpha];
   p_put_pixel(gdrw, x, y, &pixel[0]);
}

/* ============================================================================
 * p_create_pv_image
 * ============================================================================
 */
static GimpImage *
p_create_pv_image (GimpDrawable  *src_drawable,
                   GimpLayer    **layer)
{
  GimpImage    *new_image;
  guint         new_width;
  guint         new_height;
  GimpImageType type;
  guint         x, y;
  double        scale;
  guchar        pixel[4];
  t_GDRW        src_gdrw;
  t_GDRW        dst_gdrw;
  gint          src_width;
  gint          src_height;

  src_width  = gimp_drawable_get_width  (src_drawable);
  src_height = gimp_drawable_get_height (src_drawable);

  new_image = gimp_image_new (PREVIEW_SIZE_X, PREVIEW_SIZE_Y,
                              gimp_image_get_base_type (gimp_item_get_image (GIMP_ITEM (src_drawable))));
  gimp_image_undo_disable (new_image);

  type = gimp_drawable_type (src_drawable);
  if (src_height > src_width)
    {
      new_height = PV_IMG_HEIGHT;
      new_width = (src_width * new_height) / src_height;
      scale = (float) src_height / PV_IMG_HEIGHT;
    }
  else
    {
      new_width = PV_IMG_WIDTH;
      new_height = (src_height * new_width) / src_width;
      scale = (float) src_width / PV_IMG_WIDTH;
    }

  *layer = gimp_layer_new (new_image, "preview_original",
                           new_width, new_height,
                           type,
                           100.0,    /* opacity */
                           0);       /* mode NORMAL */
  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (*layer)))
    {
      /* always add alpha channel */
      gimp_layer_add_alpha (*layer);
    }

  gimp_image_insert_layer (new_image, *layer, NULL, 0);

  p_init_gdrw (&src_gdrw, src_drawable,           FALSE);
  p_init_gdrw (&dst_gdrw, GIMP_DRAWABLE (*layer), FALSE);

  for (y = 0; y < new_height; y++)
    {
      for (x = 0; x < new_width; x++)
        {
          p_get_pixel(&src_gdrw, x * scale, y * scale, &pixel[0]);
          p_put_pixel(&dst_gdrw, x, y, &pixel[0]);
        }
    }

  p_end_gdrw (&src_gdrw);
  p_end_gdrw (&dst_gdrw);

  return new_image;
}

/* ============================================================================
 * p_add_layer
 * ============================================================================
 */
static GimpLayer *
p_add_layer (gint          width,
             gint          height,
             GimpDrawable *src_drawable)
{
  GimpImageType  type;
  GimpLayer     *new_layer;
  char          *name;
  char          *name2;
  gdouble        opacity;
  GimpLayerMode  mode;
  gint           visible;
  GimpImage     *image;
  gint           stack_position;

  image = gimp_item_get_image (GIMP_ITEM (src_drawable));
  stack_position = 0;                                  /* TODO:  should be same as src_layer */

  /* copy type, name, opacity and mode from src_drawable */
  type   = gimp_drawable_type (src_drawable);
  visible  = gimp_item_get_visible (GIMP_ITEM (src_drawable));

  name2 = gimp_item_get_name (GIMP_ITEM (src_drawable));
  name = g_strdup_printf ("%s_b", name2);
  g_free (name2);

  mode = gimp_layer_get_mode (GIMP_LAYER (src_drawable));
  opacity = gimp_layer_get_opacity (GIMP_LAYER (src_drawable));  /* full opacity */

  new_layer = gimp_layer_new (image, name,
                              width, height,
                              type,
                              opacity,
                              mode);

  g_free (name);
  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (new_layer)))
    {
      /* always add alpha channel */
      gimp_layer_add_alpha (new_layer);
    }

  /* add the copied layer to the temp. working image */
  gimp_image_insert_layer (image, new_layer, NULL, stack_position);

  /* copy visibility state */
  gimp_item_set_visible (GIMP_ITEM (new_layer), visible);

  return new_layer;
}

/* ============================================================================
 * p_bender_calculate_iter_curve
 * ============================================================================
 */

static void
p_bender_calculate_iter_curve (BenderDialog        *cd,
                               GimpProcedureConfig *config,
                               gint32               xmax,
                               gint32               ymax)
{
  gint outline;
  gint curve_type;

  curve_type = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                    "curve-type");
  outline = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (cd->config),
                                                 "curve-border");

  if (gb_debug)
    g_printf ("p_bender_calculate_iter_curve NORMAL1\n");

  if (curve_type == SMOOTH)
    {
      g_object_set (cd->config,
                    "curve-border", "upper",
                    NULL);
      bender_calculate_curve (cd, xmax, ymax, FALSE);
      g_object_set (cd->config,
                    "curve-border", "lower",
                    NULL);
      bender_calculate_curve (cd, xmax, ymax, FALSE);
    }
  else
    {
      p_stretch_curves (cd, xmax, ymax);
    }

  g_object_set (config,
                "curve-border", (outline == OUTLINE_UPPER) ?
                                "upper" : "lower",
                NULL);
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
  gint32   row, col;
  gint32   first_row, first_col, last_row, last_col;
  gint32   x, y;
  gint32   x2, y2;
  gint32   curvy, nb_curvy, nb2_curvy;
  gint32   desty, othery;
  gint32   miny, maxy;
  gint32   sign, dy, diff;
  gint32   topshift;
  float    progress_step;
  float    progress_max;
  float    progress;

  t_Last  *last_arr;
  t_Last  *first_arr;
  guchar   color[4];
  guchar   mixcolor[4];
  gint     alias_dir;
  guchar   mixmask;

  gboolean antialias;
  gboolean smoothing;

  g_object_get (cd->config,
                "antialias", &antialias,
                "smoothing", &smoothing,
                NULL);

  topshift = p_upper_curve_extend (cd,
                                   src_gdrw->width,
                                   src_gdrw->height);
  diff = curvy = nb_curvy = nb2_curvy= miny = maxy = 0;

  /* allocate array of last values (one element foreach x coordinate) */
  last_arr  = g_new (t_Last, src_gdrw->x2);
  first_arr = g_new (t_Last, src_gdrw->x2);

  /* ------------------------------------------------
   * foreach pixel in the SAMPLE_drawable:
   * ------------------------------------------------
   * the inner loops (x/y) are designed to process
   * all pixels of one tile in the sample drawable, the outer loops (row/col) do step
   * to the next tiles. (this was done to reduce tile swapping)
   */

  first_row = src_gdrw->y1 / src_gdrw->tile_height;
  last_row  = (src_gdrw->y2 / src_gdrw->tile_height);
  first_col = src_gdrw->x1 / src_gdrw->tile_width;
  last_col  = (src_gdrw->x2 / src_gdrw->tile_width);

  /* init progress */
  progress_max = (1 + last_row - first_row) * (1 + last_col - first_col);
  progress_step = 1.0 / progress_max;
  progress = 0.0;
  if (cd->show_progress)
    gimp_progress_init ( _("Curve Bend"));

  for (row = first_row; row <= last_row; row++)
    {
      for (col = first_col; col <= last_col; col++)
        {
          if (col == first_col)
            x = src_gdrw->x1;
          else
            x = col * src_gdrw->tile_width;
          if (col == last_col)
            x2 = src_gdrw->x2;
          else
            x2 = (col +1) * src_gdrw->tile_width;

          if (cd->show_progress)
            gimp_progress_update (progress += progress_step);

          for( ; x < x2; x++)
            {
              if (row == first_row)
                y = src_gdrw->y1;
              else
                y = row * src_gdrw->tile_height;

              if (row == last_row)
                y2 = src_gdrw->y2;
              else
                y2 = (row +1) * src_gdrw->tile_height ;

              for( ; y < y2; y++)
                {
                  /* ---------- copy SRC_PIXEL to curve position ------ */

                  p_get_pixel (src_gdrw, x, y, color);

                  curvy = p_curve_get_dy (cd, x,
                                          (gint32)src_gdrw->width,
                                          (gint32)src_gdrw->height,
                                          (gdouble)y);
                  desty = y + topshift + curvy;

                  /* ----------- SMOOTHING ------------------ */
                  if (smoothing && (x > 0))
                    {
                      nb_curvy = p_curve_get_dy (cd, x -1,
                                                 (gint32) src_gdrw->width,
                                                 (gint32) src_gdrw->height,
                                                 (gdouble) y);
                      if ((nb_curvy == curvy) && (x > 1))
                        {
                          nb2_curvy = p_curve_get_dy (cd, x -2,
                                                      (gint32) src_gdrw->width,
                                                      (gint32) src_gdrw->height,
                                                      (gdouble) y);
                        }
                      else
                        {
                          nb2_curvy = nb_curvy;
                        }

                      p_put_mix_pixel (dst_gdrw, x, desty, color, nb_curvy, nb2_curvy, curvy);
                    }
                  else
                    {
                      p_put_pixel (dst_gdrw, x, desty, color);
                    }

                  /* ----------- render ANTIALIAS ------------------ */

                  if (antialias)
                    {
                      othery = desty;

                      if (y == src_gdrw->y1)             /* Upper outline */
                        {
                          first_arr[x].y = curvy;
                          memcpy (first_arr[x].color, color,
                                  dst_gdrw->bpp);

                          if (x > 0)
                            {
                              memcpy (mixcolor, first_arr[x-1].color,
                                      dst_gdrw->bpp);

                              diff = abs(first_arr[x - 1].y - curvy) +1;
                              miny = MIN(first_arr[x - 1].y, curvy) -1;
                              maxy = MAX(first_arr[x - 1].y, curvy) +1;

                              othery = (src_gdrw->y2 -1)
                                + topshift
                                + p_curve_get_dy(cd, x,
                                                 (gint32)src_gdrw->width,
                                                 (gint32)src_gdrw->height,
                                                 (gdouble)(src_gdrw->y2 -1));
                            }
                        }

                      if (y == src_gdrw->y2 - 1)      /* Lower outline */
                        {
                          if (x > 0)
                            {
                              memcpy (mixcolor, last_arr[x-1].color,
                                      dst_gdrw->bpp);

                              diff = abs (last_arr[x - 1].y - curvy) +1;
                              maxy = MAX (last_arr[x - 1].y, curvy) +1;
                              miny = MIN (last_arr[x - 1].y, curvy) -1;
                            }

                          othery = (src_gdrw->y1)
                            + topshift
                            + p_curve_get_dy(cd, x,
                                             (gint32)src_gdrw->width,
                                             (gint32)src_gdrw->height,
                                             (gdouble)(src_gdrw->y1));
                        }

                      if (desty < othery)
                        alias_dir =  1;          /* fade to transp. with descending dy */
                      else if (desty > othery)
                        alias_dir = -1;          /* fade to transp. with ascending dy */
                      else
                        alias_dir =  0;          /* no antialias at curve crossing point(s) */

                      if (alias_dir != 0)
                        {
                          guchar alpha_lo = 20;

                          if (gimp_drawable_has_alpha (src_gdrw->drawable))
                            {
                              alpha_lo = MIN (20, mixcolor[src_gdrw->index_alpha]);
                            }

                          for (dy = 0; dy < diff; dy++)
                            {
                              /* iterate for fading alpha channel */
                              mixmask =  255 * ((gdouble)(dy + 1) / (gdouble) (diff+1));
                              mixcolor[dst_gdrw->index_alpha] = MIX_CHANNEL(color[dst_gdrw->index_alpha], alpha_lo, mixmask);

                              if(alias_dir > 0)
                                {
                                  p_put_pixel (dst_gdrw, x -1, y + topshift  + miny + dy, mixcolor);
                                }
                              else
                                {
                                  p_put_pixel (dst_gdrw, x -1, y + topshift  + (maxy - dy), mixcolor);
                                }

                            }
                        }
                    }

                  /* ------------------ FILL HOLES ------------------ */

                  if (y == src_gdrw->y1)
                    {
                      diff = 0;
                      sign = 1;
                    }
                  else
                    {
                      diff = last_arr[x].y - curvy;
                      if (diff < 0)
                        {
                          diff = 0 - diff;
                          sign = -1;
                        }
                      else
                        {
                          sign = 1;
                        }

                      memcpy (mixcolor, color, dst_gdrw->bpp);
                    }

                  for (dy = 1; dy <= diff; dy++)
                    {
                      /* y differs more than 1 pixel from last y in the
                       * destination drawable. So we have to fill the empty
                       * space between using a mixed color
                       */

                      if (smoothing)
                        {
                          /* smoothing is on, so we are using a mixed color */
                          gulong alpha1 = last_arr[x].color[3];
                          gulong alpha2 = color[3];
                          gulong alpha;

                          mixmask =  255 * ((gdouble)(dy) / (gdouble)(diff+1));
                          alpha = alpha1 * mixmask + alpha2 * (255 - mixmask);
                          mixcolor[3] = alpha/255;
                          if (mixcolor[3])
                            {
                              mixcolor[0] = (alpha1 * mixmask * last_arr[x].color[0]
                                             + alpha2 * (255 - mixmask) * color[0])/alpha;
                              mixcolor[1] = (alpha1 * mixmask * last_arr[x].color[1]
                                             + alpha2 * (255 - mixmask) * color[1])/alpha;
                              mixcolor[2] = (alpha1 * mixmask * last_arr[x].color[2]
                                             + alpha2 * (255 - mixmask) * color[2])/alpha;
                              /*mixcolor[2] =  MIX_CHANNEL(last_arr[x].color[2], color[2], mixmask);*/
                            }
                        }
                      else
                        {
                          /* smoothing is off, so we are using this color or
                             the last color */
                          if (dy < diff / 2)
                            {
                              memcpy (mixcolor, color,
                                      dst_gdrw->bpp);
                            }
                          else
                            {
                              memcpy (mixcolor, last_arr[x].color,
                                      dst_gdrw->bpp);
                            }
                        }

                      if (smoothing)
                        {
                          p_put_mix_pixel (dst_gdrw, x,
                                           desty + (dy * sign),
                                           mixcolor,
                                           nb_curvy, nb2_curvy, curvy );
                        }
                      else
                        {
                          p_put_pixel (dst_gdrw, x,
                                       desty + (dy * sign), mixcolor);
                        }
                    }

                  /* store y and color */
                  last_arr[x].y = curvy;
                  memcpy (last_arr[x].color, color, dst_gdrw->bpp);
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

static GimpLayer *
p_main_bend (BenderDialog        *cd,
             GimpDrawable        *original_drawable,
             GimpProcedureConfig *config,
             gint                 work_on_copy)
{
   t_GDRW        src_gdrw;
   t_GDRW        dst_gdrw;
   GimpDrawable *src_drawable;
   GimpDrawable *dst_drawable;
   gint          src_width;
   gint          src_height;
   gint32        dst_height;
   GimpImage    *image;
   GimpLayer    *tmp_layer;
   gint32        interpolation;
   gint          offset_x, offset_y;
   gint          center_x, center_y;
   gint32        xmax, ymax;
   gdouble       rotation;

   g_object_get (config,
                 "smoothing", &interpolation,
                 "rotation",  &rotation,
                 NULL);

   image = gimp_item_get_image (GIMP_ITEM (original_drawable));
   gimp_drawable_get_offsets (original_drawable, &offset_x, &offset_y);

   center_x = offset_x + (gimp_drawable_get_width  (original_drawable) / 2 );
   center_y = offset_y + (gimp_drawable_get_height (original_drawable) / 2 );

   /* always copy original_drawable to a tmp src_layer */
   tmp_layer = gimp_layer_copy (GIMP_LAYER (original_drawable));
   /* set layer invisible and dummyname and
    * add at top of the image while working
    * (for the case of undo GIMP must know,
    *  that the layer was part of the image)
    */
   gimp_image_insert_layer (image, tmp_layer, NULL, 0);
   gimp_item_set_visible (GIMP_ITEM (tmp_layer), FALSE);
   gimp_item_set_name (GIMP_ITEM (tmp_layer), "curve_bend_dummylayer");

   if(gb_debug) g_printf("p_main_bend  tmp_layer_id %d\n",
                         (int) gimp_item_get_id (GIMP_ITEM (tmp_layer)));

   if (rotation != 0.0)
     {
       if(gb_debug) g_printf("p_main_bend rotate: %f\n", (float) rotation);
       p_gimp_rotate (image, GIMP_DRAWABLE (tmp_layer),
                      interpolation, rotation);
     }

   src_drawable = GIMP_DRAWABLE (tmp_layer);

   src_width  = gimp_drawable_get_width  (GIMP_DRAWABLE (tmp_layer));
   src_height = gimp_drawable_get_height (GIMP_DRAWABLE (tmp_layer));

   xmax = ymax = src_width -1;
   cd->curve_ptr[OUTLINE_UPPER] = g_new (gint32, 1+xmax);
   cd->curve_ptr[OUTLINE_LOWER] = g_new (gint32, 1+xmax);

   p_bender_calculate_iter_curve (cd, config, xmax, ymax);
   bender_init_min_max (cd, xmax);

   dst_height = src_height
                + p_upper_curve_extend(cd, src_width, src_height)
                + p_lower_curve_extend(cd, src_width, src_height);

   if(gb_debug) g_printf("p_main_bend: dst_height:%d\n", dst_height);

   if (work_on_copy)
     {
       dst_drawable = GIMP_DRAWABLE (p_add_layer (src_width, dst_height,
                                                  src_drawable));
       if (gb_debug) g_printf("p_main_bend: DONE add layer\n");
     }
   else
     {
       /* work on the original */
       gimp_layer_resize (GIMP_LAYER (original_drawable),
                          src_width,
                          dst_height,
                          offset_x, offset_y);
       if (gb_debug) g_printf("p_main_bend: DONE layer resize\n");
       if (! gimp_drawable_has_alpha (original_drawable))
         {
           /* always add alpha channel */
           gimp_layer_add_alpha (GIMP_LAYER (original_drawable));
         }

       dst_drawable = original_drawable;
     }

   gimp_drawable_fill (dst_drawable, GIMP_FILL_TRANSPARENT);

   p_init_gdrw (&src_gdrw, src_drawable, FALSE);
   p_init_gdrw (&dst_gdrw, dst_drawable, FALSE);

   p_vertical_bend (cd, &src_gdrw, &dst_gdrw);

   if(gb_debug) g_printf("p_main_bend: DONE vertical bend\n");

   p_end_gdrw (&src_gdrw);
   p_end_gdrw (&dst_gdrw);

   if (rotation != 0.0)
     {
       p_gimp_rotate (image, dst_drawable,
                      interpolation, (gdouble)(360.0 - rotation));

       /* TODO: here we should crop dst_drawable to cut off full transparent borderpixels */
     }

   /* set offsets of the resulting new layer
    *(center == center of original_drawable)
    */
   offset_x = center_x - (gimp_drawable_get_width  (dst_drawable) / 2 );
   offset_y = center_y - (gimp_drawable_get_height (dst_drawable) / 2 );
   gimp_layer_set_offsets (GIMP_LAYER (dst_drawable), offset_x, offset_y);

   /* delete the temp layer */
   gimp_image_remove_layer (image, tmp_layer);

   g_free (cd->curve_ptr[OUTLINE_UPPER]);
   g_free (cd->curve_ptr[OUTLINE_LOWER]);

   if (gb_debug) g_printf("p_main_bend: DONE bend main\n");

   return GIMP_LAYER (dst_drawable);
}
