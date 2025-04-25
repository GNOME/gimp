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


static gboolean  save_paths             (TIFF          *tif,
                                         GimpImage     *image,
                                         gdouble        width,
                                         gdouble        height,
                                         gint           offset_x,
                                         gint           offset_y);

static void      byte2bit               (const guchar  *byteline,
                                         gint           width,
                                         guchar        *bitline,
                                         gboolean       invert);


static void
double_to_psd_fixed (gdouble  value,
                     gchar   *target)
{
  gdouble in, frac;
  gint    i, f;

  frac = modf (value, &in);
  if (frac < 0)
    {
      in -= 1;
      frac += 1;
    }

  i = (gint) CLAMP (in, -16, 15);
  f = CLAMP ((gint) (frac * 0xFFFFFF), 0, 0xFFFFFF);

  target[0] = i & 0xFF;
  target[1] = (f >> 16) & 0xFF;
  target[2] = (f >>  8) & 0xFF;
  target[3] = f & 0xFF;
}

static gboolean
save_paths (TIFF      *tif,
            GimpImage *image,
            gdouble    width,
            gdouble    height,
            gint       offset_x,
            gint       offset_y)
{
  gint     id = 2000; /* Photoshop paths have IDs >= 2000 */
  GList   *path;
  GList   *iter;
  gint     v;
  gsize    num_strokes;
  gint    *strokes, s;
  GString *ps_tag;

  path = gimp_image_list_paths (image);

  if (! path)
    return FALSE;

  ps_tag = g_string_new ("");

  /* Only up to 1000 paths supported */
  for (iter = path, v = 0;
       iter && v < 1000;
       iter = g_list_next (iter), v++)
    {
      GString *data;
      gchar   *name, *nameend;
      gsize    len;
      gint     lenpos;
      gchar    pointrecord[26] = { 0, };
      gchar   *tmpname;
      GError  *err = NULL;

      data = g_string_new ("8BIM");
      g_string_append_c (data, id / 256);
      g_string_append_c (data, id % 256);

      /*
       * - use iso8859-1 if possible
       * - otherwise use UTF-8, prepended with \xef\xbb\xbf (Byte-Order-Mark)
       */
      name = gimp_item_get_name (iter->data);
      tmpname = g_convert (name, -1, "iso8859-1", "utf-8", NULL, &len, &err);

      if (tmpname && err == NULL)
        {
          g_string_append_c (data, MIN (len, 255));
          g_string_append_len (data, tmpname, MIN (len, 255));
          g_free (tmpname);
        }
      else
        {
          /* conversion failed, we fall back to UTF-8 */
          len = g_utf8_strlen (name, 255 - 3);  /* need three marker-bytes */

          nameend = g_utf8_offset_to_pointer (name, len);
          len = nameend - name; /* in bytes */
          g_assert (len + 3 <= 255);

          g_string_append_c (data, len + 3);
          g_string_append_len (data, "\xEF\xBB\xBF", 3); /* Unicode 0xfeff */
          g_string_append_len (data, name, len);

          if (tmpname)
            g_free (tmpname);
        }

      if (data->len % 2)  /* padding to even size */
        g_string_append_c (data, 0);
      g_free (name);

      lenpos = data->len;
      g_string_append_len (data, "\0\0\0\0", 4); /* will be filled in later */
      len = data->len; /* to calculate the data size later */

      pointrecord[1] = 6;  /* fill rule record */
      g_string_append_len (data, pointrecord, 26);

      strokes = gimp_path_get_strokes (iter->data, &num_strokes);

      for (s = 0; s < num_strokes; s++)
        {
          GimpPathStrokeType  type;
          gdouble            *points;
          gsize               num_points;
          gboolean            closed;
          gint                p = 0;

          type = gimp_path_stroke_get_points (iter->data, strokes[s],
                                              &num_points, &points, &closed);

          if (type != GIMP_PATH_STROKE_TYPE_BEZIER ||
              num_points > 65535                   ||
              num_points % 6)
            {
              g_printerr ("tiff-export: unsupported stroke type: "
                          "%d (%" G_GSIZE_FORMAT " points)\n", type, num_points);
              continue;
            }

          memset (pointrecord, 0, 26);
          pointrecord[1] = closed ? 0 : 3;
          pointrecord[2] = (num_points / 6) / 256;
          pointrecord[3] = (num_points / 6) % 256;
          g_string_append_len (data, pointrecord, 26);

          for (p = 0; p < num_points; p += 6)
            {
              pointrecord[1] = closed ? 2 : 5;

              double_to_psd_fixed ((points[p+1] - offset_y) / height, pointrecord + 2);
              double_to_psd_fixed ((points[p+0] - offset_x) / width,  pointrecord + 6);
              double_to_psd_fixed ((points[p+3] - offset_y) / height, pointrecord + 10);
              double_to_psd_fixed ((points[p+2] - offset_x) / width,  pointrecord + 14);
              double_to_psd_fixed ((points[p+5] - offset_y) / height, pointrecord + 18);
              double_to_psd_fixed ((points[p+4] - offset_x) / width,  pointrecord + 22);

              g_string_append_len (data, pointrecord, 26);
            }
        }

      g_free (strokes);

      /* fix up the length */

      len = data->len - len;
      data->str[lenpos + 0] = (len & 0xFF000000) >> 24;
      data->str[lenpos + 1] = (len & 0x00FF0000) >> 16;
      data->str[lenpos + 2] = (len & 0x0000FF00) >>  8;
      data->str[lenpos + 3] = (len & 0x000000FF) >>  0;

      g_string_append_len (ps_tag, data->str, data->len);
      g_string_free (data, TRUE);
      id ++;
    }

  TIFFSetField (tif, TIFFTAG_PHOTOSHOP, ps_tag->len, ps_tag->str);
  g_string_free (ps_tag, TRUE);

  g_list_free (path);

  return TRUE;
}

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
  gint              cols, rows, row, i;
  glong             rowsperstrip;
  gushort           compression;
  gushort           extra_samples[1];
  gboolean          alpha;
  gshort            predictor;
  gshort            photometric;
  const Babl       *format;
  const Babl       *type;
  gshort            samplesperpixel;
  gshort            bitspersample;
  gshort            sampleformat;
  gint              bytesperrow;
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
  gboolean          config_save_profile;
  gboolean          config_cmyk;

  g_object_get (config,
                "gimp-comment",            &config_comment,
                "include-comment",         &config_save_comment,
                "save-transparent-pixels", &config_save_transp_pixels,
                "save-geotiff",            &config_save_geotiff_tags,
                "include-color-profile",   &config_save_profile,
                "cmyk",                    &config_cmyk,
                NULL);

  config_compression = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "compression");
  compression = gimp_compression_to_tiff_compression (config_compression);

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

  switch (drawable_type)
    {
    case GIMP_RGB_IMAGE:
      predictor       = 2;
      samplesperpixel = 3;
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
      samplesperpixel = 1;
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
      samplesperpixel = 4;
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
      samplesperpixel = 2;
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

      samplesperpixel = (drawable_type == GIMP_INDEXEDA_IMAGE) ? 2 : 1;
      bytesperrow     = cols;
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

  bytesperrow = cols * babl_format_get_bytes_per_pixel (format);

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
      samplesperpixel = alpha ? 5 : 4;
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

      TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, 1, extra_samples);
    }

  TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, photometric);
  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
  /* TIFFSetField( tif, TIFFTAG_STRIPBYTECOUNTS, rows / rowsperstrip ); */
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

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

  /* save path data. we need layer information for that,
    * so we have to do this in here. :-( */
  if (page == 0)
    save_paths (tif, orig_image, cols, rows, offset_x, offset_y);

  /* array to rearrange data */
  src  = g_new (guchar, bytesperrow * tile_height);
  data = g_new (guchar, bytesperrow);

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
          guchar *t = src + bytesperrow * (row - y);

          switch (drawable_type)
            {
            case GIMP_INDEXED_IMAGE:
            case GIMP_INDEXEDA_IMAGE:
              if (is_bw)
                {
                  byte2bit (t, bytesperrow, data, invert);
                  success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
                }
              else
                {
                  success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
                }
              break;

            case GIMP_GRAY_IMAGE:
            case GIMP_GRAYA_IMAGE:
            case GIMP_RGB_IMAGE:
            case GIMP_RGBA_IMAGE:
              success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
              break;

            default:
              success = FALSE;
              break;
            }

          if (!success)
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
  TIFF             *tif                 = NULL;
  const Babl       *space               = NULL;
  gboolean          status              = FALSE;
  gboolean          out_linear          = FALSE;
  gint32            num_layers;
  gint32            current_layer       = 0;
  GList            *layers;
  GList            *iter;
  gint              origin_x            = 0;
  gint              origin_y            = 0;
  gint              saved_bpp;
  gboolean          bigtiff;
  gboolean          config_save_profile;
  gboolean          config_save_thumbnail;
  gboolean          config_cmyk;

  g_object_get (config,
                "bigtiff",               &bigtiff,
                "include-color-profile", &config_save_profile,
                "include-thumbnail",     &config_save_thumbnail,
                "cmyk",                  &config_cmyk,
                NULL);

  layers = gimp_image_list_layers (image);
  layers = g_list_reverse (layers);
  num_layers = g_list_length (layers);

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  /* Open file and write some global data */
  tif = tiff_open (file, (bigtiff ? "w8" : "w"), error);

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
      out_linear = (gimp_color_profile_is_linear (profile));

      if (profile)
        space = gimp_color_profile_get_space (profile,
                                              config_cmyk ?
                                              gimp_image_get_simulation_intent (image) :
                                              GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                              &error);

      if (error)
        {
          g_printerr ("%s: error getting the profile space: %s",
                      G_STRFUNC, error->message);
          g_error_free (error);
          space = NULL;
        }

      g_object_unref (profile);
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
save_dialog (GimpImage     *image,
             GimpProcedure *procedure,
             GObject       *config,
             gboolean       has_alpha,
             gboolean       is_monochrome,
             gboolean       is_indexed,
             gboolean       is_multi_layer,
             gboolean       classic_tiff_failed)
{
  GtkWidget        *dialog;
  GtkWidget        *profile_label;
  gchar           **parasites;
  GimpCompression   compression;
  gboolean          run;
  gboolean          has_geotiff  = FALSE;
  gint              i;
  GimpColorProfile *cmyk_profile = NULL;
  GParamSpec       *cspec;
  GimpChoice       *choice;

  cspec  = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "compression");
  choice = gimp_param_spec_choice_get_choice (cspec);
  gimp_choice_set_sensitive (choice, "ccittfax3", is_monochrome);
  gimp_choice_set_sensitive (choice, "ccittfax4", is_monochrome);
  gimp_choice_set_sensitive (choice, "jpeg",      ! is_indexed);

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

  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "layers-frame", "save-layers", FALSE,
                                    "crop-layers");
  /* TODO: if single-layer TIFF, set the toggle insensitive and show it
   * as unchecked though I don't actually change the config value to
   * keep storing previously chosen value.
   * This used to be so before. We probably need to add some logics in
   * the GimpProcedureDialog generation for such case.
   */
  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "layers-frame", is_multi_layer,
                                       NULL, NULL, FALSE);
  /* TODO: same for "save-transparent-pixels", we probably want to show
   * it unchecked even though it doesn't matter for processing.
   */
  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "save-transparent-pixels",
                                       has_alpha && ! is_indexed,
                                       NULL, NULL, FALSE);

  /* Profile label. */
  profile_label = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                                   "profile-label", _("No soft-proofing profile"),
                                                   FALSE, FALSE);
  gtk_label_set_xalign (GTK_LABEL (profile_label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (profile_label), PANGO_ELLIPSIZE_END);
  gimp_label_set_attributes (GTK_LABEL (profile_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gimp_help_set_help_data (profile_label,
                           _("Name of the color profile used for CMYK export."), NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "cmyk-frame", "cmyk", FALSE,
                                    "profile-label");

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

  gimp_export_procedure_dialog_add_metadata (GIMP_EXPORT_PROCEDURE_DIALOG (dialog), "save-geotiff");
  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "save-geotiff",
                                       has_geotiff, NULL, NULL, FALSE);

  if (classic_tiff_failed)
    gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                "big-tif-warning",
                                "compression",
                                "bigtiff",
                                "layers-frame",
                                "save-transparent-pixels",
                                "cmyk-frame",
                                NULL);
  else
    gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                "compression",
                                "bigtiff",
                                "layers-frame",
                                "save-transparent-pixels",
                                "cmyk-frame",
                                NULL);

  compression = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "compression");

  if (! is_monochrome)
    {
      if (compression == GIMP_COMPRESSION_CCITTFAX3 ||
          compression == GIMP_COMPRESSION_CCITTFAX4)
        g_object_set (config, "compression", "none", NULL);
    }

  if (is_indexed && compression == GIMP_COMPRESSION_JPEG)
    g_object_set (config, "compression", "none", NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
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
      if (*(byteline++)) bitval |= 0x80;
      if (*(byteline++)) bitval |= 0x40;
      if (*(byteline++)) bitval |= 0x20;
      if (*(byteline++)) bitval |= 0x10;
      if (*(byteline++)) bitval |= 0x08;
      if (*(byteline++)) bitval |= 0x04;
      if (*(byteline++)) bitval |= 0x02;
      if (*(byteline++)) bitval |= 0x01;
      *(bitline++) = invert ? ~bitval : bitval;
      width -= 8;
    }

  if (width > 0)
    {
      memset (rest, 0, 8);
      memcpy (rest, byteline, width);
      bitval = 0;
      byteline = rest;
      if (*(byteline++)) bitval |= 0x80;
      if (*(byteline++)) bitval |= 0x40;
      if (*(byteline++)) bitval |= 0x20;
      if (*(byteline++)) bitval |= 0x10;
      if (*(byteline++)) bitval |= 0x08;
      if (*(byteline++)) bitval |= 0x04;
      if (*(byteline++)) bitval |= 0x02;
      *bitline = invert ? ~bitval & (0xff << (8 - width)) : bitval;
    }
}
