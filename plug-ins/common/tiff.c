/* tiff loading and saving for the GIMP
 *  -Peter Mattis
 * The TIFF loading code has been completely revamped by Nick Lamb
 * njl195@zepler.org.uk -- 18 May 1998
 * And it now gains support for tiles (and doubtless a zillion bugs)
 * njl195@zepler.org.uk -- 12 June 1999
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
#ifdef GIMP_HAVE_RESOLUTION_INFO
#include "libgimp/gimpunit.h"
#endif

typedef struct
{
  gint  compression;
  gint  fillorder;
} TiffSaveVals;

typedef struct
{
  gint  run;
} TiffSaveInterface;

typedef struct {
  gint32 ID;
  GDrawable *drawable;
  GPixelRgn pixel_rgn;
  guchar *pixels;
  guchar *pixel;
} channel_data;

/* Declare some local functions.
 */
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
static gint32 load_image (char   *filename);

static void   load_rgba  (TIFF *tif, channel_data *channel);
static void   load_lines (TIFF *tif, channel_data *channel,
                         unsigned short bps, unsigned short photomet,
                         int alpha, int extra);
static void   load_tiles (TIFF *tif, channel_data *channel,
                         unsigned short bps, unsigned short photomet,
                         int alpha, int extra);

static void   read_separate (guchar *source, channel_data *channel,
                             unsigned short bps, unsigned short photomet,
                             int startcol, int startrow, int rows, int cols,
                             int alpha, int extra, int sample);
static void   read_16bit (guchar *source, channel_data *channel,
                          unsigned short photomet,
                          int startcol, int startrow, int rows, int cols,
                          int alpha, int extra);
static void   read_8bit (guchar *source, channel_data *channel,
                         unsigned short photomet,
                         int startcol, int startrow, int rows, int cols,
                         int alpha, int extra);
static void   read_default (guchar *source, channel_data *channel,
                            unsigned short bps, unsigned short photomet,
                            int startcol, int startrow, int rows, int cols,
                            int alpha, int extra);

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
static void   comment_entry_callback  (GtkWidget *widget,
				       gpointer   data);

#define DEFAULT_COMMENT "Created with The GIMP"

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

static char *image_comment= NULL;

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
                          "1995-1996,1998-1999",
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
#ifdef GIMP_HAVE_PARASITES
  Parasite *parasite;
#endif /* GIMP_HAVE_PARASITES */
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

/* Do this right this time, if POSSIBLE query for parasites, otherwise
   or if there isn't one, choose the DEFAULT_COMMENT */

#ifdef GIMP_HAVE_PARASITES
      int image = param[1].data.d_int32;

      parasite = gimp_image_find_parasite(image, "gimp-comment");
      if (parasite)
        image_comment = g_strdup(parasite->data);
      parasite_free(parasite);
#endif /* GIMP_HAVE_PARASITES */

      if (!image_comment) image_comment = g_strdup(DEFAULT_COMMENT);	  

      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	{
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_tiff_save", &tsvals);
#ifdef GIMP_HAVE_PARASITES
	  parasite = gimp_image_find_parasite(image, "tiff-save-options");
	  if (parasite)
	  {
	    tsvals.compression = ((TiffSaveVals *)parasite->data)->compression;
	    tsvals.fillorder   = ((TiffSaveVals *)parasite->data)->fillorder;
	  }
	  parasite_free(parasite);
#endif /* GIMP_HAVE_PARASITES */

	  /*  First acquire information with a dialog  */
	  if (! save_dialog ())
	    return;
	} break;

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
	{
	  gimp_get_data ("file_tiff_save", &tsvals);
#ifdef GIMP_HAVE_PARASITES
	  parasite = gimp_image_find_parasite(image, "tiff-save-options");
	  if (parasite)
	  {
	    tsvals.compression = ((TiffSaveVals *)parasite->data)->compression;
	    tsvals.fillorder   = ((TiffSaveVals *)parasite->data)->fillorder;
	  }
	  parasite_free(parasite);
#endif /* GIMP_HAVE_PARASITES */
	}
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
  int cols, rows, alpha;
  int image, image_type = RGB;
  int layer, layer_type = RGB_IMAGE;
  unsigned short extra, *extra_types;
  channel_data *channel= NULL;

  unsigned short *redmap, *greenmap, *bluemap;
  guchar colors[3]= {0, 0, 0};
  guchar cmap[768];

  int i, j, worst_case = 0;
  char *name;

  TiffSaveVals save_vals;
#ifdef GIMP_HAVE_PARASITES
  Parasite *parasite;
#endif /* GIMP_HAVE_PARASITES */
  guint16 tmp;
  tif = TIFFOpen (filename, "r");
  if (!tif) {
    g_message("TIFF Can't open %s\n", filename);
    gimp_quit ();
  }

  name = g_malloc (strlen (filename) + 12);
  sprintf (name, "Loading %s:", filename);
  gimp_progress_init (name);
  g_free (name);

  TIFFGetFieldDefaulted (tif, TIFFTAG_BITSPERSAMPLE, &bps);

  if (bps > 8 && bps != 16) {
    worst_case = 1; /* Wrong sample width => RGBA */
  }

  TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);

  if (!TIFFGetField (tif, TIFFTAG_EXTRASAMPLES, &extra, &extra_types))
    extra = 0;

  if (!TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &cols)) {
    g_message("TIFF Can't get image width\n");
    gimp_quit ();
  }

  if (!TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &rows)) {
    g_message("TIFF Can't get image length\n");
    gimp_quit ();
  }

  if (!TIFFGetField (tif, TIFFTAG_PHOTOMETRIC, &photomet)) {
    g_message("TIFF Can't get photometric\nAssuming min-is-black\n");
    /* old AppleScan software misses out the photometric tag (and
     * incidentally assumes min-is-white, but xv assumes min-is-black,
     * so we follow xv's lead.  It's not much hardship to invert the
     * image later). */
    photomet = PHOTOMETRIC_MINISBLACK;
  }

  /* test if the extrasample represents an associated alpha channel... */
  if (extra > 0 && (extra_types[0] == EXTRASAMPLE_ASSOCALPHA)) {
    alpha = 1;
    --extra;
  } else {
    alpha = 0;
  }

  if (photomet == PHOTOMETRIC_RGB && spp > 3 + extra) {
    alpha= 1;
    extra= spp - 4; 
  } else if (photomet != PHOTOMETRIC_RGB && spp > 1 + extra) {
    alpha= 1;
    extra= spp - 2;
  }

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

    default:
      worst_case = 1;
  }

  if (worst_case) {
    image_type = RGB;
    layer_type = RGBA_IMAGE;
  }

  if ((image = gimp_image_new (cols, rows, image_type)) == -1) {
    g_message("TIFF Can't create a new image\n");
    gimp_quit ();
  }
  gimp_image_set_filename (image, filename);

  /* attach a parasite containing the compression/fillorder */
  if (!TIFFGetField (tif, TIFFTAG_COMPRESSION, &tmp))
    save_vals.compression = COMPRESSION_NONE;
  else
    save_vals.compression = tmp;
  if (!TIFFGetField (tif, TIFFTAG_FILLORDER, &tmp))
    save_vals.fillorder = FILLORDER_LSB2MSB;
  else
    save_vals.fillorder = tmp;
#ifdef GIMP_HAVE_PARASITES
  parasite = parasite_new("tiff-save-options", 0,
			  sizeof(save_vals), &save_vals);
  gimp_image_attach_parasite(image, parasite);
  parasite_free(parasite);
#endif /* GIMP_HAVE_PARASITES */


  /* Attach a parasite containing the image description.  Pretend to
   * be a gimp comment so other plugins will use this description as
   * an image comment where appropriate. */
#ifdef GIMP_HAVE_PARASITES
  {
    char *img_desc;

    if (TIFFGetField (tif, TIFFTAG_IMAGEDESCRIPTION, &img_desc))
    {
      int len;

      len = strlen(img_desc) + 1;
      len = MIN(len, 241);
      img_desc[len-1] = '\000';

      parasite = parasite_new("gimp-comment", PARASITE_PERSISTENT,
			      len, img_desc);
      gimp_image_attach_parasite(image, parasite);
      parasite_free(parasite);
    }
  }
#endif /* GIMP_HAVE_PARASITES */

  /* any resolution info in the file? */
#ifdef GIMP_HAVE_RESOLUTION_INFO
  {
    float xres = 72.0, yres = 72.0;
    unsigned short read_unit;
    GUnit unit = UNIT_PIXEL; /* invalid unit */

    if (TIFFGetField (tif, TIFFTAG_XRESOLUTION, &xres)) {
      if (TIFFGetField (tif, TIFFTAG_YRESOLUTION, &yres)) {

	if (TIFFGetFieldDefaulted (tif, TIFFTAG_RESOLUTIONUNIT, &read_unit)) {
	  switch(read_unit) {
	  case RESUNIT_NONE:
	    /* ImageMagick writes files with this silly resunit */
	    g_message("TIFF warning: resolution units meaningless, "
		      "forcing 72 dpi\n");
	    break;

	  case RESUNIT_INCH:
	    unit = UNIT_INCH;
	    break;

	  case RESUNIT_CENTIMETER:
	    xres *= 2.54;
	    yres *= 2.54;
	    unit = UNIT_MM; /* as this is our default metric unit */
	    break;

	  default:
	    g_message("TIFF file error: unknown resolution unit type %d, "
		      "assuming dpi\n", read_unit);
	  }
	} else { /* no res unit tag */
	  /* old AppleScan software produces these */
	  g_message("TIFF warning: resolution specified without\n"
		    "any units tag, assuming dpi\n");
	}
      } else { /* xres but no yres */
	g_message("TIFF warning: no y resolution info, assuming same as x\n");
	yres = xres;
      }

      /* sanity check, since division by zero later could be embarrassing */
      if (xres < 1e-5 || yres < 1e-5) {
	g_message("TIFF: image resolution is zero: forcing 72 dpi\n");
	xres = 72.0;
	yres = 72.0;
      }

      /* now set the new image's resolution info */
      gimp_image_set_resolution (image, xres, yres);
      if (unit != UNIT_PIXEL)
	gimp_image_set_unit (image, unit);
    }

    /* no x res tag => we assume we have no resolution info, so we
     * don't care.  Older versions of this plugin used to write files
     * with no resolution tags at all. */

    /* TODO: haven't caught the case where yres tag is present, but
       not xres.  This is left as an exercise for the reader - they
       should feel free to shoot the author of the broken program
       that produced the damaged TIFF file in the first place. */
  }
#endif /* GIMP_HAVE_RESOLUTION_INFO */


  /* Install colormap for INDEXED images only */
  if (image_type == INDEXED) {
    if (!TIFFGetField (tif, TIFFTAG_COLORMAP, &redmap, &greenmap, &bluemap)) {
      g_message("TIFF Can't get colormaps\n");
      gimp_quit ();
    }

    for (i = 0, j = 0; i < (1 << bps); i++) {
      cmap[j++] = redmap[i] >> 8;
      cmap[j++] = greenmap[i] >> 8;
      cmap[j++] = bluemap[i] >> 8;
    }
    gimp_image_set_cmap (image, cmap, (1 << bps));
  }

  /* Allocate channel_data for all channels, even the background layer */
  channel = g_new (channel_data, extra + 1);
  layer = gimp_layer_new (image, "Background", cols, rows, layer_type,
			     100, NORMAL_MODE);
  channel[0].ID= layer;
  gimp_image_add_layer (image, layer, 0);
  channel[0].drawable= gimp_drawable_get(layer);

  if (extra > 0 && !worst_case) {
    /* Add alpha channels as appropriate */
    for (i= 1; i <= extra; ++i) {
      channel[i].ID= gimp_channel_new(image, "TIFF Channel", cols, rows,
                                                            100.0, colors);
      gimp_image_add_channel(image, channel[i].ID, 0);
      channel[i].drawable= gimp_drawable_get (channel[i].ID);
    }
  }

  if (worst_case) {
    g_message("TIFF Fell back to RGBA, image may be inverted\n");
    load_rgba (tif, channel);
  } else if (TIFFIsTiled(tif)) {
    load_tiles (tif, channel, bps, photomet, alpha, extra);
  } else { /* Load scanlines in tile_height chunks */
    load_lines (tif, channel, bps, photomet, alpha, extra);
  }

  for (i= 0; i < extra; ++i) {
    gimp_drawable_flush (channel[i].drawable);
    gimp_drawable_detach (channel[i].drawable);
  }

  return image;
}

static void
load_rgba (TIFF *tif, channel_data *channel)
{
  uint32 imageWidth, imageLength;
  gulong *buffer;

  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageLength);
  gimp_pixel_rgn_init (&(channel[0].pixel_rgn), channel[0].drawable,
                          0, 0, imageWidth, imageLength, TRUE, FALSE);
  buffer = g_new(gulong, imageWidth * imageLength);
  channel[0].pixels = (guchar*) buffer;
  if (buffer == NULL) {
    g_message("TIFF Unable to allocate temporary buffer\n");
  }
  if (!TIFFReadRGBAImage(tif, imageWidth, imageLength, buffer, 0))
    g_message("TIFF Unsupported layout, no RGBA loader\n");

  gimp_pixel_rgn_set_rect(&(channel[0].pixel_rgn), channel[0].pixels,
                              0, 0, imageWidth, imageLength);
  g_message("TIFF Fell back to RGBA, image may be inverted\n");
}

static void
load_tiles (TIFF *tif, channel_data *channel,
	   unsigned short bps, unsigned short photomet,
	   int alpha, int extra)
{
  uint16 planar= PLANARCONFIG_CONTIG;
  uint32 imageWidth, imageLength;
  uint32 tileWidth, tileLength;
  uint32 x, y, rows, cols;
  guchar *buffer;
  double progress= 0.0, one_row;
  int i;

  TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planar);
  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageLength);
  TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth);
  TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileLength);
  one_row = (double) tileLength / (double) imageLength;
  buffer = g_malloc(TIFFTileSize(tif));

  for (i= 0; i <= extra; ++i) {
    channel[i].pixels= g_new(guchar, tileWidth * tileLength *
                                      channel[i].drawable->bpp);
  }

  for (y = 0; y < imageLength; y += tileLength) {
    for (x = 0; x < imageWidth; x += tileWidth) {
      gimp_progress_update (progress + one_row *
                            ( (double) x / (double) imageWidth));
      TIFFReadTile(tif, buffer, x, y, 0, 0);
      cols= MIN(imageWidth - x, tileWidth);
      rows= MIN(imageLength - y, tileLength);
      if (bps == 16) {
        read_16bit(buffer, channel, photomet, y, x, rows, cols, alpha, extra);
      } else if (bps == 8) {
        read_8bit(buffer, channel, photomet, y, x, rows, cols, alpha, extra);
      } else {
        read_default(buffer, channel, bps, photomet,
                          y, x, rows, cols, alpha, extra);
      }
    }
    progress+= one_row;
  }
  for (i= 0; i <= extra; ++i) {
    g_free(channel[i].pixels);
  }
  g_free(buffer);
}

static void
load_lines (TIFF *tif, channel_data *channel,
	   unsigned short bps, unsigned short photomet,
	   int alpha, int extra)
{
  uint16 planar= PLANARCONFIG_CONTIG;
  uint32 imageLength, lineSize, cols, rows;
  guchar *buffer;
  int i, y, tile_height = gimp_tile_height ();

  TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planar);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageLength);
  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &cols);
  lineSize= TIFFScanlineSize(tif);

  for (i= 0; i <= extra; ++i) {
    channel[i].pixels= g_new(guchar, tile_height * cols
                                          * channel[i].drawable->bpp);
  }

  buffer = g_malloc(lineSize * tile_height);
  if (planar == PLANARCONFIG_CONTIG) {
    for (y = 0; y < imageLength; y+= tile_height ) {
      gimp_progress_update ( (double) y / (double) imageLength);
      rows = MIN(tile_height, imageLength - y);
      for (i = 0; i < rows; ++i)
	TIFFReadScanline(tif, buffer + i * lineSize, y + i, 0);
      if (bps == 16) {
	read_16bit(buffer, channel, photomet, y, 0, rows, cols, alpha, extra);
      } else if (bps == 8) {
	read_8bit(buffer, channel, photomet, y, 0, rows, cols, alpha, extra);
      } else {
	read_default(buffer, channel, bps, photomet,
                             y, 0, rows, cols, alpha, extra);
      }
    }
  } else { /* PLANARCONFIG_SEPARATE  -- Just say "No" */
    uint16 s, samples;
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samples);
    for (s = 0; s < samples; ++s) {
      for (y = 0; y < imageLength; y+= tile_height ) {
	gimp_progress_update ( (double) y / (double) imageLength);
	rows = MIN(tile_height, imageLength - y);
	for (i = 0; i < rows; ++i)
	  TIFFReadScanline(tif, buffer + i * lineSize, y + i, s);
	read_separate (buffer, channel, bps, photomet,
		         y, 0, rows, cols, alpha, extra, s);
      }
    }
  }
  for (i= 0; i <= extra; ++i) {
    g_free(channel[i].pixels);
  }
  g_free(buffer);
}

static void
read_16bit(guchar *source, channel_data *channel, unsigned short photomet,
             int startrow, int startcol, int rows, int cols,
             int alpha, int extra)
{
  guchar *dest;
  int gray_val, red_val, green_val, blue_val, alpha_val;
  int col, row, i;

  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                          startcol, startrow, cols, rows, TRUE, FALSE);
  }

  for (row = 0; row < rows; ++row) {
    dest= channel[0].pixels + row * cols * channel[0].drawable->bpp;

    for (i= 1; i <= extra; ++i) {
      channel[i].pixel= channel[i].pixels + row * cols;
    }

    for (col = 0; col < cols; col++) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
          if (alpha) {
            gray_val= *source; source+= 2;
            alpha_val= *source; source+= 2;
            if (alpha_val)
              *dest++ = gray_val * 255 / alpha_val;
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = *source; source+= 2;
          }
          break;

        case PHOTOMETRIC_MINISWHITE:
          if (alpha) {
            gray_val= *source; source+= 2;
            alpha_val= *source; source+= 2;
            if (alpha_val)
              *dest++ = ((255 - gray_val) * 255) / alpha_val;
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = ~(*source); source+= 2;
          }
          break;

        case PHOTOMETRIC_PALETTE:
          *dest++= *source; source+= 2;
          if (alpha) *dest++= *source; source+= 2;
          break;
  
        case PHOTOMETRIC_RGB:
          if (alpha) {
            red_val= *source; source+= 2;
            green_val= *source; source+= 2;
            blue_val= *source; source+= 2;
            alpha_val= *source; source+= 2;
            if (alpha_val) {
              *dest++ = (red_val * 255) / alpha_val;
              *dest++ = (green_val * 255) / alpha_val;
              *dest++ = (blue_val * 255) / alpha_val;
            } else {
              *dest++ = 0;
              *dest++ = 0;
              *dest++ = 0;
	    }
	    *dest++ = alpha_val;
	  } else {
	    *dest++ = *source; source+= 2;
	    *dest++ = *source; source+= 2;
	    *dest++ = *source; source+= 2;
	  }
          break;

        default:
          /* This case was handled earlier */
          g_assert_not_reached();
      }
      for (i= 1; i <= extra; ++i) {
        *channel[i].pixel++ = *source; source+= 2;
      }
    }
  }
  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                              startcol, startrow, cols, rows);
  }
}

static void
read_8bit(guchar *source, channel_data *channel, unsigned short photomet,
             int startrow, int startcol, int rows, int cols,
             int alpha, int extra)
{
  guchar *dest;
  int gray_val, red_val, green_val, blue_val, alpha_val;
  int col, row, i;

  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                          startcol, startrow, cols, rows, TRUE, FALSE);
  }

  for (row = 0; row < rows; ++row) {
    dest= channel[0].pixels + row * cols * channel[0].drawable->bpp;

    for (i= 1; i <= extra; ++i) {
      channel[i].pixel= channel[i].pixels + row * cols;
    }

    for (col = 0; col < cols; col++) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
          if (alpha) {
            gray_val= *source++;
            alpha_val= *source++;
            if (alpha_val)
              *dest++ = gray_val * 255 / alpha_val;
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = *source++;
          }
          break;

        case PHOTOMETRIC_MINISWHITE:
          if (alpha) {
            gray_val= *source++;
            alpha_val= *source++;
            if (alpha_val)
              *dest++ = ((255 - gray_val) * 255) / alpha_val;
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = ~(*source++);
          }
          break;

        case PHOTOMETRIC_PALETTE:
          *dest++= *source++;
          if (alpha) *dest++= *source++;
          break;
  
        case PHOTOMETRIC_RGB:
          if (alpha) {
            red_val= *source++;
            green_val= *source++;
            blue_val= *source++;
            alpha_val= *source++;
            if (alpha_val) {
              *dest++ = (red_val * 255) / alpha_val;
              *dest++ = (green_val * 255) / alpha_val;
              *dest++ = (blue_val * 255) / alpha_val;
            } else {
              *dest++ = 0;
              *dest++ = 0;
              *dest++ = 0;
	    }
	    *dest++ = alpha_val;
	  } else {
	    *dest++ = *source++;
	    *dest++ = *source++;
	    *dest++ = *source++;
	  }
          break;

        default:
          /* This case was handled earlier */
          g_assert_not_reached();
      }
      for (i= 1; i <= extra; ++i) {
        *channel[i].pixel++ = *source++;
      }
    }
  }
  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                              startcol, startrow, cols, rows);
  }
}

/* Step through all <= 8-bit samples in an image */

#define NEXTSAMPLE(var)                       \
  {                                           \
      if (bitsleft == 0)                      \
      {                                       \
	  source++;                           \
	  bitsleft = 8;                       \
      }                                       \
      bitsleft -= bps;                        \
      var = ( *source >> bitsleft ) & maxval; \
  }

static void
read_default(guchar *source, channel_data *channel,
             unsigned short bps, unsigned short photomet,
             int startrow, int startcol, int rows, int cols,
             int alpha, int extra)
{
  guchar *dest;
  int gray_val, red_val, green_val, blue_val, alpha_val;
  int col, row, i;
  int bitsleft = 8, maxval = (1 << bps) - 1;

  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
		startcol, startrow, cols, rows, TRUE, FALSE);
  }

  for (row = 0; row < rows; ++row) {
    dest= channel[0].pixels + row * cols * channel[0].drawable->bpp;

    for (i= 1; i <= extra; ++i) {
      channel[i].pixel= channel[i].pixels + row * cols;
    }

    for (col = 0; col < cols; col++) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
          NEXTSAMPLE(gray_val);
          if (alpha) {
            NEXTSAMPLE(alpha_val);
            if (alpha_val)
              *dest++ = (gray_val * 65025) / (alpha_val * maxval);
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = (gray_val * 255) / maxval;
          }
          break;

        case PHOTOMETRIC_MINISWHITE:
          NEXTSAMPLE(gray_val);
          if (alpha) {
            NEXTSAMPLE(alpha_val);
            if (alpha_val)
              *dest++ = ((maxval - gray_val) * 65025) / (alpha_val * maxval);
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = ((maxval - gray_val) * 255) / maxval;
          }
          break;

        case PHOTOMETRIC_PALETTE:
          NEXTSAMPLE(*dest++);
          if (alpha) {
            NEXTSAMPLE(*dest++);
          }
          break;
  
        case PHOTOMETRIC_RGB:
          NEXTSAMPLE(red_val)
          NEXTSAMPLE(green_val)
          NEXTSAMPLE(blue_val)
          if (alpha) {
            NEXTSAMPLE(alpha_val)
            if (alpha_val) {
              *dest++ = (red_val * 255) / alpha_val;
              *dest++ = (green_val * 255) / alpha_val;
              *dest++ = (blue_val * 255) / alpha_val;
            } else {
              *dest++ = 0;
              *dest++ = 0;
              *dest++ = 0;
	    }
	    *dest++ = alpha_val;
	  } else {
	    *dest++ = red_val;
	    *dest++ = green_val;
	    *dest++ = blue_val;
	  }
          break;

        default:
          /* This case was handled earlier */
          g_assert_not_reached();
      }
      for (i= 1; i <= extra; ++i) {
        NEXTSAMPLE(alpha_val);
        *channel[i].pixel++ = alpha_val;
      }
    }
    bitsleft= 0;
  }
  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                              startcol, startrow, cols, rows);
  }
}

static void
read_separate (guchar *source, channel_data *channel,
               unsigned short bps, unsigned short photomet,
               int startrow, int startcol, int rows, int cols,
               int alpha, int extra, int sample)
{
  guchar *dest;
  int col, row, c;
  int bitsleft = 8, maxval = (1 << bps) - 1;

  if (bps > 8) {
    g_message("TIFF Unsupported layout\n");
    gimp_quit();
  }

  if (sample < channel[0].drawable->bpp) {
    c = 0;
  } else {
    c = (sample - channel[0].drawable->bpp) + 4;
    photomet = PHOTOMETRIC_MINISBLACK;
  }

  gimp_pixel_rgn_init (&(channel[c].pixel_rgn), channel[c].drawable,
                         startcol, startrow, cols, rows, TRUE, FALSE);

  gimp_pixel_rgn_get_rect(&(channel[c].pixel_rgn), channel[c].pixels,
                            startcol, startrow, cols, rows);
  for (row = 0; row < rows; ++row) {
    dest = channel[c].pixels + row * cols * channel[c].drawable->bpp;
    if (c == 0) {
      for (col = 0; col < cols; ++col) {
        NEXTSAMPLE(dest[col * channel[0].drawable->bpp + sample]);
      }
    } else {
      for (col = 0; col < cols; ++col)
        NEXTSAMPLE(dest[col]);
    }
  }
  gimp_pixel_rgn_set_rect(&(channel[c].pixel_rgn), channel[c].pixels,
                            startcol, startrow, cols, rows);
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
      predictor = 2;
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
      predictor = 2;
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
     default:
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
  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
  /* TIFFSetField( tif, TIFFTAG_STRIPBYTECOUNTS, rows / rowsperstrip ); */
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

#ifdef GIMP_HAVE_RESOLUTION_INFO
  /* resolution fields */
  {
    double xresolution;
    double yresolution;
    unsigned short save_unit = RESUNIT_INCH;
    GUnit unit;
    float factor;

    gimp_image_get_resolution (image, &xresolution, &yresolution);
    unit = gimp_image_get_unit (image);
    factor = gimp_unit_get_factor (unit);

    /*  if we have a metric unit, save the resolution as centimeters
     */
    if ((ABS(factor - 0.0254) < 1e-5) ||  /* m */
	(ABS(factor - 0.254) < 1e-5) ||   /* why not ;) */
	(ABS(factor - 2.54) < 1e-5) ||    /* cm */
	(ABS(factor - 25.4) < 1e-5))      /* mm */
      {
	save_unit = RESUNIT_CENTIMETER;
	xresolution /= 2.54;
	yresolution /= 2.54;
      }

    if (xresolution > 1e-5 && yresolution > 1e-5)
      {
	TIFFSetField (tif, TIFFTAG_XRESOLUTION, xresolution);
	TIFFSetField (tif, TIFFTAG_YRESOLUTION, yresolution);
	TIFFSetField (tif, TIFFTAG_RESOLUTIONUNIT, save_unit);
      }
  }
#endif /* GIMP_HAVE_RESOLUTION_INFO */

  /* do we have a comment?  If so, create a new parasite to hold it,
   * and attach it to the image. The attach function automatically
   * detaches a previous incarnation of the parasite. */
#ifdef GIMP_HAVE_PARASITES
  if (image_comment && *image_comment != '\000')
  {
    Parasite *parasite;

    TIFFSetField (tif, TIFFTAG_IMAGEDESCRIPTION, image_comment);
    parasite = parasite_new ("gimp-comment", 1,
			      strlen(image_comment)+1, image_comment);
    gimp_image_attach_parasite (image, parasite);
    parasite_free (parasite);
  }
#endif /* GIMP_HAVE_PARASITES */

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
	      success = FALSE;
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
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
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

  /* hbox for compression and fillorder settings */
  hbox = gtk_hbox_new (FALSE, 5);

  /*  compression  */
  frame = gtk_frame_new ("Compression");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);
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
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), use_none);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "LZW");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &use_lzw);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), use_lzw);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "Pack Bits");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &use_packbits);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), use_packbits);
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);

  /*  fillorder  */
  frame = gtk_frame_new ("Fill Order");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);
  toggle_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

  group = NULL;
  toggle = gtk_radio_button_new_with_label (group, "LSB to MSB (PC)");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &use_lsb2msb);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), use_lsb2msb);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "MSB to LSB (Mac)");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &use_msb2lsb);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), use_msb2lsb);
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);


  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);


  /* comment entry */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, FALSE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  label = gtk_label_new ("Comment: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), image_comment);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) comment_entry_callback,
                      NULL);

  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);
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

static void
comment_entry_callback (GtkWidget *widget,
			gpointer   data)
{
  int len;
  char *text;

  text = gtk_entry_get_text (GTK_ENTRY (widget));
  len = strlen(text);

  /* Temporary kludge for overlength strings - just return */
  if (len > 240)
    {
      g_message ("TIFF save: Your comment string is too long.\n");
      return;
    }

  g_free(image_comment);
  image_comment = g_strdup(text);

  /* g_print ("COMMENT: %s\n", image_comment); */
}
