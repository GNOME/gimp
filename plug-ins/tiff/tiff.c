/* tiff loading and saving for the GIMP
 *  -Peter Mattis
 * The TIFF loading code has been completely revamped by Nick Lamb
 * njl195@zepler.org.uk -- 18 May 1998
 *
 * The code for this filter is based on "tifftopnm" and "pnmtotiff",
 *  2 programs that are a part of the netpbm package.
 */

/*
** tifftopnm.c - converts a Tagged Image File to a portable anymap
**
** Derived by Jef Poskanzer from tif2ras.c, which is:
**
** Copyright (c) 1990 by Sun Microsystems, Inc.
**
** Author: Patrick J. Naughton
** naughton@wind.sun.com
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted,
** provided that the above copyright notice appear in all copies and that
** both that copyright notice and this permission notice appear in
** supporting documentation.
**
** This file is provided AS IS with no warranties of any kind.  The author
** shall have no liability with respect to the infringement of copyrights,
** trade secrets or any patents by this file or any part thereof.  In no
** event will the author be liable for any lost revenue or profits or
** other special, indirect and consequential damages.
*/

#include <stdlib.h>
#include <string.h>
#include <tiffio.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

typedef struct
{
  gint  compression;
  gint  fillorder;
} TiffSaveVals;

typedef struct
{
  gint  run;
} TiffSaveInterface;

/* Declare some local functions.
 */
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
static gint32 load_image (char   *filename);
static gint   save_image (char   *filename,
			  gint32  image,
			  gint32  drawable);

static gint   save_dialog ();

static void   save_close_callback  (GtkWidget *widget,
				    gpointer   data);
static void   save_ok_callback     (GtkWidget *widget,
				    gpointer   data);
static void   save_toggle_update   (GtkWidget *widget,
				    gpointer   data);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static TiffSaveVals tsvals =
{
  COMPRESSION_LZW,    /*  compression  */
  FILLORDER_LSB2MSB,  /*  fillorder    */
};

static TiffSaveInterface tsint =
{
  FALSE                /*  run  */
};


MAIN ()

static void
query ()
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
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
    { PARAM_INT32, "compression", "Compression type: { NONE (0), LZW (1), PACKBITS (2)" },
    { PARAM_INT32, "fillorder", "Fill Order: { MSB to LSB (0), LSB to MSB (1)" }
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_tiff_load",
                          "loads files of the tiff file format",
                          "FIXME: write help for tiff_load",
                          "Spencer Kimball, Peter Mattis & Nick Lamb",
                          "Nick Lamb <njl195@zepler.org.uk>",
                          "1995-1996,1998",
                          "<Load>/Tiff",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_tiff_save",
                          "saves files in the tiff file format",
                          "FIXME: write help for tiff_save",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1996",
                          "<Save>/Tiff",
			  "RGB*, GRAY*, INDEXED",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_tiff_load", "tif,tiff", "",
             "0,string,II*\\0,0,string,MM\\0*");
  gimp_register_save_handler ("file_tiff_save", "tif,tiff", "");
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
  gint32 image;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_tiff_load") == 0)
    {
      image = load_image (param[1].data.d_string);

      if (image != -1)
	{
	  *nreturn_vals = 2;
	  values[0].data.d_status = STATUS_SUCCESS;
	  values[1].type = PARAM_IMAGE;
	  values[1].data.d_image = image;
	}
      else
	{
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_tiff_save") == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_tiff_save", &tsvals);

	  /*  First acquire information with a dialog  */
	  if (! save_dialog ())
	    return;
	  break;

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 7)
	    status = STATUS_CALLING_ERROR;
	  if (status == STATUS_SUCCESS)
	    {
	      switch (param[5].data.d_int32)
		{
		case 0: tsvals.compression = COMPRESSION_NONE;     break;
		case 1: tsvals.compression = COMPRESSION_LZW;      break;
		case 2: tsvals.compression = COMPRESSION_PACKBITS; break;
		default: status = STATUS_CALLING_ERROR; break;
		}
	      switch (param[6].data.d_int32)
		{
		case 0: tsvals.fillorder = FILLORDER_MSB2LSB; break;
		case 1: tsvals.fillorder = FILLORDER_LSB2MSB; break;
		default: status = STATUS_CALLING_ERROR; break;
		}
	    }

	case RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_tiff_save", &tsvals);
	  break;

	default:
	  break;
	}

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string, param[1].data.d_int32, param[2].data.d_int32))
	{
	  /*  Store mvals data  */
	  gimp_set_data ("file_tiff_save", &tsvals, sizeof (TiffSaveVals));

	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
}

static gint32 load_image (char *filename) {
  TIFF *tif;
  unsigned short bps, spp, photomet;
  int cols, rows, maxval, alpha;
  int image, layer, tile_height;
  unsigned short *redmap, *greenmap, *bluemap;
  guchar cmap[768];
  int image_type= 0, layer_type= 0;
  unsigned short extra, *extra_types;

  int col, row, start, i, j;
  unsigned char sample;
  int bitsleft;
  int gray_val, red_val, green_val, blue_val, alpha_val;

  guchar *source, *s, *dest, *d;
  GDrawable *drawable;
  GPixelRgn pixel_rgn;
  char *name;

  guchar colors[3]= {0, 0, 0};

  typedef struct {
    gint32 ID;
    GDrawable *drawable;
    GPixelRgn pixel_rgn;
    guchar *pixels;
    guchar *pixel;
  } channel_data;

  channel_data *channel= NULL;

  tif = TIFFOpen (filename, "r");
  if (!tif) {
    g_message("TIFF Can't open \n%s", filename);
    gimp_quit ();
  }

  name = g_malloc (strlen (filename) + 12);
  sprintf (name, "Loading %s:", filename);
  gimp_progress_init (name);
  g_free (name);

  if (!TIFFGetField (tif, TIFFTAG_BITSPERSAMPLE, &bps))
    bps = 1;

  if (bps > 8) {
    g_message("TIFF Can't handle samples wider than 8-bit");
    gimp_quit();
  }

  if (!TIFFGetField (tif, TIFFTAG_SAMPLESPERPIXEL, &spp))
    spp = 1;
  if (!TIFFGetField (tif, TIFFTAG_EXTRASAMPLES, &extra, &extra_types))
    extra = 0;

  if (!TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &cols)) {
    g_message("TIFF Can't get image width");
    gimp_quit ();
  }

  if (!TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &rows)) {
    g_message("TIFF Can't get image length");
    gimp_quit ();
  }

  if (!TIFFGetField (tif, TIFFTAG_PHOTOMETRIC, &photomet)) {
    g_message("TIFF Can't get photometric");
    gimp_quit ();
  }

  /* test if the extrasample represents an associated alpha channel... */
  if (extra > 0 && (extra_types[0] == EXTRASAMPLE_ASSOCALPHA)) {
    alpha = 1;
  } else {
    alpha = 0;
  }

  if (photomet == PHOTOMETRIC_RGB && spp > extra + 3) {
    extra= spp - 3; 
    alpha= 1;
  } else if (photomet != PHOTOMETRIC_RGB && spp > extra + 1) {
    extra= spp - 1;
    alpha= 1;
  }

  maxval = (1 << bps) - 1;

  switch (photomet) {
    case PHOTOMETRIC_MINISBLACK:
    case PHOTOMETRIC_MINISWHITE:
      image_type = GRAY;
      layer_type = (alpha) ? GRAYA_IMAGE : GRAY_IMAGE;
      break;

    case PHOTOMETRIC_RGB:
      image_type = RGB;
      layer_type = (alpha) ? RGBA_IMAGE : RGB_IMAGE;
      break;

    case PHOTOMETRIC_PALETTE:
      image_type = INDEXED;
      layer_type = (alpha) ? INDEXEDA_IMAGE : INDEXED_IMAGE;
      break;

    case PHOTOMETRIC_MASK:
      g_message ("TIFF Can't handle PHOTOMETRIC_MASK");
      gimp_quit ();
      break;
    default:
      g_message ("TIFF Unknown photometric\n Number %d", photomet);
      gimp_quit ();
  }

  if ((image = gimp_image_new (cols, rows, image_type)) == -1) {
    g_message("TIFF Can't create a new image\n");
    gimp_quit ();
  }
  gimp_image_set_filename (image, filename);

  /* Install colormap for INDEXED images only */
  if (image_type == INDEXED) {
    if (!TIFFGetField (tif, TIFFTAG_COLORMAP, &redmap, &greenmap, &bluemap)) {
      g_message("TIFF Can't get colormaps");
      gimp_quit ();
    }

    for (i = 0, j = 0; i <= maxval; i++) {
      cmap[j++] = redmap[i] >> 8;
      cmap[j++] = greenmap[i] >> 8;
      cmap[j++] = bluemap[i] >> 8;
    }
    gimp_image_set_cmap (image, cmap, maxval + 1);
  }

  layer = gimp_layer_new (image, "Background", cols, rows, layer_type,
			     100, NORMAL_MODE);
  gimp_image_add_layer (image, layer, 0);
  drawable = gimp_drawable_get (layer);

  source= g_new (guchar, TIFFScanlineSize (tif));
  tile_height = gimp_tile_height ();
  dest = g_new (guchar, tile_height * cols * drawable->bpp);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, cols, rows, TRUE, FALSE);

  if (extra - alpha > 0)
    channel = g_new (channel_data, extra - alpha);

  /* Add alpha channels as appropriate */
  for (i= 0; alpha + i < extra; ++i) {
    channel[i].ID= gimp_channel_new(image, "TIFF Channel", cols, rows,
                                                          100.0, colors);
    gimp_image_add_channel(image, channel[i].ID, 0);
    channel[i].drawable= gimp_drawable_get (channel[i].ID);
    channel[i].pixels= g_new(guchar, tile_height * cols);

    gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable, 0, 0,
			   cols, rows, TRUE, FALSE);
  }

/* Step through all <= 8-bit samples in an image */

#define NEXTSAMPLE(var)                       \
  {                                           \
      if (bitsleft == 0)                      \
      {                                       \
	  s++;                                \
	  bitsleft = 8;                       \
      }                                       \
      bitsleft -= bps;                        \
      var = ( *s >> bitsleft ) & maxval;      \
  }

  for (start= 0, row = 0; row < rows; ++row) {
    d= dest + cols * (row % tile_height) * drawable->bpp;

    /* Special cases: Scanline is compatible with GIMP storage */
    if (extra == 0 && bps == 8) {  
      if (TIFFReadScanline (tif, d, row, 0) < 0) {
        g_message("TIFF Bad data read on line %d", row);
        gimp_quit ();
      }

    /* Or read in and process each sample -- slower */
    } else {
      if (TIFFReadScanline (tif, source, row, 0) < 0) {
        g_message("TIFF Bad data read on line %d\n", row);
        gimp_quit ();
      }

      for (i= 0; alpha + i < extra; ++i) {
        channel[i].pixel= channel[i].pixels + cols * (row % tile_height);
      }

      /* Set s/bitleft ready to use NEXTSAMPLE macro */

      s= source;
      bitsleft= 8;

      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
          for (col = 0; col < cols; col++) {
            NEXTSAMPLE(gray_val);
            if (alpha) {
              NEXTSAMPLE(alpha_val);
              if (alpha_val)
                *d++ = (gray_val * 65025) / (alpha_val * maxval);
              else
                *d++ = 0;
              *d++ = alpha_val;
            } else {
              *d++ = (gray_val * 255) / maxval;
            }
            for (i= 0; alpha + i < extra; ++i) {
              NEXTSAMPLE(sample);
              *channel[i].pixel++ = sample;
            }
	  }
	  break;

        case PHOTOMETRIC_MINISWHITE:
	  for (col = 0; col < cols; col++) {
            NEXTSAMPLE(gray_val);
            if (alpha) {
              NEXTSAMPLE(alpha_val);
              if (alpha_val)
                *d++ = ((maxval - gray_val) * 65025) / (alpha_val * maxval);
              else
                *d++ = 0;
              *d++ = alpha_val;
            } else {
              *d++ = ((maxval - gray_val) * 255) / maxval;
            }
            for (i= 0; alpha + i < extra; ++i) {
              NEXTSAMPLE(sample);
              *channel[i].pixel++ = sample;
            }
          }
          break;

        case PHOTOMETRIC_PALETTE:
          for (col = 0; col < cols; col++) {
            NEXTSAMPLE(*d++);
            if (alpha) {
              NEXTSAMPLE(*d++);
            }
            for (i= 0; alpha + i < extra; ++i) {
              NEXTSAMPLE(sample);
              *channel[i].pixel++ = sample;
            }
          }
          break;
  
        case PHOTOMETRIC_RGB:
          for (col = 0; col < cols; col++) {
            NEXTSAMPLE(red_val)
            NEXTSAMPLE(green_val)
            NEXTSAMPLE(blue_val)
            if (alpha) {
              NEXTSAMPLE(alpha_val)
              if (alpha_val) {
                *d++ = (red_val * 255) / alpha_val;
                *d++ = (green_val * 255) / alpha_val;
                *d++ = (blue_val * 255) / alpha_val;
              } else {
                *d++ = 0;
                *d++ = 0;
                *d++ = 0;
	      }
	      *d++ = alpha_val;
	    } else {
	      *d++ = red_val;
	      *d++ = green_val;
	      *d++ = blue_val;
	    }
	    for (i= 0; alpha + i < extra; ++i) {
              NEXTSAMPLE(sample);
	      *channel[i].pixel++ = sample;
            }
          }
          break;

        default:
          /* This case was handled earlier */
          g_assert_not_reached();
      }
    }

    
    if (((row + 1) % tile_height) == 0 || row + 1 == rows) {
      gimp_pixel_rgn_set_rect (&pixel_rgn, dest, 0, start, cols, 1+row-start);
      for (i= 0; alpha + i < extra; ++i) {
	gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                                0, start, cols, 1+row-start);
      }
      gimp_progress_update ((double) row / (double) rows);
      start= row + 1;
    }
  }

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return image;
}

/*
** pnmtotiff.c - converts a portable anymap to a Tagged Image File
**
** Derived by Jef Poskanzer from ras2tif.c, which is:
**
** Copyright (c) 1990 by Sun Microsystems, Inc.
**
** Author: Patrick J. Naughton
** naughton@wind.sun.com
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted,
** provided that the above copyright notice appear in all copies and that
** both that copyright notice and this permission notice appear in
** supporting documentation.
**
** This file is provided AS IS with no warranties of any kind.  The author
** shall have no liability with respect to the infringement of copyrights,
** trade secrets or any patents by this file or any part thereof.  In no
** event will the author be liable for any lost revenue or profits or
** other special, indirect and consequential damages.
*/

static gint save_image (char *filename, gint32 image, gint32 layer) {
  TIFF *tif;
  unsigned short red[256];
  unsigned short grn[256];
  unsigned short blu[256];
  int cols, col, rows, row, i;
  long g3options;
  long rowsperstrip;
  unsigned short compression;
  unsigned short fillorder;
  unsigned short extra_samples[1];
  int alpha;
  short predictor;
  short photometric;
  short samplesperpixel;
  short bitspersample;
  int bytesperrow;
  guchar *t, *src, *data;
  guchar *cmap;
  int colors;
  int success;
  GDrawable *drawable;
  GDrawableType drawable_type;
  GPixelRgn pixel_rgn;
  int tile_height;
  int y, yend;
  char *name;

  compression = tsvals.compression;
  fillorder = tsvals.fillorder;

  g3options = 0;
  predictor = 0;
  rowsperstrip = 0;

  tif = TIFFOpen (filename, "w");
  if (!tif) {
    g_print ("Can't write image to\n%s", filename);
    return 0;
  }

  name = malloc (strlen (filename) + 11);
  sprintf (name, "Saving %s:", filename);
  gimp_progress_init (name);
  free (name);

  drawable = gimp_drawable_get (layer);
  drawable_type = gimp_drawable_type (layer);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

  cols = drawable->width;
  rows = drawable->height;

  switch (drawable_type)
    {
    case RGB_IMAGE:
      samplesperpixel = 3;
      bitspersample = 8;
      photometric = PHOTOMETRIC_RGB;
      bytesperrow = cols * 3;
      alpha = 0;
      break;
    case GRAY_IMAGE:
      samplesperpixel = 1;
      bitspersample = 8;
      photometric = PHOTOMETRIC_MINISBLACK;
      bytesperrow = cols;
      alpha = 0;
      break;
    case RGBA_IMAGE:
      samplesperpixel = 4;
      bitspersample = 8;
      photometric = PHOTOMETRIC_RGB;
      bytesperrow = cols * 4;
      alpha = 1;
      break;
    case GRAYA_IMAGE:
      samplesperpixel = 2;
      bitspersample = 8;
      photometric = PHOTOMETRIC_MINISBLACK;
      bytesperrow = cols * 2;
      alpha = 1;
      break;
    case INDEXED_IMAGE:
      samplesperpixel = 1;
      bitspersample = 8;
      photometric = PHOTOMETRIC_PALETTE;
      bytesperrow = cols;
      alpha = 0;

      cmap = gimp_image_get_cmap (image, &colors);

      for (i = 0; i < colors; i++)
	{
	  red[i] = *cmap++ * 65535 / 255;
	  grn[i] = *cmap++ * 65535 / 255;
	  blu[i] = *cmap++ * 65535 / 255;
	}
      break;
    case INDEXEDA_IMAGE:
      return 0;
    }

  if (rowsperstrip == 0)
    rowsperstrip = (8 * 1024) / bytesperrow;
  if (rowsperstrip == 0)
    rowsperstrip = 1;

  /* Set TIFF parameters. */
  TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, cols);
  TIFFSetField (tif, TIFFTAG_IMAGELENGTH, rows);
  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, bitspersample);
  TIFFSetField (tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField (tif, TIFFTAG_COMPRESSION, compression);
  if ((compression == COMPRESSION_LZW) && (predictor != 0))
    TIFFSetField (tif, TIFFTAG_PREDICTOR, predictor);
  if (alpha)
    {
      extra_samples [0] = EXTRASAMPLE_ASSOCALPHA;
      TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, 1, extra_samples);
    }
  TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, photometric);
  TIFFSetField (tif, TIFFTAG_FILLORDER, fillorder);
  TIFFSetField (tif, TIFFTAG_DOCUMENTNAME, filename);
  TIFFSetField (tif, TIFFTAG_IMAGEDESCRIPTION, "Created with The GIMP");
  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
  /* TIFFSetField( tif, TIFFTAG_STRIPBYTECOUNTS, rows / rowsperstrip ); */
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  if (drawable_type == INDEXED_IMAGE)
    TIFFSetField (tif, TIFFTAG_COLORMAP, red, grn, blu);

  /* array to rearrange data */
  tile_height = gimp_tile_height ();
  src = g_new (guchar, bytesperrow * tile_height);
  data = g_new (guchar, bytesperrow);

  /* Now write the TIFF data. */
  for (y = 0; y < rows; y = yend)
    {
      yend = y + tile_height;
      yend = MIN (yend, rows);

      gimp_pixel_rgn_get_rect (&pixel_rgn, src, 0, y, cols, yend - y);

      for (row = y; row < yend; row++)
	{
	  t = src + bytesperrow * (row - y);

	  switch (drawable_type)
	    {
	    case INDEXED_IMAGE:
	      success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	      break;
	    case GRAY_IMAGE:
	      success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	      break;
	    case GRAYA_IMAGE:
	      for (col = 0; col < cols*samplesperpixel; col+=samplesperpixel)
		{
		  /* pre-multiply gray by alpha */
		  data[col + 0] = (t[col + 0] * t[col + 1]) / 255;
		  data[col + 1] = t[col + 1];  /* alpha channel */
		}
	      success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
	      break;
	    case RGB_IMAGE:
	      success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	      break;
	    case RGBA_IMAGE:
	      for (col = 0; col < cols*samplesperpixel; col+=samplesperpixel)
		{
		  /* pre-multiply rgb by alpha */
		  data[col+0] = t[col + 0] * t[col + 3] / 255;
		  data[col+1] = t[col + 1] * t[col + 3] / 255;
		  data[col+2] = t[col + 2] * t[col + 3] / 255;
		  data[col+3] = t[col + 3];  /* alpha channel */
		}
	      success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
	      break;
	    default:
	      break;
	    }

	  if (!success) {
	      g_message("TIFF Failed a scanline write on row %d", row);
	      return 0;
	  }
	}

      gimp_progress_update ((double) row / (double) rows);
    }

  TIFFFlushData (tif);
  TIFFClose (tif);

  gimp_drawable_detach (drawable);
  g_free (data);

  return 1;
}

static gint
save_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *toggle_vbox;
  GSList *group;
  gchar **argv;
  gint argc;
  gint use_none = (tsvals.compression == COMPRESSION_NONE);
  gint use_lzw = (tsvals.compression == COMPRESSION_LZW);
  gint use_packbits = (tsvals.compression == COMPRESSION_PACKBITS);
  gint use_lsb2msb = (tsvals.fillorder == FILLORDER_LSB2MSB);
  gint use_msb2lsb = (tsvals.fillorder == FILLORDER_MSB2LSB);

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("save");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Save as Tiff");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) save_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) save_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  compression  */
  frame = gtk_frame_new ("Compression");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, FALSE, TRUE, 0);
  toggle_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

  group = NULL;
  toggle = gtk_radio_button_new_with_label (group, "None");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &use_none);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_none);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "LZW");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &use_lzw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_lzw);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "Pack Bits");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &use_packbits);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_packbits);
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);

  /*  fillorder  */
  frame = gtk_frame_new ("Fill Order");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, FALSE, TRUE, 0);
  toggle_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

  group = NULL;
  toggle = gtk_radio_button_new_with_label (group, "LSB to MSB");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &use_lsb2msb);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_lsb2msb);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "MSB to LSB");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &use_msb2lsb);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_msb2lsb);
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  if (use_none)
    tsvals.compression = COMPRESSION_NONE;
  else if (use_lzw)
    tsvals.compression = COMPRESSION_LZW;
  else if (use_packbits)
    tsvals.compression = COMPRESSION_PACKBITS;

  if (use_lsb2msb)
    tsvals.fillorder = FILLORDER_LSB2MSB;
  else if (use_msb2lsb)
    tsvals.fillorder = FILLORDER_MSB2LSB;

  return tsint.run;
}


/*  Save interface functions  */

static void
save_close_callback (GtkWidget *widget,
		     gpointer   data)
{
  gtk_main_quit ();
}

static void
save_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  tsint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
save_toggle_update (GtkWidget *widget,
		    gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}
