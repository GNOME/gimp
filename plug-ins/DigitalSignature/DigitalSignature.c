/**********************************************************************
 *  Digital Signature Plug-In (Version 1.00)
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
#include <string.h>
#include <math.h>

#include "libgimp/gimp.h"
#include "gtk/gtk.h"
#include "logo.h"

#define ENTRY_WIDTH 200

typedef struct {
    gint mode;
    gint licence;
    gint contents;
    char signature[50];
    char email[30];
    char homepage[50];
    char date[20];
    gint warningmessage;
} signatureValues;

typedef struct {
  gint run;
} signatureInterface;


/* Declare local functions.
 */
static void query(void);
static void run(char *name, int nparams,
		GParam *param,
		int *nreturn_vals,
		GParam **return_vals);
static void drawsignature(GDrawable *drawable);
static void readsignature(GDrawable *drawable);
static gint signature_dialog(void);
static gint signature_warning_dialog(void);
static gint message_dialog(char *, char *, char *);
GtkWidget * signature_logo_dialog(void);




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

static signatureValues wvals = {
        1,0,0,"","","","",1
}; /* wvals */

static signatureInterface bint =
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
    { PARAM_STRING, "signature[50]", "Signature (Name of author, company etc.)" },
    { PARAM_STRING, "e-mail[30]", "e-mail address" },
    { PARAM_STRING, "homepage[50]", "Address of the homepage" },
    { PARAM_STRING, "date[20]", "Date of creation" },
    { PARAM_INT8, "mode", "FALSE: Write signature; TRUE: Read signature" },
    { PARAM_INT8, "licence", "FALSE: Freely distributable; TRUE: Restricted distribution" },
    { PARAM_INT8, "contents", "TRUE: Adult only contents" },
  };

  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args)/ sizeof(args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure("plug_in_signature",
			 "Puts a hidden signature on your images.",
			 "",
			 "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
			 "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
			 "December, 1997",
			 "<Image>/Filters/Crypt/Digital Signature",
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
  gchar **argv;
  gint argc;
  gint ending=FALSE;

  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  
  *nreturn_vals = 1;
  *return_vals = values;
  
  drawable = gimp_drawable_get(param[2].data.d_drawable);

  switch(run_mode) {
  case RUN_INTERACTIVE:
	/* Possibly retrieve data */
	gimp_get_data("plug_in_signature", &wvals);
	/* Get information from the dialog */
        argc = 1;
        argv = g_new(gchar *, 1);
        argv[0] = g_strdup("apply_signature");
        gtk_init(&argc, &argv);
        gtk_rc_parse(gimp_gtkrc());
	
	ending=signature_dialog();
	
        if (ending==FALSE) return;
        if ((wvals.warningmessage) && (ending!=100))
           if (!signature_warning_dialog())
               return;
	       
	gtk_object_unref (GTK_OBJECT (tips));
        if (ending==100) {wvals.mode=1;} else {wvals.mode=0;}
	break;

  case RUN_NONINTERACTIVE:
	/* Make sure all the arguments are present */
	if (nparams != 10)
           status = STATUS_CALLING_ERROR;
	if (status == STATUS_SUCCESS)
           wvals.mode = param[7].data.d_int8;
	   strncpy (wvals.signature,param[3].data.d_string, 50);
	   strncpy (wvals.email,param[4].data.d_string, 30);
	   strncpy (wvals.homepage,param[5].data.d_string, 50);
	   strncpy (wvals.date,param[6].data.d_string, 20);
	   wvals.signature[49]='\0';
           wvals.email[29]='\0';
           wvals.homepage[49]='\0';
           wvals.date[19]='\0';

           wvals.licence = param[8].data.d_int8;
           wvals.contents = param[9].data.d_int8;
	   argc = 1;
           argv = g_new(gchar *, 1);
           argv[0] = g_strdup("apply_signature");
           gtk_init(&argc, &argv);
           gtk_rc_parse(gimp_gtkrc());
           tips = gtk_tooltips_new ();

	break;
  case RUN_WITH_LAST_VALS:
	/* Possibly retrieve data */
	   argc = 1;
           argv = g_new(gchar *, 1);
           argv[0] = g_strdup("apply_signature");
           gtk_init(&argc, &argv);
           gtk_rc_parse(gimp_gtkrc());
           tips = gtk_tooltips_new ();
           gimp_get_data("plug_in_signature", &wvals);
	
	break;
  default:
    break;
  }
  if (status == STATUS_SUCCESS) {
    if (!wvals.mode) {
        gimp_tile_cache_ntiles(2 *(drawable->width / gimp_tile_width() + 1));
        gimp_progress_init("Writing signature. Please wait...");
        drawsignature(drawable);
        if(run_mode != RUN_NONINTERACTIVE)
          gimp_displays_flush();
        if(run_mode == RUN_INTERACTIVE)
          gimp_set_data("plug_in_signature", &wvals, sizeof(signatureValues));
        values[0].data.d_status = status;
        gimp_drawable_detach(drawable);
    } else {
        readsignature(drawable);
        if(run_mode == RUN_INTERACTIVE)
          gimp_set_data("plug_in_signature", &wvals, sizeof(signatureValues));
        values[0].data.d_status = status;
        gimp_drawable_detach(drawable);
    }
  }    
}

static void
drawsignature(GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  gint row;
  gint x1, y1, x2, y2, ix, iy;
  guchar *src, *dest;
  gint i, col;
  gint value;
  long count=1,smallcount=0;
  gfloat regionwidth, regionheight, dx, dy;
  gfloat a, b, x, y;
  glong pixelpos, pos;
  guchar bgr_red, bgr_blue, bgr_green;
  char *point;
  char *point2;
  int finished_atleast_once=0;
  
  
  typedef struct {
    char header[3];
    char signature[50];
    char email[30];
    char homepage[50];
    char date[20];
    char flags[1];
    char reserved[45];
  } _wholedata;

  _wholedata wholedata;
  wvals.signature[49]='\0';
  wvals.email[29]='\0';
  wvals.homepage[49]='\0';
  wvals.date[19]='\0';
  point2=(char*) &wholedata;
  for (i=0;i<200; i++)
  {
    point2[i]='\0';
  }
  strncpy(wholedata.header,"SIG",3);
  strncpy(wholedata.signature,wvals.signature,50);  
  strncpy(wholedata.email,wvals.email,30);  
  strncpy(wholedata.homepage,wvals.homepage,50);  
  strncpy(wholedata.date,wvals.date,20);  
  if (wvals.contents) wholedata.flags[1]|=1;
  if (wvals.licence) wholedata.flags[1]|=2;
    
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
  
  point=(char *)&wholedata;  
  
  
  for(col = 0; col < regionwidth; col++) {
    dx = (gfloat)col - a;
    for(row = 0; row < regionheight; row++) {
          pixelpos = (col+row*regionwidth)*bytes;
          dy = -((gfloat)row - b);
          x = dx+a;
	  y = -dy+b;
	  ix=(int)x;
	  iy=(int)y;
	  smallcount++;
	  if (smallcount==9) {
	     count++;
	     point++;
	     smallcount=1;
	  }
	  if ((count % 200)==0)  { point=(char *)&wholedata; finished_atleast_once=1;}
          pos = ((gint)(iy)*regionwidth + (gint)(ix)) * bytes;
          for(i = 0; i < bytes; i++) {
	      value = src[pos+i];
	      if (point[0]&(1<<(smallcount-1))){
	         if (((int)((float)value/2.0))*2==value){
		    value+=1;
		    if (value==257) { value=255; }
                 }		    
	      } else {
	         if (!(((int)((float)value/2.0))*2==value)){
		    value+=1;
		    if (value==256) { value=254; }
                 }		    
	      } 
	      dest[pixelpos+i] = value; }    }
    if(((gint)(regionwidth-col) % 5) == 0)
    gimp_progress_update((gdouble)col/(gdouble)regionwidth);
  }
 
  gimp_pixel_rgn_set_rect(&destPR, dest, x1, y1, regionwidth, regionheight);
  g_free(src);
  g_free(dest);

  if (!finished_atleast_once) {
     message_dialog("Error", "Could not write signature", "The picture you wanted to process is to small\n"
                                                         "to contain the whole signature information.\n"
							 "Use an image with larger dimensions.");
  } else {
     message_dialog("Signature written successfully", NULL, "\nYour signature has been written\n"
                                                       "successfully! Start the Digital\n"
						       "Signature plug-in once again and\n"
						       "select 'read' to view the hidden\n"
						       "information included in the image.\n");
  }
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1,(x2 - x1),(y2 - y1));
}

static void
readsignature(GDrawable *drawable)
{
  GPixelRgn srcPR;
  gint width, height;
  gint bytes;
  gint row, value;
  gint x1, y1, x2, y2, ix, iy, i;
  guchar *src;
  gint col, finished_atleast_once=FALSE;
  gfloat regionwidth, regionheight, dx, dy;
  gfloat a, b, x, y;
  glong pixelpos, pos;
  guchar bgr_red, bgr_blue, bgr_green;
  long count=1,smallcount=0;
  char  displaystring[500];
  char * point;
  char *point2;
  typedef struct {
    char header[3];
    char signature[50];
    char email[30];
    char homepage[50];
    char date[20];
    char flags[1];
    char reserved[45];
  } _wholedata;

  _wholedata wholedata;
  point2=(char*) &wholedata;
  for (i=0;i<200; i++)
  {
    point2[i]='\0';
  }

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

  src = g_malloc((x2-x1)*(y2-y1)*bytes);
  gimp_pixel_rgn_get_rect(&srcPR, src, x1, y1, regionwidth, regionheight);

  point=(char *)&wholedata;  
    
  for(col = 0; col < regionwidth; col++) {
    dx = (gfloat)col - a;
    for(row = 0; row < regionheight; row++) {
          pixelpos = (col+row*regionwidth)*bytes;
          dy = -((gfloat)row - b);
          x = dx+a;
	  y = -dy+b;
	  ix=(int)x;
	  iy=(int)y;
          pos = ((gint)(iy)*regionwidth + (gint)(ix)) * bytes;
	
	  smallcount++;
	  if (smallcount==9) {
	     count++;
	     point++;
	     smallcount=1;
	  }
	  if ((count % 200)==0) {finished_atleast_once=1; goto fin;}
  
	  
	      value = src[pos];
              if (!(((int)((float)value/2.0))*2==value)){
      	          point[0]|=(1<<(smallcount-1));
             } 
    }
  }
fin:
  g_free(src);

  if (!finished_atleast_once) {
     message_dialog("Error", "Could not read signature", "The picture you wanted to process is to small\n"
                                                         "to contain the whole signature information.\n"
							 "Use an image with larger dimensions.");
     return;
  }
  if (strncmp("SIG",wholedata.header,3)) {
     message_dialog("Error", "No signature", "The picture you wanted to process contains\n"
                                             "no signature or the signature has been destroyed\n"
					     "with some image manipulation.");
     return;
  }
  sprintf(displaystring, "----------------------------------\nImage creator:\n%s\n\n"
                         "E-mail address:\n%s\n\n"
			 "Internet homepage:\n%s\n\n"
			 "Date of creation:\n%s\n\n"
			 "Freely distributable:\n%s\n\n"
			 "Adult contents:\n%s\n----------------------------------"
		       , wholedata.signature
		       , wholedata.email
		       , wholedata.homepage
		       , wholedata.date
		       , (wholedata.flags[1] & 2) ? "Yes" : "No"
		       , (wholedata.flags[1] & 1) ? "Yes" : "No"
		       );   
  
 message_dialog("Signature read successfully", "Image information", displaystring );
}

void
signature_close_callback(GtkWidget *widget,  gpointer   data)
{ 
  gtk_widget_destroy(maindlg);
  gtk_main_quit();
}


void
signature_logo_close_callback(GtkWidget *widget,  gpointer   data)
{ 
  gtk_main_quit();
}

void
signature_ok_callback(GtkWidget *widget, gpointer   data)
{
  bint.run = TRUE;
  gtk_widget_destroy(GTK_WIDGET(data));
}

void
signature_ok2_callback(GtkWidget *widget, gpointer   data)
{
  bint.run = 100;
  gtk_widget_destroy(GTK_WIDGET(data));
}

void
signature_logo_ok_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, TRUE); 
  gtk_widget_destroy(logodlg);
}

void
signature_about_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, FALSE); 
  signature_logo_dialog();
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
signature_entry1_callback (GtkWidget * widget, gpointer data)
{
  strncpy (wvals.signature, gtk_entry_get_text (GTK_ENTRY (widget)), 50);
}

static void
signature_entry2_callback (GtkWidget * widget, gpointer data)
{
  strncpy (wvals.email, gtk_entry_get_text (GTK_ENTRY (widget)), 30);
}
static void
signature_entry3_callback (GtkWidget * widget, gpointer data)
{
  strncpy (wvals.homepage, gtk_entry_get_text (GTK_ENTRY (widget)), 50);
}
static void
signature_entry4_callback (GtkWidget * widget, gpointer data)
{
  strncpy (wvals.date, gtk_entry_get_text (GTK_ENTRY (widget)), 20);
}

static void
set_tooltip (GtkTooltips *tooltips, GtkWidget *widget, const char *desc)
{
  if (desc && desc[0])
    gtk_tooltips_set_tips (tooltips, widget, (char *) desc);
}

static gint
signature_dialog()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  gchar buffer[100];
  
  
  bint.run=FALSE;
  
  dlg = maindlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Digital Signature <cotting@mygale.org>");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)signature_close_callback,
		     NULL);
		     
		     
		     
  frame = gtk_frame_new("Settings");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  tips = gtk_tooltips_new ();
  	/* use black as foreground: */
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
  
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  
  label = gtk_label_new("Signature: ");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%s", wvals.signature);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)signature_entry1_callback,
		     NULL); 
  gtk_widget_show(entry);
  set_tooltip(tips,entry,"Here you can specify your signature (name of author, company etc.)");

  gtk_widget_show(hbox);
  
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new("E-mail: ");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%s", wvals.email);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
 		     (GtkSignalFunc)signature_entry2_callback,
		     NULL); 
  gtk_widget_show(entry);
  set_tooltip(tips,entry,"Here you can specify an e-mail address, where the author/company can be contacted.");

  gtk_widget_show(hbox);
  
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new("Internet address: ");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%s", wvals.homepage);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
 		     (GtkSignalFunc)signature_entry3_callback,
		     NULL); 
  gtk_widget_show(entry);
  set_tooltip(tips,entry,"Here you can specify an internet address, where the homepage of the author/company can be found.");

  gtk_widget_show(hbox);
  
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

  label = gtk_label_new("Date of creation: ");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%s", wvals.date);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
 		     (GtkSignalFunc)signature_entry4_callback,
		     NULL); 
  gtk_widget_show(entry);
  set_tooltip(tips,entry,"Here you can specify the date, when the image was created.");

  gtk_widget_show(hbox);

  toggle = gtk_check_button_new_with_label ("Restricted image distribution");
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &wvals.licence);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), wvals.licence);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If this option is enabled, a flag will be set, that the image is not freely distributable.");

  toggle = gtk_check_button_new_with_label ("Adult contents");
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &wvals.contents);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), wvals.contents);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If this option is enabled, a flag will be set, that the contents of the image should only be accessible to adults.");

  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  button = gtk_button_new_with_label("Write");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)signature_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  set_tooltip(tips,button,"Close the dialog box and write signature.");
  
  button = gtk_button_new_with_label("Read");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)signature_ok2_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  set_tooltip(tips,button,"Close the dialog box and read signature.");

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Close the dialog box without reading or writing a signature.");

  button = gtk_button_new_with_label("About...");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)signature_about_callback,button);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Show information about the author and the plug-in.");

  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush(); 

  return bint.run;
}

static gint
message_dialog(char * title, char *title2, char * text)
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  bint.run=FALSE;

  dlg = gtk_dialog_new();
    	/* use black as foreground: */
        tips_fg.red   = 0;
        tips_fg.green = 0;
        tips_fg.blue  = 0;
       /* postit yellow (khaki) as background: */
        gdk_color_alloc (gtk_widget_get_colormap (dlg), &tips_fg);
        tips_bg.red   = 61669;
        tips_bg.green = 59113;
        tips_bg.blue  = 35979;
        gdk_color_alloc (gtk_widget_get_colormap (dlg), &tips_bg);
        gtk_tooltips_set_colors (tips,&tips_bg,&tips_fg);

  gtk_window_set_title(GTK_WINDOW(dlg), title);
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)signature_close_callback,
		     NULL);


  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Close this message box and continue");

  frame = gtk_frame_new(title2);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);


  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new(text);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  gtk_widget_show(hbox);

  gtk_widget_show(vbox);
  gtk_widget_show(frame);
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();
  return 0;
}



static gint
signature_warning_dialog()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  bint.run=FALSE;

  dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Warning");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)signature_close_callback,
		     NULL);

  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)signature_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Proceed with writing the signature.");

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Cancel the writing of signature.");

  frame = gtk_frame_new("Please note:");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);


  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new("Sensitive Signature Warning:\n\n"
                        "Remember to save your image in a non-destructive format!\n"
			"For indexed images GIF could be a good choice, for RGB\n"
			"use BMP/TIFF/TGA etc. Never use a JPEG-compression!\n\n"
			"You shouldn't apply any filter to the image after the\n"
			"signature has been written. No blur/constrast etc.\n\n"
			"This restriction will hopefully be removed in a future\n"
			"version of the plug-in (available at cotting.citeweb.net)\n");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  toggle = gtk_check_button_new_with_label ("Show warning message every time");
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &wvals.warningmessage);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), wvals.warningmessage);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If you want this message to be displayed every time you write a signature, then check this box.");

  gtk_widget_show(hbox);

  gtk_widget_show(vbox);
  gtk_widget_show(frame);
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return bint.run;
}

GtkWidget * 
signature_logo_dialog()
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
                     (GtkSignalFunc)signature_logo_close_callback,
		     NULL);

  xbutton = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(xbutton, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(xbutton), "clicked",
                     (GtkSignalFunc)signature_logo_ok_callback,
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
          "Digital Signature\n Plug-In for the GIMP\n"
          "Version 1.00\n";
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
