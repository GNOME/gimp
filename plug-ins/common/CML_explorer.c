/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * CML_explorer.c -- This is a plug-in for The GIMP 1.0
 * Time-stamp: <1999-08-28 20:37:08 yasuhiro>
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
 * Version: 1.0.11
 * URL: http://www.inetq.or.jp/~narazaki/TheGIMP/
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
 *
 * Comment:
 *  CML is the abbreviation of Coupled-Map Lattice that is a model of
 *  complex systems, proposed by a physicist[1,2].
 *
 *  Similar models are summaried as follows:
 *
 *			Value	 Time	  Space
 *  Coupled-Map Lattice	cont.	 discrete discrete
 *  Celluar Automata	discrete discrete discrete
 *  Diffrential Eq.	cont.	 cont.	  cont.
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
 *    	Hue
 *
 *  The old format for CML_explorer included in gimp-0.99.[89] is:
 *    ; CML parameter file (version: 1.0)
 *    ;	Hue
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>		/* for seed of random number */

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define PARAM_FILE_FORMAT_VERSION 1.0
#define	PLUG_IN_NAME	          "plug_in_CML_explorer"
#define SHORT_NAME                "CML_explorer"
#define VALS                      CML_explorer_vals
#define PROGRESS_UPDATE_NUM        100
#define CML_LINE_SIZE             1024
#define TILE_CACHE_SIZE             32
#define SCALE_WIDTH                130
#define PREVIEW_WIDTH               64
#define PREVIEW_HEIGHT             220

#define	RANDOM               ((gdouble) ((double) rand ()/((double) G_MAXRAND)))
#define CANNONIZE(p, x)      (255*(((p).range_h - (p).range_l)*(x) + (p).range_l))
#define HCANNONIZE(p, x)     (254*(((p).range_h - (p).range_l)*(x) + (p).range_l))
#define POS_IN_TORUS(i,size) ((i < 0) ? size + i : ((size <= i) ? i - size : i))

typedef struct
{
  GtkWidget  *widget;
  gpointer    value;
  void      (*updater) ();
} WidgetEntry;

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
  CML_SIN_CURVE_STEP
};

static gchar *function_names[] =
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
  COMP_MIN_LINEAR_M1U
};

static gchar *composition_names[] =
{
  N_("None"),
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
  RAND_AND_P
};

static gchar *arrange_names[] =
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
  CML_INITIAL_RANDOM_FROM_SEED_SHARED
};

static gchar *initial_value_names[] =
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

#define CML_PARAM_NUM	15

typedef struct
{
  gint    function;
  gint    composition;
  gint    arrange;
  gint    cyclic_range;
  gdouble mod_rate;		/* diff / old-value */
  gdouble env_sensitivity;	/* self-diff : env-diff */
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

static gchar *channel_names[] =
{
  N_("Hue"),
  N_("Saturation"),
  N_("Value")
};

static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);

static GStatusType   CML_main_function     (gint       preview_p);
static void          CML_compute_next_step (gint       size,
					    gdouble  **h,
					    gdouble  **s,
					    gdouble  **v,
					    gdouble  **hn,
					    gdouble  **sn,
					    gdouble  **vn,
					    gdouble  **haux,
					    gdouble  **saux,
					    gdouble  **vaux);
static gdouble       CML_next_value        (gdouble   *vec,
					    gint       pos,
					    gint       size,
					    gdouble    c1,
					    gdouble    c2,
					    CML_PARAM *param,
					    gdouble    aux);
static gdouble       logistic_function     (CML_PARAM *param,
					    gdouble    x,
					    gdouble    power);


static gint	   CML_explorer_dialog           (void);
static GtkWidget * CML_dialog_channel_panel_new  (gchar     *name,
						  CML_PARAM *param,
						  gint       channel_id);
static GtkWidget * CML_dialog_advanced_panel_new (gchar     *name);

static void     CML_explorer_toggle_entry_init   (WidgetEntry *widget_entry,
						  GtkWidget   *widget,
						  gpointer     value_ptr);

static void     CML_explorer_int_entry_init      (WidgetEntry *widget_entry,
						  GtkObject   *object,
						  gpointer     value_ptr);

static void     CML_explorer_double_entry_init   (WidgetEntry *widget_entry,
						  GtkObject   *object,
						  gpointer     value_ptr);

static void     CML_explorer_menu_update         (GtkWidget   *widget,
						  gpointer     data);
static void     CML_initial_value_menu_update    (GtkWidget   *widget,
						  gpointer     data);
static void     CML_explorer_menu_entry_init     (WidgetEntry *widget_entry,
						  GtkWidget   *widget,
						  gpointer     value_ptr);

static void    preview_update                      (void);
static void    function_graph_new                  (GtkWidget *widget,
						    gpointer   data);
static void    CML_set_or_randomize_seed_callback  (GtkWidget *widget,
						    gpointer   data);
static void    CML_copy_parameters_callback        (GtkWidget *widget,
						    gpointer   data);
static void    CML_initial_value_sensitives_update (void);
static void    CML_explorer_ok_callback            (GtkWidget *widget,
						    gpointer   data);


static void    CML_save_to_file_callback (GtkWidget *widget,
					  gpointer   data);
static void    CML_execute_save_to_file  (GtkWidget *widget,
					  gpointer   data);
static gint    force_overwrite           (gchar     *filename);
static void    CML_overwrite_ok_callback (GtkWidget *widget,
					  gpointer   data);

static void    CML_preview_update_callback (GtkWidget *widget,
					    gpointer   data);
static void    CML_load_from_file_callback (GtkWidget *widget,
					    gpointer   data);
static gint    CML_load_parameter_file     (gchar     *filename,
					    gint       interactive_mode);
static void    CML_execute_load_from_file  (GtkWidget *widget,
					    gpointer   data);
static gint    parse_line_to_gint          (FILE      *file,
					    gint      *flag);
static gdouble parse_line_to_gdouble       (FILE      *file,
					    gint      *flag);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

typedef struct
{
  gint run;
} Interface;

static Interface INTERFACE =
{
  FALSE
};

static GtkWidget   *preview;
static WidgetEntry  widget_pointers[4][CML_PARAM_NUM];

typedef struct
{
  GtkWidget *widget;
  gint       logic;
} CML_sensitive_widget_table;

#define	RANDOM_SENSITIVES_NUM 5

static CML_sensitive_widget_table random_sensitives[RANDOM_SENSITIVES_NUM] =
{
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 }
};

static gint     drawable_id = 0;
static gint     copy_source = 0;
static gint     copy_destination = 0;
static gint     selective_load_source = 0;
static gint     selective_load_destination = 0;
static gint     CML_preview_defer = FALSE;
static gint     overwritable = FALSE;

static gdouble *mem_chank0 = NULL;
static gint     mem_chank0_size = 0;
static guchar  *mem_chank1 = NULL;
static gint     mem_chank1_size = 0;
static guchar  *mem_chank2 = NULL;
static gint     mem_chank2_size = 0;

MAIN ()

static void
query (void)
{
  static GParamDef args [] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    { PARAM_IMAGE, "image", "Input image (not used)"},
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_STRING, "parameter_file_name", "The name of parameter file. CML_explorer makes an image with its settings." },
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  INIT_I18N();

  gimp_install_procedure (PLUG_IN_NAME,
			  _("Make an image of Coupled-Map Lattice"),
			  _("Make an image of Coupled-Map Lattice (CML). CML is a kind of Cellula Automata on continuous (value) domain. In RUN_NONINTERACTIVE, the name of a prameter file is passed as the 4th arg. You can control CML_explorer via parameter file."),
			  /*  Or do you want to call me with over 50 args? */
			  "Shuji Narazaki (narazaki@InetQ.or.jp); http://www.inetq.or.jp/~narazaki/TheGIMP/",
			  "Shuji Narazaki",
			  "1997",
			  N_("<Image>/Filters/Render/Pattern/CML Explorer..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GStatusType   status = STATUS_EXECUTION_ERROR;
  GRunModeType  run_mode;

  run_mode = param[0].data.d_int32;
  drawable_id = param[2].data.d_drawable;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data (PLUG_IN_NAME, &VALS);
      if (! CML_explorer_dialog ())
	return;
      break;
    case RUN_NONINTERACTIVE:
      {
	gchar *filename = param[3].data.d_string;

	if (! CML_load_parameter_file (filename, FALSE))
	  return;
	break;
      }
    case RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_NAME, &VALS);
      break;
    }

  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);
  status = CML_main_function (FALSE);

  if (run_mode != RUN_NONINTERACTIVE)
    gimp_displays_flush();
  if (run_mode == RUN_INTERACTIVE && status == STATUS_SUCCESS)
    gimp_set_data (PLUG_IN_NAME, &VALS, sizeof (ValueType));

  if (mem_chank0)
    g_free (mem_chank0);
  if (mem_chank1)
    g_free (mem_chank1);
  if (mem_chank2)
    g_free (mem_chank2);

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static GStatusType
CML_main_function (gint preview_p)
{
  GDrawable *drawable = NULL;
  GPixelRgn  dest_rgn, src_rgn;
  guchar    *dest_buffer = NULL;
  guchar    *src_buffer = NULL;
  gint       x1, x2, y1, y2;
  gint       dx, dy;
  gint       dest_has_alpha = FALSE;
  gint       dest_is_gray = FALSE;
  gint       src_has_alpha = FALSE;
  gint       src_is_gray = FALSE;
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
  gimp_drawable_mask_bounds (drawable_id, &x1, &y1, &x2, &y2);
  src_has_alpha = dest_has_alpha = gimp_drawable_has_alpha (drawable_id);
  src_is_gray = dest_is_gray = gimp_drawable_is_gray (drawable_id);
  src_bpp = dest_bpp = (src_is_gray ? 1 : 3) + (src_has_alpha ? 1 : 0);

  if (preview_p)
    {
      dest_has_alpha = FALSE;
      dest_is_gray = FALSE;
      dest_bpp = 3;

      if (PREVIEW_WIDTH < x2 - x1)	/* preview < drawable (selection) */
	x2 = x1 + PREVIEW_WIDTH;
      if (PREVIEW_HEIGHT < y2 - y1)
	y2 = y1 + PREVIEW_HEIGHT;
    }
  width_by_pixel = x2 - x1;
  height_by_pixel = y2 - y1;
  dest_bpl = width_by_pixel * dest_bpp;
  src_bpl = width_by_pixel * src_bpp;
  cell_num = (width_by_pixel - 1)/ VALS.scale + 1;
  total = height_by_pixel * width_by_pixel;
  keep_height = VALS.scale;

  /* configure reusable memories */
  if (mem_chank0_size < 9 * cell_num * sizeof (gdouble))
    {
      if (mem_chank0)
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
      if (mem_chank1)
	g_free (mem_chank1);
      mem_chank1_size = src_bpl * keep_height;
      mem_chank1 = (guchar *) g_malloc (mem_chank1_size);
    }
  src_buffer = mem_chank1;

  if (mem_chank2_size < dest_bpl * keep_height)
    {
      if (mem_chank2)
	g_free (mem_chank2);
      mem_chank2_size = dest_bpl * keep_height;
      mem_chank2 = (guchar *) g_malloc (mem_chank2_size);
    }
  dest_buffer = mem_chank2;
  
  if (! preview_p)
    gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1,
			 width_by_pixel, height_by_pixel,
			 TRUE, TRUE);
  
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1,
		       width_by_pixel, height_by_pixel,
		       FALSE, FALSE);
  
  /* Note: Why seed is modulo of (1 << 15)? I use iscale_entry to control
     the value. Adjustment widget in Scale uses gdouble(?). But it is not
     precise to hold a long integer! Thus, in updating scale, the value will
     be rounded. Since I can't imagine another solution, I use short (16bit)
     integer as the seed of random number sequence. This approach works on
     my i386-based Linux.
  */
  if (VALS.initial_value < CML_INITIAL_RANDOM_FROM_SEED)
    VALS.seed = time (NULL) % (1 << 15);
  srand (VALS.seed);

  for (index = 0; index < cell_num; index++)
    {
      switch (VALS.hue.arrange)
	{
	case RAND_POWER0:
	  haux [index] = RANDOM * 10;
	  break;
	case RAND_POWER2:
	case MULTIPLY_GRADIENT:
	  haux [index] = (gdouble) abs ((index % 511) - 255) / (gdouble) 256;
	  break;
	case RAND_POWER1:
	case MULTIPLY_RANDOM0:
	  haux [index] = RANDOM;
	  break;
	case MULTIPLY_RANDOM1:
	  haux [index] = RANDOM * 2;
	  break;
	case RAND_AND_P:
	  haux [index] = (index % (2 * VALS.hue.diffusion_dist) == 0) ? (RANDOM) : VALS.hue.power;
	  break;
	default:
	  haux [index] = VALS.hue.power;
	  break;
	}
      switch (VALS.sat.arrange)
	{
	case RAND_POWER0:
	  saux [index] = RANDOM * 10;
	  break;
	case RAND_POWER2:
	case MULTIPLY_GRADIENT:
	  saux [index] = (gdouble) abs ((index % 511) - 255) / (gdouble) 256;
	  break;
	case RAND_POWER1:
	case MULTIPLY_RANDOM0:
	  saux [index] = RANDOM;
	  break;
	case MULTIPLY_RANDOM1:
	  saux [index] = RANDOM * 2;
	  break;
	case RAND_AND_P:
	  saux [index] = (index % (2 * VALS.sat.diffusion_dist) == 0) ? (RANDOM) : VALS.sat.power;
	  break;
	default:
	  saux [index] = VALS.sat.power;
	  break;
	}
      switch (VALS.val.arrange)
	{
	case RAND_POWER0:
	  vaux [index] = RANDOM * 10;
	  break;
	case RAND_POWER2:
	case MULTIPLY_GRADIENT:
	  vaux [index] = (gdouble) abs ((index % 511) - 255) / (gdouble) 256;
	  break;
	case RAND_POWER1:
	case MULTIPLY_RANDOM0:
	  vaux [index] = RANDOM;
	  break;
	case MULTIPLY_RANDOM1:
	  vaux [index] = RANDOM * 2;
	  break;
	case RAND_AND_P:
	  vaux [index] = (index % (2 * VALS.val.diffusion_dist) == 0) ? (RANDOM) : VALS.val.power;
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
	case 3:			/* use the values of the image (drawable) */
	  break;		/* copy from the drawable after this loop */
	case 4:			/* grandient 1 */
	  hues[index] = sats[index] = vals[index]
	    = (gdouble) (index % 256) / (gdouble) 256;
	  break;		/* gradinet 2 */
	case 5:
	  hues[index] = sats[index] = vals[index]
	    = (gdouble) abs ((index % 511) - 255) / (gdouble) 256;
	  break;
	case CML_INITIAL_RANDOM_INDEPENDENT:
	case CML_INITIAL_RANDOM_FROM_SEED:
	  hues[index] = RANDOM;
	  sats[index] = RANDOM;
	  vals[index] = RANDOM;
	  break;
	case CML_INITIAL_RANDOM_SHARED:
	case CML_INITIAL_RANDOM_FROM_SEED_SHARED:
	  hues[index] = sats[index] = vals[index] = RANDOM;
	  break;
	}
    }

  if (VALS.initial_value == 3)
    {
      int	index;

      for (index = 0; index < MIN (cell_num, width_by_pixel / VALS.scale); index++)
	{
	  guchar buffer[4];
	  int	rgbi[3];
	  int	i;

	  gimp_pixel_rgn_get_pixel (&src_rgn, buffer,
				    x1 + (index * VALS.scale), y1);
	  for (i = 0; i < 3; i++) rgbi[i] = buffer[i];
	  gimp_rgb_to_hsv (rgbi, rgbi + 1, rgbi + 2);
	  hues[index] = (gdouble) rgbi[0] / (gdouble) 255;
	  sats[index] = (gdouble) rgbi[1] / (gdouble) 255;
	  vals[index] = (gdouble) rgbi[2] / (gdouble) 255;
	}
    }
  if (! preview_p) gimp_progress_init (_("CML_explorer: evoluting..."));

  /* rolling start */
  for (index = 0; index < VALS.start_offset; index++)
    CML_compute_next_step (cell_num, &hues, &sats, &vals, &newh, &news, &newv,
			   &haux, &saux, &vaux);

  /* rendering */
  for (dy = 0; dy < height_by_pixel; dy += VALS.scale)
    {
      gint	i = 0;
      gint	r, g, b, h, s, v;
      gint	offset_x, offset_y, dest_offset;

      if (height_by_pixel < dy + keep_height)
	keep_height = height_by_pixel - dy;
      
      if ((VALS.hue.function == CML_KEEP_VALUES) ||
	  (VALS.sat.function == CML_KEEP_VALUES) ||
	  (VALS.val.function == CML_KEEP_VALUES))
	gimp_pixel_rgn_get_rect (&src_rgn, src_buffer,
				 x1, y1 + dy, width_by_pixel, keep_height);
      
      CML_compute_next_step (cell_num, &hues, &sats, &vals, &newh, &news, &newv,
			     &haux, &saux, &vaux);

      for (dx = 0; dx < cell_num; dx++)
	{
	  h = r = HCANNONIZE (VALS.hue, hues[dx]);
	  s = g = CANNONIZE (VALS.sat, sats[dx]);
	  v = b = CANNONIZE (VALS.val, vals[dx]);

	  if (! dest_is_gray)
	    gimp_hsv_to_rgb (&r, &g, &b);
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
		    int	rgbi[3];
		    int	i;
		  
		    for (i = 0; i < src_bpp; i++)
		      rgbi[i] = src_buffer[offset_y * src_bpl
					  + (dx * VALS.scale + offset_x) * src_bpp + i];
		    if (src_is_gray && (VALS.val.function == CML_KEEP_VALUES))
		      b = rgbi[0];
		    else
		      {
			gimp_rgb_to_hsv (rgbi, rgbi + 1, rgbi + 2);

			r = (VALS.hue.function == CML_KEEP_VALUES) ? rgbi[0] : h;
			g = (VALS.sat.function == CML_KEEP_VALUES) ? rgbi[1] : s;
			b = (VALS.val.function == CML_KEEP_VALUES) ? rgbi[2] : v;
			gimp_hsv_to_rgb (&r, &g, &b);
		      }
		  }
		dest_offset = offset_y * dest_bpl + (dx * VALS.scale + offset_x) * dest_bpp;
		if (dest_is_gray)
		  dest_buffer[dest_offset++] = b;
		else
		  {
		    dest_buffer[dest_offset++] = r;
		    dest_buffer[dest_offset++] = g;
		    dest_buffer[dest_offset++] = b;
		  }
		if (dest_has_alpha)
		  dest_buffer[dest_offset] = 255;
		if ((!preview_p) && (++processed % (total / PROGRESS_UPDATE_NUM)) == 0)
		  gimp_progress_update ((double)processed /(double) total);
	      }
	}
      if (preview_p)
	for (i = 0; i < keep_height; i++)
	  gtk_preview_draw_row (GTK_PREVIEW (preview),
				dest_buffer + dest_bpl * i, 0, dy + i,
				width_by_pixel);
      else
	gimp_pixel_rgn_set_rect (&dest_rgn, dest_buffer, x1, y1 + dy,
				 width_by_pixel, keep_height);
    }
  if (preview_p)
    {
      gtk_widget_draw (preview, NULL);
      gdk_flush ();
    }
  else
    {
      gimp_progress_update (1.0);
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->id, TRUE);
      gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
      gimp_drawable_detach (drawable);
    }

  return STATUS_SUCCESS;
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
  gint	index;

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

#define GD_SWAP(x, y)	{ gdouble *tmp = *x; *x = *y; *y = tmp; }
  GD_SWAP (h, hn);
  GD_SWAP (s, sn);
  GD_SWAP (v, vn);
#undef	SWAP
}

#define	AVE_DIST(x, y)	(((x) * (x) + (y) * (y))/ 2.0)
#define LOGISTICS(x)	logistic_function (param, x, power)
#define ENV_FACTOR(x)	(param->env_sensitivity * LOGISTICS (x))
#define C_ENV_FACTOR(x)	(param->mod_rate * ENV_FACTOR (x))
#define CHN_FACTOR(x)	(param->ch_sensitivity * LOGISTICS (x))
#define C_CHN_FACTOR(x)	(param->mod_rate * CHN_FACTOR (x))

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
	gdouble	tmp;
	
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
  if (RANDOM < param->mutation_rate)
    {
      val += ((RANDOM < 0.5) ? -1.0 : 1.0) * param->mutation_dist * RANDOM;
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
#undef AVE_DIST
#undef LOGISTICS
#undef ENV_FACTOR
#undef C_ENV_FACTOR
#undef CHN_FACTOR
#undef C_CHN_FACTOR

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
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *abox;
  GtkWidget *pframe;
  GtkWidget *hseparator;
  GtkWidget *button;
  guchar  *color_cube;
  gchar	 **argv;
  gint     argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup (SHORT_NAME);

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());

  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  dlg = gimp_dialog_new (_("Coupled-Map-Lattice Explorer"), "cml_explorer",
			 gimp_plugin_help_func, "filters/cml_explorer.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), CML_explorer_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  memset (&widget_pointers, (gint) 0, sizeof (widget_pointers));

  CML_preview_defer = TRUE;

  gimp_help_init ();

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), pframe);
  gtk_widget_show (pframe);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview),
		    PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_container_add (GTK_CONTAINER (pframe), preview);
  gtk_widget_show (preview);

  button = gtk_button_new_with_label (_("Save"));
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (CML_save_to_file_callback),
		      &VALS);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Load"));
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (CML_load_from_file_callback),
		      &VALS);
  gtk_widget_show (button);

  hseparator = gtk_hseparator_new ();
  gtk_box_pack_end (GTK_BOX (vbox), hseparator, FALSE, FALSE, 4);
  gtk_widget_show (hseparator);

  button = gtk_button_new_with_label (_("Random Seed"));
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (CML_set_or_randomize_seed_callback),
		      &VALS);
  gtk_widget_show (button);

  random_sensitives[2].widget = button;
  random_sensitives[2].logic  = FALSE;

  button = gtk_button_new_with_label (_("Fix Seed"));
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (CML_set_or_randomize_seed_callback),
		      &VALS);
  gtk_widget_show (button);

  random_sensitives[1].widget = button;
  random_sensitives[1].logic  = TRUE;

  button = gtk_button_new_with_label (_("New Seed"));
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (CML_preview_update_callback),
		      &VALS);
  gtk_widget_show (button);

  random_sensitives[0].widget = button;
  random_sensitives[0].logic  = TRUE;

  {
    GtkWidget *notebook;
    GtkWidget *page;

    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_box_pack_start (GTK_BOX (hbox), notebook, TRUE, TRUE, 0);
    gtk_widget_show (notebook);

    page = CML_dialog_channel_panel_new (_("Hue Settings"), 
					 &VALS.hue, 0);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
			      gtk_label_new (_("Hue")));

    page = CML_dialog_channel_panel_new (_("Saturation Settings"),
					 &VALS.sat, 1);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
			      gtk_label_new (_("Saturation")));

    page = CML_dialog_channel_panel_new (_("Value (Gray Image) Settings"),
					 &VALS.val, 2);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
			      gtk_label_new (_("Value")));

    page = CML_dialog_advanced_panel_new (_("Advanced Settings"));
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
			      gtk_label_new (_("Advanced")));

    {
      GtkWidget *table;
      GtkWidget *frame;
      GtkWidget *optionmenu;
      GtkWidget *subframe;
      GtkWidget *vbox;
      GtkObject *adj;

      frame = gtk_frame_new (_("Other Parameter Settings"));
      gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 4);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      subframe = gtk_frame_new (_("Channel Independed Parameters"));
      gtk_box_pack_start (GTK_BOX (vbox), subframe, FALSE, FALSE, 0);
      gtk_widget_show (subframe);

      table = gtk_table_new (3, 3, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_container_set_border_width (GTK_CONTAINER (table), 4);
      gtk_container_add (GTK_CONTAINER (subframe), table);
      gtk_widget_show (table);

      optionmenu =
	gimp_option_menu_new2
	(FALSE, CML_initial_value_menu_update,
	 &VALS.initial_value,
	 (gpointer) VALS.initial_value,

	 gettext (initial_value_names[0]),
	 (gpointer)                   0, NULL,

	 gettext (initial_value_names[1]),
	 (gpointer)                   1, NULL,

	 gettext (initial_value_names[2]),
	 (gpointer)                   2, NULL,

	 gettext (initial_value_names[3]),
	 (gpointer)                   3, NULL,

	 gettext (initial_value_names[4]),
	 (gpointer)                   4, NULL,

	 gettext (initial_value_names[5]),
	 (gpointer)                   5, NULL,

	 gettext (initial_value_names[CML_INITIAL_RANDOM_INDEPENDENT]),
	 (gpointer)                   CML_INITIAL_RANDOM_INDEPENDENT, NULL,

	 gettext (initial_value_names[CML_INITIAL_RANDOM_SHARED]),
	 (gpointer)                   CML_INITIAL_RANDOM_SHARED, NULL,

	 gettext (initial_value_names[CML_INITIAL_RANDOM_FROM_SEED]),
	 (gpointer)                   CML_INITIAL_RANDOM_FROM_SEED, NULL,

	 gettext (initial_value_names[CML_INITIAL_RANDOM_FROM_SEED_SHARED]),
	 (gpointer)                   CML_INITIAL_RANDOM_FROM_SEED_SHARED, NULL,

	 NULL);
      CML_explorer_menu_entry_init (&widget_pointers[3][0],
				    optionmenu, &VALS.initial_value);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
				 _("Initial Value:"), 1.0, 0.5,
				 optionmenu, 2, FALSE);

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
				  _("Zoom Scale:"), SCALE_WIDTH, 0,
				  VALS.scale, 1, 10, 1, 2, 0,
				  TRUE, 0, 0,
				  NULL, NULL);
      CML_explorer_int_entry_init (&widget_pointers[3][1],
				   adj, &VALS.scale);

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
				  _("Start Offset:"), SCALE_WIDTH, 0,
				  VALS.start_offset, 0, 100, 1, 10, 0,
				  TRUE, 0, 0,
				  NULL, NULL);
      CML_explorer_int_entry_init (&widget_pointers[3][2],
				   adj, &VALS.start_offset);

      subframe =
	gtk_frame_new (_("Seed of Random (only for \"From Seed\" Modes)"));
      gtk_box_pack_start (GTK_BOX (vbox), subframe, FALSE, FALSE, 0);
      gtk_widget_show (subframe);

      table = gtk_table_new (2, 3, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_container_set_border_width (GTK_CONTAINER (table), 4);
      gtk_container_add (GTK_CONTAINER (subframe), table);
      gtk_widget_show (table);

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
				  _("Seed:"), SCALE_WIDTH, 0,
				  VALS.seed, 0, G_MAXRAND, 1, 10, 0,
				  TRUE, 0, 0,
				  NULL, NULL);
      CML_explorer_int_entry_init (&widget_pointers[3][3],
				   adj, &VALS.seed);

      random_sensitives[3].widget = table;
      random_sensitives[3].logic  = FALSE;

      button =
	gtk_button_new_with_label
	(_("Switch to \"From Seed\" with the last Seed"));
      gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 3, 1, 2);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (CML_set_or_randomize_seed_callback),
			  &VALS);
      gtk_widget_show (button);

      random_sensitives[4].widget = button;
      random_sensitives[4].logic  = TRUE;

      gimp_help_set_help_data (button,
			       _("\"Fix seed\" button is an alias of me.\n"
				 "The same seed produces the same image, "
				 "if (1) the widths of images are same "
				 "(this is the reason why image on drawable "
				 "is different from preview), and (2) all "
				 "mutation rates equal to zero."), NULL);

      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame,
				gtk_label_new (_("Others")));
    }
    {
      GtkWidget	*table;
      GtkWidget *frame;
      GtkWidget *subframe;
      GtkWidget *optionmenu;
      GtkWidget *vbox;

      frame = gtk_frame_new (_("Misc Operations"));
      gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 4);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      subframe = gtk_frame_new (_("Copy Settings"));
      gtk_box_pack_start (GTK_BOX (vbox), subframe, FALSE, FALSE, 0);
      gtk_widget_show (subframe);

      table = gtk_table_new (3, 2, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_container_set_border_width (GTK_CONTAINER (table), 4);
      gtk_container_add (GTK_CONTAINER (subframe), table);
      gtk_widget_show (table);

      optionmenu = gimp_option_menu_new2 (FALSE, gimp_menu_item_update,
					  &copy_source, (gpointer) copy_source,

					  gettext (channel_names[0]),
					  (gpointer)             0, NULL,
					  gettext (channel_names[1]),
					  (gpointer)             1, NULL,
					  gettext (channel_names[2]),
					  (gpointer)             2, NULL,

					  NULL);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
				 _("Source Channel:"), 1.0, 0.5,
				 optionmenu, 1, TRUE);

      optionmenu = gimp_option_menu_new2 (FALSE, gimp_menu_item_update,
					  &copy_destination,
					  (gpointer) copy_destination,

					  gettext (channel_names[0]),
					  (gpointer)             0, NULL,
					  gettext (channel_names[1]),
					  (gpointer)             1, NULL,
					  gettext (channel_names[2]),
					  (gpointer)             2, NULL,

					  NULL);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
				 _("Destination Channel:"), 1.0, 0.5,
				 optionmenu, 1, TRUE);

      button = gtk_button_new_with_label (_("Copy Parameters"));
      gtk_table_attach (GTK_TABLE (table), button, 0, 2, 2, 3,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (CML_copy_parameters_callback),
			  &VALS);
      gtk_widget_show (button);

      subframe = gtk_frame_new (_("Selective Load Settings"));
      gtk_box_pack_start (GTK_BOX (vbox), subframe, FALSE, FALSE, 0);
      gtk_widget_show (subframe);

      table = gtk_table_new (2, 2, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_container_set_border_width (GTK_CONTAINER (table), 4);
      gtk_container_add (GTK_CONTAINER (subframe), table);
      gtk_widget_show (table);

      optionmenu = gimp_option_menu_new2 (FALSE, gimp_menu_item_update,
					  &selective_load_source,
					  (gpointer) selective_load_source,

					  _("NULL"),
					  (gpointer) 0, NULL,
					  gettext (channel_names[0]),
					  (gpointer) 1, NULL,
					  gettext (channel_names[1]),
					  (gpointer) 2, NULL,
					  gettext (channel_names[2]),
					  (gpointer) 3, NULL,

					  NULL);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
				 _("Source Channel in File:"), 1.0, 0.5,
				 optionmenu, 1, TRUE);

      optionmenu = gimp_option_menu_new2 (FALSE, gimp_menu_item_update,
					  &selective_load_destination,
					  (gpointer) selective_load_destination,

					  _("NULL"),
					  (gpointer) 0, NULL,
					  gettext (channel_names[0]),
					  (gpointer) 1, NULL,
					  gettext (channel_names[1]),
					  (gpointer) 2, NULL,
					  gettext (channel_names[2]),
					  (gpointer) 3, NULL,

					  NULL);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
				 _("Destination Channel:"), 1.0, 0.5,
				 optionmenu, 1, TRUE);

      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame,
				gtk_label_new (_("Misc Ops.")));
    }
  }

  CML_initial_value_sensitives_update ();

  /*  Displaying preview might takes a long time. Thus, fisrt, dialog itself
   *  should be shown before making preview in it.
   */
  gtk_widget_show (dlg);
  gdk_flush ();

  CML_preview_defer = FALSE;
  preview_update ();

  gtk_main ();
  gimp_help_free ();
  gdk_flush ();

  return INTERFACE.run;
}

static GtkWidget *
CML_dialog_channel_panel_new (gchar     *name,
			      CML_PARAM *param,
			      gint       channel_id)
{
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *optionmenu;
  GtkWidget *toggle;
  GtkWidget *button;
  GtkObject *adj;
  gpointer  *chank;
  gint       index = 0;

  frame = gtk_frame_new (name);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_widget_show (frame);

  table = gtk_table_new (13, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  optionmenu =
    gimp_option_menu_new2 (FALSE, CML_explorer_menu_update,
			   &param->function, (gpointer) param->function,

			   gettext (function_names[CML_KEEP_VALUES]),
			   (gpointer)              CML_KEEP_VALUES, NULL,

			   gettext (function_names[CML_KEEP_FIRST]),
			   (gpointer)              CML_KEEP_FIRST, NULL,

			   gettext (function_names[CML_FILL]),
			   (gpointer)              CML_FILL, NULL,

			   gettext (function_names[CML_LOGIST]),
			   (gpointer)              CML_LOGIST, NULL,

			   gettext (function_names[CML_LOGIST_STEP]),
			   (gpointer)              CML_LOGIST_STEP, NULL,

			   gettext (function_names[CML_POWER]),
			   (gpointer)              CML_POWER, NULL,

			   gettext (function_names[CML_POWER_STEP]),
			   (gpointer)              CML_POWER_STEP, NULL,

			   gettext (function_names[CML_REV_POWER]),
			   (gpointer)              CML_REV_POWER, NULL,

			   gettext (function_names[CML_REV_POWER_STEP]),
			   (gpointer)              CML_REV_POWER_STEP, NULL,

			   gettext (function_names[CML_DELTA]),
			   (gpointer)              CML_DELTA, NULL,

			   gettext (function_names[CML_DELTA_STEP]),
			   (gpointer)              CML_DELTA_STEP, NULL,

			   gettext (function_names[CML_SIN_CURVE]),
			   (gpointer)              CML_SIN_CURVE, NULL,

			   gettext (function_names[CML_SIN_CURVE_STEP]),
			   (gpointer)              CML_SIN_CURVE_STEP, NULL,

			   NULL);
  CML_explorer_menu_entry_init (&widget_pointers[channel_id][index],
				optionmenu, &param->function);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, index,
			     _("Function Type:"), 1.0, 0.5,
			     optionmenu, 2, FALSE);
  index++;

  optionmenu =
    gimp_option_menu_new2 (FALSE, CML_explorer_menu_update,
			   &param->composition, (gpointer) param->composition,

			   gettext (composition_names[COMP_NONE]),
			   (gpointer)                 COMP_NONE, NULL,

			   gettext (composition_names[COMP_MAX_LINEAR]),
			   (gpointer)                 COMP_MAX_LINEAR, NULL,

			   gettext (composition_names[COMP_MAX_LINEAR_P1]),
			   (gpointer)                 COMP_MAX_LINEAR_P1, NULL,

			   gettext (composition_names[COMP_MAX_LINEAR_M1]),
			   (gpointer)                 COMP_MAX_LINEAR_M1, NULL,

			   gettext (composition_names[COMP_MIN_LINEAR]),
			   (gpointer)                 COMP_MIN_LINEAR, NULL,

			   gettext (composition_names[COMP_MIN_LINEAR_P1]),
			   (gpointer)                 COMP_MIN_LINEAR_P1, NULL,

			   gettext (composition_names[COMP_MIN_LINEAR_M1]),
			   (gpointer)                 COMP_MIN_LINEAR_M1, NULL,

			   gettext (composition_names[COMP_MAX_LINEAR_P1L]),
			   (gpointer)                 COMP_MAX_LINEAR_P1L, NULL,

			   gettext (composition_names[COMP_MAX_LINEAR_P1U]),
			   (gpointer)                 COMP_MAX_LINEAR_P1U, NULL,

			   gettext (composition_names[COMP_MAX_LINEAR_M1L]),
			   (gpointer)                 COMP_MAX_LINEAR_M1L, NULL,

			   gettext (composition_names[COMP_MAX_LINEAR_M1U]),
			   (gpointer)                 COMP_MAX_LINEAR_M1U, NULL,

			   gettext (composition_names[COMP_MIN_LINEAR_P1L]),
			   (gpointer)                 COMP_MIN_LINEAR_P1L, NULL,

			   gettext (composition_names[COMP_MIN_LINEAR_P1U]),
			   (gpointer)                 COMP_MIN_LINEAR_P1U, NULL,

			   gettext (composition_names[COMP_MIN_LINEAR_M1L]),
			   (gpointer)                 COMP_MIN_LINEAR_M1L, NULL,

			   gettext (composition_names[COMP_MIN_LINEAR_M1U]),
			   (gpointer)                 COMP_MIN_LINEAR_M1U, NULL,

			   NULL);
  CML_explorer_menu_entry_init (&widget_pointers[channel_id][index],
				optionmenu, &param->composition);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, index,
			     _("Composition:"), 1.0, 0.5,
			     optionmenu, 2, FALSE);
  index++;

  optionmenu =
    gimp_option_menu_new2 (FALSE, CML_explorer_menu_update,
			   &param->arrange, (gpointer) param->arrange,

			   gettext (arrange_names[STANDARD]),
			   (gpointer)             STANDARD, NULL,

			   gettext (arrange_names[AVERAGE]),
			   (gpointer)             AVERAGE, NULL,

			   gettext (arrange_names[ANTILOG]),
			   (gpointer)             ANTILOG, NULL,

			   gettext (arrange_names[RAND_POWER0]),
			   (gpointer)             RAND_POWER0, NULL,

			   gettext (arrange_names[RAND_POWER1]),
			   (gpointer)             RAND_POWER1, NULL,

			   gettext (arrange_names[RAND_POWER2]),
			   (gpointer)             RAND_POWER2, NULL,

			   gettext (arrange_names[MULTIPLY_RANDOM0]),
			   (gpointer)             MULTIPLY_RANDOM0, NULL,

			   gettext (arrange_names[MULTIPLY_RANDOM1]),
			   (gpointer)             MULTIPLY_RANDOM1, NULL,

			   gettext (arrange_names[MULTIPLY_GRADIENT]),
			   (gpointer)             MULTIPLY_GRADIENT, NULL,

			   gettext (arrange_names[RAND_AND_P]),
			   (gpointer)             RAND_AND_P, NULL,

			   NULL);
  CML_explorer_menu_entry_init (&widget_pointers[channel_id][index],
				optionmenu, &param->arrange);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, index,
			     _("Misc Arrange:"), 1.0, 0.5,
			     optionmenu, 2, FALSE);
  index++;

  toggle = gtk_check_button_new_with_label (_("Use Cyclic Range"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				param->cyclic_range);
  gtk_table_attach_defaults (GTK_TABLE (table), toggle, 0, 3, index, index + 1);
  CML_explorer_toggle_entry_init (&widget_pointers[channel_id][index],
				  toggle, &param->cyclic_range);
  gtk_widget_show (toggle);
  index++;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, index,
			      _("Mod. Rate:"), SCALE_WIDTH, 0,
			      param->mod_rate, 0.0, 1.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
				  adj, &param->mod_rate);
  index++;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, index,
			      _("Env. Sensitivity:"), SCALE_WIDTH, 0,
			      param->env_sensitivity, 0.0, 1.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
				  adj, &param->env_sensitivity);
  index++;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, index,
			      _("Diffusion Dist.:"), SCALE_WIDTH, 0,
			      param->diffusion_dist, 2, 10, 1, 2, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  CML_explorer_int_entry_init (&widget_pointers[channel_id][index],
			       adj, &param->diffusion_dist);
  index++;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, index,
			      _("# of Subranges:"), SCALE_WIDTH, 0,
			      param->range_num, 1, 10, 1, 2, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  CML_explorer_int_entry_init (&widget_pointers[channel_id][index],
			       adj, &param->range_num);
  index++;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, index,
			      _("P(ower Factor):"), SCALE_WIDTH, 0,
			      param->power, 0.0, 10.0, 0.1, 1.0, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
				  adj, &param->power);
  index++;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, index,
			      _("Parameter k:"), SCALE_WIDTH, 0,
			      param->parameter_k, 0.0, 10.0, 0.1, 1.0, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
				  adj, &param->parameter_k);
  index++;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, index,
			      _("Range Low:"), SCALE_WIDTH, 0,
			      param->range_l, 0.0, 1.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
				  adj, &param->range_l);
  index++;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, index,
			      _("Range High:"), SCALE_WIDTH, 0,
			      param->range_h, 0.0, 1.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  CML_explorer_double_entry_init (&widget_pointers[channel_id][index],
				  adj, &param->range_h);
  index++;

  chank = g_new (gpointer, 2);
  chank[0] = (gpointer) channel_id;
  chank[1] = (gpointer) param;

  button = gtk_button_new_with_label (_("Plot the Graph of the Settings"));
  gtk_table_attach_defaults (GTK_TABLE (table), button,
			     0, 3, index, index + 1);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (function_graph_new),
		      chank);
  gtk_widget_show (button);
  index++;

  return frame;
}

static GtkWidget *
CML_dialog_advanced_panel_new (gchar *name)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *subframe;
  GtkWidget *table;
  GtkObject *adj;

  gint       index = 0;
  gint       widget_offset = 12;
  gint       channel_id;
  CML_PARAM *param;

  frame = gtk_frame_new (name);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  for (channel_id = 0; channel_id < 3; channel_id++)
    {
      param = (CML_PARAM *)&VALS + channel_id;

      subframe = gtk_frame_new (gettext (channel_names[channel_id]));
      gtk_box_pack_start (GTK_BOX (vbox), subframe, FALSE, FALSE, 0);
      gtk_widget_show (subframe);

      table = gtk_table_new (3, 3, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_container_set_border_width (GTK_CONTAINER (table), 4);
      gtk_container_add (GTK_CONTAINER (subframe), table);
      gtk_widget_show (table);

      index = 0;

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, index,
				  _("Ch. Sensitivity:"), SCALE_WIDTH, 0,
				  param->ch_sensitivity, 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      CML_explorer_double_entry_init (&widget_pointers[channel_id][index +
								  widget_offset],
				      adj, &param->ch_sensitivity);
      index++;

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, index,
				  _("Mutation Rate:"), SCALE_WIDTH, 0,
				  param->mutation_rate, 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      CML_explorer_double_entry_init (&widget_pointers[channel_id][index +
								  widget_offset],
				      adj, &param->mutation_rate);
      index++;

      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, index,
				  _("Mutation Dist.:"), SCALE_WIDTH, 0,
				  param->mutation_dist, 0.0, 1.0, 0.01, 0.1, 2,
				  TRUE, 0, 0,
				  NULL, NULL);
      CML_explorer_double_entry_init (&widget_pointers[channel_id][index +
								  widget_offset],
				      adj, &param->mutation_dist);
    }
  return frame;
}

void
preview_update (void)
{
  if (CML_preview_defer == FALSE)
    CML_main_function (TRUE);
}

static void
function_graph_new (GtkWidget *widget,
		    gpointer   data)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *abox;
  GtkWidget *preview;
  gint	     channel_id = *(int *) data;
  CML_PARAM *param = (CML_PARAM *) *((gpointer *) data + 1);

  dlg = gimp_dialog_new (_("Graph of the current settings"), "cml_explorer",
			 gimp_plugin_help_func, "filters/cml_explorer.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), gtk_widget_destroy,
			 NULL, 1, NULL, TRUE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  frame = gtk_frame_new (_("The Graph"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), 256, 256);
  gtk_container_add (GTK_CONTAINER (abox), preview);

  {
    gint   x, y, last_y, yy;
    guchar rgbc[3];
    gint   rgbi[3];
    guchar black[] = { 0, 0, 0 };
    guchar white[] = { 255, 255, 255 };

    for (x = 0; x < 256; x++)
      {
#define FAST_AND_FULLCOLOR	1
#ifdef FAST_AND_FULLCOLOR
	/* hue */
	rgbi[0] = rgbi[1] = rgbi[2] = 127;
	if ((0 <= channel_id) && (channel_id <= 2))
	  rgbi[channel_id] = CANNONIZE ((*param), ((gdouble) x / (gdouble) 255));
	gimp_hsv_to_rgb (rgbi, rgbi+1, rgbi+2);
	for (y = 0; y < 3; y++)
	  rgbc[y] = rgbi[y];
#endif
	for (y = 0; y < 256; y++)
	  gtk_preview_draw_row (GTK_PREVIEW (preview),
#ifdef FAST_AND_FULLCOLOR
				rgbc,
#else
				white,
#endif
				x, y, 1);
      }
    y = 255 * CLAMP (logistic_function (param, x / (gdouble) 255, param->power),
		     0, 1.0);
    for (x = 0; x < 256; x++)
      {
	last_y = y;
	/* curve */
	gtk_preview_draw_row (GTK_PREVIEW (preview), white, x, 255 - x, 1);
	y = 255 * CLAMP (logistic_function (param, x/(gdouble)255, param->power),
			 0, 1.0);
	for (yy = MIN (y, last_y); yy <= MAX (y, last_y); yy++)
	  gtk_preview_draw_row (GTK_PREVIEW (preview), black, x, 255 - yy, 1);
      }
  }

  gtk_widget_show (preview);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();
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
  gdk_flush ();

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
      gdk_flush ();
      return;
    }
  memcpy (channel_params[copy_destination],
	  channel_params[copy_source],
	  sizeof (CML_PARAM));
  CML_preview_defer = TRUE;
  widgets = widget_pointers[copy_destination];
  for (index = 0; index < CML_PARAM_NUM; index++)
    if (widgets[index].widget && widgets[index].updater)
      (widgets[index].updater) (widgets + index);

  gdk_flush ();
  CML_preview_defer = FALSE;
  preview_update ();
}

static void
CML_initial_value_sensitives_update (void)
{
  gint	i = 0;
  gint	flag1, flag2;

  flag1 = (CML_INITIAL_RANDOM_INDEPENDENT <= VALS.initial_value)
    & (VALS.initial_value <= CML_INITIAL_RANDOM_FROM_SEED_SHARED);
  flag2 = (CML_INITIAL_RANDOM_INDEPENDENT <= VALS.initial_value)
    & (VALS.initial_value <= CML_INITIAL_RANDOM_SHARED);

  for (; i < sizeof (random_sensitives) / sizeof (random_sensitives[0]); i++)
    if (random_sensitives[i].widget)
      gtk_widget_set_sensitive (random_sensitives[i].widget,
				flag1 & (random_sensitives[i].logic == flag2));
}

static void
CML_explorer_ok_callback (GtkWidget *widget,
			  gpointer   data)
{
  INTERFACE.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
CML_preview_update_callback (GtkWidget *widget,
			     gpointer   data)
{
  WidgetEntry seed_widget = widget_pointers[3][3];

  preview_update ();

  CML_preview_defer = TRUE;

  if (seed_widget.widget && seed_widget.updater)
    (seed_widget.updater) (&seed_widget);

  CML_preview_defer = FALSE;
}

/*  parameter file saving functions  */

static void
CML_save_to_file_callback (GtkWidget *widget,
			   gpointer   data)
{
  GtkWidget *filesel;

  filesel = gtk_file_selection_new (_("Save Parameters to"));
  gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked",
		      GTK_SIGNAL_FUNC (CML_execute_save_to_file),
		      filesel);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			     "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (filesel));

  if (strlen (VALS.last_file_name) > 0)
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				     VALS.last_file_name);

  gimp_help_connect_help_accel (filesel, gimp_plugin_help_func,
				"filters/cml_explorer.html");

  gtk_widget_show (filesel);
}

static void
CML_execute_save_to_file (GtkWidget *widget,
			  gpointer   data)
{
  gchar       *filename;
  struct stat  buf;
  FILE        *file = NULL;
  gint         channel_id;
  gint         err;

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (data));
  if (! filename)
    return;

  err = stat (filename, &buf);	/* returns zero if success */
  if ((err == 0) || (errno == ENOENT))
    {
      if (errno == ENOENT)
	{
	  file = fopen (filename, "w");
	}
      else if (buf.st_mode & S_IFDIR)
	{
	  GString *s = g_string_new (filename);

	  if (filename[strlen (filename) - 1] != '/')
	    g_string_append_c (s, '/');
	  gtk_file_selection_set_filename (GTK_FILE_SELECTION (data),
					   s->str);
	  g_string_free (s, TRUE);
	  return;
	}
      else if (buf.st_mode & S_IFREG) /* already exists */
	{
	  gtk_widget_set_sensitive (GTK_WIDGET (data), FALSE);

	  if (! force_overwrite (filename))
	    {
	      gtk_widget_set_sensitive (GTK_WIDGET (data), TRUE);
	      return;
	    }
	  else
	    {
	      file = fopen (filename, "w");
	    }
	}
    }
  if ((err != 0) && (file == NULL))
    {
      g_message (_("Error: could not open \"%s\""), filename);
      return;
    }
  else
    {
      fprintf (file, "; This is a parameter file for CML_explorer\n");
      fprintf (file, "; File format version: %1.1f\n", PARAM_FILE_FORMAT_VERSION);
      fprintf (file, ";\n");
      for (channel_id = 0; channel_id < 3; channel_id++)
	{
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
	  fprintf (file, "Mod. rate        : %f\n", param.mod_rate);
	  fprintf (file, "Env_sensitivtiy  : %f\n", param.env_sensitivity);
	  fprintf (file, "Diffusion dist.  : %d\n", param.diffusion_dist);
	  fprintf (file, "Ch. sensitivity  : %f\n", param.ch_sensitivity);
	  fprintf (file, "Num. of Subranges: %d\n", param.range_num);
	  fprintf (file, "Power_factor     : %f\n", param.power);
	  fprintf (file, "Parameter_k      : %f\n", param.parameter_k);
	  fprintf (file, "Range_low        : %f\n", param.range_l);
	  fprintf (file, "Range_high       : %f\n", param.range_h);
	  fprintf (file, "Mutation_rate    : %f\n", param.mutation_rate);
	  fprintf (file, "Mutation_distance: %f\n", param.mutation_dist);
	}
      fprintf (file, "\n");
      fprintf (file, "Initial value  : %d (%s)\n",
	       VALS.initial_value, initial_value_names[VALS.initial_value]);
      fprintf (file, "Zoom scale     : %d\n", VALS.scale);
      fprintf (file, "Start offset   : %d\n", VALS.start_offset);
      fprintf (file, "Random seed    : %d\n", VALS.seed);
      fclose(file);

      g_message (_("Parameters were Saved to\n\"%s\""), filename);

      if (sizeof (VALS.last_file_name) <= strlen (filename))
	filename[sizeof (VALS.last_file_name) - 1] = '\0';
      strcpy (VALS.last_file_name, filename);
    }

  gtk_widget_destroy (GTK_WIDGET (data));
}

static gint
force_overwrite (gchar *filename)
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *hbox;
  gchar     *buffer;
  gint       tmp;

  dlg = gimp_dialog_new (_("CML File Operation Warning"), "cml_explorer",
			 gimp_plugin_help_func, "filters/cml_explorer.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, FALSE, FALSE,

			 _("OK"), CML_overwrite_ok_callback,
			 NULL, NULL, NULL, FALSE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, TRUE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 6);
  gtk_widget_show (hbox);

  buffer = g_strdup_printf (_("%s\nexists, Overwrite?"), filename);
  label = gtk_label_new (buffer);
  g_free (buffer);

  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 6);
  gtk_widget_show (label);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  tmp = overwritable;
  overwritable = FALSE;

  return tmp;
}

static void
CML_overwrite_ok_callback (GtkWidget *widget,
			   gpointer   data)
{
  overwritable = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

/*  parameter file loading functions  */

static void
CML_load_from_file_callback (GtkWidget *widget,
			     gpointer   data)
{
  GtkWidget *filesel;

  if ((selective_load_source == 0) || (selective_load_destination == 0))
    filesel = gtk_file_selection_new (_("Load Parameters from"));
  else
    filesel = gtk_file_selection_new (_("Selective Load from"));
  gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked",
		      GTK_SIGNAL_FUNC (CML_execute_load_from_file),
		      filesel);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			     "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (filesel));

  if (strlen (VALS.last_file_name) > 0)
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				     VALS.last_file_name);

  gimp_help_connect_help_accel (filesel, gimp_plugin_help_func,
				"filters/cml_explorer.html");

  gtk_widget_show (filesel);
}

static void
CML_execute_load_from_file (GtkWidget *widget,
			    gpointer   data)
{
  gchar *filename;
  gint   channel_id;
  gint   flag = TRUE;

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (data));

  gtk_widget_set_sensitive (GTK_WIDGET (data), FALSE);
  flag = CML_load_parameter_file (filename, TRUE);
  gtk_widget_destroy (GTK_WIDGET(data));

  if (flag)
    {
      gint index;
      WidgetEntry *widgets;

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

      gdk_flush ();

      CML_preview_defer = FALSE;

      preview_update ();
    }
}

static gint
CML_load_parameter_file (gchar *filename,
			 gint   interactive_mode)
{
  FILE      *file;
  gint       channel_id;
  gint       flag = TRUE;
  CML_PARAM  ch[3];
  gint       initial_value = 0;
  gint       scale = 1;
  gint       start_offset = 0;
  gint       seed = 0;
  gint       old2new_function_id[] = { 3, 4, 5, 6, 7, 9, 10, 11, 1, 2 };

  file = fopen (filename, "r");

  if (!file)
    {
      g_message (_("Error: could not open \"%s\""), filename);
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
	    g_message (_("Warning: it's an old format file."));
	  if (PARAM_FILE_FORMAT_VERSION < version)
	    g_message (_("Warning: Hmmm, it's a parameter file for newer CML_explorer than me."));
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
      fclose(file);
    }

  if (flag == FALSE)
    {
      if (interactive_mode)
	gimp_message (_("Error: failed to load parameters"));
    }
  else
    {
      if ((selective_load_source == 0) || (selective_load_destination == 0))
	{
	  memcpy (&(VALS.hue), (void *)&ch[0], sizeof (CML_PARAM));
	  memcpy (&(VALS.sat), (void *)&ch[1], sizeof (CML_PARAM));
	  memcpy (&(VALS.val), (void *)&ch[2], sizeof (CML_PARAM));
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

      if ( sizeof (VALS.last_file_name) <= strlen (filename))
	filename[sizeof (VALS.last_file_name) - 1] = '\0';
      strcpy (VALS.last_file_name, filename);
    }
  return flag;
}

static gint
parse_line_to_gint (FILE *file,
		    gint *flag)
{
  gchar  line[CML_LINE_SIZE];
  gchar *str;
  gint   value;

  if (! *flag)
    return 0;
  if (fgets (line, CML_LINE_SIZE - 1, file) == NULL)
    {
      *flag = FALSE;		/* set FALSE if fail to parse */
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
      str++;
  value = (gint) atoi (str + 1);

  return value;
}

static gdouble
parse_line_to_gdouble (FILE *file,
		       gint *flag)
{
  gchar    line[CML_LINE_SIZE];
  gchar   *str;
  gdouble  value;

  if (! *flag)
    return 0;
  if (fgets (line, CML_LINE_SIZE - 1, file) == NULL)
    {
      *flag = FALSE;		/* set FALSE if fail to parse */
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
      str++;
  value = (gdouble) atof (str + 1);

  return value;
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
  gtk_signal_connect (GTK_OBJECT (widget), "toggled",
		      GTK_SIGNAL_FUNC (CML_explorer_toggle_update),
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
CML_explorer_int_entry_init (WidgetEntry *widget_entry,
			     GtkObject   *adjustment,
			     gpointer     value_ptr)
{
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (CML_explorer_int_adjustment_update),
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
CML_explorer_double_entry_init (WidgetEntry *widget_entry,
				GtkObject   *adjustment,
				gpointer     value_ptr)
{
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (CML_explorer_double_adjustment_update),
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
  gimp_menu_item_update (widget, data);

  preview_update ();
}

static void
CML_initial_value_menu_update (GtkWidget *widget,
			       gpointer   data)
{
  gimp_menu_item_update (widget, data);

  CML_initial_value_sensitives_update ();
  preview_update ();
}

static void
CML_explorer_menu_entry_change_value (WidgetEntry *widget_entry)
{
  gtk_option_menu_set_history (GTK_OPTION_MENU (widget_entry->widget),
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
