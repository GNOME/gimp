/* tiff exporting for GIMP
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
#include <gexiv2/gexiv2.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "file-tiff.h"
#include "file-tiff-io.h"
#include "file-tiff-export.h"

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_ROLE "gimp-file-tiff-export"


static void         update_format_options    (GtkWidget    *dialog,
                                              GObject      *config,
                                              gboolean      transparent_pixels_available,
                                              gboolean      save_layers_available,
                                              gboolean      leaving_photoshop_mode,
                                              gboolean      entering_photoshop_mode);

static void         evaluate_format_options  (GObject      *config,
                                              GParamSpec   *pspec,
                                              gpointer      user_data);

static void         byte2bit                 (const guchar *byteline,
                                              gint          width,
                                              guchar       *bitline,
                                              gboolean      invert);

static const char * get_layer_key            (gshort        bitspersample);

static const Babl * get_extra_format         (gshort        bitspersample,
                                              gshort        sampleformat);

static gboolean     should_export_layer_info (GimpImage    *orig_image);


/*
 * pnmtotiff.c - converts a portable anymap to a Tagged Image File
 *
 * Derived by Jef Poskanzer from ras2tif.c, which is:
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 */

static gboolean
save_layer (TIFF        *tif,
            GObject     *config,
            const Babl  *space,
            GimpImage   *image,
            GimpLayer   *layer,
            gint32       page,
            gint32       num_pages,
            GimpImage   *orig_image, /* the export function might
                                      * have created a duplicate  */
            gint         origin_x,
            gint         origin_y,
            gint        *saved_bpp,
            gboolean     out_linear,
            GError     **error)
{
  gboolean          status = FALSE;
  gushort           red[256];
  gushort           grn[256];
  gushort           blu[256];
  gint              cols, col, rows, row, i;
  glong             rowsperstrip;
  gushort           compression;
  gushort          *extra_samples = NULL;
  gboolean          alpha;
  gshort            predictor;
  gshort            photometric;
  const Babl       *format;
  const Babl       *mask_format;
  const Babl       *type;
  gshort            samplesperpixel;
  gshort            bitspersample;
  gshort            sampleformat;
  guchar           *src = NULL;
  guchar           *data = NULL;
  GimpPalette      *palette;
  guchar           *cmap;
  gint              num_colors;
  gint              success;
  GimpImageType     drawable_type;
  GeglBuffer       *buffer = NULL;
  gint              tile_height;
  gint              y, yend;
  gboolean          is_bw    = FALSE;
  gboolean          invert   = TRUE;
  const guchar      bw_map[] = { 0, 0, 0, 255, 255, 255 };
  const guchar      wb_map[] = { 255, 255, 255, 0, 0, 0 };
  gchar            *layer_name = NULL;
  const gdouble     progress_base = (gdouble) page / (gdouble) num_pages;
  const gdouble     progress_fraction = 1.0 / (gdouble) num_pages;
  gdouble           xresolution;
  gdouble           yresolution;
  gushort           save_unit = RESUNIT_INCH;
  gint              offset_x, offset_y;
  gint              config_compression;
  gchar            *config_comment;
  gboolean          config_save_comment;
  gboolean          config_save_transp_pixels;
  gboolean          config_save_geotiff_tags;
  gboolean          config_save_resources;
  gboolean          config_save_profile;
  gboolean          config_cmyk;
  GimpFormat        config_format;
  GList            *channels;
  gushort           extra;
  GeglBuffer      **channel_buffers = NULL;
  int               base_channels;
  int               bytes_per_sample;
  int               base_bytesperrow;
  int               final_bytesperrow;

  g_object_get (config,
                "gimp-comment",            &config_comment,
                "include-comment",         &config_save_comment,
                "save-transparent-pixels", &config_save_transp_pixels,
                "save-geotiff",            &config_save_geotiff_tags,
                "save-resources",          &config_save_resources,
                "include-color-profile",   &config_save_profile,
                "cmyk",                    &config_cmyk,
                NULL);

  config_compression = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "compression");
  compression = gimp_compression_to_tiff_compression (config_compression);
  config_format = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "format");

  layer_name = gimp_item_get_name (GIMP_ITEM (layer));

  /* Disabled because this isn't in older releases of libtiff, and it
   * wasn't helping much anyway
   */
#if 0
  if (TIFFFindCODEC((uint16) compression) == NULL)
    compression = COMPRESSION_NONE; /* CODEC not available */
#endif

  predictor = 0;
  tile_height = gimp_tile_height ();
  rowsperstrip = tile_height;

  drawable_type = gimp_drawable_type (GIMP_DRAWABLE (layer));
  buffer        = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  format = gegl_buffer_get_format (buffer);
  type   = babl_format_get_type (format, 0);

  switch (gimp_image_get_precision (image))
    {
    case GIMP_PRECISION_U8_LINEAR:
    case GIMP_PRECISION_U8_NON_LINEAR:
    case GIMP_PRECISION_U8_PERCEPTUAL:
      /* Promote to 16-bit if storage and export TRC don't match. */
      if ((gimp_image_get_precision (image) == GIMP_PRECISION_U8_LINEAR && out_linear) ||
          (gimp_image_get_precision (image) != GIMP_PRECISION_U8_LINEAR && ! out_linear))
        {
          bitspersample = 8;
          sampleformat  = SAMPLEFORMAT_UINT;
        }
      else
        {
          bitspersample = 16;
          sampleformat  = SAMPLEFORMAT_UINT;
          type          = babl_type ("u16");
        }
      break;

    case GIMP_PRECISION_U16_LINEAR:
    case GIMP_PRECISION_U16_NON_LINEAR:
    case GIMP_PRECISION_U16_PERCEPTUAL:
      bitspersample = 16;
      sampleformat  = SAMPLEFORMAT_UINT;
      break;

    case GIMP_PRECISION_U32_LINEAR:
    case GIMP_PRECISION_U32_NON_LINEAR:
    case GIMP_PRECISION_U32_PERCEPTUAL:
      bitspersample = 32;
      sampleformat  = SAMPLEFORMAT_UINT;
      break;

    case GIMP_PRECISION_HALF_LINEAR:
    case GIMP_PRECISION_HALF_NON_LINEAR:
    case GIMP_PRECISION_HALF_PERCEPTUAL:
      bitspersample = 16;
      sampleformat  = SAMPLEFORMAT_IEEEFP;
      break;

    default:
    case GIMP_PRECISION_FLOAT_LINEAR:
    case GIMP_PRECISION_FLOAT_NON_LINEAR:
    case GIMP_PRECISION_FLOAT_PERCEPTUAL:
      bitspersample = 32;
      sampleformat  = SAMPLEFORMAT_IEEEFP;
      break;

    case GIMP_PRECISION_DOUBLE_LINEAR:
    case GIMP_PRECISION_DOUBLE_NON_LINEAR:
    case GIMP_PRECISION_DOUBLE_PERCEPTUAL:
      bitspersample = 64;
      sampleformat  = SAMPLEFORMAT_IEEEFP;
      break;
    }

  *saved_bpp = bitspersample;

  cols = gegl_buffer_get_width (buffer);
  rows = gegl_buffer_get_height (buffer);

  channels = gimp_image_list_channels (orig_image);
  extra    = g_list_length (channels);

  if (extra > 0)
    {
      channel_buffers = g_new (GeglBuffer *, extra);
      for (gint j = 0; j < extra; j++)
        {
          GimpDrawable *temp_channel = g_list_nth_data (channels, j);

          channel_buffers[j] = gimp_drawable_get_buffer (temp_channel);
        }
    }

  g_list_free (channels);

  switch (drawable_type)
    {
    case GIMP_RGB_IMAGE:
      predictor       = 2;
      samplesperpixel = 3 + extra;
      photometric     = PHOTOMETRIC_RGB;
      alpha           = FALSE;
      if (out_linear)
        {
          format = babl_format_new (babl_model ("RGB"),
                                    type,
                                    babl_component ("R"),
                                    babl_component ("G"),
                                    babl_component ("B"),
                                    NULL);
        }
      else
        {
          format = babl_format_new (babl_model ("R'G'B'"),
                                    type,
                                    babl_component ("R'"),
                                    babl_component ("G'"),
                                    babl_component ("B'"),
                                    NULL);
        }
      break;

    case GIMP_GRAY_IMAGE:
      samplesperpixel = 1 + extra;
      photometric     = PHOTOMETRIC_MINISBLACK;
      alpha           = FALSE;
      if (out_linear)
        {
          format = babl_format_new (babl_model ("Y"),
                                    type,
                                    babl_component ("Y"),
                                    NULL);
        }
      else
        {
          format = babl_format_new (babl_model ("Y'"),
                                    type,
                                    babl_component ("Y'"),
                                    NULL);
        }
      break;

    case GIMP_RGBA_IMAGE:
      predictor       = 2;
      samplesperpixel = 3 + 1 + extra;
      photometric     = PHOTOMETRIC_RGB;
      alpha           = TRUE;
      if (config_save_transp_pixels)
        {
          if (out_linear)
            {
              format = babl_format_new (babl_model ("RGBA"),
                                        type,
                                        babl_component ("R"),
                                        babl_component ("G"),
                                        babl_component ("B"),
                                        babl_component ("A"),
                                        NULL);
            }
          else
            {
              format = babl_format_new (babl_model ("R'G'B'A"),
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
          if (out_linear)
            {
              format = babl_format_new (babl_model ("RaGaBaA"),
                                        type,
                                        babl_component ("Ra"),
                                        babl_component ("Ga"),
                                        babl_component ("Ba"),
                                        babl_component ("A"),
                                        NULL);
            }
          else
            {
              format = babl_format_new (babl_model ("R'aG'aB'aA"),
                                        type,
                                        babl_component ("R'a"),
                                        babl_component ("G'a"),
                                        babl_component ("B'a"),
                                        babl_component ("A"),
                                        NULL);
            }
        }
      break;

    case GIMP_GRAYA_IMAGE:
      samplesperpixel = 1 + 1 + extra;
      photometric     = PHOTOMETRIC_MINISBLACK;
      alpha           = TRUE;
      if (config_save_transp_pixels)
        {
          if (out_linear)
            {
              format = babl_format_new (babl_model ("YA"),
                                        type,
                                        babl_component ("Y"),
                                        babl_component ("A"),
                                        NULL);
            }
          else
            {
              format = babl_format_new (babl_model ("Y'A"),
                                        type,
                                        babl_component ("Y'"),
                                        babl_component ("A"),
                                        NULL);
            }
        }
      else
        {
          if (out_linear)
            {
              format = babl_format_new (babl_model ("YaA"),
                                        type,
                                        babl_component ("Ya"),
                                        babl_component ("A"),
                                        NULL);
            }
          else
            {
              format = babl_format_new (babl_model ("Y'aA"),
                                        type,
                                        babl_component ("Y'a"),
                                        babl_component ("A"),
                                        NULL);
            }
        }
      break;

    case GIMP_INDEXED_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      palette = gimp_image_get_palette (image);
      format  = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
      cmap    = gimp_palette_get_colormap (palette, babl_format_with_space ("R'G'B' u8", format), &num_colors, NULL);

      if (drawable_type == GIMP_INDEXED_IMAGE && (num_colors == 2 || num_colors == 1))
        {
          is_bw = (memcmp (cmap, bw_map, 3 * num_colors) == 0);
          photometric = PHOTOMETRIC_MINISWHITE;

          if (!is_bw)
            {
              is_bw = (memcmp (cmap, wb_map, 3 * num_colors) == 0);

              if (is_bw)
                invert = FALSE;
            }
       }

      if (is_bw)
        {
          bitspersample = 1;
        }
      else
        {
          bitspersample = 8;
          photometric   = PHOTOMETRIC_PALETTE;

          for (i = 0; i < num_colors; i++)
            {
              red[i] = cmap[i * 3 + 0] * 65535 / 255;
              grn[i] = cmap[i * 3 + 1] * 65535 / 255;
              blu[i] = cmap[i * 3 + 2] * 65535 / 255;
            }
        }

      samplesperpixel = 1 + (drawable_type == GIMP_INDEXEDA_IMAGE ? 1 : 0) + extra;
      alpha           = (drawable_type == GIMP_INDEXEDA_IMAGE);

      g_free (cmap);
      break;

    default:
      goto out;
    }

  if (config_cmyk)
    {
      if (alpha)
        format = babl_format_new (babl_model ("CMYKA"),
                                  type,
                                  babl_component ("Cyan"),
                                  babl_component ("Magenta"),
                                  babl_component ("Yellow"),
                                  babl_component ("Key"),
                                  babl_component ("A"),
                                  NULL);
      else
        format = babl_format_new (babl_model ("CMYK"),
                                  type,
                                  babl_component ("Cyan"),
                                  babl_component ("Magenta"),
                                  babl_component ("Yellow"),
                                  babl_component ("Key"),
                                  NULL);

      format =
        babl_format_with_space (babl_format_get_encoding (format),
                                space);
    }
  else
    {
      format = babl_format_with_space (babl_format_get_encoding (format),
                                       space ? space : gegl_buffer_get_format (buffer));
    }

  if (compression == COMPRESSION_CCITTFAX3 ||
      compression == COMPRESSION_CCITTFAX4)
    {
      if (bitspersample != 1 || samplesperpixel != 1)
        {
          const gchar *msg = _("Only monochrome pictures can be compressed "
                               "with \"CCITT Group 4\" or \"CCITT Group 3\".");

          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, msg);

          goto out;
        }
    }

  if (compression == COMPRESSION_JPEG)
    {
      if (gimp_image_get_base_type (image) == GIMP_INDEXED)
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Indexed pictures cannot be compressed "
                                 "with \"JPEG\"."));
          goto out;
        }
    }

#ifdef TIFFTAG_ICCPROFILE
  if (config_save_profile || config_cmyk)
    {
      const guint8     *icc_data     = NULL;
      gsize             icc_length;
      GimpColorProfile *profile;
      GimpColorProfile *cmyk_profile = NULL;

      profile = gimp_image_get_effective_color_profile (orig_image);
      if (config_cmyk)
        cmyk_profile = gimp_image_get_simulation_profile (image);

      /* If a non-CMYK profile was assigned as the simulation profile,
       * set it back to NULL and save the RGB profile instead
       */
      if (cmyk_profile && ! gimp_color_profile_is_cmyk (cmyk_profile))
        g_clear_object (&cmyk_profile);

      /* Write the RGB or CMYK color profile to the TIFF file */
      if (profile && ! config_cmyk)
        icc_data = gimp_color_profile_get_icc_profile (profile, &icc_length);
      else if (cmyk_profile)
        icc_data = gimp_color_profile_get_icc_profile (cmyk_profile, &icc_length);

      if (icc_data)
        TIFFSetField (tif, TIFFTAG_ICCPROFILE, icc_length, icc_data);

      g_object_unref (profile);
      g_clear_object (&cmyk_profile);
    }
#endif

  /* Set CMYK Properties */
  if (config_cmyk)
    {
      photometric = PHOTOMETRIC_SEPARATED;
      /* If there's transparency, save as CMYKA format */
      samplesperpixel = 4 + alpha + extra;
      TIFFSetField (tif, TIFFTAG_INKSET, INKSET_CMYK);
      TIFFSetField (tif, TIFFTAG_NUMBEROFINKS, 4);
    }

  /* Set TIFF parameters. */
  if (config_save_comment && config_comment && *config_comment)
    {
      const gchar *c = config_comment;
      gint         len;

      /* The TIFF spec explicitly says ASCII for the image description. */
      for (len = strlen (c); len; c++, len--)
        {
          if ((guchar) *c > 127)
            {
              g_message (_("The TIFF format only supports comments in\n"
                           "7bit ASCII encoding. No comment is saved."));
              g_free (config_comment);
              config_comment = NULL;

              break;
            }
        }

      if (config_comment)
        TIFFSetField (tif, TIFFTAG_IMAGEDESCRIPTION, config_comment);
    }

  if (num_pages > 1)
    {
      TIFFSetField (tif, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
      TIFFSetField (tif, TIFFTAG_PAGENUMBER, page, num_pages);
    }
  TIFFSetField (tif, TIFFTAG_PAGENAME, layer_name);
  TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, cols);
  TIFFSetField (tif, TIFFTAG_IMAGELENGTH, rows);
  TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, photometric);
  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, bitspersample);
  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, sampleformat);
  TIFFSetField (tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField (tif, TIFFTAG_COMPRESSION, compression);

  if ((compression == COMPRESSION_LZW ||
       compression == COMPRESSION_ADOBE_DEFLATE) &&
      (predictor != 0))
    {
      TIFFSetField (tif, TIFFTAG_PREDICTOR, predictor);
    }

  if (alpha || extra > 0)
    {
      extra_samples = g_new (gushort, alpha + extra);
      if (alpha)
        {
          if (config_save_transp_pixels ||
              /* Associated alpha, hence premultiplied components is
               * meaningless for palette images with transparency in TIFF
               * format, since alpha is set per pixel, not per color (so a
               * given color could be set to different alpha on different
               * pixels, hence it cannot be premultiplied).
               */
              drawable_type == GIMP_INDEXEDA_IMAGE)
            extra_samples [0] = EXTRASAMPLE_UNASSALPHA;
          else
            extra_samples [0] = EXTRASAMPLE_ASSOCALPHA;
        }

      for (gushort j = 0; j < extra; j++)
        extra_samples[alpha + j] = EXTRASAMPLE_UNSPECIFIED;

      TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, alpha + extra, extra_samples);
    }

  /* resolution fields */
  gimp_image_get_resolution (orig_image, &xresolution, &yresolution);

  if (gimp_unit_is_metric (gimp_image_get_unit (orig_image)))
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

  gimp_drawable_get_offsets (GIMP_DRAWABLE (layer), &offset_x, &offset_y);

  offset_x -= origin_x;
  offset_y -= origin_y;

  if (offset_x || offset_y)
    {
      TIFFSetField (tif, TIFFTAG_XPOSITION, offset_x / xresolution);
      TIFFSetField (tif, TIFFTAG_YPOSITION, offset_y / yresolution);
    }

  if (! is_bw && ! config_cmyk &&
      (drawable_type == GIMP_INDEXED_IMAGE || drawable_type == GIMP_INDEXEDA_IMAGE))
    TIFFSetField (tif, TIFFTAG_COLORMAP, red, grn, blu);

  /* Save Photoshop image resources (paths, channel names, ...) to TIFFTAG_PHOTOSHOP */
  if (config_save_resources &&
      ((config_format == GIMP_TIFF_FORMAT_STANDARD_TIFF && page == 0) ||
        config_format == GIMP_TIFF_FORMAT_PHOTOSHOP_TIFF))
    {
      GFile             *temp_file     = NULL;
      GimpProcedure     *procedure;
      GimpValueArray    *return_vals   = NULL;
      GimpPDBStatusType  export_status;
      guchar            *data_buffer   = NULL;
      gsize              data_length   = 0;

      temp_file   = gimp_temp_file ("tmp");
      procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (),
                                               "file-psd-export-metadata");
      return_vals = gimp_procedure_run (procedure,
                                        "file",          temp_file,
                                        "image",         orig_image,
                                        "metadata-type", FALSE,
                                        "cmyk",          config_cmyk,
                                        NULL);

      export_status = GIMP_VALUES_GET_ENUM (return_vals, 0);
      if (export_status == GIMP_PDB_SUCCESS)
        {
          if (g_file_get_contents (g_file_peek_path (temp_file),
                                   (gchar **) &data_buffer,
                                   &data_length, NULL) && data_length > 0)
            {
              TIFFSetField (tif, TIFFTAG_PHOTOSHOP, (uint32_t) data_length,
                            data_buffer);
            }
        }

      g_free (data_buffer);
      g_file_delete (temp_file, NULL, NULL);
      g_object_unref (temp_file);
      gimp_value_array_unref (return_vals);
    }

  /* Save Photoshop layer data to TIFFTAG_IMAGESOURCEDATA */
  if (config_format == GIMP_TIFF_FORMAT_PHOTOSHOP_TIFF &&
      should_export_layer_info (orig_image))
    {
      GFile             *temp_file     = NULL;
      GimpProcedure     *procedure;
      GimpValueArray    *return_vals   = NULL;
      GimpPDBStatusType  export_status;
      guchar            *data_buffer   = NULL;
      gsize              data_length   = 0;
      guchar            *tag_buffer    = NULL;
      gsize              tag_length    = 0;

      temp_file   = gimp_temp_file ("tmp");
      procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (),
                                               "file-psd-export-metadata");
      return_vals = gimp_procedure_run (procedure,
                                        "file",          temp_file,
                                        "image",         orig_image,
                                        "metadata-type", TRUE,
                                        "cmyk",          config_cmyk,
                                        NULL);

      export_status = GIMP_VALUES_GET_ENUM (return_vals, 0);
      if (export_status == GIMP_PDB_SUCCESS)
        {
          if (g_file_get_contents (g_file_peek_path (temp_file),
                                   (gchar **) &data_buffer,
                                   &data_length, NULL) && data_length > 0)
            {
              const gchar  hdr[]   = "Adobe Photoshop Document Data Block";
              const gchar  sig[]   = "8BIM";
              const gsize  hdr_len = sizeof (hdr);
              const gsize  pad     = (4 - data_length % 4) % 4;
              const gchar *key     = get_layer_key (bitspersample);

              tag_length = hdr_len + sizeof (sig) + sizeof (key) +
                           data_length + pad;
              tag_buffer = g_malloc0 (tag_length);

              memcpy (tag_buffer, hdr, hdr_len);
              memcpy (tag_buffer + hdr_len, sig, 4);
              memcpy (tag_buffer + hdr_len + 4, key, 4);
              memcpy (tag_buffer + hdr_len + 8, data_buffer, data_length);

              TIFFSetField (tif, TIFFTAG_IMAGESOURCEDATA, (uint32_t) tag_length,
                            tag_buffer);
            }
        }

      g_free (tag_buffer);
      g_free (data_buffer);
      g_file_delete (temp_file, NULL, NULL);
      g_object_unref (temp_file);
      gimp_value_array_unref (return_vals);
    }

  /* array to rearrange data */
  base_channels    = babl_format_get_n_components (format);
  bytes_per_sample = bitspersample / 8;

  base_bytesperrow  = cols * base_channels  * bytes_per_sample;
  final_bytesperrow = cols * samplesperpixel * bytes_per_sample;

  src  = g_new (guchar, base_bytesperrow * tile_height);
  data = g_new (guchar, final_bytesperrow);

  /* Now write the TIFF data. */
  for (y = 0; y < rows; y = yend)
    {
      yend = y + tile_height;
      yend = MIN (yend, rows);

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, y, cols, yend - y), 1.0,
                       format, src,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (row = y; row < yend; row++)
        {
          guchar *src_row_base = src + base_bytesperrow * (row - y);

          switch (drawable_type)
            {
            case GIMP_INDEXED_IMAGE:
            case GIMP_INDEXEDA_IMAGE:
              if (is_bw)
                {
                  byte2bit (src_row_base, base_bytesperrow, data, invert);
                  success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
                }
              else
                {
                  success = (TIFFWriteScanline (tif, src_row_base, row, 0) >= 0);
                }
              break;

            case GIMP_GRAY_IMAGE:
            case GIMP_GRAYA_IMAGE:
            case GIMP_RGB_IMAGE:
            case GIMP_RGBA_IMAGE:
              {
                mask_format = get_extra_format (bitspersample, sampleformat);

                for (col = 0; col < cols; col++)
                  {
                    gint src_off = col * base_channels * bytes_per_sample;
                    gint dst_off = col * samplesperpixel * bytes_per_sample;

                    /* Copy base channels (e.g. RGB or RGBA or GRAY) */
                    memcpy (data + dst_off,
                            src_row_base + src_off,
                            base_channels * bytes_per_sample);

                    /* Append extra channels */
                    for (gint c = 0; channel_buffers && c < extra; c++)
                      {
                        guint8 sample[8];

                        gegl_buffer_get (channel_buffers[c],
                                         GEGL_RECTANGLE (col, row, 1, 1),
                                         1.0,
                                         mask_format,
                                         sample,
                                         GEGL_AUTO_ROWSTRIDE,
                                         GEGL_ABYSS_NONE);

                        memcpy (data + dst_off + (base_channels + c) * bytes_per_sample,
                                sample, bytes_per_sample);
                      }
                  }

                success = TIFFWriteScanline (tif, data, row, 0) >= 0;
              }
              break;

            default:
              success = FALSE;
              break;
            }

          if (! success)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Failed a scanline write on row %d"), row);
              goto out;
            }
        }

      if ((row % 32) == 0)
        gimp_progress_update (progress_base + progress_fraction
                              * (gdouble) row / (gdouble) rows);
    }

  /* Save GeoTIFF tags to file, if available */
  if (config_save_geotiff_tags)
    {
      GimpParasite *parasite = NULL;
      gchar        *parasite_data;
      guint32       parasite_size;

      parasite = gimp_image_get_parasite (image,"Gimp_GeoTIFF_ModelPixelScale");
      if (parasite)
        {
          parasite_data = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
          TIFFSetField (tif,
                        GEOTIFF_MODELPIXELSCALE,
                        (parasite_size / TIFFDataWidth (TIFF_DOUBLE)),
                        parasite_data);
          gimp_parasite_free (parasite);
        }

      parasite = gimp_image_get_parasite (image,"Gimp_GeoTIFF_ModelTiePoint");
      if (parasite)
        {
          parasite_data = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
          TIFFSetField (tif,
                        GEOTIFF_MODELTIEPOINT,
                        (parasite_size / TIFFDataWidth (TIFF_DOUBLE)),
                        parasite_data);
          gimp_parasite_free (parasite);
        }

      parasite = gimp_image_get_parasite (image,"Gimp_GeoTIFF_ModelTransformation");
      if (parasite)
        {
          parasite_data = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
          TIFFSetField (tif,
                        GEOTIFF_MODELTRANSFORMATION,
                        (parasite_size / TIFFDataWidth (TIFF_DOUBLE)),
                        parasite_data);
          gimp_parasite_free (parasite);
        }

      parasite = gimp_image_get_parasite (image,"Gimp_GeoTIFF_KeyDirectory");
      if (parasite)
        {
          parasite_data = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
          TIFFSetField (tif,
                        GEOTIFF_KEYDIRECTORY,
                        (parasite_size / TIFFDataWidth (TIFF_SHORT)),
                        parasite_data);
          gimp_parasite_free (parasite);
        }

      parasite = gimp_image_get_parasite (image,"Gimp_GeoTIFF_DoubleParams");
      if (parasite)
        {
          parasite_data = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
          TIFFSetField (tif,
                        GEOTIFF_DOUBLEPARAMS,
                        (parasite_size / TIFFDataWidth (TIFF_DOUBLE)),
                        parasite_data);
          gimp_parasite_free (parasite);
        }

      parasite = gimp_image_get_parasite (image,"Gimp_GeoTIFF_Asciiparams");
      if (parasite)
        {
          parasite_data = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
          parasite_data = g_strndup (parasite_data, parasite_size);
          TIFFSetField (tif,
                        GEOTIFF_ASCIIPARAMS,
                        parasite_data);
          gimp_parasite_free (parasite);
          g_free (parasite_data);
        }
    }

  TIFFWriteDirectory (tif);

  gimp_progress_update (progress_base + progress_fraction);

  status = TRUE;

out:
  if (buffer)
    g_object_unref (buffer);

  if (channel_buffers != NULL)
    {
      for (gint j = 0; j < extra; j++)
        g_object_unref (channel_buffers[j]);

      g_free (channel_buffers);
    }

  if (extra_samples)
    g_free (extra_samples);

  g_free (data);
  g_free (src);
  g_free (layer_name);

  return status;
}

/* FIXME Most of the stuff in save_metadata except the
 * thumbnail saving should probably be moved to
 * gimpmetadata.c and gimpmetadata-save.c.
 */
static void
save_metadata (GFile        *file,
               GObject      *config,
               GimpImage    *image,
               GimpMetadata *metadata,
               gint          saved_bpp,
               gboolean      cmyk)
{
  gchar    **exif_tags;

  /* See bug 758909: clear TIFFTAG_MIN/MAXSAMPLEVALUE because
   * exiv2 saves them with wrong type and the original values
   * could be invalid, see also bug 761823.
   * we also clear some other tags that were only meaningful
   * for the original imported image.
   */
  static const gchar *exif_tags_to_remove[] =
  {
    "Exif.Image.0x0118",  /* MinSampleValue */
    "Exif.Image.0x0119",  /* MaxSampleValue */
    "Exif.Image.0x011d",  /* PageName */
    "Exif.Image.Compression",
    "Exif.Image.FillOrder",
    "Exif.Image.InterColorProfile",
    "Exif.Image.NewSubfileType",
    "Exif.Image.PageNumber",
    "Exif.Image.PhotometricInterpretation",
    "Exif.Image.PlanarConfiguration",
    "Exif.Image.Predictor",
    "Exif.Image.RowsPerStrip",
    "Exif.Image.SampleFormat",
    "Exif.Image.SamplesPerPixel",
    "Exif.Image.StripByteCounts",
    "Exif.Image.StripOffsets"
  };
  static const guint n_keys = G_N_ELEMENTS (exif_tags_to_remove);

  for (int k = 0; k < n_keys; k++)
    {
      gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (metadata),
                                     exif_tags_to_remove[k], NULL);
    }

  /* get rid of all the EXIF tags for anything but the first sub image. */
  exif_tags = gexiv2_metadata_get_exif_tags (GEXIV2_METADATA (metadata));
  for (char **tag = exif_tags; *tag; tag++)
    {
      /* Keeping Exif.Image2, 3 can cause exiv2 to save faulty extra TIFF pages
       * that are empty except for the Exif metadata. See issue #7195. */
      if (g_str_has_prefix (*tag, "Exif.Image")
          && (*tag)[strlen ("Exif.Image")] >= '0'
          && (*tag)[strlen ("Exif.Image")] <= '9')
        gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (metadata), *tag, NULL);
      if (g_str_has_prefix (*tag, "Exif.SubImage")
          && (*tag)[strlen ("Exif.SubImage")] >= '0'
          && (*tag)[strlen ("Exif.SubImage")] <= '9')
        gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (metadata), *tag, NULL);
      if (g_str_has_prefix (*tag, "Exif.Thumbnail"))
        gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (metadata), *tag, NULL);
    }

  gimp_metadata_set_bits_per_sample (metadata, saved_bpp);
  if (cmyk)
    gimp_metadata_set_colorspace (metadata, GIMP_METADATA_COLORSPACE_UNCALIBRATED);

  gimp_procedure_config_save_metadata (GIMP_PROCEDURE_CONFIG (config),
                                       image, file);
}

gboolean
export_image (GFile         *file,
              GimpImage     *image,
              GimpImage     *orig_image, /* the export function might
                                          * have created a duplicate */
              GObject       *config,
              GimpMetadata  *metadata,
              GError       **error)
{
  TIFF       *tif           = NULL;
  const Babl *space         = NULL;
  gboolean    status        = FALSE;
  gboolean    out_linear    = FALSE;
  guint32     num_layers;
  gint32      current_layer = 0;
  GList      *layers;
  GList      *iter;
  gint        origin_x      = 0;
  gint        origin_y      = 0;
  gint        saved_bpp;
  gboolean    bigtiff;
  gboolean    config_save_profile;
  gboolean    config_save_thumbnail;
  gboolean    config_cmyk;
  GimpFormat  config_format;

  g_object_get (config,
                "bigtiff",               &bigtiff,
                "include-color-profile", &config_save_profile,
                "include-thumbnail",     &config_save_thumbnail,
                "cmyk",                  &config_cmyk,
                NULL);

  config_format = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG(config), "format");

  layers = gimp_image_list_layers (image);
  layers = g_list_reverse (layers);
  num_layers = g_list_length (layers);

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  /* Open file and write some global data */
  tif = tiff_open (file, (bigtiff ? "wb8" : "wb"), error);

  if (! tif)
    {
      if (! error)
        g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                     _("Could not open '%s' for writing: %s"),
                     gimp_file_get_utf8_name (file), g_strerror (errno));
      goto out;
    }

  if (config_save_profile || config_cmyk)
    {
      GimpColorProfile *profile;
      GError           *error = NULL;

      if (config_cmyk)
        {
          profile = gimp_image_get_simulation_profile (image);

          if (profile && ! gimp_color_profile_is_cmyk (profile))
            g_clear_object (&profile);
        }
      else
        {
          profile = gimp_image_get_effective_color_profile (orig_image);
        }

      /* Curve of the exported data depends on the saved profile, i.e.
       * any explicitly-set profile in priority, or the default one for
       * the storage format as fallback.
       */
      out_linear = (profile != NULL && gimp_color_profile_is_linear (profile));

      if (profile)
        {
          space = gimp_color_profile_get_space (profile,
                                                config_cmyk ?
                                                gimp_image_get_simulation_intent (image) :
                                                GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                &error);
          g_object_unref (profile);
        }

      if (error)
        {
          g_printerr ("%s: error getting the profile space: %s",
                      G_STRFUNC, error->message);
          g_error_free (error);
          space = NULL;
        }
    }

  /* calculate the top-left coordinates */
  for (iter = layers; iter; iter = g_list_next (iter))
    {
      GimpDrawable *drawable = iter->data;
      gint          offset_x, offset_y;

      gimp_drawable_get_offsets (drawable, &offset_x, &offset_y);

      origin_x = MIN (origin_x, offset_x);
      origin_y = MIN (origin_y, offset_y);
    }

  /* === Photoshop-TIFF MODE === */
  if (config_format == GIMP_TIFF_FORMAT_PHOTOSHOP_TIFF)
    {
      /* flatten for composite image (merges layer & removes alpha) */
      gimp_image_flatten (image);

      g_list_free (layers);

      layers     = gimp_image_list_layers (image);
      num_layers = g_list_length (layers);

      /* write composite image */
      if (! save_layer (tif, config, space, image,
                        g_list_nth_data (layers, 0),
                        0, num_layers,
                        orig_image,
                        origin_x, origin_y,
                        &saved_bpp, out_linear, error))
        goto out;

      TIFFFlushData (tif);
      TIFFClose (tif);
      tif = NULL;

      status = TRUE;
      goto out;
    }

  /* === Standard TIFF (non-Photoshop) === */

  /* write last layer as first page. */
  if (! save_layer (tif,  config, space, image,
                    g_list_nth_data (layers, current_layer),
                    current_layer, num_layers,
                    orig_image,
                    origin_x, origin_y,
                    &saved_bpp, out_linear, error))
    {
      goto out;
    }
  current_layer++;

  /* close file so we can safely let exiv2 work on it to write metadata.
   * this can be simplified once multi page TIFF is supported by exiv2
   */
  TIFFFlushData (tif);
  TIFFClose (tif);
  tif = NULL;
  if (metadata)
    save_metadata (file, config, image, metadata, saved_bpp, config_cmyk);

  /* write the remaining layers */
  if (num_layers > 1)
    {
      tif = tiff_open (file, "a", error);

      if (! tif)
        {
          if (! error)
            g_set_error (error, G_FILE_ERROR,
                          g_file_error_from_errno (errno),
                          _("Could not open '%s' for writing: %s"),
                          gimp_file_get_utf8_name (file),
                          g_strerror (errno));
          goto out;
        }

      for (; current_layer < num_layers; current_layer++)
        {
          gint tmp_saved_bpp;

          if (! save_layer (tif, config, space, image,
                            g_list_nth_data (layers, current_layer),
                            current_layer, num_layers, orig_image,
                            origin_x, origin_y,
                            &tmp_saved_bpp, out_linear, error))
            {
              goto out;
            }

          if (tmp_saved_bpp != saved_bpp)
            {
              /* this should never happen. if it does, decide if it's
               * really an error.
               */
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Writing pages with different bit depth "
                             "is strange."));
              goto out;
            }

          gimp_progress_update ((gdouble) (current_layer + 1) / num_layers);
        }
    }

  status = TRUE;

out:
  /* close the file for good */
  if (tif)
    {
      TIFFFlushData (tif);
      TIFFClose (tif);
    }

  gimp_progress_update (1.0);

  g_list_free (layers);

  return status;
}

gboolean
export_dialog (GimpImage     *image,
               GimpProcedure *procedure,
               GObject       *config,
               gboolean       has_alpha,
               gboolean       is_monochrome,
               gboolean       is_indexed,
               gboolean       is_multi_layer,
               gboolean       classic_tiff_failed)
{
  GtkWidget         *dialog;
  GtkWidget         *profile_label;
  gchar            **parasites;
  GimpCompression    compression;
  gboolean           run;
  gboolean           has_geotiff  = FALSE;
  gint               i;
  GimpColorProfile  *cmyk_profile = NULL;
  GParamSpec        *compression_spec;
  GimpChoice        *compression_choice;
  GimpFormat         format;
  gboolean           save_transparent_pixels;
  gboolean           transparent_pixels_available;
  gboolean           save_layers;
  gboolean           save_layers_available;
  gboolean           save_resources;

  compression_spec   = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                     "compression");
  compression_choice = gimp_param_spec_choice_get_choice (compression_spec);

  gimp_choice_set_sensitive (compression_choice, "ccittfax3", is_monochrome);
  gimp_choice_set_sensitive (compression_choice, "ccittfax4", is_monochrome);
  gimp_choice_set_sensitive (compression_choice, "jpeg",      ! is_indexed);

  parasites = gimp_image_get_parasite_list (image);
  for (i = 0; i < g_strv_length (parasites); i++)
    {
      if (g_str_has_prefix (parasites[i], "Gimp_GeoTIFF_"))
        {
          has_geotiff = TRUE;
          break;
        }
    }
  g_strfreev (parasites);

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  /* Save initial values (from config) and availability flags (from image/export state).
   *
   * IMPORTANT: the "last-save-*" fields are treated as the user's last
   * Standard-TIFF choices. They are updated when switching from Standard -> Photoshop
   * (right before we force/lock Photoshop-mode values), and re-applied when switching
   * back (Photoshop -> Standard).
   */
  transparent_pixels_available = has_alpha && ! is_indexed;
  save_layers_available        = is_multi_layer;

  format = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                "format");

  g_object_get (config,
                "save-layers",             &save_layers,
                "save-resources",          &save_resources,
                "save-transparent-pixels", &save_transparent_pixels,
                NULL);

  g_object_set_data (G_OBJECT (dialog),
                     "tiff-prev-format",
                     GINT_TO_POINTER ((gint) format));

  g_object_set_data (G_OBJECT (dialog),
                     "transparent-pixels-available",
                     GINT_TO_POINTER (transparent_pixels_available));
  g_object_set_data (G_OBJECT (dialog),
                     "save-layers-available",
                     GINT_TO_POINTER (save_layers_available));

  g_object_set_data (G_OBJECT (dialog),
                     "last-save-transparent-pixels",
                     GINT_TO_POINTER (save_transparent_pixels));
  g_object_set_data (G_OBJECT (dialog),
                     "last-save-layers",
                     GINT_TO_POINTER (save_layers));
  g_object_set_data (G_OBJECT (dialog),
                     "last-save-resources",
                     GINT_TO_POINTER (save_resources));

  if (classic_tiff_failed)
    {
      GtkWidget *label;
      gchar     *text;

      /* Warning sign emoticone. */
      text = g_strdup_printf ("\xe2\x9a\xa0 %s",
                              _("Warning: maximum TIFF file size exceeded. "
                                "Retry as BigTIFF or with a different compression algorithm, "
                                "or cancel."));
      label = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                               "big-tif-warning", text,
                                               FALSE, FALSE);
      g_free (text);

      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_label_set_line_wrap_mode (GTK_LABEL (label), PANGO_WRAP_WORD);
      gtk_label_set_max_width_chars (GTK_LABEL (label), 60);
    }

  gimp_export_procedure_dialog_add_metadata (GIMP_EXPORT_PROCEDURE_DIALOG (dialog),
                                             "save-geotiff");
  gimp_export_procedure_dialog_add_metadata (GIMP_EXPORT_PROCEDURE_DIALOG (dialog),
                                             "save-resources");

  /* Profile label. */
  profile_label = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                                   "profile-label",
                                                   _("No soft-proofing profile"),
                                                   FALSE, FALSE);
  gtk_widget_set_margin_bottom (profile_label, 6);
  gtk_label_set_xalign (GTK_LABEL (profile_label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (profile_label), PANGO_ELLIPSIZE_END);
  gimp_label_set_attributes (GTK_LABEL (profile_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gimp_help_set_help_data (profile_label,
                           _("Name of the color profile used for CMYK export."),
                           NULL);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "cmyk-box",
                                  "profile-label",
                                  NULL);

  /* ...and make that box the content of a frame controlled by the "cmyk" toggle. */
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "cmyk-frame",
                                    "cmyk",
                                    FALSE,
                                    "cmyk-box");

  cmyk_profile = gimp_image_get_simulation_profile (image);
  if (cmyk_profile)
    {
      gchar *label_text;

      if (gimp_color_profile_is_cmyk (cmyk_profile))
        {
          label_text = g_strdup_printf (_("Profile: %s"),
                                        gimp_color_profile_get_label (cmyk_profile));
        }
      else
        {
          label_text = g_strdup_printf (_("The assigned soft-proofing profile is not a CMYK profile.\n"
                                          "This profile will not be included in the exported image."));
        }

      gtk_label_set_text (GTK_LABEL (profile_label), label_text);
      gimp_label_set_attributes (GTK_LABEL (profile_label),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_NORMAL,
                                 -1);
      g_free (label_text);

      g_object_unref (cmyk_profile);
    }

  if (classic_tiff_failed)
    gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                "big-tif-warning",
                                "compression",
                                "bigtiff",
                                "cmyk-frame",
                                "save-layers",
                                "format",
                                "save-transparent-pixels",
                                "crop-layers",
                                NULL);
  else
    gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                "compression",
                                "bigtiff",
                                "cmyk-frame",
                                "save-layers",
                                "format",
                                "save-transparent-pixels",
                                "crop-layers",
                                NULL);

  /* GeoTIFF option only makes sense when GeoTIFF parasites exist. */
  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "save-geotiff",
                                       has_geotiff,
                                       NULL, NULL, FALSE);

  /* Keep options consistent with the selected format. */
  g_signal_connect (config, "notify::format",
                    G_CALLBACK (evaluate_format_options),
                    dialog);

  /* Run once to bring UI/config into a consistent initial state */
  evaluate_format_options (config, NULL, dialog);

  compression = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                     "compression");

  if (! is_monochrome)
    {
      if (compression == GIMP_COMPRESSION_CCITTFAX3 ||
          compression == GIMP_COMPRESSION_CCITTFAX4)
        g_object_set (config, "compression", "none", NULL);
    }

  if (is_indexed && compression == GIMP_COMPRESSION_JPEG)
    g_object_set (config, "compression", "none", NULL);

  gtk_widget_set_visible (dialog, TRUE);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
update_format_options (GtkWidget *dialog,
                       GObject   *config,
                       gboolean   transparent_pixels_available,
                       gboolean   save_layers_available,
                       gboolean   leaving_photoshop_mode,
                       gboolean   entering_photoshop_mode)
{
  /* This function owns the edge-triggered snapshot/restore logic.
   * - On Standard -> Photoshop: snapshot current UI values into dialog data
   *   ("last-save-*") BEFORE we force Photoshop-recommended values.
   * - On Photoshop -> Standard: restore from dialog data.
   */
  GimpFormat format;
  gboolean   save_layers                  = FALSE;
  gboolean   save_resources               = FALSE;
  gboolean   save_transparent_pixels      = FALSE;
  gboolean   last_save_layers             = FALSE;
  gboolean   last_save_resources          = FALSE;
  gboolean   last_save_transparent_pixels = FALSE;

  format = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                               "format");

  /* Current config state. */
  g_object_get (config,
                "save-layers",             &save_layers,
                "save-resources",          &save_resources,
                "save-transparent-pixels", &save_transparent_pixels,
                NULL);

  /* Read previous snapshot from dialog data (defaults to current if missing). */
  last_save_layers =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog),
                                        "last-save-layers"));
  last_save_resources =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog),
                                        "last-save-resources"));
  last_save_transparent_pixels =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog),
                                        "last-save-transparent-pixels"));

  /* Edge: Standard -> Photoshop.
   * Snapshot current values BEFORE we force/lock Photoshop-mode values.
   */
  if (entering_photoshop_mode)
    {
      g_object_set_data (G_OBJECT (dialog),
                         "last-save-layers",
                         GINT_TO_POINTER (save_layers));
      g_object_set_data (G_OBJECT (dialog),
                         "last-save-resources",
                         GINT_TO_POINTER (save_resources));
      g_object_set_data (G_OBJECT (dialog),
                         "last-save-transparent-pixels",
                         GINT_TO_POINTER (save_transparent_pixels));
    }

  if (format == GIMP_TIFF_FORMAT_PHOTOSHOP_TIFF)
    {
      /* Photoshop TIFF => force a consistent "recommended" set. */
      if (! save_resources)
        g_object_set (config, "save-resources", TRUE, NULL);

      if (! save_layers)
        g_object_set (config, "save-layers", TRUE, NULL);

      if (save_transparent_pixels)
        g_object_set (config, "save-transparent-pixels", FALSE, NULL);

      /* Lock options in Photoshop mode */
      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "save-resources",
                                           FALSE,
                                           NULL, NULL, FALSE);

      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "save-layers",
                                           FALSE,
                                           NULL, NULL, FALSE);

      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "save-transparent-pixels",
                                           FALSE,
                                           NULL, NULL, FALSE);
    }
  else
    {
      /* Standard TIFF: unlock controls first (actual availability below). */
      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "save-resources",
                                           TRUE,
                                           NULL, NULL, FALSE);

      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "save-layers",
                                           TRUE,
                                           NULL, NULL, FALSE);

      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "save-transparent-pixels",
                                           TRUE,
                                           NULL, NULL, FALSE);

      /* Edge: Photoshop -> Standard. */
      if (leaving_photoshop_mode)
        {
          if (save_layers != last_save_layers)
            g_object_set (config, "save-layers", last_save_layers, NULL);

          if (save_transparent_pixels != last_save_transparent_pixels)
            g_object_set (config, "save-transparent-pixels", last_save_transparent_pixels, NULL);

          if (save_resources != last_save_resources)
            g_object_set (config, "save-resources", last_save_resources, NULL);
        }

      /* Apply availability-derived sensitivity (layers + transparent pixels only). */
      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "save-layers",
                                           save_layers_available,
                                           NULL, NULL, FALSE);

      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "save-transparent-pixels",
                                           transparent_pixels_available,
                                           NULL, NULL, FALSE);

      if (! save_layers_available && save_layers)
        g_object_set (config, "save-layers", FALSE, NULL);

      if (! transparent_pixels_available && save_transparent_pixels)
        g_object_set (config, "save-transparent-pixels", FALSE, NULL);
    }
}

static void
evaluate_format_options (GObject    *config,
                         GParamSpec *pspec,
                         gpointer    user_data)
{
  GtkWidget  *dialog = GTK_WIDGET (user_data);
  gboolean    transparent_pixels_available;
  gboolean    save_layers_available;
  GimpFormat  format;
  gpointer    prev_p;
  GimpFormat  prev_format;
  gboolean    entering_photoshop;
  gboolean    leaving_photoshop_mode;

  /* Read availability from dialog data (derived from image/export state). */
  transparent_pixels_available =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog),
                                        "transparent-pixels-available"));
  save_layers_available =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog),
                                        "save-layers-available"));

  /* Detect format transitions. */
  format = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                "format");

  prev_p      = g_object_get_data (G_OBJECT (dialog), "tiff-prev-format");
  prev_format = (GimpFormat) GPOINTER_TO_INT (prev_p);

  leaving_photoshop_mode =
    (prev_format == GIMP_TIFF_FORMAT_PHOTOSHOP_TIFF &&
     format      != GIMP_TIFF_FORMAT_PHOTOSHOP_TIFF);

  entering_photoshop =
    (prev_format != GIMP_TIFF_FORMAT_PHOTOSHOP_TIFF &&
     format      == GIMP_TIFF_FORMAT_PHOTOSHOP_TIFF);

  /* Store current as previous for next time. */
  g_object_set_data (G_OBJECT (dialog),
                     "tiff-prev-format",
                     GINT_TO_POINTER ((gint) format));

  update_format_options (dialog, config,
                         transparent_pixels_available,
                         save_layers_available,
                         leaving_photoshop_mode,
                         entering_photoshop);
}

/* Convert n bytes of 0/1 to a line of bits */
static void
byte2bit (const guchar *byteline,
          gint          width,
          guchar       *bitline,
          gboolean      invert)
{
  guchar bitval;
  guchar rest[8];

  while (width >= 8)
    {
      bitval = 0;
      if (*(byteline++))
        bitval |= 0x80;
      if (*(byteline++))
        bitval |= 0x40;
      if (*(byteline++))
        bitval |= 0x20;
      if (*(byteline++))
        bitval |= 0x10;
      if (*(byteline++))
        bitval |= 0x08;
      if (*(byteline++))
        bitval |= 0x04;
      if (*(byteline++))
        bitval |= 0x02;
      if (*(byteline++))
        bitval |= 0x01;

      *(bitline++) = invert ? ~bitval : bitval;
      width -= 8;
    }

  if (width > 0)
    {
      memset (rest, 0, 8);
      memcpy (rest, byteline, width);

      bitval   = 0;
      byteline = rest;

      if (*(byteline++))
        bitval |= 0x80;
      if (*(byteline++))
        bitval |= 0x40;
      if (*(byteline++))
        bitval |= 0x20;
      if (*(byteline++))
        bitval |= 0x10;
      if (*(byteline++))
        bitval |= 0x08;
      if (*(byteline++))
        bitval |= 0x04;
      if (*(byteline++))
        bitval |= 0x02;

      *bitline = invert ? ~bitval & (0xff << (8 - width)) : bitval;
    }
}

static const char *
get_layer_key (gshort bitspersample)
{
  switch (bitspersample)
    {
    case 16:
      return "Lr16";
    case 32:
      return "Lr32";
    default:
      return "Layr";
    }
}

static const Babl *
get_extra_format (gshort bitspersample, gshort sampleformat)
{
  if (sampleformat == SAMPLEFORMAT_UINT)
    {
      switch (bitspersample)
        {
        case 16:
          return babl_format ("Y u16");
        case 32:
          return babl_format ("Y u32");
        default:
          return babl_format ("Y u8");
        }
    }
  else if (sampleformat == SAMPLEFORMAT_IEEEFP)
    {
      switch (bitspersample)
        {
        case 16:
          return babl_format ("Y half");
        case 32:
          return babl_format ("Y float");
        case 64:
          return babl_format ("Y double");
        default:
          return babl_format ("Y u8");
        }
    }
  return babl_format ("Y u8");
}

static gboolean
should_export_layer_info (GimpImage *orig_image)
{
  gint       image_width;
  gint       image_height;
  GList     *layers;
  GimpLayer *layer;
  gint       layer_width;
  gint       layer_height;
  gint       layer_offset_x;
  gint       layer_offset_y;

  image_width  = gimp_image_get_width (orig_image);
  image_height = gimp_image_get_height (orig_image);
  layers       = gimp_image_list_layers (orig_image);

  if (layers && layers->data)
    {
      layer = layers->data;

      if (layers->next)
        goto export_true;

      if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
        goto export_true;

      if (gimp_layer_get_mask (layer))
        goto export_true;

      if (gimp_layer_get_opacity (layer) < 100.0)
        goto export_true;

      if (gimp_layer_get_mode (layer) != GIMP_LAYER_MODE_NORMAL)
        goto export_true;

      layer_width  = gimp_drawable_get_width (GIMP_DRAWABLE (layer));
      layer_height = gimp_drawable_get_height (GIMP_DRAWABLE (layer));

      if (layer_width != image_width || layer_height != image_height)
        goto export_true;

      gimp_drawable_get_offsets (GIMP_DRAWABLE (layer), &layer_offset_x,
                                 &layer_offset_y);

      if (layer_offset_x != 0 || layer_offset_y != 0)
        goto export_true;
    }

  g_list_free (layers);
  return FALSE;

export_true:
  g_list_free (layers);
  return TRUE;
}