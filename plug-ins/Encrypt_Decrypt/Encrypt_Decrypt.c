/**********************************************************************
    ATTENTION: Plug-In Arguments Have Been Changed!!!!!
               This version will not be fully backwards compatible
	       with the version 1.0x. Nevertheless it can encrypt and
	       decrypt version 1.0x images.
**********************************************************************/	       


/**********************************************************************
 *  Encrypt_Decrypt Plug-In (Version 2.02)
 *  Daniel Cotting (cotting@mygale.org)
 **********************************************************************
 *  Official homepages: http://www.mygale.org/~cotting
 *                      http://cotting.citeweb.net
 *                      http://village.cyberbrain.com/cotting
 *********************************************************************/    

 
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

 
/**********************************************************************
 * Include files
 *********************************************************************/    
 
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "libgimp/gimp.h"
#include "gtk/gtk.h"
#include "logo.h"

/**********************************************************************
 * Define constants
 *********************************************************************/    

#define ENTRY_WIDTH 260
/* Large Prime */
#define MULT 1103515245l

/**********************************************************************
 * Declare some of the local functions
 *********************************************************************/    
 
static void query(void);
static void run(char *name, int nparams,
		GParam *param,
		int *nreturn_vals,
		GParam **return_vals);
static void drawEncrypt(GDrawable *drawable);
static gint encrypt_dialog(void);
static gint encrypt_warning_dialog(void);
static gint encrypt_enter_dialog(void);
static gint encrypt_no_last_val_dialog(void);
GtkWidget * encrypt_logo_dialog(void);


/**********************************************************************
 * Define types of structures
 *********************************************************************/    

typedef struct {
    char password[128];
    gint warningmessage;
    gint compatibility;
    gint remember;
} EncryptValues;

typedef struct {
  gint run;
} EncryptInterface;

/**********************************************************************
 * Define structure and set their values
 *********************************************************************/    

GPlugInInfo PLUG_IN_INFO =
{
  NULL, /* init_proc */
  NULL, /* quit_proc */
  query, /* query_proc */
  run, /* run_proc */
};

static EncryptValues wvals = {
        "Enter password here.",1,0,0,
}; /* wvals */

static EncryptInterface bint =
{
  FALSE  /*  run  */
};

/**********************************************************************
 * Declare global variables
 *********************************************************************/    

GtkWidget *maindlg;
GtkWidget *logodlg;
GtkTooltips *tips;
GdkColor tips_fg,tips_bg;	
/* This is initialized with the password or a part of the password. It is
   used as the seed value in StandardRandom */
long Seed;
/* Table for R250 number generator */
long Table[250];
/* Increment table - speeds R250 up, but not really needed in this case */
unsigned char IncrementTable[250];
/* Two Index values into R250 table */
unsigned char Index1, Index2;


/**********************************************************************
 * call MAIN()
 *********************************************************************/    

MAIN()

/**********************************************************************
 * FUNCTION query
 *********************************************************************/    

static void
query(void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_STRING, "password", "Password (used to encrypt and decrypt)" },
    { PARAM_INT8, "warning", "Disable warning message toggle (only in RUN_INTERACTIVE)" },
    { PARAM_INT8, "compatibility", "Use version 1.0x compatibility mode" },
    { PARAM_INT8, "remember", "Remember the password after execution" },
  };

  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args)/ sizeof(args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure("plug_in_encrypt",
			 "Encrypt the image using a code, second call with same code decrypts image.",
			 "",
			 "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
			 "Daniel Cotting (cotting@mygale.org, http://www.mygale.org/~cotting)",
			 "October, 1997",
			 "<Image>/Filters/Image/Encrypt & Decrypt",
			 "RGB*, GRAY*, INDEXED*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);
}

/**********************************************************************
 * FUNCTION run
 *********************************************************************/    

static void
run(char *name,
    int nparams,
    GParam *param,
    int *nreturn_vals,
    GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  int succeeded=1;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gchar **argv;
  gint argc;
  int iaa, expression;

  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  
  *nreturn_vals = 1;
  *return_vals = values;
  
  drawable = gimp_drawable_get(param[2].data.d_drawable);

  switch(run_mode) {
  case RUN_INTERACTIVE:
	/* Possibly retrieve data */
	gimp_get_data("plug_in_encrypt", &wvals);
	/* Get information from the dialog */
  
        argc = 1;
        argv = g_new(gchar *, 1);
        argv[0] = g_strdup("apply_encrypt");
        gtk_init(&argc, &argv);
        gtk_rc_parse(gimp_gtkrc());

	do {
	   if (!encrypt_dialog())
               return;
  	   expression=((!strcmp(wvals.password,"Enter password here.") || 
	              (wvals.compatibility&&(atoi(wvals.password)==0)))
	              && (succeeded=encrypt_enter_dialog()));       
	} while (expression);
	
	if (!succeeded)	   
	   return;
        if (wvals.warningmessage)
           if (!encrypt_warning_dialog())
               return;
	break;
  case RUN_NONINTERACTIVE:
	/* Make sure all the arguments are present */
	if (nparams != 7)
           status = STATUS_CALLING_ERROR;
	if (status == STATUS_SUCCESS) {
           strcpy(wvals.password,param[3].data.d_string);
           wvals.warningmessage = param[4].data.d_int8;
           wvals.compatibility = param[5].data.d_int8;
           wvals.remember = param[6].data.d_int8;
	}   
	break;
  case RUN_WITH_LAST_VALS:
	/* Possibly retrieve data */
	gimp_get_data("plug_in_encrypt", &wvals);
        argc = 1;
        argv = g_new(gchar *, 1);
        argv[0] = g_strdup("apply_encrypt");
        gtk_init(&argc, &argv);
        gtk_rc_parse(gimp_gtkrc());
	if (!wvals.remember) { 
	   encrypt_no_last_val_dialog();
	   return;
	}   
	break;
  default:
    break;
  }
  if (status == STATUS_SUCCESS) {
    gimp_tile_cache_ntiles(2 *(drawable->width / gimp_tile_width() + 1));
    gimp_progress_init("Encrypting image. Please wait...");
    drawEncrypt(drawable);
    if (!wvals.remember){
       /* Delete active Password */
       for (iaa=0; iaa<128; iaa++) wvals.password[iaa] = 0;
    }
    if(run_mode != RUN_NONINTERACTIVE)
      gimp_displays_flush();
    if(run_mode == RUN_INTERACTIVE)
      gimp_set_data("plug_in_encrypt", &wvals, sizeof(EncryptValues));
    values[0].data.d_status = status;
    gimp_drawable_detach(drawable);
  }
}



/**********************************************************************
 * FUNCTIONS for the random number generator
 *********************************************************************/    

/* Code provided by Pascal Schuppli, Worb (Switzerland)
   Fast high security encryption based on two 'random' number generators
   Sets up the increment table and the two index pointers */
void InitGenerator() {
 int i;
 for (i=0; i<249; i++) IncrementTable[i] = i+1;
 IncrementTable[249] = 0;
 Index1 = 0;
 Index2 = 103;
}

/* StandardRandom is used to initialize the R250 random number table. It is
   neither fast nor very good. */
long StandardRandom() {

 unsigned long lo, hi, ll, lh, hh, hl;

 lo = Seed & 0xFFFF;
 hi = Seed >> 16;
 Seed = Seed * MULT + 12345;
 ll = lo*(MULT & 0xFFFF);
 lh = lo*(MULT >> 16);
 hl = hi*(MULT & 0xFFFF);
 hh = hi*(MULT >> 16);
 return ((ll + 12345) >> 16) + lh + hl + (hh << 16);
}

/* Fill R250 table. Use two seeds (64 bit encoding)
   This is where most of the processing power goes when trying to break an
   encrypted file. Because StandardRandom is not very fast, and because it is
   called 500 times, this takes the CPU a couple thousand cycles. The table
   needed to decrypt a file thus can't be constructed without serious delays,
   which is the whole point of high security encryption. */
void FillTable(long FirstSeed, long SecondSeed) {
 int i;
 /* Construct table with first seed */
 Seed = FirstSeed;
 for (i=0; i<250; i++)
   Table[i] = StandardRandom();

 /* Xor second seed into table. Not sure whether this makes the random numbers
    better or worse, but it takes time */
 Seed = SecondSeed;
 for (i=0; i<250; i++)
   Table[i] ^= StandardRandom();

}

/* Returns a four-byte random number generated by 'xoring' two table entries */
unsigned long R250Random() {
 unsigned long ret;
 ret = (Table[Index1] ^= Table[Index2]);

 Index1 = IncrementTable[Index1];
 Index2 = IncrementTable[Index2];
 return ret;
}

/**********************************************************************
 * FUNCTION drawEncrypt
 *********************************************************************/    

static void
drawEncrypt(GDrawable *drawable)
{
  char Password[128];

  /* Take some primes to start with */
  long s1 = 17, s2 = 23;

  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  gint row;
  gint x1, y1, x2, y2, ix, iy;
  guchar *src, *dest;
  int password;
  gint i, col;
  gfloat regionwidth, regionheight, dx, dy;
  gfloat a, b, x, y;
  glong pixelpos, pos;
  guchar bgr_red, bgr_blue, bgr_green;
  
  if (!wvals.compatibility){
     for (i=0; i<128; i++) Password[i] = 1;
     strcpy(Password, wvals.password);
     InitGenerator();


 /* The hash algorithm should make sure that the hash codes it generates
    differ so that code breakers cannot concentrate on a certain range of
    seeds to speed up the process of breaking the code.

    Hash up keyword, -> create two four-byte seeds */
    
    for (i=0; i<strlen(Password); i++) {
      s1 ^= *(long *)&Password[i];
      s1 <<= 1;
    }
    for (i=strlen(Password)-1; i>=0; i--) {
      s2 ^= *(long *)&Password[i];
      s2 <<= 3;
    }

 /* Generate table with hash value */
    FillTable(s1, s2);
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
  if (wvals.compatibility)  {
     password=atoi(wvals.password);
     srand(password);
  }
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
	      if (!wvals.compatibility) {
                 dest[pixelpos+i] = src[pos+i]^R250Random();
	      } else {	 
                 dest[pixelpos+i] = src[pos+i] ^(int)(rand()*255)^(int)(rand()*255);
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

/**********************************************************************
 * FUNCTIONS: callbacks
 *********************************************************************/    

static void
encrypt_close_callback(GtkWidget *widget,  gpointer   data)
{ 
  gtk_main_quit();
}


static void
encrypt_ok_callback(GtkWidget *widget, gpointer   data)
{
  bint.run = TRUE;
  gtk_widget_destroy(GTK_WIDGET (data));
}

static void
encrypt_logo_ok_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, TRUE);
  gtk_widget_destroy(logodlg);
}

static void
encrypt_about_callback(GtkWidget *widget, gpointer   data)
{
  gtk_widget_set_sensitive (maindlg, FALSE);
  encrypt_logo_dialog();
}

static void
encrypt_entry_callback(GtkWidget *widget, gpointer   data)
{
  strcpy(wvals.password,gtk_entry_get_text(GTK_ENTRY(widget)));
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

/**********************************************************************
 * FUNCTION set_tooltip
 *********************************************************************/    

static void
set_tooltip (GtkTooltips *tooltips, GtkWidget *widget, const char *desc)
{
  if (desc && desc[0])
    gtk_tooltips_set_tips (tooltips, widget, (char *) desc);
}

/**********************************************************************
 * FUNCTIONS: encrypt_dialog
 *********************************************************************/    

static gint
encrypt_dialog()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  gchar buffer[128];
  bint.run=FALSE;
  
  
  dlg = maindlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Encrypt & Decrypt <cotting@mygale.org>");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)encrypt_close_callback,
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
  
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new("Password: ");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, FALSE, 0);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%s", wvals.password);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)encrypt_entry_callback,
		     NULL);
  gtk_widget_show(entry);
  set_tooltip(tips,entry,"Here, you can specify your password. To decrypt your image, just enter the value you used for the encryption. Unless you use version 1.0x compatibility (numeric passwords), the password can contain any characters.");

  gtk_widget_show(hbox);

  toggle = gtk_check_button_new_with_label ("Show warning message");
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &wvals.warningmessage);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), wvals.warningmessage);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If this option is enabled, a warning message will be displayed every time you encrypt/decrypt an image. Make sure to read these warnings at least once.");

  toggle = gtk_check_button_new_with_label ("Enable version 1.0x compatibility");
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &wvals.compatibility);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), wvals.compatibility);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If this option is enabled, the encryption/decryption will be compatible with the version 1.0x plug-in.");

  toggle = gtk_check_button_new_with_label ("Remember password");
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &wvals.remember);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), wvals.remember);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If this option is enabled, the password will be remembered for a further encryption/decryption.");

  gtk_widget_show(vbox);
  gtk_widget_show(frame);

  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)encrypt_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  set_tooltip(tips,button,"Close the dialog box and encrypt/decrypt your image with the specified password.");

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Close the dialog box without altering your image.");

  button = gtk_button_new_with_label("About...");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)encrypt_about_callback,button);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Show information about the author and the plug-in.");

  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return bint.run;
}

/**********************************************************************
 * FUNCTION encrypt_warning_dialog
 *********************************************************************/    

static gint
encrypt_warning_dialog()
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
                     (GtkSignalFunc)encrypt_close_callback,
		     NULL);

  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)encrypt_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Proceed with the encryption/decryption.");

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Cancel the encryption/decryption.");

  frame = gtk_frame_new("Please note:");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);


  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  
  if (wvals.compatibility) {
  label = gtk_label_new("Version 1.0x compatibility mode:\n\n"
                        "This plug-in uses a numeric password to encrypt your image.\n\n"
                        "Although it has been tested thoroughly, the author cannot be\n"
			"sure that it will work properly in all circumstances. Therefore\n"
			"the author doesn't want to take any responsibility in case of\n"
			"data loss or any other damage this plug-in could cause.\n\n"
			"*******USE IT AT YOUR OWN RISK (AND ENJOY IT)!*******\n\n"
			"The plug-in could fail in the following situations, because of\n"
			"a possibly different implementation of the random number\n"
			"generator: - Encrypt a picture and decrypt it on a different\n"
			"plattform. OR  - Decrypt a picture that was encrypted on a\n"
			"computer with a different OS-version or a different math-lib.\n\n"
			"Remember to save your image in a non-destructive format!\n"
			"For indexed images GIF could be a good choice, for RGB\n"
			"use BMP/TIFF/TGA etc. Never use a JPEG-compression!\n");
  } else {
  label = gtk_label_new("This plug-in uses a password to encrypt your image.\n\n"
                        "Although it has been tested thoroughly, the author cannot be\n"
			"sure that it will work properly in all circumstances. Therefore\n"
			"the author doesn't want to take any responsibility in case of\n"
			"data loss or any other damage this plug-in could cause.\n\n"
			"*******USE IT AT YOUR OWN RISK (AND ENJOY IT)!*******\n\n"
			"Remember to save your image in a non-destructive format!\n"
			"For indexed images GIF could be a good choice, for RGB\n"
			"use BMP/TIFF/TGA etc. Never use a JPEG-compression!\n");
  
  }			
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  toggle = gtk_check_button_new_with_label ("Show warning message every time");
  gtk_box_pack_start(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) toggle_update,
		      &wvals.warningmessage);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), wvals.warningmessage);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If you want this message to be displayed every time you encrypt/decrypt an image, then check this box.");

  gtk_widget_show(hbox);

  gtk_widget_show(vbox);
  gtk_widget_show(frame);
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return bint.run;
}


/**********************************************************************
 * FUNCTION encrypt_enter_dialog
 *********************************************************************/    

static gint
encrypt_enter_dialog()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  bint.run=FALSE;
  dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Warning");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)encrypt_close_callback,
		     NULL);

  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)encrypt_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Repeat password selection.");

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  set_tooltip(tips,button,"Cancel process of encryption/decryption.");

  frame = gtk_frame_new("Password needed:");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);


  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new("You have not entered a valid, personalized password.\n"
                        "If you want to use the version 1.0x compatibility\n"
			"mode, make sure that the password consists only of\n"
			"numerical characters (0-9).\n\n"
                        "Choose OK to repeat your password selection.\n"
			"Choose CANCEL to abort the encryption.\n");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  gtk_widget_show(hbox);

  gtk_widget_show(vbox);
  gtk_widget_show(frame);
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return bint.run;
}


/**********************************************************************
 * FUNCTION encrypt_no_last_val_dialog
 *********************************************************************/    

static gint
encrypt_no_last_val_dialog()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  bint.run=FALSE;
  dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Error");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)encrypt_close_callback,
		     NULL);

  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc)encrypt_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  frame = gtk_frame_new("Run with last values:");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);


  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new("For security reasons, the last password has not\n"
                        "been saved for further use. Thus the plug-in cannot\n"
			"be executed in non-interactive mode. If you want to\n"
			"be able to do this, enable the option 'remember password'\n"
			"in the dialog box of the plug-in.\n");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  gtk_widget_show(hbox);

  gtk_widget_show(vbox);
  gtk_widget_show(frame);
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return bint.run;
}

/**********************************************************************
 * FUNCTION encrypt_logo_dialog
 *********************************************************************/    

GtkWidget * 
encrypt_logo_dialog()
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
                     (GtkSignalFunc)encrypt_close_callback,
		     NULL);

  xbutton = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(xbutton, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(xbutton), "clicked",
                     (GtkSignalFunc)encrypt_logo_ok_callback,
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
          "Encrypt & Decrypt\n Plug-In for the GIMP\n"
          "Version 2.02\n";
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


