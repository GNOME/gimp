/*
 * This is the FlareFX plug-in for the GIMP 0.99
 * Version 1.04
 *
 * Copyright (C) 1997-1998 Karl-Johan Andersson (t96kja@student.tdb.uu.se)
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
 * Please send any comments or suggestions to me,
 * Karl-Johan Andersson (t96kja@student.tdb.uu.se)
 * 
 * TODO:
 * - add "streaks" from lightsource
 * - improve the user interface
 * - speed it up
 * - more flare types, more control (color, size, intensity...)
 *
 * Missing something? - please contact me!
 *  
 */ 

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/gimpmath.h>

#include "libgimp/stdplugins-intl.h"

/* --- Defines --- */
#define ENTRY_WIDTH   75
#define PREVIEW_SIZE 100
#define PREVIEW_MASK (GDK_EXPOSURE_MASK | \
		      GDK_BUTTON_PRESS_MASK | \
		      GDK_BUTTON1_MOTION_MASK)

#define PREVIEW	  0x1
#define CURSOR	  0x2
#define ALL	  0xf

#if 0
#define DEBUG1 printf
#else
#define DEBUG1 dummy_printf
static void
dummy_printf (gchar *fmt, ...) {}
#endif


/* --- Typedefs --- */
typedef struct
{
  gint posx;
  gint posy;
} FlareValues;

typedef struct
{
  gint run;
} FlareInterface;

typedef struct RGBFLOAT
{
  gfloat r;
  gfloat g;
  gfloat b;
} RGBfloat;

typedef struct REFLECT
{
  RGBfloat ccol;
  gfloat size;
  gint   xp;
  gint   yp;
  gint   type;
} Reflect;

typedef struct
{
  GDrawable *drawable;
  gint       dwidth;
  gint       dheight;
  gint       bpp;
  GtkObject *xadj;
  GtkObject *yadj;
  GtkWidget *preview;
  gint       pwidth;
  gint       pheight;
  gint       cursor;
  gint       curx, cury;		 /* x,y of cursor in preview */
  gint       oldx, oldy;
  gint       in_call;
} FlareCenter;

/* --- Declare local functions --- */
static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);

static void FlareFX (GDrawable *drawable);

static gint flare_dialog      (GDrawable *drawable);
static void flare_ok_callback (GtkWidget *widget,
			       gpointer   data);

static GtkWidget * flare_center_create            (GDrawable     *drawable);
static void	   flare_center_destroy           (GtkWidget     *widget,
						   gpointer       data);
static void	   flare_center_preview_init      (FlareCenter   *center);
static void	   flare_center_draw              (FlareCenter   *center,
						   gint           update);
static void	   flare_center_adjustment_update (GtkAdjustment *adjustment,
						   gpointer       data);
static void	   flare_center_cursor_update     (FlareCenter   *center);
static gint	   flare_center_preview_expose    (GtkWidget     *widget,
						   GdkEvent      *event);
static gint	   flare_center_preview_events    (GtkWidget     *widget,
						   GdkEvent      *event);

void mcolor  (guchar *s, gfloat h);
void mglow   (guchar *s, gfloat h);
void minner  (guchar *s, gfloat h);
void mouter  (guchar *s, gfloat h);
void mhalo   (guchar *s, gfloat h);
void initref (gint sx, gint sy, gint width, gint height, gint matt);
void fixpix  (guchar *data, float procent, RGBfloat colpro);
void mrt1    (guchar *s, gint i, gint col, gint row);
void mrt2    (guchar *s, gint i, gint col, gint row);
void mrt3    (guchar *s, gint i, gint col, gint row);
void mrt4    (guchar *s, gint i, gint col, gint row);

/* --- Variables --- */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static FlareValues fvals =
{
  128, 128		/* posx, posy */
};

static FlareInterface fint =
{
  FALSE     /* run */
};

gfloat   scolor, sglow, sinner, souter; /* size     */
gfloat   shalo;
gint     xs, ys;     
gint     numref;
RGBfloat color, glow, inner, outer, halo;
Reflect  ref1[19];

/* --- Functions --- */
MAIN ()

static void
query (void) 
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "posx", "X-position" },
    { PARAM_INT32, "posy", "Y-position" }
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0; 

  INIT_I18N();

  gimp_install_procedure ("plug_in_flarefx",
			  _("Add lens flare effects"),
			  _("More here later"),
			  "Karl-Johan Andersson", /* Author */
			  "Karl-Johan Andersson", /* Copyright */
			  "1998",
			  N_("<Image>/Filters/Light Effects/FlareFX..."),
			  "RGB*",
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
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  
  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  
  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_flarefx", &fvals);

      /*  First acquire information with a dialog  */
      if (! flare_dialog (drawable))
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  fvals.posx = (gint) param[3].data.d_int32;
	  fvals.posy = (gint) param[4].data.d_int32;
	}
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_flarefx", &fvals);
      break;

    default:
      break;
    }
  
  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init (_("Render flare..."));
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
	  
	  FlareFX (drawable);
	  
	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush (); 

	  /*  Store data  */
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_flarefx", &fvals, sizeof (FlareValues));
	}
      else
	{
	  /* gimp_message ("FlareFX: cannot operate on indexed color images"); */
	  status = STATUS_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;
  
  gimp_drawable_detach (drawable);
}

static gint
flare_dialog (GDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  guchar  *color_cube;
  gchar  **argv;
  gint     argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("flarefx");

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

#if 0
  g_print ("Waiting... (pid %d)\n", getpid ());
  kill (getpid (), 19); /* SIGSTOP */
#endif

  dlg = gimp_dialog_new (_("FlareFX"), "flarefx",
			 gimp_plugin_help_func, "filters/flarefx.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), flare_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*  parameter settings  */
  frame = flare_center_create (drawable);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  gtk_widget_show (frame);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return fint.run;
}

/* --- Interface functions --- */
static void
flare_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  fint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

/* --- Filter functions --- */
static void
FlareFX (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  guchar *dest, *d;
  guchar *cur_row, *s;
  gint row, col, i;
  gint x1, y1, x2, y2;
  gint matt;
  gfloat hyp;

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;
  matt = width;
  
  /*  allocate row buffers  */
  cur_row  = (guchar *) malloc ((x2 - x1) * bytes);
  dest = (guchar *) malloc ((x2 - x1) * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  xs = fvals.posx;
  ys = fvals.posy;

  scolor = (gfloat)matt * 0.0375;
  sglow  = (gfloat)matt * 0.078125;
  sinner = (gfloat)matt * 0.1796875;
  souter = (gfloat)matt * 0.3359375;
  shalo  = (gfloat)matt * 0.084375;

  color.r = 239.0/255.0; color.g = 239.0/255.0; color.b = 239.0/255.0;
  glow.r  = 245.0/255.0; glow.g  = 245.0/255.0; glow.b  = 245.0/255.0;
  inner.r = 255.0/255.0; inner.g = 38.0/255.0;  inner.b = 43.0/255.0;
  outer.r = 69.0/255.0;  outer.g = 59.0/255.0;  outer.b = 64.0/255.0;
  halo.r  = 80.0/255.0;  halo.g  = 15.0/255.0;  halo.b  = 4.0/255.0;
 
  initref (xs, ys, width, height, matt);

  /*  Loop through the rows */
  for (row = y1; row < y2; row++) /* y-coord */
    {
      gimp_pixel_rgn_get_row (&srcPR, cur_row, x1, row, x2-x1);
      d = dest;
      s = cur_row;
      for (col = x1; col < x2; col++) /* x-coord */
	{      
	  hyp = hypot (col-xs, row-ys);
	  mcolor (s, hyp); /* make color */
	  mglow (s, hyp);  /* make glow  */ 
	  minner (s, hyp); /* make inner */
	  mouter (s, hyp); /* make outer */
	  mhalo (s, hyp);  /* make halo  */
	  for (i = 0; i < numref; i++) {
	    switch (ref1[i].type) {
	    case 1: 
	      mrt1 (s, i, col, row); 
	      break;
	    case 2:
	      mrt2 (s, i, col, row);
	      break;
	    case 3:
	      mrt3 (s, i, col, row);
	      break;
	    case 4:
	      mrt4 (s, i, col, row);
	      break;
	    }
	  }
	  s+=bytes;
	}
      /*  store the dest  */
      gimp_pixel_rgn_set_row (&destPR, cur_row, x1, row, (x2 - x1));
      
      if ((row % 5) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  /*  update the textured region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  free (cur_row);
  free (dest);
}

void
mcolor (guchar *s,
	gfloat  h)
{
  static gfloat procent;
  
  procent = scolor - h;
  procent/=scolor;
  if (procent > 0.0)
    {
      procent*=procent;
      fixpix (s, procent, color); 
    }
}

void
mglow (guchar *s,
       gfloat  h)
{
  static gfloat procent;
  
  procent = sglow - h;
  procent/=sglow;
  if (procent > 0.0)
    {
      procent*=procent;
      fixpix (s, procent, glow); 
    }
}

void
minner (guchar *s,
	gfloat  h)
{
  static gfloat procent;
  
  procent = sinner - h;
  procent/=sinner;
  if (procent > 0.0)
    {
      procent*=procent;
      fixpix (s, procent, inner); 
    }
}

void
mouter (guchar *s,
	gfloat  h)
{
  static gfloat procent;
  
  procent = souter - h;
  procent/=souter;
  if (procent > 0.0) 
    fixpix (s, procent, outer); 
}

void
mhalo (guchar *s,
       gfloat  h)
{
  static gfloat procent;
  
  procent = h - shalo;
  procent/=(shalo*0.07);
  procent = fabs (procent);
  if (procent < 1.0)
    fixpix (s, 1.0 - procent, halo); 
}

void
fixpix (guchar   *data,
	float     procent,
	RGBfloat  colpro)
{
  data[0] = data[0] + (255 - data[0]) * procent * colpro.r;
  data[1] = data[1] + (255 - data[1]) * procent * colpro.g;
  data[2] = data[2] + (255 - data[2]) * procent * colpro.b;
}

void
initref (gint sx,
	 gint sy,
	 gint width,
	 gint height,
	 gint matt)
{
  gint xh, yh, dx, dy;
  
  xh = width / 2; yh = height / 2;
  dx = xh - sx;   dy = yh - sy;
  numref = 19;
  ref1[0].type=1; ref1[0].size=(gfloat)matt*0.027;
  ref1[0].xp=0.6699*dx+xh; ref1[0].yp=0.6699*dy+yh;
  ref1[0].ccol.r=0.0; ref1[0].ccol.g=14.0/255.0; ref1[0].ccol.b=113.0/255.0;
  ref1[1].type=1; ref1[1].size=(gfloat)matt*0.01;
  ref1[1].xp=0.2692*dx+xh; ref1[1].yp=0.2692*dy+yh;
  ref1[1].ccol.r=90.0/255.0; ref1[1].ccol.g=181.0/255.0; ref1[1].ccol.b=142.0/255.0;
  ref1[2].type=1; ref1[2].size=(gfloat)matt*0.005;
  ref1[2].xp=-0.0112*dx+xh; ref1[2].yp=-0.0112*dy+yh;
  ref1[2].ccol.r=56.0/255.0; ref1[2].ccol.g=140.0/255.0; ref1[2].ccol.b=106.0/255.0;
  ref1[3].type=2; ref1[3].size=(gfloat)matt*0.031;
  ref1[3].xp=0.6490*dx+xh; ref1[3].yp=0.6490*dy+yh;
  ref1[3].ccol.r=9.0/255.0; ref1[3].ccol.g=29.0/255.0; ref1[3].ccol.b=19.0/255.0;
  ref1[4].type=2; ref1[4].size=(gfloat)matt*0.015;
  ref1[4].xp=0.4696*dx+xh; ref1[4].yp=0.4696*dy+yh;
  ref1[4].ccol.r=24.0/255.0; ref1[4].ccol.g=14.0/255.0; ref1[4].ccol.b=0.0;
  ref1[5].type=2; ref1[5].size=(gfloat)matt*0.037;
  ref1[5].xp=0.4087*dx+xh; ref1[5].yp=0.4087*dy+yh;
  ref1[5].ccol.r=24.0/255.0; ref1[5].ccol.g=14.0/255.0; ref1[5].ccol.b=0.0;
  ref1[6].type=2; ref1[6].size=(gfloat)matt*0.022;
  ref1[6].xp=-0.2003*dx+xh; ref1[6].yp=-0.2003*dy+yh;
  ref1[6].ccol.r=42.0/255.0; ref1[6].ccol.g=19.0/255.0; ref1[6].ccol.b=0.0;
  ref1[7].type=2; ref1[7].size=(gfloat)matt*0.025;
  ref1[7].xp=-0.4103*dx+xh; ref1[7].yp=-0.4103*dy+yh;
  ref1[7].ccol.b=17.0/255.0; ref1[7].ccol.g=9.0/255.0; ref1[7].ccol.r=0.0;
  ref1[8].type=2; ref1[8].size=(gfloat)matt*0.058;
  ref1[8].xp=-0.4503*dx+xh; ref1[8].yp=-0.4503*dy+yh;
  ref1[8].ccol.b=10.0/255.0; ref1[8].ccol.g=4.0/255.0; ref1[8].ccol.r=0.0;
  ref1[9].type=2; ref1[9].size=(gfloat)matt*0.017;
  ref1[9].xp=-0.5112*dx+xh; ref1[9].yp=-0.5112*dy+yh;
  ref1[9].ccol.r=5.0/255.0; ref1[9].ccol.g=5.0/255.0; ref1[9].ccol.b=14.0/255.0;
  ref1[10].type=2; ref1[10].size=(gfloat)matt*0.2;
  ref1[10].xp=-1.496*dx+xh; ref1[10].yp=-1.496*dy+yh;
  ref1[10].ccol.r=9.0/255.0; ref1[10].ccol.g=4.0/255.0; ref1[10].ccol.b=0.0;
  ref1[11].type=2; ref1[11].size=(gfloat)matt*0.5;
  ref1[11].xp=-1.496*dx+xh; ref1[11].yp=-1.496*dy+yh;
  ref1[11].ccol.r=9.0/255.0; ref1[11].ccol.g=4.0/255.0; ref1[11].ccol.b=0.0;
  ref1[12].type=3; ref1[12].size=(gfloat)matt*0.075;
  ref1[12].xp=0.4487*dx+xh; ref1[12].yp=0.4487*dy+yh;
  ref1[12].ccol.r=34.0/255.0; ref1[12].ccol.g=19.0/255.0; ref1[12].ccol.b=0.0;
  ref1[13].type=3; ref1[13].size=(gfloat)matt*0.1;
  ref1[13].xp=dx+xh; ref1[13].yp=dy+yh;
  ref1[13].ccol.r=14.0/255.0; ref1[13].ccol.g=26.0/255.0; ref1[13].ccol.b=0.0;
  ref1[14].type=3; ref1[14].size=(gfloat)matt*0.039;
  ref1[14].xp=-1.301*dx+xh; ref1[14].yp=-1.301*dy+yh;
  ref1[14].ccol.r=10.0/255.0; ref1[14].ccol.g=25.0/255.0; ref1[14].ccol.b=13.0/255.0;
  ref1[15].type=4; ref1[15].size=(gfloat)matt*0.19;
  ref1[15].xp=1.309*dx+xh; ref1[15].yp=1.309*dy+yh;
  ref1[15].ccol.r=9.0/255.0; ref1[15].ccol.g=0.0; ref1[15].ccol.b=17.0/255.0;
  ref1[16].type=4; ref1[16].size=(gfloat)matt*0.195;
  ref1[16].xp=1.309*dx+xh; ref1[16].yp=1.309*dy+yh;
  ref1[16].ccol.r=9.0/255.0; ref1[16].ccol.g=16.0/255.0; ref1[16].ccol.b=5.0/255.0;
  ref1[17].type=4; ref1[17].size=(gfloat)matt*0.20;
  ref1[17].xp=1.309*dx+xh; ref1[17].yp=1.309*dy+yh;
  ref1[17].ccol.r=17.0/255.0; ref1[17].ccol.g=4.0/255.0; ref1[17].ccol.b=0.0;
  ref1[18].type=4; ref1[18].size=(gfloat)matt*0.038;
  ref1[18].xp=-1.301*dx+xh; ref1[18].yp=-1.301*dy+yh;
  ref1[18].ccol.r=17.0/255.0; ref1[18].ccol.g=4.0/255.0; ref1[18].ccol.b=0.0;
}

void
mrt1 (guchar *s,
      gint    i,
      gint    col,
      gint    row)
{
  static gfloat procent;
  
  procent = ref1[i].size - hypot (ref1[i].xp - col, ref1[i].yp - row);
  procent/=ref1[i].size;
  if (procent > 0.0)
    {
      procent*=procent;
      fixpix (s, procent, ref1[i].ccol); 
    }  
}

void
mrt2 (guchar *s,
      gint    i,
      gint    col,
      gint    row)
{
  static gfloat procent;
  
  procent = ref1[i].size - hypot (ref1[i].xp - col, ref1[i].yp - row);
  procent/=(ref1[i].size * 0.15);
  if (procent > 0.0)
    {
      if (procent > 1.0) procent = 1.0;
      fixpix (s, procent, ref1[i].ccol); 
    }  
}

void
mrt3 (guchar *s,
      gint    i,
      gint    col,
      gint    row)
{
  static gfloat procent;
  
  procent = ref1[i].size - hypot (ref1[i].xp - col, ref1[i].yp - row);
  procent/=(ref1[i].size * 0.12);
  if (procent > 0.0)
    {
      if (procent > 1.0) procent = 1.0 - (procent * 0.12);
      fixpix (s, procent, ref1[i].ccol); 
    }  
}

void
mrt4 (guchar *s,
      gint    i,
      gint    col,
      gint    row)
{
  static gfloat procent;
  
  procent = hypot (ref1[i].xp - col, ref1[i].yp - row) - ref1[i].size;
  procent/=(ref1[i].size*0.04);
  procent = fabs (procent);
  if (procent < 1.0) 
    fixpix(s, 1.0 - procent, ref1[i].ccol); 
}

/*=================================================================
    CenterFrame

    A frame that contains one preview and 2 entrys, used for positioning
    of the center of Flare.
==================================================================*/


/*
 * Create new CenterFrame, and return it (GtkFrame).
 */

static GtkWidget *
flare_center_create (GDrawable *drawable)
{
  FlareCenter *center;
  GtkWidget   *frame;
  GtkWidget   *table;
  GtkWidget   *label;
  GtkWidget   *pframe;
  GtkWidget   *preview;
  GtkWidget   *spinbutton;

  center = g_new (FlareCenter, 1);
  center->drawable = drawable;
  center->dwidth   = gimp_drawable_width(drawable->id );
  center->dheight  = gimp_drawable_height(drawable->id );
  center->bpp	   = gimp_drawable_bpp(drawable->id);
  if (gimp_drawable_has_alpha (drawable->id))
    center->bpp--;
  center->cursor   = FALSE;
  center->curx     = 0;
  center->cury     = 0;
  center->oldx     = 0;
  center->oldy     = 0;
  center->in_call  = TRUE;  /* to avoid side effects while initialization */

  frame = gtk_frame_new (_("Center of FlareFX"));
  gtk_signal_connect (GTK_OBJECT (frame), "destroy",
		      GTK_SIGNAL_FUNC (flare_center_destroy),
		      center);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);

  table = gtk_table_new (2, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new (_("X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5 );
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton =
    gimp_spin_button_new (&center->xadj,
                          fvals.posx, G_MININT, G_MAXINT,
                          1, 10, 10, 0, 0);
  gtk_object_set_user_data (GTK_OBJECT (center->xadj), center);
  gtk_signal_connect (GTK_OBJECT (center->xadj), "value_changed",
                      GTK_SIGNAL_FUNC (flare_center_adjustment_update),
                      &fvals.posx);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  label = gtk_label_new (_("Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5 );
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton =
    gimp_spin_button_new (&center->yadj,
                          fvals.posy, G_MININT, G_MAXINT,
                          1, 10, 10, 0, 0);
  gtk_object_set_user_data (GTK_OBJECT (center->yadj), center);
  gtk_signal_connect (GTK_OBJECT (center->yadj), "value_changed",
                      GTK_SIGNAL_FUNC (flare_center_adjustment_update),
                      &fvals.posy);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 3, 4, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  /* frame (shadow_in) that contains preview */
  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), pframe, 0, 4, 1, 2, 0, 0, 0, 0);

  /* PREVIEW */
  center->preview = preview = gtk_preview_new (center->bpp==3 ? GTK_PREVIEW_COLOR : GTK_PREVIEW_GRAYSCALE);
  gtk_object_set_user_data (GTK_OBJECT (preview), center);
  gtk_widget_set_events (GTK_WIDGET (preview), PREVIEW_MASK);
  gtk_signal_connect_after (GTK_OBJECT (preview), "expose_event",
			    (GtkSignalFunc) flare_center_preview_expose,
			    center );
  gtk_signal_connect (GTK_OBJECT (preview), "event",
		      (GtkSignalFunc) flare_center_preview_events,
		      center);
  gtk_container_add (GTK_CONTAINER (pframe ), center->preview);

  /*
   * Resize the greater one of dwidth and dheight to PREVIEW_SIZE
   */
  if ( center->dwidth > center->dheight )
    {
      center->pheight = center->dheight * PREVIEW_SIZE / center->dwidth;
      center->pwidth = PREVIEW_SIZE;
    }
  else
    {
      center->pwidth = center->dwidth * PREVIEW_SIZE / center->dheight;
      center->pheight = PREVIEW_SIZE;
    }
  gtk_preview_size (GTK_PREVIEW  (preview), center->pwidth, center->pheight);

  /* Draw the contents of preview, that is saved in the preview widget */
  flare_center_preview_init (center);
  gtk_widget_show (preview);

  gtk_widget_show (pframe);
  gtk_widget_show (table);
  gtk_widget_show (frame);

  flare_center_cursor_update (center);

  center->cursor = FALSE;    /* Make sure that the cursor has not been drawn */
  center->in_call = FALSE;   /* End of initialization */
  DEBUG1 ("fvals center=%d,%d\n", fvals.posx, fvals.posy);
  DEBUG1 ("center cur=%d,%d\n", center->curx, center->cury);

  return frame;
}

static void
flare_center_destroy (GtkWidget *widget,
		      gpointer   data)
{
  FlareCenter *center = data;
  g_free (center);
}

static void
render_preview (GtkWidget *preview,
		GPixelRgn *srcrgn);

/*
 *  Initialize preview
 *  Draw the contents into the internal buffer of the preview widget
 */
static void
flare_center_preview_init (FlareCenter *center)
{
  GtkWidget *preview;
  GPixelRgn  src_rgn;
  gint	     dwidth, dheight, pwidth, pheight, bpp;

  preview = center->preview;
  dwidth = center->dwidth;
  dheight = center->dheight;
  pwidth = center->pwidth;
  pheight = center->pheight;
  bpp = center->bpp;

  gimp_pixel_rgn_init (&src_rgn, center->drawable, 0, 0,
		       center->dwidth, center->dheight, FALSE, FALSE);
  render_preview (center->preview, &src_rgn);
}


/*======================================================================
		Preview Rendering Util routine
=======================================================================*/

#define CHECKWIDTH 4
#define LIGHTCHECK 192
#define DARKCHECK  128
#ifndef OPAQUE
#define OPAQUE	   255
#endif

static void
render_preview (GtkWidget *preview,
		GPixelRgn *srcrgn)
{
  guchar *src_row, *dest_row, *src, *dest;
  gint    row, col;
  gint    dwidth, dheight, pwidth, pheight;
  gint   *src_col;
  gint    bpp, alpha, has_alpha, b;
  guchar  check;

  dwidth  = srcrgn->w;
  dheight = srcrgn->h;
  if (GTK_PREVIEW (preview)->buffer)
    {
      pwidth  = GTK_PREVIEW (preview)->buffer_width;
      pheight = GTK_PREVIEW (preview)->buffer_height;
    }
  else
    {
      pwidth  = preview->requisition.width;
      pheight = preview->requisition.height;
    }

  bpp = srcrgn->bpp;
  alpha = bpp;
  has_alpha = gimp_drawable_has_alpha (srcrgn->drawable->id);
  if (has_alpha) alpha--;
  /*  printf("render_preview: %d %d %d", bpp, alpha, has_alpha);
      printf(" (%d %d %d %d)\n", dwidth, dheight, pwidth, pheight); */

  src_row = g_new (guchar, dwidth * bpp);
  dest_row = g_new (guchar, pwidth * bpp);
  src_col = g_new (gint, pwidth);

  for (col = 0; col < pwidth; col++)
    src_col[ col ] = (col * dwidth / pwidth) * bpp;

  for (row = 0; row < pheight; row++)
    {
      gimp_pixel_rgn_get_row (srcrgn, src_row,
			      0, row * dheight / pheight, dwidth);
      dest = dest_row;
      for (col = 0; col < pwidth; col++)
	{
	  src = &src_row[ src_col[col] ];
	  if (!has_alpha || src[alpha] == OPAQUE)
	    {
	      /* no alpha channel or opaque -- simple way */
	      for ( b = 0; b < alpha; b++ )
		dest[b] = src[b];
	    }
	  else
	    {
	      /* more or less transparent */
	      if ((col % (CHECKWIDTH*2) < CHECKWIDTH) ^
		  (row % (CHECKWIDTH*2) < CHECKWIDTH))
		check = LIGHTCHECK;
	      else
		check = DARKCHECK;

	      if (src[alpha] == 0)
		{
		  /* full transparent -- check */
		  for (b = 0; b < alpha; b++)
		    dest[b] = check;
		}
	      else
		{
		  /* middlemost transparent -- mix check and src */
		  for (b = 0; b < alpha; b++)
		    dest[b] = (src[b]*src[alpha] + check*(OPAQUE-src[alpha])) / OPAQUE;
		}
	    }
	  dest += alpha;
	}
      gtk_preview_draw_row (GTK_PREVIEW (preview), dest_row,
			    0, row, pwidth);
    }

  g_free (src_col);
  g_free (src_row);
  g_free (dest_row);
}

/*======================================================================
		Preview Rendering Util routine End
=======================================================================*/

/*
 *   Drawing CenterFrame
 *   if update & PREVIEW, draw preview
 *   if update & CURSOR,  draw cross cursor
 */

static void
flare_center_draw (FlareCenter *center,
		   gint         update)
{
  if (update & PREVIEW)
    {
      center->cursor = FALSE;
      DEBUG1 ("draw-preview\n");
    }

  if (update & CURSOR)
    {
      DEBUG1 ("draw-cursor %d old=%d,%d cur=%d,%d\n",
	      center->cursor,
	      center->oldx, center->oldy, center->curx, center->cury);
      gdk_gc_set_function (center->preview->style->black_gc, GDK_INVERT);
      if (center->cursor)
	{
	  gdk_draw_line (center->preview->window,
			 center->preview->style->black_gc,
			 center->oldx, 1, center->oldx, center->pheight-1);
	  gdk_draw_line (center->preview->window,
			 center->preview->style->black_gc,
			 1, center->oldy, center->pwidth-1, center->oldy);
	}
      gdk_draw_line (center->preview->window,
		     center->preview->style->black_gc,
		     center->curx, 1, center->curx, center->pheight-1);
      gdk_draw_line (center->preview->window,
		     center->preview->style->black_gc,
		     1, center->cury, center->pwidth-1, center->cury);
      /* current position of cursor is updated */
      center->oldx = center->curx;
      center->oldy = center->cury;
      center->cursor = TRUE;
      gdk_gc_set_function (center->preview->style->black_gc, GDK_COPY);
    }
}


/*
 *  CenterFrame entry callback
 */

static void
flare_center_adjustment_update (GtkAdjustment *adjustment,
				gpointer       data)
{
  FlareCenter *center;

  gimp_int_adjustment_update (adjustment, data);

  center = gtk_object_get_user_data (GTK_OBJECT (adjustment));

  if (!center->in_call)
    {
      flare_center_cursor_update (center);
      flare_center_draw (center, CURSOR);
    }
}

/*
 *  Update the cross cursor's  coordinates accoding to pvals.[xy]center
 *  but not redraw it
 */

static void
flare_center_cursor_update (FlareCenter *center)
{
  center->curx = fvals.posx * center->pwidth / center->dwidth;
  center->cury = fvals.posy * center->pheight / center->dheight;

  if ( center->curx < 0)		     center->curx = 0;
  else if (center->curx >= center->pwidth)   center->curx = center->pwidth-1;
  if (center->cury < 0)		             center->cury = 0;
  else if (center->cury >= center->pheight)  center->cury = center->pheight-1;

}

/*
 *    Handle the expose event on the preview
 */
static gint
flare_center_preview_expose (GtkWidget *widget,
			     GdkEvent  *event)
{
  FlareCenter *center;

  center = gtk_object_get_user_data (GTK_OBJECT (widget));
  flare_center_draw (center, ALL);
  return FALSE;
}


/*
 *    Handle other events on the preview
 */

static gint
flare_center_preview_events (GtkWidget *widget,
			     GdkEvent  *event)
{
  FlareCenter *center;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;

  center = gtk_object_get_user_data (GTK_OBJECT (widget));

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      center->curx = bevent->x;
      center->cury = bevent->y;
      goto mouse;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if (!mevent->state) break;
      center->curx = mevent->x;
      center->cury = mevent->y;
    mouse:
      flare_center_draw (center, CURSOR);
      center->in_call = TRUE;
      gtk_adjustment_set_value (GTK_ADJUSTMENT (center->xadj),
                                center->curx * center->dwidth /
                                center->pwidth);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (center->yadj),
                                center->cury * center->dheight /
                                center->pheight);
      center->in_call = FALSE;
      break;

    default:
      break;
    }

  return FALSE;
}
