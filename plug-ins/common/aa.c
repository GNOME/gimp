/**
 * aa.c version 1.0
 * A plugin that uses libaa (ftp://ftp.ta.jcu.cz/pub/aa) to save images as
 * ASCII.
 * NOTE: This plugin *requires* aalib 1.2 or later. Earlier versions will
 * not work.
 * Code copied from all over the GIMP source.
 * Tim Newsome <nuisance@cmu.edu>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aalib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/* 
 * Declare some local functions.
 */
static void       query      (void);
static void       run        (gchar      *name, 
			      gint        nparams, 
			      GParam     *param, 
			      gint       *nreturn_vals, 
			      GParam    **return_vals);
static gboolean   aa_savable (gint32      drawable_ID);
static gboolean   save_aa    (gint        output_type, 
			      gchar      *filename, 
			      gint32      image,
			      gint32      drawable);
static void       gimp2aa    (gint32      image, 
			      gint32      drawable_ID, 
			      aa_context *context);

static gint   type_dialog                 (int        selected);
static void   type_dialog_ok_callback     (GtkWidget *widget,
					   gpointer   data);
static void   type_dialog_toggle_update   (GtkWidget *widget, 
					   gpointer   data);
static void   type_dialog_cancel_callback (GtkWidget *widget, 
					   gpointer   data);

/* 
 * Some global variables.
 */

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

/**
 * Type the user selected. (Global for easier UI coding.)
 */
static gint selected_type = 0;


MAIN ()

static void 
query (void)
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
  static gint nsave_args = sizeof(save_args) / sizeof(save_args[0]);

  gimp_install_procedure ("file_aa_save",
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

  gimp_register_save_handler ("file_aa_save",
			      "ansi,txt,text,html",
			      "");
}

/**
 * Searches aa_formats defined by aalib to find the index of the type
 * specified by string.
 * -1 means it wasn't found.
 */
static int 
get_type_from_string (gchar *string)
{
  gint type = 0;
  aa_format **p = aa_formats;

  while (*p && strcmp ((*p)->formatname, string))
    {
      p++;
      type++;
    }

  if (*p == NULL)
    return -1;

  return type;
}

static void 
run (gchar   *name, 
     gint     nparams, 
     GParam  *param, 
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType  run_mode;
  GStatusType   status = STATUS_SUCCESS;
  gint          output_type = 0;
  static int    last_type = 0;
  gint32        image_ID;
  gint32        drawable_ID;
  GimpExportReturnType export = EXPORT_CANCEL;

  /* Set us up to return a status. */
  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_EXECUTION_ERROR;

  run_mode    = param[0].data.d_int32;
  image_ID    = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  /*  eventually export the image */ 
  switch (run_mode)
    {
    case RUN_INTERACTIVE:
    case RUN_WITH_LAST_VALS:
      INIT_I18N_UI();
      gimp_ui_init ("aa", FALSE);
      export = gimp_export_image (&image_ID, &drawable_ID, "AA", 
				  (CAN_HANDLE_GRAY |
				   CAN_HANDLE_ALPHA));
      if (export == EXPORT_CANCEL)
	{
	  values[0].data.d_status = STATUS_CANCEL;
	  return;
	}
      break;
    default:
      INIT_I18N();
      break;
    }

  if (!aa_savable (drawable_ID)) 
    {
      status = STATUS_CALLING_ERROR;
    }

  if (status == STATUS_SUCCESS)
    {
      switch (run_mode) 
	{
	case RUN_INTERACTIVE:
	  gimp_get_data ("file_aa_save", &last_type);
	  output_type = type_dialog (last_type);
	  if (output_type < 0)
	    status = STATUS_CANCEL;
	  break;

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 6)
	    {
	      status = STATUS_CALLING_ERROR;
	    }
	  else
	    {
	      output_type = get_type_from_string (param[5].data.d_string);
	      if (output_type < 0)
		status = STATUS_CALLING_ERROR;
	    }
	  break;

	case RUN_WITH_LAST_VALS:
	  gimp_get_data ("file_aa_save", &last_type);
	  output_type = last_type;
	  break;

	default:
	  break;
	}
    }

  if (status == STATUS_SUCCESS)
    {
      if (save_aa (output_type, param[3].data.d_string, image_ID, drawable_ID))
	{
	  status = STATUS_EXECUTION_ERROR;
	}
      else
	{
	  last_type = output_type;
	  gimp_set_data ("file_aa_save", &last_type, sizeof (last_type));
	}
    }

  if (export == EXPORT_EXPORT)
    gimp_image_delete (image_ID);  

  values[0].data.d_status = status;
}

/**
 * The actual save function. What it's all about.
 * The image type has to be GRAY.
 */
static gboolean
save_aa (gint    output_type, 
	 gchar  *filename, 
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
    return TRUE;

  gimp2aa (image_ID, drawable_ID, context);
  aa_flush (context);
  aa_close (context);

  /*fprintf(stderr, "Success!\n"); */

  return FALSE;
}

static void
gimp2aa (gint32      image, 
	 gint32      drawable_ID, 
	 aa_context *context)
{
  gint width, height, x, y;
  guchar *buffer;
  GDrawable *drawable = NULL;
  GPixelRgn pixel_rgn;
  aa_renderparams *renderparams = NULL;
  gint bpp;

  width = aa_imgwidth (context);
  height = aa_imgheight (context);
  /*fprintf(stderr, "gimp2aa %i x %i\n", width, height); */

  drawable = gimp_drawable_get (drawable_ID);

  bpp = drawable->bpp;
  buffer = g_new (guchar, width * bpp);

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width,
		       drawable->height, FALSE, FALSE);

  for (y = 0; y < height; y++) 
    {
      gimp_pixel_rgn_get_row (&pixel_rgn, buffer, 0, y, width);
      for (x = 0; x < width; x++) 
	{
	  /* Just copy one byte. If it's indexed that's all we need. Otherwise
	   * it'll be the most significant one. */
	  aa_putpixel (context, x, y, buffer[x * bpp]);
	}
    }

  renderparams = aa_getrenderparams ();
  renderparams->dither = AA_FLOYD_S;
  aa_render (context, renderparams, 0, 0, 
	     aa_scrwidth (context), aa_scrheight (context));
}

static gboolean 
aa_savable (gint32 drawable_ID)
{
  GDrawableType drawable_type;

  drawable_type = gimp_drawable_type (drawable_ID);

  if (drawable_type != GRAY_IMAGE && drawable_type != GRAYA_IMAGE)
    return FALSE;

  return TRUE;
}

/* 
 * User Interface dialog thingie.
 */

static gint 
type_dialog (int selected) 
{
  GtkWidget *dlg;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *toggle_vbox;
  GSList *group;

  /* Create the actual window. */
  dlg = gimp_dialog_new (_("Save as Text"), "aa",
			 gimp_plugin_help_func, "filters/aa.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), gtk_widget_destroy,
			 NULL, 1, NULL, TRUE, FALSE,
			 _("Cancel"), type_dialog_cancel_callback,
			 NULL, NULL, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*  file save type  */
  frame = gtk_frame_new (_("Data Formatting"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);

  toggle_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER(toggle_vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);
  
  group = NULL;
  {
    aa_format **p = aa_formats;
    gint current = 0;

    while (*p != NULL) 
      {
	toggle = gtk_radio_button_new_with_label (group, (*p)->formatname);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
	gtk_box_pack_start (GTK_BOX  (toggle_vbox), toggle, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			    GTK_SIGNAL_FUNC (type_dialog_toggle_update),
			    (*p)->formatname);
	if (current == selected)
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), TRUE);
	
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
  selected_type = get_type_from_string ((char *) data);
}
