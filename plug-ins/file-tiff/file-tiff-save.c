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

#include "file-tiff-io.h"
#include "file-tiff-save.h"

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_ROLE "gimp-file-tiff-save"


static gboolean  save_paths             (TIFF          *tif,
                                         gint32         image,
                                         gdouble        width,
                                         gdouble        height,
                                         gint           offset_x,
                                         gint           offset_y);

static void      comment_entry_callback (GtkWidget     *widget,
                                         gchar        **comment);

static void      byte2bit               (const guchar  *byteline,
                                         gint           width,
                                         guchar        *bitline,
                                         gboolean       invert);

static void      save_thumbnail         (TiffSaveVals  *tsvals,
                                         gint32         image,
                                         TIFF          *tif);


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
save_paths (TIFF    *tif,
            gint32   image,
            gdouble  width,
            gdouble  height,
            gint     offset_x,
            gint     offset_y)
{
  gint id = 2000; /* Photoshop paths have IDs >= 2000 */
  gint num_vectors, *vectors, v;
  gint num_strokes, *strokes, s;
  GString *ps_tag;

  vectors = gimp_image_get_vectors (image, &num_vectors);

  if (num_vectors <= 0)
    return FALSE;

  ps_tag = g_string_new ("");

  /* Only up to 1000 paths supported */
  for (v = 0; v < MIN (num_vectors, 1000); v++)
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
      name = gimp_item_get_name (vectors[v]);
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

      strokes = gimp_vectors_get_strokes (vectors[v], &num_strokes);

      for (s = 0; s < num_strokes; s++)
        {
          GimpVectorsStrokeType type;
          gdouble  *points;
          gint      num_points;
          gboolean  closed;
          gint      p = 0;

          type = gimp_vectors_stroke_get_points (vectors[v], strokes[s],
                                                 &num_points, &points, &closed);

          if (type != GIMP_VECTORS_STROKE_TYPE_BEZIER ||
              num_points > 65535 ||
              num_points % 6)
            {
              g_printerr ("tiff-save: unsupported stroke type: "
                          "%d (%d points)\n", type, num_points);
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

  g_free (vectors);

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
save_layer (TIFF         *tif,
            TiffSaveVals *tsvals,
            gint32        image,
            gint32        layer,
            gint32        page,
            gint32        num_pages,
            gint32        orig_image, /* the export function might */
                                      /* have created a duplicate  */
            gint         *saved_bpp,
            gboolean      out_linear,
            GError      **error)
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

  compression = tsvals->compression;

  layer_name = gimp_item_get_name (layer);

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

  drawable_type = gimp_drawable_type (layer);
  buffer        = gimp_drawable_get_buffer (layer);

  format = gegl_buffer_get_format (buffer);
  type   = babl_format_get_type (format, 0);

  switch (gimp_image_get_precision (image))
    {
    case GIMP_PRECISION_U8_LINEAR:
    case GIMP_PRECISION_U8_GAMMA:
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
    case GIMP_PRECISION_U16_GAMMA:
      bitspersample = 16;
      sampleformat  = SAMPLEFORMAT_UINT;
      break;

    case GIMP_PRECISION_U32_LINEAR:
    case GIMP_PRECISION_U32_GAMMA:
      bitspersample = 32;
      sampleformat  = SAMPLEFORMAT_UINT;
      break;

    case GIMP_PRECISION_HALF_LINEAR:
    case GIMP_PRECISION_HALF_GAMMA:
      bitspersample = 16;
      sampleformat  = SAMPLEFORMAT_IEEEFP;
      break;

    default:
    case GIMP_PRECISION_FLOAT_LINEAR:
    case GIMP_PRECISION_FLOAT_GAMMA:
      bitspersample = 32;
      sampleformat  = SAMPLEFORMAT_IEEEFP;
      break;

    case GIMP_PRECISION_DOUBLE_LINEAR:
    case GIMP_PRECISION_DOUBLE_GAMMA:
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
      if (tsvals->save_transp_pixels)
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
      if (tsvals->save_transp_pixels)
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
      cmap = gimp_image_get_colormap (image, &num_colors);

      if (num_colors == 2 || num_colors == 1)
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
      format          = gimp_drawable_get_format (layer);

      g_free (cmap);
      break;

    default:
      goto out;
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
      if (gimp_image_base_type (image) == GIMP_INDEXED)
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Indexed pictures cannot be compressed "
                                 "with \"JPEG\"."));
          goto out;
        }
    }


  /* Set TIFF parameters. */
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
      if (tsvals->save_transp_pixels ||
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

  gimp_drawable_offsets (layer, &offset_x, &offset_y);

  if (offset_x || offset_y)
    {
      TIFFSetField (tif, TIFFTAG_XPOSITION, offset_x / xresolution);
      TIFFSetField (tif, TIFFTAG_YPOSITION, offset_y / yresolution);
    }

  if (! is_bw &&
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
              g_message (_("Failed a scanline write on row %d"), row);
              goto out;
            }
        }

      if ((row % 32) == 0)
        gimp_progress_update (progress_base + progress_fraction
                              * (gdouble) row / (gdouble) rows);
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

static void
save_thumbnail (TiffSaveVals *tsvals,
                gint32        image,
                TIFF         *tif)
{
  /* now switch IFD and write thumbnail
   *
   * the thumbnail must be saved in a subimage of the image.
   * otherwise the thumbnail will be saved in a second page of the
   * same image.
   *
   * Exif saves the thumbnail as a second page. To avoid this, the
   * thumbnail must be saved with the functions of libtiff.
   */
  if (tsvals->save_thumbnail)
    {
      GdkPixbuf *thumb_pixbuf;
      guchar    *thumb_pixels;
      guchar    *buf;
      gint       image_width;
      gint       image_height;
      gint       thumbw;
      gint       thumbh;
      gint       x, y;

#define EXIF_THUMBNAIL_SIZE 256

      image_width  = gimp_image_width  (image);
      image_height = gimp_image_height (image);

      if (image_width > image_height)
        {
          thumbw = EXIF_THUMBNAIL_SIZE;
          thumbh = EXIF_THUMBNAIL_SIZE * image_height / image_width;
        }
      else
        {
          thumbh = EXIF_THUMBNAIL_SIZE;
          thumbw = EXIF_THUMBNAIL_SIZE * image_width / image_height;
        }

      thumb_pixbuf = gimp_image_get_thumbnail (image, thumbw, thumbh,
                                               GIMP_PIXBUF_KEEP_ALPHA);

      thumb_pixels = gdk_pixbuf_get_pixels (thumb_pixbuf);

      TIFFSetField (tif, TIFFTAG_SUBFILETYPE, FILETYPE_REDUCEDIMAGE);
      TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, thumbw);
      TIFFSetField (tif, TIFFTAG_IMAGELENGTH, thumbh);
      TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
      TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
      TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, thumbh);
      TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
      TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
      TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
      TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 3);

      buf = _TIFFmalloc (thumbw * 3);

      for (y = 0; y < thumbh; y++)
        {
          guchar *p = buf;

          for (x = 0; x < thumbw; x++)
            {
              *p++ = *thumb_pixels++; /* r */
              *p++ = *thumb_pixels++; /* g */
              *p++ = *thumb_pixels++; /* b */
              thumb_pixels++;
            }

          TIFFWriteScanline (tif, buf, y, 0);
        }

      _TIFFfree (buf);

      TIFFWriteDirectory (tif);

      g_object_unref (thumb_pixbuf);
    }
}

static void
save_metadata (GFile                 *file,
               TiffSaveVals          *tsvals,
               gint32                 image,
               GimpMetadata          *metadata,
               GimpMetadataSaveFlags  metadata_flags,
               gint                   saved_bpp)
{
  gchar **exif_tags;

  /* See bug 758909: clear TIFFTAG_MIN/MAXSAMPLEVALUE because
   * exiv2 saves them with wrong type and the original values
   * could be invalid, see also bug 761823.
   * we also clear some other tags that were only meaningful
   * for the original imported image.
   */
  static const gchar *exif_tags_to_remove[] = {
    "Exif.Image.0x0118",
    "Exif.Image.0x0119",
    "Exif.Image.0x011d",
    "Exif.Image.Compression",
    "Exif.Image.FillOrder",
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
  static const guint n_keys = G_N_ELEMENTS(exif_tags_to_remove);

  for (int k = 0; k < n_keys; k++)
    {
      gexiv2_metadata_clear_tag (GEXIV2_METADATA (metadata),
                                 exif_tags_to_remove[k]);
    }

  /* get rid of all the EXIF tags for anything but the first sub image. */
  exif_tags = gexiv2_metadata_get_exif_tags (GEXIV2_METADATA(metadata));
  for (char **tag = exif_tags; *tag; tag++)
    {
      if (g_str_has_prefix (*tag, "Exif.Image")
          && (*tag)[strlen ("Exif.Image")] >= '0'
          && (*tag)[strlen ("Exif.Image")] <= '9')
        gexiv2_metadata_clear_tag (GEXIV2_METADATA (metadata), *tag);
    }

  gimp_metadata_set_bits_per_sample (metadata, saved_bpp);

  if (tsvals->save_exif)
    metadata_flags |= GIMP_METADATA_SAVE_EXIF;
  else
    metadata_flags &= ~GIMP_METADATA_SAVE_EXIF;

  if (tsvals->save_xmp)
    metadata_flags |= GIMP_METADATA_SAVE_XMP;
  else
    metadata_flags &= ~GIMP_METADATA_SAVE_XMP;

  if (tsvals->save_iptc)
    metadata_flags |= GIMP_METADATA_SAVE_IPTC;
  else
    metadata_flags &= ~GIMP_METADATA_SAVE_IPTC;

  /* never save metadata thumbnails for TIFF, see bug #729952 */
  metadata_flags &= ~GIMP_METADATA_SAVE_THUMBNAIL;

  if (tsvals->save_profile)
    metadata_flags |= GIMP_METADATA_SAVE_COLOR_PROFILE;
  else
    metadata_flags &= ~GIMP_METADATA_SAVE_COLOR_PROFILE;

  gimp_image_metadata_save_finish (image,
                                   "image/tiff",
                                   metadata, metadata_flags,
                                   file, NULL);
}

gboolean
save_image (GFile                  *file,
            TiffSaveVals           *tsvals,
            gint32                  image,
            gint32                  orig_image,    /* the export function */
                                                   /* might have created  */
                                                   /* a duplicate         */
            const gchar            *image_comment,
            gint                   *saved_bpp,
            GimpMetadata           *metadata,
            GimpMetadataSaveFlags   metadata_flags,
            GError                **error)
{
  TIFF     *tif;
  gboolean  status              = FALSE;
  gboolean  out_linear          = FALSE;
  gint      number_of_sub_IFDs  = 1;
  toff_t    sub_IFDs_offsets[1] = { 0UL };
  gint32    num_layers, *layers, current_layer = 0;

  layers = gimp_image_get_layers (image, &num_layers);

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  /* Open file and write some gloabl data */
  tif = tiff_open (file, "w", error);

  if (! tif)
    {
      if (! error)
        g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                     _("Could not open '%s' for writing: %s"),
                     gimp_file_get_utf8_name (file), g_strerror (errno));
      goto out;
    }

  TIFFSetField (tif, TIFFTAG_DOCUMENTNAME, g_file_get_path (file));

  /* The TIFF spec explicitly says ASCII for the image description. */
  if (image_comment)
    {
      const gchar *c = image_comment;
      gint         len;

      for (len = strlen (c); len; c++, len--)
        {
          if ((guchar) *c > 127)
            {
              g_message (_("The TIFF format only supports comments in\n"
                           "7bit ASCII encoding. No comment is saved."));
              image_comment = NULL;

              break;
            }
        }
    }

  /* do we have a comment?  If so, create a new parasite to hold it,
   * and attach it to the image. The attach function automatically
   * detaches a previous incarnation of the parasite. */
  if (image_comment && *image_comment)
    {
      GimpParasite *parasite;

      TIFFSetField (tif, TIFFTAG_IMAGEDESCRIPTION, image_comment);
      parasite = gimp_parasite_new ("gimp-comment",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (image_comment) + 1, image_comment);
      gimp_image_attach_parasite (orig_image, parasite);
      gimp_parasite_free (parasite);
    }

#ifdef TIFFTAG_ICCPROFILE
  if (tsvals->save_profile)
    {
      GimpColorProfile *profile;
      const guint8     *icc_data;
      gsize             icc_length;

      profile = gimp_image_get_effective_color_profile (orig_image);

      /* Curve of the exported data depends on the saved profile, i.e.
       * any explicitly-set profile in priority, or the default one for
       * the storage format as fallback.
       */
      out_linear = (gimp_color_profile_is_linear (profile));

      /* Write the profile to the TIFF file. */
      icc_data = gimp_color_profile_get_icc_profile (profile, &icc_length);
      TIFFSetField (tif, TIFFTAG_ICCPROFILE, icc_length, icc_data);
      g_object_unref (profile);
    }
#endif

  /* we put the whole file's thumbnail into the first IFD (i.e., page) */
  if (tsvals->save_thumbnail)
    TIFFSetField (tif, TIFFTAG_SUBIFD, number_of_sub_IFDs, sub_IFDs_offsets);

  /* write last layer as first page. */
  if (! save_layer (tif,  tsvals, image,
                    layers[num_layers - current_layer - 1],
                    current_layer, num_layers,
                    orig_image, saved_bpp, out_linear, error))
    {
      goto out;
    }
  current_layer++;

  /* write thumbnail */
  if (tsvals->save_thumbnail)
    save_thumbnail (tsvals, image, tif);

  /* close file so we can savely let exiv2 work on it to write metadata.
   * this can be simplified once multi page TIFF is supported by exiv2
   */
  TIFFFlushData (tif);
  TIFFClose (tif);
  tif = NULL;
  if (metadata)
    save_metadata (file, tsvals, image, metadata, metadata_flags, *saved_bpp);

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
          if (! save_layer (tif,  tsvals, image,
                            layers[num_layers - current_layer - 1],
                            current_layer, num_layers, orig_image,
                            &tmp_saved_bpp, out_linear, error))
            {
              goto out;
            }
          if (tmp_saved_bpp != *saved_bpp)
            {
              /* this should never happen.
               * if it does, decide if it's really an error.
               */
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Writing pages with different bit depth "
                             "is strange."));
              goto out;
            }
          gimp_progress_update ((gdouble) (current_layer + 1) / num_layers);
        }
    }

  /* close the file for good */
  if (tif)
    {
      TIFFFlushData (tif);
      TIFFClose (tif);
    }

  gimp_progress_update (1.0);

  status = TRUE;

out:
  return status;
}

gboolean
save_dialog (TiffSaveVals  *tsvals,
             const gchar   *help_id,
             gboolean       has_alpha,
             gboolean       is_monochrome,
             gboolean       is_indexed,
             gchar        **image_comment)
{
  GError      *error = NULL;
  GtkWidget   *dialog;
  GtkWidget   *vbox;
  GtkWidget   *frame;
  GtkWidget   *entry;
  GtkWidget   *toggle;
  GtkWidget   *cmp_g3;
  GtkWidget   *cmp_g4;
  GtkWidget   *cmp_jpeg;
  GtkBuilder  *builder;
  gchar       *ui_file;
  gboolean     run;

  dialog = gimp_export_dialog_new (_("TIFF"), PLUG_IN_ROLE, help_id);

  builder = gtk_builder_new ();
  ui_file = g_build_filename (gimp_data_directory (),
                              "ui", "plug-ins", "plug-in-file-tiff.ui",
                              NULL);
  if (! gtk_builder_add_from_file (builder, ui_file, &error))
    {
      gchar *display_name = g_filename_display_name (ui_file);

      g_printerr (_("Error loading UI file '%s': %s"),
                  display_name, error ? error->message : _("Unknown error"));

      g_free (display_name);
    }

  g_free (ui_file);

  vbox = GTK_WIDGET (gtk_builder_get_object (builder, "tiff_export_vbox"));

  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  vbox = GTK_WIDGET (gtk_builder_get_object (builder, "radio_button_box"));

  frame = gimp_int_radio_group_new (TRUE, _("Compression"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &tsvals->compression, tsvals->compression,

                                    _("_None"),      COMPRESSION_NONE,          NULL,
                                    _("_LZW"),       COMPRESSION_LZW,           NULL,
                                    _("_Pack Bits"), COMPRESSION_PACKBITS,      NULL,
                                    _("_Deflate"),   COMPRESSION_ADOBE_DEFLATE, NULL,
                                    _("_JPEG"),      COMPRESSION_JPEG,          &cmp_jpeg,
                                    _("CCITT Group _3 fax"), COMPRESSION_CCITTFAX3, &cmp_g3,
                                    _("CCITT Group _4 fax"), COMPRESSION_CCITTFAX4, &cmp_g4,

                                    NULL);

  gtk_widget_set_sensitive (cmp_g3, is_monochrome);
  gtk_widget_set_sensitive (cmp_g4, is_monochrome);
  gtk_widget_set_sensitive (cmp_jpeg, ! is_indexed);

  if (! is_monochrome)
    {
      if (tsvals->compression == COMPRESSION_CCITTFAX3 ||
          tsvals->compression == COMPRESSION_CCITTFAX4)
        {
          gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (cmp_g3),
                                           COMPRESSION_NONE);
        }
    }

  if (is_indexed && tsvals->compression == COMPRESSION_JPEG)
    {
      gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (cmp_jpeg),
                                       COMPRESSION_NONE);
    }

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle = GTK_WIDGET (gtk_builder_get_object (builder, "save-alpha"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                has_alpha && (tsvals->save_transp_pixels || is_indexed));
  gtk_widget_set_sensitive (toggle, has_alpha && ! is_indexed);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &tsvals->save_transp_pixels);

  entry = GTK_WIDGET (gtk_builder_get_object (builder, "commentfield"));
  gtk_entry_set_text (GTK_ENTRY (entry), *image_comment ? *image_comment : "");

  g_signal_connect (entry, "changed",
                    G_CALLBACK (comment_entry_callback),
                    image_comment);

  toggle = GTK_WIDGET (gtk_builder_get_object (builder, "save-exif"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                tsvals->save_exif);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &tsvals->save_exif);

  toggle = GTK_WIDGET (gtk_builder_get_object (builder, "save-xmp"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                tsvals->save_xmp);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &tsvals->save_xmp);

  toggle = GTK_WIDGET (gtk_builder_get_object (builder, "save-iptc"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                tsvals->save_iptc);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &tsvals->save_iptc);

  toggle = GTK_WIDGET (gtk_builder_get_object (builder, "save-thumbnail"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                tsvals->save_thumbnail);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &tsvals->save_thumbnail);

  toggle = GTK_WIDGET (gtk_builder_get_object (builder, "save-color-profile"));
#ifdef TIFFTAG_ICCPROFILE
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                tsvals->save_profile);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &tsvals->save_profile);
#else
  gtk_widget_hide (toggle);
#endif

  toggle = GTK_WIDGET (gtk_builder_get_object (builder, "save-layers"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                tsvals->save_layers);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &tsvals->save_layers);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
comment_entry_callback (GtkWidget  *widget,
                        gchar     **comment)
{
  const gchar *text = gtk_entry_get_text (GTK_ENTRY (widget));

  g_free (*comment);
  *comment = g_strdup (text);
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
