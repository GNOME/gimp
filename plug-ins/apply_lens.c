/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Apply lens plug-in --- makes your selected part of the image look like it
 *                        is viewed under a solid lens.
 * Copyright (C) 1997 Morten Eriksen
 * mortene@pvv.ntnu.no
 * (If you do anything cool with this plug-in, or have ideas for
 * improvements (which aren't on my ToDo-list) - send me an email).
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
 */

/*
 * Compile with (on Linux):
 * gcc -I/usr/local/include -I/usr/local/include/glib -o apply_lens apply_lens.c -L/usr/local/lib -L/usr/X11/lib -lgtk -lgdk -lgimp -lglib -lXext -lX11 -lm
 *
 */

/* Version 0.1:
 * 
 * First release. No known serious bugs, and basically does what you want.
 * All fancy features postponed until the next release, though. :)
 *
 */

/*
  TO DO:
  - antialiasing
  - preview image
  - adjustable (R, G, B and A) filter
  - optimize for speed!
  - refraction index warning dialog box when value < 1.0
  - use "true" lens with specified thickness
  - option to apply inverted lens
  - adjustable "c" value in the ellipsoid formula
  - radiobuttons for "ellipsoid" or "only horiz" and "only vert" (like in the
    Ad*b* Ph*t*sh*p Spherify plug-in..)
  - clean up source code
 */

#include <stdlib.h>
#include <math.h>

#include "libgimp/gimp.h"
#include "gtk/gtk.h"

#define ENTRY_WIDTH 100

/* Declare local functions.
 */
static void query(void);
static void run(char *name, int nparams,
		GParam *param,
		int *nreturn_vals,
		GParam **return_vals);

static void drawlens(GDrawable *drawable);
static gint lens_dialog(GDrawable *drawable);

GPlugInInfo PLUG_IN_INFO =
{
  NULL, /* init_proc */
  NULL, /* quit_proc */
  query, /* query_proc */
  run, /* run_proc */
};

typedef struct
{
  gdouble refraction;
  gint keep_surr, use_bkgr, set_transparent;
} LensValues;

static LensValues lvals =
{
  /* Lens refraction value */
  1.7,
  /* Surroundings options */
  TRUE, FALSE, FALSE
};

typedef struct
{
  gint run;
} LensInterface;

static LensInterface bint =
{
  FALSE  /*  run  */
};

MAIN()

static void
query(void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_FLOAT, "refraction", "Lens refraction index" },
    { PARAM_INT32, "keep_surroundings", "Keep lens surroundings" },
    { PARAM_INT32, "set_background", "Set lens surroundings to bkgr value" },
    { PARAM_INT32, "set_transparent", "Set lens surroundings transparent" },
  };

  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args)/ sizeof(args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure("plug_in_applylens",
			 "Apply a lens effect",
			 "This plug-in uses Snell's law to draw an ellipsoid lens over the image",
			 "Morten Eriksen",
			 "Morten Eriksen",
			 "1997",
			 "<Image>/Filters/Distorts/Apply lens",
			 "RGB*, GRAY*, INDEXED*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);
}

static void
run(char *name,
    int nparams,
    GParam *param,
    int *nreturn_vals,
    GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  
  *nreturn_vals = 1;
  *return_vals = values;
  
  drawable = gimp_drawable_get(param[2].data.d_drawable);

  switch(run_mode) {
  case RUN_INTERACTIVE:
    gimp_get_data("plug_in_applylens", &lvals);
    if(!lens_dialog(drawable)) return;
    break;

  case RUN_NONINTERACTIVE:
    if(nparams != 7) status = STATUS_CALLING_ERROR;
    if(status == STATUS_SUCCESS) {
      lvals.refraction = param[3].data.d_float;
      lvals.keep_surr = param[4].data.d_int32;
      lvals.use_bkgr = param[5].data.d_int32;
      lvals.set_transparent = param[6].data.d_int32;
    }

    if(status == STATUS_SUCCESS && (lvals.refraction < 1.0))
      status = STATUS_CALLING_ERROR;
    break;

  case RUN_WITH_LAST_VALS:
    gimp_get_data ("plug_in_applylens", &lvals);
    break;
    
  default:
    break;
  }

  gimp_tile_cache_ntiles(2 *(drawable->width / gimp_tile_width() + 1));
  gimp_progress_init("Applying lens...");
  drawlens(drawable);

  if(run_mode != RUN_NONINTERACTIVE)
    gimp_displays_flush();
  if(run_mode == RUN_INTERACTIVE)
    gimp_set_data("plug_in_applylens", &lvals, sizeof(LensValues));

  values[0].data.d_status = status;
  
  gimp_drawable_detach(drawable);
}

/*
  Ellipsoid formula: x^2/a^2 + y^2/b^2 + z^2/c^2 = 1
 */
static void
find_projected_pos(gfloat a, gfloat b,
		   gfloat x, gfloat y,
		   gfloat *projx, gfloat *projy)
{
  gfloat c;
  gfloat n[3];
  gfloat nxangle, nyangle, theta1, theta2;
  gfloat ri1 = 1.0, ri2 = lvals.refraction;

  /* PARAM */
  c = MIN(a, b);

  n[0] = x;
  n[1] = y;
  n[2] = sqrt((1-x*x/(a*a)-y*y/(b*b))*(c*c));

  nxangle = acos(n[0]/sqrt(n[0]*n[0]+n[2]*n[2]));
  theta1 = M_PI/2 - nxangle;
  theta2 = asin(sin(theta1)*ri1/ri2);
  theta2 = M_PI/2 - nxangle - theta2;
  *projx = x - tan(theta2)*n[2];

  nyangle = acos(n[1]/sqrt(n[1]*n[1]+n[2]*n[2]));
  theta1 = M_PI/2 - nyangle;
  theta2 = asin(sin(theta1)*ri1/ri2);
  theta2 = M_PI/2 - nyangle - theta2;
  *projy = y - tan(theta2)*n[2];
}

static void
drawlens(GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  gint row;
  gint x1, y1, x2, y2;
  guchar *src, *dest;
  gint i, col;
  gfloat regionwidth, regionheight, dx, dy, xsqr, ysqr;
  gfloat a, b, asqr, bsqr, midx, midy, x, y;
  glong pixelpos, pos;
  guchar bgr_red, bgr_blue, bgr_green, alphaval;
  GDrawableType drawtype = gimp_drawable_type(drawable->id);

  gimp_palette_get_background(&bgr_red, &bgr_green, &bgr_blue);

  gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);
  regionwidth = x2-x1;
  a = regionwidth/2;
  regionheight = y2-y1;
  b = regionheight/2;

  asqr = a*a;
  bsqr = b*b;

  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init(&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  src = g_malloc((x2-x1)*(y2-y1)*bytes);
  dest = g_malloc((x2-x1)*(y2-y1)*bytes);
  gimp_pixel_rgn_get_rect(&srcPR, src, x1, y1, regionwidth, regionheight);

  for(col = 0; col < regionwidth; col++) {
    dx = (gfloat)col - a + 0.5;
    xsqr = dx*dx;
    for(row = 0; row < regionheight; row++) {
      pixelpos = (col+row*regionwidth)*bytes;
      dy = -((gfloat)row - b) - 0.5;
      ysqr = dy*dy;
      if(ysqr < (bsqr - (bsqr*xsqr)/asqr)) {
	find_projected_pos(a, b, dx, dy, &x, &y);
	y = -y;
	pos = ((gint)(y+b)*regionwidth + (gint)(x+a)) * bytes;

	for(i = 0; i < bytes; i++) {
	  dest[pixelpos+i] = src[pos+i];
	}
      }
      else {
	if(lvals.keep_surr) {
	  for(i = 0; i < bytes; i++) {
	    dest[pixelpos+i] = src[pixelpos+i];
	  }
	}
	else {
	  if(lvals.set_transparent) alphaval = 0;
	  else alphaval = 255;

	  switch(drawtype) {
	  case INDEXEDA_IMAGE:
	    dest[pixelpos+1] = alphaval;
	  case INDEXED_IMAGE:
	    dest[pixelpos+0] = 0;
	    break;

	  case RGBA_IMAGE:
	    dest[pixelpos+3] = alphaval;
	  case RGB_IMAGE:
	    dest[pixelpos+0] = bgr_red;
	    dest[pixelpos+1] = bgr_green;
	    dest[pixelpos+2] = bgr_blue;
	    break;

	  case GRAYA_IMAGE:
	    dest[pixelpos+1] = alphaval;
	  case GRAY_IMAGE:
	    dest[pixelpos+0] = bgr_red;
	    break;
	  }
	}
      }
    }

      
    if(((gint)(regionwidth-col) % 5) == 0)
      gimp_progress_update((gdouble)col/(gdouble)regionwidth);
  }

  gimp_pixel_rgn_set_rect(&destPR, dest, x1, y1, regionwidth, regionheight);
  g_free(src);
  g_free(dest);

  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1,(x2 - x1),(y2 - y1));
}

static void
lens_close_callback(GtkWidget *widget,
		    gpointer   data)
{
  gtk_main_quit();
}

static void
lens_ok_callback(GtkWidget *widget,
		 gpointer   data)
{
  bint.run = TRUE;
  gtk_widget_destroy(GTK_WIDGET (data));
}

static void
lens_toggle_update(GtkWidget *widget,
		   gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *)data;

  if(GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
lens_entry_callback(GtkWidget *widget,
		    gpointer   data)
{
  lvals.refraction = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
  if(lvals.refraction < 1.0) lvals.refraction = 1.0;
}


static gint
lens_dialog(GDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  gchar buffer[12];
  gchar **argv;
  gint argc;
  GSList *group = NULL;
  GDrawableType drawtype;

  drawtype = gimp_drawable_type(drawable->id);

  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup("apply_lens");

  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());

  dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Lens effect");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)lens_close_callback,
		     NULL);

  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)lens_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  frame = gtk_frame_new("Parameter Settings");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  toggle = gtk_radio_button_new_with_label(group,
					   "Keep original surroundings");
  group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
                     (GtkSignalFunc) lens_toggle_update,
		     &lvals.keep_surr);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), lvals.keep_surr);
  gtk_widget_show(toggle);

  toggle =
    gtk_radio_button_new_with_label(group,
				    drawtype == INDEXEDA_IMAGE ||
				    drawtype == INDEXED_IMAGE ?
				    "Set surroundings to index 0" :
				    "Set surroundings to background color");
  group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
                     (GtkSignalFunc) lens_toggle_update,
		     &lvals.use_bkgr);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), lvals.use_bkgr);
  gtk_widget_show(toggle);

  if((drawtype == INDEXEDA_IMAGE) ||
     (drawtype == GRAYA_IMAGE) ||
     (drawtype == RGBA_IMAGE)) {
    toggle = gtk_radio_button_new_with_label(group,
					     "Make surroundings transparent");
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) lens_toggle_update,
		       &lvals.set_transparent);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle),
				lvals.set_transparent);
    gtk_widget_show(toggle);
  }


  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new("Lens refraction index: ");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%.2f", lvals.refraction);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)lens_entry_callback,
		     NULL);
  gtk_widget_show(entry);

  gtk_widget_show(hbox);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return bint.run;
}
