/*
 * This is a plugin for the GIMP.
 *
 * Copyright (C) 1997 Xavier Bouchoux
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
 */

/*
 * This plug-in produces sinus textures.
 *
 * Please send any patches or suggestions to me: Xavier.Bouchoux@ensimag.imag.fr.
 */

/* Version 0.99 */

#define USE_LOGO

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/gimpmath.h>

#include <plug-ins/megawidget/megawidget.h>

#include "libgimp/stdplugins-intl.h"

#ifdef USE_LOGO
#include "sinus_logo.h"
#endif

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif /* RAND_MAX */

/*
 * This structure is used for persistent data.
 */

#define B_W        0L  /* colors setting */
#define USE_FG_BG  1L
#define USE_COLORS 2L

#define LINEAR     0L  /* colorization settings */
#define BILINEAR   1L
#define SINUS      2L

#define IDEAL      0L  /* Perturbation settings */
#define PERTURBED  1L

typedef struct
{
  gdouble   scalex;
  gdouble   scaley;
  gdouble   cmplx;
  gdouble   blend_power;
  gint      seed;
  gint      tiling;
  glong     perturbation;
  glong     colorization;
  glong     colors;
  guchar    col1[4];
  guchar    col2[4];
} SinusVals;

static SinusVals svals = 
{
  15.0,
  15.0,
  1.0,
  0.0,
  42,
  TRUE,
  PERTURBED,
  LINEAR, 
  USE_COLORS,
  { 255, 255, 0, 255 },
  { 0, 0, 255, 255 }
};

typedef struct
{
  gint   height, width;
  double c11, c12, c13, c21, c22, c23, c31, c32, c33;
  double (*blend) (double );
  guchar r, g, b, a;
  int    dr, dg, db, da;
} params;


static gint              drawable_is_grayscale = FALSE;
static struct mwPreview *thePreview;
static GDrawable        *drawable;

/* Declare functions */

static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);
static void sinus (void);

static gdouble linear   (gdouble v);
static gdouble bilinear (gdouble v);
static gdouble cosinus  (gdouble v);

static gint    sinus_dialog     (void);
static void    sinus_do_preview (GtkWidget *widget);

static inline void compute_block_4 (guchar *dest_row, guint rowstride,
				    gint x0, gint y0, gint h, gint w, params *p);
static inline void compute_block_3 (guchar *dest_row, guint rowstride,
				    gint x0, gint y0, gint h, gint w, params *p);
static inline void compute_block_2 (guchar *dest_row, guint rowstride,
				    gint x0, gint y0, gint h, gint w, params *p);
static inline void compute_block_1 (guchar *dest_row, guint rowstride,
				    gint x0, gint y0, gint h, gint w, params *p);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32,    "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE,    "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },

    { PARAM_FLOAT,    "xscale", "Scale value for x axis" },
    { PARAM_FLOAT,    "yscale", "Scale value dor y axis" },
    { PARAM_FLOAT,    "complex", "Complexity factor" },
    { PARAM_INT32,    "seed", "Seed value for random number generator" },
    { PARAM_INT32,    "tiling", "If set, the pattern generated will tile" },
    { PARAM_INT32,    "perturb", "If set, the pattern is a little more distorted..." },
    { PARAM_INT32,    "colors", "where to take the colors (0= B&W,  1= fg/bg, 2= col1/col2)"},
    { PARAM_COLOR,    "col1", "fist color (sometimes unused)" },
    { PARAM_COLOR,    "col2", "second color (sometimes unused)" },
    { PARAM_FLOAT,    "alpha1", "alpha for the first color (used if the drawable has an alpha chanel)" },
    { PARAM_FLOAT,    "alpha2", "alpha for the second color (used if the drawable has an alpha chanel)" },
    { PARAM_INT32,    "blend", "0= linear, 1= bilinear, 2= sinusoidal" },
    { PARAM_FLOAT,    "blend_power", "Power used to strech the blend" }
  };

  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  INIT_I18N ();

  gimp_install_procedure ("plug_in_sinus",
			  _("Generates a texture with sinus functions"),
			  "FIX ME: sinus help",
			  "Xavier Bouchoux",
			  "Xavier Bouchoux",
			  "1997",
			  N_("<Image>/Filters/Render/Sinus..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void 
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N_UI();

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_sinus", &svals);

      /* In order to prepare the dialog I need to know wether it's grayscale or not */
      drawable = gimp_drawable_get (param[2].data.d_drawable);
      thePreview = mw_preview_build_virgin(drawable);
      if (gimp_drawable_is_gray (drawable->id))
	drawable_is_grayscale = TRUE;
      else
	drawable_is_grayscale = FALSE;

      if (!sinus_dialog())
        return;

      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 16)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  svals.scalex       = param[3].data.d_float;
	  svals.scaley       = param[4].data.d_float;
	  svals.cmplx        = param[5].data.d_float;
	  svals.seed         = param[6].data.d_int32;
	  svals.tiling       = param[7].data.d_int32;
	  svals.perturbation = param[8].data.d_int32;
	  svals.colors       = param[9].data.d_int32;
	  svals.col1[0]      = param[10].data.d_color.red;
	  svals.col1[1]      = param[10].data.d_color.green;
	  svals.col1[2]      = param[10].data.d_color.blue;
	  svals.col2[0]      = param[11].data.d_color.red;
	  svals.col2[1]      = param[11].data.d_color.green;
	  svals.col2[2]      = param[11].data.d_color.blue;
	  svals.col1[3]      = param[12].data.d_float * 255.0;
	  svals.col2[3]      = param[13].data.d_float * 255.0;
	  svals.colorization = param[14].data.d_int32;
	  svals.blend_power  = param[15].data.d_float;
	}
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_sinus", &svals);
      break;

    default:
      break;
    }

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Make sure that the drawable is gray or RGB */
  if ((status == STATUS_SUCCESS) &&
      (gimp_drawable_is_rgb (drawable->id) ||
       gimp_drawable_is_gray (drawable->id)))
    {
      gimp_progress_init ("Calculating picture...");
      gimp_tile_cache_ntiles (1);
      sinus ();

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
        gimp_set_data ("plug_in_sinus", &svals, sizeof (SinusVals));
    }
  else
    {
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/*
 *  Main procedure
 */

static void 
prepare_coef (params *p)
{
  typedef struct { guchar r, g, b, a; } type_color;
  type_color col1,col2;
  double scalex = svals.scalex;
  double scaley = svals.scaley;

  srand(svals.seed);
  switch (svals.colorization)
    {
    case BILINEAR:
      p->blend = bilinear;
      break;
    case SINUS:
      p->blend = cosinus;
      break;
    case LINEAR:
    default:
      p->blend = linear;
    }

  if (svals.perturbation==IDEAL)
    {
      p->c11= 0*rand();
      p->c12= (2.0*rand()/(RAND_MAX+1.0)-1)*scaley; /*rand+rand is used to keep */
      p->c13= (2*G_PI*rand())/RAND_MAX;
      p->c21= 0*rand();
      p->c22= (2.0*rand()/(RAND_MAX+1.0)-1)*scaley; /*correspondance beetween Ideal*/
      p->c23= (2*G_PI*rand())/RAND_MAX;
      p->c31= (2.0*rand()/(RAND_MAX+1.0)-1)*scalex; /*and perturbed coefs (I hope...)*/
      p->c32= 0*rand();
      p->c33= (2*G_PI*rand())/RAND_MAX;
    }
  else
    {
      p->c11= (2.0*rand()/(RAND_MAX+1.0)-1)*scalex;
      p->c12= (2.0*rand()/(RAND_MAX+1.0)-1)*scaley;
      p->c13= (2*G_PI*rand())/RAND_MAX;
      p->c21= (2.0*rand()/(RAND_MAX+1.0)-1)*scalex;
      p->c22= (2.0*rand()/(RAND_MAX+1.0)-1)*scaley;
      p->c23= (2*G_PI*rand())/RAND_MAX;
      p->c31= (2.0*rand()/(RAND_MAX+1.0)-1)*scalex;
      p->c32= (2.0*rand()/(RAND_MAX+1.0)-1)*scaley;
      p->c33= (2*G_PI*rand())/RAND_MAX;
    }

  if (svals.tiling)
    {
      p->c11= ROUND (p->c11/(2*G_PI))*2*G_PI;
      p->c12= ROUND (p->c12/(2*G_PI))*2*G_PI;
      p->c21= ROUND (p->c21/(2*G_PI))*2*G_PI;
      p->c22= ROUND (p->c22/(2*G_PI))*2*G_PI;
      p->c31= ROUND (p->c31/(2*G_PI))*2*G_PI;
      p->c32= ROUND (p->c32/(2*G_PI))*2*G_PI;
    }

  col2.a=svals.col2[3];
  col1.a=svals.col1[3];

  if (drawable_is_grayscale)
    {
      col1.r=col1.g=col1.b=255;
      col2.r=col2.g=col2.b=0;
    }
  else
    {
      switch (svals.colors)
	{
	case USE_COLORS:
	  col1.r=svals.col1[0];
	  col1.g=svals.col1[1];
	  col1.b=svals.col1[2];
	  col2.r=svals.col2[0];
	  col2.g=svals.col2[1];
	  col2.b=svals.col2[2];
	  break;
	case B_W:
	  col1.r=col1.g=col1.b=255;
	  col2.r=col2.g=col2.b=0;
	  break;
	case USE_FG_BG:
	  gimp_palette_get_foreground(&col2.r, &col2.g, &col2.b);
	  gimp_palette_get_background(&col1.r, &col1.g, &col1.b);
	  break;
	}
    }
  p->r = col1.r;
  p->g = col1.g;
  p->b = col1.b;
  p->a = col1.a;
  p->dr=(int)col2.r-col1.r;
  p->dg=(int)col2.g-col1.g;
  p->db=(int)col2.b-col1.b;
  p->da=(int)col2.a-col1.a;
}

static void
sinus (void)
{
  params  p;
  gint    bytes;
  GPixelRgn dest_rgn;
  int     ix1, iy1, ix2, iy2;     /* Selected image size. */
  gpointer pr;
  gint progress, max_progress;
  
  prepare_coef(&p);
  
  gimp_drawable_mask_bounds(drawable->id, &ix1, &iy1, &ix2, &iy2);
  
  p.width = drawable->width;
  p.height = drawable->height;
  bytes = drawable->bpp;
  
  gimp_pixel_rgn_init(&dest_rgn, drawable, ix1, iy1, ix2-ix1, iy2-iy1, TRUE,TRUE);
  progress = 0;
  max_progress = (ix2-ix1)*(iy2-iy1);
  
  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      switch (bytes)
	{
	case 4:
	  compute_block_4 (dest_rgn.data, dest_rgn.rowstride,
			   dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h, &p);
	  break;
	case 3:
	  compute_block_3 (dest_rgn.data, dest_rgn.rowstride,
			   dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h, &p);
	  break;
	case 2:
	  compute_block_2 (dest_rgn.data, dest_rgn.rowstride,
			   dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h, &p);
	  break;
	case 1:
	  compute_block_1 (dest_rgn.data, dest_rgn.rowstride,
			   dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h, &p);
	  break;
	}
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, ix1, iy1, (ix2-ix1), (iy2-iy1));
}

static gdouble 
linear (gdouble v)
{
  register double a = v - (int) v;

  return (a < 0 ? 1.0 + a : a);
}

static gdouble 
bilinear (gdouble v)
{
  register double a = v - (int) v;

  a = (a < 0 ? 1.0 + a : a);
  return (a > 0.5 ? 2 - 2 * a : 2 * a);
}

static gdouble 
cosinus (gdouble v)
{
  return 0.5 - 0.5 * sin ((v + 0.25) * G_PI * 2);
}

static inline void 
compute_block_4 (guchar *dest_row, 
		 guint   rowstride,
		 gint    x0,
		 gint    y0,
		 gint    w,
		 gint    h, 
		 params *p)
{
  gint i,j;
  double x,y, grey;
  guchar *dest;

  for (j = y0; j < (y0 + h); j++)
    {
      y=((double)j)/p->height;
      dest = dest_row;
      for (i= x0; i<(x0+w); i++)
	{
	  x=((double)i)/p->width;

	  grey = sin(p->c11*x + p->c12*y + p->c13) * (0.5+0.5*sin(p->c31*x + p->c32*y +p->c33)) \
	    + sin(p->c21*x + p->c22*y + p->c23) * (0.5-0.5*sin(p->c31*x + p->c32*y +p->c33));
	  grey=pow(p->blend(svals.cmplx*(0.5+0.5*grey)),exp(svals.blend_power));

	  *dest++= p->r+(int)(grey*p->dr);
	  *dest++= p->g+(int)(grey*p->dg);
	  *dest++= p->b+(int)(grey*p->db);
	  *dest++= p->a+(int)(grey*p->da);
	}
      dest_row += rowstride;
    }
}

static inline void 
compute_block_3 (guchar *dest_row, 
		 guint   rowstride,
		 gint    x0,
		 gint    y0,
		 gint    w,
		 gint    h, 
		 params *p)
{
  gint i,j;
  double x,y, grey;
  guchar *dest;

  for (j=y0; j<(y0+h); j++)
    {
      y=((double)j)/p->height;
      dest = dest_row;
      for (i= x0; i<(x0+w); i++)
	{
	  x=((double)i)/p->width;

	  grey = sin(p->c11*x + p->c12*y + p->c13) * (0.5+0.5*sin(p->c31*x + p->c32*y +p->c33)) \
	    + sin(p->c21*x + p->c22*y + p->c23) * (0.5-0.5*sin(p->c31*x + p->c32*y +p->c33));
	  grey=pow(p->blend(svals.cmplx*(0.5+0.5*grey)),exp(svals.blend_power));

	  *dest++= p->r+(int)(grey*p->dr);
	  *dest++= p->g+(int)(grey*p->dg);
	  *dest++= p->b+(int)(grey*p->db);
	}
      dest_row += rowstride;
    }
}

static inline void 
compute_block_2 (guchar *dest_row, 
		 guint   rowstride,
		 gint    x0,
		 gint    y0,
		 gint    w,
		 gint    h, 
		 params *p)
{
  gint i,j;
  double x,y, grey;
  guchar *dest;

  for (j=y0; j<(y0+h); j++)
    {
      y=((double)j)/p->height;
      dest = dest_row;
      for (i= x0; i<(x0+w); i++)
	{
	  x=((double)i)/p->width;

	  grey = sin(p->c11*x + p->c12*y + p->c13) * (0.5+0.5*sin(p->c31*x + p->c32*y +p->c33)) \
	    + sin(p->c21*x + p->c22*y + p->c23) * (0.5-0.5*sin(p->c31*x + p->c32*y +p->c33));
	  grey=pow(p->blend(svals.cmplx*(0.5+0.5*grey)),exp(svals.blend_power));

	  *dest++= (guchar)(grey*255.0);
	  *dest++= p->a+(int)(grey*p->da);
	}
      dest_row += rowstride;
    }
}

static inline void 
compute_block_1 (guchar *dest_row, 
		 guint   rowstride,
		 gint    x0,
		 gint    y0,
		 gint    w,
		 gint    h, 
		 params *p)
{
  gint i,j;
  double x,y, grey;
  guchar *dest;

  for (j=y0; j<(y0+h); j++)
    {
      y=((double)j)/p->height;
      dest = dest_row;
      for (i= x0; i<(x0+w); i++)
	{
	  x=((double)i)/p->width;

	  grey = sin(p->c11*x + p->c12*y + p->c13) * (0.5+0.5*sin(p->c31*x + p->c32*y +p->c33)) \
	    + sin(p->c21*x + p->c22*y + p->c23) * (0.5-0.5*sin(p->c31*x + p->c32*y +p->c33));
	  grey=pow(p->blend(svals.cmplx*(0.5+0.5*grey)),exp(svals.blend_power));

	  *dest++= (guchar)(grey*255.0);
	}
      dest_row += rowstride;
    }
}

/* ---------------------------------------------------------------*/
/*  -------------------------- UI ------------------------------- */
/* -------------------------------------------------------------- */

static void
alpha_scale_cb (GtkAdjustment *adj,
		gpointer       data)
{
  guchar *val;
  GtkWidget *color_button;

  val = (guchar*) data;

  *val = (guchar)(adj->value * 255.0);

  color_button = gtk_object_get_user_data (GTK_OBJECT (adj));
  if (GIMP_IS_COLOR_BUTTON (color_button))
    gimp_color_button_update (GIMP_COLOR_BUTTON (color_button));
}

static void
alpha_scale_update (GtkWidget *color_button,
		    gpointer   data)
{
  guchar *val;
  GtkWidget *adj;

  val = (guchar*)data;

  adj = gtk_object_get_user_data (GTK_OBJECT (color_button));
  gtk_signal_handler_block_by_data (GTK_OBJECT (adj), data);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), (gfloat)(*val) / 255.0);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (adj), data);

  if (do_preview)
    sinus_do_preview (NULL);
}

gboolean run_flag = FALSE;

static void
sinus_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  run_flag = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
sinus_toggle_button_update (GtkWidget *widget,
			    gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  if (do_preview)
    sinus_do_preview (NULL);
}

static void
sinus_radio_button_update (GtkWidget *widget,
			   gpointer   data)
{
  gimp_radio_button_update (widget, data);

  if (do_preview)
    sinus_do_preview (NULL);
}

static void
sinus_int_adjustment_update (GtkAdjustment *adjustment,
			     gpointer       data)
{
  gimp_int_adjustment_update (adjustment, data);

  if (do_preview)
    sinus_do_preview (NULL);
}

static void
sinus_double_adjustment_update (GtkAdjustment *adjustment,
				gpointer       data)
{
  gimp_double_adjustment_update (adjustment, data);

  if (do_preview)
    sinus_do_preview (NULL);
}

/*****************************************/
/* The note book                         */
/*****************************************/

gint
sinus_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *main_hbox;
  GtkWidget *preview;
  GtkWidget *notebook;
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkWidget *push_col1 = NULL;
  GtkWidget *push_col2 = NULL;
  GtkWidget *spinbutton;
  GtkObject *adj;
#ifdef USE_LOGO
  GtkWidget *logo;
  gint x,y;
  char buf[3*100];
  guchar *data;
#endif
  gchar **argv;
  gint argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("sinus");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  /* Create Main window with a vbox */
  /* ============================== */
  dlg = gimp_dialog_new (_("Sinus"), "sinus",
			 gimp_plugin_help_func, "filters/sinus.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), sinus_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  main_hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_hbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (main_hbox);

  /* Create preview */
  /* ============== */
  vbox = gtk_vbox_new (TRUE, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  preview = mw_preview_new (vbox, thePreview, &sinus_do_preview);
  sinus_do_preview (preview);

#ifdef USE_LOGO
  logo = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW(logo), 100, 100);
  gtk_box_pack_start (GTK_BOX (vbox), logo, TRUE, FALSE, 0);
  gtk_widget_show (logo);

  data = (guchar *) logo_data;
  for (y = 0; y < 100; y++)
    {
      for (x=0; x<100; x++)
	{
	  HEADER_PIXEL (data, (&buf[3 * x]));
	}
      gtk_preview_draw_row (GTK_PREVIEW(logo), (guchar *) buf, 0, y, 100);
    }
#endif

  /* Create the notebook */
  /* =================== */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (main_hbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  /* Create the drawing settings frame */
  /* ================================= */
  page = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  frame = gtk_frame_new (_("Drawing Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new(3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER(frame), table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("X Scale:"), 140, 0,
			      svals.scalex, 0.0001, 100.0, 0.0001, 5, 4,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (sinus_double_adjustment_update),
		      &svals.scalex);
  
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Y Scale:"), 140, 0,
			      svals.scaley, 0.0001, 100.0, 0.0001, 5, 4,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (sinus_double_adjustment_update),
		      &svals.scaley);
  
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Complexity:"), 140, 0,
			      svals.cmplx, 0.0, 15.0, 0.01, 5, 2,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (sinus_double_adjustment_update),
		      &svals.cmplx);

  gtk_widget_show (table);

  frame= gtk_frame_new (_("Calculation Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Random Seed:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&adj, svals.seed,
				     -10000000000, 1000000000, 1, 10,
				     0, 0, 0);
  gtk_widget_set_usize (spinbutton, 100, -1);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (sinus_int_adjustment_update),
		      &svals.seed);
  gtk_widget_show (spinbutton);

  toggle = gtk_check_button_new_with_label (_("Force Tiling?"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), svals.tiling);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (sinus_toggle_button_update),
		      &svals.tiling);
  gtk_widget_show (toggle);

  vbox2 =
    gimp_radio_group_new2 (FALSE, NULL,
			   sinus_radio_button_update,
			   &svals.perturbation, (gpointer) svals.perturbation,

			   _("Ideal"),     (gpointer) IDEAL, NULL,
			   _("Distorted"), (gpointer) PERTURBED, NULL,

			   NULL);

  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 0);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  label = gtk_label_new (_("Settings"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  /* Color settings dialog: */
  /* ====================== */
  page = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  if (drawable_is_grayscale) 
    {
      frame = gtk_frame_new (_("Colors"));
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
      gtk_box_pack_start(GTK_BOX(page), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      /*if in grey scale, the colors are necessarily black and white */
      label = gtk_label_new (_("The colors are white and black."));
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_container_add (GTK_CONTAINER (vbox), label);
      gtk_widget_show (label);
    } 
  else
    {
      frame = gimp_radio_group_new2 (TRUE, _("Colors"),
				     sinus_radio_button_update,
				     &svals.colors, (gpointer) svals.colors,

				     _("Black & White"),
				     (gpointer) B_W, NULL,
				     _("Foreground & Background"),
				     (gpointer) USE_FG_BG, NULL,
				     _("Choose here:"),
				     (gpointer) USE_COLORS, NULL,

				     NULL);

      gtk_box_pack_start(GTK_BOX(page), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = GTK_BIN (frame)->child;

      hbox = gtk_hbox_new (TRUE, 20);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      push_col1 = gimp_color_button_new (_("First Color"),
					 32, 32, svals.col1, 4);
      gtk_box_pack_start (GTK_BOX (hbox), push_col1, FALSE, FALSE, 0);
      gtk_widget_show (push_col1);

      push_col2 = gimp_color_button_new (_("Second Color"),
					 32, 32, svals.col2, 4);
      gtk_box_pack_start (GTK_BOX (hbox), push_col2, FALSE, FALSE, 0);
      gtk_widget_show (push_col2);

      gtk_widget_show (hbox);
    }

  frame = gtk_frame_new (_("Alpha Channels"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("First Color:"), 0, 0,
			      svals.col1[3] / 255.0, 0.0, 1.0, 0.01, 0.1, 2,
			      NULL, NULL);

  gtk_object_set_user_data (GTK_OBJECT (adj), push_col1);      
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (alpha_scale_cb),
                      &svals.col1[3]);

  if (push_col1)
    {
      gtk_object_set_user_data (GTK_OBJECT (push_col1), adj);
      gtk_signal_connect (GTK_OBJECT (push_col1), "color_changed",
			  GTK_SIGNAL_FUNC (alpha_scale_update),
			  &svals.col1[3]);
    }

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Second Color:"), 0, 0,
			      svals.col2[3] / 255.0, 0.0, 1.0, 0.01, 0.1, 2,
			      NULL, NULL);

  gtk_object_set_user_data (GTK_OBJECT (adj), push_col2);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (alpha_scale_cb),
                      &svals.col2[3]);

  if (push_col2)
    {
      gtk_object_set_user_data (GTK_OBJECT (push_col2), adj);
      gtk_signal_connect (GTK_OBJECT (push_col2), "color_changed",
			  GTK_SIGNAL_FUNC (alpha_scale_update),
			  &svals.col2[3]);
    }

  gtk_widget_show (table);

  label = gtk_label_new (_("Colors"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  /* blend settings dialog: */
  /* ====================== */
  page = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page), 4);

  frame = gtk_frame_new (_("Blend Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (page), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_border_width (GTK_CONTAINER (vbox), 4);
  gtk_widget_show (vbox);

  frame =
    gimp_radio_group_new2 (TRUE, _("Gradient"),
			   sinus_radio_button_update,
			   &svals.colorization, (gpointer) svals.colorization,

			   _("Linear"),     (gpointer) LINEAR, NULL,
			   _("Bilinear"),   (gpointer) BILINEAR, NULL,
			   _("Sinusodial"), (gpointer) SINUS, NULL,

			   NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Exponent:"), 0, 0,
			      svals.blend_power, -7.5, 7.5, 0.01, 5.0, 2,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (sinus_double_adjustment_update),
		      &svals.blend_power);

  gtk_widget_show (table);

  label = gtk_label_new (_("Blend"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}

/******************************************************************/
/* Draw preview image. if DoCompute is TRUE then recompute image. */
/******************************************************************/

void
sinus_do_preview (GtkWidget *widget)
{
  static GtkWidget *theWidget = NULL;
  gint y,rowsize;
  guchar *buf, *savbuf;
  params p;

  if (theWidget == NULL)
    {
      theWidget = widget;
    }

  rowsize = thePreview->width * thePreview->bpp;
  savbuf = buf = g_new (guchar, thePreview->width*thePreview->height*thePreview->bpp);
  if (buf != NULL)
    {
      p.height = thePreview->height;
      p.width = thePreview->width;
      prepare_coef (&p);

      if (thePreview->bpp == 3)
	compute_block_3 (buf, rowsize, 0, 0,
			 thePreview->width, thePreview->height, &p);
      else if (thePreview->bpp == 1)
	{
	  compute_block_1 (buf, rowsize, 0, 0,
			   thePreview->width, thePreview->height, &p);
	}
      else
	fprintf (stderr, "Uh Oh....  Little sinus preview-only problem...\n");

      for (y = 0; y < thePreview->height; y++)
	{
	  gtk_preview_draw_row (GTK_PREVIEW (theWidget),
				buf, 0, y, thePreview->width);
	  buf += rowsize;
	}
      g_free (savbuf);
      gtk_widget_draw (theWidget, NULL);
      gdk_flush ();
    }
  else
    {
      fprintf (stderr,"Not enough mem for sinus Preview...\n");
    }
}
