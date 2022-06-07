/* tiff loading for GIMP
 *  -Peter Mattis
 *
 * The TIFF loading code has been completely revamped by Nick Lamb
 * njl195@zepler.org.uk -- 18 May 1998
 * And it now gains support for tiles (and doubtless a zillion bugs)
 * njl195@zepler.org.uk -- 12 June 1999
 * LZW patent fuss continues :(
 * njl195@zepler.org.uk -- 20 April 2000
 * The code for this filter is based on "tifftopnm" and "pnmtotiff",
 *  2 programs that are a part of the netpbm package.
 * khk@khk.net -- 13 May 2000
 * Added support for ICCPROFILE tiff tag. If this tag is present in a
 * TIFF file, then a parasite is created and vice versa.
 * peter@kirchgessner.net -- 29 Oct 2002
 * Progress bar only when run interactive
 * Added support for layer offsets - pablo.dangelo@web.de -- 7 Jan 2004
 * Honor EXTRASAMPLES tag while loading images with alphachannel
 * pablo.dangelo@web.de -- 16 Jan 2004
 */

/*
 * tifftopnm.c - converts a Tagged Image File to a portable anymap
 *
 * Derived by Jef Poskanzer from tif2ras.c, which is:
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <tiffio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "file-tiff-io.h"
#include "file-tiff-load.h"

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_ROLE "gimp-file-tiff-load"


typedef struct
{
  gint      compression;
  gint      fillorder;
  gboolean  save_transp_pixels;
} TiffSaveVals;

typedef struct
{
  gint32      ID;
  GeglBuffer *buffer;
  const Babl *format;
  guchar     *pixels;
  guchar     *pixel;
} ChannelData;

typedef enum
{
  GIMP_TIFF_LOAD_ASSOCALPHA,
  GIMP_TIFF_LOAD_UNASSALPHA,
  GIMP_TIFF_LOAD_CHANNEL
} DefaultExtra;

typedef enum
{
  GIMP_TIFF_DEFAULT,
  GIMP_TIFF_INDEXED,
  GIMP_TIFF_GRAY,
  GIMP_TIFF_GRAY_MINISWHITE,
} TiffColorMode;

/* Declare some local functions */

static GimpColorProfile * load_profile     (TIFF              *tif);

static void               load_rgba        (TIFF              *tif,
                                            ChannelData       *channel);
static void               load_contiguous  (TIFF              *tif,
                                            ChannelData       *channel,
                                            const Babl        *type,
                                            gushort            bps,
                                            gushort            spp,
                                            TiffColorMode      tiff_mode,
                                            gboolean           is_signed,
                                            gint               extra);
static void               load_separate    (TIFF              *tif,
                                            ChannelData       *channel,
                                            const Babl        *type,
                                            gushort            bps,
                                            gushort            spp,
                                            TiffColorMode      tiff_mode,
                                            gboolean           is_signed,
                                            gint               extra);
static void               load_paths       (TIFF              *tif,
                                            gint               image,
                                            gint               width,
                                            gint               height,
                                            gint               offset_x,
                                            gint               offset_y);

static gboolean   is_non_conformant_tiff   (gushort            photomet,
                                            gushort            spp);
static gushort    get_extra_channels_count (gushort            photomet,
                                            gushort            spp,
                                            gboolean           alpha);

static void               fill_bit2byte    (TiffColorMode      tiff_mode);
static void               fill_2bit2byte   (TiffColorMode      tiff_mode);
static void               fill_4bit2byte   (TiffColorMode      tiff_mode);
static void               convert_bit2byte (const guchar      *src,
                                            guchar            *dest,
                                            gint               width,
                                            gint               height);
static void              convert_2bit2byte (const guchar      *src,
                                            guchar            *dest,
                                            gint               width,
                                            gint               height);
static void              convert_4bit2byte (const guchar      *src,
                                            guchar            *dest,
                                            gint               width,
                                            gint               height);

static void             convert_miniswhite (guchar            *buffer,
                                            gint               width,
                                            gint               height);
static void               convert_int2uint (guchar            *buffer,
                                            gint               bps,
                                            gint               spp,
                                            gint               width,
                                            gint               height,
                                            gint               stride);

static gboolean           load_dialog      (TIFF              *tif,
                                            const gchar       *help_id,
                                            TiffSelectedPages *pages,
                                            const gchar       *extra_message,
                                            DefaultExtra      *default_extra);


static TiffSaveVals tsvals =
{
  COMPRESSION_NONE,    /*  compression    */
  TRUE,                /*  alpha handling */
};

/* Grayscale conversion mappings */
static const guchar _1_to_8_bitmap [2] =
{
  0, 255
};

static const guchar _1_to_8_bitmap_rev [2] =
{
  255, 0
};

static const guchar _2_to_8_bitmap [4] =
{
  0, 85, 170, 255
};

static const guchar _2_to_8_bitmap_rev [4] =
{
  255, 170, 85, 0
};

static const guchar _4_to_8_bitmap [16] =
{
  0,    17,  34,  51,  68,  85, 102, 119,
  136, 153, 170, 187, 204, 221, 238, 255
};

static const guchar _4_to_8_bitmap_rev [16] =
{
  255, 238, 221, 204, 187, 170, 153, 136,
  119, 102,  85,  68,  51,  34,  17,   0
};

static guchar   bit2byte[256 * 8];
static guchar _2bit2byte[256 * 4];
static guchar _4bit2byte[256 * 2];


/* returns a pointer into the TIFF */
static const gchar *
tiff_get_page_name (TIFF *tif)
{
  static gchar *name;

  if (TIFFGetField (tif, TIFFTAG_PAGENAME, &name) &&
      g_utf8_validate (name, -1, NULL))
    {
      return name;
    }

  return NULL;
}

/* is_non_conformant_tiff assumes TIFFTAG_EXTRASAMPLES was not set */
static gboolean
is_non_conformant_tiff (gushort photomet, gushort spp)
{
  switch (photomet)
    {
    case PHOTOMETRIC_RGB:
    case PHOTOMETRIC_YCBCR:
    case PHOTOMETRIC_CIELAB:
    case PHOTOMETRIC_ICCLAB:
    case PHOTOMETRIC_ITULAB:
    case PHOTOMETRIC_LOGLUV:
      return (spp > 3 || (spp == 2 && photomet != PHOTOMETRIC_RGB));
      break;
    case PHOTOMETRIC_SEPARATED:
      return (spp > 4);
      break;
    default:
      return (spp > 1);
      break;
    }
}

/* get_extra_channels_count returns number of channels excluding
 * alpha and color channels
 */
static gushort
get_extra_channels_count (gushort photomet, gushort spp, gboolean alpha)
{
  switch (photomet)
    {
    case PHOTOMETRIC_RGB:
    case PHOTOMETRIC_YCBCR:
    case PHOTOMETRIC_CIELAB:
    case PHOTOMETRIC_ICCLAB:
    case PHOTOMETRIC_ITULAB:
    case PHOTOMETRIC_LOGLUV:
      if (spp >= 3)
        return spp - 3 - (alpha? 1 : 0);
      else
        return spp - 1 - (alpha? 1 : 0);
      break;
    case PHOTOMETRIC_SEPARATED:
      return spp - 4 - (alpha? 1 : 0);
      break;
    default:
      return spp - 1 - (alpha? 1 : 0);
      break;
    }
}

GimpPDBStatusType
load_image (GFile        *file,
            GimpRunMode   run_mode,
            gint32       *image,
            gboolean     *resolution_loaded,
            gboolean     *profile_loaded,
            GError      **error)
{
  TIFF              *tif;
  TiffSelectedPages  pages;

  GList             *images_list      = NULL;
  DefaultExtra       default_extra    = GIMP_TIFF_LOAD_UNASSALPHA;
  gint               first_image_type = GIMP_RGB;
  gint               min_row          = G_MAXINT;
  gint               min_col          = G_MAXINT;
  gint               max_row          = 0;
  gint               max_col          = 0;
  GimpColorProfile  *first_profile      = NULL;
  const gchar       *extra_message      = NULL;
  gint               li;

  *image = 0;
  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  tif = tiff_open (file, "r", error);
  if (! tif)
    {
      if (! (error && *error))
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Not a TIFF image or image is corrupt."));

      return  GIMP_PDB_EXECUTION_ERROR;
    }

  pages.target = GIMP_PAGE_SELECTOR_TARGET_LAYERS;
  gimp_get_data (LOAD_PROC "-target", &pages.target);

  pages.keep_empty_space = TRUE;
  gimp_get_data (LOAD_PROC "-keep-empty-space",
                 &pages.keep_empty_space);

  pages.n_pages = pages.o_pages = TIFFNumberOfDirectories (tif);
  if (pages.n_pages == 0)
    {
      /* See #5837.
       * It seems we might be able to rescue some data even though the
       * TIFF is possibly syntactically wrong.
       */

      /* libtiff says max number of directory is 65535. */
      for (li = 0; li < 65536; li++)
        {
          if (TIFFSetDirectory (tif, li) == 0)
            break;
        }
      pages.n_pages = li;
      if (pages.n_pages == 0)
        {
          TIFFClose (tif);
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("TIFF '%s' does not contain any directories"),
                       gimp_file_get_utf8_name (file));

          return GIMP_PDB_EXECUTION_ERROR;
        }

      TIFFSetDirectory (tif, 0);
      g_message (ngettext ("TIFF '%s' directory count by header failed "
                           "though there seems to be %d page."
                           " Attempting to load the file with this assumption.",
                           "TIFF '%s' directory count by header failed "
                           "though there seem to be %d pages."
                           " Attempting to load the file with this assumption.",
                           pages.n_pages),
                 gimp_file_get_utf8_name (file), pages.n_pages);
    }

  pages.pages = NULL;
  pages.n_filtered_pages = pages.n_pages;

  pages.filtered_pages  = g_new0 (gint, pages.n_pages);
  for (li = 0; li < pages.n_pages; li++)
    pages.filtered_pages[li] = li;

  if (pages.n_pages == 1)
    {
      pages.pages  = g_new0 (gint, pages.n_pages);
      pages.target = GIMP_PAGE_SELECTOR_TARGET_LAYERS;
    }

  /* Check all pages if any has an unspecified or unset channel. */
  for (li = 0; li < pages.n_pages; li++)
    {
      gushort  spp;
      gushort  photomet;
      gushort  extra;
      gushort *extra_types;
      gushort  file_type = 0;
      gboolean first_page_old_jpeg = FALSE;

      if (TIFFSetDirectory (tif, li) == 0)
        continue;

      TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
      if (! TIFFGetField (tif, TIFFTAG_PHOTOMETRIC, &photomet))
        {
          guint16 compression;

          if (TIFFGetField (tif, TIFFTAG_COMPRESSION, &compression) &&
              (compression == COMPRESSION_CCITTFAX3 ||
               compression == COMPRESSION_CCITTFAX4 ||
               compression == COMPRESSION_CCITTRLE  ||
               compression == COMPRESSION_CCITTRLEW))
            {
              photomet = PHOTOMETRIC_MINISWHITE;
            }
          else
            {
              /* old AppleScan software misses out the photometric tag
               * (and incidentally assumes min-is-white, but xv
               * assumes min-is-black, so we follow xv's lead.  It's
               * not much hardship to invert the image later).
               */
              photomet = PHOTOMETRIC_MINISBLACK;
            }
        }
      if (! TIFFGetField (tif, TIFFTAG_EXTRASAMPLES, &extra, &extra_types))
        extra = 0;

      /* Try to detect if a TIFF page is a thumbnail.
       * Easy case: if subfiletype is set to FILETYPE_REDUCEDIMAGE.
       * If no subfiletype is defined we try to detect it ourselves.
       * We will consider it a thumbnail if:
       * - It's the second page
       * - PhotometricInterpretation is YCbCr
       * - Compression is old style jpeg
       * - First page uses a different compression or PhotometricInterpretation
       *
       * We could also add a check for the presence of TIFFTAG_EXIFIFD since
       * this should usually be a thumbnail part of EXIF metadata. Since that
       * probably won't make a difference, I will leave that out for now.
       */
      if (li == 0)
        {
          guint16 compression;

          if (TIFFGetField (tif, TIFFTAG_COMPRESSION, &compression) &&
              compression == COMPRESSION_OJPEG &&
              photomet    == PHOTOMETRIC_YCBCR)
            first_page_old_jpeg = TRUE;
        }

      if (TIFFGetField (tif, TIFFTAG_SUBFILETYPE, &file_type))
        {
          if (file_type == FILETYPE_REDUCEDIMAGE)
            {
              /* file_type is a mask but we will only filter out pages
               * that only have FILETYPE_REDUCEDIMAGE set */
              pages.filtered_pages[li] = -1;
              pages.n_filtered_pages--;
              g_debug ("Page %d is a FILETYPE_REDUCEDIMAGE thumbnail.\n", li);
            }
        }
      else
        {
          if (li == 1 && photomet == PHOTOMETRIC_YCBCR &&
              ! first_page_old_jpeg)
            {
              guint16 compression;

              if (TIFFGetField (tif, TIFFTAG_COMPRESSION, &compression) &&
                  compression == COMPRESSION_OJPEG)
                {
                  pages.filtered_pages[li] = -1;
                  pages.n_filtered_pages--;
                  g_debug ("Page %d is most likely a thumbnail.\n", li);
                }
            }
        }

      /* TODO: current code always assumes that the alpha channel
       * will be the first extra channel, though the TIFF spec does
       * not mandate such assumption. A future improvement should be
       * to actually loop through the extra channels and save the
       * alpha channel index.
       * Of course, this is an edge case, as most image would likely
       * have only a single extra channel anyway. But still we could
       * be more accurate.
       */
      if (extra > 0 && (extra_types[0] == EXTRASAMPLE_UNSPECIFIED))
        {
          extra_message = _("Extra channels with unspecified data.");
          break;
        }
      else if (extra == 0 && is_non_conformant_tiff (photomet, spp))
        {
          /* ExtraSamples field not set, yet we have more channels than
           * the PhotometricInterpretation field suggests.
           * This should not happen as the spec clearly says "This field
           * must be present if there are extra samples". So the files
           * can be considered non-conformant.
           * Let's ask what to do with the channel.
           */
          extra_message = _("Non-conformant TIFF: extra channels without 'ExtraSamples' field.");
        }
    }
  TIFFSetDirectory (tif, 0);

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      (pages.n_pages > 1 || extra_message) &&
      ! load_dialog (tif, LOAD_PROC, &pages,
                     extra_message, &default_extra))
    {
      TIFFClose (tif);
      g_clear_pointer (&pages.pages, g_free);

      return GIMP_PDB_CANCEL;
    }
  /* Adjust pages to take filtered out pages into account. */
  if (pages.o_pages > pages.n_filtered_pages)
    {
      gint fi;
      gint sel_index = 0;
      gint sel_add   = 0;

      for (fi = 0; fi < pages.o_pages && sel_index < pages.n_pages; fi++)
        {
          if (pages.filtered_pages[fi] == -1)
            {
              sel_add++;
            }
          if (pages.pages[sel_index] + sel_add == fi)
            {
              pages.pages[sel_index] = fi;
              sel_index++;
            }
        }
    }

  gimp_set_data (LOAD_PROC "-target",
                 &pages.target, sizeof (pages.target));
  gimp_set_data (LOAD_PROC "-keep-empty-space",
                 &pages.keep_empty_space,
                 sizeof (pages.keep_empty_space));

  /* We will loop through the all pages in case of multipage TIFF
   * and load every page as a separate layer.
   */
  for (li = 0; li < pages.n_pages; li++)
    {
      gint              ilayer;
      gushort           bps;
      gushort           spp;
      gushort           photomet;
      gshort            sampleformat;
      GimpColorProfile *profile;
      gboolean          profile_linear = FALSE;
      GimpPrecision     image_precision;
      const Babl       *type;
      const Babl       *base_format = NULL;
      const Babl       *space       = NULL;
      guint16           orientation;
      gint              cols;
      gint              rows;
      gboolean          alpha;
      gint              image_type           = GIMP_RGB;
      gint              layer;
      gint              layer_type           = GIMP_RGB_IMAGE;
      float             layer_offset_x       = 0.0;
      float             layer_offset_y       = 0.0;
      gint              layer_offset_x_pixel = 0;
      gint              layer_offset_y_pixel = 0;
      gushort           extra;
      gushort          *extra_types;
      ChannelData      *channel = NULL;
      uint16            planar  = PLANARCONFIG_CONTIG;
      TiffColorMode     tiff_mode;
      gboolean          is_signed;
      gint              i;
      gboolean          worst_case = FALSE;
      TiffSaveVals      save_vals;
      const gchar      *name;

      if (TIFFSetDirectory (tif, pages.pages[li]) == 0)
        {
          g_message (_("Couldn't read page %d of %d. Image might be corrupt.\n"),
                     li+1, pages.n_pages);
          continue;
        }
      ilayer = pages.pages[li];

      gimp_progress_update (0.0);

      TIFFGetFieldDefaulted (tif, TIFFTAG_BITSPERSAMPLE, &bps);

      TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLEFORMAT, &sampleformat);

      profile = load_profile (tif);
      if (! profile && first_profile)
        {
          profile = first_profile;
          g_object_ref (profile);
        }

      if (profile)
        {
          profile_linear = gimp_color_profile_is_linear (profile);

          if (! first_profile)
            {
              first_profile = profile;
              g_object_ref (first_profile);

              if (profile_linear && li > 0 && pages.target != GIMP_PAGE_SELECTOR_TARGET_IMAGES)
                g_message (_("This image has a linear color profile but "
                             "it was not set on the first layer. "
                             "The layers below layer # %d will be "
                             "interpreted as non linear."), li+1);
            }
          else if (pages.target != GIMP_PAGE_SELECTOR_TARGET_IMAGES &&
                   ! gimp_color_profile_is_equal (first_profile, profile))
            {
              g_message (_("This image has multiple color profiles. "
                           "We will use the first one. If this leads "
                           "to incorrect results you should consider "
                           "loading each layer as a separate image."));
            }

          if (! *image)
            *profile_loaded = TRUE;
        }

      if (bps > 64)
        {
          g_message (_("Suspicious bit depth: %d for page %d. Image may be corrupt."),
                     bps, li+1);
          continue;
        }

      if (bps > 8 && bps != 8 && bps != 16 && bps != 32 && bps != 64)
        worst_case = TRUE; /* Wrong sample width => RGBA */

      switch (bps)
        {
        case 1:
        case 2:
        case 4:
        case 8:
          if (profile_linear)
            image_precision = GIMP_PRECISION_U8_LINEAR;
          else
            image_precision = GIMP_PRECISION_U8_GAMMA;

          type = babl_type ("u8");
          break;

        case 16:
          if (sampleformat == SAMPLEFORMAT_IEEEFP)
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_HALF_LINEAR;
              else
                image_precision = GIMP_PRECISION_HALF_GAMMA;

              type = babl_type ("half");
            }
          else
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_U16_LINEAR;
              else
                image_precision = GIMP_PRECISION_U16_GAMMA;

              type = babl_type ("u16");
            }
          break;

        case 32:
          if (sampleformat == SAMPLEFORMAT_IEEEFP)
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_FLOAT_LINEAR;
              else
                image_precision = GIMP_PRECISION_FLOAT_GAMMA;

              type = babl_type ("float");
            }
          else
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_U32_LINEAR;
              else
                image_precision = GIMP_PRECISION_U32_GAMMA;

              type = babl_type ("u32");
            }
          break;

        case 64:
          if (profile_linear)
            image_precision = GIMP_PRECISION_DOUBLE_LINEAR;
          else
            image_precision = GIMP_PRECISION_DOUBLE_GAMMA;

          type = babl_type ("double");
          break;

        default:
          g_message (_("Unsupported bit depth: %d for page %d."),
                     bps, li+1);
          continue;
        }

      g_printerr ("bps: %d\n", bps);

      TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);

      if (! TIFFGetField (tif, TIFFTAG_EXTRASAMPLES, &extra, &extra_types))
        extra = 0;

      if (! TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &cols))
        {
          TIFFClose (tif);
          g_message (_("Could not get image width from '%s'"),
                     gimp_file_get_utf8_name (file));
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if (! TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &rows))
        {
          TIFFClose (tif);
          g_message (_("Could not get image length from '%s'"),
                     gimp_file_get_utf8_name (file));
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if (cols > GIMP_MAX_IMAGE_SIZE || cols <= 0 ||
          rows > GIMP_MAX_IMAGE_SIZE || rows <= 0)
        {
          g_message (_("Invalid image dimensions (%u x %u) for page %d. "
                     "Image may be corrupt."),
                     (guint32) cols, (guint32) rows, li+1);
          continue;
        }
      else
        {
          g_printerr ("Image dimensions: %u x %u.\n",
                      (guint32) cols, (guint32) rows);
        }

      if (! TIFFGetField (tif, TIFFTAG_PHOTOMETRIC, &photomet))
        {
          guint16 compression;

          if (TIFFGetField (tif, TIFFTAG_COMPRESSION, &compression) &&
              (compression == COMPRESSION_CCITTFAX3 ||
               compression == COMPRESSION_CCITTFAX4 ||
               compression == COMPRESSION_CCITTRLE  ||
               compression == COMPRESSION_CCITTRLEW))
            {
              g_message (_("Could not get photometric from '%s'. "
                         "Image is CCITT compressed, assuming min-is-white"),
                         gimp_file_get_utf8_name (file));
              photomet = PHOTOMETRIC_MINISWHITE;
            }
          else
            {
              g_message (_("Could not get photometric from '%s'. "
                         "Assuming min-is-black"),
                         gimp_file_get_utf8_name (file));

              /* old AppleScan software misses out the photometric tag
               * (and incidentally assumes min-is-white, but xv
               * assumes min-is-black, so we follow xv's lead.  It's
               * not much hardship to invert the image later).
               */
              photomet = PHOTOMETRIC_MINISBLACK;
            }
        }

      /* test if the extrasample represents an associated alpha channel... */
      if (extra > 0 && (extra_types[0] == EXTRASAMPLE_ASSOCALPHA))
        {
          alpha = TRUE;
          tsvals.save_transp_pixels = FALSE;
          extra--;
        }
      else if (extra > 0 && (extra_types[0] == EXTRASAMPLE_UNASSALPHA))
        {
          alpha = TRUE;
          tsvals.save_transp_pixels = TRUE;
          extra--;
        }
      else if (extra > 0 && (extra_types[0] == EXTRASAMPLE_UNSPECIFIED))
        {
          if (run_mode != GIMP_RUN_INTERACTIVE)
            /* In non-interactive mode, we assume unassociated alpha if unspecified.
             * We don't output messages in interactive mode as the user
             * has already the ability to choose through a dialog. */
            g_message (_("Alpha channel type not defined for %s. "
                       "Assuming alpha is not premultiplied"),
                       gimp_file_get_utf8_name (file));

          switch (default_extra)
            {
            case GIMP_TIFF_LOAD_ASSOCALPHA:
              alpha = TRUE;
              tsvals.save_transp_pixels = FALSE;
              break;
            case GIMP_TIFF_LOAD_UNASSALPHA:
              alpha = TRUE;
              tsvals.save_transp_pixels = TRUE;
              break;
            default: /* GIMP_TIFF_LOAD_CHANNEL */
              alpha = FALSE;
              break;
            }
          extra--;
        }
      else /* extra == 0 */
        {
          if (is_non_conformant_tiff (photomet, spp))
            {
              if (run_mode != GIMP_RUN_INTERACTIVE)
                g_message (_("Image '%s' does not conform to the TIFF specification: "
                           "ExtraSamples field is not set while extra channels are present. "
                           "Assuming the first extra channel is non-premultiplied alpha."),
                           gimp_file_get_utf8_name (file));

              switch (default_extra)
                {
                case GIMP_TIFF_LOAD_ASSOCALPHA:
                  alpha = TRUE;
                  tsvals.save_transp_pixels = FALSE;
                  break;
                case GIMP_TIFF_LOAD_UNASSALPHA:
                  alpha = TRUE;
                  tsvals.save_transp_pixels = TRUE;
                  break;
                default: /* GIMP_TIFF_LOAD_CHANNEL */
                  alpha = FALSE;
                  break;
                }
            }
          else
            {
              alpha = FALSE;
            }
        }

      extra = get_extra_channels_count (photomet, spp, alpha);

      tiff_mode = GIMP_TIFF_DEFAULT;
      is_signed = sampleformat == SAMPLEFORMAT_INT;

      switch (photomet)
        {
        case PHOTOMETRIC_PALETTE:
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_MINISWHITE:
          if (bps < 8)
            {
              if (photomet == PHOTOMETRIC_PALETTE)
                tiff_mode = GIMP_TIFF_INDEXED;
              else if (photomet == PHOTOMETRIC_MINISBLACK)
                tiff_mode = GIMP_TIFF_GRAY;
              else if (photomet == PHOTOMETRIC_MINISWHITE)
                tiff_mode = GIMP_TIFF_GRAY_MINISWHITE;

              /* FIXME: It should be a user choice whether this should be
               * interpreted as indexed or grayscale. For now we will
               * use indexed (see issue #6766). */
              image_type = GIMP_INDEXED;
              layer_type = alpha ? GIMP_INDEXEDA_IMAGE : GIMP_INDEXED_IMAGE;

              if ((bps == 1 || bps == 2 || bps == 4) && ! alpha && spp == 1)
                {
                  if (bps == 1)
                    fill_bit2byte (tiff_mode);
                  else if (bps == 2)
                    fill_2bit2byte (tiff_mode);
                  else if (bps == 4)
                    fill_4bit2byte (tiff_mode);
                }
            }
          else
            {
              if (photomet == PHOTOMETRIC_PALETTE)
                {
                  image_type = GIMP_INDEXED;
                  layer_type = alpha ? GIMP_INDEXEDA_IMAGE : GIMP_INDEXED_IMAGE;
                }
              else
                {
                  image_type = GIMP_GRAY;
                  layer_type = alpha ? GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE;
                }
            }

          if (photomet == PHOTOMETRIC_PALETTE)
            {
              /* Do nothing here, handled later.
               * Didn't want more indenting in the next part. */
            }
          else if (alpha)
            {
              if (tsvals.save_transp_pixels)
                {
                  if (profile_linear)
                    {
                      base_format = babl_format_new (babl_model ("YA"),
                                                     type,
                                                     babl_component ("Y"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                  else
                    {
                      base_format = babl_format_new (babl_model ("Y'A"),
                                                     type,
                                                     babl_component ("Y'"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                }
              else
                {
                  if (profile_linear)
                    {
                      base_format = babl_format_new (babl_model ("YaA"),
                                                     type,
                                                     babl_component ("Ya"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                  else
                    {
                      base_format = babl_format_new (babl_model ("Y'aA"),
                                                     type,
                                                     babl_component ("Y'a"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                }
            }
          else
            {
              if (profile_linear)
                {
                  base_format = babl_format_new (babl_model ("Y"),
                                                 type,
                                                 babl_component ("Y"),
                                                 NULL);
                }
              else
                {
                  base_format = babl_format_new (babl_model ("Y'"),
                                                 type,
                                                 babl_component ("Y'"),
                                                 NULL);
                }
            }
          break;

        case PHOTOMETRIC_RGB:
          image_type = GIMP_RGB;
          layer_type = alpha ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE;

          if (alpha)
            {
              if (tsvals.save_transp_pixels)
                {
                  if (profile_linear)
                    {
                      base_format = babl_format_new (babl_model ("RGBA"),
                                                     type,
                                                     babl_component ("R"),
                                                     babl_component ("G"),
                                                     babl_component ("B"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                  else
                    {
                      base_format = babl_format_new (babl_model ("R'G'B'A"),
                                                     type,
                                                     babl_component ("R'"),
                                                     babl_component ("G'"),
                                                     babl_component ("B'"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                }
              else
                {
                  if (profile_linear)
                    {
                      base_format = babl_format_new (babl_model ("RaGaBaA"),
                                                     type,
                                                     babl_component ("Ra"),
                                                     babl_component ("Ga"),
                                                     babl_component ("Ba"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                  else
                    {
                      base_format = babl_format_new (babl_model ("R'aG'aB'aA"),
                                                     type,
                                                     babl_component ("R'a"),
                                                     babl_component ("G'a"),
                                                     babl_component ("B'a"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                }
            }
          else
            {
              if (profile_linear)
                {
                  base_format = babl_format_new (babl_model ("RGB"),
                                                 type,
                                                 babl_component ("R"),
                                                 babl_component ("G"),
                                                 babl_component ("B"),
                                                 NULL);
                }
              else
                {
                  base_format = babl_format_new (babl_model ("R'G'B'"),
                                                 type,
                                                 babl_component ("R'"),
                                                 babl_component ("G'"),
                                                 babl_component ("B'"),
                                                 NULL);
                }
            }
          break;

        case PHOTOMETRIC_SEPARATED:
          layer_type = alpha ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE;
          /* It's possible that a CMYK image might not have an
           * attached profile, so we'll check for it and set up
           * space accordingly
           */
          if (profile && gimp_color_profile_is_cmyk (profile))
            {
              space = gimp_color_profile_get_space (profile,
                                                    GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                    error);
              g_clear_object (&profile);
            }
          else
            {
              space = NULL;
            }

          if (alpha)
            base_format = babl_format_new (babl_model ("CMYKA"),
                                           type,
                                           babl_component ("Cyan"),
                                           babl_component ("Magenta"),
                                           babl_component ("Yellow"),
                                           babl_component ("Key"),
                                           babl_component ("A"),
                                           NULL);
          else
            base_format = babl_format_new (babl_model ("CMYK"),
                                           type,
                                           babl_component ("Cyan"),
                                           babl_component ("Magenta"),
                                           babl_component ("Yellow"),
                                           babl_component ("Key"),
                                           NULL);

          base_format =
            babl_format_with_space (babl_format_get_encoding (base_format),
                                    space);
          break;

        default:
          g_printerr ("photomet: %d (%d)\n", photomet, PHOTOMETRIC_PALETTE);
          worst_case = TRUE;
          break;
        }

      /* attach a parasite containing the compression */
      {
        guint16 compression = COMPRESSION_NONE;

        if (TIFFGetField (tif, TIFFTAG_COMPRESSION, &compression))
          {
            switch (compression)
              {
              case COMPRESSION_NONE:
              case COMPRESSION_LZW:
              case COMPRESSION_PACKBITS:
              case COMPRESSION_DEFLATE:
              case COMPRESSION_ADOBE_DEFLATE:
              case COMPRESSION_JPEG:
              case COMPRESSION_CCITTFAX3:
              case COMPRESSION_CCITTFAX4:
                break;

              case COMPRESSION_OJPEG:
                worst_case  = TRUE;
                compression = COMPRESSION_JPEG;
                break;

              default:
                g_message (_("Invalid or unknown compression %u. "
                           "Setting compression to none."),
                           compression);
                compression = COMPRESSION_NONE;
                break;
              }
          }

        save_vals.compression = compression;
      }

      if (worst_case)
        {
          image_type  = GIMP_RGB;
          layer_type  = GIMP_RGBA_IMAGE;

          if (profile_linear)
            {
              base_format = babl_format_new (babl_model ("RaGaBaA"),
                                             type,
                                             babl_component ("Ra"),
                                             babl_component ("Ga"),
                                             babl_component ("Ba"),
                                             babl_component ("A"),
                                             NULL);
            }
          else
            {
              base_format = babl_format_new (babl_model ("R'aG'aB'aA"),
                                             type,
                                             babl_component ("R'a"),
                                             babl_component ("G'a"),
                                             babl_component ("B'a"),
                                             babl_component ("A"),
                                             NULL);
            }
        }

      if (pages.target == GIMP_PAGE_SELECTOR_TARGET_LAYERS)
        {
          if (li == 0)
            {
              first_image_type = image_type;
            }
          else if (image_type != first_image_type)
            {
              continue;
            }
        }

      if ((pages.target == GIMP_PAGE_SELECTOR_TARGET_IMAGES) || (! *image))
        {
          *image = gimp_image_new_with_precision (cols, rows, image_type,
                                                  image_precision);

          if (*image < 1)
            {
              TIFFClose (tif);
              g_message (_("Could not create a new image: %s"),
                         gimp_get_pdb_error ());
              return GIMP_PDB_EXECUTION_ERROR;
            }

          gimp_image_undo_disable (*image);

          if (pages.target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
            {
              gchar *fname = g_strdup_printf ("%s-%d", g_file_get_uri (file),
                                              ilayer);

              gimp_image_set_filename (*image, fname);
              g_free (fname);

              images_list = g_list_prepend (images_list,
                                            GINT_TO_POINTER (*image));
            }
          else if (pages.o_pages != pages.n_pages)
            {
              gchar *fname = g_strdup_printf (_("%s-%d-of-%d-pages"),
                                              g_file_get_uri (file),
                                              pages.n_pages, pages.o_pages);

              gimp_image_set_filename (*image, fname);
              g_free (fname);
            }
          else
            {
              gimp_image_set_filename (*image, g_file_get_uri (file));
            }
        }

      /* attach non-CMYK color profile */
      if (profile)
        {
          if (pages.target == GIMP_PAGE_SELECTOR_TARGET_IMAGES || profile == first_profile)
            gimp_image_set_color_profile (*image, profile);

          g_object_unref (profile);
        }

      /* attach parasites */
      {
        GimpParasite *parasite;
        const gchar  *img_desc;

        parasite = gimp_parasite_new ("tiff-save-options", 0,
                                      sizeof (save_vals), &save_vals);
        gimp_image_attach_parasite (*image, parasite);
        gimp_parasite_free (parasite);

        /* Attach a parasite containing the image description.
         * Pretend to be a gimp comment so other plugins will use this
         * description as an image comment where appropriate.
         */
        if (TIFFGetField (tif, TIFFTAG_IMAGEDESCRIPTION, &img_desc) &&
            g_utf8_validate (img_desc, -1, NULL))
          {
            parasite = gimp_parasite_new ("gimp-comment",
                                          GIMP_PARASITE_PERSISTENT,
                                          strlen (img_desc) + 1, img_desc);
            gimp_image_attach_parasite (*image, parasite);
            gimp_parasite_free (parasite);
          }
      }

      /* Attach GeoTIFF Tags as Parasite, If available */
      {
        GimpParasite *parasite    = NULL;
        void         *geotag_data = NULL;
        uint32        count       = 0;

        if (TIFFGetField (tif, GEOTIFF_MODELPIXELSCALE, &count, &geotag_data))
          {
            parasite = gimp_parasite_new ("Gimp_GeoTIFF_ModelPixelScale",
                                          GIMP_PARASITE_PERSISTENT,
                                          (TIFFDataWidth (TIFF_DOUBLE) * count),
                                          geotag_data);
            gimp_image_attach_parasite (*image, parasite);
            gimp_parasite_free (parasite);
          }

        if (TIFFGetField (tif, GEOTIFF_MODELTIEPOINT, &count, &geotag_data))
          {
            parasite = gimp_parasite_new ("Gimp_GeoTIFF_ModelTiePoint",
                                          GIMP_PARASITE_PERSISTENT,
                                          (TIFFDataWidth (TIFF_DOUBLE) * count),
                                          geotag_data);
            gimp_image_attach_parasite (*image, parasite);
            gimp_parasite_free (parasite);
          }

        if (TIFFGetField (tif, GEOTIFF_MODELTRANSFORMATION, &count, &geotag_data))
          {
            parasite = gimp_parasite_new ("Gimp_GeoTIFF_ModelTransformation",
                                          GIMP_PARASITE_PERSISTENT,
                                          (TIFFDataWidth (TIFF_DOUBLE) * count),
                                          geotag_data);
            gimp_image_attach_parasite (*image, parasite);
            gimp_parasite_free (parasite);
          }

        if (TIFFGetField (tif, GEOTIFF_KEYDIRECTORY, &count, &geotag_data) )
          {
            parasite = gimp_parasite_new ("Gimp_GeoTIFF_KeyDirectory",
                                          GIMP_PARASITE_PERSISTENT,
                                          (TIFFDataWidth (TIFF_SHORT) * count),
                                          geotag_data);
            gimp_image_attach_parasite (*image, parasite);
            gimp_parasite_free (parasite);
          }

        if (TIFFGetField (tif, GEOTIFF_DOUBLEPARAMS, &count, &geotag_data))
          {
            parasite = gimp_parasite_new ("Gimp_GeoTIFF_DoubleParams",
                                          GIMP_PARASITE_PERSISTENT,
                                          (TIFFDataWidth (TIFF_DOUBLE) * count),
                                          geotag_data);
            gimp_image_attach_parasite (*image, parasite);
            gimp_parasite_free (parasite);
          }

        if (TIFFGetField (tif, GEOTIFF_ASCIIPARAMS, &count, &geotag_data))
          {
            parasite = gimp_parasite_new ("Gimp_GeoTIFF_Asciiparams",
                                          GIMP_PARASITE_PERSISTENT,
                                          (TIFFDataWidth (TIFF_ASCII) * count),
                                          geotag_data);
            gimp_image_attach_parasite (*image, parasite);
            gimp_parasite_free (parasite);
          }
      }

      /* any resolution info in the file? */
      {
        gfloat   xres = 72.0;
        gfloat   yres = 72.0;
        gushort  read_unit;
        GimpUnit unit = GIMP_UNIT_PIXEL; /* invalid unit */

        if (TIFFGetField (tif, TIFFTAG_XRESOLUTION, &xres))
          {
            if (TIFFGetField (tif, TIFFTAG_YRESOLUTION, &yres))
              {
                if (TIFFGetFieldDefaulted (tif, TIFFTAG_RESOLUTIONUNIT,
                                           &read_unit))
                  {
                    switch (read_unit)
                      {
                      case RESUNIT_NONE:
                        /* ImageMagick writes files with this silly resunit */
                        break;

                      case RESUNIT_INCH:
                        unit = GIMP_UNIT_INCH;
                        break;

                      case RESUNIT_CENTIMETER:
                        xres *= 2.54;
                        yres *= 2.54;
                        unit = GIMP_UNIT_MM; /* this is our default metric unit */
                        break;

                      default:
                        g_message (_("Unknown resolution "
                                   "unit type %d, assuming dpi"), read_unit);
                        break;
                      }
                  }
                else
                  {
                    /* no res unit tag */

                    /* old AppleScan software produces these */
                    g_message (_("Warning: resolution specified without "
                               "unit type, assuming dpi"));
                  }
              }
            else
              {
                /* xres but no yres */

                g_message (_("Warning: no y resolution info, assuming same as x"));
                yres = xres;
              }

            /* now set the new image's resolution info */

            /* If it is invalid, instead of forcing 72dpi, do not set
             * the resolution at all. Gimp will then use the default
             * set by the user
             */
            if (read_unit != RESUNIT_NONE)
              {
                gimp_image_set_resolution (*image, xres, yres);
                if (unit != GIMP_UNIT_PIXEL)
                  gimp_image_set_unit (*image, unit);

                *resolution_loaded = TRUE;
              }
          }

        /* no x res tag => we assume we have no resolution info, so we
         * don't care.  Older versions of this plugin used to write
         * files with no resolution tags at all.
         */

        /* TODO: haven't caught the case where yres tag is present,
         * but not xres.  This is left as an exercise for the reader -
         * they should feel free to shoot the author of the broken
         * program that produced the damaged TIFF file in the first
         * place.
         */

        /* handle layer offset */
        if (! TIFFGetField (tif, TIFFTAG_XPOSITION, &layer_offset_x))
          layer_offset_x = 0.0;

        if (! TIFFGetField (tif, TIFFTAG_YPOSITION, &layer_offset_y))
          layer_offset_y = 0.0;

        /* round floating point position to integer position required
         * by GIMP
         */
        layer_offset_x_pixel = ROUND (layer_offset_x * xres);
        layer_offset_y_pixel = ROUND (layer_offset_y * yres);
      }

      /* Install colormap for INDEXED images only */
      if (image_type == GIMP_INDEXED)
        {
          guchar   cmap[768];

          if (photomet == PHOTOMETRIC_PALETTE)
            {
              gushort *redmap;
              gushort *greenmap;
              gushort *bluemap;
              gint     i, j;

              if (! TIFFGetField (tif, TIFFTAG_COLORMAP,
                                  &redmap, &greenmap, &bluemap))
                {
                  TIFFClose (tif);
                  g_message (_("Could not get colormaps from '%s'"),
                             gimp_file_get_utf8_name (file));
                  return GIMP_PDB_EXECUTION_ERROR;
                }

              for (i = 0, j = 0; i < (1 << bps); i++)
                {
                  cmap[j++] = redmap[i] >> 8;
                  cmap[j++] = greenmap[i] >> 8;
                  cmap[j++] = bluemap[i] >> 8;
                }

            }
          else if (photomet == PHOTOMETRIC_MINISBLACK)
            {
              gint i, j;

              if (bps == 1)
                {
                  for (i = 0, j = 0; i < (1 << bps); i++)
                    {
                      cmap[j++] = _1_to_8_bitmap[i];
                      cmap[j++] = _1_to_8_bitmap[i];
                      cmap[j++] = _1_to_8_bitmap[i];
                    }
                }
              else if (bps == 2)
                {
                  for (i = 0, j = 0; i < (1 << bps); i++)
                    {
                      cmap[j++] = _2_to_8_bitmap[i];
                      cmap[j++] = _2_to_8_bitmap[i];
                      cmap[j++] = _2_to_8_bitmap[i];
                    }
                }
              else if (bps == 4)
                {
                  for (i = 0, j = 0; i < (1 << bps); i++)
                    {
                      cmap[j++] = _4_to_8_bitmap[i];
                      cmap[j++] = _4_to_8_bitmap[i];
                      cmap[j++] = _4_to_8_bitmap[i];
                    }
                }
            }
          else if (photomet == PHOTOMETRIC_MINISWHITE)
            {
              gint i, j;

              if (bps == 1)
                {
                  for (i = 0, j = 0; i < (1 << bps); i++)
                    {
                      cmap[j++] = _1_to_8_bitmap_rev[i];
                      cmap[j++] = _1_to_8_bitmap_rev[i];
                      cmap[j++] = _1_to_8_bitmap_rev[i];
                    }
                }
              else if (bps == 2)
                {
                  for (i = 0, j = 0; i < (1 << bps); i++)
                    {
                      cmap[j++] = _2_to_8_bitmap_rev[i];
                      cmap[j++] = _2_to_8_bitmap_rev[i];
                      cmap[j++] = _2_to_8_bitmap_rev[i];
                    }
                }
              else if (bps == 4)
                {
                  for (i = 0, j = 0; i < (1 << bps); i++)
                    {
                      cmap[j++] = _4_to_8_bitmap_rev[i];
                      cmap[j++] = _4_to_8_bitmap_rev[i];
                      cmap[j++] = _4_to_8_bitmap_rev[i];
                    }
                }
            }

          gimp_image_set_colormap (*image, cmap, (1 << bps));
        }

      if (pages.target != GIMP_PAGE_SELECTOR_TARGET_IMAGES)
        load_paths (tif, *image, cols, rows,
                    layer_offset_x_pixel, layer_offset_y_pixel);
      else
        load_paths (tif, *image, cols, rows, 0, 0);

      /* Allocate ChannelData for all channels, even the background layer */
      channel = g_new0 (ChannelData, extra + 1);

      /* try and use layer name from tiff file */
      name = tiff_get_page_name (tif);

      if (name)
        {
          layer = gimp_layer_new (*image, name,
                                  cols, rows,
                                  layer_type,
                                  100,
                                  gimp_image_get_default_new_layer_mode (*image));
        }
      else
        {
          gchar *name;

          if (ilayer == 0)
            name = g_strdup (_("Background"));
          else
            name = g_strdup_printf (_("Page %d"), ilayer);

          layer = gimp_layer_new (*image, name,
                                  cols, rows,
                                  layer_type,
                                  100,
                                  gimp_image_get_default_new_layer_mode (*image));
          g_free (name);
        }

      if (! base_format && image_type == GIMP_INDEXED)
        {
          /* can't create the palette format here, need to get it from
           * an existing layer
           */
          base_format = gimp_drawable_get_format (layer);
        }
      else if (! space)
        {
          base_format =
            babl_format_with_space (babl_format_get_encoding (base_format),
                                    gimp_drawable_get_format (layer));
        }

      channel[0].ID     = layer;
      channel[0].buffer = gimp_drawable_get_buffer (layer);
      channel[0].format = base_format;

      if (extra > 0 && ! worst_case)
        {
          /* Add extra channels as appropriate */
          for (i = 1; i <= extra; i++)
            {
              GimpRGB color;

              gimp_rgb_set (&color, 0.0, 0.0, 0.0);

              channel[i].ID = gimp_channel_new (*image, _("TIFF Channel"),
                                                cols, rows,
                                                100.0, &color);
              gimp_image_insert_channel (*image, channel[i].ID, -1, 0);
              channel[i].buffer = gimp_drawable_get_buffer (channel[i].ID);
              channel[i].format = babl_format_new (babl_model ("Y'"),
                                                   type,
                                                   babl_component ("Y'"),
                                                   NULL);
            }
        }

      TIFFGetField (tif, TIFFTAG_PLANARCONFIG, &planar);

      if (worst_case)
        {
          load_rgba (tif, channel);
        }
      else if (planar == PLANARCONFIG_CONTIG)
        {
          load_contiguous (tif, channel, type, bps, spp,
                           tiff_mode, is_signed, extra);
        }
      else
        {
          load_separate (tif, channel, type, bps, spp,
                         tiff_mode, is_signed, extra);
        }

      if (TIFFGetField (tif, TIFFTAG_ORIENTATION, &orientation))
        {
          gboolean flip_horizontal = FALSE;
          gboolean flip_vertical   = FALSE;

          switch (orientation)
            {
            case ORIENTATION_TOPLEFT:
              break;

            case ORIENTATION_TOPRIGHT:
              flip_horizontal = TRUE;
              break;

            case ORIENTATION_BOTRIGHT:
              flip_horizontal = TRUE;
              flip_vertical   = TRUE;
              break;

            case ORIENTATION_BOTLEFT:
              flip_vertical = TRUE;
              break;

            default:
              g_warning ("Orientation %d not handled yet!", orientation);
              break;
            }

          if (flip_horizontal)
            gimp_item_transform_flip_simple (layer,
                                             GIMP_ORIENTATION_HORIZONTAL,
                                             TRUE /* auto_center */,
                                             -1.0 /* axis */);

          if (flip_vertical)
            gimp_item_transform_flip_simple (layer,
                                             GIMP_ORIENTATION_VERTICAL,
                                             TRUE /* auto_center */,
                                             -1.0 /* axis */);
        }

      for (i = 0; i <= extra; i++)
        {
          if (channel[i].buffer)
            g_object_unref (channel[i].buffer);
        }

      g_free (channel);
      channel = NULL;

      if (pages.target != GIMP_PAGE_SELECTOR_TARGET_IMAGES)
        {
          /* compute bounding box of all layers read so far */
          if (min_col > layer_offset_x_pixel)
            min_col = layer_offset_x_pixel;
          if (min_row > layer_offset_y_pixel)
            min_row = layer_offset_y_pixel;

          if (max_col < layer_offset_x_pixel + cols)
            max_col = layer_offset_x_pixel + cols;
          if (max_row < layer_offset_y_pixel + rows)
            max_row = layer_offset_y_pixel + rows;

          /* position the layer */
          if (layer_offset_x_pixel > 0 ||
              layer_offset_y_pixel > 0)
            {
              gimp_layer_set_offsets (layer,
                                      layer_offset_x_pixel,
                                      layer_offset_y_pixel);
            }
        }

      gimp_image_insert_layer (*image, layer, -1, -1);

      if (pages.target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
        {
          gimp_image_undo_enable (*image);
          gimp_image_clean_all (*image);
        }

      gimp_progress_update (1.0);
    }
  g_clear_object (&first_profile);

  if (pages.target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
    {
      GList *list = images_list;

      if (list)
        {
          *image = GPOINTER_TO_INT (list->data);

          list = g_list_next (list);
        }

      for (; list; list = g_list_next (list))
        {
          gimp_display_new (GPOINTER_TO_INT (list->data));
        }

      g_list_free (images_list);
    }
  else
    {
      if (! (*image))
        {
          TIFFClose (tif);
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("No data could be read from TIFF '%s'. The file is probably corrupted."),
                       gimp_file_get_utf8_name (file));

          return GIMP_PDB_EXECUTION_ERROR;
        }

      if (pages.keep_empty_space)
        {
          /* unfortunately we have no idea about empty space
             at the bottom/right of layers */
          min_col = 0;
          min_row = 0;
        }

      /* resize image to bounding box of all layers */
      gimp_image_resize (*image,
                         max_col - min_col, max_row - min_row,
                         -min_col, -min_row);

      gimp_image_undo_enable (*image);
    }

  g_free (pages.pages);
  TIFFClose (tif);

  return GIMP_PDB_SUCCESS;
}

static GimpColorProfile *
load_profile (TIFF *tif)
{
  GimpColorProfile *profile = NULL;

#ifdef TIFFTAG_ICCPROFILE
  /* If TIFFTAG_ICCPROFILE is defined we are dealing with a
   * libtiff version that can handle ICC profiles. Otherwise just
   * return a NULL profile.
   */
  uint32  profile_size;
  guchar *icc_profile;

  /* set the ICC profile - if found in the TIFF */
  if (TIFFGetField (tif, TIFFTAG_ICCPROFILE, &profile_size, &icc_profile))
    {
      profile = gimp_color_profile_new_from_icc_profile (icc_profile,
                                                         profile_size,
                                                         NULL);
    }
#endif

  return profile;
}

static void
load_rgba (TIFF        *tif,
           ChannelData *channel)
{
  guint32  image_width;
  guint32  image_height;
  guint32  row;
  guint32 *buffer;

  g_printerr ("%s\n", __func__);

  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH,  &image_width);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &image_height);

  buffer = g_new (uint32, image_width * image_height);

  if (! TIFFReadRGBAImage (tif, image_width, image_height, buffer, 0))
    {
      g_message (_("%s: Unsupported image format, no RGBA loader available"),
                 G_STRFUNC);
      g_free (buffer);
      return;
    }

  for (row = 0; row < image_height; row++)
    {
#if G_BYTE_ORDER != G_LITTLE_ENDIAN
      /* Make sure our channels are in the right order */
      guint32 row_start = row * image_width;
      guint32 row_end   = row_start + image_width;
      guint32 i;

      for (i = row_start; i < row_end; i++)
        buffer[i] = GUINT32_TO_LE (buffer[i]);
#endif

      gegl_buffer_set (channel[0].buffer,
                       GEGL_RECTANGLE (0, image_height - row - 1,
                                       image_width, 1),
                       0, channel[0].format,
                       ((guchar *) buffer) + row * image_width * 4,
                       GEGL_AUTO_ROWSTRIDE);

      if ((row % 32) == 0)
        gimp_progress_update ((gdouble) row / (gdouble) image_height);
    }

  g_free (buffer);
}

static void
load_paths (TIFF *tif,
            gint  image,
            gint  width,
            gint  height,
            gint  offset_x,
            gint  offset_y)
{
  gsize  n_bytes;
  gchar *bytes;
  gint   path_index;
  gsize  pos;

  if (! TIFFGetField (tif, TIFFTAG_PHOTOSHOP, &n_bytes, &bytes))
    return;

  path_index = 0;
  pos        = 0;

  while (pos < n_bytes)
    {
      guint16  id;
      gsize    len;
      gchar   *name;
      guint32 *val32;
      guint16 *val16;

      if (n_bytes-pos < 7 ||
          strncmp (bytes + pos, "8BIM", 4) != 0)
        break;

      pos += 4;

      val16 = (guint16 *) (bytes + pos);
      id = GUINT16_FROM_BE (*val16);
      pos += 2;

      /* g_printerr ("id: %x\n", id); */
      len = (guchar) bytes[pos];

      if (n_bytes - pos < len + 1)
        break;   /* block not big enough */

      /* do we have the UTF-marker? is it valid UTF-8?
       * if so, we assume an utf-8 encoded name, otherwise we
       * assume iso8859-1
       */
      name = bytes + pos + 1;
      if (len >= 3 &&
          name[0] == '\xEF' && name[1] == '\xBB' && name[2] == '\xBF' &&
          g_utf8_validate (name, len, NULL))
        {
          name = g_strndup (name + 3, len - 3);
        }
      else
        {
          name = g_convert (name, len, "utf-8", "iso8859-1", NULL, NULL, NULL);
        }

      if (! name)
        name = g_strdup ("(imported path)");

      pos += len + 1;

      if (pos % 2)  /* padding */
        pos++;

      if (n_bytes - pos < 4)
        break;   /* block not big enough */

      val32 = (guint32 *) (bytes + pos);
      len = GUINT32_FROM_BE (*val32);
      pos += 4;

      if (n_bytes - pos < len)
        break;   /* block not big enough */

      if (id >= 2000 && id <= 2998)
        {
          /* path information */
          guint16   type;
          gint      rec = pos;
          gint32    vectors;
          gdouble  *points          = NULL;
          gint      expected_points = 0;
          gint      pointcount      = 0;
          gboolean  closed          = FALSE;

          vectors = gimp_vectors_new (image, name);
          gimp_image_insert_vectors (image, vectors, -1, path_index);
          path_index++;

          while (rec < pos + len)
            {
              /* path records */
              val16 = (guint16 *) (bytes + rec);
              type = GUINT16_FROM_BE (*val16);

              switch (type)
                {
                case 0:  /* new closed subpath */
                case 3:  /* new open subpath */
                  val16 = (guint16 *) (bytes + rec + 2);
                  expected_points = GUINT16_FROM_BE (*val16);
                  pointcount = 0;
                  closed = (type == 0);

                  if (n_bytes - rec < (expected_points + 1) * 26)
                    {
                      g_printerr ("not enough point records\n");
                      rec = pos + len;
                      continue;
                    }

                  if (points)
                    g_free (points);
                  points = g_new (gdouble, expected_points * 6);
                  break;

                case 1:  /* closed subpath bezier knot, linked */
                case 2:  /* closed subpath bezier knot, unlinked */
                case 4:  /* open subpath bezier knot, linked */
                case 5:  /* open subpath bezier knot, unlinked */
                  /* since we already know if the subpath is open
                   * or closed and since we don't differentiate between
                   * linked and unlinked, just treat all the same...  */

                  if (pointcount < expected_points)
                    {
                      gint j;

                      for (j = 0; j < 6; j++)
                        {
                          gdouble f;
                          guint32 coord;

                          const gint size = j % 2 ? width : height;
                          const gint offset = j % 2 ? offset_x : offset_y;

                          val32 = (guint32 *) (bytes + rec + 2 + j * 4);
                          coord = GUINT32_FROM_BE (*val32);

                          f = (double) ((gchar) ((coord >> 24) & 0xFF)) +
                              (double) (coord & 0x00FFFFFF) /
                              (double) 0xFFFFFF;

                          /* coords are stored with vertical component
                           * first, gimp expects the horizontal
                           * component first. Sigh.
                           */
                          points[pointcount * 6 + (j ^ 1)] = f * size + offset;
                        }

                      pointcount++;

                      if (pointcount == expected_points)
                        {
                          gimp_vectors_stroke_new_from_points (vectors,
                                                               GIMP_VECTORS_STROKE_TYPE_BEZIER,
                                                               pointcount * 6,
                                                               points,
                                                               closed);
                        }
                    }
                  else
                    {
                      g_printerr ("Oops - unexpected point record\n");
                    }

                  break;

                case 6:  /* path fill rule record */
                case 7:  /* clipboard record (?) */
                case 8:  /* initial fill rule record (?) */
                  /* we cannot use this information */

                default:
                  break;
                }

              rec += 26;
            }

          if (points)
            g_free (points);
        }

      pos += len;

      if (pos % 2)  /* padding */
        pos++;

      g_free (name);
    }
}


static void
load_contiguous (TIFF         *tif,
                 ChannelData  *channel,
                 const Babl   *type,
                 gushort       bps,
                 gushort       spp,
                 TiffColorMode tiff_mode,
                 gboolean      is_signed,
                 gint          extra)
{
  guint32     image_width;
  guint32     image_height;
  guint32     tile_width;
  guint32     tile_height;
  gint        bytes_per_pixel;
  const Babl *src_format;
  guchar     *buffer;
  guchar     *bw_buffer = NULL;
  gdouble     progress  = 0.0;
  gdouble     one_row;
  guint32     y;
  gint        i;
  gboolean    needs_upscale = FALSE;

  g_printerr ("%s\n", __func__);

  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH,  &image_width);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &image_height);

  tile_width = image_width;

  if (TIFFIsTiled (tif))
    {
      TIFFGetField (tif, TIFFTAG_TILEWIDTH,  &tile_width);
      TIFFGetField (tif, TIFFTAG_TILELENGTH, &tile_height);

      buffer = g_malloc (TIFFTileSize (tif));
    }
  else
    {
      tile_width  = image_width;
      tile_height = 1;

      buffer = g_malloc (TIFFScanlineSize (tif));
    }

  if (tiff_mode != GIMP_TIFF_DEFAULT && bps < 8)
    {
      needs_upscale = TRUE;
      bw_buffer = g_malloc (tile_width * tile_height);
    }

  one_row = (gdouble) tile_height / (gdouble) image_height;

  src_format = babl_format_n (type, spp);

  /* consistency check */
  bytes_per_pixel = 0;
  for (i = 0; i <= extra; i++)
    bytes_per_pixel += babl_format_get_bytes_per_pixel (channel[i].format);

  g_printerr ("bytes_per_pixel: %d, format: %d\n",
              bytes_per_pixel,
              babl_format_get_bytes_per_pixel (src_format));

  for (y = 0; y < image_height; y += tile_height)
    {
      guint32 x;

      for (x = 0; x < image_width; x += tile_width)
        {
          GeglBuffer *src_buf;
          guint32     rows;
          guint32     cols;
          gint        offset;

          gimp_progress_update (progress + one_row *
                                ((gdouble) x / (gdouble) image_width));

          if (TIFFIsTiled (tif))
            {
              if (TIFFReadTile (tif, buffer, x, y, 0, 0) == -1)
                {
                  g_message (_("Reading tile failed. Image may be corrupt at line %d."), y);
                  g_free (buffer);
                  g_free (bw_buffer);
                  return;
                }
            }
          else if (TIFFReadScanline (tif, buffer, y, 0) == -1)
            {
              /* Error reading scanline, stop loading */
              g_message (_("Reading scanline failed. Image may be corrupt at line %d."), y);
              g_free (buffer);
              g_free (bw_buffer);
              return;
            }

          cols = MIN (image_width  - x, tile_width);
          rows = MIN (image_height - y, tile_height);

          if (needs_upscale)
            {
              if (bps == 1)
                convert_bit2byte (buffer, bw_buffer, cols, rows);
              else if (bps == 2)
                convert_2bit2byte (buffer, bw_buffer, cols, rows);
              else if (bps == 4)
                convert_4bit2byte (buffer, bw_buffer, cols, rows);
            }
          else if (is_signed)
            {
              convert_int2uint (buffer, bps, spp, cols, rows,
                                tile_width * bytes_per_pixel);
            }

          if (tiff_mode == GIMP_TIFF_GRAY_MINISWHITE && bps == 8)
            {
              convert_miniswhite (buffer, cols, rows);
            }

          src_buf = gegl_buffer_linear_new_from_data (needs_upscale ? bw_buffer : buffer,
                                                      src_format,
                                                      GEGL_RECTANGLE (0, 0, cols, rows),
                                                      tile_width * bytes_per_pixel,
                                                      NULL, NULL);

          offset = 0;

          for (i = 0; i <= extra; i++)
            {
              GeglBufferIterator *iter;
              gint                src_bpp;
              gint                dest_bpp;

              src_bpp  = babl_format_get_bytes_per_pixel (src_format);
              dest_bpp = babl_format_get_bytes_per_pixel (channel[i].format);

              iter = gegl_buffer_iterator_new (src_buf,
                                               GEGL_RECTANGLE (0, 0, cols, rows),
                                               0, NULL,
                                               GEGL_ACCESS_READ,
                                               GEGL_ABYSS_NONE, 2);
              gegl_buffer_iterator_add (iter, channel[i].buffer,
                                        GEGL_RECTANGLE (x, y, cols, rows),
                                        0, channel[i].format,
                                        GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

              while (gegl_buffer_iterator_next (iter))
                {
                  guchar *s      = iter->items[0].data;
                  guchar *d      = iter->items[1].data;
                  gint    length = iter->length;

                  s += offset;

                  while (length--)
                    {
                      memcpy (d, s, dest_bpp);
                      d += dest_bpp;
                      s += src_bpp;
                    }
                }

              offset += dest_bpp;
            }

          g_object_unref (src_buf);
        }

      progress += one_row;
    }

  g_free (buffer);
  g_free (bw_buffer);
}


static void
load_separate (TIFF         *tif,
               ChannelData  *channel,
               const Babl   *type,
               gushort       bps,
               gushort       spp,
               TiffColorMode tiff_mode,
               gboolean      is_signed,
               gint          extra)
{
  guint32     image_width;
  guint32     image_height;
  guint32     tile_width;
  guint32     tile_height;
  gint        bytes_per_pixel;
  const Babl *src_format;
  guchar     *buffer;
  guchar     *bw_buffer = NULL;
  gdouble     progress  = 0.0;
  gdouble     one_row;
  gint        i, compindex;
  gboolean    needs_upscale = FALSE;

  g_printerr ("%s\n", __func__);

  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH,  &image_width);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &image_height);

  tile_width = image_width;

  if (TIFFIsTiled (tif))
    {
      TIFFGetField (tif, TIFFTAG_TILEWIDTH,  &tile_width);
      TIFFGetField (tif, TIFFTAG_TILELENGTH, &tile_height);

      buffer = g_malloc (TIFFTileSize (tif));
    }
  else
    {
      tile_width  = image_width;
      tile_height = 1;

      buffer = g_malloc (TIFFScanlineSize (tif));
    }

  if (tiff_mode != GIMP_TIFF_DEFAULT && bps < 8)
    {
      needs_upscale = TRUE;
      bw_buffer = g_malloc (tile_width * tile_height);
    }

  one_row = (gdouble) tile_height / (gdouble) image_height;

  src_format = babl_format_n (type, 1);

  /* consistency check */
  bytes_per_pixel = 0;
  for (i = 0; i <= extra; i++)
    bytes_per_pixel += babl_format_get_bytes_per_pixel (channel[i].format);

  g_printerr ("bytes_per_pixel: %d, format: %d\n",
              bytes_per_pixel,
              babl_format_get_bytes_per_pixel (src_format));

  compindex = 0;

  for (i = 0; i <= extra; i++)
    {
      gint n_comps;
      gint src_bpp;
      gint dest_bpp;
      gint offset;
      gint j;

      n_comps  = babl_format_get_n_components (channel[i].format);
      src_bpp  = babl_format_get_bytes_per_pixel (src_format);
      dest_bpp = babl_format_get_bytes_per_pixel (channel[i].format);

      offset = 0;

      for (j = 0; j < n_comps; j++)
        {
          guint32 y;

          for (y = 0; y < image_height; y += tile_height)
            {
              guint32 x;

              for (x = 0; x < image_width; x += tile_width)
                {
                  GeglBuffer         *src_buf;
                  GeglBufferIterator *iter;
                  guint32             rows;
                  guint32             cols;

                  gimp_progress_update (progress + one_row *
                                        ((gdouble) x / (gdouble) image_width));

                  if (TIFFIsTiled (tif))
                    {
                      if (TIFFReadTile (tif, buffer, x, y, 0, compindex) == -1)
                        {
                          g_message (_("Reading tile failed. Image may be corrupt at line %d."), y);
                          g_free (buffer);
                          g_free (bw_buffer);
                          return;
                        }
                    }
                  else if (TIFFReadScanline (tif, buffer, y, compindex) == -1)
                    {
                      /* Error reading scanline, stop loading */
                      g_message (_("Reading scanline failed. Image may be corrupt at line %d."), y);
                      g_free (buffer);
                      g_free (bw_buffer);
                      return;
                    }

                  cols = MIN (image_width  - x, tile_width);
                  rows = MIN (image_height - y, tile_height);

                  if (needs_upscale)
                    {
                      if (bps == 1)
                        convert_bit2byte (buffer, bw_buffer, cols, rows);
                      else if (bps == 2)
                        convert_2bit2byte (buffer, bw_buffer, cols, rows);
                      else if (bps == 4)
                        convert_4bit2byte (buffer, bw_buffer, cols, rows);
                    }
                  else if (is_signed)
                    {
                      convert_int2uint (buffer, bps, 1, cols, rows,
                                        tile_width * bytes_per_pixel);
                    }

                  if (tiff_mode == GIMP_TIFF_GRAY_MINISWHITE && bps == 8)
                    {
                      convert_miniswhite (buffer, cols, rows);
                    }

                  src_buf = gegl_buffer_linear_new_from_data (needs_upscale ? bw_buffer : buffer,
                                                              src_format,
                                                              GEGL_RECTANGLE (0, 0, cols, rows),
                                                              GEGL_AUTO_ROWSTRIDE,
                                                              NULL, NULL);

                  iter = gegl_buffer_iterator_new (src_buf,
                                                   GEGL_RECTANGLE (0, 0, cols, rows),
                                                   0, NULL,
                                                   GEGL_ACCESS_READ,
                                                   GEGL_ABYSS_NONE, 2);
                  gegl_buffer_iterator_add (iter, channel[i].buffer,
                                            GEGL_RECTANGLE (x, y, cols, rows),
                                            0, channel[i].format,
                                            GEGL_ACCESS_READWRITE,
                                            GEGL_ABYSS_NONE);

                  while (gegl_buffer_iterator_next (iter))
                    {
                      guchar *s      = iter->items[0].data;
                      guchar *d      = iter->items[1].data;
                      gint    length = iter->length;

                      d += offset;

                      while (length--)
                        {
                          memcpy (d, s, src_bpp);
                          d += dest_bpp;
                          s += src_bpp;
                        }
                    }

                  g_object_unref (src_buf);
                }
            }

          offset += src_bpp;
          compindex++;
        }

      progress += one_row;
    }

  g_free (buffer);
  g_free (bw_buffer);
}

static void
fill_bit2byte (TiffColorMode tiff_mode)
{
  static gboolean filled = FALSE;

  guchar *dest;
  gint    i, j;

  if (filled)
    return;

  dest = bit2byte;

  if (tiff_mode == GIMP_TIFF_INDEXED)
    {
      for (j = 0; j < 256; j++)
        for (i = 7; i >= 0; i--)
          {
            *(dest++) = ((j & (1 << i)) != 0);
          }
    }
  else if (tiff_mode != GIMP_TIFF_DEFAULT)
    {
      guchar *_to_8_bitmap = NULL;

      if (tiff_mode == GIMP_TIFF_GRAY)
        _to_8_bitmap = (guchar *) &_1_to_8_bitmap;
      else if (tiff_mode == GIMP_TIFF_GRAY_MINISWHITE)
        _to_8_bitmap = (guchar *) &_1_to_8_bitmap_rev;

      for (j = 0; j < 256; j++)
        for (i = 7; i >= 0; i--)
          {
            gint idx;

            idx = ((j & (1 << i)) != 0);
            *(dest++) = _to_8_bitmap[idx];
          }
    }

  filled = TRUE;
}

static void
fill_2bit2byte (TiffColorMode tiff_mode)
{
  static gboolean filled2 = FALSE;

  guchar *dest;
  gint    i, j;

  if (filled2)
    return;

  dest = _2bit2byte;

  if (tiff_mode == GIMP_TIFF_INDEXED)
    {
      for (j = 0; j < 256; j++)
        {
        for (i = 3; i >= 0; i--)
          {
            *(dest++) = ((j & (3 << (2*i))) >> (2*i));
          }
        }
    }
  else if (tiff_mode != GIMP_TIFF_DEFAULT)
    {
      guchar *_to_8_bitmap = NULL;

      if (tiff_mode == GIMP_TIFF_GRAY)
        _to_8_bitmap = (guchar *) &_2_to_8_bitmap;
      else if (tiff_mode == GIMP_TIFF_GRAY_MINISWHITE)
        _to_8_bitmap = (guchar *) &_2_to_8_bitmap_rev;

      for (j = 0; j < 256; j++)
        {
        for (i = 3; i >= 0; i--)
          {
            gint idx;

            idx = ((j & (3 << (2*i))) >> (2*i));
            *(dest++) = _to_8_bitmap[idx];
          }
        }
    }

  filled2 = TRUE;
}

static void
fill_4bit2byte (TiffColorMode tiff_mode)
{
  static gboolean filled4 = FALSE;

  guchar *dest;
  gint    i, j;

  if (filled4)
    return;

  dest = _4bit2byte;

  if (tiff_mode == GIMP_TIFF_INDEXED)
    {
      for (j = 0; j < 256; j++)
        {
        for (i = 1; i >= 0; i--)
          {
            *(dest++) = ((j & (15 << (4*i))) >> (4*i));
          }
        }
    }
  else if (tiff_mode != GIMP_TIFF_DEFAULT)
    {
      guchar *_to_8_bitmap = NULL;

      if (tiff_mode == GIMP_TIFF_GRAY)
        _to_8_bitmap = (guchar *) &_4_to_8_bitmap;
      else if (tiff_mode == GIMP_TIFF_GRAY_MINISWHITE)
        _to_8_bitmap = (guchar *) &_4_to_8_bitmap_rev;

      for (j = 0; j < 256; j++)
        {
        for (i = 1; i >= 0; i--)
          {
            gint idx;

            idx = ((j & (15 << (4*i))) >> (4*i));
            *(dest++) = _to_8_bitmap[idx];
          }
        }
    }

  filled4 = TRUE;
}

static void
convert_bit2byte (const guchar *src,
                  guchar       *dest,
                  gint          width,
                  gint          height)
{
  gint y;

  for (y = 0; y < height; y++)
    {
      gint x = width;

      while (x >= 8)
        {
          memcpy (dest, bit2byte + *src * 8, 8);
          dest += 8;
          x -= 8;
          src++;
        }

      if (x > 0)
        {
          memcpy (dest, bit2byte + *src * 8, x);
          dest += x;
          src++;
        }
    }
}

static void
convert_2bit2byte (const guchar *src,
                   guchar       *dest,
                   gint          width,
                   gint          height)
{
  gint y;

  for (y = 0; y < height; y++)
    {
      gint x = width;

      while (x >= 4)
        {
          memcpy (dest, _2bit2byte + *src * 4, 4);
          dest += 4;
          x -= 4;
          src++;
        }

      if (x > 0)
        {
          memcpy (dest, _2bit2byte + *src * 4, x);
          dest += x;
          src++;
        }
    }
}

static void
convert_4bit2byte (const guchar *src,
                   guchar       *dest,
                   gint          width,
                   gint          height)
{
  gint y;

  for (y = 0; y < height; y++)
    {
      gint x = width;

      while (x >= 2)
        {
          memcpy (dest, _4bit2byte + *src * 2, 2);
          dest += 2;
          x -= 2;
          src++;
        }

      if (x > 0)
        {
          memcpy (dest, _4bit2byte + *src * 2, x);
          dest += x;
          src++;
        }
    }
}

static void
convert_miniswhite (guchar *buffer,
                    gint    width,
                    gint    height)
{
  gint    y;
  guchar *buf = buffer;

  for (y = 0; y < height; y++)
    {
      gint x;

      for (x = 0; x < width; x++)
        {
          *buf = ~*buf;
          buf++;
        }
    }
}

static void
convert_int2uint (guchar *buffer,
                  gint    bps,
                  gint    spp,
                  gint    width,
                  gint    height,
                  gint    stride)
{
  gint bytes_per_pixel = bps / 8;
  gint y;

  for (y = 0; y < height; y++)
    {
      guchar *d = buffer + stride * y;
      gint    x;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
      d += bytes_per_pixel - 1;
#endif

      for (x = 0; x < width * spp; x++)
        {
          *d ^= 0x80;

          d += bytes_per_pixel;
        }
    }
}

static gboolean
load_dialog (TIFF              *tif,
             const gchar       *help_id,
             TiffSelectedPages *pages,
             const gchar       *extra_message,
             DefaultExtra      *default_extra)
{
  GtkWidget  *dialog;
  GtkWidget  *vbox;
  GtkWidget  *selector    = NULL;
  GtkWidget  *crop_option = NULL;
  GtkWidget  *extra_radio = NULL;
  gboolean    run;

  dialog = gimp_dialog_new (_("Import from TIFF"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, help_id,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Import"), GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);

  if (pages->n_pages > 1)
    {
      gint i, j;

      /* Page Selector */
      selector = gimp_page_selector_new ();
      gtk_widget_set_size_request (selector, 300, 200);
      gtk_box_pack_start (GTK_BOX (vbox), selector, TRUE, TRUE, 0);

      gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (selector),
                                      pages->n_filtered_pages);
      gimp_page_selector_set_target (GIMP_PAGE_SELECTOR (selector), pages->target);

      for (i = 0, j = 0; i < pages->n_pages && j < pages->n_filtered_pages; i++)
        {
          if (pages->filtered_pages[i] != -1)
            {
              const gchar *name = tiff_get_page_name (tif);

              if (name)
                gimp_page_selector_set_page_label (GIMP_PAGE_SELECTOR (selector),
                                                   j, name);
              j++;
            }

          TIFFReadDirectory (tif);
        }

      g_signal_connect_swapped (selector, "activate",
                                G_CALLBACK (gtk_window_activate_default),
                                dialog);

      /* Option to shrink the loaded image to its bounding box
         or keep as much empty space as possible.
         Note that there seems to be no way to keep the empty
         space on the right and bottom. */
      crop_option = gtk_check_button_new_with_mnemonic (_("_Keep empty space around imported layers"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (crop_option),
                                    pages->keep_empty_space);
      gtk_box_pack_start (GTK_BOX (vbox), crop_option, TRUE, TRUE, 0);
    }

  if (extra_message)
    {
      GtkWidget *warning;

      warning = g_object_new (GIMP_TYPE_HINT_BOX,
                             "icon-name", GIMP_ICON_DIALOG_WARNING,
                             "hint",      extra_message,
                             NULL);
      gtk_box_pack_start (GTK_BOX (vbox), warning, TRUE, TRUE, 0);
      gtk_widget_show (warning);

      extra_radio = gimp_int_radio_group_new (TRUE, _("Process extra channel as:"),
                                              (GCallback) gimp_radio_button_update,
                                              default_extra, GIMP_TIFF_LOAD_UNASSALPHA,
                                              _("_Non-premultiplied alpha"), GIMP_TIFF_LOAD_UNASSALPHA, NULL,
                                              _("Pre_multiplied alpha"),     GIMP_TIFF_LOAD_ASSOCALPHA, NULL,
                                              _("Channe_l"),                 GIMP_TIFF_LOAD_CHANNEL,    NULL,
                                              NULL);
      gtk_box_pack_start (GTK_BOX (vbox), extra_radio, TRUE, TRUE, 0);
      gtk_widget_show (extra_radio);
    }

  /* Setup done; display the dialog */
  gtk_widget_show_all (dialog);

  /* run the dialog */
  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      if (pages->n_pages > 1)
        {
          pages->target =
            gimp_page_selector_get_target (GIMP_PAGE_SELECTOR (selector));

          pages->pages =
            gimp_page_selector_get_selected_pages (GIMP_PAGE_SELECTOR (selector),
                                                   &pages->n_pages);

          pages->keep_empty_space =
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (crop_option));

          /* select all if none selected */
          if (pages->n_pages == 0)
            {
              gimp_page_selector_select_all (GIMP_PAGE_SELECTOR (selector));

              pages->pages =
                gimp_page_selector_get_selected_pages (GIMP_PAGE_SELECTOR (selector),
                                                       &pages->n_pages);
            }
        }
    }

  return run;
}
