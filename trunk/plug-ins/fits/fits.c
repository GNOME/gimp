/* GIMP - The GNU Image Manipulation Program
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
 * V 1.07, PK, 16-Aug-06: Fix problems with internationalization
 *                        (writing 255,0 instead of 255.0)
 *                        Fix problem with not filling up properly last record
 */

#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "fitsrw.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-fits-load"
#define SAVE_PROC      "file-fits-save"
#define PLUG_IN_BINARY "fits"


/* Load info */
typedef struct
{
  gint replace;     /* replacement for blank/NaN-values    */
  gint use_datamin; /* Use DATAMIN/MAX-scaling if possible */
  gint compose;     /* compose images with naxis==3        */
} FITSLoadVals;


/* Declare some local functions.
 */
static void   query      (void);
static void   run        (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);

static gint32 load_image (const gchar  *filename);
static gint   save_image (const gchar  *filename,
                          gint32        image_ID,
                          gint32        drawable_ID);

static FITS_HDU_LIST *create_fits_header (FITS_FILE *ofp,
					  guint width,
                                          guint height,
                                          guint bpp);
static gint save_index  (FITS_FILE *ofp,
			 gint32 image_ID,
			 gint32 drawable_ID);
static gint save_direct (FITS_FILE *ofp,
			 gint32 image_ID,
			 gint32 drawable_ID);

static gint32 create_new_image (const gchar        *filename,
				guint               pagenum,
				guint               width,
				guint               height,
				GimpImageBaseType   itype,
				GimpImageType       dtype,
				gint32             *layer_ID,
				GimpDrawable      **drawable,
				GimpPixelRgn       *pixel_rgn);

static void     check_load_vals  (void);

static gint32   load_fits        (const gchar *filename,
                                  FITS_FILE   *ifp,
                                  guint        picnum,
                                  guint        ncompose);


static gboolean load_dialog      (void);
static void     show_fits_errors (void);


static FITSLoadVals plvals =
{
  0,        /* Replace with black */
  0,        /* Do autoscale on pixel-values */
  0         /* Dont compose images */
};

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

/* The run mode */
static GimpRunMode l_run_mode;


MAIN ()

static void
query (void)

{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to load" },
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,    "image",        "Output image" },
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" },
  };

  gimp_install_procedure (LOAD_PROC,
                          "load file of the FITS file format",
                          "load file of the FITS file format "
                          "(Flexible Image Transport System)",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner (peter@kirchgessner.net)",
                          "1997",
                          N_("Flexible Image Transport System"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-fits");
  gimp_register_magic_load_handler (LOAD_PROC,
				    "fit,fits",
				    "",
                                    "0,string,SIMPLE");

  gimp_install_procedure (SAVE_PROC,
                          "save file in the FITS file format",
                          "FITS saving handles all image types except "
                          "those with alpha channels.",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner (peter@kirchgessner.net)",
                          "1997",
                          N_("Flexible Image Transport System"),
                          "RGB, GRAY, INDEXED",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-fits");
  gimp_register_save_handler (SAVE_PROC, "fit,fits", "");
}


static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_ID;
  gint32             drawable_ID;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;

  l_run_mode = run_mode = (GimpRunMode)param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      switch (run_mode)
	{
        case GIMP_RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data (LOAD_PROC, &plvals);

          if (!load_dialog ())
	    status = GIMP_PDB_CANCEL;
	  break;

        case GIMP_RUN_NONINTERACTIVE:
          if (nparams != 3)
	    status = GIMP_PDB_CALLING_ERROR;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /* Possibly retrieve data */
          gimp_get_data (LOAD_PROC, &plvals);
          break;

        default:
          break;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  check_load_vals ();
	  image_ID = load_image (param[1].data.d_string);

	  /* Write out error messages of FITS-Library */
	  show_fits_errors ();

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

	  /*  Store plvals data  */
	  if (status == GIMP_PDB_SUCCESS)
	    gimp_set_data (LOAD_PROC, &plvals, sizeof (FITSLoadVals));
	}
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_ui_init (PLUG_IN_BINARY, FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "FITS",
				      (GIMP_EXPORT_CAN_HANDLE_RGB |
				       GIMP_EXPORT_CAN_HANDLE_GRAY |
				       GIMP_EXPORT_CAN_HANDLE_INDEXED));
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
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 5)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
	{
	  if (! save_image (param[3].data.d_string, image_ID, drawable_ID))
	    status = GIMP_PDB_EXECUTION_ERROR;
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


static gint32
load_image (const gchar *filename)
{
  gint32 image_ID, *image_list, *nl;
  guint  picnum;
  gint   k, n_images, max_images, hdu_picnum;
  gint   compose;
  FILE  *fp;
  FITS_FILE *ifp;
  FITS_HDU_LIST *hdu;

  fp = g_fopen (filename, "rb");
  if (!fp)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
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

  image_list = g_new (gint32, 10);
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
  if (l_run_mode != GIMP_RUN_NONINTERACTIVE)
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
save_image (const gchar *filename,
            gint32       image_ID,
            gint32       drawable_ID)
{
  FITS_FILE* ofp;
  GimpImageType drawable_type;
  gint retval;

  drawable_type = gimp_drawable_type (drawable_ID);

  /*  Make sure we're not saving an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable_ID))
    {
      g_message (_("FITS save cannot handle images with alpha channels"));
      return FALSE;
    }

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE: case GIMP_INDEXEDA_IMAGE:
    case GIMP_GRAY_IMAGE:    case GIMP_GRAYA_IMAGE:
    case GIMP_RGB_IMAGE:     case GIMP_RGBA_IMAGE:
      break;
    default:
      g_message (_("Cannot operate on unknown image types."));
      return (FALSE);
      break;
    }

  /* Open the output file. */
  ofp = fits_open (filename, "w");
  if (!ofp)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return (FALSE);
    }

  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));

  if ((drawable_type == GIMP_INDEXED_IMAGE) ||
      (drawable_type == GIMP_INDEXEDA_IMAGE))
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
create_new_image (const gchar        *filename,
                  guint               pagenum,
                  guint               width,
                  guint               height,
                  GimpImageBaseType   itype,
                  GimpImageType       dtype,
                  gint32             *layer_ID,
                  GimpDrawable      **drawable,
                  GimpPixelRgn       *pixel_rgn)
{
  gint32 image_ID;
  char   *tmp;

  image_ID = gimp_image_new (width, height, itype);
  if ((tmp = g_malloc (strlen (filename) + 64)) != NULL)
    {
      sprintf (tmp, "%s-img%ld", filename, (long)pagenum);
      gimp_image_set_filename (image_ID, tmp);
      g_free (tmp);
    }
  else
    gimp_image_set_filename (image_ID, filename);

  gimp_image_undo_disable (image_ID);
  *layer_ID = gimp_layer_new (image_ID, _("Background"), width, height,
			      dtype, 100, GIMP_NORMAL_MODE);
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
load_fits (const gchar *filename,
           FITS_FILE   *ifp,
           guint        picnum,
           guint        ncompose)
{
  register guchar *dest, *src;
  guchar *data, *data_end, *linebuf;
  int width, height, tile_height, scan_lines;
  int i, j, channel, max_scan;
  double a, b;
  gint32 layer_ID, image_ID;
  GimpPixelRgn pixel_rgn;
  GimpDrawable *drawable;
  GimpImageBaseType itype;
  GimpImageType dtype;
  gint err = 0;
  FITS_HDU_LIST *hdulist;
  FITS_PIX_TRANSFORM trans;

  hdulist = fits_seek_image (ifp, (int)picnum);
  if (hdulist == NULL) return (-1);

  width = hdulist->naxisn[0];  /* Set the size of the FITS image */
  height = hdulist->naxisn[1];

  if (ncompose == 2) { itype = GIMP_GRAY; dtype = GIMP_GRAYA_IMAGE; }
  else if (ncompose == 3) { itype = GIMP_RGB; dtype = GIMP_RGB_IMAGE; }
  else if (ncompose == 4) { itype = GIMP_RGB; dtype = GIMP_RGBA_IMAGE; }
  else { ncompose = 1; itype = GIMP_GRAY; dtype = GIMP_GRAY_IMAGE;}

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

	  if ((i % 20) == 0)
	    gimp_progress_update ((gdouble) (i + 1) / (gdouble) height);

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

	      if ((i % 20) == 0)
		gimp_progress_update ((gdouble) (channel * height + i + 1) /
                                      (gdouble) (height * ncompose));

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
    "COMMENT Image type within GIMP: GIMP_GRAY_IMAGE",
    NULL,
    NULL,
    "COMMENT Image type within GIMP: GIMP_GRAYA_IMAGE (gray with alpha channel)",
    "COMMENT Sequence for NAXIS3   : GRAY, ALPHA",
    "CTYPE3  = 'GRAYA   '           / GRAY IMAGE WITH ALPHA CHANNEL",
    "COMMENT Image type within GIMP: GIMP_RGB_IMAGE",
    "COMMENT Sequence for NAXIS3   : RED, GREEN, BLUE",
    "CTYPE3  = 'RGB     '           / RGB IMAGE",
    "COMMENT Image type within GIMP: GIMP_RGBA_IMAGE (rgb with alpha channel)",
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
  GimpPixelRgn pixel_rgn;
  GimpDrawable *drawable;
  GimpImageType drawable_type;
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
	  nbytes += width;
	  src -= 2*bpsl;

	  if ((i % 20) == 0)
	    gimp_progress_update ((gdouble) (i + channel * height) /
                                  (gdouble) (height * bpp));
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
      g_message (_("Write error occurred"));
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
  GimpPixelRgn pixel_rgn;
  GimpDrawable *drawable;
  GimpImageType drawable_type;
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

  cmapptr = cmap = gimp_image_get_colormap (image_ID, &ncols);
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

      if ((i % 20) == 0)
	gimp_progress_update ((gdouble) (i + channel * height) /
			      (gdouble) (height * (bpp + 2)));
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

      if ((i % 20) == 0)
	gimp_progress_update ((gdouble) (i + channel * height) /
			      (gdouble) (height * (bpp + 2)));
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
      g_message (_("Write error occurred"));
      return (FALSE);
    }
  return (TRUE);
#undef GET_INDEXED_TILE
}

/*  Load interface functions  */

static gboolean
load_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *frame;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Load FITS File"), PLUG_IN_BINARY,
                            NULL, 0,
			    gimp_standard_help_func, LOAD_PROC,

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

			    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gimp_int_radio_group_new (TRUE, _("Replacement for undefined pixels"),
				    G_CALLBACK (gimp_radio_button_update),
				    &plvals.replace, plvals.replace,

				    _("Black"), 0,   NULL,
				    _("White"), 255, NULL,

				    NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame =
    gimp_int_radio_group_new (TRUE, _("Pixel value scaling"),
			      G_CALLBACK (gimp_radio_button_update),
			      &plvals.use_datamin, plvals.use_datamin,

			      _("Automatic"),          FALSE, NULL,
			      _("By DATAMIN/DATAMAX"), TRUE,  NULL,

			      NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame =
    gimp_int_radio_group_new (TRUE, _("Image Composing"),
			      G_CALLBACK (gimp_radio_button_update),
			      &plvals.compose, plvals.compose,

			      _("None"),                 FALSE, NULL,
			      "NAXIS=3, NAXIS3=2,...,4", TRUE,  NULL,

			      NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
show_fits_errors (void)
{
  gchar *msg;

  /* Write out error messages of FITS-Library */
  while ((msg = fits_get_error ()) != NULL)
    g_message (msg);
}
