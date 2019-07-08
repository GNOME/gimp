/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * jpeg-settings.c
 * Copyright (C) 2007 Raphaël Quinet <raphael@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Structure of the "jpeg-settings" parasite:
 *       1 byte  - JPEG color space (JCS_YCbCr, JCS_GRAYSCALE, JCS_CMYK, ...)
 *       1 byte  - quality (1..100 according to the IJG scale, or 0)
 *       1 byte  - number of components (0..4)
 *       1 byte  - number of quantization tables (0..4)
 *   C * 2 bytes - sampling factors for each component (1..4)
 * T * 128 bytes - quantization tables (only if different from IJG tables)
 *
 * Additional data following the quantization tables is currently
 * ignored and can be used for future extensions.
 *
 * In order to improve the compatibility with future versions of the
 * plug-in that may support more subsampling types ("subsmp"), the
 * parasite contains the original subsampling for each component
 * instead of saving only one byte containing the subsampling type as
 * used by the jpeg plug-in.  The same applies to the other settings:
 * for example, up to 4 quantization tables will be saved in the
 * parasite even if the current code cannot restore more than 2 of
 * them (4 tables may be needed by unusual JPEG color spaces such as
 * JCS_CMYK or JCS_YCCK).
 */

#include "config.h"

#include <string.h>
#include <setjmp.h>

#include <glib/gstdio.h>

#include <jpeglib.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"

#include "jpeg.h"
#include "jpeg-quality.h"
#include "jpeg-settings.h"

/**
 * jpeg_detect_original_settings:
 * @cinfo: a pointer to a JPEG decompressor info.
 * @image_ID: the image to which the parasite should be attached.
 *
 * Analyze the image being decompressed (@cinfo) and extract the
 * sampling factors, quantization tables and overall image quality.
 * Store this information in a parasite and attach it to @image_ID.
 *
 * This function must be called after jpeg_read_header() so that
 * @cinfo contains the quantization tables and the sampling factors
 * for each component.
 *
 * Return Value: TRUE if a parasite has been attached to @image_ID.
 */
gboolean
jpeg_detect_original_settings (struct jpeg_decompress_struct *cinfo,
                               gint32                         image_ID)
{
  guint         parasite_size;
  guchar       *parasite_data;
  GimpParasite *parasite;
  guchar       *dest;
  gint          quality;
  gint          num_quant_tables = 0;
  gint          t;
  gint          i;

  g_return_val_if_fail (cinfo != NULL, FALSE);
  if (cinfo->jpeg_color_space == JCS_UNKNOWN
      || cinfo->out_color_space == JCS_UNKNOWN)
    return FALSE;

  quality = jpeg_detect_quality (cinfo);
  /* no need to attach quantization tables if they are the ones from IJG */
  if (quality <= 0)
    {
      for (t = 0; t < 4; t++)
        if (cinfo->quant_tbl_ptrs[t])
          num_quant_tables++;
    }

  parasite_size = 4 + cinfo->num_components * 2 + num_quant_tables * 128;
  parasite_data = g_new (guchar, parasite_size);
  dest = parasite_data;

  *dest++ = CLAMP0255 (cinfo->jpeg_color_space);
  *dest++ = ABS (quality);
  *dest++ = CLAMP0255 (cinfo->num_components);
  *dest++ = num_quant_tables;

  for (i = 0; i < cinfo->num_components; i++)
    {
      *dest++ = CLAMP0255 (cinfo->comp_info[i].h_samp_factor);
      *dest++ = CLAMP0255 (cinfo->comp_info[i].v_samp_factor);
    }

  if (quality <= 0)
    {
      for (t = 0; t < 4; t++)
        if (cinfo->quant_tbl_ptrs[t])
          for (i = 0; i < DCTSIZE2; i++)
            {
              guint16 c = cinfo->quant_tbl_ptrs[t]->quantval[i];
              *dest++ = c / 256;
              *dest++ = c & 255;
            }
    }

  parasite = gimp_parasite_new ("jpeg-settings",
                                GIMP_PARASITE_PERSISTENT,
                                parasite_size,
                                parasite_data);
  g_free (parasite_data);
  gimp_image_attach_parasite (image_ID, parasite);
  gimp_parasite_free (parasite);
  return TRUE;
}


/*
 * TODO: compare the JPEG color space found in the parasite with the
 * GIMP color space of the drawable to be saved.  If one of them is
 * grayscale and the other isn't, then the quality setting may be used
 * but the subsampling parameters and quantization tables should be
 * ignored.  The drawable_ID needs to be passed around because the
 * color space of the drawable may be different from that of the image
 * (e.g., when saving a mask or channel).
 */

/**
 * jpeg_restore_original_settings:
 * @image_ID: the image that may contain original jpeg settings in a parasite.
 * @quality: where to store the original jpeg quality.
 * @subsmp: where to store the original subsampling type.
 * @num_quant_tables: where to store the number of quantization tables found.
 *
 * Retrieve the original JPEG settings (quality, type of subsampling
 * and number of quantization tables) from the parasite attached to
 * @image_ID.  If the number of quantization tables is greater than
 * zero, then these tables can be retrieved from the parasite by
 * calling jpeg_restore_original_tables().
 *
 * Return Value: TRUE if a valid parasite was attached to the image
 */
gboolean
jpeg_restore_original_settings (gint32           image_ID,
                                gint            *quality,
                                JpegSubsampling *subsmp,
                                gint            *num_quant_tables)
{
  GimpParasite *parasite;
  const guchar *src;
  glong         src_size;
  gint          color_space;
  gint          q;
  gint          num_components;
  gint          num_tables;
  guchar        h[3];
  guchar        v[3];

  g_return_val_if_fail (quality != NULL, FALSE);
  g_return_val_if_fail (subsmp != NULL, FALSE);
  g_return_val_if_fail (num_quant_tables != NULL, FALSE);

  parasite = gimp_image_get_parasite (image_ID, "jpeg-settings");
  if (parasite)
    {
      src = gimp_parasite_data (parasite);
      src_size = gimp_parasite_data_size (parasite);
      if (src_size >= 4)
        {
          color_space      = *src++;
          q                = *src++;
          num_components   = *src++;
          num_tables       = *src++;

          if (src_size >= (4 + num_components * 2 + num_tables * 128)
              && q <= 100 && num_tables <= 4)
            {
              *quality = q;

              /* the current plug-in can only create grayscale or YCbCr JPEGs */
              if (color_space == JCS_GRAYSCALE || color_space == JCS_YCbCr)
                *num_quant_tables = num_tables;
              else
                *num_quant_tables = -1;

              /* the current plug-in can only use subsampling for YCbCr (3) */
              *subsmp = -1;
              if (num_components == 3)
                {
                  h[0] = *src++;
                  v[0] = *src++;
                  h[1] = *src++;
                  v[1] = *src++;
                  h[2] = *src++;
                  v[2] = *src++;

                  if (h[1] == 1 && v[1] == 1 && h[2] == 1 && v[2] == 1)
                    {
                      if (h[0] == 1 && v[0] == 1)
                        *subsmp = JPEG_SUBSAMPLING_1x1_1x1_1x1;
                      else if (h[0] == 2 && v[0] == 1)
                        *subsmp = JPEG_SUBSAMPLING_2x1_1x1_1x1;
                      else if (h[0] == 1 && v[0] == 2)
                        *subsmp = JPEG_SUBSAMPLING_1x2_1x1_1x1;
                      else if (h[0] == 2 && v[0] == 2)
                        *subsmp = JPEG_SUBSAMPLING_2x2_1x1_1x1;
                    }
                }

              gimp_parasite_free (parasite);
              return TRUE;
            }
        }

      gimp_parasite_free (parasite);
    }

  *quality = -1;
  *subsmp = -1;
  *num_quant_tables = 0;

  return FALSE;
}


/**
 * jpeg_restore_original_tables:
 * @image_ID: the image that may contain original jpeg settings in a parasite.
 * @num_quant_tables: the number of quantization tables to restore.
 *
 * Retrieve the original quantization tables from the parasite
 * attached to @image_ID.  Each table is an array of coefficients that
 * can be associated with a component of a JPEG image when saving it.
 *
 * An array of newly allocated tables is returned if @num_quant_tables
 * matches the number of tables saved in the parasite.  These tables
 * are returned as arrays of unsigned integers even if they will never
 * use more than 16 bits (8 bits in most cases) because the IJG JPEG
 * library expects arrays of unsigned integers.  When these tables are
 * not needed anymore, the caller should free them using g_free().  If
 * no parasite exists or if it cannot be used, this function returns
 * NULL.
 *
 * Return Value: an array of quantization tables, or NULL.
 */
guint **
jpeg_restore_original_tables (gint32    image_ID,
                              gint      num_quant_tables)
{
  GimpParasite *parasite;
  const guchar *src;
  glong         src_size;
  gint          num_components;
  gint          num_tables;
  guint       **quant_tables;
  gint          t;
  gint          i;

  parasite = gimp_image_get_parasite (image_ID, "jpeg-settings");
  if (parasite)
    {
      src_size = gimp_parasite_data_size (parasite);
      if (src_size >= 4)
        {
          src = gimp_parasite_data (parasite);
          num_components = src[2];
          num_tables     = src[3];

          if (src_size >= (4 + num_components * 2 + num_tables * 128)
              && num_tables == num_quant_tables)
            {
              src += 4 + num_components * 2;
              quant_tables = g_new (guint *, num_tables);

              for (t = 0; t < num_tables; t++)
                {
                  quant_tables[t] = g_new (guint, 128);
                  for (i = 0; i < 64; i++)
                    {
                      guint c;

                      c = *src++ * 256;
                      c += *src++;
                      quant_tables[t][i] = c;
                    }
                }
              gimp_parasite_free (parasite);
              return quant_tables;
            }
        }
      gimp_parasite_free (parasite);
    }
  return NULL;
}


/**
 * jpeg_swap_original_settings:
 * @image_ID: the image that may contain original jpeg settings in a parasite.
 *
 * Swap the horizontal and vertical axis for the saved subsampling
 * parameters and quantization tables.  This should be done if the
 * image has been rotated by +90 or -90 degrees or if it has been
 * mirrored along its diagonal.
 */
void
jpeg_swap_original_settings (gint32 image_ID)
{
  GimpParasite *parasite;
  const guchar *src;
  glong         src_size;
  gint          num_components;
  gint          num_tables;
  guchar       *new_data;
  guchar       *dest;
  gint          t;
  gint          i;
  gint          j;

  parasite = gimp_image_get_parasite (image_ID, "jpeg-settings");
  if (parasite)
    {
      src_size = gimp_parasite_data_size (parasite);
      if (src_size >= 4)
        {
          src = gimp_parasite_data (parasite);
          num_components = src[2];
          num_tables     = src[3];

          if (src_size >= (4 + num_components * 2 + num_tables * 128))
            {
              new_data = g_new (guchar, src_size);
              dest = new_data;
              *dest++ = *src++;
              *dest++ = *src++;
              *dest++ = *src++;
              *dest++ = *src++;
              for (i = 0; i < num_components; i++)
                {
                  dest[0] = src[1];
                  dest[1] = src[0];
                  dest += 2;
                  src += 2;
                }
              for (t = 0; t < num_tables; t++)
                {
                  for (i = 0; i < 8; i++)
                    {
                      for (j = 0; j < 8; j++)
                        {
                          dest[i * 16 + j * 2]     = src[j * 16 + i * 2];
                          dest[i * 16 + j * 2 + 1] = src[j * 16 + i * 2 + 1];
                        }
                    }
                  dest += 128;
                  src += 128;
                  if (src_size > (4 + num_components * 2 + num_tables * 128))
                    {
                      memcpy (dest, src, src_size - (4 + num_components * 2
                                                     + num_tables * 128));
                    }
                }
              gimp_parasite_free (parasite);
              parasite = gimp_parasite_new ("jpeg-settings",
                                            GIMP_PARASITE_PERSISTENT,
                                            src_size,
                                            new_data);
              g_free (new_data);
              gimp_image_attach_parasite (image_ID, parasite);
            }
        }
      gimp_parasite_free (parasite);
    }
}
