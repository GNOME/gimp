/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * FITS file plugin
 * reading and writing code Copyright (C) 1997 Peter Kirchgessner
 * e-mail: peter@kirchgessner.net, WWW: http://www.kirchgessner.net
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

/* Event history:
 * V 1.00, PK, 05-May-97: Creation
 * V 1.01, PK, 19-May-97: Problem with compilation on Irix fixed
 * V 1.02, PK, 08-Jun-97: Bug with saving gray images fixed
 * V 1.03, PK, 05-Oct-97: Parse rc-file
 * V 1.04, PK, 12-Oct-97: No progress bars for non-interactive mode
 * V 1.05, nn, 20-Dec-97: Initialize image_ID in run()
 * V 1.06, PK, 21-Nov-99: Internationalization
 *                        Fix bug with gimp_export_image()
 *                        (moved it from load to save)
 */
static char ident[] = "@(#) GIMP FITS file-plugin v1.06  21-Nov-99";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "fitsrw.h"

#include "libgimp/stdplugins-intl.h"


/* Load info */
typedef struct
{
  gint replace;     /* replacement for blank/NaN-values    */
  gint use_datamin; /* Use DATAMIN/MAX-scaling if possible */
  gint compose;     /* compose images with naxis==3        */
} FITSLoadVals;

typedef struct
{
  gint run;
} FITSLoadInterface;


/* Declare some local functions.
 */
static void   query      (void);
static void   run        (gchar   *name,
                          gint     nparams,
                          GParam  *param,
                          gint    *nreturn_vals,
                          GParam **return_vals);

static gint32 load_image (gchar  *filename);
static gint   save_image (gchar  *filename,
                          gint32  image_ID,
                          gint32  drawable_ID);

static FITS_HDU_LIST *create_fits_header (FITS_FILE *ofp,
					  guint width, guint height, guint bpp);
static gint save_index  (FITS_FILE *ofp,
			 gint32 image_ID,
			 gint32 drawable_ID);
static gint save_direct (FITS_FILE *ofp,
			 gint32 image_ID,
			 gint32 drawable_ID);

static gint32 create_new_image (gchar          *filename,
				guint           pagenum,
				guint           width,
				guint           height,
				GImageType      itype,
				GDrawableType   dtype,
				gint32         *layer_ID,
				GDrawable     **drawable,
				GPixelRgn       *pixel_rgn);

static void   check_load_vals (void);

static gint32 load_fits (gchar     *filename,
			 FITS_FILE *ifp,
                         guint      picnum,
			 guint      ncompose);


static gint   load_dialog              (void);
static void   load_ok_callback         (GtkWidget *widget,
                                        gpointer   data);
static void   show_fits_errors         (void);


static FITSLoadVals plvals =
{
  0,        /* Replace with black */
  0,        /* Do autoscale on pixel-values */
  0         /* Dont compose images */
};

static FITSLoadInterface plint =
{
  FALSE
};

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

/* The run mode */
static GRunModeType l_run_mode;


MAIN ()

static void
query (void)

{
  static GParamDef load_args[] =
  {
    { PARAM_INT32,    "run_mode",     "Interactive, non-interactive" },
    { PARAM_STRING,   "filename",     "The name of the file to load" },
    { PARAM_STRING,   "raw_filename", "The name of the file to load" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE,    "image",        "Output image" },
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
    { PARAM_STRING,   "raw_filename", "The name of the file to save the image in" },
  };
  static gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  INIT_I18N();

  gimp_install_procedure ("file_fits_load",
                          "load file of the FITS file format",
                          "load file of the FITS file format (Flexible Image Transport System)",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner (peter@kirchgessner.net)",
                          "1997",
                          "<Load>/FITS",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_fits_save",
                          "save file in the FITS file format",
                          "FITS saving handles all image types except those with alpha channels.",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner (peter@kirchgessner.net)",
                          "1997",
                          "<Save>/FITS",
                          "RGB, GRAY, INDEXED",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  /* Register file plugin by plugin name and handable extensions */
  gimp_register_magic_load_handler ("file_fits_load",
				    "fit,fits",
				    "",
                                    "0,string,SIMPLE");
  gimp_register_save_handler       ("file_fits_save",
				    "fit,fits",
				    "");
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
  gint32        image_ID;
  gint32        drawable_ID;
  GimpExportReturnType export = EXPORT_CANCEL;

  l_run_mode = run_mode = (GRunModeType)param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_EXECUTION_ERROR;

  if (strcmp (name, "file_fits_load") == 0)
    {
      INIT_I18N_UI();

      switch (run_mode)
	{
        case RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data ("file_fits_load", &plvals);

          if (!load_dialog ())
	    status = STATUS_CANCEL;
	  break;

        case RUN_NONINTERACTIVE:
          if (nparams != 3)
	    status = STATUS_CALLING_ERROR;
          break;

        case RUN_WITH_LAST_VALS:
          /* Possibly retrieve data */
          gimp_get_data ("file_fits_load", &plvals);
          break;

        default:
          break;
	}

      if (status == STATUS_SUCCESS)
	{
	  check_load_vals ();
	  image_ID = load_image (param[1].data.d_string);

	  /* Write out error messages of FITS-Library */
	  show_fits_errors ();

	  if (image_ID != -1)
	    {
	      *nreturn_vals = 2;
	      values[1].type         = PARAM_IMAGE;
	      values[1].data.d_image = image_ID;
	    }
	  else
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }

	  /*  Store plvals data  */
	  if (status == STATUS_SUCCESS)
	    gimp_set_data ("file_fits_load", &plvals, sizeof (FITSLoadVals));
	}
    }
  else if (strcmp (name, "file_fits_save") == 0)
    {
      INIT_I18N_UI();

      image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	case RUN_WITH_LAST_VALS:
	  gimp_ui_init ("fits", FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "FITS",
				      (CAN_HANDLE_RGB |
				       CAN_HANDLE_GRAY |
				       CAN_HANDLE_INDEXED));
	if (export == EXPORT_CANCEL)
	  {
	    values[0].data.d_status = STATUS_CANCEL;
	    return;
	  }
	break;
      default:
	break;
      }

      switch (run_mode)
        {
        case RUN_INTERACTIVE:
          break;

        case RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 5)
            status = STATUS_CALLING_ERROR;
          break;

        case RUN_WITH_LAST_VALS:
          break;

        default:
          break;
        }

      if (status == STATUS_SUCCESS)
	{
	  if (! save_image (param[3].data.d_string, image_ID, drawable_ID))
	    status = STATUS_EXECUTION_ERROR;
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


static gint32
load_image (gchar *filename)
{
  gint32 image_ID, *image_list, *nl;
  guint picnum;
  int   k, n_images, max_images, hdu_picnum;
  int   compose;
  FILE *fp;
  FITS_FILE *ifp;
  FITS_HDU_LIST *hdu;

  fp = fopen (filename, "rb");
  if (!fp)
    {
      g_message (_("Can't open file for reading"));
      return (-1);
    }
  fclose (fp);

  ifp = fits_open (filename, "r");
  if (ifp == NULL)
    {
      g_message (_("Error during open of FITS file"));
      return (-1);
    }
  if (ifp->n_pic <= 0)
    {
      g_message (_("FITS file keeps no displayable images"));
      fits_close (ifp);
      return (-1);
    }

  image_list = (gint32 *)g_malloc (10 * sizeof (gint32));
  n_images = 0;
  max_images = 10;

  for (picnum = 1; picnum <= ifp->n_pic; )
    {
      /* Get image info to see if we can compose them */
      hdu = fits_image_info (ifp, picnum, &hdu_picnum);
      if (hdu == NULL) break;

      /* Get number of FITS-images to compose */
      compose = (   plvals.compose && (hdu_picnum == 1) && (hdu->naxis == 3)
		    && (hdu->naxisn[2] > 1) && (hdu->naxisn[2] <= 4));
      if (compose)
	compose = hdu->naxisn[2];
      else
	compose = 1;  /* Load as GRAY */

      image_ID = load_fits (filename, ifp, picnum, compose);

      /* Write out error messages of FITS-Library */
      show_fits_errors ();

      if (image_ID == -1) break;
      if (n_images == max_images)
	{
	  nl = (gint32 *)g_realloc (image_list, (max_images+10)*sizeof (gint32));
	  if (nl == NULL) break;
	  image_list = nl;
	  max_images += 10;
	}
      image_list[n_images++] = image_ID;

      picnum += compose;
    }

  /* Write out error messages of FITS-Library */
  show_fits_errors ();

  fits_close (ifp);

  /* Display images in reverse order. The last will be displayed by GIMP itself*/
  if (l_run_mode != RUN_NONINTERACTIVE)
    {
      for (k = n_images-1; k >= 1; k--)
	{
	  gimp_image_undo_enable (image_list[k]);
	  gimp_image_clean_all (image_list[k]);
	  gimp_display_new (image_list[k]);
	}
    }

  image_ID = (n_images > 0) ? image_list[0] : -1;
  g_free (image_list);

  return (image_ID);
}


static gint
save_image (gchar  *filename,
            gint32  image_ID,
            gint32  drawable_ID)
{
  FITS_FILE* ofp;
  GDrawableType drawable_type;
  gint retval;
  char *temp = ident; /* Just to satisfy lint/gcc */

  drawable_type = gimp_drawable_type (drawable_ID);

  /*  Make sure we're not saving an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable_ID))
    {
      g_message (_("FITS save cannot handle images with alpha channels"));
      return FALSE;
    }

  switch (drawable_type)
    {
    case INDEXED_IMAGE: case INDEXEDA_IMAGE:
    case GRAY_IMAGE:    case GRAYA_IMAGE:
    case RGB_IMAGE:     case RGBA_IMAGE:
      break;
    default:
      g_message (_("Cannot operate on unknown image types"));
      return (FALSE);
      break;
    }

  /* Open the output file. */
  ofp = fits_open (filename, "w");
  if (!ofp)
    {
      g_message (_("Can't open file for writing"));
      return (FALSE);
    }

  if (l_run_mode != RUN_NONINTERACTIVE)
    {
      temp = g_strdup_printf (_("Saving %s:"), filename);
      gimp_progress_init (temp);
      g_free (temp);
    }

  if ((drawable_type == INDEXED_IMAGE) || (drawable_type == INDEXEDA_IMAGE))
    retval = save_index (ofp,image_ID, drawable_ID);
  else
    retval = save_direct (ofp,image_ID, drawable_ID);

  fits_close (ofp);

  return (retval);
}


/* Check (and correct) the load values plvals */
static void
check_load_vals (void)
{
  if (plvals.replace > 255) plvals.replace = 255;
}


/* Create an image. Sets layer_ID, drawable and rgn. Returns image_ID */
static gint32
create_new_image (gchar *filename,
                  guint pagenum,
                  guint width,
                  guint height,
                  GImageType itype,
                  GDrawableType dtype,
                  gint32 *layer_ID,
                  GDrawable **drawable,
                  GPixelRgn *pixel_rgn)
{
  gint32 image_ID;
  char *tmp;

  image_ID = gimp_image_new (width, height, itype);
  if ((tmp = g_malloc (strlen (filename) + 64)) != NULL)
    {
      sprintf (tmp, "%s-img%ld", filename, (long)pagenum);
      gimp_image_set_filename (image_ID, tmp);
      g_free (tmp);
    }
  else
    gimp_image_set_filename (image_ID, filename);

  *layer_ID = gimp_layer_new (image_ID, _("Background"), width, height,
			      dtype, 100, NORMAL_MODE);
  gimp_image_add_layer (image_ID, *layer_ID, 0);

  *drawable = gimp_drawable_get (*layer_ID);
  gimp_pixel_rgn_init (pixel_rgn, *drawable, 0, 0, (*drawable)->width,
		       (*drawable)->height, TRUE, FALSE);

  return (image_ID);
}


/* Load FITS image. ncompose gives the number of FITS-images which have */
/* to be composed together. This will result in different GIMP image types: */
/* 1: GRAY, 2: GRAYA, 3: RGB, 4: RGBA */
static gint32
load_fits (gchar     *filename,
           FITS_FILE *ifp,
           guint      picnum,
           guint      ncompose)
{
  register guchar *dest, *src;
  guchar *data, *data_end, *linebuf;
  int width, height, tile_height, scan_lines;
  int i, j, channel, max_scan;
  double a, b;
  gint32 layer_ID, image_ID;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GImageType itype;
  GDrawableType dtype;
  gint err = 0;
  FITS_HDU_LIST *hdulist;
  FITS_PIX_TRANSFORM trans;

  hdulist = fits_seek_image (ifp, (int)picnum);
  if (hdulist == NULL) return (-1);

  width = hdulist->naxisn[0];  /* Set the size of the FITS image */
  height = hdulist->naxisn[1];

  if (ncompose == 2) { itype = GRAY; dtype = GRAYA_IMAGE; }
  else if (ncompose == 3) { itype = RGB; dtype = RGB_IMAGE; }
  else if (ncompose == 4) { itype = RGB; dtype = RGBA_IMAGE; }
  else { ncompose = 1; itype = GRAY; dtype = GRAY_IMAGE;}

  image_ID = create_new_image (filename, picnum, width, height, itype, dtype,
			       &layer_ID, &drawable, &pixel_rgn);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width * ncompose);
  if (data == NULL) return (-1);
  data_end = data + tile_height * width * ncompose;

  /* If the transformation from pixel value to */
  /* data value has been specified, use it */
  if (   plvals.use_datamin
      && hdulist->used.datamin && hdulist->used.datamax
      && hdulist->used.bzero && hdulist->used.bscale)
    {
      a = (hdulist->datamin - hdulist->bzero) / hdulist->bscale;
      b = (hdulist->datamax - hdulist->bzero) / hdulist->bscale;
      if (a < b) trans.pixmin = a, trans.pixmax = b;
      else trans.pixmin = b, trans.pixmax = a;
    }
  else
    {
      trans.pixmin = hdulist->pixmin;
      trans.pixmax = hdulist->pixmax;
    }
  trans.datamin = 0.0;
  trans.datamax = 255.0;
  trans.replacement = plvals.replace;
  trans.dsttyp = 'c';

  /* FITS stores images with bottom row first. Therefore we have */
  /* to fill the image from bottom to top. */

  if (ncompose == 1)
    {
      dest = data + tile_height * width;
      scan_lines = 0;

      for (i = 0; i < height; i++)
	{
	  /* Read FITS line */
	  dest -= width;
	  if (fits_read_pixel (ifp, hdulist, width, &trans, dest) != width)
	    {
	      err = 1;
	      break;
	    }

	  scan_lines++;

	  if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
	    gimp_progress_update ((double)(i+1) / (double)height);

	  if ((scan_lines == tile_height) || ((i+1) == height))
	    {
	      gimp_pixel_rgn_set_rect (&pixel_rgn, dest, 0, height-i-1,
				       width, scan_lines);
	      scan_lines = 0;
	      dest = data + tile_height * width;
	    }
	  if (err) break;
	}
    }
  else   /* multiple images to compose */
    {
      linebuf = g_malloc (width);
      if (linebuf == NULL) return (-1);

      for (channel = 0; channel < ncompose; channel++)
	{
	  dest = data + tile_height * width * ncompose + channel;
	  scan_lines = 0;

	  for (i = 0; i < height; i++)
	    {
	      if ((channel > 0) && ((i % tile_height) == 0))
		{ /* Reload a region for follow up channels */
		  max_scan = tile_height;
		  if (i + tile_height > height) max_scan = height - i;
		  gimp_pixel_rgn_get_rect (&pixel_rgn,
					   data_end-max_scan*width*ncompose,
					   0, height-i-max_scan, width,
					   max_scan);
		}

	      /* Read FITS scanline */
	      dest -= width*ncompose;
	      if (fits_read_pixel (ifp, hdulist, width, &trans, linebuf) != width)
		{
		  err = 1;
		  break;
		}
	      j = width;
	      src = linebuf;
	      while (j--)
		{
		  *dest = *(src++);
		  dest += ncompose;
		}
	      dest -= width*ncompose;
	      scan_lines++;

	      if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
		gimp_progress_update (  (double)(channel*height+i+1)
					/ (double)(height*ncompose));

	      if ((scan_lines == tile_height) || ((i+1) == height))
		{
		  gimp_pixel_rgn_set_rect (&pixel_rgn, dest-channel,
					   0, height-i-1, width, scan_lines);
		  scan_lines = 0;
		  dest = data + tile_height * width * ncompose + channel;
		}
	      if (err) break;
	    }
	}
      g_free (linebuf);
    }

  g_free (data);

  if (err)
    g_message (_("EOF encountered on reading"));

  gimp_drawable_flush (drawable);

  return (err ? -1 : image_ID);
}


static FITS_HDU_LIST *
create_fits_header (FITS_FILE *ofp,
		    guint      width,
		    guint      height,
		    guint      bpp)
{
  FITS_HDU_LIST *hdulist;
  int print_ctype3 = 0;   /* The CTYPE3-card may not be FITS-conforming */
  static char *ctype3_card[] =
  {
    NULL, NULL, NULL,  /* bpp = 0: no additional card */
    "COMMENT Image type within GIMP: GRAY_IMAGE",
    NULL,
    NULL,
    "COMMENT Image type within GIMP: GRAYA_IMAGE (gray with alpha channel)",
    "COMMENT Sequence for NAXIS3   : GRAY, ALPHA",
    "CTYPE3  = 'GRAYA   '           / GRAY IMAGE WITH ALPHA CHANNEL",
    "COMMENT Image type within GIMP: RGB_IMAGE",
    "COMMENT Sequence for NAXIS3   : RED, GREEN, BLUE",
    "CTYPE3  = 'RGB     '           / RGB IMAGE",
    "COMMENT Image type within GIMP: RGBA_IMAGE (rgb with alpha channel)",
    "COMMENT Sequence for NAXIS3   : RED, GREEN, BLUE, ALPHA",
    "CTYPE3  = 'RGBA    '           / RGB IMAGE WITH ALPHA CHANNEL"
  };

  hdulist = fits_add_hdu (ofp);
  if (hdulist == NULL) return (NULL);

  hdulist->used.simple = 1;
  hdulist->bitpix = 8;
  hdulist->naxis = (bpp == 1) ? 2 : 3;
  hdulist->naxisn[0] = width;
  hdulist->naxisn[1] = height;
  hdulist->naxisn[2] = bpp;
  hdulist->used.datamin = 1;
  hdulist->datamin = 0.0;
  hdulist->used.datamax = 1;
  hdulist->datamax = 255.0;
  hdulist->used.bzero = 1;
  hdulist->bzero = 0.0;
  hdulist->used.bscale = 1;
  hdulist->bscale = 1.0;

  fits_add_card (hdulist, "");
  fits_add_card (hdulist,
		 "HISTORY THIS FITS FILE WAS GENERATED BY GIMP USING FITSRW");
  fits_add_card (hdulist, "");
  fits_add_card (hdulist,
		 "COMMENT FitsRW is (C) Peter Kirchgessner (peter@kirchgessner.net), but available");
  fits_add_card (hdulist,
		 "COMMENT under the GNU general public licence."),
    fits_add_card (hdulist,
		   "COMMENT For sources see http://www.kirchgessner.net");
  fits_add_card (hdulist, "");
  fits_add_card (hdulist, ctype3_card[bpp*3]);
  if (ctype3_card[bpp*3+1] != NULL)
    fits_add_card (hdulist, ctype3_card[bpp*3+1]);
  if (print_ctype3 && (ctype3_card[bpp*3+2] != NULL))
    fits_add_card (hdulist, ctype3_card[bpp*3+2]);
  fits_add_card (hdulist, "");

  return (hdulist);
}


/* Save direct colours (GRAY, GRAYA, RGB, RGBA) */
static gint
save_direct (FITS_FILE *ofp,
	     gint32     image_ID,
	     gint32     drawable_ID)
{
  int height, width, i, j, channel;
  int tile_height, bpp, bpsl;
  long nbytes;
  guchar *data, *src;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  FITS_HDU_LIST *hdu;

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width = drawable->width;
  height = drawable->height;
  bpp = drawable->bpp;       /* Bytes per pixel */
  bpsl = width * bpp;        /* Bytes per scanline */
  tile_height = gimp_tile_height ();
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (unsigned char *)g_malloc (width * height * bpp);

  hdu = create_fits_header (ofp, width, height, bpp);
  if (hdu == NULL) return (FALSE);
  if (fits_write_header (ofp, hdu) < 0) return (FALSE);

#define GET_DIRECT_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gimp_pixel_rgn_get_rect (&pixel_rgn, begin, 0, height-i-scan_lines, \
    width, scan_lines); \
    src = begin+bpsl*(scan_lines-1)+channel; }

  nbytes = 0;
  for (channel = 0; channel < bpp; channel++)
    {
      for (i = 0; i < height; i++)
	{
	  if ((i % tile_height) == 0) GET_DIRECT_TILE (data); /* get more data */

	  if (bpp == 1)  /* One channel only ? Write the scanline */
	    {
	      fwrite (src, 1, width, ofp->fp);
	      src += bpsl;
	    }
	  else           /* Multiple channels */
	    {
	      for (j = 0; j < width; j++)  /* Write out bytes for current channel */
		{
		  putc (*src, ofp->fp);
		  src += bpp;
		}
	    }
	  nbytes += bpsl;
	  src -= 2*bpsl;

	  if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
	    gimp_progress_update ((double)(i+channel*height)/(double)(height*bpp));
	}
    }

  nbytes = nbytes % FITS_RECORD_SIZE;
  if (nbytes)
    {
      while (nbytes++ < FITS_RECORD_SIZE)
	putc (0, ofp->fp);
    }

  g_free (data);

  gimp_drawable_detach (drawable);

  if (ferror (ofp->fp))
    {
      g_message (_("Write error occured"));
      return (FALSE);
    }
  return (TRUE);
#undef GET_DIRECT_TILE
}


/* Save indexed colours (INDEXED, INDEXEDA) */
static gint
save_index (FITS_FILE *ofp,
            gint32     image_ID,
            gint32     drawable_ID)
{
  int height, width, i, j, channel;
  int tile_height, bpp, bpsl, ncols;
  long nbytes;
  guchar *data, *src, *cmap, *cmapptr;
  guchar red[256], green[256], blue[256];
  guchar *channels[3];
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  FITS_HDU_LIST *hdu;

  channels[0] = red;   channels[1] = green;   channels[2] = blue;

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width = drawable->width;
  height = drawable->height;
  bpp = drawable->bpp;       /* Bytes per pixel */
  bpsl = width * bpp;        /* Bytes per scanline */
  tile_height = gimp_tile_height ();
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (guchar *)g_malloc (width * height * bpp);

  cmapptr = cmap = gimp_image_get_cmap (image_ID, &ncols);
  if (ncols > sizeof (red)) ncols = sizeof (red);
  for (i = 0; i < ncols; i++)
    {
      red[i] = *(cmapptr++);
      green[i] = *(cmapptr++);
      blue[i] = *(cmapptr++);
    }
  for (i = ncols; i < sizeof (red); i++)
    red[i] = green[i] = blue[i] = 0;

  hdu = create_fits_header (ofp, width, height, bpp+2);
  if (hdu == NULL) return (FALSE);
  if (fits_write_header (ofp, hdu) < 0) return (FALSE);

#define GET_INDEXED_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gimp_pixel_rgn_get_rect (&pixel_rgn, begin, 0, height-i-scan_lines, \
                             width, scan_lines); \
    src = begin+bpsl*(scan_lines-1); }

  nbytes = 0;

  /* Write the RGB-channels */
  for (channel = 0; channel < 3; channel++)
    {
      cmapptr = channels[channel];
      for (i = 0; i < height; i++)
	{
	  if ((i % tile_height) == 0)
	    GET_INDEXED_TILE (data); /* get more data */

	  for (j = 0; j < width; j++)  /* Write out bytes for current channel */
	    {
	      putc (cmapptr[*src], ofp->fp);
	      src += bpp;
	    }
	  nbytes += width;
	  src -= 2*bpsl;
	}

      if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
	gimp_progress_update ((double) (i+channel*height) /
			      (double) (height*(bpp+2)));
    }

  /* Write the Alpha-channel */
  if (bpp > 1)
    {
      for (i = 0; i < height; i++)
	{
	  if ((i % tile_height) == 0)
	    {
	      GET_INDEXED_TILE (data); /* get more data */
	      src++;                   /* Step to alpha channel data */
	    }

	  for (j = 0; j < width; j++)  /* Write out bytes for alpha channel */
	    {
	      putc (*src, ofp->fp);
	      src += bpp;
	    }
	  nbytes += width;
	  src -= 2*bpsl;
	}

      if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
	gimp_progress_update ((double) (i+channel*height) /
			      (double) (height*(bpp+2)));
    }

  nbytes = nbytes % FITS_RECORD_SIZE;
  if (nbytes)
    {
      while (nbytes++ < FITS_RECORD_SIZE)
	putc (0, ofp->fp);
    }

  g_free (data);

  gimp_drawable_detach (drawable);

  if (ferror (ofp->fp))
    {
      g_message (_("Write error occured"));
      return (FALSE);
    }
  return (TRUE);
#undef GET_INDEXED_TILE
}

/*  Load interface functions  */

static gint
load_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *frame;

  gimp_ui_init ("fits", FALSE);

  dialog = gimp_dialog_new (_("Load FITS File"), "fits",
			    gimp_plugin_help_func, "filters/fits.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, FALSE, FALSE,

			    _("OK"), load_ok_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gimp_radio_group_new2 (TRUE, _("BLANK/NaN Pixel Replacement"),
				 gimp_radio_button_update,
				 &plvals.replace, (gpointer) plvals.replace,

				 _("Black"), (gpointer) 0, NULL,
				 _("White"), (gpointer) 255, NULL,

				 NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame =
    gimp_radio_group_new2 (TRUE, _("Pixel Value Scaling"),
			   gimp_radio_button_update,
			   &plvals.use_datamin, (gpointer) plvals.use_datamin,

			   _("Automatic"),          (gpointer) FALSE, NULL,
			   _("By DATAMIN/DATAMAX"), (gpointer) TRUE, NULL,

			   NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame =
    gimp_radio_group_new2 (TRUE, _("Image Composing"),
			   gimp_radio_button_update,
			   &plvals.compose, (gpointer) plvals.compose,

			   _("None"),                 (gpointer) FALSE, NULL,
			   "NAXIS=3, NAXIS3=2,...,4", (gpointer) TRUE, NULL,

			   NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();

  return plint.run;
}

static void
load_ok_callback (GtkWidget *widget,
                  gpointer   data)

{
  plint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
show_fits_errors (void)
{
  gchar *msg;

  /* Write out error messages of FITS-Library */
  while ((msg = fits_get_error ()) != NULL)
    g_message (msg);
}
