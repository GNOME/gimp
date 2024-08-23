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

#include <gio/gio.h>
#include <glib/gstdio.h>

#include <tiffio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "file-tiff.h"
#include "file-tiff-io.h"
#include "file-tiff-load.h"

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_ROLE "gimp-file-tiff-load"


typedef struct
{
  GimpDrawable *drawable;
  GeglBuffer   *buffer;
  const Babl   *format;
  guchar       *pixels;
  guchar       *pixel;
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

static GimpColorProfile * load_profile     (TIFF                *tif);

static void               load_rgba        (TIFF                *tif,
                                            ChannelData         *channel);
static void               load_contiguous  (TIFF                *tif,
                                            ChannelData         *channel,
                                            const Babl          *type,
                                            gushort              bps,
                                            gushort              spp,
                                            TiffColorMode        tiff_mode,
                                            gboolean             is_signed,
                                            gint                 extra);
static void               load_separate    (TIFF                *tif,
                                            ChannelData         *channel,
                                            const Babl          *type,
                                            gushort              bps,
                                            gushort              spp,
                                            TiffColorMode        tiff_mode,
                                            gboolean             is_signed,
                                            gint                 extra);

static void       load_sketchbook_layers   (TIFF                *tif,
                                            GimpImage           *image);

static gboolean   is_non_conformant_tiff   (gushort              photomet,
                                            gushort              spp);
static gushort    get_extra_channels_count (gushort              photomet,
                                            gushort              spp,
                                            gboolean             alpha);

static void               fill_bit2byte    (TiffColorMode        tiff_mode);
static void               fill_2bit2byte   (TiffColorMode        tiff_mode);
static void               fill_4bit2byte   (TiffColorMode        tiff_mode);
static void               convert_bit2byte (const guchar        *src,
                                            guchar              *dest,
                                            gint                 width,
                                            gint                 height);
static void              convert_2bit2byte (const guchar        *src,
                                            guchar              *dest,
                                            gint                 width,
                                            gint                 height);
static void              convert_4bit2byte (const guchar        *src,
                                            guchar              *dest,
                                            gint                 width,
                                            gint                 height);

static void             convert_miniswhite (guchar              *buffer,
                                            gint                 width,
                                            gint                 height);
static void               convert_int2uint (guchar              *buffer,
                                            gint                 bps,
                                            gint                 spp,
                                            gint                 width,
                                            gint                 height,
                                            gint                 stride);

static gboolean           load_dialog      (GimpProcedure       *procedure,
                                            GimpProcedureConfig *config,
                                            TiffSelectedPages   *pages,
                                            const gchar         *extra_message,
                                            DefaultExtra        *default_extra);

static void       tiff_dialog_show_reduced (GtkWidget           *toggle,
                                            gpointer             data);



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
load_image (GimpProcedure        *procedure,
            GFile                *file,
            GimpRunMode           run_mode,
            GimpImage           **image,
            gboolean             *resolution_loaded,
            gboolean             *profile_loaded,
            gboolean             *ps_metadata_loaded,
            GimpProcedureConfig  *config,
            GError              **error)
{
  TIFF              *tif;
  TiffSelectedPages  pages;

  GList             *images_list        = NULL;
  DefaultExtra       default_extra      = GIMP_TIFF_LOAD_UNASSALPHA;
  gint               first_image_type   = GIMP_RGB;
  gint               min_row            = G_MAXINT;
  gint               min_col            = G_MAXINT;
  gint               max_row            = 0;
  gint               max_col            = 0;
  gboolean           save_transp_pixels = FALSE;
  GimpColorProfile  *first_profile      = NULL;
  const gchar       *extra_message      = NULL;
  gint               li;
  gint               selectable_pages;
  gchar             *photoshop_data;
  gint32             photoshop_len;
  gboolean           is_cmyk            = FALSE;
  gchar             *sketchbook_info;
  gint               sketchbook_len;
  gboolean           sketchbook_layers  = FALSE;

  *image = NULL;
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

  g_object_get (config, "target", &pages.target, NULL);
  g_object_get (config, "keep-empty-space", &pages.keep_empty_space, NULL);

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
  pages.n_reducedimage_pages = pages.n_pages;

  pages.filtered_pages  = g_new0 (gint, pages.n_pages);
  for (li = 0; li < pages.n_pages; li++)
    pages.filtered_pages[li] = li;

  if (pages.n_pages == 1 || run_mode != GIMP_RUN_INTERACTIVE)
    {
      pages.pages  = g_new0 (gint, pages.n_pages);
      for (li = 0; li < pages.n_pages; li++)
        pages.pages[li] = li;
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
              pages.filtered_pages[li] = TIFF_REDUCEDFILE;
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
                  pages.filtered_pages[li] = TIFF_MISC_THUMBNAIL;
                  pages.n_filtered_pages--;
                  /* This is used to conditionally show reduced images
                   * if they're not a thumbnail
                   */
                  pages.n_reducedimage_pages--;
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

  /* Check if there exist layers saved from Alias/AutoDesk Sketchbook */
  sketchbook_layers = TIFFGetField (tif, TIFFTAG_ALIAS_LAYER_METADATA,
                                    &sketchbook_info, &sketchbook_len);

  pages.show_reduced = FALSE;
  if (pages.n_reducedimage_pages - pages.n_filtered_pages > 1)
    pages.show_reduced = TRUE;

  pages.tif = tif;

  if (run_mode == GIMP_RUN_INTERACTIVE     &&
      (pages.n_pages > 1 || extra_message) &&
      ! load_dialog (procedure, config, &pages, extra_message,
                     &default_extra))
    {
      TIFFClose (tif);
      g_clear_pointer (&pages.pages, g_free);

      return GIMP_PDB_CANCEL;
    }

  selectable_pages = pages.n_filtered_pages;
  if (pages.show_reduced)
    selectable_pages = pages.n_reducedimage_pages;

  /* Adjust pages to take filtered out pages into account. */
  if (pages.o_pages > selectable_pages)
    {
      gint fi;
      gint sel_index = 0;
      gint sel_add   = 0;

      for (fi = 0; fi < pages.o_pages && sel_index < pages.n_pages; fi++)
        {
          if ((pages.show_reduced && pages.filtered_pages[fi] == TIFF_MISC_THUMBNAIL) ||
              (! pages.show_reduced && pages.filtered_pages[fi] <= TIFF_MISC_THUMBNAIL))
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

  g_object_set (config, "target", pages.target, NULL);
  g_object_set (config, "keep-empty-space", pages.keep_empty_space, NULL);

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
      GimpLayer        *layer;
      gint              layer_type           = GIMP_RGB_IMAGE;
      float             layer_offset_x       = 0.0;
      float             layer_offset_y       = 0.0;
      gint              layer_offset_x_pixel = 0;
      gint              layer_offset_y_pixel = 0;
      gushort           extra;
      gushort          *extra_types;
      ChannelData      *channel = NULL;
      uint16_t          planar  = PLANARCONFIG_CONTIG;
      TiffColorMode     tiff_mode;
      gboolean          is_signed;
      gint              i;
      gboolean          worst_case = FALSE;
      gint              gimp_compression = GIMP_COMPRESSION_NONE;
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
            image_precision = GIMP_PRECISION_U8_NON_LINEAR;

          type = babl_type ("u8");
          break;

        case 16:
          if (sampleformat == SAMPLEFORMAT_IEEEFP)
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_HALF_LINEAR;
              else
                image_precision = GIMP_PRECISION_HALF_NON_LINEAR;

              type = babl_type ("half");
            }
          else
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_U16_LINEAR;
              else
                image_precision = GIMP_PRECISION_U16_NON_LINEAR;

              type = babl_type ("u16");
            }
          break;

        case 32:
          if (sampleformat == SAMPLEFORMAT_IEEEFP)
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_FLOAT_LINEAR;
              else
                image_precision = GIMP_PRECISION_FLOAT_NON_LINEAR;

              type = babl_type ("float");
            }
          else
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_U32_LINEAR;
              else
                image_precision = GIMP_PRECISION_U32_NON_LINEAR;

              type = babl_type ("u32");
            }
          break;

        case 64:
          if (profile_linear)
            image_precision = GIMP_PRECISION_DOUBLE_LINEAR;
          else
            image_precision = GIMP_PRECISION_DOUBLE_NON_LINEAR;

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
          save_transp_pixels = FALSE;
          extra--;
        }
      else if (extra > 0 && (extra_types[0] == EXTRASAMPLE_UNASSALPHA))
        {
          alpha = TRUE;
          save_transp_pixels = TRUE;
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
              save_transp_pixels = FALSE;
              break;
            case GIMP_TIFF_LOAD_UNASSALPHA:
              alpha = TRUE;
              save_transp_pixels = TRUE;
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
                  save_transp_pixels = FALSE;
                  break;
                case GIMP_TIFF_LOAD_UNASSALPHA:
                  alpha = TRUE;
                  save_transp_pixels = TRUE;
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
          /* Even for bps >= we may need to use tiff_mode, so always set it.
           * Currently we use it to detect the need to convert 8 bps miniswhite. */
          if (photomet == PHOTOMETRIC_PALETTE)
            tiff_mode = GIMP_TIFF_INDEXED;
          else if (photomet == PHOTOMETRIC_MINISBLACK)
            tiff_mode = GIMP_TIFF_GRAY;
          else if (photomet == PHOTOMETRIC_MINISWHITE)
            tiff_mode = GIMP_TIFF_GRAY_MINISWHITE;

          if (bps < 8)
            {
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
              if (save_transp_pixels)
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
              if (save_transp_pixels)
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
          is_cmyk = TRUE;
          if (profile && gimp_color_profile_is_cmyk (profile))
            {
              space = gimp_color_profile_get_space (profile,
                                                    GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                    error);
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

        gimp_compression = tiff_compression_to_gimp_compression (compression);
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

          if (! *image)
            {
              TIFFClose (tif);
              g_message (_("Could not create a new image: %s"),
                         gimp_pdb_get_last_error (gimp_get_pdb ()));
              return GIMP_PDB_EXECUTION_ERROR;
            }

          gimp_image_undo_disable (*image);

          if (pages.target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
            images_list = g_list_prepend (images_list, *image);
        }

      /* attach CMYK profile to GimpImage if applicable */
      if (profile && gimp_color_profile_is_cmyk (profile))
        {
          gimp_image_set_simulation_profile (*image, profile);
          g_clear_object (&profile);
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
        GString          *string;
        GimpConfigWriter *writer;
        GimpParasite     *parasite;
        const gchar      *img_desc;

        /* construct the save parasite manually instead of simply
         * creating and saving a save config object, because we want
         * it to contain only some properties
         */

        string = g_string_new (NULL);
        writer = gimp_config_writer_new_from_string (string);

        gimp_config_writer_open (writer, "compression");
        gimp_config_writer_printf (writer, "%d", gimp_compression);
        gimp_config_writer_close (writer);

        gimp_config_writer_finish (writer, NULL, NULL);

        parasite = gimp_parasite_new ("GimpProcedureConfig-file-tiff-save-last",
                                      GIMP_PARASITE_PERSISTENT,
                                      string->len + 1, string->str);
        gimp_image_attach_parasite (*image, parasite);
        gimp_parasite_free (parasite);

        g_string_free (string, TRUE);

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
        uint32_t      count       = 0;

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
        gdouble   xres      = 72.0;
        gdouble   yres      = 72.0;
        gushort   read_unit = RESUNIT_NONE;
        GimpUnit *unit      = gimp_unit_pixel (); /* invalid unit */

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
                        unit = gimp_unit_inch ();
                        break;

                      case RESUNIT_CENTIMETER:
                        xres *= 2.54;
                        yres *= 2.54;
                        unit = gimp_unit_mm (); /* this is our default metric unit */
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
                if (! isfinite (xres) ||
                    xres < GIMP_MIN_RESOLUTION || xres > GIMP_MAX_RESOLUTION ||
                    ! isfinite (yres) ||
                    yres < GIMP_MIN_RESOLUTION || yres > GIMP_MAX_RESOLUTION)
                  {
                    g_message (_("Invalid image resolution info, using default"));
                    /* We need valid xres and yres for computing
                     * layer_offset_x_pixel and layer_offset_y_pixel.
                     */
                    gimp_image_get_resolution (*image, &xres, &yres);
                  }
                else
                  {
                    gimp_image_set_resolution (*image, xres, yres);
                    if (unit != gimp_unit_pixel ())
                      gimp_image_set_unit (*image, unit);

                    *resolution_loaded = TRUE;
                  }
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

      if (extra > 99)
        {
          /* Validate number of channels to the same maximum as we use for
           * Photoshop. A higher number most likely means a corrupt image
           * and can cause GIMP to become unresponsive and/or stuck.
           * See m2-d0f86ab189cbe900ec389ca6d7464713.tif from imagetestsuite
           */
          g_message (_("Suspicious number of extra channels: %d. Possibly corrupt image."), extra);
          extra = 99;
        }

      /* Stop here if we have Alias/AutoDesk Sketchbook layers, as this
       * would just be the composite image */
      if (sketchbook_layers)
        break;

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
          base_format = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
        }
      else if (! space)
        {
          base_format =
            babl_format_with_space (babl_format_get_encoding (base_format),
                                    gimp_drawable_get_format (GIMP_DRAWABLE (layer)));
        }

      channel[0].drawable = GIMP_DRAWABLE (layer);
      channel[0].buffer   = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
      channel[0].format   = base_format;

      if (extra > 0 && ! worst_case)
        {
          /* Add extra channels as appropriate */
          for (i = 1; i <= extra; i++)
            {
              GeglColor *color = gegl_color_new ("black");

              channel[i].drawable = GIMP_DRAWABLE (gimp_channel_new (*image, _("TIFF Channel"),
                                                                     cols, rows, 100.0, color));
              g_object_unref (color);
              gimp_image_insert_channel (*image, GIMP_CHANNEL (channel[i].drawable), NULL, 0);
              channel[i].buffer = gimp_drawable_get_buffer (channel[i].drawable);

              /* Unlike color channels, we don't care about the source
               * TRC for extra channels. We just want to import them
               * as-is without any value conversion. Since extra
               * channels are always linear in GIMP, we consider TIFF
               * extra channels with unspecified data to be linear too.
               * Only exception are 8-bit non-linear images whose
               * channel are Y' for legacy/compatibility reasons.
               */
              if (image_precision == GIMP_PRECISION_U8_NON_LINEAR)
                channel[i].format = babl_format_new (babl_model ("Y'"),
                                                     type,
                                                     babl_component ("Y'"),
                                                     NULL);
              else
                channel[i].format = babl_format_new (babl_model ("Y"),
                                                     type,
                                                     babl_component ("Y"),
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
            gimp_item_transform_flip_simple (GIMP_ITEM (layer),
                                             GIMP_ORIENTATION_HORIZONTAL,
                                             TRUE /* auto_center */,
                                             -1.0 /* axis */);

          if (flip_vertical)
            gimp_item_transform_flip_simple (GIMP_ITEM (layer),
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

      gimp_image_insert_layer (*image, layer, NULL, -1);

      if (pages.target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
        {
          gimp_image_undo_enable (*image);
          gimp_image_clean_all (*image);
        }

      gimp_progress_update (1.0);
    }

  /* If this TIF was created in Alias/AutoDesk Sketchbook, it may have layers. */
  if (sketchbook_layers)
    load_sketchbook_layers (tif, *image);

  g_clear_object (&first_profile);

  if (pages.target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
    {
      GList *list = images_list;

      if (list)
        {
          *image = list->data;

          list = g_list_next (list);
        }

      for (; list; list = g_list_next (list))
        {
          gimp_display_new (list->data);
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
      if (! sketchbook_layers)
        gimp_image_resize (*image,
                           max_col - min_col, max_row - min_row,
                           -min_col, -min_row);

      gimp_image_undo_enable (*image);
    }

  /* Load Photoshop layer metadata */
  if (TIFFGetField (tif, TIFFTAG_PHOTOSHOP, &photoshop_len, &photoshop_data))
    {
      FILE           *fp;
      GFile          *temp_file   = NULL;
      GimpProcedure  *procedure;
      GimpValueArray *return_vals = NULL;

      temp_file = gimp_temp_file ("tmp");
      fp = g_fopen (g_file_peek_path (temp_file), "wb");

      if (! fp)
        {
          g_message (_("Error trying to open temporary %s file '%s' "
                       "for tiff metadata loading: %s"),
                     "tmp", gimp_file_get_utf8_name (temp_file),
                     g_strerror (errno));
        }

      fwrite (photoshop_data, sizeof (guchar), photoshop_len, fp);
      fclose (fp);

      procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (),
                                               "file-psd-load-metadata");
      return_vals = gimp_procedure_run (procedure,
                                        "run-mode",      GIMP_RUN_NONINTERACTIVE,
                                        "file",          temp_file,
                                        "size",          photoshop_len,
                                        "image",         *image,
                                        "metadata-type", FALSE,
                                        NULL);

      g_file_delete (temp_file, NULL, NULL);
      g_object_unref (temp_file);
      gimp_value_array_unref (return_vals);

      *ps_metadata_loaded = TRUE;
    }

  if (TIFFGetField (tif, TIFFTAG_IMAGESOURCEDATA, &photoshop_len, &photoshop_data))
    {
      FILE           *fp;
      GFile          *temp_file   = NULL;
      GimpProcedure  *procedure;
      GimpValueArray *return_vals = NULL;

      /* Photoshop metadata starts with 'Adobe Photoshop Document Data Block'
       * so we need to skip past that for the data. */
      photoshop_data += 36;
      photoshop_len  -= 36;

      temp_file = gimp_temp_file ("tmp");
      fp = g_fopen (g_file_peek_path (temp_file), "wb");

      if (! fp)
        {
          g_message (_("Error trying to open temporary %s file '%s' "
                       "for tiff metadata loading: %s"),
                     "tmp", gimp_file_get_utf8_name (temp_file),
                     g_strerror (errno));
        }

      fwrite (photoshop_data, sizeof (guchar), photoshop_len, fp);
      fclose (fp);

      procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (),
                                               "file-psd-load-metadata");
      /* We would like to use run_mode below. That way we could show a dialog
       * when unsupported Photoshop data is detected in interactive mode.
       * However, in interactive mode saved config values take precedence over
       * these values set below, so that won't work. */
      return_vals = gimp_procedure_run (procedure,
                                        "run-mode",      GIMP_RUN_NONINTERACTIVE,
                                        "file",          temp_file,
                                        "size",          photoshop_len,
                                        "image",         *image,
                                        "metadata-type", TRUE,
                                        "cmyk",          is_cmyk,
                                        NULL);

      g_file_delete (temp_file, NULL, NULL);
      g_object_unref (temp_file);
      gimp_value_array_unref (return_vals);

      *ps_metadata_loaded = TRUE;
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
  uint32_t  profile_size;
  guchar   *icc_profile;

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

  buffer = g_new (uint32_t, image_width * image_height);

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
        buffer[i] = GUINT32_FROM_LE (buffer[i]);
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

/* Loads layers stored by the Alias/AutoDesk Sketchbook program */
static void
load_sketchbook_layers (TIFF      *tif,
                        GimpImage *image)
{
  gchar          *alias_layer_info;
  gint            alias_data_len;
  guint32         image_height = gimp_image_get_height (image);
  guint32         image_width  = gimp_image_get_width (image);
  GeglColor      *fill_color;
  GeglColor      *foreground_color;
  GimpLayer      *background_layer;
  GimpLayerMode   default_mode;
  const Babl     *format = NULL;
  gchar         **image_settings;
  gchar          *hex_color;
  gint            layer_count = 0;
  gint            sub_len;
  void           *ptr;
  gchar          *endptr = NULL;

  default_mode = gimp_image_get_default_new_layer_mode (image);

  TIFFSetDirectory (tif, 0);

  TIFFGetField (tif, TIFFTAG_ALIAS_LAYER_METADATA, &alias_data_len,
                &alias_layer_info);
  if (! g_utf8_validate (alias_layer_info, -1, NULL))
    return;

  /* Create background layer. Fill it with the hex color from
   * the image-level ALIAS_LAYER_METADATA tag. The hex color
   * is in AGBR format so we need to reverse it */
  image_settings = g_strsplit (alias_layer_info, ", ", 15);

  if (image_settings[0] != NULL)
    layer_count = g_ascii_strtoll (image_settings[0], &endptr, 10);

  if (image_settings[2] != NULL && strlen (image_settings[2]) >= 8)
    hex_color =
      g_strdup_printf ("#%s%s%s%s", g_utf8_substring (image_settings[2], 6, 8),
                                    g_utf8_substring (image_settings[2], 4, 6),
                                    g_utf8_substring (image_settings[2], 2, 4),
                                    g_utf8_substring (image_settings[2], 0, 2));
  else
    hex_color = g_strdup ("transparent");

  fill_color = gegl_color_new (hex_color);
  g_free (hex_color);

  foreground_color = gegl_color_duplicate (gimp_context_get_foreground ());
  gimp_context_set_foreground (fill_color);

  background_layer = gimp_layer_new (image, _("Background"),
                                     image_width, image_height,
                                     GIMP_RGBA_IMAGE, 100, default_mode);

  gimp_image_insert_layer (image, background_layer, NULL, -1);
  gimp_drawable_fill (GIMP_DRAWABLE (background_layer), GIMP_FILL_FOREGROUND);

  g_object_unref (fill_color);
  gimp_context_set_foreground (foreground_color);
  g_object_unref (foreground_color);

  /* The layers are stored in BGRA format */
  format = babl_format_new (babl_model ("R~G~B~A"),
                                        babl_type ("u8"),
                                        babl_component ("B~"),
                                        babl_component ("G~"),
                                        babl_component ("R~"),
                                        babl_component ("A"),
                                        NULL);

  /* Layers are stored in SubIFDs of the first directory */
  if (TIFFGetField (tif, TIFFTAG_SUBIFD, &sub_len, &ptr))
    {
      toff_t offsets[sub_len];
      gint   count = 0;

      memcpy (offsets, ptr, sub_len * sizeof (offsets[0]));

      for (gint i = 0; i < sub_len; i++)
        {
          gchar  *alias_sublayer_info = NULL;
          gint32  alias_sublayer_len  = 0;

          if (! TIFFSetSubDirectory (tif, offsets[i]))
            break;

          if (TIFFGetField (tif, TIFFTAG_ALIAS_LAYER_METADATA, &alias_sublayer_len, &alias_sublayer_info) &&
              g_utf8_validate (alias_sublayer_info, -1, NULL))
            {
              gchar         **layer_settings;
              GimpProcedure  *procedure;
              GimpLayer      *layer;
              GeglBuffer     *buffer;
              const gchar    *layer_name;
              guint32         layer_width = 0;
              guint32         layer_height = 0;
              gfloat          x_pos   = 0;
              gfloat          y_pos   = 0;
              gfloat          opacity = 100;
              gboolean        visible = TRUE;
              gboolean        locked  = FALSE;
              guint32        *pixels;
              guint32         row;

              layer_settings = g_strsplit (alias_sublayer_info, ", ", 10);

              if (layer_settings[0] != NULL)
                {
                  opacity = (gfloat) g_ascii_strtod (layer_settings[0], &endptr);
                  opacity *= 100.0f;
                }
              if (layer_settings[2] != NULL)
                visible = g_ascii_strtoll (layer_settings[2], &endptr, 10);

              if (layer_settings[3] != NULL)
                locked  = g_ascii_strtoll (layer_settings[3], &endptr, 10);

              /* Additional tags in SubIFD */
              layer_name = tiff_get_page_name (tif);

              TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &layer_width);
              TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &layer_height);

              if (! TIFFGetField (tif, TIFFTAG_XPOSITION, &x_pos))
                x_pos = 0.0f;

              if (! TIFFGetField (tif, TIFFTAG_YPOSITION, &y_pos))
                y_pos = 0.0f;

              layer = gimp_layer_new (image, layer_name, layer_width,
                                      layer_height, GIMP_RGBA_IMAGE, opacity,
                                      default_mode);

              gimp_image_insert_layer (image, layer, NULL, -1);

              /* Loading pixel data */
              pixels = g_new (uint32_t, layer_width * layer_height);
              if (! TIFFReadRGBAImage (tif, layer_width, layer_height, pixels, 0))
                {
                  g_free (pixels);
                  continue;
                }

              buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

              for (row = 0; row < layer_height; row++)
                {
#if G_BYTE_ORDER != G_LITTLE_ENDIAN
                  guint32 row_start = row * layer_width;
                  guint32 row_end   = row_start + layer_width;
                  guint32 i;

                  /* Make sure our channels are in the right order */
                  for (i = row_start; i < row_end; i++)
                    pixels[i] = GUINT32_FROM_LE (pixels[i]);
#endif
                  gegl_buffer_set (buffer,
                                   GEGL_RECTANGLE (0, layer_height - row - 1,
                                                   layer_width, 1),
                                   0, format,
                                   ((guchar *) pixels) + row * layer_width * 4,
                                   GEGL_AUTO_ROWSTRIDE);
                }
              g_object_unref (buffer);
              g_free (pixels);

              /* The layers seem to have excessive padding that affects the
               * offset, since it's calculated from the bottom-left corner
               * of the layer. We can crop the layers to fix the y position
               * offset. Since the layer width can also shrink due to the
               * crop, we calculate the before and after difference and
               * adjust the x offset too. */
              procedure = gimp_pdb_lookup_procedure (gimp_get_pdb (),
                                                     "plug-in-autocrop-layer");
              gimp_procedure_run (procedure,
                                  "run-mode", GIMP_RUN_NONINTERACTIVE,
                                  "image",    image,
                                  "drawable", layer,
                                  NULL);

              x_pos += (layer_width - gimp_drawable_get_width (GIMP_DRAWABLE (layer)));
              y_pos = image_height - gimp_drawable_get_height (GIMP_DRAWABLE (layer)) - y_pos;

              gimp_layer_set_offsets (layer, ROUND (x_pos), ROUND (y_pos));
              /* In Alias/Autodesk Sketchbook, the layers are the same size as the canvas */
              gimp_layer_resize_to_image_size (layer);

              gimp_item_set_visible (GIMP_ITEM (layer), visible);
              /* Set locks after copying pixel data over */
              gimp_item_set_lock_content (GIMP_ITEM (layer), locked);
              gimp_layer_set_lock_alpha (layer, locked);

              count++;
              gimp_progress_update ((gdouble) count / (gdouble) layer_count);
            }
        }
    }
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
  gint64 x = width * height;

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

static void
convert_2bit2byte (const guchar *src,
                   guchar       *dest,
                   gint          width,
                   gint          height)
{
  gint64 x = width * height;

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

static void
convert_4bit2byte (const guchar *src,
                   guchar       *dest,
                   gint          width,
                   gint          height)
{
  gint64 x = width * height;

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
load_dialog (GimpProcedure       *procedure,
             GimpProcedureConfig *config,
             TiffSelectedPages   *pages,
             const gchar         *extra_message,
             DefaultExtra        *default_extra)
{
  GtkWidget  *dialog;
  GtkWidget  *vbox;
  GtkWidget  *toggle      = NULL;
  GtkWidget  *extra_radio = NULL;
  gboolean    run;

  pages->selector = NULL;

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Import from TIFF"));

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog), "tiff-vbox",
                                         "keep-empty-space", NULL);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "tiff-vbox", NULL);

  toggle = gtk_check_button_new_with_mnemonic (_("_Show reduced images"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                pages->show_reduced);
  gtk_widget_set_margin_bottom (toggle, 6);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_widget_set_visible (toggle, TRUE);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (tiff_dialog_show_reduced),
                    pages);

  if (pages->n_pages > 1)
    {
      /* Page Selector */
      pages->selector = gimp_page_selector_new ();
      gtk_widget_set_size_request (pages->selector, 300, 200);
      gtk_box_pack_start (GTK_BOX (vbox), pages->selector, TRUE, TRUE, 0);
      gtk_widget_set_visible (pages->selector, TRUE);

      gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (pages->selector),
                                      pages->n_filtered_pages);
      gimp_page_selector_set_target (GIMP_PAGE_SELECTOR (pages->selector),
                                     pages->target);

      /* Load a set number of pages, based on whether "Show Reduced Images"
       * is checked
       */
      tiff_dialog_show_reduced (toggle, pages);

      g_signal_connect_swapped (pages->selector, "activate",
                                G_CALLBACK (gtk_window_activate_default),
                                dialog);
    }

  if (extra_message)
    {
      GtkWidget *warning;

      warning = g_object_new (GIMP_TYPE_HINT_BOX,
                             "icon-name", GIMP_ICON_DIALOG_WARNING,
                             "hint",      extra_message,
                             NULL);
      gtk_box_pack_start (GTK_BOX (vbox), warning, TRUE, TRUE, 0);
      gtk_widget_set_visible (warning, TRUE);

      extra_radio = gimp_int_radio_group_new (TRUE, _("Process extra channel as:"),
                                              (GCallback) gimp_radio_button_update,
                                              default_extra, NULL, GIMP_TIFF_LOAD_UNASSALPHA,
                                              _("_Non-premultiplied alpha"), GIMP_TIFF_LOAD_UNASSALPHA, NULL,
                                              _("Pre_multiplied alpha"),     GIMP_TIFF_LOAD_ASSOCALPHA, NULL,
                                              _("Channe_l"),                 GIMP_TIFF_LOAD_CHANNEL,    NULL,
                                              NULL);
      gtk_box_pack_start (GTK_BOX (vbox), extra_radio, TRUE, TRUE, 0);
      gtk_widget_set_visible (extra_radio, TRUE);
    }

  toggle = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                             "keep-empty-space", G_TYPE_NONE);
  gtk_widget_set_margin_top (toggle, 6);
  gtk_widget_set_margin_bottom (toggle, 6);
  gtk_box_reorder_child (GTK_BOX (vbox), toggle, -1);

  /* Setup done; display the dialog */
  gtk_widget_set_visible (dialog, TRUE);

  /* run the dialog */
  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  if (run)
    {
      if (pages->n_pages > 1)
        {
          g_object_get (config,
                        "keep-empty-space", &pages->keep_empty_space,
                        NULL);

          pages->target =
            gimp_page_selector_get_target (GIMP_PAGE_SELECTOR (pages->selector));

          pages->pages =
            gimp_page_selector_get_selected_pages (GIMP_PAGE_SELECTOR (pages->selector),
                                                   &pages->n_pages);

          /* select all if none selected */
          if (pages->n_pages == 0)
            {
              gimp_page_selector_select_all (GIMP_PAGE_SELECTOR (pages->selector));

              pages->pages =
                gimp_page_selector_get_selected_pages (GIMP_PAGE_SELECTOR (pages->selector),
                                                       &pages->n_pages);
            }
        }
    }

  return run;
}

static void
tiff_dialog_show_reduced (GtkWidget *toggle,
                          gpointer   data)
{
  gint selectable_pages;
  gint i, j;

  TiffSelectedPages *pages = (TiffSelectedPages *) data;
  pages->show_reduced = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle));

  /* Clear current pages from selection */
  gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (pages->selector), 0);
  /* Jump back to start of the TIFF file */
  TIFFSetDirectory (pages->tif, 0);

  selectable_pages = pages->n_filtered_pages;
  if (pages->show_reduced)
    selectable_pages = pages->n_reducedimage_pages;

  gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (pages->selector),
                                  selectable_pages);

  for (i = 0, j = 0; i < pages->n_pages && j < selectable_pages; i++)
    {
      if ((pages->show_reduced && pages->filtered_pages[i] != TIFF_MISC_THUMBNAIL) ||
          (! pages->show_reduced && pages->filtered_pages[i] > TIFF_MISC_THUMBNAIL))
        {
          const gchar *name = tiff_get_page_name (pages->tif);

          if (name)
            gimp_page_selector_set_page_label (GIMP_PAGE_SELECTOR (pages->selector),
                                               j, name);
          j++;
        }

      TIFFReadDirectory (pages->tif);
    }
}
