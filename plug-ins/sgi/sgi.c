/*
 * "$Id$"
 *
 *   SGI image file plug-in for the GIMP.
 *
 *   Copyright 1997-1998 Michael Sweet (mike@easysw.com)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contents:
 *
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the plug-in...
 *   load_image()                - Load a PNG image into a new image window.
 *   save_image()                - Save the specified image to a PNG file.
 *   save_ok_callback()          - Destroy the save dialog and save the image.
 *   save_dialog()               - Pop up the save dialog.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "sgi.h"		/* SGI image library definitions */



/*
 * Constants...
 */

#define PLUG_IN_VERSION  "1.1.1 - 17 May 1998"


/*
 * Local functions...
 */

static void	query       (void);
static void	run         (gchar   *name,
			     gint     nparams,
			     GimpParam  *param,
			     gint    *nreturn_vals,
			     GimpParam **return_vals);

static gint32	load_image  (gchar   *filename);
static gint	save_image  (gchar   *filename,
			     gint32   image_ID,
			     gint32   drawable_ID);

static gint	save_dialog (void);

/*
 * Globals...
 */

GimpPlugInInfo	PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static gint  compression = SGI_COMP_RLE;
static gint  runme       = FALSE;


MAIN ()

static void
query (void)
{
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,      "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING,     "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,     "raw_filename", "The name of the file to load" },
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,      "image",        "Output image" },
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  static GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,	"run_mode",	"Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,	"image",	"Input image" },
    { GIMP_PDB_DRAWABLE,	"drawable",	"Drawable to save" },
    { GIMP_PDB_STRING,	"filename",	"The name of the file to save the image in" },
    { GIMP_PDB_STRING,	"raw_filename",	"The name of the file to save the image in" },
    { GIMP_PDB_INT32,	"compression",	"Compression level (0 = none, 1 = RLE, 2 = ARLE)" }
  };
  static gint 	nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  INIT_I18N();

  gimp_install_procedure ("file_sgi_load",
			  "Loads files in SGI image file format",
			  "This plug-in loads SGI image files.",
			  "Michael Sweet <mike@easysw.com>",
			  "Copyright 1997-1998 by Michael Sweet",
			  PLUG_IN_VERSION,
			  "<Load>/SGI",
			  NULL,
			  GIMP_PLUGIN,
			  nload_args,
			  nload_return_vals,
			  load_args,
			  load_return_vals);

  gimp_install_procedure ("file_sgi_save",
			  "Saves files in SGI image file format",
			  "This plug-in saves SGI image files.",
			  "Michael Sweet <mike@easysw.com>",
			  "Copyright 1997-1998 by Michael Sweet",
			  PLUG_IN_VERSION,
			  "<Save>/SGI",
			  "RGB*,GRAY*",
			  GIMP_PLUGIN,
			  nsave_args,
			  0,
			  save_args,
			  NULL);

  gimp_register_magic_load_handler ("file_sgi_load",
				    "rgb,bw,sgi,icon",
				    "",
				    "0,short,474");
  gimp_register_save_handler       ("file_sgi_save",
				    "rgb,bw,sgi,icon",
				    "");
}

static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[2];
  GimpRunModeType  run_mode;       
  GimpPDBStatusType   status = GIMP_PDB_SUCCESS;
  gint32	image_ID;	
  gint32        drawable_ID;
  GimpExportReturnType export = GIMP_EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  INIT_I18N_UI();

  if (strcmp (name, "file_sgi_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[1].type         = GIMP_PDB_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_sgi_save") == 0)
    {
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */ 
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_ui_init ("sgi", FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "SGI", 
				      (GIMP_EXPORT_CAN_HANDLE_RGB |
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

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  /*
	   * Possibly retrieve data...
	   */
          gimp_get_data ("file_sgi_save", &compression);

	  /*
	   * Then acquire information with a dialog...
	   */
          if (!save_dialog ())
            status = GIMP_PDB_CANCEL;
          break;

	case GIMP_RUN_NONINTERACTIVE:
	  /*
	   * Make sure all the arguments are there!
	   */
          if (nparams != 6)
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	    }
          else
	    {
	      compression = param[5].data.d_int32;

	      if (compression < 0 || compression > 2)
		status = GIMP_PDB_CALLING_ERROR;
	    };
          break;

	case GIMP_RUN_WITH_LAST_VALS:
	  /*
	   * Possibly retrieve data...
	   */
          gimp_get_data ("file_sgi_save", &compression);
          break;

	default:
          break;
	};

      if (status == GIMP_PDB_SUCCESS)
	{
	  if (save_image (param[3].data.d_string, image_ID, drawable_ID))
	    {
	      gimp_set_data ("file_sgi_save",
			     &compression, sizeof (compression));
	    }
	  else
	    {
	      status = GIMP_PDB_EXECUTION_ERROR;
	    }
	}

      if (export == GIMP_EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}


/*
 * 'load_image()' - Load a PNG image into a new image window.
 */

static gint32
load_image (gchar *filename)	/* I - File to load */
{
  int		i,		/* Looping var */
		x,		/* Current X coordinate */
		y,		/* Current Y coordinate */
		image_type,	/* Type of image */
		layer_type,	/* Type of drawable/layer */
		tile_height,	/* Height of tile in GIMP */
		count;		/* Count of rows to put in image */
  sgi_t		*sgip;		/* File pointer */
  gint32	image,		/* Image */
		layer;		/* Layer */
  GimpDrawable	*drawable;	/* Drawable for layer */
  GimpPixelRgn	pixel_rgn;	/* Pixel region for layer */
  guchar	**pixels,	/* Pixel rows */
		*pixel,		/* Pixel data */
		*pptr;		/* Current pixel */
  gushort      **rows;	        /* SGI image data */
  gchar		*progress;	/* Title for progress display... */


 /*
  * Open the file for reading...
  */

  sgip = sgiOpen(filename, SGI_READ, 0, 0, 0, 0, 0);
  if (sgip == NULL)
    {
      g_message ("can't open image file\n");
      return -1;
    };

  if (strrchr(filename, '/') != NULL)
    progress = g_strdup_printf (_("Loading %s:"), strrchr(filename, '/') + 1);
  else
    progress = g_strdup_printf (_("Loading %s:"), filename);

  gimp_progress_init(progress);
  g_free (progress);

  /*
   * Get the image dimensions and create the image...
   */
  image_type = 0; /* shut up warnings */
  layer_type = 0;
  switch (sgip->zsize)
    {
    case 1 :	/* Grayscale */
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAY_IMAGE;
      break;

    case 2 :	/* Grayscale + alpha */
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAYA_IMAGE;
      break;

    case 3 :	/* RGB */
      image_type = GIMP_RGB;
      layer_type = GIMP_RGB_IMAGE;
      break;

    case 4 :	/* RGBA */
      image_type = GIMP_RGB;
      layer_type = GIMP_RGBA_IMAGE;
      break;
    }

  image = gimp_image_new(sgip->xsize, sgip->ysize, image_type);
  if (image == -1)
    {
      g_message ("can't allocate new image\n");
      return -1;
    }

  gimp_image_set_filename(image, filename);

  /*
   * Create the "background" layer to hold the image...
   */

  layer = gimp_layer_new (image, _("Background"), sgip->xsize, sgip->ysize,
			  layer_type, 100, GIMP_NORMAL_MODE);
  gimp_image_add_layer(image, layer, 0);

  /*
   * Get the drawable and set the pixel region for our load...
   */

  drawable = gimp_drawable_get(layer);

  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, TRUE, FALSE);

  /*
   * Temporary buffers...
   */

  tile_height = gimp_tile_height();
  pixel       = g_new(guchar, tile_height * sgip->xsize * sgip->zsize);
  pixels      = g_new(guchar *, tile_height);

  for (i = 0; i < tile_height; i ++)
    pixels[i] = pixel + sgip->xsize * sgip->zsize * i;

  rows    = g_new(unsigned short *, sgip->zsize);
  rows[0] = g_new(unsigned short, sgip->xsize * sgip->zsize);

  for (i = 1; i < sgip->zsize; i ++)
    rows[i] = rows[0] + i * sgip->xsize;

  /*
   * Load the image...
   */

  for (y = 0, count = 0;
       y < sgip->ysize;
       y ++, count ++)
    {
      if (count >= tile_height)
	{
	  gimp_pixel_rgn_set_rect (&pixel_rgn, pixel,
				   0, y - count, drawable->width, count);
	  count = 0;

	  gimp_progress_update((double)y / (double)sgip->ysize);
	}

      for (i = 0; i < sgip->zsize; i ++)
	if (sgiGetRow(sgip, rows[i], sgip->ysize - 1 - y, i) < 0)
	  printf("sgiGetRow(sgip, rows[i], %d, %d) failed!\n",
		 sgip->ysize - 1 - y, i);

      if (sgip->bpp == 1)
	{
	  /*
	   * 8-bit (unsigned) pixels...
	   */

	  for (x = 0, pptr = pixels[count]; x < sgip->xsize; x ++)
	    for (i = 0; i < sgip->zsize; i ++, pptr ++)
	      *pptr = rows[i][x];
	}
      else
	{
	  /*
	   * 16-bit (unsigned) pixels...
	   */

	  for (x = 0, pptr = pixels[count]; x < sgip->xsize; x ++)
	    for (i = 0; i < sgip->zsize; i ++, pptr ++)
	      *pptr = rows[i][x] >> 8;
	}
    }

  /*
   * Do the last n rows (count always > 0)
   */

  gimp_pixel_rgn_set_rect (&pixel_rgn, pixel, 0,
			   y - count, drawable->width, count);

  /*
   * Done with the file...
   */

  sgiClose (sgip);

  g_free (pixel);
  g_free (pixels);
  g_free (rows[0]);
  g_free (rows);

  /*
   * Update the display...
   */

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return (image);
}


/*
 * 'save_image()' - Save the specified image to a SGI file.
 */

static gint
save_image (gchar  *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  gint        i, j,        /* Looping var */
              x,           /* Current X coordinate */
              y,           /* Current Y coordinate */
              tile_height, /* Height of tile in GIMP */
              count,       /* Count of rows to put in image */
              zsize;       /* Number of channels in file */
  sgi_t	     *sgip;        /* File pointer */
  GimpDrawable  *drawable;    /* Drawable for layer */
  GimpPixelRgn   pixel_rgn;   /* Pixel region for layer */
  guchar    **pixels,      /* Pixel rows */
             *pixel,       /* Pixel data */
             *pptr;        /* Current pixel */
  gushort   **rows;        /* SGI image data */
  gchar      *progress;    /* Title for progress display... */

  /*
   * Get the drawable for the current image...
   */

  drawable = gimp_drawable_get (drawable_ID);

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width,
		       drawable->height, FALSE, FALSE);

  zsize = 0;
  switch (gimp_drawable_type(drawable_ID))
    {
    case GIMP_GRAY_IMAGE :
      zsize = 1;
      break;
    case GIMP_GRAYA_IMAGE :
      zsize = 2;
      break;
    case GIMP_RGB_IMAGE :
      zsize = 3;
      break;
    case GIMP_RGBA_IMAGE :
      zsize = 4;
      break;
    default:
      g_message ("SGI: Image must be of type RGB or GRAY\n");
      return FALSE;
      break;
    }

  /*
   * Open the file for writing...
   */

  sgip = sgiOpen (filename, SGI_WRITE, compression, 1, drawable->width,
		  drawable->height, zsize);
  if (sgip == NULL)
    {
      g_message ("SGI: Can't create image file\n");
      return FALSE;
    };

  if (strrchr(filename, '/') != NULL)
    progress = g_strdup_printf (_("Saving %s:"), strrchr(filename, '/') + 1);
  else
    progress = g_strdup_printf (_("Saving %s:"), filename);

  gimp_progress_init(progress);
  g_free (progress);

  /*
   * Allocate memory for "tile_height" rows...
   */

  tile_height = gimp_tile_height();
  pixel       = g_new(guchar, tile_height * drawable->width * zsize);
  pixels      = g_new(guchar *, tile_height);

  for (i = 0; i < tile_height; i ++)
    pixels[i]= pixel + drawable->width * zsize * i;

  rows    = g_new (gushort *, sgip->zsize);
  rows[0] = g_new (gushort, sgip->xsize * sgip->zsize);

  for (i = 1; i < sgip->zsize; i ++)
    rows[i] = rows[0] + i * sgip->xsize;

  /*
   * Save the image...
   */

  for (y = 0; y < drawable->height; y += count)
    {
      /*
       * Grab more pixel data...
       */

      if ((y + tile_height) >= drawable->height)
	count = drawable->height - y;
      else
	count = tile_height;

      gimp_pixel_rgn_get_rect (&pixel_rgn, pixel, 0, y, drawable->width, count);

      /*
       * Convert to shorts and write each color plane separately...
       */

      for (i = 0, pptr = pixels[0]; i < count; i ++)
	{
	  for (x = 0; x < drawable->width; x ++)
	    for (j = 0; j < zsize; j ++, pptr ++)
	      rows[j][x] = *pptr;

	  for (j = 0; j < zsize; j ++)
	    sgiPutRow (sgip, rows[j], drawable->height - 1 - y - i, j);
	};

      gimp_progress_update ((double) y / (double) drawable->height);
    };

  /*
   * Done with the file...
   */

  sgiClose (sgip);

  g_free (pixel);
  g_free (pixels);
  g_free (rows[0]);
  g_free (rows);

  return TRUE;
}

static void
save_ok_callback (GtkWidget *widget,
                  gpointer   data)
{
  runme = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static gint
save_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;

  dlg = gimp_dialog_new (_("Save as SGI"), "sgi",
			 gimp_standard_help_func, "filters/sgi.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, FALSE, FALSE,

			 _("OK"), save_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  frame = gimp_radio_group_new2 (TRUE, _("Compression Type"),
				 gimp_radio_button_update,
				 &compression, (gpointer) compression,

				 _("No Compression"),
				 (gpointer) SGI_COMP_NONE, NULL,
				 _("RLE Compression"),
				 (gpointer) SGI_COMP_RLE, NULL,
				 _("Aggressive RLE\n(Not Supported by SGI)"),
				 (gpointer) SGI_COMP_ARLE, NULL,

				 NULL);

  gtk_container_set_border_width (GTK_CONTAINER(frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return runme;
}
