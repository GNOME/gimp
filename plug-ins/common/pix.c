/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Alias|Wavefront pix/matte image reading and writing code 
 * Copyright (C) 1997 Mike Taylor
 * (email: mtaylor@aw.sgi.com, WWW: http://reality.sgi.com/mtaylor)
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/* This plug-in was written using the online documentation from 
 * Alias|Wavefront Inc's PowerAnimator product.
 *
 * Bug reports or suggestions should be e-mailed to mtaylor@aw.sgi.com
 */

/* Event history:
 * V 1.0, MT, 02-Jul-97: initial version of plug-in
 * V 1.1, MT, 04-Dec-97: added .als file extension 
 */

/* Features
 *  - loads and saves
 *    - 24-bit (.pix) 
 *    - 8-bit (.matte, .alpha, or .mask) images
 *
 * NOTE: pix and matte files do not support alpha channels or indexed
 *       colour, so neither does this plug-in
 */

static char ident[] = "@(#) GIMP Alias|Wavefront pix image file-plugin v1.0  24-jun-97";

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* #define PIX_DEBUG */

#ifdef PIX_DEBUG
#    define PIX_DEBUG_PRINT(a,b) fprintf(stderr,a,b)
#else
#    define PIX_DEBUG_PRINT(a,b) 
#endif

/**************
 * Prototypes *
 **************/

/* Standard Plug-in Functions */

static void     query     (void);
static void     run       (gchar    *name,
			   gint      nparams,
			   GParam   *param, 
			   gint     *nreturn_vals,
			   GParam  **return_vals);

/* Local Helper Functions */ 

static gint32   load_image (gchar   *filename);
static gint     save_image (gchar   *filename,
			    gint32   image_ID,
			    gint32   drawable_ID);

static guint16  get_short  (FILE    *file);
static void     put_short  (guint16  value,
			    FILE    *file);

/******************
 * Implementation *
 ******************/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  /*
   * Description:
   *     Register the services provided by this plug-in 
   */
  static GParamDef load_args[] = 
  {
    { PARAM_INT32,  "run_mode",      "Interactive, non-interactive" },
    { PARAM_STRING, "filename",      "The name of the file to load" },
    { PARAM_STRING, "raw_filename",   "The name entered" }
  };
  static GParamDef load_return_vals[] = 
  {
    { PARAM_IMAGE, "image", "Output image" }
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  static GParamDef save_args[] = 
  {
    { PARAM_INT32,    "run_mode",     "Interactive, non-interactive" },
    { PARAM_IMAGE,    "image",        "Input image" },
    { PARAM_DRAWABLE, "drawable",     "Drawable to save" },
    { PARAM_STRING,   "filename",     "The name of the file to save the image in" },
    { PARAM_STRING,   "raw_filename", "The name of the file to save the image in" }
  };
  static gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_pix_load",
			  "loads files of the PIX file format",
			  "loads files of the PIX file format",
			  "Michael Taylor",
			  "Michael Taylor",
			  "1997",
			  "<Load>/PIX",
			  NULL,
			  PROC_PLUG_IN,
			  nload_args, nload_return_vals,
			  load_args, load_return_vals);

  gimp_install_procedure ("file_pix_save",
                          "save file in the Alias|Wavefront pix/matte file format",
                          "save file in the Alias|Wavefront pix/matte file format",
                          "Michael Taylor",
                          "Michael Taylor",
                          "1997",
                          "<Save>/PIX",
                          "RGB*, GRAY*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_load_handler ("file_pix_load",
			      "pix,matte,mask,alpha,als",
			      "");
  gimp_register_save_handler ("file_pix_save",
			      "pix,matte,mask,alpha,als",
			      "");
}

static void 
run (gchar   *name, 
     gint     nparams, 
     GParam  *param, 
     gint    *nreturn_vals,
     GParam **return_vals)
{
  /* 
   *  Description:
   *      perform registered plug-in function 
   *
   *  Arguments:
   *      name         - name of the function to perform
   *      nparams      - number of parameters passed to the function
   *      param        - parameters passed to the function
   *      nreturn_vals - number of parameters returned by the function
   *      return_vals  - parameters returned by the function
   */
  static GParam values[2];
  GRunModeType  run_mode;
  GStatusType   status = STATUS_SUCCESS;
  gint32        image_ID;
  gint32        drawable_ID;
  GimpExportReturnType export = EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_EXECUTION_ERROR;

  if (strcmp (name, "file_pix_load") == 0) 
    {
      INIT_I18N();
      /* Perform the image load */
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1) 
	{
	  /* The image load was successful */
	  *nreturn_vals = 2;
	  values[1].type         = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;
	} 
      else 
	{
	  /* The image load falied */
	  status = STATUS_EXECUTION_ERROR;
	}
    } 
  else if (strcmp (name, "file_pix_save") == 0) 
    {
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */ 
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	case RUN_WITH_LAST_VALS:
	  INIT_I18N_UI();
	  gimp_ui_init ("pix", FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "PIX", 
				      (CAN_HANDLE_RGB |
				       CAN_HANDLE_GRAY));
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

      if (status == STATUS_SUCCESS) 
	{
	  if (! save_image (param[3].data.d_string, image_ID, drawable_ID))
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }
	}

      if (export == EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else
    {
      status = STATUS_CALLING_ERROR;
    }

  values[0].data.d_status = status;  
}

	
static guint16
get_short (FILE *file)
{
  /* 
   * Description:
   *     Reads a 16-bit integer from a file in such a way that the machine's
   *     bit ordering should not matter 
   */
  guchar buf[2];

  fread (buf, 2, 1, file);
  return (buf[0] << 8) + (buf[1] << 0);
}
	
static void
put_short (guint16  value, 
	   FILE    *file)
{
  /* 
   * Description:
   *     Reads a 16-bit integer from a file in such a way that the machine's
   *     bit ordering should not matter 
   */
  guchar buf[2];
  buf[0] = (guchar) (value >> 8);
  buf[1] = (guchar) (value % 256);

  fwrite (buf, 2, 1, file);
}

static gint32
load_image (gchar *filename)
{
  /*
   *  Description:
   *      load the given image into gimp
   * 
   *  Arguments:
   *      filename      - name on the file to read
   *
   *  Return Value:
   *      Image id for the loaded image
   *      
   */
  gint       i, j, tile_height, row;	
  FILE      *file = NULL;
  gchar     *progMessage;
  guchar    *dest; 
  guchar    *dest_base;
  GDrawable *drawable;
  gint32     image_ID;
  gint32     layer_ID;
  GPixelRgn  pixel_rgn;
  gushort    width, height, depth;

  GImageType  imgtype;
  GDrawableType gdtype;
  
  /* Set up progress display */
  progMessage = g_strdup_printf (_("Loading %s:"), filename);
  gimp_progress_init (progMessage);
  free (progMessage);

  PIX_DEBUG_PRINT ("Opening file: %s\n", filename);

  /* Open the file */
  file = fopen (filename, "r");
  if (NULL == file)
    {
      return -1;
    }
  
  /* Read header information */
  width  = get_short (file);
  height = get_short (file);
  get_short (file); /* Discard obsolete fields */
  get_short (file); /* Discard obsolete fields */
  depth  = get_short (file);

  PIX_DEBUG_PRINT ("Width %hu\n", width);
  PIX_DEBUG_PRINT ("Height %hu\n", height);

  if (depth == 8)
    {
      /* Loading a matte file */
      imgtype = GRAY;
      gdtype = GRAY_IMAGE;
    }
  else if (depth == 24)
    {
      /* Loading an RGB file */
      imgtype = RGB;
      gdtype = RGB_IMAGE;
    }
  else
    {
      /* Header is invalid */
      fclose (file);
      return -1;
    }

  image_ID = gimp_image_new (width, height, imgtype);
  gimp_image_set_filename (image_ID, filename);
  layer_ID = gimp_layer_new (image_ID, _("Background"),
			     width,
			     height,
			     gdtype, 100, NORMAL_MODE);
  gimp_image_add_layer (image_ID, layer_ID, 0);
  drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, 
		       drawable->height, TRUE, FALSE);

  tile_height = gimp_tile_height ();   

  if (depth == 24)
    {
      /* Read a 24-bit Pix image */
      guchar record[4];
      gint   readlen;
      tile_height = gimp_tile_height ();

      dest_base = dest = g_new (guchar, 3 * width * tile_height); 

      for (i = 0; i < height;)
	{
	  for (dest = dest_base, row = 0;
	       row < tile_height && i < height;
	       i += 1, row += 1)
	    {
	      guchar count;

	      /* Read a row of the image */
	      j = 0;
	      while (j < width)
		{
		  readlen = fread (record, 1, 4, file);
		  if (readlen < 4)
		    break;

		  for (count = 0; count < record[0]; ++count)
		    {
		      dest[0]   = record[3];
		      dest[1]   = record[2];
		      dest[2]   = record[1];
		      dest += 3;
		      j++;
		      if (j >= width)
			break;
		    }
		}
	    }
	  gimp_pixel_rgn_set_rect (&pixel_rgn, dest_base, 0, i-row, 
				   width, row);
	  gimp_progress_update ((double) i / (double) height);
	}

      free (dest_base);
    }
  else
    {
      /* Read an 8-bit Matte image */
      guchar record[2];
      gint   readlen;  

      dest_base = dest = g_new (guchar, width * tile_height); 
      
      for (i = 0; i < height;) 
	{
	  for (dest = dest_base, row = 0; 
	       row < tile_height && i < height;
	       i += 1, row += 1)
	    {
	      guchar count;

	      /* Read a row of the image */
	      j = 0;
	      while (j < width)
		{
		  readlen = fread(record, 1, 2, file);
		  if (readlen < 2)
		    break;

		  for (count = 0; count < record[0]; ++count)
		    {
		      dest[j]   = record[1];
		      j++;
		      if (j >= width)
			break;
		    }
		}
	      dest += width;
	    }
	  gimp_pixel_rgn_set_rect (&pixel_rgn, dest_base, 0, i-row, 
				   width, row);
	  gimp_progress_update ((double) i / (double) height);
	}
      g_free (dest_base);
    }
  
  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);
  
  fclose (file);
  
  return image_ID;
}

static gint 
save_image (gchar  *filename,  
	    gint32  image_ID, 
	    gint32  drawable_ID)
{
/* 
 *  Description:
 *      save the given file out as an alias pix or matte file
 * 
 *  Arguments: 
 *      filename    - name of file to save to
 *      image_ID    - ID of image to save
 *      drawable_ID - current drawable
 */
  gint       depth, i, j, row, tile_height, writelen, rectHeight;
  /* gboolean   savingAlpha = FALSE; */
  gboolean   savingColor = TRUE;
  guchar    *src; 
  guchar    *src_base;
  gchar     *progMessage;
  GDrawable *drawable;
  GPixelRgn  pixel_rgn;
  FILE      *file;

  /* Get info about image */
  drawable = gimp_drawable_get (drawable_ID);
  
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width,
		       drawable->height, FALSE, FALSE);

  switch (gimp_drawable_type(drawable_ID))
    {
    case GRAY_IMAGE:
      savingColor = FALSE; 
      depth = 1;
      break;
    case GRAYA_IMAGE:
      savingColor = FALSE;
      depth = 2;
      break;
    case RGB_IMAGE:
      savingColor = TRUE;
      depth = 3;
      break;
    case RGBA_IMAGE:
      savingColor = TRUE;
      depth = 4;
      break;
    default:
      return FALSE;
    };

  /* Open the output file. */
  file = fopen (filename, "wb");
  if (!file)
    return FALSE;

  /* Set up progress display */
  progMessage = g_strdup_printf (_("Saving %s:"), filename);
  gimp_progress_init (progMessage);
  free (progMessage);

  /* Write the image header */
  PIX_DEBUG_PRINT ("Width %hu\n", drawable->width);
  PIX_DEBUG_PRINT ("Height %hu\n", drawable->height);
  put_short (drawable->width, file);
  put_short (drawable->height, file);
  put_short (0, file);
  put_short (0, file);
  if (savingColor)
    {
      put_short (24, file);
    }
  else
    {
      put_short (8, file);
    }

  tile_height = gimp_tile_height ();
  src_base    = g_new (guchar, tile_height * drawable->width * depth);

  if (savingColor)
    {
      /* Writing a 24-bit Pix image */
      guchar record[4];

      for (i = 0; i < drawable->height;)
	{
	  rectHeight = (tile_height < (drawable->height - i - 1)) ?
	    tile_height : (drawable->height - i - 1);
	  gimp_pixel_rgn_get_rect (&pixel_rgn, src_base, 0, i, 
				   drawable->width, rectHeight);

	  for (src = src_base, row = 0;
	       row < tile_height && i < drawable->height;
	       i += 1, row += 1)
	    {
	      /* Write a row of the image */
	      record[0] = 1;
	      record[3] = src[0];
	      record[2] = src[1];
	      record[1] = src[2];
	      src += depth;
	      for (j = 1; j < drawable->width; ++j)
		{
		  if ((record[3] != src[0]) ||
		       (record[2] != src[1]) ||
		       (record[1] != src[2]) ||
		       (record[0] == 255))
		    {
		      /* Write current RLE record and start a new one */

		      writelen = fwrite (record, 1, 4, file);
		      record[0] = 1;
		      record[3] = src[0];
		      record[2] = src[1];
		      record[1] = src[2];
		    }
		  else
		    {
		      /* increment run length in current record */
		      record[0]++;
		    }
		  src += depth;
		}
	      /* Write last record in row */
	      writelen = fwrite (record, 1, 4, file);
	    }
	  gimp_progress_update ((double) i / (double) drawable->height);
	}
    }
  else
    {
      /* Writing a 8-bit Matte (Mask) image */
      guchar record[2];

      for (i = 0; i < drawable->height;)
	{
	  rectHeight = (tile_height < (drawable->height - i - 1)) ?
	    tile_height : (drawable->height - i - 1);
	  gimp_pixel_rgn_get_rect (&pixel_rgn, src_base, 0, i, 
				   drawable->width, rectHeight);

	  for (src = src_base, row = 0;
	       row < tile_height && i < drawable->height;
	       i += 1, row += 1)
	    {
	      /* Write a row of the image */
	      record[0] = 1;
	      record[1] = src[0];
	      src += depth;
	      for (j = 1; j < drawable->width; ++j)
		{
		  if ((record[1] != src[0]) || (record[0] == 255)) 
		    {
		      /* Write current RLE record and start a new one */
		      writelen = fwrite (record, 1, 2, file);
		      record[0] = 1;
		      record[1] = src[0];
		    }
		  else
		    {
		      /* increment run length in current record */
		      record[0] ++;
		    }
		  src += depth;
		}
	      /* Write last record in row */
	      writelen = fwrite (record, 1, 2, file);
	    }
	  gimp_progress_update ((double) i / (double) drawable->height);
	}
    }

  g_free (src_base);
  
  fclose (file);
  return TRUE;
}
