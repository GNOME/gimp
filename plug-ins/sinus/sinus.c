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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * This plug-in produces sinus textures.
 *
 * Please send any patches or suggestions to me: Xavier.Bouchoux@ensimag.imag.fr.
 */


/* Version 0.99 */

#define USE_LOGO

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <plug-ins/megawidget/megawidget.h>
#ifdef USE_LOGO
#include "sinus_logo.h"
#endif

#ifdef __GNUC__
#define inline inline
#else
#define inline
#endif

#define ROUND_TO_INT(val) ((val) + 0.5)

typedef gdouble colRGBA[4];

/*
 * This structure is used for persistent data.
 */

#define B_W 0           /* colors setting */
#define USE_FG_BG 1
#define USE_COLORS 2

#define LINEAR 0        /* colorization settings */
#define BILINEAR 1
#define SINUS 2

#define IDEAL 0         /* Perturbation settings */
#define PERTURBED 1

typedef struct {
  gdouble   scalex, scaley;
  gdouble   cmplx;
  gdouble   blend_power;
  gint     seed;
  gint     tiling;
  gint     perturbation;
  gint     colorization;
  gint     colors;
  colRGBA  col1,col2;
} SinusVals;

static SinusVals svals={15.0, 15.0, 1.0, 0.0, 42, TRUE, PERTURBED, LINEAR, USE_COLORS, {1,1,0,1}, {0,0,1,1}};

typedef struct {
  gint height, width;
  double c11,c12,c13, c21,c22,c23, c31,c32,c33;
  double (*blend) (double );
  guchar r,g,b,a;
  int   dr,dg,db,da;
} params;


static gint drawable_is_grayscale= FALSE;
static struct mwPreview *thePreview;
static GDrawable *drawable;

/* Declare functions */

static void query ();
static void run (gchar   *name,
		 gint     nparams,
		 GParam  *param,
		 gint    *nreturn_vals,
		 GParam **return_vals);
static void sinus();

double frac( double v );
double linear (double v);
double bilinear(double v);
double cosinus(double v);
int sinus_dialog(void);
void sinus_do_preview(GtkWidget *w);

void DrawPreviewImage(gint DoCompute);
inline void compute_block_4(guchar *dest_row, guint rowstride,gint x0,gint y0,gint h,gint w, params *p);
inline void compute_block_3(guchar *dest_row, guint rowstride,gint x0,gint y0,gint h,gint w, params *p) ;
inline void compute_block_2(guchar *dest_row, guint rowstride,gint x0,gint y0,gint h,gint w, params *p) ;
inline void compute_block_1(guchar *dest_row, guint rowstride,gint x0,gint y0,gint h,gint w, params *p) ;

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()


static void query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },

    { PARAM_FLOAT, "xscale", "Scale value for x axis" },
    { PARAM_FLOAT, "yscale", "Scale value dor y axis" },
    { PARAM_FLOAT, "complex", "Complexity factor" },
    { PARAM_INT32, "seed", "Seed value for random number generator" },
    { PARAM_INT32, "tiling", "If set, the pattern generated will tile" },
    { PARAM_INT32, "perturb", "If set, the pattern is a little more distorted..." },

    { PARAM_INT32, "colors", "where to take the colors (0= B&W,  1= fg/bg, 2= col1/col2)"},
/*    { PARAM_COLOR, "col1", "fist color (sometimes unused)" },
    { PARAM_COLOR, "col2", "second color (sometimes unused)" },*/
    { PARAM_FLOAT, "alpha1", "alpha for the first color (used if the drawable has an alpha chanel)" },
    { PARAM_FLOAT, "alpha2", "alpha for the second color (used if the drawable has an alpha chanel)" },

    { PARAM_INT32, "blend", "0= linear, 1= bilinear, 2= sinusoidal" },
    { PARAM_FLOAT, "blend_power", "Power used to strech the blend" }
  };

  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_sinus",
			  "Generates a texture with sinus functions",
			  "FIX ME: sinus help",
			  "Xavier Bouchoux",
			  "Xavier Bouchoux",
			  "1997",
			  "<Image>/Filters/Render/Sinus",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void run (gchar   *name,
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
      if (gimp_drawable_gray (drawable->id))
	drawable_is_grayscale= TRUE;
      else
	drawable_is_grayscale= FALSE;

      if (!sinus_dialog())
        return;

      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 16)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  svals.seed = param[6].data.d_int32;
	  svals.scalex =  param[3].data.d_float;
	  svals.scaley =  param[4].data.d_float;
	  svals.cmplx =  param[5].data.d_float;
	  svals.blend_power =  param[15].data.d_float;
	  svals.tiling = param[7].data.d_int32;
	  svals.perturbation = param[8].data.d_int32;
	  svals.colorization = param[14].data.d_int32;
	  svals.colors = param[9].data.d_int32;
	  svals.col1[3] = param[12].data.d_float;
	  svals.col2[3] = param[13].data.d_float;
	  svals.col1[0] = param[10].data.d_color.red/255.999;
	  svals.col1[1] = param[10].data.d_color.green/255.999;
	  svals.col1[2] = param[10].data.d_color.blue/255.999;
	  svals.col2[0] = param[11].data.d_color.red/255.999;
	  svals.col2[1] = param[11].data.d_color.green/255.999;
	  svals.col2[2] = param[11].data.d_color.blue/255.999;
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
  if (( status == STATUS_SUCCESS) && (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id)))
    {
      gimp_progress_init("Calculating picture...");
      gimp_tile_cache_ntiles( 1 );
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

void prepare_coef( params *p)
{
  typedef struct { guchar r,g,b,a;} type_color;
  type_color col1,col2;
  double  scalex=svals.scalex;
  double  scaley=svals.scaley;

  srand(svals.seed);
  switch (svals.colorization) {
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

  if (svals.perturbation==IDEAL) {
    p->c11= 0*rand();
    p->c12= (2.0*rand()/(RAND_MAX+1.0)-1)*scaley; /*rand+rand is used to keep */
    p->c13= (2*M_PI*rand())/RAND_MAX;
    p->c21= 0*rand();
    p->c22= (2.0*rand()/(RAND_MAX+1.0)-1)*scaley; /*correspondance beetween Ideal*/
    p->c23= (2*M_PI*rand())/RAND_MAX;
    p->c31= (2.0*rand()/(RAND_MAX+1.0)-1)*scalex; /*and perturbed coefs (I hope...)*/
    p->c32= 0*rand();
    p->c33= (2*M_PI*rand())/RAND_MAX;
  } else {
    p->c11= (2.0*rand()/(RAND_MAX+1.0)-1)*scalex;
    p->c12= (2.0*rand()/(RAND_MAX+1.0)-1)*scaley;
    p->c13= (2*M_PI*rand())/RAND_MAX;
    p->c21= (2.0*rand()/(RAND_MAX+1.0)-1)*scalex;
    p->c22= (2.0*rand()/(RAND_MAX+1.0)-1)*scaley;
    p->c23= (2*M_PI*rand())/RAND_MAX;
    p->c31= (2.0*rand()/(RAND_MAX+1.0)-1)*scalex;
    p->c32= (2.0*rand()/(RAND_MAX+1.0)-1)*scaley;
    p->c33= (2*M_PI*rand())/RAND_MAX;
  }

  if (svals.tiling) {
    p->c11= ROUND_TO_INT(p->c11/(2*M_PI))*2*M_PI;
    p->c12= ROUND_TO_INT(p->c12/(2*M_PI))*2*M_PI;
    p->c21= ROUND_TO_INT(p->c21/(2*M_PI))*2*M_PI;
    p->c22= ROUND_TO_INT(p->c22/(2*M_PI))*2*M_PI;
    p->c31= ROUND_TO_INT(p->c31/(2*M_PI))*2*M_PI;
    p->c32= ROUND_TO_INT(p->c32/(2*M_PI))*2*M_PI;
  }

  col2.a=255.999*svals.col2[3];
  col1.a=255.999*svals.col1[3];

  if (drawable_is_grayscale) {
     col1.r=col1.g=col1.b=255;
     col2.r=col2.g=col2.b=0;
  } else {
    switch (svals.colors) {
    case USE_COLORS:
      col1.r=255.999*svals.col1[0];
      col1.g=255.999*svals.col1[1];
      col1.b=255.999*svals.col1[2];
      col2.r=255.999*svals.col2[0];
      col2.g=255.999*svals.col2[1];
      col2.b=255.999*svals.col2[2];
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
sinus()
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

	for (pr= gimp_pixel_rgns_register(1, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process(pr)) {
	  switch (bytes) {
	  case 4:
	    compute_block_4(dest_rgn.data, dest_rgn.rowstride, dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h, &p);
	    break;
	  case 3:
	    compute_block_3(dest_rgn.data, dest_rgn.rowstride, dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h, &p);
	    break;
	  case 2:
	    compute_block_2(dest_rgn.data, dest_rgn.rowstride, dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h, &p);
	    break;
	  case 1:
	    compute_block_1(dest_rgn.data, dest_rgn.rowstride, dest_rgn.x, dest_rgn.y, dest_rgn.w, dest_rgn.h, &p);
	    break;
	  }
	  progress+= dest_rgn.w * dest_rgn.h;
	  gimp_progress_update((double) progress/ (double) max_progress);
	}


	gimp_drawable_flush(drawable);
	gimp_drawable_merge_shadow(drawable->id, TRUE);
	gimp_drawable_update ( drawable->id, ix1, iy1, (ix2-ix1), (iy2-iy1));
}

double linear (double v)
{
	register double a=v-(int)v;
	return (a<0?1.0+a:a);
}

double bilinear(double v)
{
	register double a=v-(int)v;
	a=(a<0?1.0+a:a);
	return (a>0.5?2-2*a:2*a);
}

double cosinus(double v)
{
	return 0.5-0.5*sin((v+0.25)*M_PI*2);
}


inline void compute_block_4(guchar *dest_row, guint rowstride,gint x0,gint y0,gint w,gint h, params *p)
{
  gint i,j;
  double x,y, grey;
  guchar *dest;

  for (j=y0; j<(y0+h); j++) {
    y=((double)j)/p->height;
    dest = dest_row;
    for (i= x0; i<(x0+w); i++) {
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

inline void compute_block_3(guchar *dest_row, guint rowstride,gint x0,gint y0,gint w,gint h, params *p)
{
  gint i,j;
  double x,y, grey;
  guchar *dest;
  for (j=y0; j<(y0+h); j++) {
    y=((double)j)/p->height;
    dest = dest_row;
    for (i= x0; i<(x0+w); i++) {
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
inline void compute_block_2(guchar *dest_row, guint rowstride,gint x0,gint y0,gint w,gint h, params *p)
{
  gint i,j;
  double x,y, grey;
  guchar *dest;

  for (j=y0; j<(y0+h); j++) {
    y=((double)j)/p->height;
    dest = dest_row;
    for (i= x0; i<(x0+w); i++) {
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
inline void compute_block_1(guchar *dest_row, guint rowstride,gint x0,gint y0,gint w,gint h, params *p)
{
  gint i,j;
  double x,y, grey;
  guchar *dest;

  for (j=y0; j<(y0+h); j++) {
    y=((double)j)/p->height;
    dest = dest_row;
    for (i= x0; i<(x0+w); i++) {
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

/*****************************************/
/* The note book                         */
/*****************************************/
int sinus_dialog(void)
{
  GtkWidget *dlg;
  GtkWidget *preview;
  gint runp;
  GtkWidget *main_hbox, *notebook;
  GtkWidget *page,*frame, *label, *vbox, *hbox, *table;
  struct mwColorSel *push_col1, *push_col2;
#ifdef USE_LOGO
  GtkWidget *logo;
  gint x,y;
  char buf[3*100];
  guchar *data;
#endif
  gchar **argv;
  gint argc;

  static struct mwValueRadioGroup coloriz_radio[] = {
    { "Linear", LINEAR },
    { "Bilinear", BILINEAR },
    { "Sinusoidal", SINUS },
    { NULL, 0 },
  };
  static struct mwValueRadioGroup colors_radio[] = {
    { "Black & White", B_W },
    { "Foreground & Background", USE_FG_BG },
    { "Choose here:", USE_COLORS},
    { NULL, 0 },
  };
  static struct mwValueRadioGroup coefs_radio[] = {
    { "Ideal", IDEAL },
    { "Distorted", PERTURBED },
    { NULL, 0 },
  };

  /* Set args */
  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup("sinus");
  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());
  
  /* Create Main window with a vbox */
  /* ============================== */
  dlg = mw_app_new("plug_in_sinus", "Sinus", &runp);
  main_hbox = gtk_hbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(main_hbox), 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), main_hbox, TRUE, TRUE, 0);
  gtk_widget_show(main_hbox);

  /* Create preview */
  /* ============== */
  vbox = gtk_vbox_new(TRUE, 5);
  gtk_box_pack_start(GTK_BOX(main_hbox), vbox, TRUE, FALSE, 0);
  gtk_widget_show(vbox);

  preview = mw_preview_new(vbox, thePreview, &sinus_do_preview);
  sinus_do_preview(preview);

#ifdef USE_LOGO
  logo = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(logo), 100, 100);
  gtk_box_pack_start(GTK_BOX(vbox), logo, TRUE, FALSE, 0);
  gtk_widget_show(logo);

  data= logo_data;
  for (y=0;y<100; y++) {
    for (x=0; x<100; x++) {
      HEADER_PIXEL(data,(&buf[3*x]));
    }
    gtk_preview_draw_row(GTK_PREVIEW(logo), buf, 0, y, 100);
  }
#endif

  /* Create the notebook */
  /* =================== */
  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
  gtk_box_pack_start(GTK_BOX(main_hbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show(notebook);

  /* Create the drawing settings frame */
  /* ================================= */
  page = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(page), 5);
  
  frame= gtk_frame_new("Drawing settings");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(page), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);
  table = gtk_table_new(4, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER (table), 5);
  gtk_container_add(GTK_CONTAINER(frame), table);
  mw_fscale_entry_new(table, "X scale: ", 0.0001, 100.0, 0.001, 5, 0,
                      0, 1, 1, 2, &(svals.scalex));
  mw_fscale_entry_new(table, "Y scale: ", 0.0001, 100.0, 0.001, 5, 0,
                      0, 1, 2, 3, &(svals.scaley));
  mw_fscale_entry_new(table, "Complexity: ", 0, 15.0, 0.01, 5, 0,
                      0, 1, 3, 4, &(svals.cmplx));
  gtk_widget_show(table);

  frame= gtk_frame_new("Calculation settings");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(page), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);
  vbox= gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(vbox);
  mw_ientry_new(vbox, NULL, "Random seed:", &svals.seed);
  mw_toggle_button_new(vbox, NULL, "Force tiling?", &svals.tiling);
  mw_value_radio_group_new(vbox, NULL , coefs_radio, &svals.perturbation);

  label = gtk_label_new("Settings");
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
  gtk_widget_show(page);


  /* Color settings dialog: */
  /* ====================== */
  page = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(page), 5);

  frame = gtk_frame_new("Colors");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(page), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);
  if (drawable_is_grayscale) {
    /*if in grey scale, the colors are necessarily black and white */
    label = gtk_label_new("The colors are white and black.");
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_widget_show(label);

  } else {
    vbox= gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_widget_show(vbox);
    mw_value_radio_group_new(vbox, NULL, colors_radio, &svals.colors);
    hbox= gtk_hbox_new(TRUE, 20);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);

    push_col1 = mw_color_select_button_create(hbox, "Fisrt Color", svals.col1, FALSE);
    push_col2 = mw_color_select_button_create(hbox, "Second Color", svals.col2, FALSE);
    gtk_widget_show(hbox);

  }


  frame = gtk_frame_new("Alpha Channels");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(page), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);
  table = gtk_table_new(3, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER (table), 5);
  gtk_container_add(GTK_CONTAINER(frame), table);

  mw_fscale_entry_new(table, "first color ", 0, 1.0, 0.01, 5, 0,
                      0, 1, 1, 2, &(svals.col1[3]));
  mw_fscale_entry_new(table, "last color ", 0, 1.0, 0.01, 5, 0,
                      0, 1, 2, 3, &(svals.col2[3]));

  gtk_widget_show(table);

  label = gtk_label_new("Colors");
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
  gtk_widget_show(page);


  /* blend settings dialog: */
  /* ====================== */
  label = gtk_label_new("Blend");
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
  page = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(page), 5);

  frame = gtk_frame_new("Blend settings");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(page), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  vbox= gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_container_border_width(GTK_CONTAINER(vbox), 5);
  gtk_widget_show(vbox);
  mw_value_radio_group_new(vbox, "Gradient", coloriz_radio, &svals.colorization);

  table = gtk_table_new(2, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER (table), 5);
  gtk_container_add(GTK_CONTAINER(vbox), table);

  mw_fscale_entry_new(table, "Exponent ", -7.5, 7.5, 0.01, 5.0, 0.0,
                      0, 1, 0, 1, &svals.blend_power);
  gtk_widget_show(table);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
  gtk_widget_show(page);



  gtk_widget_show(dlg);
  gtk_main();
  gdk_flush();

  /* argp->type = mw_radio_result(mode); */
  return runp;
}

/******************************************************************/
/* Draw preview image. if DoCompute is TRUE then recompute image. */
/******************************************************************/

void sinus_do_preview(GtkWidget *w)
{
  static GtkWidget *theWidget = NULL;
  gint y,rowsize;
  guchar *buf, *savbuf;
  params p;

  if(theWidget==NULL){
    theWidget=w;
  }


  rowsize=thePreview->width*thePreview->bpp;
  savbuf = buf = (guchar *)malloc(thePreview->width*thePreview->height*thePreview->bpp);
  if (buf != NULL) {
    p.height= thePreview->height;
    p.width= thePreview->width;
    prepare_coef(&p);


    if (thePreview->bpp == 3)
      compute_block_3(buf, rowsize,0,0,thePreview->width,thePreview->height, &p);
    else if (thePreview->bpp == 1) {
      compute_block_1(buf, rowsize,0,0,thePreview->width,thePreview->height, &p);
    }
    else
      fprintf(stderr,"Uh Oh....  Little sinus preview-only problem...\n");

    for (y=0;y<thePreview->height; y++) {
      gtk_preview_draw_row(GTK_PREVIEW(theWidget),
                           buf, 0, y, thePreview->width);
      buf+= rowsize;
    }
    free(savbuf);
    gtk_widget_draw(theWidget, NULL);
    gdk_flush();
  } else {
    fprintf(stderr,"Not enough mem for sinus Preview...\n");
  }
}
