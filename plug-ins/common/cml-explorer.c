/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * CML_explorer.c
 * Time-stamp: <2000-02-13 18:18:37 yasuhiro>
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
 * Version: 1.0.11
 * URL: http://www.inetq.or.jp/~narazaki/TheGIMP/
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
 *
 * Comment:
 *  CML is the abbreviation of Coupled-Map Lattice that is a model of
 *  complex systems, proposed by a physicist[1,2].
 *
 *  Similar models are summaried as follows:
 *
 *                      Value    Time     Space
 *  Coupled-Map Lattice cont.    discrete discrete
 *  Celluar Automata    discrete discrete discrete
 *  Diffrential Eq.     cont.    cont.    cont.
 *
 *  (But this program uses a parameter: hold-rate to avoid very fast changes.
 *  Thus time is rather continuous than discrete.
 *  Yes, this change to model changes the output completely.)
 *
 *  References:
 *  1. Kunihiko Kaneko, Period-doubling of kind-antikink patterns,
 *     quasi-periodicity in antiferro-like structures and spatial
 *     intermittency in coupled map lattices -- Toward a prelude to a
 *     "field theory of chaos", Prog. Theor. Phys. 72 (1984) 480.
 *
 *  2. Kunihiko Kaneko ed., Theory and Applications of Coupled Map
 *     Lattices (Wiley, 1993).
 *
 *  About Parameter File:
 *  I assume that the possible longest line in CMP parameter file is 1023.
 *  Please read CML_save_to_file_callback if you want know details of syntax.
 *
 *  Format version 1.0 starts with:
 *    ; This is a parameter file for CML_explorer
 *    ; File format version: 1.0
 *    ;
 *      Hue
 *
 *  The old format for CML_explorer included in gimp-0.99.[89] is:
 *    ; CML parameter file (version: 1.0)
 *    ; Hue
 *
 * (This file format is interpreted as format version 0.99 now.)
 *
 * Thanks:
 *  This version contains patches from:
 *    Tim Mooney <mooney@dogbert.cc.ndsu.NoDak.edu>
 *    Sean P Cier <scier@andrew.cmu.edu>
 *    David Mosberger-Tang <davidm@azstarnet.com>
 *    Michael Sweet <mike@easysw.com>
 *
 */
#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define PARAM_FILE_FORMAT_VERSION 1.0
#define PLUG_IN_PROC              "plug-in-cml-explorer"
#define PLUG_IN_BINARY            "cml-explorer"
#define PLUG_IN_ROLE              "gimp-cml-explorer"
#define VALS                      CML_explorer_vals
#define PROGRESS_UPDATE_NUM        100
#define CML_LINE_SIZE             1024
#define TILE_CACHE_SIZE             32
#define SCALE_WIDTH                130
#define PREVIEW_WIDTH               64
#define PREVIEW_HEIGHT             220

#define CANNONIZE(p, x)      (255*(((p).range_h - (p).range_l)*(x) + (p).range_l))
#define HCANNONIZE(p, x)     (254*(((p).range_h - (p).range_l)*(x) + (p).range_l))
#define POS_IN_TORUS(i,size) ((i < 0) ? size + i : ((size <= i) ? i - size : i))

typedef struct _WidgetEntry WidgetEntry;

struct _WidgetEntry
{
  GtkWidget  *widget;
  gpointer    value;
  void      (*updater) (WidgetEntry *);
};

enum
{
  CML_KEEP_VALUES,
  CML_KEEP_FIRST,
  CML_FILL,
  CML_LOGIST,
  CML_LOGIST_STEP,
  CML_POWER,
  CML_POWER_STEP,
  CML_REV_POWER,
  CML_REV_POWER_STEP,
  CML_DELTA,
  CML_DELTA_STEP,
  CML_SIN_CURVE,
  CML_SIN_CURVE_STEP,
  CML_NUM_VALUES
};

static const gchar *function_names[CML_NUM_VALUES] =
{
  N_("Keep image's values"),
  N_("Keep the first value"),
  N_("Fill with parameter k"),
  N_("k{x(1-x)}^p"),
  N_("k{x(1-x)}^p stepped"),
  N_("kx^p"),
  N_("kx^p stepped"),
  N_("k(1-x^p)"),
  N_("k(1-x^p) stepped"),
  N_("Delta function"),
  N_("Delta function stepped"),
  N_("sin^p-based function"),
  N_("sin^p, stepped")
};

enum
{
  COMP_NONE,
  COMP_MAX_LINEAR,
  COMP_MAX_LINEAR_P1,
  COMP_MAX_LINEAR_M1,
  COMP_MIN_LINEAR,
  COMP_MIN_LINEAR_P1,
  COMP_MIN_LINEAR_M1,
  COMP_MAX_LINEAR_P1L,
  COMP_MAX_LINEAR_P1U,
  COMP_MAX_LINEAR_M1L,
  COMP_MAX_LINEAR_M1U,
  COMP_MIN_LINEAR_P1L,
  COMP_MIN_LINEAR_P1U,
  COMP_MIN_LINEAR_M1L,
  COMP_MIN_LINEAR_M1U,
  COMP_NUM_VALUES
};

static const gchar *composition_names[COMP_NUM_VALUES] =
{
  NC_("cml-composition", "None"),
  N_("Max (x, -)"),
  N_("Max (x+d, -)"),
  N_("Max (x-d, -)"),
  N_("Min (x, -)"),
  N_("Min (x+d, -)"),
  N_("Min (x-d, -)"),
  N_("Max (x+d, -), (x < 0.5)"),
  N_("Max (x+d, -), (0.5 < x)"),
  N_("Max (x-d, -), (x < 0.5)"),
  N_("Max (x-d, -), (0.5 < x)"),
  N_("Min (x+d, -), (x < 0.5)"),
  N_("Min (x+d, -), (0.5 < x)"),
  N_("Min (x-d, -), (x < 0.5)"),
  N_("Min (x-d, -), (0.5 < x)")
};

enum
{
  STANDARD,
  AVERAGE,
  ANTILOG,
  RAND_POWER0,
  RAND_POWER1,
  RAND_POWER2,
  MULTIPLY_RANDOM0,
  MULTIPLY_RANDOM1,
  MULTIPLY_GRADIENT,
  RAND_AND_P,
  ARRANGE_NUM_VALUES
};

static const gchar *arrange_names[ARRANGE_NUM_VALUES] =
{
  N_("Standard"),
  N_("Use average value"),
  N_("Use reverse value"),
  N_("With random power (0,10)"),
  N_("With random power (0,1)"),
  N_("With gradient power (0,1)"),
  N_("Multiply rand. value (0,1)"),
  N_("Multiply rand. value (0,2)"),
  N_("Multiply gradient (0,1)"),
  N_("With p and random (0,1)"),
};

enum
{
  CML_INITIAL_RANDOM_INDEPENDENT = 6,
  CML_INITIAL_RANDOM_SHARED,
  CML_INITIAL_RANDOM_FROM_SEED,
  CML_INITIAL_RANDOM_FROM_SEED_SHARED,
  CML_INITIAL_NUM_VALUES
};

static const gchar *initial_value_names[CML_INITIAL_NUM_VALUES] =
{
  N_("All black"),
  N_("All gray"),
  N_("All white"),
  N_("The first row of the image"),
  N_("Continuous gradient"),
  N_("Continuous grad. w/o gap"),
  N_("Random, ch. independent"),
  N_("Random shared"),
  N_("Randoms from seed"),
  N_("Randoms from seed (shared)")
};

#define CML_PARAM_NUM   15

typedef struct
{
  gint    function;
  gint    composition;
  gint    arrange;
  gint    cyclic_range;
  gdouble mod_rate;             /* diff / old-value */
  gdouble env_sensitivity;      /* self-diff : env-diff */
  gint    diffusion_dist;
  gdouble ch_sensitivity;
  gint    range_num;
  gdouble power;
  gdouble parameter_k;
  gdouble range_l;
  gdouble range_h;
  gdouble mutation_rate;
  gdouble mutation_dist;
} CML_PARAM;

typedef struct
{
  CML_PARAM hue;
  CML_PARAM sat;
  CML_PARAM val;
  gint      initial_value;
  gint      scale;
  gint      start_offset;
  gint      seed;
  gchar     last_file_name[256];
} ValueType;

static ValueType VALS =
{
  /* function      composition  arra
    cyc chng sens  diff cor  n  pow  k    (l,h)   rnd  dist */
  {
    CML_SIN_CURVE, COMP_NONE,   STANDARD,
    1,  0.5, 0.7,  2,   0.0, 1, 1.0, 1.0, 0, 1,   0.0, 0.1
  },
  {
    CML_FILL,      COMP_NONE,    STANDARD,
    0,  0.6, 0.1,  2,   0.0, 1, 1.4, 0.9, 0, 0.9, 0.0, 0.1
  },
  {
    CML_FILL,      COMP_NONE,    STANDARD,
    0,  0.5, 0.2,  2,   0.0, 1, 2.0, 1.0, 0, 0.9, 0.0, 0.1
  },
  6,    /* random value 1 */
  1,    /* scale */
  0,    /* start_offset */
  0,    /* seed */
  ""    /* last filename */
};

static CML_PARAM *channel_params[] =
{
  &VALS.hue,
  &VALS.sat,
  &VALS.val
};

static const gchar *channel_names[] =
{
  N_("Hue"),
  N_("Saturation"),
  N_("Value")
};

static const gchar *load_channel_names[] =
{
  N_("(None)"),
  N_("Hue"),
  N_("Saturation"),
  N_("Value")
};

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

static GimpPDBStatusType CML_main_function     (gboolean   preview_p);
static void              CML_compute_next_step (gint       size,
                                                gdouble  **h,
                                                gdouble  **s,
                                                gdouble  **v,
                                                gdouble  **hn,
                                                gdouble  **sn,
                                                gdouble  **vn,
                                                gdouble  **haux,
                                                gdouble  **saux,
                                                gdouble  **vaux);
static gdouble           CML_next_value        (gdouble   *vec,
                                                gint       pos,
                                                gint       size,
                                                gdouble    c1,
                                                gdouble    c2,
                                                CML_PARAM *param,
                                                gdouble    aux);
static gdouble           logistic_function     (CML_PARAM *param,
                                                gdouble    x,
                                                gdouble    power);


static gint        CML_explorer_dialog           (void);
static GtkWidget * CML_dialog_channel_panel_new  (CML_PARAM     *param,
                                                  gint           channel_id);
static GtkWidget * CML_dialog_advanced_panel_new (void);

static void     CML_explorer_toggle_entry_init   (WidgetEntry   *widget_entry,
                                                  GtkWidget     *widget,
                                                  gpointer       value_ptr);

static void     CML_explorer_int_entry_init      (WidgetEntry   *widget_entry,
                                                  GtkAdjustment *object,
                                                  gpointer       value_ptr);

static void     CML_explorer_double_entry_init   (WidgetEntry   *widget_entry,
                                                  GtkAdjustment *object,
                                                  gpointer       value_ptr);

static void     CML_explorer_menu_update         (GtkWidget   *widget,
                                                  gpointer     data);
static void     CML_initial_value_menu_update    (GtkWidget   *widget,
                                                  gpointer     data);
static void     CML_explorer_menu_entry_init     (WidgetEntry *widget_entry,
                                                  GtkWidget   *widget,
                                                  gpointer     value_ptr);

static void    preview_update                      (void);
static void    function_graph_new                  (GtkWidget *widget,
                                                    gpointer  *data);
static void    CML_set_or_randomize_seed_callback  (GtkWidget *widget,
                                                    gpointer   data);
static void    CML_copy_parameters_callback        (GtkWidget *widget,
                                                    gpointer   data);
static void    CML_initial_value_sensitives_update (void);

static void    CML_save_to_file_callback   (GtkWidget        *widget,
                                            gpointer          data);
static void    CML_save_to_file_response   (GtkWidget        *dialog,
                                            gint              response_id,
                                            gpointer          data);

static void    CML_preview_update_callback (GtkWidget        *widget,
                                            gpointer          data);
static void    CML_load_from_file_callback (GtkWidget        *widget,
                                            gpointer          data);
static gboolean CML_load_parameter_file     (const gchar     *filename,
                                             gboolean         interactive_mode);
static void    CML_load_from_file_response (GtkWidget        *dialog,
                                            gint              response_id,
                                            gpointer          data);
static gint    parse_line_to_gint          (FILE             *file,
                                            gboolean         *flag);
static gdouble parse_line_to_gdouble       (FILE             *file,
                                            gboolean         *flag);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static GtkWidget   *preview;
static WidgetEntry  widget_pointers[4][CML_PARAM_NUM];

static guchar          *img;
static gint             img_stride;
static cairo_surface_t *buffer;

typedef struct
{
  GtkWidget *widget;
  gint       logic;
} CML_sensitive_widget_table;

#define RANDOM_SENSITIVES_NUM 5
#define GRAPHSIZE 256

static CML_sensitive_widget_table random_sensitives[RANDOM_SENSITIVES_NUM] =
{
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 }
};

static GRand    *gr;
static gint      drawable_id = 0;
static gint      copy_source = 0;
static gint      copy_destination = 0;
static gint      selective_load_source = 0;
static gint      selective_load_destination = 0;
static gboolean  CML_preview_defer = FALSE;

static gdouble  *mem_chank0 = NULL;
static gint      mem_chank0_size = 0;
static guchar   *mem_chank1 = NULL;
static gint      mem_chank1_size = 0;
static guchar   *mem_chank2 = NULL;
static gint      mem_chank2_size = 0;

MAIN ()

static void
query (void)
{
  static const GimpParamDef args [] =
  {
    { GIMP_PDB_INT32,    "ru-_mode",           "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",              "Input image (not used)" },
    { GIMP_PDB_DRAWABLE, "drawable",           "Input drawable"  },
    { GIMP_PDB_STRING,   "parameter-filename", "The name of parameter file. CML_explorer makes an image with its settings." }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Create abstract Coupled-Map Lattice patterns"),
                          "Make an image of Coupled-Map Lattice (CML). CML is "
                          "a kind of Cellula Automata on continuous (value) "
                          "domain. In GIMP_RUN_NONINTERACTIVE, the name of a "
                          "parameter file is passed as the 4th arg. You can "
                          "control CML_explorer via parameter file.",
                          /*  Or do you want to call me with over 50 args? */
                          "Shuji Narazaki (narazaki@InetQ.or.jp); "
                          "http://www.inetq.or.jp/~narazaki/TheGIMP/",
                          "Shuji Narazaki",
                          "1997",
                          N_("CML _Explorer..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Render/Pattern");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_EXECUTION_ERROR;
  GimpRunMode       run_mode;

  run_mode    = param[0].data.d_int32;
  drawable_id = param[2].data.d_drawable;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &VALS);
      if (! CML_explorer_dialog ())
        return;
      break;
    case GIMP_RUN_NONINTERACTIVE:
      {
        gchar *filename = param[3].data.d_string;

        if (! CML_load_parameter_file (filename, FALSE))
          return;
        break;
      }
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &VALS);
      break;
    }

  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);
  status = CML_main_function (FALSE);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush();
  if (run_mode == GIMP_RUN_INTERACTIVE && status == GIMP_PDB_SUCCESS)
    gimp_set_data (PLUG_IN_PROC, &VALS, sizeof (ValueType));

  g_free (mem_chank0);
  g_free (mem_chank1);
  g_free (mem_chank2);

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static GimpPDBStatusType
CML_main_function (gboolean preview_p)
{
  GimpDrawable *drawable = NULL;
  GimpPixelRgn  dest_rgn, src_rgn;
  guchar    *dest_buffer = NULL;
  guchar    *src_buffer  = NULL;
  gint       x, y;
  gint       dx, dy;
  gboolean   dest_has_alpha = FALSE;
  gboolean   dest_is_gray   = FALSE;
  gboolean   src_has_alpha  = FALSE;
  gboolean   src_is_gray    = FALSE;
  gint       total, processed = 0;
  gint       keep_height = 1;
  gint       cell_num, width_by_pixel, height_by_pixel;
  gint       index;
  gint       src_bpp, src_bpl;
  gint       dest_bpp, dest_bpl;
  gdouble   *hues, *sats, *vals;
  gdouble   *newh, *news, *newv;
  gdouble   *haux, *saux, *vaux;

  /* open THE drawable */
  drawable = gimp_drawable_get (drawable_id);

  if (! gimp_drawable_mask_intersect (drawable_id,
                                      &x, &y,
                                      &width_by_pixel, &height_by_pixel))
    return GIMP_PDB_SUCCESS;

  src_has_alpha = dest_has_alpha = gimp_drawable_has_alpha (drawable_id);
  src_is_gray = dest_is_gray = gimp_drawable_is_gray (drawable_id);
  src_bpp = dest_bpp = (src_is_gray ? 1 : 3) + (src_has_alpha ? 1 : 0);

  if (preview_p)
    {
      dest_has_alpha = FALSE;
      dest_bpp       = 3;

      if (width_by_pixel > PREVIEW_WIDTH)      /* preview < drawable (selection) */
        width_by_pixel = PREVIEW_WIDTH;
      if (height_by_pixel > PREVIEW_HEIGHT)
        height_by_pixel = PREVIEW_HEIGHT;
    }
  dest_bpl = width_by_pixel * dest_bpp;
  src_bpl = width_by_pixel * src_bpp;
  cell_num = (width_by_pixel - 1)/ VALS.scale + 1;
  total = height_by_pixel * width_by_pixel;
  if (total < 1)
    return GIMP_PDB_EXECUTION_ERROR;
  keep_height = VALS.scale;

  /* configure reusable memories */
  if (mem_chank0_size < 9 * cell_num * sizeof (gdouble))
    {
      g_free (mem_chank0);
      mem_chank0_size = 9 * cell_num * sizeof (gdouble);
      mem_chank0 = (gdouble *) g_malloc (mem_chank0_size);
    }
  hues = mem_chank0;
  sats = mem_chank0 + cell_num;
  vals = mem_chank0 + 2 * cell_num;
  newh = mem_chank0 + 3 * cell_num;
  news = mem_chank0 + 4 * cell_num;
  newv = mem_chank0 + 5 * cell_num;
  haux = mem_chank0 + 6 * cell_num;
  saux = mem_chank0 + 7 * cell_num;
  vaux = mem_chank0 + 8 * cell_num;

  if (mem_chank1_size < src_bpl * keep_height)
    {
      g_free (mem_chank1);
      mem_chank1_size = src_bpl * keep_height;
      mem_chank1 = (guchar *) g_malloc (mem_chank1_size);
    }
  src_buffer = mem_chank1;

  if (mem_chank2_size < dest_bpl * keep_height)
    {
      g_free (mem_chank2);
      mem_chank2_size = dest_bpl * keep_height;
      mem_chank2 = (guchar *) g_malloc (mem_chank2_size);
    }
  dest_buffer = mem_chank2;

  if (! preview_p)
    gimp_pixel_rgn_init (&dest_rgn, drawable, x, y,
                         width_by_pixel, height_by_pixel,
                         TRUE, TRUE);

  gimp_pixel_rgn_init (&src_rgn, drawable, x, y,
                       width_by_pixel, height_by_pixel,
                       FALSE, FALSE);

  gr = g_rand_new ();
  if (VALS.initial_value == CML_INITIAL_RANDOM_FROM_SEED)
    g_rand_set_seed (gr, VALS.seed);

  for (index = 0; index < cell_num; index++)
    {
      switch (VALS.hue.arrange)
        {
        case RAND_POWER0:
          haux [index] = g_rand_double_range (gr, 0, 10);
          break;
        case RAND_POWER2:
        case MULTIPLY_GRADIENT:
          haux [index] = (gdouble) abs ((index % 511) - 255) / (gdouble) 256;
          break;
        case RAND_POWER1:
        case MULTIPLY_RANDOM0:
          haux [index] = g_rand_double (gr);
          break;
        case MULTIPLY_RANDOM1:
          haux [index] = g_rand_double_range (gr, 0, 2);
          break;
        case RAND_AND_P:
          haux [index] = ((index % (2 * VALS.hue.diffusion_dist) == 0) ?
                          g_rand_double (gr) : VALS.hue.power);
          break;
        default:
          haux [index] = VALS.hue.power;
          break;
        }
      switch (VALS.sat.arrange)
        {
        case RAND_POWER0:
          saux [index] = g_rand_double_range (gr, 0, 10);
          break;
        case RAND_POWER2:
        case MULTIPLY_GRADIENT:
          saux [index] = (gdouble) abs ((index % 511) - 255) / (gdouble) 256;
          break;
        case RAND_POWER1:
        case MULTIPLY_RANDOM0:
          saux [index] = g_rand_double (gr);
          break;
        case MULTIPLY_RANDOM1:
          saux [index] = g_rand_double_range (gr, 0, 2);
          break;
        case RAND_AND_P:
          saux [index] = ((index % (2 * VALS.sat.diffusion_dist) == 0) ?
                          g_rand_double (gr) : VALS.sat.power);
          break;
        default:
          saux [index] = VALS.sat.power;
          break;
        }
      switch (VALS.val.arrange)
        {
        case RAND_POWER0:
          vaux [index] = g_rand_double_range (gr, 0, 10);
          break;
        case RAND_POWER2:
        case MULTIPLY_GRADIENT:
          vaux [index] = (gdouble) abs ((index % 511) - 255) / (gdouble) 256;
          break;
        case RAND_POWER1:
        case MULTIPLY_RANDOM0:
          vaux [index] = g_rand_double (gr);
          break;
        case MULTIPLY_RANDOM1:
          vaux [index] = g_rand_double_range (gr, 0, 2);
          break;
        case RAND_AND_P:
          vaux [index] = ((index % (2 * VALS.val.diffusion_dist) == 0) ?
                          g_rand_double (gr) : VALS.val.power);
          break;
        default:
          vaux [index] = VALS.val.power;
          break;
        }
      switch (VALS.initial_value)
        {
        case 0:
        case 1:
        case 2:
          hues[index] = sats[index] = vals[index] = 0.5 * (VALS.initial_value);
          break;
        case 3:                 /* use the values of the image (drawable) */
          break;                /* copy from the drawable after this loop */
        case 4:                 /* grandient 1 */
          hues[index] = sats[index] = vals[index]
            = (gdouble) (index % 256) / (gdouble) 256;
          break;                /* gradinet 2 */
        case 5:
          hues[index] = sats[index] = vals[index]
            = (gdouble) abs ((index % 511) - 255) / (gdouble) 256;
          break;
        case CML_INITIAL_RANDOM_INDEPENDENT:
        case CML_INITIAL_RANDOM_FROM_SEED:
          hues[index] = g_rand_double (gr);
          sats[index] = g_rand_double (gr);
          vals[index] = g_rand_double (gr);
          break;
        case CML_INITIAL_RANDOM_SHARED:
        case CML_INITIAL_RANDOM_FROM_SEED_SHARED:
          hues[index] = sats[index] = vals[index] = g_rand_double (gr);
          break;
        }
    }

  if (VALS.initial_value == 3)
    {
      int       index;

      for (index = 0;
           index < MIN (cell_num, width_by_pixel / VALS.scale);
           index++)
        {
          guchar buffer[4];
          int   rgbi[3];
          int   i;

          gimp_pixel_rgn_get_pixel (&src_rgn, buffer,
                                    x + (index * VALS.scale), y);
          for (i = 0; i < 3; i++) rgbi[i] = buffer[i];
          gimp_rgb_to_hsv_int (rgbi, rgbi + 1, rgbi + 2);
          hues[index] = (gdouble) rgbi[0] / (gdouble) 255;
          sats[index] = (gdouble) rgbi[1] / (gdouble) 255;
          vals[index] = (gdouble) rgbi[2] / (gdouble) 255;
        }
    }

  if (! preview_p)
    gimp_progress_init (_("CML Explorer: evoluting"));

  /* rolling start */
  for (index = 0; index < VALS.start_offset; index++)
    CML_compute_next_step (cell_num, &hues, &sats, &vals, &newh, &news, &newv,
                           &haux, &saux, &vaux);

  /* rendering */
  for (dy = 0; dy < height_by_pixel; dy += VALS.scale)
    {
      gint r, g, b, h, s, v;
      gint offset_x, offset_y, dest_offset;

      if (height_by_pixel < dy + keep_height)
        keep_height = height_by_pixel - dy;

      if ((VALS.hue.function == CML_KEEP_VALUES) ||
          (VALS.sat.function == CML_KEEP_VALUES) ||
          (VALS.val.function == CML_KEEP_VALUES))
        gimp_pixel_rgn_get_rect (&src_rgn, src_buffer,
                                 x, y + dy, width_by_pixel, keep_height);

      CML_compute_next_step (cell_num,
                             &hues, &sats, &vals,
                             &newh, &news, &newv,
                             &haux, &saux, &vaux);

      for (dx = 0; dx < cell_num; dx++)
        {
          h = r = HCANNONIZE (VALS.hue, hues[dx]);
          s = g = CANNONIZE (VALS.sat, sats[dx]);
          v = b = CANNONIZE (VALS.val, vals[dx]);

          if (! dest_is_gray)
            gimp_hsv_to_rgb_int (&r, &g, &b);

          /* render destination */
          for (offset_y = 0;
               (offset_y < VALS.scale) && (dy + offset_y < height_by_pixel);
               offset_y++)
            for (offset_x = 0;
                 (offset_x < VALS.scale) && (dx * VALS.scale + offset_x < width_by_pixel);
                 offset_x++)
              {
                if ((VALS.hue.function == CML_KEEP_VALUES) ||
                    (VALS.sat.function == CML_KEEP_VALUES) ||
                    (VALS.val.function == CML_KEEP_VALUES))
                  {
                    int rgbi[3];
                    int i;

                    for (i = 0; i < src_bpp; i++)
                      rgbi[i] = src_buffer[offset_y * src_bpl
                                           + (dx * VALS.scale + offset_x) * src_bpp + i];
                    if (src_is_gray && (VALS.val.function == CML_KEEP_VALUES))
                      {
                        b = rgbi[0];
                      }
                    else
                      {
                        gimp_rgb_to_hsv_int (rgbi, rgbi + 1, rgbi + 2);

                        r = (VALS.hue.function == CML_KEEP_VALUES) ? rgbi[0] : h;
                        g = (VALS.sat.function == CML_KEEP_VALUES) ? rgbi[1] : s;
                        b = (VALS.val.function == CML_KEEP_VALUES) ? rgbi[2] : v;
                        gimp_hsv_to_rgb_int (&r, &g, &b);
                      }
                  }

                dest_offset = (offset_y * dest_bpl +
                               (dx * VALS.scale + offset_x) * dest_bpp);

                if (dest_is_gray)
                  {
                    dest_buffer[dest_offset++] = b;
                    if (preview_p)
                      {
                        dest_buffer[dest_offset++] = b;
                        dest_buffer[dest_offset++] = b;
                      }
                  }
                else
                  {
                    dest_buffer[dest_offset++] = r;
                    dest_buffer[dest_offset++] = g;
                    dest_buffer[dest_offset++] = b;
                  }
                if (dest_has_alpha)
                  dest_buffer[dest_offset] = 255;

                if ((!preview_p) &&
                    (++processed % (total / PROGRESS_UPDATE_NUM + 1)) == 0)
                  gimp_progress_update ((gdouble) processed / (gdouble) total);
              }
        }

      if (preview_p)
        gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                                0, dy,
                                width_by_pixel, keep_height,
                                GIMP_RGB_IMAGE,
                                dest_buffer,
                                dest_bpl);
      else
        gimp_pixel_rgn_set_rect (&dest_rgn, dest_buffer, x, y + dy,
                                 width_by_pixel, keep_height);
    }
  if (preview_p)
    {
      gtk_widget_queue_draw (preview);
    }
  else
    {
      gimp_progress_update (1.0);
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id,
                            x, y, width_by_pixel, height_by_pixel);
      gimp_drawable_detach (drawable);
    }

  g_rand_free (gr);

  return GIMP_PDB_SUCCESS;
}

static void
CML_compute_next_step (gint      size,
                       gdouble **h,
                       gdouble **s,
                       gdouble **v,
                       gdouble **hn,
                       gdouble **sn,
                       gdouble **vn,
                       gdouble **haux,
                       gdouble **saux,
                       gdouble **vaux)
{
  gint  index;

  for (index = 0; index < size; index++)
    (*hn)[index] = CML_next_value (*h, index, size,
                                   (*s)[POS_IN_TORUS (index, size)],
                                   (*v)[POS_IN_TORUS (index, size)],
                                   &VALS.hue,
                                   (*haux)[POS_IN_TORUS (index , size)]);
  for (index = 0; index < size; index++)
    (*sn)[index] = CML_next_value (*s, index, size,
                                   (*v)[POS_IN_TORUS (index   , size)],
                                   (*h)[POS_IN_TORUS (index   , size)],
                                   &VALS.sat,
                                   (*saux)[POS_IN_TORUS (index , size)]);
  for (index = 0; index < size; index++)
    (*vn)[index] = CML_next_value (*v, index, size,
                                   (*h)[POS_IN_TORUS (index   , size)],
                                   (*s)[POS_IN_TORUS (index   , size)],
                                   &VALS.val,
                                   (*vaux)[POS_IN_TORUS (index , size)]);

#define GD_SWAP(x, y)   { gdouble *tmp = *x; *x = *y; *y = tmp; }
  GD_SWAP (h, hn);
  GD_SWAP (s, sn);
  GD_SWAP (v, vn);
#undef  SWAP
}

#define LOGISTICS(x)    logistic_function (param, x, power)
#define ENV_FACTOR(x)   (param->env_sensitivity * LOGISTICS (x))
#define CHN_FACTOR(x)   (param->ch_sensitivity * LOGISTICS (x))

static gdouble
CML_next_value (gdouble   *vec,
                gint       pos,
                gint       size,
                gdouble    c1,
                gdouble    c2,
                CML_PARAM *param,
                gdouble    power)
{
  gdouble val = vec[pos];
  gdouble diff = 0;
  gdouble self_diff = 0;
  gdouble by_env = 0;
  gdouble self_mod_rate = 0;
  gdouble hold_rate = 1 - param->mod_rate;
  gdouble env_factor = 0;
  gint    index;

  self_mod_rate = (1 - param->env_sensitivity - param->ch_sensitivity);

  switch (param->arrange)
    {
    case ANTILOG:
      self_diff = self_mod_rate * LOGISTICS (1 - vec[pos]);
      for (index = 1; index <= param->diffusion_dist / 2; index++)
        env_factor += ENV_FACTOR (1 - vec[POS_IN_TORUS (pos + index, size)])
          + ENV_FACTOR (1 - vec[POS_IN_TORUS (pos - index, size)]);
      if ((param->diffusion_dist % 2) == 1)
        env_factor += (ENV_FACTOR (1 - vec[POS_IN_TORUS (pos + index, size)])
                       + ENV_FACTOR (1 - vec[POS_IN_TORUS (pos - index, size)])) / 2;
      env_factor /= (gdouble) param->diffusion_dist;
      by_env = env_factor + (CHN_FACTOR (1 - c1) + CHN_FACTOR (1 - c2)) / 2;
      diff = param->mod_rate * (self_diff + by_env);
      val = hold_rate * vec[pos] + diff;
      break;
    case AVERAGE:
      self_diff = self_mod_rate * LOGISTICS (vec[pos]);
      for (index = 1; index <= param->diffusion_dist / 2; index++)
        env_factor += vec[POS_IN_TORUS (pos + index, size)] + vec[POS_IN_TORUS (pos - index, size)];
      if ((param->diffusion_dist % 2) == 1)
        env_factor += (vec[POS_IN_TORUS (pos + index, size)] + vec[POS_IN_TORUS (pos - index, size)]) / 2;
      env_factor /= (gdouble) param->diffusion_dist;
      by_env = ENV_FACTOR (env_factor) + (CHN_FACTOR (c1) + CHN_FACTOR (c2)) / 2;
      diff = param->mod_rate * (self_diff + by_env);
      val = hold_rate * vec[pos] + diff;
      break;
    case MULTIPLY_RANDOM0:
    case MULTIPLY_RANDOM1:
    case MULTIPLY_GRADIENT:
      {
        gdouble tmp;

        tmp = power;
        power = param->power;
        self_diff = self_mod_rate * LOGISTICS (vec[pos]);
        for (index = 1; index <= param->diffusion_dist / 2; index++)
          env_factor += ENV_FACTOR (vec[POS_IN_TORUS (pos + index, size)])
            + ENV_FACTOR (vec[POS_IN_TORUS (pos - index, size)]);
        if ((param->diffusion_dist % 2) == 1)
          env_factor += (ENV_FACTOR (vec[POS_IN_TORUS (pos + index, size)])
                         + ENV_FACTOR (vec[POS_IN_TORUS (pos - index, size)])) / 2;
        env_factor /= (gdouble) param->diffusion_dist;
        by_env = (env_factor + CHN_FACTOR (c1) + CHN_FACTOR (c2)) / 2;
        diff = pow (param->mod_rate * (self_diff + by_env), tmp);
        val = hold_rate * vec[pos] + diff;
        break;
      }
    case STANDARD:
    case RAND_POWER0:
    case RAND_POWER1:
    case RAND_POWER2:
    case RAND_AND_P:
    default:
      self_diff = self_mod_rate * LOGISTICS (vec[pos]);

      for (index = 1; index <= param->diffusion_dist / 2; index++)
        env_factor += ENV_FACTOR (vec[POS_IN_TORUS (pos + index, size)])
          + ENV_FACTOR (vec[POS_IN_TORUS (pos - index, size)]);
      if ((param->diffusion_dist % 2) == 1)
        env_factor += (ENV_FACTOR (vec[POS_IN_TORUS (pos + index, size)])
                       + ENV_FACTOR (vec[POS_IN_TORUS (pos - index, size)])) / 2;
      env_factor /= (gdouble) param->diffusion_dist;
      by_env = env_factor + (CHN_FACTOR (c1) + CHN_FACTOR (c2)) / 2;
      diff = param->mod_rate * (self_diff + by_env);
      val = hold_rate * vec[pos] + diff;
      break;
    }
  /* finalize */
  if (g_rand_double (gr) < param->mutation_rate)
    {
      val += ((g_rand_double (gr) < 0.5) ? -1.0 : 1.0) * param->mutation_dist * g_rand_double (gr);
    }
  if (param->cyclic_range)
    {
      if (1.0 < val)
        val = val - (int) val;
      else if (val < 0.0)
        val = val - floor (val);
    }
  else
    /* The range of val should be [0,1], not [0,1).
      Cannonization shuold be done in color mapping phase. */
    val = CLAMP (val, 0.0, 1);

  return val;
}
#undef LOGISTICS
#undef ENV_FACTOR
#undef CHN_FACTOR


static gdouble
logistic_function (CML_PARAM *param,
                   gdouble    x,
                   gdouble    power)
{
  gdouble x1 = x;
  gdouble result = 0;
  gint n = param->range_num;
  gint step;

  step = (int) (x * (gdouble) n);
  x1 = (x - ((gdouble) step / (gdouble) n)) * n;
  switch (param->function)
    {
    case CML_KEEP_VALUES:
    case CML_KEEP_FIRST:
      result = x;
      return result;
      break;
    case CML_FILL:
      result = CLAMP (param->parameter_k, 0.0, 1.0);
      return result;
      break;
    case CML_LOGIST:
      result = param->parameter_k * pow (4 * x1 * (1.0 - x1), power);
      break;
    case CML_LOGIST_STEP:
      result = param->parameter_k * pow (4 * x1 * (1.0 - x1), power);
      result = (result + step) / (gdouble) n;
      break;
    case CML_POWER:
      result = param->parameter_k * pow (x1, power);
      break;
    case CML_POWER_STEP:
      result = param->parameter_k * pow (x1, power);
      result = (result + step) / (gdouble) n;
      break;
    case CML_REV_POWER:
      result = param->parameter_k * (1 - pow (x1, power));
      break;
    case CML_REV_POWER_STEP:
      result = param->parameter_k * (1 - pow (x1, power));
      result = (result + step) / (gdouble) n;
      break;
    case CML_DELTA:
      result = param->parameter_k * 2 * ((x1 < 0.5) ? x1 : (1.0 - x1));
      break;
    case CML_DELTA_STEP:
      result = param->parameter_k * 2 * ((x1 < 0.5) ? x1 : (1.0 - x1));
      result = (result + step) / (gdouble) n;
      break;
    case CML_SIN_CURVE:
      if (1.0 < power)
        result = 0.5 * (sin (G_PI * ABS (x1 - 0.5) / power) / sin (G_PI * 0.5 / power) + 1);
      else
        result = 0.5 * (pow (sin (G_PI * ABS (x1 - 0.5)), power) + 1);
      if (x1 < 0.5) result = 1 - result;
      break;
    case CML_SIN_CURVE_STEP:
      if (1.0 < power)
        result = 0.5 * (sin (G_PI * ABS (x1 - 0.5) / power) / sin (G_PI * 0.5 / power) + 1);
      else
        result = 0.5 * (pow (sin (G_PI * ABS (x1 - 0.5)), power) + 1);
      if (x1 < 0.5) result = 1 - result;
      result = (result + step) / (gdouble) n;
      break;
    }
  switch (param->composition)
    {
    case COMP_NONE:
      break;
    case COMP_MAX_LINEAR:
      result = MAX ((gdouble) x, (gdouble) result);
      break;
    case COMP_MAX_LINEAR_P1:
      result = MAX ((gdouble) x + (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    case COMP_MAX_LINEAR_P1L:
      if (x < 0.5)
        result = MAX ((gdouble) x + (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    case COMP_MAX_LINEAR_P1U:
      if (0.5 < x)
        result = MAX ((gdouble) x + (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    case COMP_MAX_LINEAR_M1:
      result = MAX ((gdouble) x - (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    case COMP_MAX_LINEAR_M1L:
      if (x < 0.5)
        result = MAX ((gdouble) x - (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    case COMP_MAX_LINEAR_M1U:
      if (0.5 < x)
        result = MAX ((gdouble) x - (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    case COMP_MIN_LINEAR:
      result = MIN ((gdouble) x, (gdouble) result);
      break;
    case COMP_MIN_LINEAR_P1:
      result = MIN ((gdouble) x + (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    case COMP_MIN_LINEAR_P1L:
      if (x < 0.5)
        result = MIN ((gdouble) x + (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    case COMP_MIN_LINEAR_P1U:
      if (0.5 < x)
        result = MIN ((gdouble) x + (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    case COMP_MIN_LINEAR_M1:
      result = MIN ((gdouble) x - (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    case COMP_MIN_LINEAR_M1L:
      if (x < 0.5)
        result = MIN ((gdouble) x - (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    case COMP_MIN_LINEAR_M1U:
      if (0.5 < x)
        result = MIN ((gdouble) x - (gdouble) 1 / (gdouble) 256, (gdouble) result);
      break;
    }
  return result;
}

/* dialog stuff */
static gint
CML_explorer_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *abox;
  GtkWidget *bbox;
  GtkWidget *button;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Coupled-Map-Lattice Explorer"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  CML_preview_defer = TRUE;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);

  preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview,
                               PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  bbox = gtk_button_box_new (GTK_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);

  button = gtk_button_new_with_label (_("New Seed"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (CML_preview_update_callback),
                    &VALS);

  random_sensitives[0].widget = button;
  random_sensitives[0].logic  = TRUE;

  button = gtk_button_new_with_label (_("Fix Seed"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (CML_set_or_randomize_seed_callback),
                    &VALS);

  random_sensitives[1].widget = button;
  random_sensitives[1].logic  = TRUE;

  button = gtk_button_new_with_label (_("Random Seed"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (CML_set_or_randomize_seed_callback),
                    &VALS);

  random_sensitives[2].widget = button;
  random_sensitives[2].logic  = FALSE;

  bbox = gtk_button_box_new (GTK_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);

  button = gtk_button_new_with_mnemonic (_("_Open"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (CML_load_from_file_callback),
                    &VALS);

  button = gtk_button_new_with_mnemonic (_("_Save"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (CML_save_to_file_callback),
                    &VALS);

  {
    GtkWidget *notebook;
    GtkWidget *page;

    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_box_pack_start (GTK_BOX (hbox), notebook, TRUE, TRUE, 0);
    gtk_widget_show (notebook);

    page = CML_dialog_channel_panel_new (&VALS.hue, 0);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
                              gtk_label_new_with_mnemonic (_("_Hue")));

    page = CML_dialog_channel_panel_new (&VALS.sat, 1);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
                              gtk_label_new_with_mnemonic (_("Sat_uration")));

    page = CML_dialog_channel_panel_new (&VALS.val, 2);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
                              gtk_label_new_with_mnemonic (_("_Value")));

    page = CML_dialog_advanced_panel_new ();
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
                              gtk_label_new_with_mnemonic (_("_Advanced")));

    {
      GtkSizeGroup  *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
      GtkWidget     *grid;
      GtkWidget     *label;
      GtkWidget     *combo;
      GtkWidget     *frame;
      GtkWidget     *vbox;
      GtkAdjustment *adj;

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
      gtk_widget_show (vbox);

      frame = gimp_frame_new (_("Channel Independent Parameters"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);

      combo = gimp_int_combo_box_new_array (CML_INITIAL_NUM_VALUES,
                                            initial_value_names);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                     VALS.initial_value);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (CML_initial_value_menu_update),
                        &VALS.initial_value);

      CML_explorer_menu_entry_init (&widget_pointers[3][0],
                                    combo, &VALS.initial_value);
      label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                        _("Initial value:"), 0.0, 0.5,
                                        combo, 2);
      gtk_size_group_add_widget (group, label);
      g_object_unref (group);

      adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 1,
                                       _("Zoom scale:"), SCALE_WIDTH, 3,
                                       VALS.scale, 1, 10, 1, 2, 0,
                                       TRUE, 0, 0,
                                       NULL, NULL);
      gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));
      CML_explorer_int_entry_init (&widget_pointers[3][1],
                                   adj, &VALS.scale);

      adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 2,
                                       _("Start offset:"), SCALE_WIDTH, 3,
                                       VALS.start_offset, 0, 100, 1, 10, 0,
                                       TRUE, 0, 0,
                                       NULL, NULL);
      gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));
      CML_explorer_int_entry_init (&widget_pointers[3][2],
                                   adj, &VALS.start_offset);

      frame =
        gimp_frame_new (_("Seed of Random (only for \"From Seed\" Modes)"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);

      adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 0,
                                       _("Seed:"), SCALE_WIDTH, 0,
                                       VALS.seed, 0, (guint32) -1, 1, 10, 0,
                                       TRUE, 0, 0,
                                       NULL, NULL);
      gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));
      CML_explorer_int_entry_init (&widget_pointers[3][3],
                                   adj, &VALS.seed);

      random_sensitives[3].widget = grid;
      random_sensitives[3].logic  = FALSE;

      button =
        gtk_button_new_with_label
        (_("Switch to \"From seed\" With the Last Seed"));
      gtk_grid_attach (GTK_GRID (grid), button, 0, 1, 3, 1);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (CML_set_or_randomize_seed_callback),
                        &VALS);

      random_sensitives[4].widget = button;
      random_sensitives[4].logic  = TRUE;

      gimp_help_set_help_data (button,
                               _("\"Fix seed\" button is an alias of me.\n"
                                 "The same seed produces the same image, "
                                 "if (1) the widths of images are same "
                                 "(this is the reason why image on drawable "
                                 "is different from preview), and (2) all "
                                 "mutation rates equal to zero."), NULL);

      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                                gtk_label_new_with_mnemonic (_("O_thers")));
    }

    {
      GtkSizeGroup *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
      GtkWidget    *grid;
      GtkWidget    *frame;
      GtkWidget    *label;
      GtkWidget    *combo;
      GtkWidget    *vbox;

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
      gtk_widget_show (vbox);

      frame = gimp_frame_new (_("Copy Settings"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);

      combo = gimp_int_combo_box_new_array (G_N_ELEMENTS (channel_names),
                                            channel_names);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), copy_source);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gimp_int_combo_box_get_active),
                        &copy_source);

      label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                        _("Source channel:"), 0.0, 0.5,
                                        combo, 1);
      gtk_size_group_add_widget (group, label);
      g_object_unref (group);

      combo = gimp_int_combo_box_new_array (G_N_ELEMENTS (channel_names),
                                            channel_names);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                     copy_destination);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gimp_int_combo_box_get_active),
                        &copy_destination);

      label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                        _("Destination channel:"), 0.0, 0.5,
                                        combo, 1);
      gtk_size_group_add_widget (group, label);

      button = gtk_button_new_with_label (_("Copy Parameters"));
      gtk_grid_attach (GTK_GRID (grid), button, 0, 2, 2, 1);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (CML_copy_parameters_callback),
                        &VALS);

      frame = gimp_frame_new (_("Selective Load Settings"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);

      combo = gimp_int_combo_box_new_array (G_N_ELEMENTS (load_channel_names),
                                            load_channel_names);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                     selective_load_source);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gimp_int_combo_box_get_active),
                        &selective_load_source);

      label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                        _("Source channel in file:"),
                                        0.0, 0.5,
                                        combo, 1);
      gtk_size_group_add_widget (group, label);

      combo = gimp_int_combo_box_new_array (G_N_ELEMENTS (load_channel_names),
                                            load_channel_names);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                     selective_load_destination);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gimp_int_combo_box_get_active),
                        &selective_load_destination);

      label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                        _("Destination channel:"),
                                        0.0, 0.5,
                                        combo, 1);
      gtk_size_group_add_widget (group, label);

      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                                gtk_label_new_with_mnemonic (_("_Misc Ops.")));
    }
  }

  CML_initial_value_sensitives_update ();

  gtk_widget_show (dialog);

  img_stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, GRAPHSIZE);
  img = g_malloc0 (img_stride * GRAPHSIZE);

  buffer = cairo_image_surface_create_for_data (img, CAIRO_FORMAT_RGB24,
                                                GRAPHSIZE,
                                                GRAPHSIZE,
                                                img_stride);

  /*  Displaying preview might takes a long time. Thus, first, dialog itself
   *  should be shown before making preview in it.
   */
  CML_preview_defer = FALSE;
  preview_update ();

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);
  g_free (img);
  cairo_surface_destroy (buffer);

  return run;
}

static GtkWidget *
CML_dialog_channel_panel_new (CML_PARAM *param,
                              gint       channel_id)
{
  GtkWidget     *grid;
  GtkWidget     *combo;
  GtkWidget     *toggle;
  GtkWidget     *button;
  GtkAdjustment *adj;
  gpointer      *chank;
  gint           index = 0;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_widget_show (grid);

  combo = gimp_int_combo_box_new_array (CML_NUM_VALUES, function_names);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), param->function);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (CML_explorer_menu_update),
                    &param->function);

  CML_explorer_menu_entry_init (&widget_pointers[channel_id][index],
                                combo, &param->function);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, index,
                            _("Function type:"), 0.0, 0.5,
                            combo, 2);
  index++;

  combo = gimp_int_combo_box_new_array (COMP_NUM_VALUES, composition_names);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 param->composition);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (CML_explorer_menu_update),
                    &param->composition);

  CML_explorer_menu_entry_init (&widget_pointers[channel_id][index],
                                combo, &param->composition);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, index,
                            _("Composition:"), 0.0, 0.5,
                            combo, 2);
  index++;

  combo = gimp_int_combo_box_new_array (ARRANGE_NUM_VALUES, arrange_names);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), param->arrange);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (CML_explorer_menu_update),
                    &param->arrange);

  CML_explorer_menu_entry_init (&widget_pointers[channel_id][index],
                                combo, &param->arrange);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, index,
                            _("Misc arrange:"), 0.0, 0.5,
                            combo, 2);
  index++;

  toggle = gtk_check_button_new_with_label (_("Use cyclic range"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                param->cyclic_range);
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, index, 3, 1);
  CML_explorer_toggle_entry_init (&widget_pointers[channel_id][index],
                                  toggle, &param->cyclic_range);
  gtk_widget_show (toggle);
  index++;

  adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, index,
                                   _("Mod. rate:"), SCALE_WIDTH, 5,
                                   param->mod_rate, 0.0, 1.0, 0.01, 0.1, 2,
                                   TRUE, 0, 0,
                                   NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  adj, &param->mod_rate);
  index++;

  adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, index,
                                   _("Env. sensitivity:"), SCALE_WIDTH, 5,
                                   param->env_sensitivity, 0.0, 1.0, 0.01, 0.1, 2,
                                   TRUE, 0, 0,
                                   NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  adj, &param->env_sensitivity);
  index++;

  adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, index,
                                   _("Diffusion dist.:"), SCALE_WIDTH, 5,
                                   param->diffusion_dist, 2, 10, 1, 2, 0,
                                   TRUE, 0, 0,
                                   NULL, NULL);
  CML_explorer_int_entry_init (&widget_pointers[channel_id][index],
                               adj, &param->diffusion_dist);
  index++;

  adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, index,
                                   _("# of subranges:"), SCALE_WIDTH, 5,
                                   param->range_num, 1, 10, 1, 2, 0,
                                   TRUE, 0, 0,
                                   NULL, NULL);
  CML_explorer_int_entry_init (&widget_pointers[channel_id][index],
                               adj, &param->range_num);
  index++;

  adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, index,
                                   _("P(ower factor):"), SCALE_WIDTH, 5,
                                   param->power, 0.0, 10.0, 0.1, 1.0, 2,
                                   TRUE, 0, 0,
                                   NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  adj, &param->power);
  index++;

  adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, index,
                                   _("Parameter k:"), SCALE_WIDTH, 5,
                                   param->parameter_k, 0.0, 10.0, 0.1, 1.0, 2,
                                   TRUE, 0, 0,
                                   NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  adj, &param->parameter_k);
  index++;

  adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, index,
                                   _("Range low:"), SCALE_WIDTH, 5,
                                   param->range_l, 0.0, 1.0, 0.01, 0.1, 2,
                                   TRUE, 0, 0,
                                   NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  adj, &param->range_l);
  index++;

  adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, index,
                                   _("Range high:"), SCALE_WIDTH, 5,
                                   param->range_h, 0.0, 1.0, 0.01, 0.1, 2,
                                   TRUE, 0, 0,
                                   NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  adj, &param->range_h);
  index++;

  chank = g_new (gpointer, 2);
  chank[0] = GINT_TO_POINTER (channel_id);
  chank[1] = param;

  button = gtk_button_new_with_label (_("Plot a Graph of the Settings"));
  gtk_grid_attach (GTK_GRID (grid), button, 0, index, 3, 1);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (function_graph_new),
                    chank);
  return grid;
}

static GtkWidget *
CML_dialog_advanced_panel_new (void)
{
  GtkWidget     *vbox;
  GtkWidget     *subframe;
  GtkWidget     *grid;
  GtkAdjustment *adj;
  gint           index = 0;
  gint           widget_offset = 12;
  gint           channel_id;
  CML_PARAM     *param;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_widget_show (vbox);

  for (channel_id = 0; channel_id < 3; channel_id++)
    {
      param = (CML_PARAM *)&VALS + channel_id;

      subframe = gimp_frame_new (gettext (channel_names[channel_id]));
      gtk_box_pack_start (GTK_BOX (vbox), subframe, FALSE, FALSE, 0);
      gtk_widget_show (subframe);

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_container_add (GTK_CONTAINER (subframe), grid);
      gtk_widget_show (grid);

      index = 0;

      adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, index,
                                       _("Ch. sensitivity:"), SCALE_WIDTH, 0,
                                       param->ch_sensitivity, 0.0, 1.0, 0.01, 0.1, 2,
                                       TRUE, 0, 0,
                                       NULL, NULL);
      CML_explorer_double_entry_init (&widget_pointers[channel_id][index +
                                                                  widget_offset],
                                      adj, &param->ch_sensitivity);
      index++;

      adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, index,
                                       _("Mutation rate:"), SCALE_WIDTH, 0,
                                       param->mutation_rate, 0.0, 1.0, 0.01, 0.1, 2,
                                       TRUE, 0, 0,
                                       NULL, NULL);
      CML_explorer_double_entry_init (&widget_pointers[channel_id][index +
                                                                  widget_offset],
                                      adj, &param->mutation_rate);
      index++;

      adj = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, index,
                                       _("Mutation dist.:"), SCALE_WIDTH, 0,
                                       param->mutation_dist, 0.0, 1.0, 0.01, 0.1, 2,
                                       TRUE, 0, 0,
                                       NULL, NULL);
      CML_explorer_double_entry_init (&widget_pointers[channel_id][index +
                                                                  widget_offset],
                                      adj, &param->mutation_dist);
    }
  return vbox;
}

static void
preview_update (void)
{
  if (! CML_preview_defer)
    CML_main_function (TRUE);
}

static gboolean
function_graph_draw (GtkWidget *widget,
                     cairo_t   *cr,
                     gpointer  *data)
{
  gint       x, y;
  gint       rgbi[3];
  gint       channel_id = GPOINTER_TO_INT (data[0]);
  CML_PARAM *param      = data[1];

  cairo_set_line_width (cr, 1.0);

  cairo_surface_flush (buffer);

  for (x = 0; x < GRAPHSIZE; x++)
    {
      /* hue */
      rgbi[0] = rgbi[1] = rgbi[2] = 127;
      if ((0 <= channel_id) && (channel_id <= 2))
        rgbi[channel_id] = CANNONIZE ((*param), ((gdouble) x / (gdouble) 255));
      gimp_hsv_to_rgb_int (rgbi, rgbi+1, rgbi+2);
      for (y = 0; y < GRAPHSIZE; y++)
      {
        GIMP_CAIRO_RGB24_SET_PIXEL((img+(y*img_stride+x*4)),
                                   rgbi[0],
                                   rgbi[1],
                                   rgbi[2]);
      }
    }

  cairo_surface_mark_dirty (buffer);

  cairo_set_source_surface (cr, buffer, 0.0, 0.0);

  cairo_paint (cr);
  cairo_translate (cr, 0.5, 0.5);

  cairo_move_to (cr, 0, 255);
  cairo_line_to (cr, 255, 0);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_stroke (cr);

  y = 255 * CLAMP (logistic_function (param, 0, param->power),
                     0, 1.0);
  cairo_move_to (cr, 0, 255-y);
  for (x = 0; x < GRAPHSIZE; x++)
    {
      /* curve */
      y = 255 * CLAMP (logistic_function (param, x/(gdouble)255, param->power),
                       0, 1.0);
      cairo_line_to (cr, x, 255-y);
    }

  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_stroke (cr);

  return TRUE;
}

static void
function_graph_new (GtkWidget *widget,
                    gpointer  *data)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  GtkWidget *preview;

  dialog = gimp_dialog_new (_("Graph of the Current Settings"), PLUG_IN_ROLE,
                            gtk_widget_get_toplevel (widget), 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Close"), GTK_RESPONSE_CLOSE,

                            NULL);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  preview = gtk_drawing_area_new ();
  gtk_widget_set_size_request (preview, GRAPHSIZE, GRAPHSIZE);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);
  g_signal_connect (preview, "draw",
                    G_CALLBACK (function_graph_draw),
                    data);

  gtk_widget_show (dialog);

  gimp_dialog_run (GIMP_DIALOG (dialog));

  gtk_widget_destroy (dialog);
}

static void
CML_set_or_randomize_seed_callback (GtkWidget *widget,
                                    gpointer   data)
{
  CML_preview_defer = TRUE;

  switch (VALS.initial_value)
    {
    case CML_INITIAL_RANDOM_INDEPENDENT:
      VALS.initial_value = CML_INITIAL_RANDOM_FROM_SEED;
      break;
    case CML_INITIAL_RANDOM_SHARED:
      VALS.initial_value = CML_INITIAL_RANDOM_FROM_SEED_SHARED;
      break;
    case CML_INITIAL_RANDOM_FROM_SEED:
      VALS.initial_value = CML_INITIAL_RANDOM_INDEPENDENT;
      break;
    case CML_INITIAL_RANDOM_FROM_SEED_SHARED:
      VALS.initial_value = CML_INITIAL_RANDOM_SHARED;
      break;
    default:
      break;
    }
  if (widget_pointers[3][3].widget && widget_pointers[3][3].updater)
    (widget_pointers[3][3].updater) (widget_pointers[3]+3);
  if (widget_pointers[3][0].widget && widget_pointers[3][0].updater)
    (widget_pointers[3][0].updater) (widget_pointers[3]);

  CML_initial_value_sensitives_update ();

  CML_preview_defer = FALSE;
}

static void
CML_copy_parameters_callback (GtkWidget *widget,
                              gpointer   data)
{
  gint index;
  WidgetEntry *widgets;

  if (copy_source == copy_destination)
    {
      g_message (_("Warning: the source and the destination are the same channel."));
      return;
    }
  *channel_params[copy_destination] = *channel_params[copy_source];
  CML_preview_defer = TRUE;
  widgets = widget_pointers[copy_destination];

  for (index = 0; index < CML_PARAM_NUM; index++)
    if (widgets[index].widget && widgets[index].updater)
      (widgets[index].updater) (widgets + index);

  CML_preview_defer = FALSE;
  preview_update ();
}

static void
CML_initial_value_sensitives_update (void)
{
  gint  i = 0;
  gint  flag1, flag2;

  flag1 = (CML_INITIAL_RANDOM_INDEPENDENT <= VALS.initial_value)
    & (VALS.initial_value <= CML_INITIAL_RANDOM_FROM_SEED_SHARED);
  flag2 = (CML_INITIAL_RANDOM_INDEPENDENT <= VALS.initial_value)
    & (VALS.initial_value <= CML_INITIAL_RANDOM_SHARED);

  for (; i < G_N_ELEMENTS (random_sensitives) ; i++)
    if (random_sensitives[i].widget)
      gtk_widget_set_sensitive (random_sensitives[i].widget,
                                flag1 & (random_sensitives[i].logic == flag2));
}

static void
CML_preview_update_callback (GtkWidget *widget,
                             gpointer   data)
{
  WidgetEntry seed_widget = widget_pointers[3][3];

  preview_update ();

  CML_preview_defer = TRUE;

  if (seed_widget.widget && seed_widget.updater)
    seed_widget.updater (&seed_widget);

  CML_preview_defer = FALSE;
}

/*  parameter file saving functions  */

static void
CML_save_to_file_callback (GtkWidget *widget,
                           gpointer   data)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Save CML Explorer Parameters"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (widget)),
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Save"),   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                      TRUE);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (CML_save_to_file_response),
                        NULL);
      g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (gtk_true),
                        NULL);
    }

  if (strlen (VALS.last_file_name))
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog),
                                   VALS.last_file_name);

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
CML_save_to_file_response (GtkWidget *dialog,
                           gint       response_id,
                           gpointer   data)
{
  gchar *filename;
  FILE  *file;
  gint   channel_id;

  if (response_id != GTK_RESPONSE_OK)
    {
      gtk_widget_hide (dialog);
      return;
    }

  filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
  if (! filename)
    return;

  file = g_fopen (filename, "wb");

  if (! file)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      g_free (filename);
      return;
    }

  fprintf (file, "; This is a parameter file for CML_explorer\n");
  fprintf (file, "; File format version: %1.1f\n", PARAM_FILE_FORMAT_VERSION);
  fprintf (file, ";\n");

  for (channel_id = 0; channel_id < 3; channel_id++)
    {
      gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

      CML_PARAM param = *(CML_PARAM *)(channel_params[channel_id]);

      fprintf (file, "\t%s\n", channel_names[channel_id]);
      fprintf (file, "Function_type    : %d (%s)\n",
               param.function, function_names[param.function]);
      fprintf (file, "Compostion_type  : %d (%s)\n",
               param.composition, composition_names[param.composition]);
      fprintf (file, "Arrange          : %d (%s)\n",
               param.arrange, arrange_names[param.arrange]);
      fprintf (file, "Cyclic_range     : %d (%s)\n",
               param.cyclic_range, (param.cyclic_range ? "TRUE" : "FALSE"));
      fprintf (file, "Mod. rate        : %s\n",
               g_ascii_dtostr (buf, sizeof (buf), param.mod_rate));
      fprintf (file, "Env_sensitivtiy  : %s\n",
               g_ascii_dtostr (buf, sizeof (buf), param.env_sensitivity));
      fprintf (file, "Diffusion dist.  : %d\n", param.diffusion_dist);
      fprintf (file, "Ch. sensitivity  : %s\n",
               g_ascii_dtostr (buf, sizeof (buf), param.ch_sensitivity));
      fprintf (file, "Num. of Subranges: %d\n", param.range_num);
      fprintf (file, "Power_factor     : %s\n",
               g_ascii_dtostr (buf, sizeof (buf), param.power));
      fprintf (file, "Parameter_k      : %s\n",
               g_ascii_dtostr (buf, sizeof (buf), param.parameter_k));
      fprintf (file, "Range_low        : %s\n",
               g_ascii_dtostr (buf, sizeof (buf), param.range_l));
      fprintf (file, "Range_high       : %s\n",
               g_ascii_dtostr (buf, sizeof (buf), param.range_h));
      fprintf (file, "Mutation_rate    : %s\n",
               g_ascii_dtostr (buf, sizeof (buf), param.mutation_rate));
      fprintf (file, "Mutation_distance: %s\n",
               g_ascii_dtostr (buf, sizeof (buf), param.mutation_dist));
    }

  fprintf (file, "\n");
  fprintf (file, "Initial value  : %d (%s)\n",
           VALS.initial_value, initial_value_names[VALS.initial_value]);
  fprintf (file, "Zoom scale     : %d\n", VALS.scale);
  fprintf (file, "Start offset   : %d\n", VALS.start_offset);
  fprintf (file, "Random seed    : %d\n", VALS.seed);
  fclose(file);

  g_message (_("Parameters were saved to '%s'"),
             gimp_filename_to_utf8 (filename));

  strncpy (VALS.last_file_name, filename,
           sizeof (VALS.last_file_name) - 1);

  g_free (filename);

  gtk_widget_hide (dialog);
}

/*  parameter file loading functions  */

static void
CML_load_from_file_callback (GtkWidget *widget,
                             gpointer   data)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Load CML Explorer Parameters"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (widget)),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Open"),   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (CML_load_from_file_response),
                        NULL);
      g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (gtk_true),
                        NULL);
    }

  if (strlen (VALS.last_file_name) > 0)
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog),
                                   VALS.last_file_name);

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
CML_load_from_file_response (GtkWidget *dialog,
                             gint       response_id,
                             gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar    *filename;
      gint      channel_id;
      gboolean  flag = TRUE;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      gtk_widget_set_sensitive (dialog, FALSE);
      flag = CML_load_parameter_file (filename, TRUE);

      g_free (filename);

      if (flag)
        {
          WidgetEntry *widgets;
          gint         index;

          CML_preview_defer = TRUE;

          for (channel_id = 0; channel_id < 3; channel_id++)
            {
              widgets = widget_pointers[channel_id];
              for (index = 0; index < CML_PARAM_NUM; index++)
                if (widgets[index].widget && widgets[index].updater)
                  (widgets[index].updater) (widgets + index);
            }
          /* channel independent parameters */
          widgets = widget_pointers[3];
          for (index = 0; index < 4; index++)
            if (widgets[index].widget && widgets[index].updater)
              (widgets[index].updater) (widgets + index);

          CML_preview_defer = FALSE;

          preview_update ();
        }
    }

  gtk_widget_hide (GTK_WIDGET (dialog));
}

static gboolean
CML_load_parameter_file (const gchar *filename,
                         gboolean     interactive_mode)
{
  FILE      *file;
  gint       channel_id;
  gboolean   flag = TRUE;
  CML_PARAM  ch[3];
  gint       initial_value = 0;
  gint       scale = 1;
  gint       start_offset = 0;
  gint       seed = 0;
  gint       old2new_function_id[] = { 3, 4, 5, 6, 7, 9, 10, 11, 1, 2 };

  file = g_fopen (filename, "rb");

  if (!file)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }
  else
    {
      gchar line[CML_LINE_SIZE];
      gdouble version = 0.99;

      version = parse_line_to_gdouble (file, &flag); /* old format returns 1 */
      if (version == 1.0)
        version = 0.99;
      else if (! flag)
        {
          flag = TRUE;
          version = parse_line_to_gdouble (file, &flag); /* maybe new format */
          if (flag)
            fgets (line, CML_LINE_SIZE - 1, file); /* one more comment line */
        }
      if (version == 0)
        {
          if (interactive_mode)
            gimp_message (_("Error: it's not CML parameter file."));
          fclose(file);
          return FALSE;
        }
      if (interactive_mode)
        {
          if (version < PARAM_FILE_FORMAT_VERSION)
            g_message (_("Warning: '%s' is an old format file."),
                       gimp_filename_to_utf8 (filename));

          if (PARAM_FILE_FORMAT_VERSION < version)
            g_message (_("Warning: '%s' is a parameter file for a newer "
                         "version of CML Explorer."),
                       gimp_filename_to_utf8 (filename));
        }
      for (channel_id = 0; flag && (channel_id < 3); channel_id++)
        {
          /* patched by Tim Mooney <mooney@dogbert.cc.ndsu.NoDak.edu> */
          if (fgets (line, CML_LINE_SIZE - 1, file) == NULL) /* skip channel name */
            {
              flag = FALSE;
              break;
            }
          ch[channel_id].function = parse_line_to_gint (file, &flag);
          if (version < 1.0)
            ch[channel_id].function = old2new_function_id [ch[channel_id].function];
          if (1.0 <= version)
            ch[channel_id].composition = parse_line_to_gint (file, &flag);
          else
            ch[channel_id].composition = COMP_NONE;
          ch[channel_id].arrange = parse_line_to_gint (file, &flag);
          ch[channel_id].cyclic_range = parse_line_to_gint (file, &flag);
          ch[channel_id].mod_rate = parse_line_to_gdouble (file, &flag);
          ch[channel_id].env_sensitivity = parse_line_to_gdouble (file, &flag);
          if (1.0 <= version)
            ch[channel_id].diffusion_dist = parse_line_to_gint (file, &flag);
          else
            ch[channel_id].diffusion_dist = 2;
          ch[channel_id].ch_sensitivity = parse_line_to_gdouble (file, &flag);
          ch[channel_id].range_num = parse_line_to_gint (file, &flag);
          ch[channel_id].power = parse_line_to_gdouble (file, &flag);
          ch[channel_id].parameter_k = parse_line_to_gdouble (file, &flag);
          ch[channel_id].range_l = parse_line_to_gdouble (file, &flag);
          ch[channel_id].range_h = parse_line_to_gdouble (file, &flag);
          ch[channel_id].mutation_rate = parse_line_to_gdouble (file, &flag);
          ch[channel_id].mutation_dist = parse_line_to_gdouble (file, &flag);
        }
      if (flag)
        {
          gint dummy;

          if (fgets (line, CML_LINE_SIZE - 1, file) == NULL) /* skip a line */
            dummy = 1;
          else
            {
              initial_value = parse_line_to_gint (file, &dummy);
              scale = parse_line_to_gint (file, &dummy);
              start_offset = parse_line_to_gint (file, &dummy);
              seed = parse_line_to_gint (file, &dummy);
            }
          if (! dummy)
            {
              initial_value = 0;
              scale = 1;
              start_offset = 0;
              seed = 0;
            }
        }
      fclose (file);
    }

  if (! flag)
    {
      if (interactive_mode)
        gimp_message (_("Error: failed to load parameters"));
    }
  else
    {
      if ((selective_load_source == 0) || (selective_load_destination == 0))
        {
          VALS.hue = ch[0];
          VALS.sat = ch[1];
          VALS.val = ch[2];

          VALS.initial_value = initial_value;
          VALS.scale = scale;
          VALS.start_offset = start_offset;
          VALS.seed = seed;
        }
      else
        {
          memcpy ((CML_PARAM *)&VALS + (selective_load_destination - 1),
                  (void *)&ch[selective_load_source - 1],
                  sizeof (CML_PARAM));
        }

      strncpy (VALS.last_file_name, filename,
               sizeof (VALS.last_file_name) - 1);
    }
  return flag;
}

static gint
parse_line_to_gint (FILE     *file,
                    gboolean *flag)
{
  gchar  line[CML_LINE_SIZE];
  gchar *str;

  if (! *flag)
    return 0;
  if (fgets (line, CML_LINE_SIZE - 1, file) == NULL)
    {
      *flag = FALSE;            /* set FALSE if fail to parse */
      return 0;
    }
  str = &line[0];
  while (*str != ':')
    if (*str == '\000')
      {
        *flag = FALSE;
        return 0;
      }
    else
      {
        str++;
      }

  return atoi (str + 1);
}

static gdouble
parse_line_to_gdouble (FILE     *file,
                       gboolean *flag)
{
  gchar    line[CML_LINE_SIZE];
  gchar   *str;

  if (! *flag)
    return 0.0;

  if (fgets (line, CML_LINE_SIZE - 1, file) == NULL)
    {
      *flag = FALSE;            /* set FALSE if fail to parse */
      return 0.0;
    }
  str = &line[0];
  while (*str != ':')
    if (*str == '\000')
      {
        *flag = FALSE;
        return 0.0;
      }
    else
      {
        str++;
      }

  return g_ascii_strtod (str + 1, NULL);
}


/*  toggle button functions  */

static void
CML_explorer_toggle_update (GtkWidget *widget,
                            gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  preview_update ();
}

static void
CML_explorer_toggle_entry_change_value (WidgetEntry *widget_entry)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget_entry->widget),
                                *(gint *) (widget_entry->value));
}

static void
CML_explorer_toggle_entry_init (WidgetEntry *widget_entry,
                                GtkWidget   *widget,
                                gpointer     value_ptr)
{
  g_signal_connect (widget, "toggled",
                    G_CALLBACK (CML_explorer_toggle_update),
                    value_ptr);

  widget_entry->widget  = widget;
  widget_entry->value   = value_ptr;
  widget_entry->updater = CML_explorer_toggle_entry_change_value;
}

/*  int adjustment functions  */

static void
CML_explorer_int_adjustment_update (GtkAdjustment *adjustment,
                                    gpointer       data)
{
  gimp_int_adjustment_update (adjustment, data);

  preview_update ();
}

static void
CML_explorer_int_entry_change_value (WidgetEntry *widget_entry)
{
  GtkAdjustment *adjustment = (GtkAdjustment *) (widget_entry->widget);

  gtk_adjustment_set_value (adjustment, *(gint *) (widget_entry->value));
}

static void
CML_explorer_int_entry_init (WidgetEntry   *widget_entry,
                             GtkAdjustment *adjustment,
                             gpointer       value_ptr)
{
  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (CML_explorer_int_adjustment_update),
                    value_ptr);

  widget_entry->widget  = (GtkWidget *) adjustment;
  widget_entry->value   = value_ptr;
  widget_entry->updater = CML_explorer_int_entry_change_value;
}

/*  double adjustment functions  */

static void
CML_explorer_double_adjustment_update (GtkAdjustment *adjustment,
                                       gpointer       data)
{
  gimp_double_adjustment_update (adjustment, data);

  preview_update ();
}

static void
CML_explorer_double_entry_change_value (WidgetEntry *widget_entry)
{
  GtkAdjustment *adjustment = (GtkAdjustment *) (widget_entry->widget);

  gtk_adjustment_set_value (adjustment, *(gdouble *) (widget_entry->value));
}

static void
CML_explorer_double_entry_init (WidgetEntry   *widget_entry,
                                GtkAdjustment *adjustment,
                                gpointer       value_ptr)
{
  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (CML_explorer_double_adjustment_update),
                    value_ptr);

  widget_entry->widget  = (GtkWidget *) adjustment;
  widget_entry->value   = value_ptr;
  widget_entry->updater = CML_explorer_double_entry_change_value;
}

/*  menu functions  */

static void
CML_explorer_menu_update (GtkWidget *widget,
                          gpointer   data)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), (gint *) data);

  preview_update ();
}

static void
CML_initial_value_menu_update (GtkWidget *widget,
                               gpointer   data)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), (gint *) data);

  CML_initial_value_sensitives_update ();
  preview_update ();
}

static void
CML_explorer_menu_entry_change_value (WidgetEntry *widget_entry)
{
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (widget_entry->widget),
                                 *(gint *) (widget_entry->value));
}

static void
CML_explorer_menu_entry_init (WidgetEntry *widget_entry,
                              GtkWidget   *widget,
                              gpointer     value_ptr)
{
  widget_entry->widget  = widget;
  widget_entry->value   = value_ptr;
  widget_entry->updater = CML_explorer_menu_entry_change_value;
}
