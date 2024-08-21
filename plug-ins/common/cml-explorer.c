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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Comment:
 *  CML is the abbreviation of Coupled-Map Lattice that is a model of
 *  complex systems, proposed by a physicist[1,2].
 *
 *  Similar models are summaried as follows:
 *
 *                      Value    Time     Space
 *  Coupled-Map Lattice cont.    discrete discrete
 *  Cellular Automata   discrete discrete discrete
 *  Differential Eq.    cont.    cont.    cont.
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


typedef struct _Explorer      Explorer;
typedef struct _ExplorerClass ExplorerClass;

struct _Explorer
{
  GimpPlugIn parent_instance;
};

struct _ExplorerClass
{
  GimpPlugInClass parent_class;
};


#define EXPLORER_TYPE  (explorer_get_type ())
#define EXPLORER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXPLORER_TYPE, Explorer))

GType                   explorer_get_type         (void) G_GNUC_CONST;

static GList          * explorer_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * explorer_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * explorer_run              (GimpProcedure        *procedure,
                                                   GimpRunMode           run_mode,
                                                   GimpImage            *image,
                                                   gint                  n_drawables,
                                                   GimpDrawable        **drawables,
                                                   GimpProcedureConfig  *config,
                                                   gpointer              run_data);

static gboolean          CML_main_function        (gboolean              preview_p);
static void              CML_compute_next_step    (gint                  size,
                                                   gdouble             **h,
                                                   gdouble             **s,
                                                   gdouble             **v,
                                                   gdouble             **hn,
                                                   gdouble             **sn,
                                                   gdouble             **vn,
                                                   gdouble             **haux,
                                                   gdouble             **saux,
                                                   gdouble             **vaux);
static gdouble           CML_next_value           (gdouble              *vec,
                                                   gint                  pos,
                                                   gint                  size,
                                                   gdouble               c1,
                                                   gdouble               c2,
                                                   CML_PARAM            *param,
                                                   gdouble               aux);
static gdouble           logistic_function        (CML_PARAM            *param,
                                                   gdouble               x,
                                                   gdouble               power);


static gint        CML_explorer_dialog            (GimpProcedureConfig  *config);
static GtkWidget * CML_dialog_channel_panel_new   (CML_PARAM            *param,
                                                   gint                  channel_id);
static GtkWidget * CML_dialog_advanced_panel_new  (void);

static void     CML_explorer_toggle_entry_init    (WidgetEntry          *widget_entry,
                                                   GtkWidget            *widget,
                                                   gpointer              value_ptr);

static void     CML_explorer_int_entry_init       (WidgetEntry          *widget_entry,
                                                   GtkWidget            *scale,
                                                   gpointer               value_ptr);

static void     CML_explorer_double_entry_init    (WidgetEntry          *widget_entry,
                                                   GtkWidget            *scale,
                                                   gpointer              value_ptr);

static void     CML_explorer_menu_update          (GtkWidget             *widget,
                                                   gpointer               data);
static void     CML_initial_value_menu_update     (GtkWidget             *widget,
                                                   gpointer               data);
static void     CML_explorer_menu_entry_init      (WidgetEntry           *widget_entry,
                                                   GtkWidget             *widget,
                                                   gpointer               value_ptr);

static void    preview_update                      (void);
static void    function_graph_new                  (GtkWidget            *widget,
                                                    gpointer             *data);
static void    CML_set_or_randomize_seed_callback  (GtkWidget            *widget,
                                                    gpointer              data);
static void    CML_copy_parameters_callback        (GtkWidget            *widget,
                                                    gpointer              data);
static void    CML_initial_value_sensitives_update (void);

static void    CML_save_to_file_callback           (GtkWidget            *widget,
                                                    GimpProcedureConfig  *config);
static void    CML_save_to_file_response           (GtkWidget            *dialog,
                                                    gint                  response_id,
                                                    GimpProcedureConfig  *config);

static void    CML_preview_update_callback         (GtkWidget            *widget,
                                                    gpointer              data);
static void    CML_load_from_file_callback         (GtkWidget            *widget,
                                                    GimpProcedureConfig  *config);
static gboolean CML_load_parameter_file            (GFile                *file,
                                                    gboolean              interactive_mode,
                                                    GimpProcedureConfig  *config,
                                                    GError              **error);
static void    CML_load_from_file_response         (GtkWidget            *dialog,
                                                    gint                  response_id,
                                                    GimpProcedureConfig  *config);
static gint    parse_line_to_gint                  (GDataInputStream         *input,
                                                    gboolean             *flag,
                                                    GError              **error);
static gdouble parse_line_to_gdouble               (GDataInputStream         *input,
                                                    gboolean             *flag,
                                                    GError              **error);


G_DEFINE_TYPE (Explorer, explorer, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (EXPLORER_TYPE)
DEFINE_STD_SET_I18N


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

static GRand        *gr;
static GimpDrawable *drawable = NULL;
static gint          copy_source = 0;
static gint          copy_destination = 0;
static gint          selective_load_source = 0;
static gint          selective_load_destination = 0;
static gboolean      CML_preview_defer = FALSE;

static gdouble  *mem_chank0 = NULL;
static gint      mem_chank0_size = 0;
static guchar   *mem_chank1 = NULL;
static gint      mem_chank1_size = 0;
static guchar   *mem_chank2 = NULL;
static gint      mem_chank2_size = 0;


static void
explorer_class_init (ExplorerClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = explorer_query_procedures;
  plug_in_class->create_procedure = explorer_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
explorer_init (Explorer *explorer)
{
}

static GList *
explorer_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
explorer_create_procedure (GimpPlugIn  *plug_in,
                           const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            explorer_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("CML _Explorer..."));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Render/Pattern");

      gimp_procedure_set_documentation (procedure,
                                        _("Create abstract Coupled-Map "
                                          "Lattice patterns"),
                                        _("Make an image of Coupled-Map Lattice "
                                          "(CML). CML is a kind of Cellular "
                                          "Automata on continuous (value) "
                                          "domain. In GIMP_RUN_NONINTERACTIVE, "
                                          "the name of a parameter file is "
                                          "passed as the 4th arg. You can "
                                          "control CML_explorer via parameter "
                                          "file."),
                                        /*  Or do you want to call me
                                         *  with over 50 args?
                                         */
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Shuji Narazaki (narazaki@InetQ.or.jp); "
                                      "http://www.inetq.or.jp/~narazaki/TheGIMP/",
                                      "Shuji Narazaki",
                                      "1997");

      gimp_procedure_add_file_argument (procedure, "parameter-file",
                                        _("Parameter File"),
                                        _("The parameter file from which CML_explorer makes an image. "
                                          "This argument is only used in non-interactive runs."),
                                        G_PARAM_READWRITE);

      gimp_procedure_add_bytes_aux_argument (procedure, "settings-data",
                                             "Settings data",
                                             "TODO: eventually we must implement proper args for every settings",
                                             GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
explorer_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              gint                  n_drawables,
              GimpDrawable        **drawables,
              GimpProcedureConfig  *config,
              gpointer              run_data)
{
  GBytes *settings_bytes = NULL;
  GError *error          = NULL;

  if (n_drawables != 1)
    {
      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  gegl_init (NULL, NULL);

  /* Temporary code replacing legacy gimp_[gs]et_data() using an AUX argument.
   * This doesn't actually fix the "Reset to initial values|factory defaults"
   * features, but at least makes per-run value storage work.
   * TODO: eventually we want proper separate arguments as a complete fix.
   */
  g_object_get (config, "settings-data", &settings_bytes, NULL);
  if (settings_bytes != NULL && g_bytes_get_size (settings_bytes) == sizeof (ValueType))
    VALS = *((ValueType *) g_bytes_get_data (settings_bytes, NULL));
  g_bytes_unref (settings_bytes);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      if (! CML_explorer_dialog (config))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      {
        GFile *file = NULL;

        g_object_get (config,
                      "parameter-file", &file,
                      NULL);

        if (file == NULL)
          {
            g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                         "Procedure '%s' requires \"parameter-file\" argument "
                         "in non-interactive runs.",
                         PLUG_IN_PROC);

            return gimp_procedure_new_return_values (procedure,
                                                     GIMP_PDB_CALLING_ERROR,
                                                     error);
          }
        else if (! CML_load_parameter_file (file, FALSE, config, &error))
          {
            g_object_unref (file);

            return gimp_procedure_new_return_values (procedure,
                                                     GIMP_PDB_CALLING_ERROR,
                                                     error);
          }

        g_object_unref (file);
      }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      break;
    }

  if (CML_main_function (FALSE))
    {
      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush();
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  settings_bytes = g_bytes_new (&VALS, sizeof (ValueType));
  g_object_set (config, "settings-data", settings_bytes, NULL);
  g_bytes_unref (settings_bytes);

  g_free (mem_chank0);
  g_free (mem_chank1);
  g_free (mem_chank2);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
CML_main_function (gboolean preview_p)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *src_format;
  const Babl *dest_format;
  const Babl *to_hsv;
  const Babl *to_rgb;
  guchar     *dest_buf = NULL;
  guchar     *src_buf  = NULL;
  gint        x, y;
  gint        dx, dy;
  gboolean    dest_has_alpha = FALSE;
  gboolean    dest_is_gray   = FALSE;
  gboolean    src_has_alpha  = FALSE;
  gboolean    src_is_gray    = FALSE;
  gint        total, processed = 0;
  gint        keep_height = 1;
  gint        cell_num, width_by_pixel, height_by_pixel;
  gint        index;
  gint        src_bpp, src_bpl;
  gint        dest_bpp, dest_bpl;
  gdouble    *hues, *sats, *vals;
  gdouble    *newh, *news, *newv;
  gdouble    *haux, *saux, *vaux;

  if (! gimp_drawable_mask_intersect (drawable,
                                      &x, &y,
                                      &width_by_pixel, &height_by_pixel))
    return TRUE;

  to_hsv = babl_fish (babl_format ("R'G'B' float"), babl_format ("HSV float"));
  to_rgb = babl_fish (babl_format ("HSV float"), babl_format ("R'G'B' float"));

  src_has_alpha = dest_has_alpha = gimp_drawable_has_alpha (drawable);
  src_is_gray   = dest_is_gray   = gimp_drawable_is_gray (drawable);

  if (src_is_gray)
    {
      if (src_has_alpha)
        src_format = babl_format ("Y'A u8");
      else
        src_format = babl_format ("Y' u8");
    }
  else
    {
      if (src_has_alpha)
        src_format = babl_format ("R'G'B'A u8");
      else
        src_format = babl_format ("R'G'B' u8");
    }

  dest_format = src_format;

  src_bpp = dest_bpp = babl_format_get_bytes_per_pixel (src_format);

  if (preview_p)
    {
      dest_format = babl_format ("R'G'B' u8");

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
    return FALSE;

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
  src_buf = mem_chank1;

  if (mem_chank2_size < dest_bpl * keep_height)
    {
      g_free (mem_chank2);
      mem_chank2_size = dest_bpl * keep_height;
      mem_chank2 = (guchar *) g_malloc (mem_chank2_size);
    }
  dest_buf = mem_chank2;

  if (! preview_p)
    dest_buffer = gimp_drawable_get_shadow_buffer (drawable);

  src_buffer = gimp_drawable_get_buffer (drawable);

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
      int index;

      for (index = 0;
           index < MIN (cell_num, width_by_pixel / VALS.scale);
           index++)
        {
          gfloat hsv[3] = {0, 0, 0};

          gegl_buffer_sample (src_buffer, x + (index * VALS.scale), y, NULL,
                              buffer, babl_format ("HSV float"),
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);


          hues[index] = hsv[0];
          sats[index] = hsv[1];
          vals[index] = hsv[2];
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
        {
          gegl_buffer_get (src_buffer,
                           GEGL_RECTANGLE (x, y + dy,
                                           width_by_pixel, keep_height), 1.0,
                           src_format, src_buf,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        }

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
            {
              gfloat hsv[3] = { h / 255.0, s / 255.0, v / 255.0 };
              gfloat rgb[3];

              babl_process (to_rgb, hsv, rgb, 1);

              r = ROUND (rgb[0] * 255.0);
              g = ROUND (rgb[1] * 255.0);
              b = ROUND (rgb[2] * 255.0);
            }

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
                      rgbi[i] = src_buf[offset_y * src_bpl
                                        + (dx * VALS.scale + offset_x) * src_bpp + i];
                    if (src_is_gray && (VALS.val.function == CML_KEEP_VALUES))
                      {
                        b = rgbi[0];
                      }
                    else
                      {
                        gfloat rgb[3];
                        gfloat hsv[3];

                        for (i = 0; i < 3; i++)
                          rgb[i] = (gfloat) rgbi[i] / 255.0;

                        babl_process (to_hsv, rgb, hsv, 1);

                        if (VALS.hue.function != CML_KEEP_VALUES)
                          hsv[0] = (gfloat) h / 255.0;

                        if  (VALS.sat.function != CML_KEEP_VALUES)
                          hsv[1] = (gfloat) s / 255.0;

                        if (VALS.val.function != CML_KEEP_VALUES)
                          hsv[2] = (gfloat) v / 255.0;

                        babl_process (to_rgb, hsv, rgb, 1);

                        r = ROUND (rgb[0] * 255.0);
                        g = ROUND (rgb[1] * 255.0);
                        b = ROUND (rgb[2] * 255.0);
                      }
                  }

                dest_offset = (offset_y * dest_bpl +
                               (dx * VALS.scale + offset_x) * dest_bpp);

                if (dest_is_gray)
                  {
                    dest_buf[dest_offset++] = b;
                    if (preview_p)
                      {
                        dest_buf[dest_offset++] = b;
                        dest_buf[dest_offset++] = b;
                      }
                  }
                else
                  {
                    dest_buf[dest_offset++] = r;
                    dest_buf[dest_offset++] = g;
                    dest_buf[dest_offset++] = b;
                  }
                if (dest_has_alpha)
                  dest_buf[dest_offset] = 255;

                if ((!preview_p) &&
                    (++processed % (total / PROGRESS_UPDATE_NUM + 1)) == 0)
                  gimp_progress_update ((gdouble) processed / (gdouble) total);
              }
        }

      if (preview_p)
        {
          gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                                  0, dy,
                                  width_by_pixel, keep_height,
                                  GIMP_RGB_IMAGE,
                                  dest_buf,
                                  dest_bpl);
        }
      else
        {
          gegl_buffer_set (dest_buffer,
                           GEGL_RECTANGLE (x, y + dy,
                                           width_by_pixel, keep_height), 0,
                           dest_format, dest_buf,
                           GEGL_AUTO_ROWSTRIDE);
        }
    }

  g_object_unref (src_buffer);

  if (preview_p)
    {
      gtk_widget_queue_draw (preview);
    }
  else
    {
      gimp_progress_update (1.0);

      g_object_unref (dest_buffer);

      gimp_drawable_merge_shadow (drawable, TRUE);
      gimp_drawable_update (drawable,
                            x, y, width_by_pixel, height_by_pixel);
    }

  g_rand_free (gr);

  return TRUE;
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
CML_explorer_dialog (GimpProcedureConfig *config)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *bbox;
  GtkWidget *button;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_dialog_new (_("Coupled-Map-Lattice Explorer"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_halign (frame, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
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
                    config);

  button = gtk_button_new_with_mnemonic (_("_Save"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (CML_save_to_file_callback),
                    config);

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
      GtkSizeGroup *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
      GtkWidget    *grid;
      GtkWidget    *label;
      GtkWidget    *combo;
      GtkWidget    *frame;
      GtkWidget    *vbox;
      GtkWidget    *scale;

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

      scale = gimp_scale_entry_new (_("Zoom scale:"), VALS.scale, 1, 10, 0);
      gtk_size_group_add_widget (group, gimp_labeled_get_label (GIMP_LABELED (scale)));
      CML_explorer_int_entry_init (&widget_pointers[3][1],
                                   scale, &VALS.scale);
      gtk_grid_attach (GTK_GRID (grid), scale, 0, 1, 3, 1);
      gtk_widget_show (scale);

      scale = gimp_scale_entry_new (_("Start offset:"), VALS.start_offset, 0, 100, 0);
      gtk_size_group_add_widget (group, gimp_labeled_get_label (GIMP_LABELED (scale)));
      CML_explorer_int_entry_init (&widget_pointers[3][2],
                                   scale, &VALS.start_offset);
      gtk_grid_attach (GTK_GRID (grid), scale, 0, 2, 3, 1);
      gtk_widget_show (scale);

      frame =
        gimp_frame_new (_("Seed of Random (only for \"From Seed\" Modes)"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);

      scale = gimp_scale_entry_new (_("Seed:"), VALS.seed, 0, (guint32) -1, 0);
      gtk_size_group_add_widget (group, gimp_labeled_get_label (GIMP_LABELED (scale)));
      CML_explorer_int_entry_init (&widget_pointers[3][3],
                                   scale, &VALS.seed);
      gtk_grid_attach (GTK_GRID (grid), scale, 0, 0, 3, 1);
      gtk_widget_show (scale);

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
                                gtk_label_new_with_mnemonic (_("_Misc")));
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
  GtkWidget *grid;
  GtkWidget *combo;
  GtkWidget *toggle;
  GtkWidget *button;
  GtkWidget *scale;
  gpointer  *chank;
  gint       index = 0;

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

  scale = gimp_scale_entry_new (_("Mod. rate:"), param->mod_rate, 0.0, 1.0, 2);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  scale, &param->mod_rate);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, index, 3, 1);
  gtk_widget_show (scale);
  index++;

  scale = gimp_scale_entry_new (_("Env. sensitivity:"), param->env_sensitivity, 0.0, 1.0, 2);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  scale, &param->env_sensitivity);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, index, 3, 1);
  gtk_widget_show (scale);
  index++;

  scale = gimp_scale_entry_new (_("Diffusion dist.:"), param->diffusion_dist, 2, 10, 0);
  CML_explorer_int_entry_init (&widget_pointers[channel_id][index],
                               scale, &param->diffusion_dist);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, index, 3, 1);
  gtk_widget_show (scale);
  index++;

  scale = gimp_scale_entry_new (_("# of subranges:"), param->range_num, 1, 10, 0);
  CML_explorer_int_entry_init (&widget_pointers[channel_id][index],
                               scale, &param->range_num);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, index, 3, 1);
  gtk_widget_show (scale);
  index++;

  scale = gimp_scale_entry_new (_("P(ower factor):"), param->power, 0.0, 10.0, 2);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (scale), 0.1, 1.0);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  scale, &param->power);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, index, 3, 1);
  gtk_widget_show (scale);
  index++;

  scale = gimp_scale_entry_new (_("Parameter k:"), param->parameter_k, 0.0, 10.0, 2);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (scale), 0.1, 1.0);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  scale, &param->parameter_k);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, index, 3, 1);
  gtk_widget_show (scale);
  index++;

  scale = gimp_scale_entry_new (_("Range low:"), param->range_l, 0.0, 1.0, 2);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  scale, &param->range_l);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, index, 3, 1);
  gtk_widget_show (scale);
  index++;

  scale = gimp_scale_entry_new (_("Range high:"), param->range_h, 0.0, 1.0, 2);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
                                  scale, &param->range_h);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, index, 3, 1);
  gtk_widget_show (scale);
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
  GtkWidget *vbox;
  GtkWidget *subframe;
  GtkWidget *grid;
  GtkWidget *scale;
  gint       index = 0;
  gint       widget_offset = 12;
  gint       channel_id;
  CML_PARAM *param;

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

      scale = gimp_scale_entry_new (_("Ch. sensitivity:"), param->ch_sensitivity, 0.0, 1.0, 2);
      CML_explorer_double_entry_init (&widget_pointers[channel_id][index +
                                                                  widget_offset],
                                      scale, &param->ch_sensitivity);
      gtk_grid_attach (GTK_GRID (grid), scale, 0, index, 3, 1);
      gtk_widget_show (scale);
      index++;

      scale = gimp_scale_entry_new (_("Mutation rate:"), param->mutation_rate, 0.0, 1.0, 2);
      CML_explorer_double_entry_init (&widget_pointers[channel_id][index +
                                                                  widget_offset],
                                      scale, &param->mutation_rate);
      gtk_grid_attach (GTK_GRID (grid), scale, 0, index, 3, 1);
      gtk_widget_show (scale);
      index++;

      scale = gimp_scale_entry_new (_("Mutation dist.:"), param->mutation_dist, 0.0, 1.0, 2);
      CML_explorer_double_entry_init (&widget_pointers[channel_id][index +
                                                                  widget_offset],
                                      scale, &param->mutation_dist);
      gtk_grid_attach (GTK_GRID (grid), scale, 0, index, 3, 1);
      gtk_widget_show (scale);
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
  gint        x, y;
  gint        rgbi[3];
  gfloat      hsv[3];
  gfloat      rgb[3];
  gint        channel_id = GPOINTER_TO_INT (data[0]);
  CML_PARAM  *param      = data[1];
  const Babl *fish;

  fish = babl_fish (babl_format ("HSV float"), babl_format ("R'G'B' float"));

  cairo_set_line_width (cr, 1.0);

  cairo_surface_flush (buffer);

  for (x = 0; x < GRAPHSIZE; x++)
    {
      /* hue */
      rgbi[0] = rgbi[1] = rgbi[2] = 127;
      if ((0 <= channel_id) && (channel_id <= 2))
        rgbi[channel_id] = CANNONIZE ((*param), ((gdouble) x / (gdouble) 255));

      hsv[0] = (gdouble) rgbi[0] / 255.0;
      hsv[1] = (gdouble) rgbi[1] / 255.0;
      hsv[2] = (gdouble) rgbi[2] / 255.0;

      babl_process (fish, hsv, rgb, 1);

      for (y = 0; y < GRAPHSIZE; y++)
        {
          GIMP_CAIRO_RGB24_SET_PIXEL((img+(y*img_stride+x*4)),
                                     ROUND (rgb[0] * 255.0),
                                     ROUND (rgb[1] * 255.0),
                                     ROUND (rgb[2] * 255.0));
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
CML_save_to_file_callback (GtkWidget           *widget,
                           GimpProcedureConfig *config)
{
  static GtkWidget *dialog = NULL;
  GFile            *file   = NULL;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Save CML Explorer Parameters"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (widget)),
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Save"),   GTK_RESPONSE_OK,

                                     NULL);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                      TRUE);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (CML_save_to_file_response),
                        config);
      g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (gtk_true),
                        NULL);
    }

  g_object_get (config, "parameter-file", &file, NULL);
  if (file != NULL)
    {
      GError *error = NULL;

      gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog), file, &error);
      if (error != NULL)
        g_printerr ("%s: failed to set file '%s'.\n", PLUG_IN_PROC,
                    g_file_peek_path (file));
      g_clear_error (&error);
    }
  g_clear_object (&file);

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
CML_save_to_file_response (GtkWidget           *dialog,
                           gint                 response_id,
                           GimpProcedureConfig *config)
{
  GFile             *file;
  GFileOutputStream *fstream;
  GOutputStream     *output;
  GError            *error = NULL;
  gint               channel_id;

  if (response_id != GTK_RESPONSE_OK)
    {
      gtk_widget_hide (dialog);
      return;
    }

  file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
  if (! file)
    return;

  fstream = g_file_replace (file, NULL, FALSE,
                            G_FILE_CREATE_REPLACE_DESTINATION,
                            NULL, &error);
  output  = G_OUTPUT_STREAM (fstream);

  if (error != NULL)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 g_file_peek_path (file), error->message);
      g_object_unref (file);
      g_clear_error (&error);
      return;
    }

  if (! g_output_stream_printf (output, NULL, NULL, &error,
                                "; This is a parameter file for CML_explorer\n") ||
      ! g_output_stream_printf (output, NULL, NULL, &error,
                                "; File format version: %1.1f\n", PARAM_FILE_FORMAT_VERSION) ||
      ! g_output_stream_printf (output, NULL, NULL, &error, ";\n"))
    {
      g_message (_("Could not write to '%s': %s"),
                 g_file_peek_path (file), error->message);
      g_object_unref (file);
      g_clear_error (&error);
      g_object_unref (fstream);
      return;
    }

  for (channel_id = 0; channel_id < 3; channel_id++)
    {
      gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

      CML_PARAM param = *(CML_PARAM *)(channel_params[channel_id]);

      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "\t%s\n", channel_names[channel_id]))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Function_type    : %d (%s)\n",
                                    param.function, function_names[param.function]))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Compostion_type  : %d (%s)\n",
                                    param.composition, composition_names[param.composition]))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Arrange          : %d (%s)\n",
                                    param.arrange, arrange_names[param.arrange]))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Cyclic_range     : %d (%s)\n",
                                    param.cyclic_range, (param.cyclic_range ? "TRUE" : "FALSE")))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Mod. rate        : %s\n",
                                    g_ascii_dtostr (buf, sizeof (buf), param.mod_rate)))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Env_sensitivtiy  : %s\n",
                                    g_ascii_dtostr (buf, sizeof (buf), param.env_sensitivity)))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Diffusion dist.  : %d\n", param.diffusion_dist))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Ch. sensitivity  : %s\n",
                                    g_ascii_dtostr (buf, sizeof (buf), param.ch_sensitivity)))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Num. of Subranges: %d\n", param.range_num))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Power_factor     : %s\n",
                                    g_ascii_dtostr (buf, sizeof (buf), param.power)))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Parameter_k      : %s\n",
                                    g_ascii_dtostr (buf, sizeof (buf), param.parameter_k)))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Range_low        : %s\n",
                                    g_ascii_dtostr (buf, sizeof (buf), param.range_l)))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Range_high       : %s\n",
                                    g_ascii_dtostr (buf, sizeof (buf), param.range_h)))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Mutation_rate    : %s\n",
                                    g_ascii_dtostr (buf, sizeof (buf), param.mutation_rate)))
        break;
      if (! g_output_stream_printf (output, NULL, NULL, &error,
                                    "Mutation_distance: %s\n",
                                    g_ascii_dtostr (buf, sizeof (buf), param.mutation_dist)))
        break;
    }

  if (error != NULL)
    {
      g_message (_("Could not write to '%s': %s"),
                 g_file_peek_path (file), error->message);
      g_object_unref (file);
      g_clear_error (&error);
      g_object_unref (fstream);
      return;
    }

  if (! g_output_stream_printf (output, NULL, NULL, &error, "\n") ||
      ! g_output_stream_printf (output, NULL, NULL, &error,
                                "Initial value  : %d (%s)\n", VALS.initial_value, initial_value_names[VALS.initial_value]) ||
      ! g_output_stream_printf (output, NULL, NULL, &error,
                                "Zoom scale     : %d\n", VALS.scale) ||
      ! g_output_stream_printf (output, NULL, NULL, &error,
                                "Start offset   : %d\n", VALS.start_offset) ||
      ! g_output_stream_printf (output, NULL, NULL, &error,
                                "Random seed    : %d\n", VALS.seed))
    {
      g_message (_("Could not write to '%s': %s"),
                 g_file_peek_path (file), error->message);
      g_object_unref (file);
      g_clear_error (&error);
      g_object_unref (fstream);
      return;
    }
  g_object_unref (fstream);

  g_message (_("Parameters were saved to '%s'"), g_file_peek_path (file));

  g_object_set (config, "parameter-file", file, NULL);

  g_clear_object (&file);

  gtk_widget_hide (dialog);
}

/*  parameter file loading functions  */

static void
CML_load_from_file_callback (GtkWidget           *widget,
                             GimpProcedureConfig *config)
{
  static GtkWidget *dialog = NULL;
  GFile            *file   = NULL;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Load CML Explorer Parameters"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (widget)),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Open"),   GTK_RESPONSE_OK,

                                     NULL);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (CML_load_from_file_response),
                        config);
      g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (gtk_true),
                        NULL);
    }

  g_object_get (config, "parameter-file", &file, NULL);
  if (file != NULL)
    {
      GError *error = NULL;

      gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog), file, &error);
      if (error != NULL)
        g_printerr ("%s: failed to set file '%s'.\n", PLUG_IN_PROC,
                    g_file_peek_path (file));
      g_clear_error (&error);
    }
  g_clear_object (&file);

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
CML_load_from_file_response (GtkWidget           *dialog,
                             gint                 response_id,
                             GimpProcedureConfig *config)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GFile    *file = NULL;
      gint      channel_id;
      gboolean  flag = TRUE;
      GError   *error = NULL;

      file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

      flag = CML_load_parameter_file (file, TRUE, config, &error);

      if (error != NULL)
        g_printerr ("%s: loading file '%s' failed: %s\n", PLUG_IN_PROC,
                    g_file_peek_path (file), error->message);

      g_clear_error (&error);
      g_clear_object (&file);

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
CML_load_parameter_file (GFile                *file,
                         gboolean              interactive_mode,
                         GimpProcedureConfig  *config,
                         GError              **error)
{
  GIOStream        *iostream;
  GDataInputStream *input;
  gint              channel_id;
  gboolean          flag = TRUE;
  CML_PARAM         ch[3];
  gint              initial_value = 0;
  gint              scale = 1;
  gint              start_offset = 0;
  gint              seed = 0;
  gint              old2new_function_id[] = { 3, 4, 5, 6, 7, 9, 10, 11, 1, 2 };
  gdouble           version = 0.99;

  iostream = G_IO_STREAM (g_file_open_readwrite (file, NULL, error));

  if (iostream == NULL)
    return FALSE;

  input = g_data_input_stream_new (g_io_stream_get_input_stream (iostream));

  version = parse_line_to_gdouble (input, &flag, error); /* old format returns 1 */
  if (version == 1.0)
    {
      version = 0.99;
    }
  else if (! flag)
    {
      flag = TRUE;
      g_clear_error (error);
      version = parse_line_to_gdouble (input, &flag, error); /* maybe new format */
      if (flag)
        /* one more comment line */
        g_free (g_data_input_stream_read_line (input, NULL, NULL, NULL));
    }
  if (version == 0)
    {
      if (interactive_mode)
        g_message (_("Error: \"%s\" is not a CML parameter file."), g_file_peek_path (file));
      g_object_unref (iostream);
      return FALSE;
    }
  if (interactive_mode)
    {
      if (version < PARAM_FILE_FORMAT_VERSION)
        g_message (_("Warning: '%s' is an old format file."),
                   g_file_peek_path (file));

      if (PARAM_FILE_FORMAT_VERSION < version)
        g_message (_("Warning: '%s' is a parameter file for a newer "
                     "version of CML Explorer."),
                   g_file_peek_path (file));
    }
  for (channel_id = 0; flag && (channel_id < 3); channel_id++)
    {
      gchar *line;

      /* patched by Tim Mooney <mooney@dogbert.cc.ndsu.NoDak.edu> */
      /* skip channel name */
      line = g_data_input_stream_read_line (input, NULL, NULL, NULL);
      if (line == NULL)
        {
          flag = FALSE;
          break;
        }
      ch[channel_id].function = parse_line_to_gint (input, &flag, error);
      if (version < 1.0)
        ch[channel_id].function = old2new_function_id [ch[channel_id].function];
      if (1.0 <= version)
        ch[channel_id].composition = parse_line_to_gint (input, &flag, error);
      else
        ch[channel_id].composition = COMP_NONE;
      ch[channel_id].arrange = parse_line_to_gint (input, &flag, error);
      ch[channel_id].cyclic_range = parse_line_to_gint (input, &flag, error);
      ch[channel_id].mod_rate = parse_line_to_gdouble (input, &flag, error);
      ch[channel_id].env_sensitivity = parse_line_to_gdouble (input, &flag, error);
      if (1.0 <= version)
        ch[channel_id].diffusion_dist = parse_line_to_gint (input, &flag, error);
      else
        ch[channel_id].diffusion_dist = 2;
      ch[channel_id].ch_sensitivity = parse_line_to_gdouble (input, &flag, error);
      ch[channel_id].range_num = parse_line_to_gint (input, &flag, error);
      ch[channel_id].power = parse_line_to_gdouble (input, &flag, error);
      ch[channel_id].parameter_k = parse_line_to_gdouble (input, &flag, error);
      ch[channel_id].range_l = parse_line_to_gdouble (input, &flag, error);
      ch[channel_id].range_h = parse_line_to_gdouble (input, &flag, error);
      ch[channel_id].mutation_rate = parse_line_to_gdouble (input, &flag, error);
      ch[channel_id].mutation_dist = parse_line_to_gdouble (input, &flag, error);
    }
  if (flag)
    {
      gchar    *line;
      gboolean  dummy = TRUE;

      /* skip a line */
      line = g_data_input_stream_read_line (input, NULL, NULL, NULL);
      if (line == NULL)
        {
          dummy = FALSE;
        }
      else
        {
          g_free (line);
          initial_value = parse_line_to_gint (input, &dummy, error);
          scale = parse_line_to_gint (input, &dummy, error);
          start_offset = parse_line_to_gint (input, &dummy, error);
          seed = parse_line_to_gint (input, &dummy, error);
        }
      if (! dummy)
        {
          initial_value = 0;
          scale = 1;
          start_offset = 0;
          seed = 0;
        }
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

      g_object_set (config, "parameter-file", file, NULL);
    }

  g_object_unref (input);
  g_object_unref (iostream);
  return flag;
}

static gint
parse_line_to_gint (GDataInputStream  *input,
                    gboolean          *flag,
                    GError           **error)
{
  gint   i;
  gchar *line;
  gchar *str;
  gsize  read;

  if (! *flag)
    return 0;

  line = g_data_input_stream_read_line (input, &read, NULL, error);
  if (line == NULL)
    {
      /* set FALSE if fail to parse */
      *flag = FALSE;
      return 0.0;
    }

  line[read] = '\0';
  str = line;
  while (read > 1 && *str != ':')
    {
      str++;
      read--;
    }

  if (read == 1)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   "%s: invalid line in the CML file: %s",
                   PLUG_IN_PROC, line);
      *flag = FALSE;
      g_free (line);
      return 0.0;
    }

  i = atoi (str + 1);
  g_free (line);

  return i;
}

static gdouble
parse_line_to_gdouble (GDataInputStream  *input,
                       gboolean          *flag,
                       GError           **error)
{
  gdouble  d;
  gchar   *line;
  gchar   *str;
  gsize    read;

  if (! *flag)
    return 0.0;

  line = g_data_input_stream_read_line (input, &read, NULL, error);

  if (line == NULL)
    {
      /* set FALSE if fail to parse */
      *flag = FALSE;
      return 0.0;
    }

  line[read] = '\0';
  str = line;
  while (read > 1 && *str != ':')
    {
      str++;
      read--;
    }

  if (read == 1)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   "%s: invalid line in the CML file: %s",
                   PLUG_IN_PROC, line);
      *flag = FALSE;
      g_free (line);
      return 0.0;
    }

  d = g_ascii_strtod (str + 1, NULL);
  g_free (line);

  return d;
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
CML_explorer_int_adjustment_update (GimpLabelSpin *scale,
                                    gint          *value)
{
  *value = (gint) gimp_label_spin_get_value (scale);

  preview_update ();
}

static void
CML_explorer_int_entry_change_value (WidgetEntry *widget_entry)
{
  GimpLabelSpin *scale = GIMP_LABEL_SPIN (widget_entry->widget);

  gimp_label_spin_set_value (scale, *(gint *) (widget_entry->value));
}

static void
CML_explorer_int_entry_init (WidgetEntry *widget_entry,
                             GtkWidget   *scale,
                             gpointer     value_ptr)
{
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (CML_explorer_int_adjustment_update),
                    value_ptr);

  widget_entry->widget  = scale;
  widget_entry->value   = value_ptr;
  widget_entry->updater = CML_explorer_int_entry_change_value;
}

/*  double adjustment functions  */

static void
CML_explorer_double_adjustment_update (GimpLabelSpin *scale,
                                       gdouble       *value)
{
  *value = gimp_label_spin_get_value (scale);

  preview_update ();
}

static void
CML_explorer_double_entry_change_value (WidgetEntry *widget_entry)
{
  GimpLabelSpin *scale = GIMP_LABEL_SPIN (widget_entry->widget);

  gimp_label_spin_set_value (scale, *(gdouble *) (widget_entry->value));
}

static void
CML_explorer_double_entry_init (WidgetEntry *widget_entry,
                                GtkWidget   *scale,
                                gpointer     value_ptr)
{
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (CML_explorer_double_adjustment_update),
                    value_ptr);

  widget_entry->widget  = scale;
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
