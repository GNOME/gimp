/**********************************************************************
 *  Stegano Plug-In (Version 1.00)
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
#include <sys/stat.h>

#include "libgimp/gimp.h"
#include "gtk/gtk.h"
#include "logo.h"

#define ENTRY_WIDTH 200

typedef struct {
    gint mode;
    char filename[128];
    gint warningmessage;
} steganoValues;

typedef struct {
  gint run;
} steganoInterface;


/* Declare local functions.
 */
static void query(void);
static void run(char *name, int nparams,
		GParam *param,
		int *nreturn_vals,
		GParam **return_vals);
static void drawstegano(GDrawable *drawable);
static void readstegano(GDrawable *drawable);
static gint stegano_dialog(void);
static gint stegano_save_dialog(void);
static gint stegano_warning_dialog(void);
static gint message_dialog(char *, char *, char *);
GtkWidget * stegano_logo_dialog(void);




GtkWidget *maindlg;
GtkWidget *globalentry;
GtkWidget *saveglobalentry;
GtkWidget *logodlg;
GtkWidget *warningdlg;
GtkWidget *messagedlg;
GtkWidget *savedlg;

GtkTooltips *tips;
GdkColor tips_fg,tips_bg;	
gchar filename[128];
gint saverun;
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;



GPlugInInfo PLUG_IN_INFO =
{
  NULL, /* init_proc */
  NULL, /* quit_proc */
  query, /* query_proc */
  run, /* run_proc */
};

static steganoValues wvals = {
        1,"",1
}; /* wvals */

static steganoInterface bint =
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
    { PARAM_INT8, "mode", "FALSE: Hide file in image; TRUE: Get hidden file" },
    { PARAM_STRING, "filename[128]", "Name of the file to hide in an image (only of importance if mode=FALSE" },
  };

  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args)/ sizeof(args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure("plug_in_stegano",
			 "Hides a file in an image.",
			 "",
			 "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
			 "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
			 "December, 1997",
			 "<Image>/Filters/Image/Stegano",
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
  gchar **argv;
  gint argc;

  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  
  *nreturn_vals = 1;
  *return_vals = values;
  
  drawable = gimp_drawable_get(param[2].data.d_drawable);
  

  switch(run_mode) {
  case RUN_INTERACTIVE:
	/* Possibly retrieve data */
	gimp_get_data("plug_in_stegano", &wvals);
	/* Get information from the dialog */
        argc = 1;
        argv = g_new(gchar *, 1);
        argv[0] = g_strdup("apply_stegano");
        gtk_init(&argc, &argv);
        gtk_rc_parse(gimp_gtkrc());
	
	stegano_dialog();
	return;

  case RUN_NONINTERACTIVE:
	/* Make sure all the arguments are present */
	if (nparams != 5)
           status = STATUS_CALLING_ERROR;
	if (status == STATUS_SUCCESS)
           wvals.mode = param[3].data.d_int8;
	   strncpy (wvals.filename,param[4].data.d_string, 128);
	   wvals.filename[127]='\0';
	   argc = 1;
           argv = g_new(gchar *, 1);
           argv[0] = g_strdup("apply_stegano");
           gtk_init(&argc, &argv);
           gtk_rc_parse(gimp_gtkrc());
           tips = gtk_tooltips_new ();
	   gtk_tooltips_set_delay(tips, 2500);

  	   break;
  case RUN_WITH_LAST_VALS:
	/* Possibly retrieve data */
	   argc = 1;
           argv = g_new(gchar *, 1);
           argv[0] = g_strdup("apply_stegano");
           gtk_init(&argc, &argv);
           gtk_rc_parse(gimp_gtkrc());
           tips = gtk_tooltips_new ();
	   gtk_tooltips_set_delay(tips, 2500);

           gimp_get_data("plug_in_stegano", &wvals);
	
	break;
  default:
    break;
  }
  if (status == STATUS_SUCCESS) {
    if (!wvals.mode) {
        gimp_tile_cache_ntiles(2 *(drawable->width / gimp_tile_width() + 1));
/*        gimp_progress_init("Stegano plug-in is working. Please wait...");  */
        drawstegano(drawable);
        if(run_mode != RUN_NONINTERACTIVE)
          gimp_displays_flush();
    } else {
        readstegano(drawable);
    }
  }
  if(run_mode == RUN_INTERACTIVE)
    gimp_set_data("plug_in_stegano", &wvals, sizeof(steganoValues));
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
drawstegano(GDrawable *drawable)
{

  FILE *In;

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
  
  char actualchar[1];
  gint made_it_once=FALSE;
  glong backup;
  gchar * point;
  
  typedef struct {
    gchar identity[4];
    gchar filename[128];
    gdouble len;
  } _header;
  _header header;

  point=(char*) &header;
  for (i=0;i<sizeof(header); i++)
  {
    point[i]='\0';
  }

  strncpy(header.identity,"STG",3);
  strncpy(header.filename,wvals.filename,128);
  
  header.identity[3]='\0'; 
  
  wvals.filename[127]='\0';

  In=fopen(wvals.filename, "rb");
  if (In == NULL) {
          message_dialog("Error", "Could not open file", "The file you have entered in the\n"
                                                         "edit field could not be opened.\n"
							 "Check spelling and try again.\n");
          return;
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
  gimp_pixel_rgn_init(&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  src = g_malloc((x2-x1)*(y2-y1)*bytes);
  dest = g_malloc((x2-x1)*(y2-y1)*bytes);
  
  gimp_pixel_rgn_get_rect(&srcPR, src, x1, y1, regionwidth, regionheight);

  backup = ftell(In);
  fseek(In, 0, SEEK_END);
  header.len = ftell(In)-backup;
  fseek(In, backup, SEEK_SET);

  point = (char *) &header;  
  actualchar[0]=point[0];
  
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
	  for(i = 0; i < bytes; i++) {
	    smallcount++;
	    if (smallcount==9) {
	      if  (count<sizeof(header)) 
	       {
	          point++;
	  	  actualchar[0]=point[0];
	       }
	      else
	       {  
                 if (!feof(In)) 
                    { actualchar[0]=fgetc(In);}
                 else 
                    { made_it_once=TRUE;}
               }		  
	      count++;
 	      smallcount=1;
	    }
	    value = src[pos+i];
	    if (!made_it_once) {
	         if (actualchar[0]&(1<<(smallcount-1))){ 
	            if ((value % 2) == 0){
	   	       value+=1;
                    }		    
	         } else {
	            if ((value % 2) == 1){
		       value+=1;
		       if (value==256) { value=254; }
                    }		    
	         } 
	    }
	    dest[pixelpos+i] = value; 
	  }
    }
/*    if(((gint)(regionwidth-col) % 5) == 0)
       gimp_progress_update((gdouble)col/(gdouble)regionwidth); 
*/
  }
  
  fclose(In);
  
  if (!made_it_once) {
     message_dialog("Error", "Could not hide file", "\nThe picture is to small to contain the\n"
                                                    "whole file you wanted to hide. Use an image\n"
						    "with larger dimensions and try again.\n"); 
     return;
  } else {
     message_dialog("File hidden successfully", NULL, "\nYour file has been hidden\n"
                                                       "successfully! Start the Stegano\n"
						       "plug-in once again and select\n"
						       "'read' to extract the hidden\n"
						       "file included in the image.\n");
  }
  gimp_pixel_rgn_set_rect(&destPR, dest, x1, y1, regionwidth, regionheight);
  g_free(src);
  g_free(dest);
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1,(x2 - x1),(y2 - y1));
  gimp_displays_flush(); 
  gtk_widget_destroy(maindlg);
  gtk_main_quit();
}

static void
readstegano(GDrawable *drawable)
{
  FILE * Out = NULL;
  GPixelRgn srcPR;
  gint width, height;
  gint bytes;
  gint row, value;
  gint x1, y1, x2, y2, ix, iy;
  guchar *src;
  gint col, i;
  gfloat regionwidth, regionheight, dx, dy;
  gfloat a, b, x, y;
  glong pixelpos, pos;
  guchar bgr_red, bgr_blue, bgr_green;
  long count=1,smallcount=0;

  char actualchar[1];
  gint made_it_once=FALSE;
  gint read_header=TRUE;
  gchar * point;
  gchar displaystring[200];
  
  typedef struct {
    gchar identity[4];
    gchar filename[128];
    gdouble len;
  } _header;
  _header header;

  point=(char*) &header;
  for (i=0;i<sizeof(header); i++)
  {
    point[i]='\0';
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
  




  point=(char *)&header;  
    
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
      for(i = 0; i < bytes; i++) {
	  smallcount++;
	  if (smallcount==9) {
	    if  (count<sizeof(header)) 
	     {
	        point++;
	     }
	    else
	     { 
	       if (header.len<0) {
	          made_it_once=TRUE;   
	          goto fin;
	       }
	       if (read_header) {
	           if (strncmp("STG",header.identity,3)) {
                      message_dialog("Error", NULL, "The picture you wanted to process contains\n"
                                                    "no hidden file. Maybe it has been destroyed\n"
					            "with some image manipulation.");
                      
		      printf(header.identity);
		      printf("\n");				    
		      return;
                   }

	           read_header=FALSE;
		   strncpy(filename,header.filename,128);
		   while (1)
		   {
		      if (!stegano_save_dialog()) 
		       {
                        message_dialog("Message", NULL, "File extraction has been aborted.\n");
  		        return;  
		       }
 	              Out = fopen(filename, "rb");
                      if (Out != NULL) {
                         message_dialog("Error", NULL, "The destination file already exists.\n");
                         fclose(Out);
                      } else {
		         break;  
		      }  
		   }
		   Out = fopen(filename, "wb");
                   if (Out == NULL) {
                     message_dialog("Error", NULL, "The destination file could not be created.\n");
                     fclose(Out);
                     return;
                   }
	       } else {
	          fputc(actualchar[0],Out);
	       }
	       actualchar[0]='\0';
	       header.len-=1;
             }		  
	     count++;
	     smallcount=1;
	  }
	      value = src[pos+i];
              if ((value % 2) == 1){
	                  if (read_header) 
			    {
                   	          point[0]|=(1<<(smallcount-1));
			    }
			  else  
			    {
                	          actualchar[0]|=(1<<(smallcount-1));
			    }
             } 
	}
    }
  }
fin:
  fclose(Out);
  g_free(src);

  if (!made_it_once) {
     message_dialog("Error", "File truncated", "The file included in this image\n"
                                               "has not the correct size and has\n"
					       "been corrupted. The data won't\n"
					       "be the original one.");
     return;
  }
  sprintf(displaystring,"The file '%s' has been saved successfully.", filename);
  message_dialog("File read successfully", NULL, displaystring);
}

void
stegano_close_callback(GtkWidget *widget,  gpointer   data)
{ 
  gtk_widget_destroy(maindlg);
  gtk_main_quit();
}


void
stegano_save_ok_callback(GtkWidget *widget,  gpointer   data)
{ 
  saverun = TRUE;
  gtk_widget_destroy(savedlg);
}

void
stegano_save_ok2_callback(GtkWidget *widget,  gpointer   data)
{ 
  saverun = FALSE;
  gtk_widget_destroy(savedlg);
}

void
stegano_save_close_callback(GtkWidget *widget,  gpointer   data)
{ 
  gtk_main_quit();
}


void
stegano_logo_close_callback(GtkWidget *widget,  gpointer   data)
{ 
  gtk_main_quit();
}

void
stegano_warning_close_callback(GtkWidget *widget,  gpointer   data)
{ 
  gtk_main_quit();
}

void
stegano_message_close_callback(GtkWidget *widget,  gpointer   data)
{ 
  gtk_main_quit();
}

/**********************************************************************
 FUNCTION: destroy_window
 *********************************************************************/

void
destroy_window(GtkWidget * widget,
	       GtkWidget ** window)
{
    *window = NULL;
}


void
stegano_ok_callback(GtkWidget *widget, gpointer   data)
{
        gtk_widget_set_sensitive (maindlg, FALSE); 
        if (wvals.warningmessage) stegano_warning_dialog();
        gimp_tile_cache_ntiles(2 *(drawable->width / gimp_tile_width() + 1));
/*        gimp_progress_init("Stegano plug-in is working. Please wait...");  */
        drawstegano(drawable);
	gtk_widget_set_sensitive (maindlg, TRUE); 
        gtk_widget_show(maindlg);
}

void
stegano_ok2_callback(GtkWidget *widget, gpointer   data)
{
        gtk_widget_set_sensitive (maindlg, FALSE); 
        readstegano(drawable);
	gtk_widget_set_sensitive (maindlg, TRUE); 
	gtk_widget_show(maindlg);
}

void
load_file_selection_ok(GtkWidget * w,
		       GtkFileSelection * fs,
		       gpointer data)
{
    struct stat         filestat;
    gint                err;
    char *		filename;

    filename = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs)));

    err = stat(filename, &filestat);

    if (!err && S_ISREG(filestat.st_mode)) {
       gtk_entry_set_text(GTK_ENTRY(globalentry), filename);
    }
    
    gtk_widget_destroy(GTK_WIDGET(fs));
    gtk_widget_set_sensitive (maindlg, TRUE); 

    gtk_widget_show(maindlg);

}


void
save_file_selection_ok(GtkWidget * w,
		       GtkFileSelection * fs,
		       gpointer data)
{
    struct stat         filestat;
    gint                err;

    strncpy(filename, gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs)),128);

    err = stat(filename, &filestat);

    if (!err && S_ISREG(filestat.st_mode)) {
       gtk_entry_set_text(GTK_ENTRY(saveglobalentry), filename);
    }
    
    gtk_widget_destroy(GTK_WIDGET(fs));

}


void
load_file_selection_cancel(GtkWidget * w,
		       GtkFileSelection * fs,
		       gpointer data)
{
    gtk_widget_set_sensitive (maindlg, TRUE); 
    gtk_widget_destroy(GTK_WIDGET(fs));

    gtk_widget_show(maindlg);


}

void
save_file_selection_cancel(GtkWidget * w,
		       GtkFileSelection * fs,
		       gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(fs));
}


void
stegano_browse_callback(GtkWidget *widget, gpointer   data)
{
    GtkWidget   *window = NULL;

  /* Load a single object */
    gtk_widget_set_sensitive (maindlg, FALSE); 

    window = gtk_file_selection_new("Select file to include in image");
    gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);

    gtk_signal_connect(GTK_OBJECT(window), "destroy",
		       (GtkSignalFunc) destroy_window,
		       &window);

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
		       "clicked", (GtkSignalFunc) load_file_selection_ok,
		       (gpointer) window);
    set_tooltip(tips, GTK_FILE_SELECTION(window)->ok_button, "Load the selected file into the entry field");

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(window)->cancel_button),
			      "clicked", (GtkSignalFunc) load_file_selection_cancel,
			      (gpointer)(window));
    set_tooltip(tips, GTK_FILE_SELECTION(window)->cancel_button, "Cancel selection of new file");
    if (!GTK_WIDGET_VISIBLE(window))
	gtk_widget_show(window);

}


void
stegano_save_browse_callback(GtkWidget *widget, gpointer   data)
{
    GtkWidget   *window = NULL;

  /* Load a single object */

    window = gtk_file_selection_new("Select file or enter new name");
    gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);

    gtk_signal_connect(GTK_OBJECT(window), "destroy",
		       (GtkSignalFunc) destroy_window,
		       &window);

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
		       "clicked", (GtkSignalFunc) save_file_selection_ok,
		       (gpointer) window);
    set_tooltip(tips, GTK_FILE_SELECTION(window)->ok_button, "Load the selected file into the entry field");

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(window)->cancel_button),
			      "clicked", (GtkSignalFunc) save_file_selection_cancel,
			      (gpointer)(window));
    set_tooltip(tips, GTK_FILE_SELECTION(window)->cancel_button, "Cancel selection of new file");
    if (!GTK_WIDGET_VISIBLE(window))
       gtk_widget_show(window);
}

void
stegano_logo_ok_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, TRUE); 
  gtk_widget_destroy(logodlg);
}

void
stegano_message_ok_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, TRUE); 
  gtk_widget_destroy(messagedlg);
}


void
stegano_warning_ok_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, TRUE); 
  gtk_widget_destroy(warningdlg);
}

void
stegano_about_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, FALSE); 
  stegano_logo_dialog();
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
stegano_entry1_callback (GtkWidget * widget, gpointer data)
{
  strncpy (wvals.filename, gtk_entry_get_text (GTK_ENTRY (widget)), 128);
}

static void
stegano_save_entry1_callback (GtkWidget * widget, gpointer data)
{
  strncpy (filename, gtk_entry_get_text (GTK_ENTRY (widget)), 128);
}


static gint
stegano_dialog()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  gchar buffer[100];
  
  
  bint.run=FALSE;
  
  dlg = maindlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Stegano <cotting@mygale.org>");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_NONE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)stegano_close_callback,
		     NULL);
		     
		     
		     
  frame = gtk_frame_new("Specify the file you would like to hide in the image data");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  tips = gtk_tooltips_new ();
  gtk_tooltips_set_delay(tips, 2500);

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

  
  label = gtk_label_new("Filename: ");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  globalentry=entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%s", wvals.filename);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)stegano_entry1_callback,
		     NULL); 
  gtk_widget_show(entry);
  set_tooltip(tips,entry,"Here you can specify the file you would like to hide in this image.");

  button = gtk_button_new_with_label("Browse...");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)stegano_browse_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Browse directories to select a file.");

  
  gtk_widget_show(hbox);
  
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  button = gtk_button_new_with_label("Write");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)stegano_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  set_tooltip(tips,button,"Close the dialog box and hide the specified file.");
  
  button = gtk_button_new_with_label("Read");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)stegano_ok2_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  set_tooltip(tips,button,"Close the dialog box and read a hidden file from this image.");

  button = gtk_button_new_with_label("Exit");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Close the dialog box without reading or writing a stegano.");

  button = gtk_button_new_with_label("About...");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)stegano_about_callback,button);
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
stegano_save_dialog()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  gchar buffer[100];
  
  
  saverun = FALSE;
  savedlg = dlg =  gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Save as...");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_NONE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)stegano_save_close_callback,
		     NULL);
		     
		     
		     
  frame = gtk_frame_new("Specify the name of the new file");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  tips = gtk_tooltips_new ();
  gtk_tooltips_set_delay(tips, 2500);
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

  
  label = gtk_label_new("Filename: ");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  saveglobalentry=entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%s", filename);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)stegano_save_entry1_callback,
		     NULL); 
  gtk_widget_show(entry);
  set_tooltip(tips,entry,"Here you can specify the name of the new file.");

  button = gtk_button_new_with_label("Browse...");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)stegano_save_browse_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Browse directories to select a file.");

  
  gtk_widget_show(hbox);
  
  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  button = gtk_button_new_with_label("OK");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)stegano_save_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  set_tooltip(tips,button,"Close the dialog box and extract file.");
  
  button = gtk_button_new_with_label("Cancel");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)stegano_save_ok2_callback,
			    dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Close the dialog box without extracting file.");

  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush(); 

  return saverun;
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

  messagedlg = dlg = gtk_dialog_new();
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
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_NONE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)stegano_message_close_callback,
		     NULL);


  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)stegano_message_ok_callback,
			    dlg);
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
stegano_warning_dialog()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  bint.run=FALSE;

  warningdlg = dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Warning");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_NONE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)stegano_warning_close_callback,
		     NULL);

  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)stegano_warning_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Proceed with hiding the file.");

  frame = gtk_frame_new("Please note:");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);


  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new("Warning:\n\n"
                        "Remember to save your image in a non-destructive format!\n"
			"For RGB images use BMP/TIFF/TGA etc. Never use JPEG.\n\n"
			"You shouldn't apply any filter to the image after the\n"
			"plug-in has hidden a file. No blur/constrast etc.\n");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  toggle = gtk_check_button_new_with_label ("Show warning message every time");
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &wvals.warningmessage);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), wvals.warningmessage);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If you want this message to be displayed every time you hide a file in an image, then check this box.");

  gtk_widget_show(hbox);

  gtk_widget_show(vbox);
  gtk_widget_show(frame);
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return bint.run;
}

GtkWidget * 
stegano_logo_dialog()
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
  gtk_window_position(GTK_WINDOW(xdlg), GTK_WIN_POS_NONE);
  
  gtk_signal_connect(GTK_OBJECT(xdlg), "destroy",
                     (GtkSignalFunc)stegano_logo_close_callback,
		     NULL);

  xbutton = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(xbutton, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(xbutton), "clicked",
                     (GtkSignalFunc)stegano_logo_ok_callback,
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
          "Stegano\n Plug-In for the GIMP\n"
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
