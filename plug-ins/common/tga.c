/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * $Id$
 * TrueVision Targa loading and saving file filter for the Gimp.
 * Targa code Copyright (C) 1997 Raphael FRANCOIS and Gordon Matzigkeit
 *
 * The Targa reading and writing code was written from scratch by
 * Raphael FRANCOIS <fraph@ibm.net> and Gordon Matzigkeit
 * <gord@gnu.ai.mit.edu> based on the TrueVision TGA File Format
 * Specification, Version 2.0:
 *
 *   <URL:ftp://ftp.truevision.com/pub/TGA.File.Format.Spec/>
 *
 * It does not contain any code written for other TGA file loaders.
 * Not even the RLE handling. ;)
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
 */

/*
 * Release 1.2, 1997-09-24, Gordon Matzigkeit <gord@gnu.ai.mit.edu>:
 *   - Bug fixes and source cleanups.
 *
 * Release 1.1, 1997-09-19, Gordon Matzigkeit <gord@gnu.ai.mit.edu>:
 *   - Preserve alpha channels.  For indexed images, this can only be
 *     done if there is at least one free colormap entry.
 *
 * Release 1.0, 1997-09-06, Gordon Matzigkeit <gord@gnu.ai.mit.edu>:
 *   - Handle loading all image types from the 2.0 specification.
 *   - Fix many alignment and endianness problems.
 *   - Use tiles for lower memory consumption and better speed.
 *   - Rewrite RLE code for clarity and speed.
 *   - Handle saving with RLE.
 *
 * Release 0.9, 1997-06-18, Raphael FRANCOIS <fraph@ibm.net>:
 *   - Can load 24 and 32-bit Truecolor images, with and without RLE.
 *   - Saving currently only works without RLE.
 *
 *
 * TODO:
 *   - Handle loading images that aren't 8 bits per channel.
 *   - Maybe handle special features in developer and extension sections
 *     (the `save' dialogue would give access to them).
 *   - The GIMP stores the indexed alpha channel as a separate byte,
 *     one for each pixel.  The TGA file format spec requires that the
 *     alpha channel be stored as part of the colormap, not with each
 *     individual pixel.  This means that we have no good way of
 *     saving and loading INDEXEDA images that use alpha channel values
 *     other than 0 and 255.  Find a workaround.
 */

#define SAVE_ID_STRING "CREATOR: The GIMP's TGA Filter Version 1.2"

/* Set these for debugging. */
/* #define PROFILE 1 */
/* #define VERBOSE 1 */

#include "config.h"

#ifdef PROFILE
# include <sys/times.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Round up a division to the nearest integer. */
#define ROUNDUP_DIVIDE(n,d) (((n) + (d - 1)) / (d))

typedef struct _TgaSaveVals
{
  gint rle;
} TgaSaveVals;

static TgaSaveVals tsvals =
{
  1,    /* rle */
};

typedef struct _TgaSaveInterface
{
  gint run;
} TgaSaveInterface;

static TgaSaveInterface tsint =
{
  FALSE                /*  run  */
};

struct tga_header
{
  guint8 idLength;
  guint8 colorMapType;

  /* The image type. */
#define TGA_TYPE_MAPPED      1
#define TGA_TYPE_COLOR       2
#define TGA_TYPE_GRAY        3
#define TGA_TYPE_MAPPED_RLE  9
#define TGA_TYPE_COLOR_RLE  10
#define TGA_TYPE_GRAY_RLE   11
  guint8 imageType;

  /* Color Map Specification. */
  /* We need to separately specify high and low bytes to avoid endianness
     and alignment problems. */
  guint8 colorMapIndexLo, colorMapIndexHi;
  guint8 colorMapLengthLo, colorMapLengthHi;
  guint8 colorMapSize;

  /* Image Specification. */
  guint8 xOriginLo, xOriginHi;
  guint8 yOriginLo, yOriginHi;

  guint8 widthLo, widthHi;
  guint8 heightLo, heightHi;

  guint8 bpp;

  /* Image descriptor.
     3-0: attribute bpp
     4:   left-to-right ordering
     5:   top-to-bottom ordering
     7-6: zero
     */
#define TGA_DESC_ABITS      0x0f
#define TGA_DESC_HORIZONTAL 0x10
#define TGA_DESC_VERTICAL   0x20
  guint8 descriptor;
};

static struct
{
  guint32 extensionAreaOffset;
  guint32 developerDirectoryOffset;
#define TGA_SIGNATURE "TRUEVISION-XFILE"
  gchar signature[16];
  gchar dot;
  gchar null;
} tga_footer;


/* Declare some local functions.
 */
static void   query               (void);
static void   run                 (gchar   *name,
				   gint     nparams,
				   GParam  *param,
				   gint    *nreturn_vals,
				   GParam **return_vals);

static gint32 load_image           (gchar  *filename);
static gint   save_image           (gchar  *filename,
				    gint32  image_ID,
				    gint32  drawable_ID);

static gint   save_dialog          (void);
static void   save_ok_callback     (GtkWidget *widget,
				    gpointer   data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

#ifdef VERBOSE
static int verbose = VERBOSE;
#endif

static void
query (void)
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name entered" }
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);

  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" }
  };
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
    { PARAM_INT32, "rle", "Enable RLE compression" }
  } ;
  static gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_tga_load",
                          "Loads files of Targa file format",
                          "FIXME: write help for tga_load",
                          "Raphael FRANCOIS, Gordon Matzigkeit",
                          "Raphael FRANCOIS, Gordon Matzigkeit",
                          "1997",
                          "<Load>/TGA",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_tga_save",
                          "saves files in the Targa file format",
                          "FIXME: write help for tga_save",
			  "Raphael FRANCOIS, Gordon Matzigkeit",
                          "Raphael FRANCOIS, Gordon Matzigkeit",
                          "1997",
                          "<Save>/TGA",
			  "RGB*, GRAY*, INDEXED*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_load_handler ("file_tga_load",
			      "tga",
			      "");
		
  gimp_register_save_handler       ("file_tga_save",
				    "tga",
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

#ifdef PROFILE
  struct tms tbuf1, tbuf2;
#endif

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_EXECUTION_ERROR;

#ifdef VERBOSE
  if (verbose)
    printf ("TGA: RUN %s\n", name);
#endif

  if (strcmp (name, "file_tga_load") == 0)
    {
      INIT_I18N();

#ifdef PROFILE
      times (&tbuf1);
#endif

      image_ID = load_image (param[1].data.d_string);

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
    }
  else if (strcmp (name, "file_tga_save") == 0)
    {
      INIT_I18N_UI();
      gimp_ui_init ("tga", FALSE);

      image_ID     = param[1].data.d_int32;
      drawable_ID  = param[2].data.d_int32;

      /*  eventually export the image */ 
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	case RUN_WITH_LAST_VALS:
	  export = gimp_export_image (&image_ID, &drawable_ID, "TGA", 
				      (CAN_HANDLE_RGB |
				       CAN_HANDLE_GRAY |
				       CAN_HANDLE_INDEXED | 
				       CAN_HANDLE_ALPHA));
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
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_tga_save", &tsvals);

	  /*  First acquire information with a dialog  */
	  if (! save_dialog ())
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
	      tsvals.rle = (param[5].data.d_int32) ? TRUE : FALSE;
	    }
	  break;

	case RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_tga_save", &tsvals);
	  break;

	default:
	  break;
	}

#ifdef PROFILE
      times (&tbuf1);
#endif

      if (status == STATUS_SUCCESS)
	{
	  if (save_image (param[3].data.d_string, image_ID, drawable_ID))
	    {
	      /*  Store psvals data  */
	      gimp_set_data ("file_tga_save", &tsvals, sizeof (tsvals));
	    }
	  else
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

#ifdef PROFILE
  times (&tbuf2);
  printf ("TGA: %s profile: %ld user %ld system\n", name,
	  (long) tbuf2.tms_utime - tbuf1.tms_utime,
	  (long) tbuf2.tms_stime - tbuf2.tms_stime);
#endif
}

static gint32 ReadImage (FILE              *fp,
			 struct tga_header *hdr,
			 gchar             *filename);

static gint32
load_image (gchar *filename)
{
  FILE *fp;
  char * name_buf;
  struct tga_header hdr;

  gint32 image_ID = -1;

  fp = fopen (filename, "rb");
  if (!fp)
    {
      g_message ("TGA: can't open \"%s\"\n", filename);
      return -1;
    }

  name_buf = g_strdup_printf( _("Loading %s:"), filename);
  gimp_progress_init (name_buf);
  g_free (name_buf);

  /* Check the footer. */
  if (fseek (fp, 0L - (sizeof (tga_footer)), SEEK_END)
      || fread (&tga_footer, sizeof (tga_footer), 1, fp) != 1)
    {
      g_message ("TGA: Cannot read footer from \"%s\"\n", filename);
      return -1;
    }

  /* Check the signature. */
#ifdef VERBOSE
  if (memcmp (tga_footer.signature, TGA_SIGNATURE,
	      sizeof (tga_footer.signature)) == 0)
    {
      if (verbose)
	printf ("TGA: found New TGA\n");
    }
  else if (verbose)
    printf ("TGA: found Original TGA\n");
#endif

  if (fseek (fp, 0, SEEK_SET) ||
      fread (&hdr, sizeof (hdr), 1, fp) != 1)
    {
      g_message ("TGA: Cannot read header from \"%s\"\n", filename);
      return -1;
    }

  /* Skip the image ID field. */
  if (hdr.idLength && fseek (fp, hdr.idLength, SEEK_CUR))
    {
      g_message ("TGA: Cannot skip ID field in \"%s\"\n", filename);
      return -1;
    }

  image_ID = ReadImage (fp, &hdr, filename);
  fclose (fp);
  return image_ID;
}


#ifdef VERBOSE
static int totbytes = 0;
#endif

static int
std_fread (guchar *buf, 
	   int     datasize, 
	   int     nelems, 
	   FILE   *fp)
{
#ifdef VERBOSE
  if (verbose > 1)
    {
      totbytes += nelems * datasize;
      printf ("TGA: std_fread %d (total %d)\n",
	      nelems * datasize, totbytes);
    }
#endif

  return fread (buf, datasize, nelems, fp);
}

static int
std_fwrite (guchar *buf, 
	    int     datasize, 
	    int     nelems, 
	    FILE   *fp)
{
#ifdef VERBOSE
  if (verbose > 1)
    {
      totbytes += nelems * datasize;
      printf ("TGA: std_fwrite %d (total %d)\n",
	      nelems * datasize, totbytes);
    }
#endif

  return fwrite (buf, datasize, nelems, fp);
}

#define RLE_PACKETSIZE 0x80

/* Decode a bufferful of file. */
static int
rle_fread (guchar *buf, 
	   int     datasize, 
	   int     nelems, 
	   FILE   *fp)
{
  static guchar *statebuf = 0;
  static int statelen = 0;
  static int laststate = 0;

  int j, k;
  int buflen, count, bytes;
  guchar *p;
#ifdef VERBOSE
  int curbytes = totbytes;
#endif

  /* Scale the buffer length. */
  buflen = nelems * datasize;

  j = 0;
  while (j < buflen)
    {
      if (laststate < statelen)
	{
	  /* Copy bytes from our previously decoded buffer. */
	  bytes = MIN (buflen - j, statelen - laststate);
	  memcpy (buf + j, statebuf + laststate, bytes);
	  j += bytes;
	  laststate += bytes;

	  /* If we used up all of our state bytes, then reset them. */
	  if (laststate >= statelen)
	    {
	      laststate = 0;
	      statelen = 0;
	    }

	  /* If we filled the buffer, then exit the loop. */
	  if (j >= buflen)
	    break;
	}

      /* Decode the next packet. */
      count = fgetc (fp);
      if (count == EOF)
	{
#ifdef VERBOSE
	  if (verbose)
	    printf ("TGA: hit EOF while looking for count\n");
#endif
	  return j / datasize;
	}

      /* Scale the byte length to the size of the data. */
      bytes = ((count & ~RLE_PACKETSIZE) + 1) * datasize;

      if (j + bytes <= buflen)
	{
	  /* We can copy directly into the image buffer. */
	  p = buf + j;
	}
      else {
#if defined(PROFILE) || defined(VERBOSE)
	printf ("TGA: needed to use statebuf for %d bytes\n", buflen - j);
#endif
	/* Allocate the state buffer if we haven't already. */
	if (!statebuf)
	  statebuf = (guchar *) g_malloc (RLE_PACKETSIZE * datasize);
	p = statebuf;
      }

      if (count & RLE_PACKETSIZE)
	{
	  /* Fill the buffer with the next value. */
	  if (fread (p, datasize, 1, fp) != 1)
	    {
#ifdef VERBOSE
	      if (verbose)
		printf ("TGA: EOF while reading %d/%d element RLE packet\n",
			bytes, datasize);
#endif
	      return j / datasize;
	    }

	  /* Optimized case for single-byte encoded data. */
	  if (datasize == 1)
	    memset (p + 1, *p, bytes - 1);
	  else
	    for (k = datasize; k < bytes; k += datasize)
	      memcpy (p + k, p, datasize);
	}
      else
	{
	  /* Read in the buffer. */
	  if (fread (p, bytes, 1, fp) != 1)
	    {
#ifdef VERBOSE
	      if (verbose)
		printf ("TGA: EOF while reading %d/%d element raw packet\n",
			bytes, datasize);
#endif
	      return j / datasize;
	    }
	}

#ifdef VERBOSE
      if (verbose > 1)
	{
	  totbytes += bytes;
	  if (verbose > 2)
	    printf ("TGA: %s packet %d/%d\n",
		    (count & RLE_PACKETSIZE) ? "RLE" : "raw",
		    bytes, totbytes);
	}
#endif

      /* We may need to copy bytes from the state buffer. */
      if (p == statebuf)
	statelen = bytes;
      else
	j += bytes;
    }

#ifdef VERBOSE
  if (verbose > 1)
    {
      printf ("TGA: rle_fread %d/%d (total %d)\n",
	      nelems * datasize, totbytes - curbytes, totbytes);
    }
#endif
  return nelems;
}


/* This function is stateless, which means that we always finish packets
   on buffer boundaries.  As a beneficial side-effect, rle_fread
   never has to allocate a state buffer when it loads our files, provided
   it is called using the same buffer lengths!

   So, we get better compression than line-by-line encoders, and better
   loading performance than whole-stream images. */
/* RunLength Encode a bufferful of file. */
static int
rle_fwrite (guchar *buf, 
	    int     datasize, 
	    int     nelems, 
	    FILE   *fp)
{
  /* Now runlength-encode the whole buffer. */
  int count, j, buflen;
  guchar *begin;

#ifdef VERBOSE
  int curbytes = totbytes;
#endif

  /* Scale the buffer length. */
  buflen = datasize * nelems;

  begin = buf;
  j = datasize;
  while (j < buflen)
    {
      /* BUF[J] is our lookahead element, BEGIN is the beginning of this
	 run, and COUNT is the number of elements in this run. */
      if (memcmp (buf + j, begin, datasize) == 0)
	{
	  /* We have a run of identical characters. */
	  count = 1;
	  do
	    {
	      j += datasize;
	      count ++;
	    }
	  while (j < buflen && count < RLE_PACKETSIZE &&
		 memcmp (buf + j, begin, datasize) == 0);

	  /* J now either points to the beginning of the next run,
	     or close to the end of the buffer. */

	  /* Write out the run. */
	  if (fputc ((count - 1) | RLE_PACKETSIZE, fp) == EOF ||
	      fwrite (begin, datasize, 1, fp) != 1)
	    return 0;

#ifdef VERBOSE
	  if (verbose > 1)
	    {
	      totbytes += count * datasize;
	      if (verbose > 2)
		printf ("TGA: RLE packet %d/%d\n", count * datasize, totbytes);
	    }
#endif
	}
      else
	{
	  /* We have a run of raw characters. */
	  count = 0;
	  do
	    {
	      j += datasize;
	      count ++;
	    }
	  while (j < buflen && count < RLE_PACKETSIZE &&
		 memcmp (buf + j - datasize, buf + j, datasize) != 0);

	  /* Back up to the previous character. */
	  j -= datasize;

	  /* J now either points to the beginning of the next run,
	     or at the end of the buffer. */

	  /* Write out the raw packet. */
	  if (fputc (count - 1, fp) == EOF ||
	      fwrite (begin, datasize, count, fp) != count)
	    return 0;

#ifdef VERBOSE
	  if (verbose > 1)
	    {
	      totbytes += count * datasize;
	      if (verbose > 2)
		printf ("TGA: raw packet %d/%d\n", count * datasize, totbytes);
	    }
#endif
	}

      /* Set the beginning of the next run and the next lookahead. */
      begin = buf + j;
      j += datasize;
    }

  /* If we didn't encode all the elements, write one last packet. */
  if (begin < buf + buflen)
    {
#ifdef VERBOSE
      if (verbose > 1)
	{
	  totbytes += datasize;
	  if (verbose > 2)
	    printf ("TGA: FINAL raw packet %d/%d\n", datasize, totbytes);
	}
#endif
      if (fputc (0, fp) == EOF ||
	  fwrite (begin, datasize, 1, fp) != 1)
	return 0;
    }

#ifdef VERBOSE
  if (verbose > 1)
    {
      printf ("TGA: rle_fwrite %d/%d (total %d)\n",
	      totbytes - curbytes, nelems * datasize, totbytes);
    }
#endif
  return nelems;
}




static gint32
ReadImage (FILE              *fp, 
	   struct tga_header *hdr, 
	   char              *filename)
{
  static gint32 image_ID;
  gint32 layer_ID;

  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  guchar *data, *buffer;
  GDrawableType dtype;
  GImageType itype;
  guchar *alphas;

  int width, height, bpp, abpp, pbpp, nalphas;
  int i, j, k;
  int pelbytes, tileheight, wbytes, bsize, npels, pels;
  int rle, badread;

  int (*myfread)(guchar *, int, int, FILE *);

  /* Find out whether the image is horizontally or vertically reversed.
     The GIMP likes things left-to-right, top-to-bottom. */
  gchar horzrev = hdr->descriptor & TGA_DESC_HORIZONTAL;
  gchar vertrev = !(hdr->descriptor & TGA_DESC_VERTICAL);

  /* Reassemble the multi-byte values correctly, regardless of
     host endianness. */
  width = (hdr->widthHi << 8) | hdr->widthLo;
  height = (hdr->heightHi << 8) | hdr->heightLo;

  bpp = hdr->bpp;
  abpp = hdr->descriptor & TGA_DESC_ABITS;

  if (hdr->imageType == TGA_TYPE_COLOR ||
      hdr->imageType == TGA_TYPE_COLOR_RLE)
    pbpp = MIN (bpp / 3, 8) * 3;
  else if (abpp < bpp)
    pbpp = bpp - abpp;
  else
    pbpp = bpp;

  if (abpp + pbpp > bpp)
    {
      printf ("TGA: %d bit image, %d bit alpha is greater than %d total bits per pixel\n",
	      pbpp, abpp, bpp);

      /* Assume that alpha bits were set incorrectly. */
      abpp = bpp - pbpp;
      printf ("TGA: reducing to %d bit alpha\n", abpp);
    }
  else if (abpp + pbpp < bpp)
    {
      printf ("TGA: %d bit image, %d bit alpha is less than %d total bits per pixel\n",
	      pbpp, abpp, bpp);

      /* Again, assume that alpha bits were set incorrectly. */
      abpp = bpp - pbpp;
      printf ("TGA: increasing to %d bit alpha\n", abpp);
    }

  rle = 0;
  switch (hdr->imageType)
    {
    case TGA_TYPE_MAPPED_RLE:
      rle = 1;
    case TGA_TYPE_MAPPED:
      itype = INDEXED;

      /* Find the size of palette elements. */
      pbpp = MIN (hdr->colorMapSize / 3, 8) * 3;
      if (pbpp < hdr->colorMapSize)
	abpp = hdr->colorMapSize - pbpp;
      else
	abpp = 0;

#ifdef VERBOSE
      if (verbose)
	printf ("TGA: %d bit indexed image, %d bit alpha (%d bit indices)\n",
		pbpp, abpp, bpp);
#endif

      if (bpp != 8)
	{
	  /* We can only cope with 8-bit indices. */
	  printf ("TGA: index sizes other than 8 bits are unimplemented\n");
	  return -1;
	}

      if (abpp)
	dtype = INDEXEDA_IMAGE;
      else
	dtype = INDEXED_IMAGE;
      break;

    case TGA_TYPE_GRAY_RLE:
      rle = 1;
    case TGA_TYPE_GRAY:
      itype = GRAY;
#ifdef VERBOSE
      if (verbose)
	printf ("TGA: %d bit grayscale image, %d bit alpha\n",
		pbpp, abpp);
#endif

      if (abpp)
	dtype = GRAYA_IMAGE;
      else
	dtype = GRAY_IMAGE;
      break;

    case TGA_TYPE_COLOR_RLE:
      rle = 1;
    case TGA_TYPE_COLOR:
      itype = RGB;
#ifdef VERBOSE
      if (verbose)
	printf ("TGA: %d bit color image, %d bit alpha\n", pbpp, abpp);
#endif

      if (abpp)
	dtype = RGBA_IMAGE;
      else
	dtype = RGB_IMAGE;
      break;

    default:
      printf ("TGA: unrecognized image type %d\n", hdr->imageType);
      return -1;
    }

  if ((abpp && abpp != 8) ||
      ((itype == RGB || itype == INDEXED) && pbpp != 24) ||
      (itype == GRAY && pbpp != 8))
    {
      /* FIXME: We haven't implemented bit-packed fields yet. */
      printf ("TGA: channel sizes other than 8 bits are unimplemented\n");
      return -1;
    }

  /* Check that we have a color map only when we need it. */
  if (itype == INDEXED)
    {
      if (hdr->colorMapType != 1)
	{
	  printf ("TGA: indexed image has invalid color map type %d\n",
		  hdr->colorMapType);
	  return -1;
	}
    }
  else if (hdr->colorMapType != 0)
    {
      printf ("TGA: non-indexed image has invalid color map type %d\n",
	      hdr->colorMapType);
      return -1;
    }

  image_ID = gimp_image_new (width, height, itype);
  gimp_image_set_filename (image_ID, filename);

  alphas = 0;
  nalphas = 0;
  if (hdr->colorMapType == 1)
    {
      /* We need to read in the colormap. */
      int index, length, colors;
      int tmp;

      guchar *cmap;

      index = (hdr->colorMapIndexHi << 8) | hdr->colorMapIndexLo;
      length = (hdr->colorMapLengthHi << 8) | hdr->colorMapLengthLo;

#ifdef VERBOSE
      if (verbose)
	printf ("TGA: reading color map (%d + %d) * (%d bits)\n",
		index, length, hdr->colorMapSize);
#endif
      if (length == 0)
	{
	  printf ("TGA: invalid color map length %d\n", length);
	  gimp_image_delete (image_ID);
	  return -1;
	}

      pelbytes = ROUNDUP_DIVIDE (hdr->colorMapSize, 8);
      colors = length + index;
      cmap = g_malloc (colors * pelbytes);

      /* Zero the entries up to the beginning of the map. */
      memset (cmap, 0, index * pelbytes);

      /* Read in the rest of the colormap. */
      if (fread (cmap + (index * pelbytes), pelbytes, length, fp) != length)
	{
	  printf ("TGA: error reading colormap (ftell == %ld)\n", ftell (fp));
	  gimp_image_delete (image_ID);
	  return -1;
	}

      /* If we have an alpha channel, then create a mapping to the alpha
	 values. */
      if (pelbytes > 3)
	alphas = (guchar *) g_malloc (colors);

      k = 0;
      for (j = 0; j < colors * pelbytes; j += pelbytes)
	{
	  /* Swap from BGR to RGB. */
	  tmp = cmap[j];
	  cmap[k ++] = cmap[j + 2];
	  cmap[k ++] = cmap[j + 1];
	  cmap[k ++] = tmp;

	  /* Take the alpha values out of the colormap. */
	  if (alphas)
	    alphas[nalphas ++] = cmap[j + 3];
	}

      /* If the last color was transparent, then omit it from the
	 GIMP mapping. */
      if (nalphas && alphas[nalphas - 1] == 0)
	colors --;

      /* Set the colormap. */
      gimp_image_set_cmap (image_ID, cmap, colors);
      g_free (cmap);

      /* Now pretend as if we only have 8 bpp. */
      abpp = 0;
      pbpp = 8;
    }

  /* Continue initializing. */
  layer_ID = gimp_layer_new (image_ID,
			     _("Background"),
			     width, height,
			     dtype,
			     100,
			     NORMAL_MODE);

  gimp_image_add_layer (image_ID, layer_ID, 0);

  drawable = gimp_drawable_get (layer_ID);

  /* Prepare the pixel region. */
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, TRUE, FALSE);

  /* Calculate number of GIMP bytes per pixel. */
  pelbytes = drawable->bpp;

  /* Calculate TGA bytes per pixel. */
  bpp = ROUNDUP_DIVIDE (pbpp + abpp, 8);

  /* Allocate the data. */
  tileheight = gimp_tile_height ();
  data = (guchar *) g_malloc (width * tileheight * pelbytes);

  /* Maybe we need to reverse the data. */
  buffer = 0;
  if (horzrev || vertrev)
    buffer = (guchar *) g_malloc (width * tileheight * pelbytes);

  if (rle)
    myfread = rle_fread;
  else
    myfread = std_fread;

  wbytes = width * pelbytes;
  badread = 0;
  for (i = 0; i < height; i += tileheight)
    {
      tileheight = MIN (tileheight, height - i);

#ifdef VERBOSE
      if (verbose > 1)
	printf ("TGA: reading %dx(%d+%d)x%d pixel region\n", width,
		(vertrev ? (height - (i + tileheight)) : i),
		tileheight, pelbytes);
#endif

      npels = width * tileheight;
      bsize = wbytes * tileheight;

      /* Suck in the data one tileheight at a time. */
      if (badread)
	pels = 0;
      else
	pels = (*myfread) (data, bpp, npels, fp);

      if (pels != npels)
	{
	  if (!badread)
	    {
	      /* Probably premature end of file. */
	      printf ("TGA: error reading (ftell == %ld)\n", ftell (fp));
	      badread = 1;
	    }

#if 0
  verbose = 2;
  printf ("TGA: debug %d\n", getpid ());
  kill (getpid (), 19);
#endif

	  /* Fill the rest of this tile with zeros. */
	  memset (data + (pels * bpp), 0, ((npels - pels) * bpp));
	}

      /* If we have indexed alphas, then set them. */
      if (nalphas)
	{
	  /* Start at the end of the buffer, and work backwards. */
	  k = (npels - 1) * bpp;
	  for (j = bsize - pelbytes; j >= 0; j -= pelbytes)
	    {
	      /* Find the alpha for this index. */
	      data[j + 1] = alphas[data[k]];
	      data[j] = data[k --];
	    }
	}

      if (pelbytes >= 3)
	{
	  /* Rearrange the colors from BGR to RGB. */
	  int tmp;
	  for (j = 0; j < bsize; j += pelbytes)
	    {
	      tmp = data[j];
	      data[j] = data[j + 2];
	      data[j + 2] = tmp;
	    }
	}


      if (horzrev || vertrev)
	{
	  guchar *tmp;
	  if (vertrev)
	    {
	      /* We need to mirror only vertically. */
	      for (j = 0; j < bsize; j += wbytes)
		memcpy (buffer + j,
			data + bsize - (j + wbytes), wbytes);
	    }
	  else if (horzrev)
	    {
	      /* We need to mirror only horizontally. */
	      for (j = 0; j < bsize; j += wbytes)
		for (k = 0; k < wbytes; k += pelbytes)
		  memcpy (buffer + k + j,
			  data + (j + wbytes) - (k + pelbytes), pelbytes);
	    }
	  else
	    {
	      /* Completely reverse the pixels in the buffer. */
	      for (j = 0; j < bsize; j += pelbytes)
		memcpy (buffer + j,
			data + bsize - (j + pelbytes), pelbytes);
	    }

	  /* Swap the buffers because we modified them. */
	  tmp = buffer;
	  buffer = data;
	  data = tmp;
	}

      gimp_progress_update ((double) (i + tileheight) / (double) height);

      /* If vertically reversed, put the data at the end. */
      gimp_pixel_rgn_set_rect
	(&pixel_rgn, data,
	 0, (vertrev ? (height - (i + tileheight)) : i),
	 width, tileheight);
    }

  if (fgetc (fp) != EOF)
    printf ("TGA: too much input data, ignoring extra...\n");

  g_free (data);
  g_free (buffer);

  if (alphas)
    g_free (alphas);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return image_ID;
}  /*read_image*/


static gint
save_image (gchar  *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType dtype;
  int width, height;
  FILE *fp;
  guchar *name_buf;
  int i, j, k;
  int npels, tileheight, pelbytes, bsize;
  int transparent, status;

  struct tga_header hdr;

  int (*myfwrite)(guchar *, int, int, FILE *);
  guchar *data;

  drawable = gimp_drawable_get(drawable_ID);
  dtype = gimp_drawable_type(drawable_ID);
  width = drawable->width;
  height = drawable->height;

  name_buf = g_strdup_printf( _("Saving %s:"), filename);
  gimp_progress_init((char *)name_buf);
  g_free(name_buf);

  memset (&hdr, 0, sizeof (hdr));
  /* We like our images top-to-bottom, thank you! */
  hdr.descriptor |= TGA_DESC_VERTICAL;

  /* Choose the imageType based on our drawable and compression option. */
  switch (dtype)
    {
    case INDEXEDA_IMAGE:
    case INDEXED_IMAGE:
      hdr.bpp = 8;
      hdr.imageType = TGA_TYPE_MAPPED;
      break;

    case GRAYA_IMAGE:
      hdr.bpp = 8;
      hdr.descriptor |= 8;
    case GRAY_IMAGE:
      hdr.bpp += 8;
      hdr.imageType = TGA_TYPE_GRAY;
      break;

    case RGBA_IMAGE:
      hdr.bpp = 8;
      hdr.descriptor |= 8;
    case RGB_IMAGE:
      hdr.bpp += 24;
      hdr.imageType = TGA_TYPE_COLOR;
      break;

    default:
      return FALSE;
    }

  if (tsvals.rle)
    {
      /* Here we take advantage of the fact that the RLE image type codes
	 are exactly 8 greater than the non-RLE. */
      hdr.imageType += 8;
      myfwrite = rle_fwrite;
    }
  else
    myfwrite = std_fwrite;

  hdr.widthLo = (width & 0xff);
  hdr.widthHi = (width >> 8);

  hdr.heightLo = (height & 0xff);
  hdr.heightHi = (height >> 8);

  if((fp = fopen(filename, "wb")) == NULL)
    {
      g_message ("TGA: can't create \"%s\"\n", filename);
      return FALSE;
    }

  /* Mark our save ID. */
  hdr.idLength = strlen (SAVE_ID_STRING);

  /* See if we have to write out the colour map. */
  transparent = 0;
  if (hdr.imageType == TGA_TYPE_MAPPED_RLE ||
      hdr.imageType == TGA_TYPE_MAPPED)
    {
      guchar *cmap;
      int colors;

      hdr.colorMapType = 1;
      cmap = gimp_image_get_cmap (image_ID, &colors);
      if (colors > 256)
	{
	  g_message ("TGA: cannot handle colormap with more than 256 colors (got %d)\n", colors);
	  return FALSE;
	}

      /* If we already have more than 256 colors, then ignore the
	 alpha channel.  Otherwise, create an entry for any completely
	 transparent pixels. */
      if (dtype == INDEXEDA_IMAGE && colors < 256)
	{
	  transparent = colors;
	  hdr.colorMapSize = 32;
	  hdr.colorMapLengthLo = ((colors + 1) & 0xff);
	  hdr.colorMapLengthHi = ((colors + 1) >> 8);
	}
      else
	{
	  hdr.colorMapSize = 24;
	  hdr.colorMapLengthLo = (colors & 0xff);
	  hdr.colorMapLengthHi = (colors >> 8);
	}

      /* Write the header. */
      if (fwrite(&hdr, sizeof (hdr), 1, fp) != 1)
	return FALSE;
      if (fwrite (SAVE_ID_STRING, hdr.idLength, 1, fp) != 1)
	return FALSE;

      pelbytes = ROUNDUP_DIVIDE (hdr.colorMapSize, 8);
      if (transparent)
	{
	  guchar *newcmap;

	  /* Reallocate our colormap to have an alpha channel and
	     a fully transparent color. */
	  newcmap = (guchar *) g_malloc ((colors + 1) * pelbytes);

	  k = 0;
	  for (j = 0; j < colors * 3; j += 3)
	    {
	      /* Rearrange from RGB to BGR. */
	      newcmap[k ++] = cmap[j + 2];
	      newcmap[k ++] = cmap[j + 1];
	      newcmap[k ++] = cmap[j];

	      /* Set to maximum opacity. */
	      newcmap[k ++] = -1;
	    }

	  /* Add the transparent color. */
	  memset (newcmap + k, 0, pelbytes);

	  /* Write out the colormap. */
	  if (fwrite (newcmap, pelbytes, colors + 1, fp) != colors + 1)
	    return FALSE;
	  g_free (newcmap);
	}
      else
	{
	  /* Rearrange the colors from RGB to BGR. */
	  int tmp;
	  for (j = 0; j < colors * pelbytes; j += pelbytes)
	    {
	      tmp = cmap[j];
	      cmap[j] = cmap[j + 2];
	      cmap[j + 2] = tmp;
	    }

	  /* Write out the colormap. */
	  if (fwrite (cmap, pelbytes, colors, fp) != colors)
	    return FALSE;
	}

      g_free (cmap);
    }
  /* Just write the header. */
  else
    {
      if (fwrite(&hdr, sizeof (hdr), 1, fp) != 1)
	return FALSE;
      if (fwrite (SAVE_ID_STRING, hdr.idLength, 1, fp) != 1)
	return FALSE;
    }

  /* Allocate a new set of pixels. */
  tileheight = gimp_tile_height ();
  data = (guchar *) g_malloc(width * tileheight * drawable->bpp);

  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* Write out the pixel data. */
  pelbytes = ROUNDUP_DIVIDE (hdr.bpp, 8);
  status = TRUE;
  for (i = 0; i < height; i += tileheight)
    {
      /* Get a horizontal slice of the image. */
      tileheight = MIN(tileheight, height - i);
      gimp_pixel_rgn_get_rect(&pixel_rgn, data, 0, i, width, tileheight);

#ifdef VERBOSE
      if (verbose > 1)
	printf ("TGA: writing %dx(%d+%d)x%d pixel region\n",
		width, i, tileheight, pelbytes);
#endif

      npels = width * tileheight;
      bsize = npels * pelbytes;

      /* If the GIMP's bpp does not match the TGA format, strip
	 the excess bytes. */
      if (drawable->bpp > pelbytes)
	{
	  int nbytes, difference, fullbsize;

	  j = k = 0;
	  fullbsize = npels * drawable->bpp;

	  difference = drawable->bpp - pelbytes;
	  while (j < fullbsize)
	    {
	      nbytes = 0;
	      for (nbytes = 0; nbytes < pelbytes; nbytes ++)
		/* Be careful to handle overlapping pixels. */
		data[k ++] = data[j ++];

	      /* If this is an indexed image, and data[j] (alpha
		 channel) is zero, then we should write our transparent
		 pixel's index. */
	      if (dtype == INDEXEDA_IMAGE && transparent && data[j] == 0)
		data[k - 1] = transparent;

	      /* Increment J to the next pixel. */
	      j += difference;
	    }
	}

      if (pelbytes >= 3)
	{
	  /* Rearrange the colors from RGB to BGR. */
	  int tmp;
	  for (j = 0; j < bsize; j += pelbytes)
	    {
	      tmp = data[j];
	      data[j] = data[j + 2];
	      data[j + 2] = tmp;
	    }
	}

      if ((*myfwrite) (data, pelbytes, npels, fp) != npels)
	{
	  /* We have no choice but to break and truncate the file
	     if we are writing with RLE. */
	  status = FALSE;
	  break;
	}

      gimp_progress_update ((double) (i + tileheight) / (double) height);
    }

  gimp_drawable_detach (drawable);
  g_free (data);

  fclose (fp);
  return status;
}

static gint
save_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;

  dlg = gimp_dialog_new (_("Save as TGA"), "tga",
			 gimp_plugin_help_func, "filters/tga.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), save_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* regular tga parameter settings */
  frame = gtk_frame_new (_("Targa Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /*  rle  */
  toggle = gtk_check_button_new_with_label (_("RLE compression"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &tsvals.rle);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), tsvals.rle);
  gtk_widget_show (toggle);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return tsint.run;
}

static void
save_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  tsint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
