/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * FITS file plugin
 * reading and writing code Copyright (C) 1997 Peter Kirchgessner
 * e-mail: pkirchg@aol.com, WWW: http://members.aol.com/pkirchg
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
 */
static char ident[] = "@(#) GIMP FITS file-plugin v1.05  20-Dec-97";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "fitsrw.h"

/* Load info */
typedef struct
{
  guint replace;    /* replacement for blank/NaN-values */
  guint use_datamin;/* Use DATAMIN/MAX-scaling if possible */
  guint compose;    /* compose images with naxis==3 */
} FITSLoadVals;

typedef struct
{
  gint run;
} FITSLoadInterface;

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


/* Declare some local functions.
 */
static void   init       (void);
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);

static gint32 load_image (char *filename);
static gint   save_image (char *filename,
                          gint32  image_ID,
                          gint32  drawable_ID);

static FITS_HDU_LIST *create_fits_header (FITS_FILE *ofp,
                        guint width, guint height, guint bpp);
static gint save_index (FITS_FILE *ofp,
                        gint32 image_ID,
                        gint32 drawable_ID);
static gint save_direct(FITS_FILE *ofp,
                        gint32 image_ID,
                        gint32 drawable_ID);

static gint32 create_new_image (char *filename, guint pagenum,
                guint width, guint height,
                GImageType itype,
                GDrawableType dtype,
                gint32 *layer_ID, GDrawable **drawable,
                GPixelRgn *pixel_rgn);

static void   check_load_vals (void);

static gint32 load_fits (char *filename, FITS_FILE *ifp,
                         guint picnum, guint ncompose);


/* Dialog-handling */
#define LOAD_FITS_TOGGLES 3
typedef struct
{
  GtkWidget *dialog;
  int toggle_val[2*LOAD_FITS_TOGGLES];
} LoadDialogVals;


static gint   load_dialog              (void);
static void   load_close_callback      (GtkWidget *widget,
                                        gpointer   data);
static void   load_ok_callback         (GtkWidget *widget,
                                        gpointer   data);
static void   load_toggle_update       (GtkWidget *widget,
                                        gpointer   data);
static void   show_fits_errors         (void);
static void   show_message             (char *);


GPlugInInfo PLUG_IN_INFO =
{
  init,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


/* The run mode */
static GRunModeType l_run_mode;


MAIN ()


static void
init (void)

{
  gimp_set_data ("file_fits_load", &plvals, sizeof (FITSLoadVals));
}


static void
query (void)

{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name of the file to load" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals)
                               / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename",
            "The name of the file to save the image in" },
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_fits_load",
                          "load file of the FITS file format",
                          "load file of the FITS file format (Flexible Image\
 Transport System)",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner (pkirchg@aol.com)",
                          "1997",
                          "<Load>/FITS",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_fits_save",
                          "save file in the FITS file format",
                          "FITS saving handles all image types except \
those with alpha channels.",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner (pkirchg@aol.com)",
                          "1997",
                          "<Save>/FITS",
                          "RGB*, GRAY*, INDEXED*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  /* Register file plugin by plugin name and handable extensions */
  gimp_register_magic_load_handler ("file_fits_load", "fit,fits", "",
                                    "0,string,SIMPLE");
  gimp_register_save_handler ("file_fits_save", "fit,fits", "");
}


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)

{
  static GParam values[2];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint32 image_ID;

  /* initialize */

  image_ID = -1;

  l_run_mode = run_mode = (GRunModeType)param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_fits_load") == 0)
    {
      *nreturn_vals = 2;
      values[1].type = PARAM_IMAGE;
      values[1].data.d_image = -1;

      switch (run_mode)
      {
        case RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data ("file_fits_load", &plvals);

          if (!load_dialog ()) return;
          break;

        case RUN_NONINTERACTIVE:
          if (nparams != 3)
          {
            status = STATUS_CALLING_ERROR;
          }
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

        status = (image_ID != -1) ? STATUS_SUCCESS : STATUS_EXECUTION_ERROR;

        /*  Store plvals data  */
        if (status == STATUS_SUCCESS)
          gimp_set_data ("file_fits_load", &plvals, sizeof (FITSLoadVals));
      }
      values[0].data.d_status = status;
      values[1].data.d_image = image_ID;
    }
  else if (strcmp (name, "file_fits_save") == 0)
    {
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

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string, param[1].data.d_int32,
                      param[2].data.d_int32))
          values[0].data.d_status = STATUS_SUCCESS;
      else
        values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
}


static gint32
load_image (char *filename)

{gint32 image_ID, *image_list, *nl;
 guint picnum;
 int   k, n_images, max_images, hdu_picnum;
 int   compose;
 FILE *fp;
 FITS_FILE *ifp;
 FITS_HDU_LIST *hdu;

 fp = fopen (filename, "rb");
 if (!fp)
 {
   show_message ("can't open file for reading");
   return (-1);
 }
 fclose (fp);

 ifp = fits_open (filename, "r");
 if (ifp == NULL)
 {
   show_message ("error during open of FITS file");
   return (-1);
 }
 if (ifp->n_pic <= 0)
 {
   show_message ("FITS file keeps no displayable images");
   fits_close (ifp);
   return (-1);
 }

 image_list = (gint32 *)g_malloc (10 * sizeof (gint32));
 if (image_list == NULL)
 {
   show_message ("out of memory");
   return (-1);
 }
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
     gimp_image_enable_undo (image_list[k]);
     gimp_image_clean_all (image_list[k]);
     gimp_display_new (image_list[k]);
   }
 }

 image_ID = (n_images > 0) ? image_list[0] : -1;
 g_free (image_list);

 return (image_ID);
}


static gint
save_image (char *filename,
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
    show_message ("FITS save cannot handle images with alpha channels");
    return FALSE;
  }

  switch (drawable_type)
  {
    case INDEXED_IMAGE: case INDEXEDA_IMAGE:
    case GRAY_IMAGE:    case GRAYA_IMAGE:
    case RGB_IMAGE:     case RGBA_IMAGE:
      break;
    default:
      show_message ("cannot operate on unknown image types");
      return (FALSE);
      break;
  }

  /* Open the output file. */
  ofp = fits_open (filename, "w");
  if (!ofp)
  {
    show_message ("cant open file for writing");
    return (FALSE);
  }

  if (l_run_mode != RUN_NONINTERACTIVE)
  {
    temp = g_malloc (strlen (filename) + 11);
    sprintf (temp, "Saving %s:", filename);
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
create_new_image (char *filename,
                  guint pagenum,
                  guint width,
                  guint height,
                  GImageType itype,
                  GDrawableType dtype,
                  gint32 *layer_ID,
                  GDrawable **drawable,
                  GPixelRgn *pixel_rgn)

{gint32 image_ID;
 char *tmp;

 image_ID = gimp_image_new (width, height, itype);
 if ((tmp = g_malloc (strlen (filename) + 32)) != NULL)
 {
   sprintf (tmp, "%s-img%ld", filename, (long)pagenum);
   gimp_image_set_filename (image_ID, tmp);
   g_free (tmp);
 }
 else
   gimp_image_set_filename (image_ID, filename);

 *layer_ID = gimp_layer_new (image_ID, "Background", width, height,
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
load_fits (char *filename,
           FITS_FILE *ifp,
           guint picnum,
           guint ncompose)

{register unsigned char *dest, *src;
 unsigned char *data, *data_end, *linebuf;
 int width, height, tile_height, scan_lines;
 int i, j, channel, max_scan;
 double a, b;
 gint32 layer_ID, image_ID;
 GPixelRgn pixel_rgn;
 GDrawable *drawable;
 GImageType itype;
 GDrawableType dtype;
 int err = 0;
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
         gimp_pixel_rgn_get_rect (&pixel_rgn, data_end-max_scan*width*ncompose,
                                  0, height-i-max_scan, width, max_scan);
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
         gimp_pixel_rgn_set_rect (&pixel_rgn, dest-channel, 0, height-i-1,
                                  width, scan_lines);
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
   show_message ("EOF encountered on reading");

 gimp_drawable_flush (drawable);

 return (err ? -1 : image_ID);
}


static FITS_HDU_LIST *
create_fits_header (FITS_FILE *ofp,
                   guint width,
                   guint height,
                   guint bpp)

{FITS_HDU_LIST *hdulist;
 int print_ctype3 = 0;   /* The CTYPE3-card may not be FITS-conforming */
 static char *ctype3_card[] = {
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
  "COMMENT FitsRW is (C) Peter Kirchgessner (pkirchg@aol.com), but available");
 fits_add_card (hdulist,
  "COMMENT under the GNU general public licence."),
 fits_add_card (hdulist,
  "COMMENT For sources see ftp://members.aol.com/pkirchg/pub/gimp");
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
save_direct  (FITS_FILE *ofp,
              gint32 image_ID,
              gint32 drawable_ID)

{ int height, width, i, j, channel;
  int tile_height, bpp, bpsl;
  long nbytes;
  unsigned char *data, *src;
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
    show_message ("write error occured");
    return (FALSE);
  }
  return (TRUE);
#undef GET_DIRECT_TILE
}


/* Save indexed colours (INDEXED, INDEXEDA) */
static gint
save_index (FITS_FILE *ofp,
            gint32 image_ID,
            gint32 drawable_ID)

{ int height, width, i, j, channel;
  int tile_height, bpp, bpsl, ncols;
  long nbytes;
  unsigned char *data, *src, *cmap, *cmapptr;
  unsigned char red[256], green[256], blue[256];
  unsigned char *channels[3];
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
  src = data = (unsigned char *)g_malloc (width * height * bpp);

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
      if ((i % tile_height) == 0) GET_INDEXED_TILE (data); /* get more data */

      for (j = 0; j < width; j++)  /* Write out bytes for current channel */
      {
        putc (cmapptr[*src], ofp->fp);
        src += bpp;
      }
      nbytes += width;
      src -= 2*bpsl;
    }

    if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
      gimp_progress_update ((double)(i+channel*height)/(double)(height*(bpp+2)));
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
      gimp_progress_update ((double)(i+channel*height)/(double)(height*(bpp+2)));
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
    show_message ("write error occured");
    return (FALSE);
  }
  return (TRUE);
#undef GET_INDEXED_TILE
}


/*  Load interface functions  */

static gint
load_dialog (void)

{
  LoadDialogVals *vals;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *toggle_vbox;
  GSList *group;
  gchar **argv;
  gint argc, k, j;
  char **textptr;
  static char *toggle_text[] = {
    "BLANK/NaN pixel replacement", "Black", "White",
    "Pixel value scaling", "Automatic", "by DATAMIN/DATAMAX",
    "Image composing", "None", "NAXIS=3, NAXIS3=2,...,4"
  };


  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("Load");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  vals = g_malloc (sizeof (*vals));

  vals->toggle_val[0] = (plvals.replace == 0);
  vals->toggle_val[1] = !(vals->toggle_val[0]);
  vals->toggle_val[2] = (plvals.use_datamin == 0);
  vals->toggle_val[3] = !(vals->toggle_val[2]);
  vals->toggle_val[4] = (plvals.compose == 0);
  vals->toggle_val[5] = !(vals->toggle_val[4]);

  vals->dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (vals->dialog), "Load FITS");
  gtk_window_position (GTK_WINDOW (vals->dialog), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (vals->dialog), "destroy",
                      (GtkSignalFunc) load_close_callback,
                      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) load_ok_callback,
                      vals);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT (vals->dialog));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_show (button);

  textptr = toggle_text;
  for (k = 0; k < LOAD_FITS_TOGGLES; k++)
  {
    frame = gtk_frame_new (*(textptr++));
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_border_width (GTK_CONTAINER (frame), 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->vbox),
                        frame, FALSE, TRUE, 0);
    toggle_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
    gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

    group = NULL;
    for (j = 0; j < 2; j++)
    {
      toggle = gtk_radio_button_new_with_label (group, *(textptr++));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
      gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                          (GtkSignalFunc) load_toggle_update,
                          &(vals->toggle_val[k*2+j]));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				    vals->toggle_val[k*2+j]);
      gtk_widget_show (toggle);
    }
    gtk_widget_show (toggle_vbox);
    gtk_widget_show (frame);
  }

  gtk_widget_show (vals->dialog);

  gtk_main ();
  gdk_flush ();

  g_free (vals);

  return plint.run;
}


static void
load_close_callback (GtkWidget *widget,
                     gpointer   data)

{
  gtk_main_quit ();
}


static void
load_ok_callback (GtkWidget *widget,
                  gpointer   data)

{LoadDialogVals *vals = (LoadDialogVals *)data;

  /* Read replacement */
  plvals.replace = (vals->toggle_val[0]) ? 0 : 255;

  /* Read pixel value scaling */
  plvals.use_datamin = vals->toggle_val[3];

  /* Read image composing */
  plvals.compose = vals->toggle_val[5];

  plint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (vals->dialog));
}


static void
load_toggle_update (GtkWidget *widget,
                    gpointer   data)

{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = 1;
  else
    *toggle_val = 0;
}


static void show_fits_errors (void)

{
  char *msg;

  /* Write out error messages of FITS-Library */
  while ((msg = fits_get_error ()) != NULL)
    show_message (msg);
}


/* Show a message. Where to show it, depends on the runmode */
static void show_message (char *message)

{
#ifdef Simple_Message_Box_Available
 /* If there would be a simple message box like the one */
 /* used in ../app/interface.h, I would like to use it. */
 if (l_run_mode == RUN_INTERACTIVE)
   gtk_message_box (message);
 else
#endif
   fprintf (stderr, "Fits: %s\n", message);
}
