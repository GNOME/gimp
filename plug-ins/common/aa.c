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
static void     query      (void);
static void     run        (const gchar      *name,
                            gint              nparams,
                            const GimpParam  *param,
                            gint             *nreturn_vals,
                            GimpParam       **return_vals);
static gboolean save_aa    (gint32            drawable_ID,
                            gchar            *filename,
                            gint              output_type);
static void     gimp2aa    (gint32            drawable_ID,
                            aa_context       *context);

static gint     type_dialog                 (gint       selected);
static void     type_dialog_toggle_update   (GtkWidget *widget,
                                             gpointer   data);

/*
 * Some global variables.
 */

GimpPlugInInfo PLUG_IN_INFO =
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
  static GimpParamDef save_args[] =
  {
    {GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE,    "image",        "Input image"},
    {GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save"},
    {GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in"},
    {GIMP_PDB_STRING,   "raw_filename", "The name entered"},
    {GIMP_PDB_STRING,   "file_type",    "File type to use"}
  };

  gimp_install_procedure ("file_aa_save",
			  "Saves grayscale image in various text formats",
			  "This plug-in uses aalib to save grayscale image "
                          "as ascii art into a variety of text formats",
			  "Tim Newsome <nuisance@cmu.edu>",
			  "Tim Newsome <nuisance@cmu.edu>",
			  "1997",
			  "<Save>/AA",
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (save_args), 0,
			  save_args, NULL);

  gimp_register_save_handler ("file_aa_save",
			      "ansi,txt,text",
			      "");
}

/**
 * Searches aa_formats defined by aalib to find the index of the type
 * specified by string.
 * -1 means it wasn't found.
 */
static gint
get_type_from_string (const gchar *string)
{
  gint type = 0;
  aa_format **p = (aa_format **) aa_formats;

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
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint              output_type = 0;
  gint32            image_ID;
  gint32            drawable_ID;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;

  INIT_I18N ();

  /* Set us up to return a status. */
  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
  run_mode    = param[0].data.d_int32;
  image_ID    = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  /*  eventually export the image */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init ("aa", FALSE);
      export = gimp_export_image (&image_ID, &drawable_ID, "AA",
				  (GIMP_EXPORT_CAN_HANDLE_RGB  |
                                   GIMP_EXPORT_CAN_HANDLE_GRAY |
				   GIMP_EXPORT_CAN_HANDLE_ALPHA ));
      if (export == GIMP_EXPORT_CANCEL)
	{
	  values[0].data.d_status = GIMP_PDB_CANCEL;
	  return;
	}
      break;
    default:
      break;
    }

  if (! (gimp_drawable_is_rgb (drawable_ID) ||
         gimp_drawable_is_gray (drawable_ID)))
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  gimp_get_data ("file_aa_save", &output_type);
	  output_type = type_dialog (output_type);
	  if (output_type < 0)
	    status = GIMP_PDB_CANCEL;
	  break;

	case GIMP_RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 6)
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	    }
	  else
	    {
	      output_type = get_type_from_string (param[5].data.d_string);
	      if (output_type < 0)
		status = GIMP_PDB_CALLING_ERROR;
	    }
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_get_data ("file_aa_save", &output_type);
	  break;

	default:
	  break;
	}
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (save_aa (drawable_ID, param[3].data.d_string, output_type))
	{
	  gimp_set_data ("file_aa_save", &output_type, sizeof (output_type));
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image_ID);

  values[0].data.d_status = status;
}

/**
 * The actual save function. What it's all about.
 * The image type has to be GRAY.
 */
static gboolean
save_aa (gint32  drawable_ID,
         gchar  *filename,
         gint    output_type)
{
  aa_savedata  savedata;
  aa_context  *context;
  aa_format    format = *aa_formats[output_type];

  format.width  = gimp_drawable_width (drawable_ID)  / 2;
  format.height = gimp_drawable_height (drawable_ID) / 2;

  /* Get a libaa context which will save its output to filename. */
  savedata.name   = filename;
  savedata.format = &format;

  context = aa_init (&save_d, &aa_defparams, &savedata);
  if (!context)
    return FALSE;

  gimp2aa (drawable_ID, context);
  aa_flush (context);
  aa_close (context);

  return TRUE;
}

static void
gimp2aa (gint32      drawable_ID,
	 aa_context *context)
{
  GimpDrawable    *drawable;
  GimpPixelRgn     pixel_rgn;
  aa_renderparams *renderparams;

  gint    width;
  gint    height;
  gint    x, y;
  gint    bpp;
  guchar *buffer;
  guchar *p;

  drawable = gimp_drawable_get (drawable_ID);

  width  = aa_imgwidth  (context);
  height = aa_imgheight (context);
  bpp    = drawable->bpp;

  gimp_tile_cache_ntiles ((width / gimp_tile_width ()) + 1);

  gimp_pixel_rgn_init (&pixel_rgn,
                       drawable, 0, 0, width, height,
                       FALSE, FALSE);

  buffer = g_new (guchar, width * bpp);

  for (y = 0; y < height; y++)
    {
      gimp_pixel_rgn_get_row (&pixel_rgn, buffer, 0, y, width);

      switch (bpp)
        {
        case 1:  /* GRAY */
          for (x = 0, p = buffer; x < width; x++, p++)
            aa_putpixel (context, x, y, *p);
          break;

        case 2:  /* GRAYA, blend over black */
          for (x = 0, p = buffer; x < width; x++, p += 2)
            aa_putpixel (context, x, y, (p[0] * (p[1] + 1)) >> 8);
          break;

        case 3:  /* RGB */
          for (x = 0, p = buffer; x < width; x++, p += 3)
            aa_putpixel (context, x, y,
                         GIMP_RGB_INTENSITY (p[0], p[1], p[2]) + 0.5);
          break;

        case 4:  /* RGBA, blend over black */
          for (x = 0, p = buffer; x < width; x++, p += 4)
            aa_putpixel (context, x, y,
                         ((guchar) (GIMP_RGB_INTENSITY (p[0], p[1], p[2]) + 0.5)
                          * (p[3] + 1)) >> 8);
          break;

        default:
          g_assert_not_reached ();
          break;
        }
    }

  g_free (buffer);

  renderparams = aa_getrenderparams ();
  renderparams->dither = AA_FLOYD_S;

  aa_render (context, renderparams, 0, 0,
	     aa_scrwidth (context), aa_scrheight (context));
}

/*
 * User Interface dialog thingie.
 */

static gint
type_dialog (gint selected)
{
  GtkWidget *dlg;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *toggle_vbox;
  GSList    *group;

  /* Create the actual window. */
  dlg = gimp_dialog_new (_("Save as Text"), "aa",
                         NULL, 0,
			 gimp_standard_help_func, "filters/aa.html",

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

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
    aa_format **p = (aa_format **) aa_formats;
    gint current = 0;

    while (*p != NULL)
      {
	toggle = gtk_radio_button_new_with_label (group, (*p)->formatname);
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
	gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
	gtk_widget_show (toggle);

	g_signal_connect (toggle, "toggled",
                          G_CALLBACK (type_dialog_toggle_update),
                          (gpointer) (*p)->formatname);

	if (current == selected)
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), TRUE);

	p++;
	current++;
      }
  }

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  if (gimp_dialog_run (GIMP_DIALOG (dlg)) != GTK_RESPONSE_OK)
    selected_type = -1;

  gtk_widget_destroy (dlg);

  return selected_type;
}

/*
 * Callbacks for the dialog.
 */

static void
type_dialog_toggle_update (GtkWidget *widget,
			   gpointer   data)
{
  selected_type = get_type_from_string ((const gchar *) data);
}
