/*
 * This is the FlareFX plug-in for the GIMP 0.99
 * Version 1.05
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
 * May 2000 - tim copperfield [timecop@japan.co.jp]
 * preview window now draws a "mini flarefx" to show approximate
 * positioning after final render.
 *
 * Note, the algorithm does not render into an alpha channel.
 * Therefore, changed RGB* to RGB in the capabilities.
 * Someone who actually knows something about graphics should
 * take a look to see why this doesnt render on alpha channel :)
 *  
 */ 

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* --- Defines --- */
#define ENTRY_WIDTH  75
#define PREVIEW_SIZE 128
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

static void FlareFX                    (GDrawable *drawable, 
					gint       preview_mode);
static void fill_preview_with_thumb    (GtkWidget *preview_widget, 
					gint32     drawable_ID);
static GtkWidget *preview_widget       (GDrawable *drawable);

static gint flare_dialog               (GDrawable *drawable);
static void flare_ok_callback          (GtkWidget *widget,
					gpointer   data);
static void flare_show_cursor_callback (GtkWidget *widget,
					gpointer   data);

static GtkWidget * flare_center_create            (GDrawable     *drawable);
static void	   flare_center_destroy           (GtkWidget     *widget,
						   gpointer       data);
static void	   flare_center_draw              (FlareCenter   *center,
						   gint           update);
static void	   flare_center_adjustment_update (GtkAdjustment *adjustment,
						   gpointer       data);
static void	   flare_center_cursor_update     (FlareCenter   *center);
static gint	   flare_center_preview_expose    (GtkWidget     *widget,
						   GdkEvent      *event);
static gint	   flare_center_preview_events    (GtkWidget     *widget,
						   GdkEvent      *event);

static void mcolor  (guchar *s, gfloat h);
static void mglow   (guchar *s, gfloat h);
static void minner  (guchar *s, gfloat h);
static void mouter  (guchar *s, gfloat h);
static void mhalo   (guchar *s, gfloat h);
static void initref (gint sx, gint sy, gint width, gint height, gint matt);
static void fixpix  (guchar *data, float procent, RGBfloat colpro);
static void mrt1    (guchar *s, gint i, gint col, gint row);
static void mrt2    (guchar *s, gint i, gint col, gint row);
static void mrt3    (guchar *s, gint i, gint col, gint row);
static void mrt4    (guchar *s, gint i, gint col, gint row);

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

static  gfloat     scolor, sglow, sinner, souter; /* size     */
static  gfloat     shalo;
static  gint       xs, ys;     
static  gint       numref;
static  RGBfloat   color, glow, inner, outer, halo;
static  Reflect    ref1[19];
static  guchar    *preview_bits;
static  GtkWidget *preview;
static  gdouble    preview_scale_x;
static  gdouble    preview_scale_y;
static  gboolean   show_cursor = 0;

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
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_flarefx",
			  "Add lens flare effects",
			  "Adds a lens flare effects.  Makes your image look like it was snapped with a cheap camera with a lot of lens :)",
			  "Karl-Johan Andersson", /* Author */
			  "Karl-Johan Andersson", /* Copyright */
			  "May 2000",
			  N_("<Image>/Filters/Light Effects/FlareFX..."),
			  "RGB",
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
      if (gimp_drawable_is_rgb (drawable->id) ||
	  gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init (_("Render Flare..."));
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
	  
	  FlareFX (drawable, 0);
	  
	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush (); 

	  /*  Store data  */
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_flarefx", &fvals, sizeof (FlareValues));
	    g_free(preview_bits);
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
  GtkWidget *main_vbox;
  GtkWidget *toggle;
  GtkWidget *frame;
  FlareCenter *center;

  gimp_ui_init ("flarefx", TRUE);

  dlg = gimp_dialog_new (_("FlareFX"), "flarefx",
			 gimp_standard_help_func, "filters/flarefx.html",
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
  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);


  frame = flare_center_create (drawable);
  center = gtk_object_get_user_data (GTK_OBJECT (frame));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

  /* show / hide cursor */
  toggle = gtk_check_button_new_with_label (_("Show Cursor"));
  gtk_container_set_border_width (GTK_CONTAINER (toggle), 6);
  gtk_object_set_user_data (GTK_OBJECT (toggle), center);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), show_cursor);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      GTK_SIGNAL_FUNC (flare_show_cursor_callback),
                      &show_cursor);
  gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);


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

static void
flare_show_cursor_callback (GtkWidget *widget, 
			    gpointer   data)
{
  FlareCenter *center;

  center = gtk_object_get_user_data (GTK_OBJECT (widget));
  gimp_toggle_button_update (widget, data);
  FlareFX (center->drawable, TRUE);
}

/* --- Filter functions --- */
static void
FlareFX (GDrawable *drawable, 
	 gboolean   preview_mode)
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

  if (preview_mode) 
    {
      width  = GTK_PREVIEW (preview)->buffer_width;
      height = GTK_PREVIEW (preview)->buffer_height;
      bytes  = GTK_PREVIEW (preview)->bpp;
      gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

      xs = (gdouble)fvals.posx * preview_scale_x;
      ys = (gdouble)fvals.posy * preview_scale_y;

      /* now, we clobber the x1 x2 y1 y2 with the real sizes */
      x1 = y1 = 0;
      x2 = width;
      y2 = height;
    } 
  else 
    {
      gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
      width  = drawable->width;
      height = drawable->height;
      bytes  = drawable->bpp;

      xs = fvals.posx; /* set x,y of flare center */
      ys = fvals.posy;
    }

  matt = width;
  /*  allocate row buffers  */
  cur_row  = g_new (guchar, (x2 - x1) * bytes);
  dest     = g_new (guchar, (x2 - x1) * bytes);

  if (!preview_mode) 
    {
      /*  initialize the pixel regions  */
      gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
      gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);
    }

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
      if (preview_mode) 
	memcpy(cur_row,preview_bits+(width*bytes*row),width*bytes);
      else 
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
	  for (i = 0; i < numref; i++) 
	    {
	      switch (ref1[i].type) 
		{
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
      if (preview_mode) 
	{
	  memcpy (GTK_PREVIEW (preview)->buffer + (width * bytes * row), 
		  cur_row, width * bytes);
	} 
      else 
	{
	  /*  store the dest  */
	  gimp_pixel_rgn_set_row (&destPR, cur_row, x1, row, (x2 - x1));
	}
      
      if ((row % 5) == 0 && !preview_mode)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  if (preview_mode) 
    {
      gtk_widget_queue_draw (preview);
    } 
  else 
    {
      /*  update the textured region  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->id, TRUE);
      gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
    }

  g_free (cur_row);
  g_free (dest);
}

static void
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

static void
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

static void
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

static void
mouter (guchar *s,
	gfloat  h)
{
  static gfloat procent;
  
  procent = souter - h;
  procent/=souter;
  if (procent > 0.0) 
    fixpix (s, procent, outer); 
}

static void
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

static void
fixpix (guchar   *data,
	float     procent,
	RGBfloat  colpro)
{
  data[0] = data[0] + (255 - data[0]) * procent * colpro.r;
  data[1] = data[1] + (255 - data[1]) * procent * colpro.g;
  data[2] = data[2] + (255 - data[2]) * procent * colpro.b;
}

static void
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

static void
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

static void
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

static void
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

static void
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
    This whole thing is just too ugly, but I don't want to dig into it
     - tim
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
  preview = preview_widget (drawable);
  gtk_object_set_user_data (GTK_OBJECT (preview), center);
  gtk_widget_set_events (GTK_WIDGET (preview), PREVIEW_MASK);
  gtk_signal_connect_after (GTK_OBJECT (preview), "expose_event",
			    (GtkSignalFunc) flare_center_preview_expose,
			    center);
  gtk_signal_connect (GTK_OBJECT (preview), "event",
		      (GtkSignalFunc) flare_center_preview_events,
		      center);
  gtk_container_add (GTK_CONTAINER (pframe ), preview);
  gtk_widget_show (preview);

  gtk_widget_show (pframe);
  gtk_widget_show (table);
  gtk_object_set_user_data (GTK_OBJECT (frame), center);
  gtk_widget_show (frame);

  flare_center_cursor_update (center);

  center->cursor = FALSE;    /* Make sure that the cursor has not been drawn */
  center->in_call = FALSE;   /* End of initialization */
  FlareFX(drawable, 1);
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

/*
 *  Initialize preview
 *  Draw the contents into the internal buffer of the preview widget
 */

static GtkWidget *
preview_widget (GDrawable *drawable)
{
  GtkWidget *preview;
  gint       size;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  fill_preview_with_thumb (preview, drawable->id);
  size = (GTK_PREVIEW (preview)->buffer_width) * 
	 (GTK_PREVIEW (preview)->buffer_height) * 
	 (GTK_PREVIEW (preview)->bpp);
  preview_bits = g_malloc (size);
  memcpy (preview_bits, GTK_PREVIEW (preview)->buffer, size);

  return preview;
}

static void
fill_preview_with_thumb (GtkWidget *widget, 
			 gint32     drawable_ID)
{
  guchar  *drawable_data;
  gint     bpp;
  gint     x,y;
  gint     width  = PREVIEW_SIZE;
  gint     height = PREVIEW_SIZE;
  guchar  *src;
  gdouble  r, g, b, a;
  gdouble  c0, c1;
  guchar  *p0, *p1;
  guchar  *even, *odd;

  bpp = 0; /* Only returned */
  
  drawable_data = 
    gimp_drawable_get_thumbnail_data (drawable_ID, &width, &height, &bpp);

  if (width < 1 || height < 1)
    return;

  gtk_preview_size (GTK_PREVIEW (widget), width, height);
  preview_scale_x = (gdouble)width  / (gdouble)gimp_drawable_width  (drawable_ID);
  preview_scale_y = (gdouble)height / (gdouble)gimp_drawable_height (drawable_ID);

  even = g_malloc (width * 3);
  odd  = g_malloc (width * 3);
  src  = drawable_data;

  for (y = 0; y < height; y++)
    {
      p0 = even;
      p1 = odd;
      
      for (x = 0; x < width; x++) 
	{
	  if (bpp == 4)
	    {
	      r = ((gdouble)src[x*4+0]) / 255.0;
	      g = ((gdouble)src[x*4+1]) / 255.0;
	      b = ((gdouble)src[x*4+2]) / 255.0;
	      a = ((gdouble)src[x*4+3]) / 255.0;
	    }
	  else if (bpp == 3)
	    {
	      r = ((gdouble)src[x*3+0]) / 255.0;
	      g = ((gdouble)src[x*3+1]) / 255.0;
	      b = ((gdouble)src[x*3+2]) / 255.0;
	      a = 1.0;
	    }
	else
	  {
	    r = ((gdouble)src[x*bpp+0]) / 255.0;
	    g = b = r;
	    if(bpp == 2)
	      a = ((gdouble)src[x*bpp+1]) / 255.0;
	    else
	      a = 1.0;
	  }
	
	if ((x / GIMP_CHECK_SIZE_SM) & 1) 
	  {
	    c0 = GIMP_CHECK_LIGHT;
	    c1 = GIMP_CHECK_DARK;
	  } 
	else 
	  {
	    c0 = GIMP_CHECK_DARK;
	    c1 = GIMP_CHECK_LIGHT;
	  }
	
	*p0++ = (c0 + (r - c0) * a) * 255.0;
	*p0++ = (c0 + (g - c0) * a) * 255.0;
	*p0++ = (c0 + (b - c0) * a) * 255.0;
	
	*p1++ = (c1 + (r - c1) * a) * 255.0;
	*p1++ = (c1 + (g - c1) * a) * 255.0;
	*p1++ = (c1 + (b - c1) * a) * 255.0;
	
      } /* for */
      
      if ((y / GIMP_CHECK_SIZE_SM) & 1)
	{
	  gtk_preview_draw_row (GTK_PREVIEW (widget), (guchar *)odd,  0, y, width);
	}
      else
	{
	  gtk_preview_draw_row (GTK_PREVIEW (widget), (guchar *)even, 0, y, width);
	}
      src += width * bpp;
    }

  g_free (even);
  g_free (odd);
  g_free (drawable_data);
}

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
      gdk_gc_set_function (preview->style->black_gc, GDK_INVERT);

      if (show_cursor) 
	{
	  if (center->cursor)
	    {
	      gdk_draw_line (preview->window,
			     preview->style->black_gc,
			     center->oldx, 1, 
			     center->oldx, 
			     GTK_PREVIEW (preview)->buffer_height - 1);
	      gdk_draw_line (preview->window,
			     preview->style->black_gc,
			     1, center->oldy, 
			     GTK_PREVIEW (preview)->buffer_width - 1, 
			     center->oldy);
	    }

	  gdk_draw_line (preview->window,
			 preview->style->black_gc,
			 center->curx, 1, 
			 center->curx, 
			 GTK_PREVIEW (preview)->buffer_height - 1);
	  gdk_draw_line (preview->window,
			 preview->style->black_gc,
			 1, center->cury, 
			 GTK_PREVIEW (preview)->buffer_width - 1, 
			 center->cury);
	}

      /* current position of cursor is updated */
      center->oldx = center->curx;
      center->oldy = center->cury;
      center->cursor = TRUE;

      gdk_gc_set_function (preview->style->black_gc, GDK_COPY);
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
      FlareFX(center->drawable, 1);
    }
}

/*
 *  Update the cross cursor's coordinates accoding to pvals.[xy]center
 *  but not redraw it
 */

static void
flare_center_cursor_update (FlareCenter *center)
{
  center->curx = 
    fvals.posx * GTK_PREVIEW (preview)->buffer_width / center->dwidth;
  center->cury = 
    fvals.posy * GTK_PREVIEW (preview)->buffer_height / center->dheight;

  if ( center->curx < 0)		     
    center->curx = 0;
  else if (center->curx >= GTK_PREVIEW (preview)->buffer_width)   
    center->curx = GTK_PREVIEW (preview)->buffer_width - 1;

  if (center->cury < 0)		             
    center->cury = 0;
  else if (center->cury >= GTK_PREVIEW (preview)->buffer_height)  
    center->cury = GTK_PREVIEW (preview)->buffer_height - 1;
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
      if (!mevent->state) 
	break;
      center->curx = mevent->x;
      center->cury = mevent->y;
    mouse:
      flare_center_draw (center, CURSOR);
      center->in_call = TRUE;
      gtk_adjustment_set_value (GTK_ADJUSTMENT (center->xadj),
                                center->curx * center->dwidth /
                                GTK_PREVIEW (preview)->buffer_width);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (center->yadj),
                                center->cury * center->dheight /
                                GTK_PREVIEW (preview)->buffer_height);
      center->in_call = FALSE;
      FlareFX(center->drawable, 1);
      break;

    default:
      break;
    }

  return FALSE;
}
