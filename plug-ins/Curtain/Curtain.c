/**********************************************************************
 *  Curtain Plug-In (Version 1.03)
 *  Daniel Cotting (cotting@mygale.org)
 **********************************************************************
 *  Official homepages: http://www.mygale.org/~cotting
 *                      http://cotting.citeweb.net
 *                      http://village.cyberbrain.com/cotting
 **********************************************************************    
 */

/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>
#include <math.h>

#include "libgimp/gimp.h"
#include "gtk/gtk.h"
#include "logo.h"

#define ENTRY_WIDTH 100

typedef struct {
    gint horiz;
    gint vert;
} CurtainValues;

typedef struct {
  gint run;
} CurtainInterface;



/* Declare local functions.
 */
static void query(void);
static void run(char *name, int nparams,
		GParam *param,
		int *nreturn_vals,
		GParam **return_vals);
static void drawCurtain(GDrawable *drawable);
static gint curtain_dialog(void);

GtkWidget * curtain_logo_dialog(void);

GtkWidget *maindlg;
GtkWidget *logodlg;
GtkTooltips *tips;
GdkColor tips_fg,tips_bg;	



GPlugInInfo PLUG_IN_INFO =
{
  NULL, /* init_proc */
  NULL, /* quit_proc */
  query, /* query_proc */
  run, /* run_proc */
};

static CurtainValues wvals = {
        TRUE, FALSE
}; /* wvals */

static CurtainInterface bint =
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
    { PARAM_INT32, "horizontal", "horizontal curtain flag (TRUE/FALSE)" },
    { PARAM_INT32, "vertical", "vertical curtain flag (TRUE/FALSE)" },
  };

  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args)/ sizeof(args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure("plug_in_curtain",
			 "Apply a curtain effect",
			 "",
			 "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
			 "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
			 "October, 1997",
			 "<Image>/Filters/Image/Curtain",
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
  	/* Possibly retrieve data */
	gimp_get_data("plug_in_curtain", &wvals);
	/* Get information from the dialog */
        if (!curtain_dialog())
               return;
        break;
  case RUN_NONINTERACTIVE:
        if (nparams != 5)
           status = STATUS_CALLING_ERROR;
	if (status == STATUS_SUCCESS) {
           wvals.horiz = param[3].data.d_int32;
           wvals.vert = param[4].data.d_int32; }
	break;
  case RUN_WITH_LAST_VALS:
    /* Possibly retrieve data */
    gimp_get_data("plug_in_curtain", &wvals);
    break;
  default:
    break;
  }

  gimp_tile_cache_ntiles(2 *(drawable->width / gimp_tile_width() + 1));
  gimp_progress_init("Applying curtain. Please wait...");
  drawCurtain(drawable);

  if(run_mode != RUN_NONINTERACTIVE)
    gimp_displays_flush();
  if(run_mode == RUN_INTERACTIVE)
    gimp_set_data("plug_in_curtain", &wvals, sizeof(CurtainValues));

  values[0].data.d_status = status;
  
  gimp_drawable_detach(drawable);
}

static void
set_tooltip (GtkTooltips *tooltips, GtkWidget *widget, const char *desc)
{
  if (desc && desc[0])
    gtk_tooltips_set_tips (tooltips, widget, (char *) desc);
}

static void
drawCurtain(GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  gint row;
  gint x1, y1, x2, y2, ix, iy;
  guchar *src, *dest;
  gint i, col, position;
  gfloat regionwidth, regionheight, dx, dy;
  gfloat a, b, x, y;
  glong pixelpos, pos;
  guchar bgr_red, bgr_blue, bgr_green;

  gimp_palette_get_background(&bgr_red, &bgr_green, &bgr_blue);

  gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);
  regionwidth = x2-x1;
  a = regionwidth/2;
  regionheight = y2-y1;
  b = regionheight/2;

  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init(&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  src = g_malloc((x2-x1)*(y2-y1)*bytes);
  dest = g_malloc((x2-x1)*(y2-y1)*bytes);
  gimp_pixel_rgn_get_rect(&srcPR, src, x1, y1, regionwidth, regionheight);

  for(col = 0; col < regionwidth; col++) {
    dx = (gfloat)col - a;
    for(row = 0; row < regionheight; row++) {
          pixelpos = (col+row*regionwidth)*bytes;
          dy = -((gfloat)row - b);
          x = dx+a;
	  y = -dy+b;
	  ix=(int)x;
	  iy=(int)y;
	  
	  /* 
	     Flags horiz and vert inversed by mistake!
	     Corrected this with a small hack in the next lines.
	     (one more inversion)
	  */	     
	  
          if ((((int)((float)ix/2.0))*2==ix) && (wvals.vert)) {
             pos = ((gint)(regionheight-1-iy)*regionwidth) * bytes;
            }else{  
             pos = ((gint)(iy)*regionwidth) * bytes;
	  } 
          for(i = 0; i < bytes; i++) {
	    if ((((int)((float)iy/2.0))*2==iy) && (wvals.horiz)) {
	       position=pos+regionwidth*bytes-ix*bytes+i;}else{position=pos+ix*bytes+i;}
            dest[pixelpos+i] = src[position];
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
curtain_close_callback(GtkWidget *widget,  gpointer   data)
{ 
  gtk_main_quit();
}


static void
curtain_ok_callback(GtkWidget *widget, gpointer   data)
{
  bint.run = TRUE;
  gtk_widget_destroy(GTK_WIDGET (data));

}

static void
toggle_update (GtkWidget *widget,
		       gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
    
    
}

static void
curtain_logo_ok_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, TRUE);
  gtk_widget_destroy(logodlg);
}


static void
curtain_about_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, FALSE);
  curtain_logo_dialog();
}


static gint
curtain_dialog()
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *toggle,*toggle2;
  GtkWidget *frame;
  GtkWidget *vbox;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup("apply_curtain_filter");

  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());
  bint.run=FALSE;
  dlg = maindlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Curtain");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)curtain_close_callback,
		     NULL);
  frame = gtk_frame_new("Parameter Settings");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  	/* use black as foreground: */
        tips = gtk_tooltips_new ();
        tips_fg.red   = 0;
        tips_fg.green = 0;
        tips_fg.blue  = 0;
       /* postit yellow (khaki) as background: */
        gdk_color_alloc (gtk_widget_get_colormap (frame), &tips_fg);
        tips_bg.red   = 61669;
        tips_bg.green = 59113;
        tips_bg.blue  = 35979;
        gdk_color_alloc (gtk_widget_get_colormap (frame), &tips_bg);
        gtk_tooltips_set_colors (tips,&tips_bg,&tips_fg);


  toggle = gtk_check_button_new_with_label ("Apply horizontal curtain");
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &wvals.horiz);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), wvals.horiz);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If this option is enabled, a horizontal curtain effect will be applied.");

  toggle2 = gtk_check_button_new_with_label ("Apply vertical curtain");
  gtk_box_pack_start(GTK_BOX(vbox), toggle2, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle2), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &wvals.vert);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle2), wvals.vert);
  gtk_widget_show (toggle2);
  set_tooltip(tips,toggle2,"If this option is enabled, a vertical curtain effect will be applied.");

  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)curtain_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Click here to close this dialog box and apply the curtain effects.");

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Click here to close this dialog box without altering the image.");


    	button = gtk_button_new_with_label("About...");
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)curtain_about_callback,dlg);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
        gtk_widget_show(button);
  set_tooltip(tips,button,"Click here for information about the author and the plug-in.");
  
  
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return bint.run;
}



GtkWidget * 
curtain_logo_dialog()
{
  GtkWidget *xdlg;
  GtkWidget *xlabel;
  GtkWidget *xbutton;
  GtkWidget *xlogo_box;
  GtkWidget *xpreview;
  GtkWidget *xframe,*xframe2;
  GtkWidget *xvbox;
  GtkWidget *xhbox;
  char *text;
  guchar *temp,*temp2;
  guchar *datapointer;
  gint y,x;
  xdlg = logodlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(xdlg), "About");
  gtk_window_position(GTK_WINDOW(xdlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(xdlg), "destroy",
                     (GtkSignalFunc)curtain_close_callback,
		     NULL);

  xbutton = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(xbutton, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(xbutton), "clicked",
                     (GtkSignalFunc)curtain_logo_ok_callback,
		     xdlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(xdlg)->action_area),
		     xbutton, TRUE, TRUE, 0);
  gtk_widget_grab_default(xbutton);
  gtk_widget_show(xbutton);
  set_tooltip(tips,xbutton,"Click here to close the information box.");

  xframe = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(xframe), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(xframe), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(xdlg)->vbox), xframe, TRUE, TRUE, 0);
  xvbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(xvbox), 10);
  gtk_container_add(GTK_CONTAINER(xframe), xvbox);

  /*  The logo frame & drawing area  */
  xhbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (xvbox), xhbox, FALSE, TRUE, 0);

  xlogo_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (xhbox), xlogo_box, FALSE, FALSE, 0);

  xframe2 = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (xframe2), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (xlogo_box), xframe2, FALSE, FALSE, 0);

  xpreview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (xpreview), logo_width, logo_height);
  temp = g_malloc((logo_width+10)*3);
  datapointer=header_data+logo_height*logo_width-1;
  for (y = 0; y < logo_height; y++){
    temp2=temp;
    for (x = 0; x< logo_width; x++) {
      HEADER_PIXEL(datapointer,temp2); temp2+=3;}
    gtk_preview_draw_row (GTK_PREVIEW (xpreview),
			  temp,
			  0, y, logo_width); 
  }			  
  g_free(temp);
  gtk_container_add (GTK_CONTAINER (xframe2), xpreview);
  gtk_widget_show (xpreview);
  gtk_widget_show (xframe2);
  gtk_widget_show (xlogo_box);
  gtk_widget_show (xhbox);

  xhbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(xvbox), xhbox, TRUE, TRUE, 0);
  text = "\nCotting Software Productions\n"
         "Bahnhofstrasse 31\n"
  	 "CH-3066 Stettlen (Switzerland)\n\n"
	 "cotting@mygale.org\n"
	 "http://www.mygale.org/~cotting\n\n"
          "Curtain Plug-In for the GIMP\n"
          "Version 1.03\n";
  xlabel = gtk_label_new(text);
  gtk_box_pack_start(GTK_BOX(xhbox), xlabel, TRUE, FALSE, 0);
  gtk_widget_show(xlabel);

  gtk_widget_show(xhbox);

  gtk_widget_show(xvbox);
  gtk_widget_show(xframe);
  gtk_widget_show(xdlg);

  gtk_main();
  gdk_flush();
  return xdlg;
}
