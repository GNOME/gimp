/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * CML_explorer.c -- This is a plug-in for The GIMP 1.0
 * Time-stamp: <1997/11/22 23:54:47 narazaki@InetQ.or.jp>
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

#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>		/* for seed of random number */

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif /* RAND_MAX */

#define PARAM_FILE_FORMAT_VERSION	1.0
#define	PLUG_IN_NAME	"plug_in_CML_explorer"
#define SHORT_NAME	"CML_explorer"
#define PROGRESS_NAME	"CML_explorer: evoluting..."
#define MENU_POSITION	"<Image>/Filters/Render/CML explorer"
#define	VERBOSE_DIALOGS	1
#define	MAIN_FUNCTION	CML
#define INTERFACE	CML_explorer_interface
#define	DIALOG		CML_explorer_dialog
#define VALS		CML_explorer_vals
#define OK_CALLBACK	CML_explorer_ok_callback
#define PROGRESS_UPDATE_NUM	100
#define CML_LINE_SIZE	1024
#define TILE_CACHE_SIZE 32
/* gtkWrapper */
/* gtkW is the abbreviation of gtk Wrapper */
#define GTKW_ENTRY_WIDTH	60
#define GTKW_SCALE_WIDTH	130
#define GTKW_PREVIEW_WIDTH	64
#define GTKW_PREVIEW_HEIGHT	220
#define	GTKW_BORDER_WIDTH	3
#define GTKW_FLOAT_MIN_ERROR	0.000001
#define GTKW_ENTRY_BUFFER_SIZE	32
/* gtkW type */
typedef struct
{
  GtkWidget	*widget;
  gpointer	value;
  void	 	(*updater)();
} gtkW_widget_table;

typedef struct
{
  gchar *name;
  gpointer data;
} gtkW_menu_item;
/* gtkW global variables */
gint	gtkW_border_width = GTKW_BORDER_WIDTH;
gint	gtkW_border_height = 0;
gint	gtkW_homogeneous_layout	= FALSE;
gint	gtkW_frame_shadow_type = GTK_SHADOW_ETCHED_IN;
gint	gtkW_align_x = GTK_FILL|GTK_EXPAND;
gint	gtkW_align_y = GTK_FILL;
gint	gtkW_tooltips_color_allocated = FALSE;
GdkColor	tooltips_foreground, tooltips_background;
/* gtkW callback */
static void	gtkW_close_callback (GtkWidget *widget, gpointer data);
static void	gtkW_toggle_update (GtkWidget *widget, gpointer data);
static void	gtkW_iscale_update (GtkAdjustment *adjustment, gpointer data);
static void	gtkW_ientry_update (GtkWidget *widget, gpointer data);
static void	gtkW_dscale_update (GtkAdjustment *adjustment, gpointer data);
static void	gtkW_dentry_update (GtkWidget *widget, gpointer data);
static void	gtkW_dscale_entry_change_value (gtkW_widget_table *wtable);
static void	gtkW_iscale_entry_change_value (gtkW_widget_table *wtable);
static void	gtkW_menu_change_value (gtkW_widget_table *wtable);
static void	gtkW_menu_update (GtkWidget *widget, gpointer   data);
static void	gtkW_toggle_change_value (gtkW_widget_table	*wtable);
/* gtkW method */
GtkWidget	*gtkW_check_button_new (GtkWidget	*parent,
					gchar		*name,
					GtkSignalFunc	update,
					gint		*value);
static GtkWidget	*gtkW_dialog_new (gchar	*name,
					  GtkSignalFunc	ok_callback,
					  GtkSignalFunc	close_callback);
static GtkWidget	*gtkW_frame_new (GtkWidget *parent, gchar *name);
static GtkWidget	*gtkW_hbox_new (GtkWidget *parent);
static void	gtkW_message_dialog (gint gtk_was_initialized, gchar *message);
static GtkWidget	*gtkW_message_dialog_new (gchar * name);
static GtkWidget	*gtkW_table_add_button (GtkWidget	*table,
						gchar		*name,
						gint		x0,
						gint		x1,
						gint		y,
						GtkSignalFunc	callback,
						gpointer value);
static GtkWidget	*gtkW_table_add_dscale_entry (GtkWidget	*table,
						      gchar	*name,
						      gint	x,
						      gint	y,
						      GtkSignalFunc scale_update,
						      GtkSignalFunc entry_update,
						      gdouble	*value,
						      gdouble	min,
						      gdouble	max,
						      gdouble	step,
						      gpointer	table_entry);
static GtkWidget	*gtkW_table_add_iscale_entry (GtkWidget	*table,
						      gchar	*name,
						      gint	x,
						      gint	y,
						      GtkSignalFunc scale_update,
						      GtkSignalFunc entry_update,
						      gint	*value,
						      gdouble	min,
						      gdouble	max,
						      gdouble	step,
						      gpointer	table_entry);
static GtkWidget	 *gtkW_table_add_menu (GtkWidget	*parent,
					       gchar		*name,
					       gint		x,
					       gint		y,
					       GtkSignalFunc	imenu_update,
					       gint		*val,
					       gtkW_menu_item	*item,
					       gint		item_num,
					       gpointer		table_entry);
static GtkWidget	*gtkW_table_add_toggle (GtkWidget	*table,
						gchar		*name,
						gint		x,
						gint		y,
						GtkSignalFunc	update,
						gint		*value,
						gpointer	widget_entry);
static GtkWidget	*gtkW_table_new (GtkWidget *parent, gint col, gint row);
static GtkWidget	*gtkW_vbox_add_button (GtkWidget	*vbox,
					       gchar		*name,
					       GtkSignalFunc	update,
					       gpointer		data);
static GtkWidget	*gtkW_vbox_new (GtkWidget *parent);
static GtkTooltips	*gtkW_tooltips_new (GtkWidget *frame);
/* end of GtkW */

#define	RANDOM	((gdouble) ((double) rand ()/((double) RAND_MAX)))
#define CANNONIZE(p, x)	(255*(((p).range_h - (p).range_l)*(x) + (p).range_l))
#define HCANNONIZE(p, x)	(254*(((p).range_h - (p).range_l)*(x) + (p).range_l))
#define POS_IN_TORUS(i,size)	((i < 0) ? size + i : ((size <= i) ? i - size : i))

gtkW_menu_item function_menu [] =
{
#define	CML_KEEP_VALUES	0
  { "Keep image's values", NULL },
#define	CML_KEEP_FIRST	1
  { "Keep the first value", NULL },
#define	CML_FILL	2
  { "Fill with parameter k", NULL },
#define	CML_LOGIST	3
  { "k{x(1-x)}^p", NULL },
#define	CML_LOGIST_STEP	4
  { "k{x(1-x)}^p stepped", NULL },
#define	CML_POWER	5
  { "kx^p", NULL },
#define	CML_POWER_STEP	6
  { "kx^p stepped", NULL },
#define	CML_REV_POWER	7
  { "k(1 - x^p)", NULL },
#define	CML_REV_POWER_STEP	8
  { "k(1 - x^p) stepped", NULL },
#define	CML_DELTA	9
  { "Delta function", NULL },
#define	CML_DELTA_STEP	10
  { "Delta function stepped", NULL },
#define CML_SIN_CURVE	11
  { "sin^p-based function", NULL },
#define CML_SIN_CURVE_STEP	12
  { "sin^p, stepped", NULL },
};

gtkW_menu_item composition_menu [] =
{
#define COMP_NONE		0
  { "None", NULL },
#define COMP_MAX_LINEAR		1
  { "Max (x, -)", NULL },
#define COMP_MAX_LINEAR_P1	2
  { "Max (x+d, -)", NULL },
#define COMP_MAX_LINEAR_M1	3
  { "Max (x-d, -)", NULL },
#define COMP_MIN_LINEAR		4
  { "Mix (x, -)", NULL },
#define COMP_MIN_LINEAR_P1	5
  { "Min (x+d, -)", NULL },
#define COMP_MIN_LINEAR_M1	6
  { "Min (x-d, -)", NULL },
#define COMP_MAX_LINEAR_P1L	7
  { "Max (x+d, -), (x < 0.5)", NULL },
#define COMP_MAX_LINEAR_P1U	8
  { "Max (x+d, -), (0.5 < x)", NULL },
#define COMP_MAX_LINEAR_M1L	9
  { "Max (x-d, -), (x < 0.5)", NULL },
#define COMP_MAX_LINEAR_M1U	10
  { "Max (x-d, -), (0.5 < x)", NULL },
#define COMP_MIN_LINEAR_P1L	11
  { "Min (x+d, -), (x < 0.5)", NULL },
#define COMP_MIN_LINEAR_P1U	12
  { "Min (x+d, -), (0.5 < x)", NULL },
#define COMP_MIN_LINEAR_M1L	13
  { "Min (x-d, -), (x < 0.5)", NULL },
#define COMP_MIN_LINEAR_M1U	14
  { "Min (x-d, -), (0.5 < x)", NULL }
};

gtkW_menu_item arrange_menu [] =
{
#define	STANDARD	0
  { "Standard", NULL },
#define	AVERAGE		1
  { "Use average value", NULL },
#define	ANTILOG		2
  { "Use reverse value", NULL },
#define	RAND_POWER0	3
  { "With random power (0,10)", NULL },
#define	RAND_POWER1	4
  { "With random power (0,1)", NULL },
#define	RAND_POWER2	5
  { "With gradient power (0,1)", NULL },
#define	MULTIPLY_RANDOM0	6
  { "Multiply rand. value (0,1)", NULL },
#define	MULTIPLY_RANDOM1	7
  { "Multiply rand. value (0,2)", NULL },
#define	MULTIPLY_GRADIENT	8
  { "Multiply gradient (0,1)", NULL },
#define RAND_AND_P	9
  { "With p and random (0,1)", NULL },
};

gtkW_menu_item initial_value_menu [] =
{
  { "All black", NULL },	/* 0 */
  { "All gray", NULL },		/* 1 */
  { "All white", NULL },	/* 2 */
  { "The first row of the image", NULL }, /* 3 */
  { "Continuous gradient", NULL }, /* 4 */
  { "Continuous grad. w/o gap", NULL }, /* 5 */
#define	CML_INITIAL_RANDOM_INDEPENDENT	6
  { "Random, ch. independent", NULL },
#define	CML_INITIAL_RANDOM_SHARED	7
  { "Random shared", NULL },
#define	CML_INITIAL_RANDOM_FROM_SEED	8
  { "Randoms from seed" , NULL },
#define	CML_INITIAL_RANDOM_FROM_SEED_SHARED	9
  { "Randoms from seed (shared)" , NULL }
} ;

#define CML_PARAM_NUM	15
typedef struct
{
  gint	  function;
  gint	  composition;
  gint	  arrange;
  gint	  cyclic_range;
  gdouble mod_rate;		/* diff / old-value */
  gdouble env_sensitivity;	/* self-diff : env-diff */
  gint	  diffusion_dist;
  gdouble ch_sensitivity;
  gint	  range_num;
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
  gint	initial_value;
  gint	scale;
  gint	start_offset;
  gint	seed;
  gchar last_file_name[256];
} ValueType;

static ValueType VALS =
{
  /* function     composition  arra
   cyc chng sens diff cor  n  pow   k   (l,h) rnd dist */
  {CML_SIN_CURVE, COMP_NONE, STANDARD,
   1,  0.5, 0.7,  2,  0.0, 1, 1.0, 1.0, 0, 1, 0.0, 0.1},
  {CML_FILL     , COMP_NONE, STANDARD,
   0,  0.6, 0.1,  2,  0.0, 1, 1.4, 0.9, 0, 0.9, 0.0, 0.1},
  {CML_FILL     , COMP_NONE, STANDARD,
   0,  0.5, 0.2,  2,  0.0, 1, 2.0, 1.0, 0, 0.9, 0.0, 0.1},
  6,				/* random value 1 */
  1,				/* scale */
  0,				/* start_offset */
  0,				/* seed */
  ""
};

gtkW_menu_item channel_menu [] =
{
  { "Hue", (gpointer) &VALS.hue },
  { "Saturation", (gpointer) &VALS.sat },
  { "Value", (gpointer) &VALS.val },
};

gtkW_menu_item sload_menu [] =
{
  { "NULL", NULL },
  { "Hue", NULL },
  { "Saturation", NULL },
  { "Value", NULL },
};

static void	query	(void);
static void	run	(char	*name,
			 int	nparams,
			 GParam	*param,
			 int	*nreturn_vals,
			 GParam **return_vals);
static GStatusType	MAIN_FUNCTION (gint preview_p);
static void	CML_compute_next_step (gint size,
		       gdouble **h, gdouble **s, gdouble **v,
		       gdouble **hn, gdouble **sn, gdouble **vn,
		       gdouble **haux, gdouble **saux, gdouble **vaux);
static gdouble CML_next_value (gdouble *vec, gint pos, gint size,
			       gdouble c1, gdouble c2,
			       CML_PARAM *param, gdouble aux);
static void	rgb_to_hsv (int *r, int *g, int *b);
static void	hsv_to_rgb (int *h, int *s, int *v);
static gdouble	logistic_function (CML_PARAM *param, gdouble x, gdouble power);
static gint	DIALOG ();
static GtkWidget *CML_dialog_sub_panel_new (GtkWidget *parent,
					    gchar *name,
					    CML_PARAM *param,
					    gint channel_id);
static GtkWidget *CML_dialog_advanced_panel_new (GtkWidget *parent, gchar *name);
void	preview_update ();
static void	function_graph_new (GtkWidget *widget, gpointer data);
static void	CML_set_or_randomize_seed_callback (GtkWidget *widget, gpointer data);
static void	CML_copy_parameters_callback (GtkWidget *widget, gpointer data);
static void	CML_initial_value_sensitives_update ();
static void	CML_menu_update (GtkWidget *widget, gpointer data);
static void	CML_initial_value_menu_update (GtkWidget *widget, gpointer data);
static void	OK_CALLBACK (GtkWidget *widget, gpointer   data);
static void	CML_save_to_file_callback (GtkWidget *widget, gpointer client_data);
static void	CML_execute_save_to_file (GtkWidget *widget, gpointer client_data);
static gint	force_overwrite (char *filename);
static void	CML_overwrite_ok_callback (GtkWidget *widget, gpointer   data);
static void	CML_preview_update_callback (GtkWidget *widget, gpointer   data);
static void	CML_load_from_file_callback (GtkWidget *widget, gpointer client_data);
static gint	CML_load_parameter_file (gchar *filename, gint interactive_mode);
static void	CML_execute_load_from_file (GtkWidget *widget, gpointer client_data);
static gint	parse_line_to_gint (FILE *file, gint *flag);
static gdouble	parse_line_to_gdouble (FILE *file, gint *flag);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,				/* init_proc  */
  NULL,				/* quit_proc */
  query,			/* query_proc */
  run,				/* run_proc */
};

typedef struct
{
  gint run;
} Interface;

static Interface INTERFACE = { FALSE };

GtkWidget	*preview;
gtkW_widget_table	widget_pointers[4][CML_PARAM_NUM];
typedef struct
{
  GtkWidget	*widget;
  gint		logic;
} CML_sensitive_widget_table;
#define	RANDOM_SENSITIVES_NUM	5
CML_sensitive_widget_table	random_sensitives[RANDOM_SENSITIVES_NUM] =
{ { NULL, 0 }, { NULL, 0 }, { NULL, 0 }, { NULL, 0 }, { NULL, 0} };
gint		drawable_id = 0;
gint		copy_source = 0;
gint		copy_destination = 0;
gint		selective_load_source = 0;
gint		selective_load_destination = 0;
gint		CML_preview_defer = FALSE;
gint		overwritable = 0;

gdouble		*mem_chank0 = NULL;
gint		mem_chank0_size = 0;
guchar		*mem_chank1 = NULL;
gint		mem_chank1_size = 0;
guchar		*mem_chank2 = NULL;
gint		mem_chank2_size = 0;

MAIN ()

static void
query ()
{
  static GParamDef args [] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    { PARAM_IMAGE, "image", "Input image (not used)"},
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_STRING, "parameter_file_name", "the name of paremter file. CML_explorer makes an image with its settings." },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure (PLUG_IN_NAME,
			  "Make an image of Coupled-Map Lattice",
			  "Make an image of Coupled-Map Lattice (CML). CML is a kind of Cellula Automata on continuous (value) domain. In RUN_NONINTERACTIVE, the name of a prameter file is passed as the 4th arg. You can control CML_explorer via parameter file.",
			  /*  Or do you want to call me with over 50 args? */
			  "Shuji Narazaki (narazaki@InetQ.or.jp); http://www.inetq.or.jp/~narazaki/TheGIMP/",
			  "Shuji Narazaki",
			  "1997",
			  MENU_POSITION,
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char	*name,
     int	nparams,
     GParam	*param,
     int	*nreturn_vals,
     GParam	**return_vals)
{
  GParam	*values;
  GStatusType	status = STATUS_EXECUTION_ERROR;
  GRunModeType	run_mode;

  values = g_new (GParam, 1);
  run_mode = param[0].data.d_int32;
  drawable_id = param[2].data.d_drawable;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_NAME, &VALS);
      if (! DIALOG ())
	return;
#ifdef INTERACTIVE_DIALOG
      else
	status = STATUS_SUCCESS;
#endif
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
#ifdef INTERACTIVE_DIALOG
  if (run_mode != RUN_INTERACTIVE)
    status = MAIN_FUNCTION (FALSE);
#else
  status = MAIN_FUNCTION (FALSE);
#endif

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
MAIN_FUNCTION (gint preview_p)
{
  GDrawable	*drawable = NULL;
  GPixelRgn	dest_rgn, src_rgn;
  guchar	*dest_buffer = NULL;
  guchar	*src_buffer = NULL;
  gint		x1, x2, y1, y2;
  gint		dx, dy;
  gint		dest_has_alpha = FALSE;
  gint		dest_is_gray = FALSE;
  gint		src_has_alpha = FALSE;
  gint		src_is_gray = FALSE;
  gint		total, processed = 0;
  gint		keep_height = 1;
  gint		cell_num, width_by_pixel, height_by_pixel;
  gint		index;
  gint		src_bpp, src_bpl;
  gint		dest_bpp, dest_bpl;
  gdouble	*hues, *sats, *vals;
  gdouble	*newh, *news, *newv;
  gdouble	*haux, *saux, *vaux;

  /* open THE drawable */
  drawable = gimp_drawable_get (drawable_id);
  gimp_drawable_mask_bounds (drawable_id, &x1, &y1, &x2, &y2);
  src_has_alpha = dest_has_alpha = gimp_drawable_has_alpha (drawable_id);
  src_is_gray = dest_is_gray = gimp_drawable_gray (drawable_id);
  src_bpp = dest_bpp = (src_is_gray ? 1 : 3) + (src_has_alpha ? 1 : 0);

  if (preview_p)
    {
      dest_has_alpha = FALSE;
      dest_is_gray = FALSE;
      dest_bpp = 3;

      if (GTKW_PREVIEW_WIDTH < x2 - x1)	/* preview < drawable (selection) */
	x2 = x1 + GTKW_PREVIEW_WIDTH;
      if (GTKW_PREVIEW_HEIGHT < y2 - y1)
	y2 = y1 + GTKW_PREVIEW_HEIGHT;
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
	  rgb_to_hsv (rgbi, rgbi + 1, rgbi + 2);
	  hues[index] = (gdouble) rgbi[0] / (gdouble) 255;
	  sats[index] = (gdouble) rgbi[1] / (gdouble) 255;
	  vals[index] = (gdouble) rgbi[2] / (gdouble) 255;
	}
    }
  if (! preview_p) gimp_progress_init (PROGRESS_NAME);

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
	    hsv_to_rgb (&r, &g, &b);
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
			rgb_to_hsv (rgbi, rgbi + 1, rgbi + 2);

			r = (VALS.hue.function == CML_KEEP_VALUES) ? rgbi[0] : h;
			g = (VALS.sat.function == CML_KEEP_VALUES) ? rgbi[1] : s;
			b = (VALS.val.function == CML_KEEP_VALUES) ? rgbi[2] : v;
			hsv_to_rgb (&r, &g, &b);
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
CML_compute_next_step (gint size,
		       gdouble **h, gdouble **s, gdouble **v,
		       gdouble **hn, gdouble **sn, gdouble **vn,
		       gdouble **haux, gdouble **saux, gdouble **vaux)
{
  int	index;

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
CML_next_value (gdouble *vec, gint pos, gint size,
		gdouble c1, gdouble c2,
		CML_PARAM *param, gdouble power)
{
  gdouble	val = vec[pos];
  gdouble	diff = 0;
  gdouble	self_diff = 0;
  gdouble	by_env = 0;
  gdouble	self_mod_rate = 0;
  gdouble	hold_rate = 1 - param->mod_rate;
  gdouble	env_factor = 0;
  gint		index;

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
#undef	AVE_DIST
#undef LOGISTICS
#undef ENV_FACTOR
#undef C_ENV_FACTOR
#undef CHN_FACTOR
#undef C_CHN_FACTOR

static gdouble
logistic_function (CML_PARAM *param, gdouble x, gdouble power)
{
  gdouble x1 = x;
  gdouble result = 0;
  int n = param->range_num;
  int step;

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
	result = 0.5 * (sin (M_PI * ABS (x1 - 0.5) / power) / sin (M_PI * 0.5 / power) + 1);
      else
	result = 0.5 * (pow (sin (M_PI * ABS (x1 - 0.5)), power) + 1);
      if (x1 < 0.5) result = 1 - result;
      break;
    case CML_SIN_CURVE_STEP:
      if (1.0 < power)
	result = 0.5 * (sin (M_PI * ABS (x1 - 0.5) / power) / sin (M_PI * 0.5 / power) + 1);
      else
	result = 0.5 * (pow (sin (M_PI * ABS (x1 - 0.5)), power) + 1);
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

static void
rgb_to_hsv (int *r,
	    int *g,
	    int *b)
{
  int red, green, blue;
  float h = 0 , s, v;
  int min, max;
  int delta;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  v = max;

  if (max != 0)
    s = ((max - min) * 255) / (float) max;
  else
    s = 0;

  if (s == 0)
    h = 0;
  else
    {
      delta = max - min;
      if (red == max)
	h = (green - blue) / (float) delta;
      else if (green == max)
	h = 2 + (blue - red) / (float) delta;
      else if (blue == max)
	h = 4 + (red - green) / (float) delta;
      h *= 42.5;

      if (h < 0)
	h += 255;
      if (h > 255)
	h -= 255;
    }

  *r = h;
  *g = s;
  *b = v;
}

static void
hsv_to_rgb (int *h,
	    int *s,
	    int *v)
{
  float hue, saturation, value;
  float f, p, q, t;

  if (*s == 0)
    {
      *h = *v;
      *s = *v;
      *v = *v;
    }
  else
    {
      hue = *h * 6.0 / 255.0;
      if (hue == 6.0)
	hue = 0.0;
      saturation = *s / 255.0;
      value = *v / 255.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - (saturation * f));
      t = value * (1.0 - (saturation * (1.0 - f)));

      switch ((int) hue)
	{
	case 0:
	  *h = value * 255;
	  *s = t * 255;
	  *v = p * 255;
	  break;
	case 1:
	  *h = q * 255;
	  *s = value * 255;
	  *v = p * 255;
	  break;
	case 2:
	  *h = p * 255;
	  *s = value * 255;
	  *v = t * 255;
	  break;
	case 3:
	  *h = p * 255;
	  *s = q * 255;
	  *v = value * 255;
	  break;
	case 4:
	  *h = t * 255;
	  *s = p * 255;
	  *v = value * 255;
	  break;
	case 5:
	  *h = value * 255;
	  *s = p * 255;
	  *v = q * 255;
	  break;
	}
    }
}

/* dialog stuff */
static int
DIALOG ()
{
  GtkWidget	*dlg;
  GtkWidget	*hbox;
  GtkWidget	*button;
  GtkTooltips *tooltips;
  gchar		**argv;
  gint		argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup (PLUG_IN_NAME);
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  
  dlg = gtkW_dialog_new ("Coupled-Map-Lattice Explorer",
			 (GtkSignalFunc) OK_CALLBACK,
			 (GtkSignalFunc) gtkW_close_callback);

  memset(&widget_pointers, (int)0, sizeof(widget_pointers));

  CML_preview_defer = TRUE;
  hbox = gtkW_hbox_new (GTK_DIALOG (dlg)->vbox);
  {
    GtkWidget *table;
    GtkWidget *frame;
    GtkWidget *preview_box;
    GtkWidget *preview_table;
    GtkWidget *preview_frame;
    GtkWidget *hseparator;

    table = gtkW_table_new (hbox, 8, 1);
    frame = gtkW_frame_new (NULL, "Preview");
    gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
		      0, 0, 0, gtkW_border_width);
    preview_box = gtkW_hbox_new (frame);
    gtkW_border_width = 0;
    preview_table = gtkW_table_new (preview_box, 1, 1);
    gtkW_border_width = GTKW_BORDER_WIDTH;
    gtkW_frame_shadow_type = GTK_SHADOW_IN;
    preview_frame = gtkW_frame_new (NULL, NULL);
    gtkW_frame_shadow_type = GTK_SHADOW_ETCHED_IN;
    gtk_widget_set_usize (preview_frame, 0, 0);

    gtk_container_border_width (GTK_CONTAINER (preview_frame), 0);
    gtk_table_attach (GTK_TABLE (preview_table), preview_frame,
		      0, 1, 0, 1, 0, 0, 0, 0);

    /* I don't understand well about the following codes */
    gdk_set_use_xshm (gimp_use_xshm ());
    gtk_preview_set_gamma (gimp_gamma ());
    gtk_preview_set_install_cmap (gimp_install_cmap ());
    {
      guchar *color_cube;
      color_cube = gimp_color_cube ();
      gtk_preview_set_color_cube (color_cube[0], color_cube[1],
				  color_cube[2], color_cube[3]);
    }
    gtk_widget_set_default_visual (gtk_preview_get_visual ());
    gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

    preview = gtk_preview_new (GTK_PREVIEW_COLOR);
    gtk_preview_size (GTK_PREVIEW (preview), GTKW_PREVIEW_WIDTH, GTKW_PREVIEW_HEIGHT);
    gtk_container_add (GTK_CONTAINER (preview_frame), preview);
    gtk_widget_show (preview);

    {
      GtkWidget *hbox = gtkW_hbox_new (NULL);

      gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 1, 2,
			GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
    }

    button = gtkW_table_add_button (table, "New seed", 0, 1, 2,
				    (GtkSignalFunc) CML_preview_update_callback,
				    &VALS);
    random_sensitives[0].widget = button;
    random_sensitives[0].logic = TRUE;

    button = gtkW_table_add_button (table, "Fix seed", 0, 1, 3,
				    (GtkSignalFunc) CML_set_or_randomize_seed_callback,
				    &VALS);
    random_sensitives[1].widget = button;
    random_sensitives[1].logic = TRUE;

    button = gtkW_table_add_button (table, "Random seed", 0, 1, 4,
				    (GtkSignalFunc) CML_set_or_randomize_seed_callback,
				    &VALS);
    random_sensitives[2].widget = button;
    random_sensitives[2].logic = FALSE;

    hseparator = gtk_hseparator_new ();
    gtk_widget_show (hseparator);
    gtk_table_attach (GTK_TABLE (table), hseparator, 0, 1, 5, 6,
		      GTK_FILL, GTK_FILL,
		      gtkW_border_width, 2 * gtkW_border_width);

    gtkW_table_add_button (table, "Load", 0, 1, 6,
			   (GtkSignalFunc) CML_load_from_file_callback,
			   &VALS);
    gtkW_table_add_button (table, "Save", 0, 1, 7,
			   (GtkSignalFunc) CML_save_to_file_callback,
			   &VALS);
  }

  {
    GtkWidget *notebook;
    GtkWidget *label;
    GtkWidget *page;
    GtkWidget *parent = hbox;

    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_box_pack_start (GTK_BOX (parent), notebook, TRUE, TRUE, 0);
    gtk_widget_show (notebook);
    parent = NULL;

    page = CML_dialog_sub_panel_new (parent, "Hue settings", &VALS.hue, 0);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
			      gtk_label_new ("Hue"));

    page = CML_dialog_sub_panel_new (parent, "Saturation settings", &VALS.sat, 1);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
			      gtk_label_new ("Saturation"));

    page = CML_dialog_sub_panel_new (parent, "Value (grayimage) settings", &VALS.val, 2);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
			      gtk_label_new ("Value"));

    page = CML_dialog_advanced_panel_new (parent, "Advanced settings");
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
			      gtk_label_new ("Advanced"));

    {
      GtkWidget	*table;
      GtkWidget *subtable;
      GtkWidget *frame;
      GtkWidget *subframe;
      GtkWidget *box;
      GtkWidget *vbox;
      gint	index;
      gint	sindex;

      frame = gtkW_frame_new (NULL, "Other parameter settings");
      vbox = gtkW_vbox_new (frame);
      table = gtkW_table_new (vbox, 6, 1);
      index = 0;
      subframe = gtkW_frame_new (NULL, "Channel independed parameters");
      box = gtkW_vbox_new (subframe);
      subtable = gtkW_table_new (box, 3, 2);
      sindex = 0;
      gtkW_table_add_menu (subtable, "Initial value", 0, sindex++,
			   (GtkSignalFunc) CML_initial_value_menu_update,
			   &VALS.initial_value,
			   initial_value_menu,
			   sizeof (initial_value_menu) / sizeof (initial_value_menu[0]),
			   &widget_pointers[3][0]);
      gtkW_table_add_iscale_entry (subtable, "Zoom scale", 0, sindex++,
				   (GtkSignalFunc) gtkW_iscale_update,
				   (GtkSignalFunc) gtkW_ientry_update,
				   &VALS.scale,
				   1, 10, 1, &widget_pointers[3][1]);
      gtkW_table_add_iscale_entry (subtable, "Start offset", 0, sindex++,
				   (GtkSignalFunc) gtkW_iscale_update,
				   (GtkSignalFunc) gtkW_ientry_update,
				   &VALS.start_offset,
				   0, 100, 1, &widget_pointers[3][2]);

      gtk_table_attach (GTK_TABLE (table), subframe, 0, 1, index, index + 1 ,
			GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_SHRINK, 0, 0);
      index++;

      {
	GtkWidget *hbox = gtkW_hbox_new (NULL);

	gtkW_border_width = 0;
	gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, index, index + 1,
			  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtkW_border_width = GTKW_BORDER_WIDTH;
	index++;
      }

      subframe = gtkW_frame_new (NULL, "Seed of random (only for \"from seed\" modes)");
      box = gtkW_vbox_new (subframe);
      subtable = gtkW_table_new (box, 1, 2);
      sindex = 0;
      gtkW_table_add_iscale_entry (subtable, "Seed", 0, sindex++,
				   (GtkSignalFunc) gtkW_iscale_update,
				   (GtkSignalFunc) gtkW_ientry_update,
				   &VALS.seed, 0, 1 << 15, 1,
				   &widget_pointers[3][3]);
      random_sensitives[3].widget = subtable;
      random_sensitives[3].logic = FALSE;
      {
	GtkWidget *button;

	button = gtkW_vbox_add_button (box, "Switch to \"from seed\" with the last seed",
				       (GtkSignalFunc) CML_set_or_randomize_seed_callback,
				       &VALS);
	random_sensitives[4].widget = button;
	random_sensitives[4].logic = TRUE;

	tooltips = gtkW_tooltips_new (frame);
	gtk_tooltips_set_tip  (tooltips, button, "\"Fix seed\" button is an alias of me.\nThe same seed produces the same image, if (1) the widths of images are same (this is the reason why image on drawable is different from preview), and (2) all mutation rates equal to zero.",NULL);
	gtk_tooltips_enable (tooltips);
      }
      gtk_table_attach (GTK_TABLE (table), subframe, 0, 1, index, index + 1,
			GTK_FILL|GTK_EXPAND, GTK_SHRINK|GTK_FILL, 0, 0);
      index++;

      {
	GtkWidget *hbox = gtkW_hbox_new (NULL);

	gtkW_border_width = 0;
	gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, index, index + 1,
			  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtkW_border_width = GTKW_BORDER_WIDTH;
	index++;
      }

      label = gtk_label_new ("Others");
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);
    }
    {
      GtkWidget	*table, *subtable, *frame, *subframe, *box, *vbox;
      int	index, sindex;

      frame = gtkW_frame_new (NULL, "Misc operations");
      vbox = gtkW_vbox_new (frame);
      table = gtkW_table_new (vbox, 4, 1);
      index = 0;

      subframe = gtkW_frame_new (NULL, "Copy settings");
      box = gtkW_vbox_new (subframe);
      subtable = gtkW_table_new (box, 3, 2);
      sindex = 0;
      gtkW_table_add_menu (subtable, "Source ch.", 0, sindex++,
			   (GtkSignalFunc) gtkW_menu_update,
			   &copy_source,
			   channel_menu,
			   sizeof (channel_menu) / sizeof (channel_menu[0]),
			   NULL);
      gtkW_table_add_menu (subtable, "Destination ch.", 0, sindex++,
			   (GtkSignalFunc) gtkW_menu_update,
			   &copy_destination,
			   channel_menu,
			   sizeof (channel_menu) / sizeof (channel_menu[0]),
			   NULL);
      gtkW_table_add_button (subtable, "Do copy parameters",
			     0, 2, sindex++,
			     (GtkSignalFunc) CML_copy_parameters_callback,
			     &VALS);
      gtk_table_attach (GTK_TABLE (table), subframe, 0, 1, index, index + 1,
			GTK_FILL, GTK_FILL, 0, 0);
      index++;

      {
	GtkWidget *hbox = gtkW_hbox_new (NULL);

	gtkW_border_width = 0;
	gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, index, index + 1,
			  GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
	gtkW_border_width = GTKW_BORDER_WIDTH;
	index++;
      }

      subframe = gtkW_frame_new (NULL, "Selective load settings");
      box = gtkW_vbox_new (subframe);
      subtable = gtkW_table_new (box, 2, 2);
      sindex = 0;
      gtkW_table_add_menu (subtable, "Source ch. in file", 0, sindex++,
			   (GtkSignalFunc) gtkW_menu_update,
			   &selective_load_source,
			   sload_menu,
			   sizeof (sload_menu) / sizeof (sload_menu[0]),
			   NULL);
      gtkW_table_add_menu (subtable, "Destination ch.", 0, sindex++,
			   (GtkSignalFunc) gtkW_menu_update,
			   &selective_load_destination,
			   sload_menu,
			   sizeof (sload_menu) / sizeof (sload_menu[0]),
			   NULL);
      gtk_table_attach (GTK_TABLE (table), subframe, 0, 1, index, index + 1,
			GTK_FILL, GTK_FILL, 0, 0);
      index++;

      {
	GtkWidget *hbox = gtkW_hbox_new (NULL);

	gtkW_border_width = 0;
	gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, index, index + 1,
			  GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
	gtkW_border_width = GTKW_BORDER_WIDTH;
	index++;
      }

      label = gtk_label_new ("Misc ops.");
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);
    }
  }
  CML_initial_value_sensitives_update ();

  /* Displaying preview might takes a long time. Thus, fisrt, dialog itself
     should be shown before making preview in it. */
  gtk_widget_show (dlg);
  gdk_flush ();
  CML_preview_defer = FALSE;
  preview_update ();

  gtk_main ();

  gtk_object_unref (GTK_OBJECT (tooltips));

  gdk_flush ();

  return INTERFACE.run;
}


static GtkWidget *
CML_dialog_sub_panel_new (GtkWidget	*parent,
			  gchar	*name,
			  CML_PARAM	*param,
			  gint		channel_id)
{
  GtkWidget *subframe, *table;
  gpointer *chank;
  int	index = 0;

  subframe = gtkW_frame_new (parent, name);
  table = gtkW_table_new (subframe, 14, 2);

  gtkW_table_add_menu (table, "Function type", 0, index,
		       (GtkSignalFunc) CML_menu_update,
		       &param->function,
		       function_menu,
		       sizeof (function_menu) / sizeof (function_menu[0]),
		       &widget_pointers[channel_id][index]);
  index++;
  gtkW_table_add_menu (table, "Composition", 0, index,
		       (GtkSignalFunc) CML_menu_update,
		       &param->composition,
		       composition_menu,
		       sizeof (composition_menu) / sizeof (composition_menu[0]),
		       &widget_pointers[channel_id][index]);
  index++;
  gtkW_table_add_menu (table, "Misc arrange", 0, index,
		       (GtkSignalFunc) CML_menu_update,
		       &param->arrange,
		       arrange_menu,
		       sizeof (arrange_menu) / sizeof (arrange_menu[0]),
		       &widget_pointers[channel_id][index]);
  index++;
  gtkW_table_add_toggle (table, "Use cyclic range", 0, index,
			 (GtkSignalFunc) gtkW_toggle_update,
			 &param->cyclic_range,
			 &widget_pointers[channel_id][index]);
  index++;
  gtkW_table_add_dscale_entry (table, "Mod. rate", 0, index,
			       (GtkSignalFunc) gtkW_dscale_update,
			       (GtkSignalFunc) gtkW_dentry_update,
			       &param->mod_rate,
			       0.0, 1.0, 0.01,
			       &widget_pointers[channel_id][index]);
  index++;
  gtkW_table_add_dscale_entry (table, "Env. sensitivity", 0, index,
			       (GtkSignalFunc) gtkW_dscale_update,
			       (GtkSignalFunc) gtkW_dentry_update,
			       &param->env_sensitivity,
			       0.0, 1.0, 0.01,
			       &widget_pointers[channel_id][index]);
  index++;
  gtkW_table_add_iscale_entry (table, "Diffusion dist.", 0, index,
			       (GtkSignalFunc) gtkW_iscale_update,
			       (GtkSignalFunc) gtkW_ientry_update,
			       &param->diffusion_dist,
			       2, 10, 1,
			       &widget_pointers[channel_id][index]);
  index++;
  gtkW_table_add_iscale_entry (table, "# of subranges", 0, index,
			       (GtkSignalFunc) gtkW_iscale_update,
			       (GtkSignalFunc) gtkW_ientry_update,
			       &param->range_num,
			       1, 10, 1,
			       &widget_pointers[channel_id][index]);
  index++;
  gtkW_table_add_dscale_entry (table, "P(ower factor)", 0, index,
			       (GtkSignalFunc) gtkW_dscale_update,
			       (GtkSignalFunc) gtkW_dentry_update,
			       &param->power,
			       0.0, 10.0, 0.01,
			       &widget_pointers[channel_id][index]);
  index++;
  gtkW_table_add_dscale_entry (table, "Parameter k", 0, index,
			       (GtkSignalFunc) gtkW_dscale_update,
			       (GtkSignalFunc) gtkW_dentry_update,
			       &param->parameter_k,
			       0.0, 10.0, 0.01,
			       &widget_pointers[channel_id][index]);
  index++;
  gtkW_table_add_dscale_entry (table, "Range low", 0, index,
			       (GtkSignalFunc) gtkW_dscale_update,
			       (GtkSignalFunc) gtkW_dentry_update,
			       &param->range_l,
			       0, 1.0, 0.01,
			       &widget_pointers[channel_id][index]);
  index++;
  gtkW_table_add_dscale_entry (table, "Range high", 0, index,
			       (GtkSignalFunc) gtkW_dscale_update,
			       (GtkSignalFunc) gtkW_dentry_update,
			       &param->range_h,
			       0, 1.0, 0.01,
			       &widget_pointers[channel_id][index]);
  index++;
  {
    GtkWidget *hbox = gtkW_hbox_new (NULL);

    gtkW_border_width = 0;
    gtk_table_attach (GTK_TABLE (table), hbox, 0, 2, index, index + 1,
		      0, GTK_FILL|GTK_EXPAND, 0, 0);
    gtkW_border_width = GTKW_BORDER_WIDTH;
    index++;
  }
  chank = (gpointer *) g_malloc (2 *sizeof (gpointer));
  chank[0] = (gpointer) channel_id;
  chank[1] = (gpointer) param;
  gtkW_table_add_button (table, "Plot the graph of the settings",
			 0, 2, index++,
			 (GtkSignalFunc) function_graph_new, chank);

  return subframe;
}

static GtkWidget *
CML_dialog_advanced_panel_new (GtkWidget *parent, gchar *name)
{
  GtkWidget	*frame;
  GtkWidget	*subframe;
  GtkWidget	*table;
  GtkWidget	*subtable;
  gint		index = 0;
  gint		widget_offset = 12;
  gint		channel_id;
  gchar	*ch_name[] = { "Hue", "Saturation", "Value" };
  CML_PARAM	*param;

  frame = gtkW_frame_new (parent, name);
  table = gtkW_table_new (frame, 3, 1);

  for (channel_id = 0; channel_id < 3; channel_id++)
    {
      param = (CML_PARAM *)&VALS + channel_id;

      subframe = gtkW_frame_new (NULL, ch_name[channel_id]);
      gtk_table_attach (GTK_TABLE (table), subframe,
			0, 1, channel_id, channel_id + 1,
			GTK_FILL, GTK_FILL, 0, 0);
      subtable = gtkW_table_new (subframe, 3, 2);

      index = 0;
      gtkW_table_add_dscale_entry (subtable, "Ch. sensitivity", 0, index,
				   (GtkSignalFunc) gtkW_dscale_update,
				   (GtkSignalFunc) gtkW_dentry_update,
				   &param->ch_sensitivity,
				   0.0, 1.0, 0.01,
				   &widget_pointers[channel_id][index + widget_offset]);
      index++;
      gtkW_table_add_dscale_entry (subtable, "Mutation rate", 0, index,
				   (GtkSignalFunc) gtkW_dscale_update,
				   (GtkSignalFunc) gtkW_dentry_update,
				   &param->mutation_rate,
				   0, 1.0, 0.01,
				   &widget_pointers[channel_id][index + widget_offset]);
      index++;
      gtkW_table_add_dscale_entry (subtable, "Mutation dist.", 0, index,
				   (GtkSignalFunc) gtkW_dscale_update,
				   (GtkSignalFunc) gtkW_dentry_update,
				   &param->mutation_dist,
				   0.0, 1.0, 0.01,
				   &widget_pointers[channel_id][index + widget_offset]);
    }
  return frame;
}

void
preview_update ()
{
  if (CML_preview_defer == FALSE)
    MAIN_FUNCTION (TRUE);
}

static void
function_graph_new (GtkWidget *widget, gpointer data)
{
  GtkWidget	*dlg;
  GtkWidget	*button;
  GtkWidget	*frame;
  GtkWidget	*vbox;
  GtkWidget	*preview;
  gint		channel_id = *(int *) data;
  CML_PARAM	*param = (CML_PARAM *) *((gpointer *)data + 1);

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Graph of the current settings");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) gtkW_close_callback, NULL);
  /* Action Area */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT(dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  frame = gtkW_frame_new (GTK_DIALOG (dlg)->vbox, "The Graph");
  vbox = gtkW_vbox_new (frame);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), 256, 256);
  gtk_box_pack_start (GTK_BOX (vbox), preview, TRUE, TRUE, 5);
  {
    int	x, y, last_y, yy;
    guchar rgbc[3];
    int rgbi[3];
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
	hsv_to_rgb (rgbi, rgbi+1, rgbi+2);
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
    y = 255 * CLAMP (logistic_function (param, x/(gdouble)255, param->power),
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
CML_set_or_randomize_seed_callback (GtkWidget *widget, gpointer data)
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
CML_copy_parameters_callback (GtkWidget *widget, gpointer data)
{
  int index;
  gtkW_widget_table *widgets;

  if (copy_source == copy_destination)
    {
      gtkW_message_dialog (TRUE, "Warning: the source and the destination are the same channel.");
      gdk_flush ();
      return;
    }
  memcpy (channel_menu[copy_destination].data,
	  channel_menu[copy_source].data,
	  sizeof (CML_PARAM));
  CML_preview_defer = TRUE;
  widgets = widget_pointers[copy_destination];
  for (index = 0; index < CML_PARAM_NUM; index++)
    if (widgets[index].widget && widgets[index].updater)
      (widgets[index].updater) (widgets+index);

  gdk_flush ();
  CML_preview_defer = FALSE;
  preview_update ();
  /* gtk_main_quit (); */
}

static void
CML_menu_update (GtkWidget *widget, gpointer data)
{
  gint	**buffer = (gint **) data;

  if (*buffer[1] != (gint) buffer[2])
    {
      *buffer[1] = (gint) buffer[2];
      preview_update ();
    }
}

static void
CML_initial_value_sensitives_update ()
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
CML_initial_value_menu_update (GtkWidget *widget, gpointer data)
{
  gint	**buffer = (gint **) data;

  if (*buffer[1] != (gint) buffer[2])
    {
      *buffer[1] = (gint) buffer[2];
      CML_initial_value_sensitives_update ();
      preview_update ();
    }
}

static void
OK_CALLBACK (GtkWidget *widget,
	      gpointer   data)
{
  INTERFACE.run = TRUE;
#ifdef INTERACTIVE_DIALOG
  MAIN_FUNCTION (FALSE);
#endif
  gtk_widget_destroy (GTK_WIDGET (data));
}

#ifdef INTERACTIVE_DIALOG
static GtkWidget *
CML_dialog_new (char * name,
		 GtkSignalFunc execute_callback,
		 GtkSignalFunc ok_callback,
		 GtkSignalFunc close_callback)
{
  GtkWidget *dlg, *button;

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), name);
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) close_callback, NULL);

  /* Action Area */
  button = gtk_button_new_with_label ("Execute");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) execute_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Execute and Exit");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ok_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Exit");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT(dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_show (button);

  return dlg;
}
#endif

static void
CML_save_to_file_callback (GtkWidget *widget, gpointer client_data)
{
  GtkWidget *filesel;

  filesel = gtk_file_selection_new ("Save parameters to");
  gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", (GtkSignalFunc) CML_execute_save_to_file,
		      filesel);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			     "clicked", (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (GTK_WINDOW (filesel)));

  if (strlen (VALS.last_file_name) > 0)
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				     VALS.last_file_name);

  gtk_widget_show (filesel);
}

static void
CML_execute_save_to_file (GtkWidget *widget, gpointer client_data)
{
  char	*filename;
  struct stat buf;
  FILE	*file = NULL;
  int	channel_id;
  int	err;

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (client_data));
  if (! filename)
    return;

  err = stat (filename, &buf);	/* returns zero if success */
  if ((err == 0) || (errno == ENOENT))
    {
      if (errno == ENOENT)
	file = fopen (filename, "w");
      else if (buf.st_mode & S_IFDIR)
	{
	  GString *s = g_string_new (filename);

	  if (filename[strlen (filename) - 1] != '/')
	    g_string_append_c (s, '/');
	  gtk_file_selection_set_filename (GTK_FILE_SELECTION (client_data),
					   s->str);
	  g_string_free (s, TRUE);
	  return;
	}
      else if (buf.st_mode & S_IFREG) /* already exists */
	{
#ifdef	VERBOSE_DIALOGS
	if (! force_overwrite (filename))
	  return;
	else
	  file = fopen (filename, "w");
#else
      file = fopen (filename, "w");
#endif
	}
    }
  if ((err != 0) && (file == NULL))
    {
      gchar buffer[CML_LINE_SIZE];

      sprintf (buffer, "Error: could not open \"%s\"", filename);
      gtkW_message_dialog (TRUE, buffer);
      return;
    }
  else
    {
      fprintf (file, "; This is a parameter file for CML_explorer\n");
      fprintf (file, "; File format version: %1.1f\n", PARAM_FILE_FORMAT_VERSION);
      fprintf (file, ";\n");
      for (channel_id = 0; channel_id < 3; channel_id++)
	{
	  CML_PARAM param = *(CML_PARAM *)(channel_menu[channel_id].data);

	  fprintf (file, "\t%s\n", channel_menu[channel_id].name);
	  fprintf (file, "Function_type    : %d (%s)\n",
		   param.function, function_menu[param.function].name);
	  fprintf (file, "Compostion_type  : %d (%s)\n",
		   param.composition, composition_menu[param.composition].name);
	  fprintf (file, "Arrange          : %d (%s)\n",
		   param.arrange, arrange_menu[param.arrange].name);
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
	       VALS.initial_value, initial_value_menu[VALS.initial_value].name);
      fprintf (file, "Zoom scale     : %d\n", VALS.scale);
      fprintf (file, "Start offset   : %d\n", VALS.start_offset);
      fprintf (file, "Random seed    : %d\n", VALS.seed);
      fclose(file);
#ifdef	VERBOSE_DIALOGS
      {
	gchar buffer[CML_LINE_SIZE];

	sprintf (buffer, "Parameters were saved to \"%s\"", filename);
	gtkW_message_dialog (TRUE, buffer);
      }
#endif
      if ( sizeof (VALS.last_file_name) <= strlen (filename))
	filename[sizeof (VALS.last_file_name) - 1] = '\0';
      strcpy (VALS.last_file_name, filename);
    }
  gtk_widget_destroy(GTK_WIDGET(client_data));
}

static gint
force_overwrite (char *filename)
{
  GtkWidget	*dlg;
  GtkWidget	*label;
  GtkWidget	*table;
  gchar		buffer[CML_LINE_SIZE];
  gint		tmp;

  dlg = gtkW_dialog_new ("CML file operation warning",
			 (GtkSignalFunc) CML_overwrite_ok_callback,
			 (GtkSignalFunc) gtkW_close_callback);

  table = gtkW_table_new (NULL, 1, 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);

  sprintf (buffer, "%s exists, overwrite?", filename);
  label = gtk_label_new (buffer);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL|GTK_EXPAND,
		    0, 0, 0);
  gtk_widget_show (label);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  tmp = overwritable;
  overwritable = 0;

  return tmp;
}

static void
CML_overwrite_ok_callback (GtkWidget *widget, gpointer   data)
{
  overwritable = 1;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
CML_preview_update_callback (GtkWidget *widget, gpointer   data)
{
  gtkW_widget_table seed_widget = widget_pointers[3][3];

  preview_update ();

  CML_preview_defer = TRUE;
  if (seed_widget.widget && seed_widget.updater)
	  (seed_widget.updater) (&seed_widget);
  CML_preview_defer = FALSE;
}

static void
CML_load_from_file_callback (GtkWidget *widget, gpointer client_data)
{
  GtkWidget *filesel;

  if ((selective_load_source == 0) || (selective_load_destination == 0))
    filesel = gtk_file_selection_new ("Load parameters from");
  else
    filesel = gtk_file_selection_new ("Selective load from");
  gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", (GtkSignalFunc) CML_execute_load_from_file,
		      filesel);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			     "clicked", (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (GTK_WINDOW (filesel)));

  if (strlen (VALS.last_file_name) > 0)
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				     VALS.last_file_name);

  gtk_widget_show (filesel);
}

static void
CML_execute_load_from_file (GtkWidget *widget, gpointer client_data)
{
  char	*filename;
  int	channel_id;
  int	flag = TRUE;

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (client_data));

  flag = CML_load_parameter_file (filename, TRUE);
  gtk_widget_destroy(GTK_WIDGET(client_data));

  if (flag)
    {
      int index;
      gtkW_widget_table *widgets;

      CML_preview_defer = TRUE;
      for (channel_id = 0; channel_id < 3; channel_id++)
	{
	  widgets = widget_pointers[channel_id];
	  for (index = 0; index < CML_PARAM_NUM; index++)
	    if (widgets[index].widget && widgets[index].updater)
	      (widgets[index].updater) (widgets+index);
	}
      /* channel independent parameters */
      widgets = widget_pointers[3];
      for (index = 0; index < 4; index++)
	if (widgets[index].widget && widgets[index].updater)
	  (widgets[index].updater) (widgets+index);

      gdk_flush ();
      CML_preview_defer = FALSE;
      preview_update ();
    }
}

static gint
CML_load_parameter_file (gchar *filename, gint interactive_mode)
{
  FILE	*file;
  int	channel_id;
  int	flag = TRUE;
  CML_PARAM ch[3];
  gint	initial_value = 0;
  gint	scale = 1;
  gint	start_offset = 0;
  gint	seed = 0;
  gint	old2new_function_id[] = { 3, 4, 5, 6, 7, 9, 10, 11, 1, 2 };

  file = fopen (filename, "r");

  if (!file)
    {
      gchar buffer[CML_LINE_SIZE];

      if (interactive_mode)
	{
	  sprintf (buffer, "Error: could not open \"%s\"", filename);
	  gtkW_message_dialog (TRUE, buffer);
	}
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
	    gtkW_message_dialog (TRUE, "Error: it's not CML parameter file.");
	  fclose(file);
	  return FALSE;
	}
      if (interactive_mode)
	{
	  if (version < PARAM_FILE_FORMAT_VERSION)
	    gtkW_message_dialog (TRUE, "Warning: it's an old format file.");
	  if (PARAM_FILE_FORMAT_VERSION < version)
	    gtkW_message_dialog (TRUE, "Warning: Hmmm, it's a parameter file for newer CML_explorer than me.");
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
	gtkW_message_dialog (TRUE, "Error: failed to load paramters");
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
parse_line_to_gint (FILE *file, gint *flag)
{
  gchar line[CML_LINE_SIZE];
  gchar *str;
  gint	value;

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
parse_line_to_gdouble (FILE *file, gint *flag)
{
  gchar line[CML_LINE_SIZE];
  gchar *str;
  gdouble value;

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

/* gtkW functions */
/* gtkW callback */
static void
gtkW_close_callback (GtkWidget *widget,
		     gpointer   data)
{
  gtk_main_quit ();
}

static void
gtkW_toggle_update (GtkWidget *widget,
		    gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
  preview_update ();
}

static void
gtkW_iscale_update (GtkAdjustment *adjustment,
		    gpointer       data)
{
  GtkWidget *entry;
  gchar buffer[32];
  int *val;

  val = data;
  if (*val != (int) adjustment->value)
    {
      *val = adjustment->value;
      entry = gtk_object_get_user_data (GTK_OBJECT (adjustment));
      sprintf (buffer, "%d", (int) adjustment->value);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      preview_update ();
    }
}

static void
gtkW_ientry_update (GtkWidget *widget,
		    gpointer   data)
{
  GtkAdjustment *adjustment;
  int new_val;
  int *val;

  val = data;
  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));

  if (*val != new_val)
    {
      adjustment = gtk_object_get_user_data (GTK_OBJECT (widget));

      if ((new_val >= adjustment->lower) &&
	  (new_val <= adjustment->upper))
	{
	  *val = new_val;
	  adjustment->value = new_val;
	  gtk_signal_emit_by_name (GTK_OBJECT (adjustment), "value_changed");
	  preview_update ();
	}
    }
}

static void
gtkW_dscale_update (GtkAdjustment *adjustment,
		    gpointer       data)
{
  GtkWidget *entry;
  gchar buffer[32];
  gdouble *val;

  val = data;
  if (GTKW_FLOAT_MIN_ERROR < ABS (*val - (gdouble) adjustment->value))
    {
      *val = adjustment->value;
      entry = gtk_object_get_user_data (GTK_OBJECT (adjustment));
      sprintf (buffer, "%6f", (gdouble) adjustment->value);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      preview_update ();
    }
}

static void
gtkW_dentry_update (GtkWidget *widget,
		    gpointer   data)
{
  GtkAdjustment *adjustment;
  gdouble new_val;
  gdouble *val;

  val = data;
  new_val = atof (gtk_entry_get_text (GTK_ENTRY (widget)));

  if (GTKW_FLOAT_MIN_ERROR < ABS (*val - new_val))
    {
      adjustment = gtk_object_get_user_data (GTK_OBJECT (widget));

      if ((new_val >= adjustment->lower) &&
	  (new_val <= adjustment->upper))
	{
	  *val = new_val;
	  adjustment->value = new_val;
	  gtk_signal_emit_by_name (GTK_OBJECT (adjustment), "value_changed");
	  preview_update ();
	}
    }
}

static void
gtkW_dscale_entry_change_value (gtkW_widget_table *wtable)
{
  GtkWidget *entry;
  gchar buffer[32];
  GtkAdjustment *adjustment = (GtkAdjustment *) (wtable->widget);

  adjustment->value = *(gdouble *)(wtable->value);
  gtk_signal_emit_by_name (GTK_OBJECT (adjustment), "value_changed");
  entry = gtk_object_get_user_data (GTK_OBJECT (adjustment));
  sprintf (buffer, "%6f", (gdouble) adjustment->value);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  preview_update ();
}

static void
gtkW_iscale_entry_change_value (gtkW_widget_table *wtable)
{
  GtkWidget *entry;
  gchar buffer[32];
  GtkAdjustment *adjustment = (GtkAdjustment *) (wtable->widget);

  /*  adustment->value is double, that is not precise to hold long interger. */
  adjustment->value = *(gint *)(wtable->value);
  gtk_signal_emit_by_name (GTK_OBJECT (adjustment), "value_changed");
  entry = gtk_object_get_user_data (GTK_OBJECT (adjustment));
  sprintf (buffer, "%d", (gint) adjustment->value);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  preview_update ();
}

static void
gtkW_menu_change_value (gtkW_widget_table *wtable)
{
  gtk_option_menu_set_history (GTK_OPTION_MENU (wtable->widget),
			       *(gint *)(wtable->value));
}

static void
gtkW_menu_update (GtkWidget *widget,
		  gpointer   data)
{
  gint	**buffer = (gint **) data;

  if (*buffer[1] != (gint) buffer[2])
    *buffer[1] = (gint) buffer[2];
}

static void
gtkW_toggle_change_value (gtkW_widget_table *wtable)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wtable->widget),
				*(gint *)(wtable->value));
}

/* gtkW method */

GtkWidget *
gtkW_check_button_new (GtkWidget	*parent,
		       gchar		*name,
		       GtkSignalFunc	update,
		       gint		*value)
{
  GtkWidget *toggle;

  toggle = gtk_check_button_new_with_label (name);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) update,
		      value);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *value);
  gtk_container_add (GTK_CONTAINER (parent), toggle);
  gtk_widget_show (toggle);
  return toggle;
}

static GtkWidget *
gtkW_dialog_new (gchar		*name,
		 GtkSignalFunc	ok_callback,
		 GtkSignalFunc	close_callback)
{
  GtkWidget *dlg, *button;

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), name);
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) close_callback, NULL);

  /* Action Area */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ok_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT(dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_show (button);

  return dlg;
}

static GtkWidget *
gtkW_frame_new (GtkWidget	*parent,
		gchar		*name)
{
  GtkWidget *frame;

  frame = gtk_frame_new (name);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), gtkW_frame_shadow_type);
  gtk_container_border_width (GTK_CONTAINER (frame), gtkW_border_width);
  if (parent != NULL)
    gtk_box_pack_start (GTK_BOX (parent), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return frame;
}

static GtkWidget *
gtkW_hbox_new (GtkWidget *parent)
{
  GtkWidget	*hbox;

  hbox = gtk_hbox_new (gtkW_homogeneous_layout, 2);
  gtk_container_border_width (GTK_CONTAINER (hbox), gtkW_border_width);
  if (parent)
    gtk_container_add (GTK_CONTAINER (parent), hbox);
  gtk_widget_show (hbox);

  return hbox;
}

static void
gtkW_message_dialog (gint gtk_was_initialized, gchar *message)
{
  GtkWidget *dlg;
  GtkWidget *table;
  GtkWidget *label;
  gchar	**argv;
  gint	argc;

  if (! gtk_was_initialized)
    {
      argc = 1;
      argv = g_new (gchar *, 1);
      argv[0] = g_strdup (PLUG_IN_NAME);
      gtk_init (&argc, &argv);
      gtk_rc_parse (gimp_gtkrc ());
    }

  dlg = gtkW_message_dialog_new (PLUG_IN_NAME);

  table = gtkW_table_new (GTK_DIALOG (dlg)->vbox, 1, 1);

  label = gtk_label_new (message);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL|GTK_EXPAND,
		    0, 0, 0);

  gtk_widget_show (label);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();
}

static GtkWidget *
gtkW_message_dialog_new (gchar * name)
{
  GtkWidget *dlg, *button;

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), name);
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) gtkW_close_callback, NULL);

  /* Action Area */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
		      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  return dlg;
}

static GtkWidget *
gtkW_table_add_button (GtkWidget	*table,
		       gchar		*name,
		       gint		x0,
		       gint		x1,
		       gint		y,
		       GtkSignalFunc	callback,
		       gpointer		value)
{
  GtkWidget *button;

  button = gtk_button_new_with_label (name);
  gtk_table_attach (GTK_TABLE(table), button, x0, x1, y, y+1,
		    gtkW_align_x, gtkW_align_y, 0, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) callback, value);
  gtk_widget_show (button);

  return button;
}

static GtkWidget *
gtkW_table_add_dscale_entry (GtkWidget		*table,
			     gchar		*name,
			     gint		x,
			     gint		y,
			     GtkSignalFunc	scale_update,
			     GtkSignalFunc	entry_update,
			     gdouble		*value,
			     gdouble		min,
			     gdouble		max,
			     gdouble		step,
			     gpointer		widget_entry)
{
  GtkObject *adjustment;
  GtkWidget *label, *hbox, *scale, *entry;
  gchar *buffer;

  label = gtk_label_new (name);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, x, x+1, y, y+1,
		    gtkW_align_x, gtkW_align_y,
		    gtkW_border_width, gtkW_border_height);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    gtkW_align_x, gtkW_border_width,
		    gtkW_border_width, gtkW_border_height);

  adjustment = gtk_adjustment_new (*value, min, max, step, step, 0.0);
  gtk_widget_show (hbox);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, GTKW_SCALE_WIDTH, 0);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) scale_update, value);

  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), adjustment);
  gtk_object_set_user_data (adjustment, entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize (entry, GTKW_ENTRY_WIDTH, 0);

  buffer = (gchar *) g_malloc (GTKW_ENTRY_BUFFER_SIZE);
  sprintf (buffer, "%6f", *value);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entry_update, value);

  gtk_widget_show (label);
  gtk_widget_show (scale);
  gtk_widget_show (entry);

  if (widget_entry)
    {
      gtkW_widget_table *tentry = (gtkW_widget_table *) widget_entry;

      tentry->widget = (GtkWidget *) adjustment;
      tentry->updater = gtkW_dscale_entry_change_value;
      tentry->value = value;
    }

  return entry;
}

static GtkWidget *
gtkW_table_add_iscale_entry (GtkWidget		*table,
			     gchar		*name,
			     gint		x,
			     gint		y,
			     GtkSignalFunc	scale_update,
			     GtkSignalFunc	entry_update,
			     gint		*value,
			     gdouble		min,
			     gdouble		max,
			     gdouble		step,
			     gpointer		widget_entry)
{
  GtkObject *adjustment;
  GtkWidget *label, *hbox, *scale, *entry;
  gchar *buffer;

  label = gtk_label_new (name);
  gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, x, x+1, y, y+1,
		    gtkW_align_x, gtkW_align_y,
		    gtkW_border_width, gtkW_border_height);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    gtkW_align_x, gtkW_align_y,
		    gtkW_border_width, gtkW_border_height);

  adjustment = gtk_adjustment_new (*value, min, max, step, step, 0.0);
  gtk_widget_show (hbox);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, GTKW_SCALE_WIDTH, 0);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) scale_update, value);

  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), adjustment);
  gtk_object_set_user_data (adjustment, entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize (entry, GTKW_ENTRY_WIDTH, 0);
  buffer = (gchar *) g_malloc (GTKW_ENTRY_BUFFER_SIZE);
  sprintf (buffer, "%d", *value);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entry_update, value);

  gtk_widget_show (label);
  gtk_widget_show (scale);
  gtk_widget_show (entry);

  if (widget_entry)
    {
      gtkW_widget_table *tentry = (gtkW_widget_table *) widget_entry;

      tentry->widget = (GtkWidget *) adjustment;
      tentry->updater = gtkW_iscale_entry_change_value;
      tentry->value = value;
    }

  return entry;
}

static GtkWidget *
gtkW_table_add_menu (GtkWidget		*table,
		     gchar		*name,
		     gint		x,
		     gint		y,
		     GtkSignalFunc	menu_update,
		     gint		*val,
		     gtkW_menu_item	*item,
		     gint		item_num,
		     gpointer		widget_entry)
{
  GtkWidget *label;
  GtkWidget *menu, *menuitem, *option_menu;
  gint i;

  label = gtk_label_new (name);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, x, x + 1, y, y + 1,
		    gtkW_align_x, gtkW_align_y,
		    gtkW_border_width, gtkW_border_height);
  gtk_widget_show (label);

  menu = gtk_menu_new ();

  for (i = 0; i < item_num; i++)
    {
      gint	**buffer;

      buffer = (gint **) g_malloc (3 * sizeof (gint *));
      buffer[0] = (gint *) ((*val == i) ? TRUE : FALSE); /* for trick */
      buffer[1] = val;				/* for pointer */
      buffer[2] = (gint *) i;			/* for value */
      menuitem = gtk_menu_item_new_with_label (item[i].name);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  (GtkSignalFunc) menu_update,
			  buffer);
      gtk_widget_show (menuitem);
    }

  option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), *val);
  gtk_table_attach (GTK_TABLE (table), option_menu, x + 1, x + 2, y, y + 1,
		    gtkW_align_x, gtkW_align_y,
		    gtkW_border_width, gtkW_border_height);
  gtk_widget_show (option_menu);

  if (widget_entry)
    {
      gtkW_widget_table *tentry = (gtkW_widget_table *) widget_entry;

      tentry->widget = option_menu;
      tentry->updater = gtkW_menu_change_value;
      tentry->value = val;
    }

  return option_menu;
}

static GtkWidget *
gtkW_table_add_toggle (GtkWidget	*table,
		       gchar		*name,
		       gint		x,
		       gint		y,
		       GtkSignalFunc	update,
		       gint		*value,
		       gpointer		widget_entry)
{
  GtkWidget *toggle;

  toggle = gtk_check_button_new_with_label(name);
  gtk_table_attach (GTK_TABLE (table), toggle, x, x + 2, y, y+1,
		    gtkW_align_x, gtkW_align_y,
		    gtkW_border_width, gtkW_border_height);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) update,
		      value);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *value);
  gtk_widget_show (toggle);

  if (widget_entry)
    {
      gtkW_widget_table * tentry = (gtkW_widget_table *) widget_entry;

      tentry->widget = toggle;
      tentry->updater = gtkW_toggle_change_value;
      tentry->value = value;
    }
  return toggle;
}

static GtkWidget *
gtkW_table_new (GtkWidget *parent, gint col, gint row)
{
  GtkWidget	*table;

  table = gtk_table_new (col, row, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), gtkW_border_width);
  if (parent)
    gtk_container_add (GTK_CONTAINER (parent), table);
  gtk_widget_show (table);

  return table;
}

static GtkWidget *
gtkW_vbox_add_button (GtkWidget		*vbox,
		      gchar		*name,
		      GtkSignalFunc	update,
		      gpointer		data)
{
  GtkWidget *button;

  button = gtk_button_new_with_label(name);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) update, data);
  gtk_widget_show (button);
  return button;
}

GtkWidget *
gtkW_vbox_new (GtkWidget *parent)
{
  GtkWidget *vbox;

  vbox = gtk_vbox_new (gtkW_homogeneous_layout, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), gtkW_border_width);
  if (parent)
    gtk_container_add (GTK_CONTAINER (parent), vbox);
  gtk_widget_show (vbox);

  return vbox;
}

static GtkTooltips *
gtkW_tooltips_new (GtkWidget *frame)
{
  GtkTooltips	*tooltips;

  tooltips = gtk_tooltips_new ();

  if (gtkW_tooltips_color_allocated == FALSE)
    {
      tooltips_foreground.red   = 0;
      tooltips_foreground.green = 0;
      tooltips_foreground.blue  = 0;
      /* postit yellow (khaki) as background: */
      gdk_color_alloc (gtk_widget_get_colormap (frame), &tooltips_foreground);
      tooltips_background.red   = 61669;
      tooltips_background.green = 59113;
      tooltips_background.blue  = 35979;
      gdk_color_alloc (gtk_widget_get_colormap (frame), &tooltips_background);
      gtkW_tooltips_color_allocated = TRUE;
    }
  
    gtk_tooltips_set_colors (tooltips,
			     &tooltips_background,
			     &tooltips_foreground);

    return tooltips;
}
/* end of gtkW functions */
/* CML_explorer.c ends here */
