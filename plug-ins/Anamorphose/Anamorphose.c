/**********************************************************************
 *  Conical Anamorphose Distortion Plug-In (Version 1.03)
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

/* Declare local functions.
 */
static void query(void);
static void run(char *name, int nparams,
		GParam *param,
		int *nreturn_vals,
		GParam **return_vals);

static void drawanamorphose(GDrawable *drawable);
static gint anamorphose_dialog(GDrawable *drawable);

GPlugInInfo PLUG_IN_INFO =
{
  NULL, /* init_proc */
  NULL, /* quit_proc */
  query, /* query_proc */
  run, /* run_proc */
};

typedef struct
{
  gdouble cone_radius;
  gdouble base_angle;
  gint keep_surr, use_bkgr, set_transparent, use_antialias,flip,dedouble;
} anamorphoseValues;

static anamorphoseValues lvals =
{
  /* anamorphose cone radius value */
  100,60,
  /* Surroundings options */
  TRUE, FALSE, FALSE, TRUE, TRUE, FALSE
};

typedef struct
{
  gint run;
} anamorphoseInterface;

static anamorphoseInterface bint =
{
  FALSE  /*  run  */
};

GtkWidget * anamorphose_logo_dialog(void);


GtkWidget *maindlg;
GtkWidget *logodlg;
GtkTooltips *tips;
GdkColor tips_fg,tips_bg;	



MAIN()

static void
query(void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_FLOAT, "cone_radius", "Radius of the reflecting cone in the center" },
    { PARAM_FLOAT, "base_angle", "Base angle of the reflecting cone (in degrees)" },
    { PARAM_INT32, "keep_surroundings", "Keep anamorphose surroundings" },
    { PARAM_INT32, "set_background", "Set anamorphose surroundings to bkgr value" },
    { PARAM_INT32, "set_transparent", "Set anamorphose surroundings transparent (Only on Image w/ Alpha-Layer)" },
    { PARAM_INT32, "use_antialias", "Use antialias for better and smoother results" },
    { PARAM_INT32, "flip", "Flip the image vertically" },
    { PARAM_INT32, "double", "Use double reflection algorithm" },
  };

  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args)/ sizeof(args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure("plug_in_anamorphose",
			 "Apply an anamorphose effect",
			 "",
			 "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
			 "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
			 "October, 1997",
			 "<Image>/Filters/Distorts/Conical Anamorphose",
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
    gimp_get_data("plug_in_anamorphose", &lvals);
    if(!anamorphose_dialog(drawable)) return;
    break;

  case RUN_NONINTERACTIVE:
    if(nparams != 11) status = STATUS_CALLING_ERROR;
    if(status == STATUS_SUCCESS) {
      lvals.cone_radius = param[3].data.d_float;
      lvals.base_angle = param[4].data.d_float;
      lvals.keep_surr = param[5].data.d_int32;
      lvals.use_bkgr = param[6].data.d_int32;
      lvals.set_transparent = param[7].data.d_int32;
      lvals.use_antialias = param[8].data.d_int32;
      lvals.flip = param[9].data.d_int32;
      lvals.dedouble = param[10].data.d_int32;
    }

    if(status == STATUS_SUCCESS && (lvals.cone_radius <= 0))
      status = STATUS_CALLING_ERROR;
    break;

  case RUN_WITH_LAST_VALS:
    gimp_get_data ("plug_in_anamorphose", &lvals);
    break;
    
  default:
    break;
  }

  gimp_tile_cache_ntiles(2 *(drawable->width / gimp_tile_width() + 1));
  gimp_progress_init("Applying anamorphose. Please wait...");
  drawanamorphose(drawable);

  if(run_mode != RUN_NONINTERACTIVE)
    gimp_displays_flush();
  if(run_mode == RUN_INTERACTIVE)
    gimp_set_data("plug_in_anamorphose", &lvals, sizeof(anamorphoseValues));

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
drawanamorphose(GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  gdouble abstand;
  gdouble radius;
  gdouble angle, alpha, gamma, abstandvonzentrum, distanzzukreis, pi ,seiteb, verhaltniss, a1,a2,a3,a4,faktor,a_tot;
  gint row;
  gint x1, y1, x2, y2, ix, iy;
  guchar *src, *dest;
  gint i, col, succeeded;
  gfloat regionwidth, regionheight, dx, dy, xsqr, ysqr;
  gfloat a, b, asqr, bsqr, x, y;
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

  radius=lvals.cone_radius;
  angle=lvals.base_angle;
  
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init(&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  src = g_malloc((x2-x1)*(y2-y1)*bytes);
  dest = g_malloc((x2-x1)*(y2-y1)*bytes);
  gimp_pixel_rgn_get_rect(&srcPR, src, x1, y1, regionwidth, regionheight);

  for(col = 0; col < regionwidth; col++) {
    dx = (gfloat)col - a+0.5;
    xsqr = dx*dx;
    for(row = 0; row < regionheight; row++) {
      pixelpos = (col+row*regionwidth)*bytes;
      dy = -((gfloat)row - b-0.5);
      ysqr = dy*dy;
      abstand=(sqrt(dx*dx+dy*dy));
      succeeded=1;
      if (abstand>radius){ 
	alpha=2.0*angle-90.0;
	gamma=90.0-angle;
	distanzzukreis=abstand-radius;
	pi=atan(1.0)*4.0;
	seiteb=sin(alpha/180.0*pi)/sin(gamma/180.0*pi)*distanzzukreis;
        abstandvonzentrum=radius-(sin(gamma/180.0*pi)*seiteb);
	if ((abstandvonzentrum<0) && (lvals.dedouble==0)) { 
	  succeeded=0;
	}else{  
	  verhaltniss=abstandvonzentrum/abstand;
          if (lvals.flip) dy = -dy;
          x = dx*verhaltniss+a;
	  y = dy*verhaltniss+b;
	  ix=(int)x;
	  iy=(int)y;
	  if (lvals.use_antialias) {
	     if ((ix - x)<0){
               if ((iy - y)<0){} else {iy-=1;}
	     } else{
               if ((iy - y)<0){ix-=1;} else {iy-=1;ix-=1;}
	     }
             a1=1/((ix-x)*(ix-x)+(iy-y)*(iy-y));   
             a2=1/((ix+1-x)*(ix+1-x)+(iy-y)*(iy-y));   
             a3=1/((ix-x)*(ix-x)+(iy+1-y)*(iy+1-y));   
             a4=1/((ix+1-x)*(ix+1-x)+(iy+1-y)*(iy+1-y));
             a_tot=a1+a2+a3+a4;
             faktor=1/a_tot;
             a1*=faktor;
             a2*=faktor;
             a3*=faktor;
             a4*=faktor;
	
             pos = ((gint)(iy)*regionwidth + (gint)(ix)) * bytes;

	     for (i = 0; i < bytes; i++) {
	       dest[pixelpos+i] = (gint)((gdouble)(src[pos+i])*a1+(gdouble)(src[pos+i+bytes])*a2+(gdouble)(src[pos+i+(gint)(regionwidth*bytes)])*a3+(gdouble)(src[pos+i+(gint)(regionwidth*bytes)+bytes])*a4);
	     }
	   } else {
	     pos = ((gint)(iy)*regionwidth + (gint)(ix)) * bytes;
	     for(i = 0; i < bytes; i++) {
	        dest[pixelpos+i] = src[pos+i];
	     }
	  }
	}
	} else {
	  succeeded=0;
	}
	if (succeeded==0){
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
anamorphose_close_callback(GtkWidget *widget,  gpointer   data)
{
  gtk_main_quit();
}

static void
anamorphose_ok_callback(GtkWidget *widget, gpointer   data)
{
  bint.run = TRUE;
  gtk_widget_destroy(GTK_WIDGET (data));
}

static void
anamorphose_toggle_update(GtkWidget *widget, gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *)data;

  if(GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
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
anamorphose_entry_callback(GtkWidget *widget, gpointer   data)
{
  lvals.cone_radius = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
  if(lvals.cone_radius <= 0) lvals.cone_radius = 10;
}

static void
anamorphose_entry_callback2(GtkWidget *widget, gpointer   data)
{
  lvals.base_angle= atof(gtk_entry_get_text(GTK_ENTRY(widget)));
  if(lvals.base_angle <= 0) lvals.base_angle = 60;
  if(lvals.base_angle >= 90) lvals.base_angle = 60;
}


static void
anamorphose_logo_ok_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, TRUE);
  gtk_widget_destroy(logodlg);
}


static void
anamorphose_about_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, FALSE);
  anamorphose_logo_dialog();
}


static gint
anamorphose_dialog(GDrawable *drawable)
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
  argv[0] = g_strdup("apply_anamorphose");

  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());

  dlg = maindlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Apply anamorphose effect");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)anamorphose_close_callback,
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

  
  toggle = gtk_radio_button_new_with_label(group,
					   "Keep original surroundings");
  group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
                     (GtkSignalFunc) anamorphose_toggle_update,
		     &lvals.keep_surr);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), lvals.keep_surr);
  gtk_widget_show(toggle);
  set_tooltip(tips,toggle,"In the center, where the cone is placed, the plug-in can't render a reflected image. If this option is enabled, this area will be filled with the original image.");

  toggle =
    gtk_radio_button_new_with_label(group,
				    drawtype == INDEXEDA_IMAGE ||
				    drawtype == INDEXED_IMAGE ?
				    "Set surroundings to index 0" :
				    "Set surroundings to background color");
  group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
                     (GtkSignalFunc) anamorphose_toggle_update,
		     &lvals.use_bkgr);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), lvals.use_bkgr);
  gtk_widget_show(toggle);
  set_tooltip(tips,toggle,"In the center, where the cone is placed, the plug-in can't render a reflected image. If this option is enabled, this area will be filled with the active background color or the index-0-color, depending on the image type.");

  if((drawtype == INDEXEDA_IMAGE) ||
     (drawtype == GRAYA_IMAGE) ||
     (drawtype == RGBA_IMAGE)) {
    toggle = gtk_radio_button_new_with_label(group,
					     "Make surroundings transparent");
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) anamorphose_toggle_update,
		       &lvals.set_transparent);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle),
				lvals.set_transparent);
    gtk_widget_show(toggle);
    set_tooltip(tips,toggle,"In the center, where the cone is placed, the plug-in can't render a reflected image. If this option is enabled, this area will be kept transparent.");

  }

  if((drawtype != INDEXEDA_IMAGE) && (drawtype != INDEXED_IMAGE)){
    toggle = gtk_check_button_new_with_label ("Use antialias (recommended)");
    gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
  		      (GtkSignalFunc) toggle_update,
  		      &lvals.use_antialias);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), lvals.use_antialias);
    gtk_widget_show (toggle);
    set_tooltip(tips,toggle,"If this option is enabled, an antialias algorithm will be used to produce a better and smoother output.");

  } else { lvals.use_antialias=FALSE;}
  toggle = gtk_check_button_new_with_label ("Flip image vertically");
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &lvals.flip);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), lvals.flip);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"With this option you can flip the image vertically.");

  toggle = gtk_check_button_new_with_label ("Double reflection");
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &lvals.dedouble);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), lvals.dedouble);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If this option is enabled, the plug-in will use the double reflection functionality. The produced images are only mathematically possible. A cone won't reflect all pixels to their original location.");


  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new("Radius of cone: ");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%.2f", lvals.cone_radius);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)anamorphose_entry_callback,
		     NULL);
  gtk_widget_show(entry);
  gtk_widget_show(hbox);
  set_tooltip(tips,entry,"Specify the base radius of the reflecting cone, here.");

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  label = gtk_label_new("Base angle of cone (in degrees): ");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%.2f", lvals.base_angle);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)anamorphose_entry_callback2,
		     NULL);
  gtk_widget_show(entry);
  set_tooltip(tips,entry,"Specify the base angle of the reflecting cone, here. The angle must have a value between 0 and 90 degrees.");

  gtk_widget_show(hbox);
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)anamorphose_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Close the dialog box and apply distortion.");

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Close the dialog box without applying distortion.");

  	button = gtk_button_new_with_label("About...");
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)anamorphose_about_callback,button);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
        gtk_widget_show(button);
  set_tooltip(tips,button,"Show information about the author and the plug-in.");

  
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return bint.run;
}

GtkWidget * 
anamorphose_logo_dialog()
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
                     (GtkSignalFunc)anamorphose_close_callback,
		     NULL);

  xbutton = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(xbutton, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(xbutton), "clicked",
                     (GtkSignalFunc)anamorphose_logo_ok_callback,
		     xdlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(xdlg)->action_area),
		     xbutton, TRUE, TRUE, 0);
  gtk_widget_grab_default(xbutton);
  gtk_widget_show(xbutton);
  set_tooltip(tips,xbutton,"Close the information box.");

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
  datapointer=header_data+logo_width*logo_height-1;
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
          "Conical Anamorphose\nPlug-In for the GIMP\n"
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
