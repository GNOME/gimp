/*
 * "$Id$"
 *
 *   Portable Network Graphics (PNG) plug-in for The GIMP -- an image
 *   manipulation program
 *
 *   Copyright 1997-1998 Michael Sweet (mike@easysw.com) and
 *   Daniel Skarda (0rfelyus@atrey.karlin.mff.cuni.cz).
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
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
 *   save_close_callback()       - Close the save dialog window.
 *   save_ok_callback()          - Destroy the save dialog and save the image.
 *   save_compression_callback() - Update the image compression level.
 *   save_interlace_update()     - Update the interlacing option.
 *   save_dialog()               - Pop up the save dialog.
 *
 * Revision History:
 *
 *   see ChangeLog
 */

#include <stdio.h>
#include <stdlib.h>

#include <png.h>		/* PNG library definitions */

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "config.h"
#include "libgimp/stdplugins-intl.h"

/*
 * Constants...
 */

#define PLUG_IN_VERSION		"1.1.9 - 2 Oct 1999"
#define SCALE_WIDTH		125

#define DEFAULT_GAMMA		2.20

/*
 * Structures...
 */

typedef struct
{
  gint	interlaced;
  gint	compression_level;
} PngSaveVals;


/*
 * Local functions...
 */

static void	query                     (void);
static void	run                       (char *, int, GParam *, int *, GParam **);
static gint32	load_image                (char *);
static gint	save_image                (char *, gint32, gint32, gint32);
static void     init_gtk                  (void);
static gint	save_dialog               (void);
static void	save_close_callback       (GtkWidget *, gpointer);
static void	save_ok_callback          (GtkWidget *, gpointer);
static void	save_compression_update   (GtkAdjustment *, gpointer);
static void	save_interlace_update     (GtkWidget *, gpointer);


/*
 * Globals...
 */

GPlugInInfo	PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

PngSaveVals	pngvals = 
{
  FALSE,
  6
};

int		runme = FALSE;


/*
 * 'main()' - Main entry - just call gimp_main()...
 */

MAIN()

/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query (void)
{
  static GParamDef	load_args[] =
  {
    { PARAM_INT32,      "run_mode",     "Interactive, non-interactive" },
    { PARAM_STRING,     "filename",     "The name of the file to load" },
    { PARAM_STRING,     "raw_filename", "The name of the file to load" },
  };
  static GParamDef	load_return_vals[] =
  {
    { PARAM_IMAGE,      "image",        "Output image" },
  };
  static int		nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int		nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);
  static GParamDef	save_args[] =
  {
    { PARAM_INT32,	"run_mode",	"Interactive, non-interactive" },
    { PARAM_IMAGE,	"image",	"Input image" },
    { PARAM_DRAWABLE,	"drawable",	"Drawable to save" },
    { PARAM_STRING,	"filename",	"The name of the file to save the image in" },
    { PARAM_STRING,	"raw_filename",	"The name of the file to save the image in" },
    { PARAM_INT32,	"interlace",	"Save with interlacing option enabled" },
    { PARAM_INT32,	"compression",	"Compression level" }
  };
  static int		nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  INIT_I18N ();

  gimp_install_procedure ("file_png_load",
      _("Loads files in PNG file format"),
      _("This plug-in loads Portable Network Graphics (PNG) files."),
      "Michael Sweet <mike@easysw.com>, Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
      "Michael Sweet <mike@easysw.com>, Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
      PLUG_IN_VERSION,
      "<Load>/PNG", NULL, PROC_PLUG_IN, nload_args, nload_return_vals,
      load_args, load_return_vals);

  gimp_install_procedure ("file_png_save",
      "Saves files in PNG file format",
      "This plug-in saves Portable Network Graphics (PNG) files.",
      "Michael Sweet <mike@easysw.com>, Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
      "Michael Sweet <mike@easysw.com>, Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
      PLUG_IN_VERSION,
      "<Save>/PNG", "RGB*,GRAY*,INDEXED", PROC_PLUG_IN, nsave_args, 0, save_args, NULL);

  gimp_register_magic_load_handler ("file_png_load", "png", "", "0,string,\211PNG\r\n\032\n");
  gimp_register_save_handler       ("file_png_save", "png", "");
}


/*
 * 'run()' - Run the plug-in...
 */

static void
run (char    *name,		/* I - Name of filter program. */
     int      nparams,		/* I - Number of parameters passed in */
     GParam  *param,		/* I - Parameter values */
     int     *nreturn_vals,	/* O - Number of return values */
     GParam **return_vals)	/* O - Return values */
{
  gint32	 image_ID;
  gint32         drawable_ID;
  gint32         orig_image_ID;
  GRunModeType   run_mode;
  GParam	*values;	/* Return values */
  GimpExportReturnType export = EXPORT_CANCEL;

 /*
  * Initialize parameter data...
  */

  values = g_new (GParam, 2);

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_SUCCESS;

  *return_vals  = values;

 /*
  * Load or save an image...
  */

  if (strcmp (name, "file_png_load") == 0)
  {
    INIT_I18N ();

    *nreturn_vals = 2;

    image_ID = load_image (param[1].data.d_string);

    if (image_ID != -1)
    {
      values[1].type         = PARAM_IMAGE;
      values[1].data.d_image = image_ID;
    }
    else
      values[0].data.d_status = STATUS_EXECUTION_ERROR;
  }
  else if (strcmp (name, "file_png_save") == 0)
  {
    INIT_I18N_UI();

    run_mode = param[0].data.d_int32;
    image_ID = orig_image_ID = param[1].data.d_int32;
    drawable_ID = param[2].data.d_int32;
    *nreturn_vals = 1;
    
    /*  eventually export the image */ 
    switch (run_mode)
      {
      case RUN_INTERACTIVE:
      case RUN_WITH_LAST_VALS:
	init_gtk ();
	export = gimp_export_image (&image_ID, &drawable_ID, "PNG", 
				    (CAN_HANDLE_RGB | CAN_HANDLE_GRAY | CAN_HANDLE_INDEXED | CAN_HANDLE_ALPHA));
	if (export == EXPORT_CANCEL)
	  {
	    *nreturn_vals = 1;
	    values[0].data.d_status = STATUS_EXECUTION_ERROR;
	    return;
	  }
	break;
      default:
	break;
      }

    switch (run_mode)
    {
      case RUN_INTERACTIVE :
         /*
          * Possibly retrieve data...
          */

          gimp_get_data ("file_png_save", &pngvals);

         /*
          * Then acquire information with a dialog...
          */

          if (!save_dialog())
            return;
          break;

      case RUN_NONINTERACTIVE :
         /*
          * Make sure all the arguments are there!
          */

          if (nparams != 7)
            values[0].data.d_status = STATUS_CALLING_ERROR;
          else
          {
            pngvals.interlaced        = param[5].data.d_int32;
            pngvals.compression_level = param[6].data.d_int32;

            if (pngvals.compression_level < 0 ||
                pngvals.compression_level > 9)
              values[0].data.d_status = STATUS_CALLING_ERROR;
          };
          break;

      case RUN_WITH_LAST_VALS :
         /*
          * Possibly retrieve data...
          */
          gimp_get_data ("file_png_save", &pngvals);
          break;

      default :
          break;
    };

    if (values[0].data.d_status == STATUS_SUCCESS)
      {
	if (save_image (param[3].data.d_string, image_ID, drawable_ID, orig_image_ID))
	  gimp_set_data ("file_png_save", &pngvals, sizeof (pngvals));
	else
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
      };

    if (export == EXPORT_EXPORT)
      gimp_image_delete (image_ID);
  }
  else
    values[0].data.d_status = STATUS_EXECUTION_ERROR;
}


/*
 * 'load_image()' - Load a PNG image into a new image window.
 */

static gint32
load_image (char *filename)	/* I - File to load */
{
  int		i,		/* Looping var */
                trns = 0,       /* Transparency present */
		bpp = 0,	/* Bytes per pixel */
		image_type = 0,	/* Type of image */
		layer_type = 0,	/* Type of drawable/layer */
		num_passes,	/* Number of interlace passes in file */
		pass,		/* Current pass in file */
		tile_height,	/* Height of tile in GIMP */
		begin,		/* Beginning tile row */
		end,		/* Ending tile row */
		num;		/* Number of rows to load */
  FILE		*fp;		/* File pointer */
  gint32	image = -1,	/* Image */
		layer;		/* Layer */
  GDrawable	*drawable;	/* Drawable for layer */
  GPixelRgn	pixel_rgn;	/* Pixel region for layer */
  png_structp	pp;		/* PNG read pointer */
  png_infop	info;		/* PNG info pointers */
  guchar	**pixels,	/* Pixel rows */
		*pixel;		/* Pixel data */
  char		progress[255];	/* Title for progress display... */
  guchar	alpha[256],	/* Index -> Alpha */
  		*alpha_ptr;	/* Temporary pointer */

 /*
  * Setup the PNG data structures...
  */

#if PNG_LIBPNG_VER > 88
 /*
  * Use the "new" calling convention...
  */

  pp   = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info = png_create_info_struct(pp);
#else
 /*
  * SGI (and others) supply libpng-88 and not -89c...
  */

  pp = (png_structp)calloc(sizeof(png_struct), 1);
  png_read_init(pp);

  info = (png_infop)calloc(sizeof(png_info), 1);
#endif /* PNG_LIBPNG_VER > 88 */

  if (setjmp (pp->jmpbuf))
  {
    g_message ("%s\nPNG error. File corrupted?", filename);
    return image;
  }

 /*
  * Open the file and initialize the PNG read "engine"...
  */

  fp = fopen(filename, "rb");

  if (fp == NULL) 
    {
      g_message ("%s\nis not present or is unreadable", filename);
      gimp_quit ();
  }

  png_init_io(pp, fp);

  if (strrchr(filename, '/') != NULL)
    sprintf (progress, _("Loading %s:"), strrchr(filename, '/') + 1);
  else
    sprintf (progress, _("Loading %s:"), filename);

  gimp_progress_init(progress);

 /*
  * Get the image dimensions and create the image...
  */

  png_read_info(pp, info);

 /*
  * This is a new version with support for tRNS (alpha + colourmaps)
  * Mail me if you have trouble -- njl195@zepler.org.uk
  */

  if (info->bit_depth < 8) 
  {
    png_set_packing(pp);
    if (info->color_type != PNG_COLOR_TYPE_PALETTE) {
      png_set_expand(pp);
    }
  }
  else if (info->bit_depth == 16)
    png_set_strip_16(pp);

 /*
  * Turn on interlace handling...
  */

  if (info->interlace_type)
    num_passes = png_set_interlace_handling(pp);
  else
    num_passes = 1;

 /*
  * Special handling for INDEXED + tRNS (transparency palette)
  */

  if (info->valid & PNG_INFO_tRNS &&
      info->color_type == PNG_COLOR_TYPE_PALETTE)
  {
    png_get_tRNS(pp, info, &alpha_ptr, &num, NULL);
    /* Copy the existing alpha values from the tRNS chunk */
    for (i= 0; i < num; ++i)
      alpha[i]= alpha_ptr[i];
    /* And set any others to fully opaque (255)  */
    for (i= num; i < 256; ++i)
      alpha[i]= 255;
    trns= 1;
  }

 /*
  * Update the info structures after the transformations take effect
  */

  png_read_update_info(pp, info);
  
  switch (info->color_type)
  {
    case PNG_COLOR_TYPE_RGB :		/* RGB */
        bpp        = 3;
        image_type = RGB;
        layer_type = RGB_IMAGE;
        break;

    case PNG_COLOR_TYPE_RGB_ALPHA :	/* RGBA */
        bpp        = 4;
        image_type = RGB;
        layer_type = RGBA_IMAGE;
        break;

    case PNG_COLOR_TYPE_GRAY :		/* Grayscale */
        bpp        = 1;
        image_type = GRAY;
        layer_type = GRAY_IMAGE;
        break;

    case PNG_COLOR_TYPE_GRAY_ALPHA :	/* Grayscale + alpha */
        bpp        = 2;
        image_type = GRAY;
        layer_type = GRAYA_IMAGE;
        break;

    case PNG_COLOR_TYPE_PALETTE :	/* Indexed */
        bpp        = 1;
        image_type = INDEXED;
        layer_type = INDEXED_IMAGE;
        break;
  };

  image = gimp_image_new(info->width, info->height, image_type);
  if (image == -1)
  {
    g_message("Can't allocate new image\n%s", filename);
    gimp_quit();
  };

  /*
   * Find out everything we can about the image resolution
   */

#ifdef GIMP_HAVE_RESOLUTION_INFO
  if (info->valid & PNG_INFO_pHYs)
    {
      if (info->phys_unit_type == PNG_RESOLUTION_METER)
	gimp_image_set_resolution(image,
				  ((double) info->x_pixels_per_unit) * 0.0254,
				  ((double) info->y_pixels_per_unit) * 0.0254);
      else  /*  set aspect ratio as resolution  */
	gimp_image_set_resolution(image,
				  ((double) info->x_pixels_per_unit),
				  ((double) info->y_pixels_per_unit));
    }
#endif /* GIMP_HAVE_RESOLUTION_INFO */

  gimp_image_set_filename(image, filename);

 /*
  * Load the colormap as necessary...
  */

  if (info->color_type & PNG_COLOR_MASK_PALETTE)
    gimp_image_set_cmap(image, (guchar *)info->palette, info->num_palette);

 /*
  * Create the "background" layer to hold the image...
  */

  layer = gimp_layer_new(image, _("Background"), info->width, info->height,
                         layer_type, 100, NORMAL_MODE);
  gimp_image_add_layer(image, layer, 0);

 /*
  * Get the drawable and set the pixel region for our load...
  */

  drawable = gimp_drawable_get(layer);

  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, TRUE, FALSE);

 /*
  * Temporary buffer...
  */

  tile_height = gimp_tile_height ();
  pixel       = g_new(guchar, tile_height * info->width * bpp);
  pixels      = g_new(guchar *, tile_height);

  for (i = 0; i < tile_height; i ++)
    pixels[i] = pixel + info->width * info->channels * i;

  for (pass = 0; pass < num_passes; pass ++)
  {
   /*
    * This works if you are only reading one row at a time...
    */

    for (begin = 0, end = tile_height;
         begin < info->height;
         begin += tile_height, end += tile_height)
    {
      if (end > info->height)
        end = info->height;

      num = end - begin;
	
      if (pass != 0) /* to handle interlaced PiNGs */
        gimp_pixel_rgn_get_rect(&pixel_rgn, pixel, 0, begin,
                                drawable->width, num);

      png_read_rows(pp, pixels, NULL, num);

      gimp_pixel_rgn_set_rect(&pixel_rgn, pixel, 0, begin,
                              drawable->width, num);

      gimp_progress_update(((double)pass + (double)end / (double)info->height) /
                           (double)num_passes);
    };
  };

 /*
  * Done with the file...
  */

  png_read_end(pp, info);
  png_read_destroy(pp, info, NULL);

  g_free(pixel);
  g_free(pixels);
  free(pp);
  free(info);

  fclose(fp);


  if (trns) {
    gimp_layer_add_alpha(layer);
    drawable = gimp_drawable_get(layer);
    gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                        drawable->height, TRUE, FALSE);

    pixel  = g_new(guchar, tile_height * drawable->width * 2); /* bpp == 1 */

    for (begin = 0, end = tile_height;
         begin < drawable->height;
         begin += tile_height, end += tile_height)
    {
      if (end > drawable->height) end = drawable->height;
      num= end - begin;

      gimp_pixel_rgn_get_rect(&pixel_rgn, pixel, 0, begin,
                                drawable->width, num);

      for (i= 0; i < tile_height * drawable->width; ++i) {
        pixel[i*2+1]= alpha [ pixel[i*2] ];
      }

      gimp_pixel_rgn_set_rect(&pixel_rgn, pixel, 0, begin,
                              drawable->width, num);
    }
    g_free(pixel);
  }

 /*
  * Update the display...
  */

  gimp_drawable_flush(drawable);
  gimp_drawable_detach(drawable);

  return (image);
}


/*
 * 'save_image ()' - Save the specified image to a PNG file.
 */

static gint
save_image (char   *filename,	        /* I - File to save to */
	    gint32  image_ID,	        /* I - Image to save */
	    gint32  drawable_ID,	/* I - Current drawable */
	    gint32  orig_image_ID)      /* I - Original image before export */
{
  int		i,		/* Looping var */
		bpp = 0,	/* Bytes per pixel */
		type,		/* Type of drawable/layer */
		num_passes,	/* Number of interlace passes in file */
		pass,		/* Current pass in file */
		tile_height,	/* Height of tile in GIMP */
		begin,		/* Beginning tile row */
		end,		/* Ending tile row */
		num;		/* Number of rows to load */
  FILE		*fp;		/* File pointer */
  GDrawable	*drawable;	/* Drawable for layer */
  GPixelRgn	pixel_rgn;	/* Pixel region for layer */
  png_structp	pp;		/* PNG read pointer */
  png_infop	info;		/* PNG info pointer */
  gint		num_colors;	/* Number of colors in colormap */
  guchar	**pixels,	/* Pixel rows */
		*pixel;		/* Pixel data */
  char		progress[255];	/* Title for progress display... */
  gdouble       xres, yres;	/* GIMP resolution (dpi) */
  gdouble	gamma;

 /*
  * Setup the PNG data structures...
  */

#if PNG_LIBPNG_VER > 88
 /*
  * Use the "new" calling convention...
  */

  pp   = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info = png_create_info_struct (pp);
#else
 /*
  * SGI (and others) supply libpng-88 and not -89c...
  */

  pp = (png_structp)calloc(sizeof(png_struct), 1);
  png_write_init(pp);

  info = (png_infop)calloc(sizeof(png_info), 1);
  png_info_init(info);
#endif /* PNG_LIBPNG_VER > 88 */

 /*
  * Open the file and initialize the PNG write "engine"...
  */

  fp = fopen(filename, "wb");
  if (fp == NULL)
    return (0);

  png_init_io(pp, fp);

  if (strrchr(filename, '/') != NULL)
    sprintf(progress, _("Saving %s:"), strrchr(filename, '/') + 1);
  else
    sprintf(progress, _("Saving %s:"), filename);

  gimp_progress_init(progress);

 /*
  * Get the drawable for the current image...
  */

  drawable = gimp_drawable_get (drawable_ID);
  type     = gimp_drawable_type (drawable_ID);

  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, FALSE, FALSE);

 /*
  * Set the image dimensions and save the image...
  */

  png_set_compression_level (pp, pngvals.compression_level);

  gamma = gimp_gamma();

  info->width          = drawable->width;
  info->height         = drawable->height;
  info->bit_depth      = 8;
  info->gamma          = 1.0 / (gamma != 1.00 ? gamma : DEFAULT_GAMMA);
  info->sig_bit.red    = 8;
  info->sig_bit.green  = 8;
  info->sig_bit.blue   = 8;
  info->sig_bit.gray   = 8;
  info->sig_bit.alpha  = 8;
  info->interlace_type = pngvals.interlaced;
  info->valid          |= PNG_INFO_gAMA;

#ifdef GIMP_HAVE_RESOLUTION_INFO
  info->valid |= PNG_INFO_pHYs;
  gimp_image_get_resolution (orig_image_ID, &xres, &yres);
  info->phys_unit_type = PNG_RESOLUTION_METER;
  info->x_pixels_per_unit = (int) (xres * 39.37);
  info->y_pixels_per_unit = (int) (yres * 39.37);
#endif /* GIMP_HAVE_RESOLUTION_INFO */
 
  switch (type)
  {
    case RGB_IMAGE :
        info->color_type = PNG_COLOR_TYPE_RGB;
        bpp              = 3;
        break;
    case RGBA_IMAGE :
        info->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        bpp              = 4;
        break;
    case GRAY_IMAGE :
        info->color_type = PNG_COLOR_TYPE_GRAY;
        bpp              = 1;
        break;
    case GRAYA_IMAGE :
        info->color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
        bpp              = 2;
        break;
    case INDEXED_IMAGE :
	bpp		 = 1;
	info->valid      |= PNG_INFO_PLTE;
        info->color_type = PNG_COLOR_TYPE_PALETTE;
        info->palette    = (png_colorp)gimp_image_get_cmap (image_ID, &num_colors);
        info->num_palette= num_colors;
        break;
    case INDEXEDA_IMAGE :
	g_message (_("Can't save image with alpha\n"));
	return 0;
	break;

    default:
        abort ();
  };

  png_write_info (pp, info);

 /*
  * Turn on interlace handling...
  */

  if (pngvals.interlaced)
    num_passes = png_set_interlace_handling(pp);
  else
     num_passes = 1;

 /*
  * Allocate memory for "tile_height" rows and save the image...
  */

  tile_height = gimp_tile_height();
  pixel       = g_new(guchar, tile_height * drawable->width * bpp);
  pixels      = g_new(guchar *, tile_height);

  for (i = 0; i < tile_height; i ++)
    pixels[i]= pixel + drawable->width * bpp * i;

  for (pass = 0; pass < num_passes; pass ++)
  {
   /*
    * This works if you are only writing one row at a time...
    */

    for (begin = 0, end = tile_height;
         begin < drawable->height;
         begin += tile_height, end += tile_height)
      {
	if (end > drawable->height)
	  end = drawable->height;

	num = end - begin;
	
	gimp_pixel_rgn_get_rect (&pixel_rgn, pixel, 0, begin, drawable->width, num);
	
	png_write_rows (pp, pixels, num);
	
	gimp_progress_update (((double)pass + (double)end / (double)info->height) /
			      (double)num_passes);
      };
  };

  png_write_end (pp, info);
  png_write_destroy (pp);

  g_free (pixel);
  g_free (pixels);

 /*
  * Done with the file...
  */

  free (pp);
  free (info);

  fclose (fp);

  return (1);
}


/*
 * 'save_close_callback()' - Close the save dialog window.
 */

static void
save_close_callback(GtkWidget *widget,	/* I - Close button */
                    gpointer  data)	/* I - Callback data */
{
  gtk_main_quit();
}


/*
 * 'save_ok_callback()' - Destroy the save dialog and save the image.
 */

static void
save_ok_callback (GtkWidget *widget,	/* I - OK button */
		  gpointer  data)	/* I - Callback data */
{
  runme = TRUE;

  gtk_widget_destroy(GTK_WIDGET(data));
}


/*
 * 'save_compression_callback()' - Update the image compression level.
 */

static void
save_compression_update (GtkAdjustment *adjustment,	/* I - Scale adjustment */
                         gpointer      data)		/* I - Callback data */
{
  pngvals.compression_level = (gint32)adjustment->value;
}


/*
 * 'save_interlace_update()' - Update the interlacing option.
 */

static void
save_interlace_update (GtkWidget *widget,	/* I - Interlace toggle button */
		       gpointer  data)		/* I - Callback data  */
{
  pngvals.interlaced = GTK_TOGGLE_BUTTON (widget)->active;
}

static void 
init_gtk ()
{
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("png");
  
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
}


/*
 * 'save_dialog()' - Pop up the save dialog.
 */

static gint
save_dialog (void)
{
  GtkWidget	*dlg,		/* Dialog window */
		*button,	/* OK/cancel buttons */
		*frame,		/* Frame for dialog */
		*table,		/* Table for dialog options */
		*toggle,	/* Interlace toggle button */
		*label,		/* Label for controls */
		*scale;		/* Compression level scale */
  GtkObject	*scale_data;	/* Scale data */

 /*
  * Open a dialog window...
  */

  dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), _("PNG Options"));
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     (GtkSignalFunc)save_close_callback, NULL);

 /*
  * OK/cancel buttons...
  */

  button = gtk_button_new_with_label(_("OK"));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT (button), "clicked",
                     (GtkSignalFunc)save_ok_callback,
                     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            (GtkSignalFunc)gtk_widget_destroy, GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show(button);

 /*
  * Compression level, interlacing controls...
  */

  frame = gtk_frame_new(_("Parameter Settings"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX (GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new(2, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 10);
  gtk_container_add(GTK_CONTAINER(frame), table);

  toggle = gtk_check_button_new_with_label(_("Interlace"));
  gtk_table_attach(GTK_TABLE(table), toggle, 0, 2, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
                     (GtkSignalFunc)save_interlace_update, NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), pngvals.interlaced);
  gtk_widget_show(toggle);

  label = gtk_label_new(_("Compression level"));
  gtk_misc_set_alignment(GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);

  scale_data = gtk_adjustment_new(pngvals.compression_level, 1.0, 9.0, 1.0, 1.0, 0.0);
  scale      = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
  gtk_table_attach(GTK_TABLE(table), scale, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits(GTK_SCALE (scale), 1);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
                     (GtkSignalFunc)save_compression_update, NULL);
  gtk_widget_show(label);
  gtk_widget_show(scale);
  gtk_widget_show(table);
  gtk_widget_show(frame);
  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return (runme);
}

/*
 * End of "$Id$".
 */
