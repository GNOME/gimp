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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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


#include <stdlib.h>
#include <math.h>
#include "libgimp/gimp.h"
#include "gtk/gtk.h"

/* --- Defines --- */
#define ENTRY_WIDTH 75

/* --- Typedefs --- */
typedef struct {
    gint posx;
    gint posy;
} FlareValues;

typedef struct {
    gint run;
} FlareInterface;

typedef struct RGBFLOAT {
  gfloat r;
  gfloat g;
  gfloat b;
} RGBfloat;

typedef struct REFLECT {
  RGBfloat ccol;
  gfloat size;
  gint   xp;
  gint   yp;
  gint   type;
} Reflect;

/* --- Declare local functions --- */
static void query (void);
static void run (char      *name,
	         int        nparams,
       	         GParam    *param,
		 int       *nreturn_vals,
	         GParam   **return_vals);
static gint flare_dialog (void);
static void flare_close_callback (GtkWidget *widget, gpointer data);
static void flare_ok_callback (GtkWidget *widget, gpointer data);
static void flare_entry_callback (GtkWidget *widget, gpointer data);
static void FlareFX (GDrawable *drawable);
void mcolor (guchar *s, gfloat h);
void mglow (guchar *s, gfloat h);
void minner (guchar *s, gfloat h);
void mouter (guchar *s, gfloat h);
void mhalo (guchar *s, gfloat h);
void initref (gint sx, gint sy, gint width, gint height, gint matt);
void fixpix (guchar *data, float procent, RGBfloat colpro);
void mrt1 (guchar *s, gint i, gint col, gint row);
void mrt2 (guchar *s, gint i, gint col, gint row);
void mrt3 (guchar *s, gint i, gint col, gint row);
void mrt4 (guchar *s, gint i, gint col, gint row);

/* --- Variables --- */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static FlareValues fvals =
{
    50,    /* posx */
    50     /* posy */
};

static FlareInterface fint =
{
  FALSE     /* run */
};

gfloat scolor, sglow, sinner, souter; /* size     */
gfloat shalo;
gint   xs, ys;     
gint   numref;
RGBfloat color, glow, inner, outer, halo;
Reflect ref1[19];

/* --- Functions --- */
MAIN ()

static void query () 
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

  gimp_install_procedure ("plug_in_flarefx",
			  "Add lens flare effetcs",
			  "More here later",
			  "Karl-Johan Andersson", /* Author */
			  "Karl-Johan Andersson", /* Copyright */
			  "1998",
			  "<Image>/Filters/Light Effects/FlareFX",
			  "RGB*",
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
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_flarefx", &fvals);

      /*  First acquire information with a dialog  */
      if (! flare_dialog ())
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
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
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_flarefx", &fvals);
      break;

    default:
      break;
    }
  
  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id))
	{
	  gimp_progress_init ("Render flare...");
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

static gint flare_dialog()
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *entry;
  gchar **argv;
  gint  argc;
  gchar buffer[32];

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("flarefx");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "FlareFX");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) flare_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) flare_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ("Parameter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new ("X-position");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0 );
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0 );
  gtk_widget_set_usize( entry, ENTRY_WIDTH, 0 );
  sprintf( buffer, "%d", fvals.posx );
  gtk_entry_set_text (GTK_ENTRY (entry), buffer );
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) flare_entry_callback,
		      &fvals.posx);
  gtk_widget_show (entry);

  label = gtk_label_new ("Y-position");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0 );
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0 );
  gtk_widget_set_usize( entry, ENTRY_WIDTH, 0 );
  sprintf( buffer, "%d", fvals.posy );
  gtk_entry_set_text (GTK_ENTRY (entry), buffer );
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) flare_entry_callback,
		      &fvals.posy);
  gtk_widget_show (entry);

  /*  label = gtk_label_new ("Turbulence");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);

  scale_data = gtk_adjustment_new (pvals.turbulence, 0.1, 7.0, 0.1, 0.1, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 1);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) plasma_scale_update,
			  &pvals.turbulence);
  gtk_widget_show (scale); */

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return fint.run;
}

/* --- Interface functions --- */
static void flare_close_callback (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}

static void flare_ok_callback (GtkWidget *widget, gpointer data)
{
  fint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void flare_entry_callback (GtkWidget *widget, gpointer data)
{
  gint *text_val;

  text_val = (gint*)data;
  *text_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}

/* --- Filter functions --- */
static void FlareFX (GDrawable *drawable)
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

void mcolor (guchar *s, gfloat h)
{
  static gfloat procent;
  
  procent = scolor - h;
  procent/=scolor;
  if (procent > 0.0) {
    procent*=procent;
    fixpix (s, procent, color); 
  }
}

void mglow (guchar *s, gfloat h)
{
  static gfloat procent;
  
  procent = sglow - h;
  procent/=sglow;
  if (procent > 0.0) {
    procent*=procent;
    fixpix (s, procent, glow); 
  }
}

void minner (guchar *s, gfloat h)
{
  static gfloat procent;
  
  procent = sinner - h;
  procent/=sinner;
  if (procent > 0.0) {
    procent*=procent;
    fixpix (s, procent, inner); 
  }
}

void mouter (guchar *s, gfloat h)
{
  static gfloat procent;
  
  procent = souter - h;
  procent/=souter;
  if (procent > 0.0) 
    fixpix (s, procent, outer); 
}

void mhalo (guchar *s, gfloat h)
{
  static gfloat procent;
  
  procent = h - shalo;
  procent/=(shalo*0.07);
  procent = fabs (procent);
  if (procent < 1.0)
    fixpix (s, 1.0 - procent, halo); 
}

void fixpix (guchar *data, float procent, RGBfloat colpro)
{
  data[0] = data[0] + (255 - data[0]) * procent * colpro.r;
  data[1] = data[1] + (255 - data[1]) * procent * colpro.g;
  data[2] = data[2] + (255 - data[2]) * procent * colpro.b;
}

void initref (gint sx, gint sy, gint width, gint height, gint matt)
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

void mrt1 (guchar *s, gint i, gint col, gint row)
{
  static gfloat procent;
  
  procent = ref1[i].size - hypot (ref1[i].xp - col, ref1[i].yp - row);
  procent/=ref1[i].size;
  if (procent > 0.0) {
    procent*=procent;
    fixpix (s, procent, ref1[i].ccol); 
  }  
}

void mrt2 (guchar *s, gint i, gint col, gint row)
{
  static gfloat procent;
  
  procent = ref1[i].size - hypot (ref1[i].xp - col, ref1[i].yp - row);
  procent/=(ref1[i].size * 0.15);
  if (procent > 0.0) {
    if (procent > 1.0) procent = 1.0;
    fixpix (s, procent, ref1[i].ccol); 
  }  
}

void mrt3 (guchar *s, gint i, gint col, gint row)
{
  static gfloat procent;
  
  procent = ref1[i].size - hypot (ref1[i].xp - col, ref1[i].yp - row);
  procent/=(ref1[i].size * 0.12);
  if (procent > 0.0) {
    if (procent > 1.0) procent = 1.0 - (procent * 0.12);
    fixpix (s, procent, ref1[i].ccol); 
  }  
}

void mrt4 (guchar *s, gint i, gint col, gint row)
{
  static gfloat procent;
  
  procent = hypot (ref1[i].xp - col, ref1[i].yp - row) - ref1[i].size;
  procent/=(ref1[i].size*0.04);
  procent = fabs (procent);
  if (procent < 1.0) 
    fixpix(s, 1.0 - procent, ref1[i].ccol); 
}
