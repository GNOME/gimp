/**
 * aa.c version 1.0
 * A plugin that uses libaa (ftp://ftp.ta.jcu.cz/pub/aa) to save images as
 * ASCII.
 * NOTE: This plugin *requires* aalib 1.2 or later. Earlier versions will
 * not work.
 * Code copied from all over the GIMP source.
 * Tim Newsome <nuisance@cmu.edu>
 */

#include <aalib.h>
#include <string.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

/* 
 * Declare some local functions.
 */
static void query      (void);
static void run        (char       *name, 
			int         nparams, 
			GParam     *param, 
		        int        *nreturn_vals, 
			GParam    **return_vals);
static void init_gtk   (void);
static gint aa_savable (gint32      drawable_ID);
static gint save_aa    (int         output_type, 
			char       *filename, 
			gint32      image,
			gint32      drawable);
static gint gimp2aa    (gint32      image, 
			gint32      drawable_ID, 
			aa_context *context);

static gint type_dialog                 (int        selected);
static void type_dialog_close_callback  (GtkWidget *widget,
					 gpointer   data);
static void type_dialog_ok_callback     (GtkWidget *widget,
					 gpointer   data);
static void type_dialog_toggle_update   (GtkWidget *widget, 
					 gpointer   data);
static void type_dialog_cancel_callback (GtkWidget *widget, 
					 gpointer data);

/* 
 * Some global variables.
 */

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

/**
 * Type the user selected. (Global for easier UI coding.
 */
static int selected_type = 0;


MAIN()
/**
 * Called by the GIMP to figure out what this plugin does.
 */
static void 
query()
{
  static GParamDef save_args[] =
  {
    {PARAM_INT32,    "run_mode",     "Interactive, non-interactive"},
    {PARAM_IMAGE,    "image",        "Input image"},
    {PARAM_DRAWABLE, "drawable",     "Drawable to save"},
    {PARAM_STRING,   "filename",     "The name of the file to save the image in"},
    {PARAM_STRING,   "raw_filename", "The name entered"},
    {PARAM_STRING,   "file_type",    "File type to use"}
  };
  static int nsave_args = sizeof(save_args) / sizeof(save_args[0]);

  gimp_install_procedure("file_aa_save",
			 "Saves files in various text formats",
			 "Saves files in various text formats",
			 "Tim Newsome <nuisance@cmu.edu>",
			 "Tim Newsome <nuisance@cmu.edu>",
			 "1997",
			 "<Save>/AA",
			 "GRAY*",		/* support grayscales */
			 PROC_PLUG_IN,
			 nsave_args, 0,
			 save_args, NULL);
  
  gimp_register_save_handler("file_aa_save", "ansi,txt,text,html", "");
}

/**
 * Searches aa_formats defined by aalib to find the index of the type
 * specified by string.
 * -1 means it wasn't found.
 */
static int 
get_type_from_string (char *string)
{
  int type = 0;
  aa_format **p = aa_formats;
  
  while (*p && strcmp((*p)->formatname, string)) {
    p++;
    type++;
  }

  if (*p == NULL)
    return -1;
  
  return type;
}

/**
 * Called by the GIMP to run the actual plugin.
 */
static void 
run (char    *name, 
     int      nparams, 
     GParam  *param, 
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GStatusType status = STATUS_SUCCESS;
  GRunModeType run_mode;
  int output_type = 0;
  static int last_type = 0;
  gint32 image_ID;
  gint32 drawable_ID;
  GimpExportReturnType export = EXPORT_CANCEL;
 

  /* Set us up to return a status. */
  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;
  
  run_mode    = param[0].data.d_int32;
  image_ID    = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;
  
  /*  eventually export the image */ 
  switch (run_mode)
    {
    case RUN_INTERACTIVE:
    case RUN_WITH_LAST_VALS:
      init_gtk ();
      export = gimp_export_image (&image_ID, &drawable_ID, "AA", 
				  (CAN_HANDLE_GRAY | CAN_HANDLE_ALPHA));
      if (export == EXPORT_CANCEL)
	{
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
	  return;
	}
      break;
    default:
      break;
    }
  
  if (!aa_savable (drawable_ID)) 
    {
      values[0].data.d_status = STATUS_CALLING_ERROR;
      return;
    }
  
  switch (run_mode) 
    {
    case RUN_INTERACTIVE:
      gimp_get_data ("file_aa_save", &last_type);
      output_type = type_dialog (last_type);
      break;
      
    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 6)
	status = STATUS_CALLING_ERROR;
      else
	output_type = get_type_from_string (param[5].data.d_string);
      break;
      
    case RUN_WITH_LAST_VALS:
      gimp_get_data ("file_aa_save", &last_type);
      output_type = last_type;
      break;
      
    default:
      break;
    }
  
  if (output_type < 0) 
    {
      status = STATUS_CALLING_ERROR;
      return;
    }
  
  if (save_aa (output_type, param[3].data.d_string, image_ID, drawable_ID))
    {
      values[0].data.d_status = STATUS_EXECUTION_ERROR;
      last_type = output_type;
      gimp_set_data ("file_aa_save", &last_type, sizeof(last_type));
    }    
  else
    values[0].data.d_status = STATUS_SUCCESS;
  
  if (export == EXPORT_EXPORT)
    gimp_image_delete (image_ID);  
}

/**
 * The actual save function. What it's all about.
 * The image type has to be GRAY.
 */
static gint 
save_aa (int     output_type, 
	 char   *filename, 
	 gint32  image_ID,
	 gint32  drawable_ID)
{
  aa_savedata savedata = {NULL, NULL};
  aa_context *context  = NULL;
  GDrawable *drawable  = NULL;
  aa_format format;
  
  /*fprintf(stderr, "save %s\n", filename); */
  
  drawable = gimp_drawable_get (drawable_ID);
  memcpy (&format, aa_formats[output_type], sizeof (format));
  format.width = drawable->width / 2;
  format.height = drawable->height / 2;
  
  /*fprintf(stderr, "save_aa %i x %i\n", format.width, format.height); */
  
  /* Get a libaa context which will save its output to filename. */
  savedata.name = filename;
  savedata.format = &format;
  
  context = aa_init (&save_d, &aa_defparams, &savedata);
  if (context == NULL)
    return 1;
  
  gimp2aa (image_ID, drawable_ID, context);
  aa_flush (context);
  aa_close (context);
  
  /*fprintf(stderr, "Success!\n"); */
  
  return 0;
}

static gint 
gimp2aa (gint32      image, 
	 gint32      drawable_ID, 
	 aa_context *context)
{
  int width, height, x, y;
  guchar *buffer;
  GDrawable *drawable = NULL;
  GPixelRgn pixel_rgn;
  aa_renderparams *renderparams = NULL;
  int bpp;
  
  width = aa_imgwidth (context);
  height = aa_imgheight (context);
  /*fprintf(stderr, "gimp2aa %i x %i\n", width, height); */
  
  drawable = gimp_drawable_get(drawable_ID);
  
  bpp = drawable->bpp;
  buffer = g_new(guchar, width * bpp);
  if (buffer == NULL)
    return 1;
  
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width,
		       drawable->height, FALSE, FALSE);

  for (y = 0; y < height; y++) 
    {
      gimp_pixel_rgn_get_row (&pixel_rgn, buffer, 0, y, width);
      for (x = 0; x < width; x++) 
	{
	  /* Just copy one byte. If it's indexed that's all we need. Otherwise
	   * it'll be the most significant one. */
	  aa_putpixel(context, x, y, buffer[x * bpp]);
	}
    }
  
  renderparams = aa_getrenderparams ();
  renderparams->dither = AA_FLOYD_S;
  aa_render (context, renderparams, 0, 0, 
	    aa_scrwidth (context), aa_scrheight (context));

  return 0;
}

static gint 
aa_savable (gint32 drawable_ID)
{
  GDrawableType drawable_type;
  
  drawable_type = gimp_drawable_type (drawable_ID);
  
  if (drawable_type != GRAY_IMAGE && drawable_type != GRAYA_IMAGE)
    return 0;
  
  return 1;
}

/* 
 * User Interface dialog thingie.
 */

static void 
init_gtk ()
{
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("save");
  
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
}

static gint 
type_dialog (int selected) 
{
  GtkWidget *dlg;
  GtkWidget *hbbox;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *toggle_vbox;
  GSList *group;
  
  /* Create the actual window. */
  dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Save as text");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
		     (GtkSignalFunc) type_dialog_close_callback, NULL);
  
 /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) type_dialog_ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) type_dialog_cancel_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  file save type  */
  frame = gtk_frame_new ("Data Formatting");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, FALSE, TRUE, 0);
  toggle_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER(toggle_vbox), 5);
  gtk_container_add (GTK_CONTAINER(frame), toggle_vbox);
  
  group = NULL;
  {
    aa_format **p = aa_formats;
    int current = 0;
    
    while (*p != NULL) 
      {
	toggle = gtk_radio_button_new_with_label (group, (*p)->formatname);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON(toggle));
	gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT(toggle), "toggled",
			    (GtkSignalFunc) type_dialog_toggle_update,
			    (*p)->formatname);
	if (current == selected)
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(toggle), 1);
	else
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(toggle), 0);
	
	gtk_widget_show (toggle);
	p++;
	current++;
      }
  }
  
  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);
  
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return selected_type;
}

/*
 * Callbacks for the dialog.
 */

static void 
type_dialog_close_callback (GtkWidget *widget, 
			    gpointer   data) 
{
  gtk_main_quit ();
}

static void 
type_dialog_ok_callback (GtkWidget *widget, 
			 gpointer   data) 
{
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void 
type_dialog_cancel_callback (GtkWidget *widget, 
			     gpointer   data) 
{
  selected_type = -1;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void 
type_dialog_toggle_update (GtkWidget *widget, 
			   gpointer   data) 
{
  selected_type = get_type_from_string ((char *)data);
}
