/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <png.h>

/* #define ICO_DBG */

#include "ico.h"
#include "ico-load.h"

#include "libgimp/stdplugins-intl.h"


#define A_VAL(p) ((guchar *)(p))[3]
#define R_VAL(p) ((guchar *)(p))[2]
#define G_VAL(p) ((guchar *)(p))[1]
#define B_VAL(p) ((guchar *)(p))[0]

#define A_VAL_GIMP(p) ((guchar *)(p))[3]
#define R_VAL_GIMP(p) ((guchar *)(p))[0]
#define G_VAL_GIMP(p) ((guchar *)(p))[1]
#define B_VAL_GIMP(p) ((guchar *)(p))[2]


static gint       ico_read_int8  (FILE        *fp,
                                  guint8      *data,
                                  gint         count);
static gint       ico_read_int16 (FILE        *fp,
                                  guint16     *data,
                                  gint         count);
static gint       ico_read_int32 (FILE        *fp,
                                  guint32     *data,
                                  gint         count);

static gint
ico_read_int32 (FILE    *fp,
                guint32 *data,
                gint     count)
{
  gint i, total;

  total = count;
  if (count > 0)
    {
      ico_read_int8 (fp, (guint8 *) data, count * 4);
      for (i = 0; i < count; i++)
        data[i] = GUINT32_FROM_LE (data[i]);
    }

  return total * 4;
}


static gint
ico_read_int16 (FILE    *fp,
                guint16 *data,
                gint     count)
{
  gint i, total;

  total = count;
  if (count > 0)
    {
      ico_read_int8 (fp, (guint8 *) data, count * 2);
      for (i = 0; i < count; i++)
        data[i] = GUINT16_FROM_LE (data[i]);
    }

  return total * 2;
}


static gint
ico_read_int8 (FILE   *fp,
               guint8 *data,
               gint    count)
{
  gint total;
  gint bytes;

  total = count;
  while (count > 0)
    {
      bytes = fread ((gchar *) data, sizeof (gchar), count, fp);
      if (bytes <= 0) /* something bad happened */
        break;

      count -= bytes;
      data += bytes;
    }

  return total;
}


static IcoFileHeader
ico_read_init (FILE *fp)
{
  IcoFileHeader header;

  /* read and check file header */
  if (! ico_read_int16 (fp, &header.reserved, 1)      ||
      ! ico_read_int16 (fp, &header.resource_type, 1) ||
      ! ico_read_int16 (fp, &header.icon_count, 1)    ||
      header.reserved != 0 ||
      (header.resource_type != 1 && header.resource_type != 2))
    {
      header.icon_count = 0;
      return header;
    }

  return header;
}


static gboolean
ico_read_size (FILE        *fp,
               gint32       file_offset,
               IcoLoadInfo *info)
{
  png_structp png_ptr;
  png_infop   info_ptr;
  png_uint_32 w, h;
  gint32      bpp;
  gint32      color_type;
  guint32     magic;

  if (fseek (fp, info->offset + file_offset, SEEK_SET) < 0)
    return FALSE;

  ico_read_int32 (fp, &magic, 1);

  if (magic == ICO_PNG_MAGIC)
    {
      png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL,
                                        NULL);
      if (! png_ptr)
        return FALSE;

      info_ptr = png_create_info_struct (png_ptr);
      if (! info_ptr)
        {
          png_destroy_read_struct (&png_ptr, NULL, NULL);
          return FALSE;
        }

      if (setjmp (png_jmpbuf (png_ptr)))
        {
          png_destroy_read_struct (&png_ptr, NULL, NULL);
          return FALSE;
        }
      png_init_io (png_ptr, fp);
      png_set_sig_bytes (png_ptr, 4);
      png_read_info (png_ptr, info_ptr);
      png_get_IHDR (png_ptr, info_ptr, &w, &h, &bpp, &color_type,
                    NULL, NULL, NULL);
      png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
      info->width = w;
      info->height = h;
      D(("ico_read_size: PNG: %ix%i\n", info->width, info->height));
      return TRUE;
    }
  else if (magic == 40)
    {
      if (ico_read_int32 (fp, &info->width, 1) &&
          ico_read_int32 (fp, &info->height, 1))
        {
          info->height /= 2;
          D(("ico_read_size: ICO: %ix%i\n", info->width, info->height));
          return TRUE;
        }
      else
        {
          info->width = 0;
          info->height = 0;
          return FALSE;
        }
    }
  return FALSE;
}

static IcoLoadInfo*
ico_read_info (FILE    *fp,
               gint     icon_count,
               gint32   file_offset,
               GError **error)
{
  gint            i;
  IcoFileEntry   *entries;
  IcoLoadInfo    *info;

  /* read icon entries */
  entries = g_new (IcoFileEntry, icon_count);
  if (fread (entries, sizeof (IcoFileEntry), icon_count, fp) <= 0)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Could not read '%lu' bytes"),
                   (long unsigned int) sizeof (IcoFileEntry));
      g_free (entries);
      return NULL;
    }

  info = g_new (IcoLoadInfo, icon_count);
  for (i = 0; i < icon_count; i++)
    {
      info[i].width  = entries[i].width;
      info[i].height = entries[i].height;
      info[i].planes = entries[i].planes;
      info[i].bpp    = GUINT16_FROM_LE (entries[i].bpp);
      info[i].size   = GUINT32_FROM_LE (entries[i].size);
      info[i].offset = GUINT32_FROM_LE (entries[i].offset);

      if (info[i].width == 0 || info[i].height == 0)
        {
          ico_read_size (fp, file_offset, info + i);
        }

      D(("ico_read_info: %ix%i (%i bits, size: %i, offset: %i)\n",
         info[i].width, info[i].height, info[i].bpp,
         info[i].size, info[i].offset));

      if (info[i].width == 0 || info[i].height == 0)
        {
          g_set_error (error, G_FILE_ERROR, 0,
                       _("Icon #%d has zero width or height"), i);
          g_free (info);
          g_free (entries);
          return NULL;
        }
    }

  g_free (entries);

  return info;
}

static gboolean
ico_read_png (FILE    *fp,
              guint32  header,
              guchar  *buf,
              gint     maxsize,
              gint    *width,
              gint    *height)
{
  png_structp   png_ptr;
  png_infop     info;
  png_uint_32   w;
  png_uint_32   h;
  gint32        bit_depth;
  gint32        color_type;
  guint32     **rows;
  gint          i;

  png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (! png_ptr)
    return FALSE;
  info = png_create_info_struct (png_ptr);
  if (! info)
    {
      png_destroy_read_struct (&png_ptr, NULL, NULL);
      return FALSE;
    }

  if (setjmp (png_jmpbuf (png_ptr)))
    {
      png_destroy_read_struct (&png_ptr, &info, NULL);
      return FALSE;
    }

  png_init_io (png_ptr, fp);
  png_set_sig_bytes (png_ptr, 4);
  png_read_info (png_ptr, info);
  png_get_IHDR (png_ptr, info, &w, &h, &bit_depth, &color_type,
                NULL, NULL, NULL);
  /* Check for overflow */
  if ((w * h * 4) < w       ||
      (w * h * 4) < h       ||
      (w * h * 4) < (w * h) ||
      (w * h * 4) > maxsize)
    {
      png_destroy_read_struct (&png_ptr, &info, NULL);
      return FALSE;
    }
  D(("ico_read_png: %ix%i, %i bits, %i type\n", (gint)w, (gint)h,
     bit_depth, color_type));
  switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
      png_set_expand_gray_1_2_4_to_8 (png_ptr);
      if (bit_depth == 16)
        png_set_strip_16 (png_ptr);
      png_set_gray_to_rgb (png_ptr);
      png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);
      break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
      png_set_expand_gray_1_2_4_to_8 (png_ptr);
      if (bit_depth == 16)
        png_set_strip_16 (png_ptr);
      png_set_gray_to_rgb (png_ptr);
      break;
    case PNG_COLOR_TYPE_PALETTE:
      png_set_palette_to_rgb (png_ptr);
      if (png_get_valid (png_ptr, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha (png_ptr);
      else
        png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);
      break;
    case PNG_COLOR_TYPE_RGB:
      if (bit_depth == 16)
        png_set_strip_16 (png_ptr);
      png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);
      break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
      if (bit_depth == 16)
        png_set_strip_16 (png_ptr);
      break;
    }

  *width = w;
  *height = h;
  rows = g_new (guint32*, h);
  rows[0] = (guint32*) buf;
  for (i = 1; i < h; i++)
    rows[i] = rows[i-1] + w;
  png_read_image (png_ptr, (png_bytepp) rows);
  png_destroy_read_struct (&png_ptr, &info, NULL);
  g_free (rows);
  return TRUE;
}

gint
ico_get_bit_from_data (const guint8 *data,
                       gint          line_width,
                       gint          bit)
{
  gint line;
  gint width32;
  gint offset;
  gint result;

  /* width per line in multiples of 32 bits */
  width32 = (line_width % 32 == 0 ? line_width/32 : line_width/32 + 1);
  line = bit / line_width;
  offset = bit % line_width;

  result = (data[line * width32 * 4 + offset/8] & (1 << (7 - (offset % 8))));

  return (result ? 1 : 0);
}


gint
ico_get_nibble_from_data (const guint8 *data,
                          gint          line_width,
                          gint          nibble)
{
  gint line;
  gint width32;
  gint offset;
  gint result;

  /* width per line in multiples of 32 bits */
  width32 = (line_width % 8 == 0 ? line_width/8 : line_width/8 + 1);
  line = nibble / line_width;
  offset = nibble % line_width;

  result =
    (data[line * width32 * 4 + offset/2] & (0x0F << (4 * (1 - offset % 2))));

  if (offset % 2 == 0)
    result = result >> 4;

  return result;
}

gint
ico_get_byte_from_data (const guint8 *data,
                        gint          line_width,
                        gint          byte)
{
  gint line;
  gint width32;
  gint offset;

  /* width per line in multiples of 32 bits */
  width32 = (line_width % 4 == 0 ? line_width / 4 : line_width / 4 + 1);
  line = byte / line_width;
  offset = byte % line_width;

  return data[line * width32 * 4 + offset];
}

static gboolean
ico_read_icon (FILE    *fp,
               guint32  header_size,
               guchar  *buf,
               gint     maxsize,
               gint    *width,
               gint    *height)
{
  IcoFileDataHeader   data;
  gint                length;
  gint                x, y, w, h;
  guchar             *xor_map, *and_map;
  guint32            *palette;
  guint32            *dest_vec;
  guchar             *row;
  gint                rowstride;

  palette = NULL;

  data.header_size = header_size;
  ico_read_int32 (fp, &data.width, 1);
  ico_read_int32 (fp, &data.height, 1);
  ico_read_int16 (fp, &data.planes, 1);
  ico_read_int16 (fp, &data.bpp, 1);
  ico_read_int32 (fp, &data.compression, 1);
  ico_read_int32 (fp, &data.image_size, 1);
  ico_read_int32 (fp, &data.x_res, 1);
  ico_read_int32 (fp, &data.y_res, 1);
  ico_read_int32 (fp, &data.used_clrs, 1);
  ico_read_int32 (fp, &data.important_clrs, 1);

  D(("  header size %i, "
     "w %i, h %i, planes %i, size %i, bpp %i, used %i, imp %i.\n",
     data.header_size, data.width, data.height,
     data.planes, data.image_size, data.bpp,
     data.used_clrs, data.important_clrs));

  if (data.planes      != 1 ||
      data.compression != 0)
    {
      D(("skipping image: invalid header\n"));
      return FALSE;
    }

  if (data.bpp != 1  &&
      data.bpp != 4  &&
      data.bpp != 8  &&
      data.bpp != 24 &&
      data.bpp != 32)
    {
      D(("skipping image: invalid depth: %i\n", data.bpp));
      return FALSE;
    }

  if (data.width * data.height * 2 > maxsize)
    {
      D(("skipping image: too large\n"));
      return FALSE;
    }

  w = data.width;
  h = data.height / 2;

  if (data.bpp <= 8)
    {
      if (data.used_clrs == 0)
        data.used_clrs = (1 << data.bpp);

      D(("  allocating a %i-slot palette for %i bpp.\n",
         data.used_clrs, data.bpp));

      palette = g_new0 (guint32, data.used_clrs);
      ico_read_int8 (fp, (guint8 *) palette, data.used_clrs * 4);
    }

  xor_map = ico_alloc_map (w, h, data.bpp, &length);
  ico_read_int8 (fp, xor_map, length);
  D(("  length of xor_map: %i\n", length));

  /* Read in and_map. It's padded out to 32 bits per line: */
  and_map = ico_alloc_map (w, h, 1, &length);
  ico_read_int8 (fp, and_map, length);
  D(("  length of and_map: %i\n", length));

  dest_vec = (guint32 *) buf;
  switch (data.bpp)
    {
    case 1:
      for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
          {
            guint32 color = palette[ico_get_bit_from_data (xor_map,
                                                           w, y * w + x)];
            guint32 *dest = dest_vec + (h - 1 - y) * w + x;

            R_VAL_GIMP (dest) = R_VAL (&color);
            G_VAL_GIMP (dest) = G_VAL (&color);
            B_VAL_GIMP (dest) = B_VAL (&color);

            if (ico_get_bit_from_data (and_map, w, y * w + x))
              A_VAL_GIMP (dest) = 0;
            else
              A_VAL_GIMP (dest) = 255;
          }
      break;

    case 4:
      for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
          {
            guint32 color = palette[ico_get_nibble_from_data (xor_map,
                                                              w, y * w + x)];
            guint32 *dest = dest_vec + (h - 1 - y) * w + x;

            R_VAL_GIMP (dest) = R_VAL (&color);
            G_VAL_GIMP (dest) = G_VAL (&color);
            B_VAL_GIMP (dest) = B_VAL (&color);

            if (ico_get_bit_from_data (and_map, w, y * w + x))
              A_VAL_GIMP (dest) = 0;
            else
              A_VAL_GIMP (dest) = 255;
          }
      break;

    case 8:
      for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
          {
            guint32 color = palette[ico_get_byte_from_data (xor_map,
                                                            w, y * w + x)];
            guint32 *dest = dest_vec + (h - 1 - y) * w + x;

            R_VAL_GIMP (dest) = R_VAL (&color);
            G_VAL_GIMP (dest) = G_VAL (&color);
            B_VAL_GIMP (dest) = B_VAL (&color);

            if (ico_get_bit_from_data (and_map, w, y * w + x))
              A_VAL_GIMP (dest) = 0;
            else
              A_VAL_GIMP (dest) = 255;
          }
      break;

    default:
      {
        gint bytespp = data.bpp / 8;

        rowstride = ico_rowstride (w, data.bpp);

        for (y = 0; y < h; y++)
          {
            row = xor_map + rowstride * y;

            for (x = 0; x < w; x++)
              {
                guint32 *dest = dest_vec + (h - 1 - y) * w + x;

                B_VAL_GIMP (dest) = row[0];
                G_VAL_GIMP (dest) = row[1];
                R_VAL_GIMP (dest) = row[2];

                if (data.bpp < 32)
                  {
                    if (ico_get_bit_from_data (and_map, w, y * w + x))
                      A_VAL_GIMP (dest) = 0;
                    else
                      A_VAL_GIMP (dest) = 255;
                  }
                else
                  {
                    A_VAL_GIMP (dest) = row[3];
                  }

                row += bytespp;
              }
          }
      }
    }
  if (palette)
    g_free (palette);
  g_free (xor_map);
  g_free (and_map);
  *width = w;
  *height = h;
  return TRUE;
}

static GimpLayer *
ico_load_layer (FILE        *fp,
                GimpImage   *image,
                gint32       icon_num,
                guchar      *buf,
                gint         maxsize,
                gint32       file_offset,
                gchar       *layer_prefix,
                IcoLoadInfo *info)
{
  gint        width, height;
  GimpLayer  *layer;
  guint32     first_bytes;
  GeglBuffer *buffer;

  if (fseek (fp, info->offset + file_offset, SEEK_SET) < 0 ||
      ! ico_read_int32 (fp, &first_bytes, 1))
    return NULL;

  if (first_bytes == ICO_PNG_MAGIC)
    {
      if (!ico_read_png (fp, first_bytes, buf, maxsize, &width, &height))
        return NULL;
    }
  else if (first_bytes == 40)
    {
      if (!ico_read_icon (fp, first_bytes, buf, maxsize, &width, &height))
        return NULL;
    }
  else
    {
      return NULL;
    }

  /* read successfully. add to image */
  layer = gimp_layer_new (image, layer_prefix, width, height,
                          GIMP_RGBA_IMAGE, 100,
                          gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, layer, NULL, icon_num);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL, buf, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);

  return layer;
}


GimpImage *
ico_load_image (GFile        *file,
                gint32       *file_offset,
                gint          frame_num,
                GError      **error)
{
  FILE          *fp;
  IcoFileHeader  header;
  IcoLoadInfo   *info;
  gint           max_width, max_height;
  gint           i;
  GimpImage     *image;
  guchar        *buf;
  guint          icon_count;
  gint           maxsize;
  gchar         *str;

  if (! file_offset)
    gimp_progress_init_printf (_("Opening '%s'"),
                               gimp_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  if (file_offset)
    fseek (fp, *file_offset, SEEK_SET);

  header = ico_read_init (fp);
  icon_count = header.icon_count;
  if (!icon_count)
    {
      fclose (fp);
      return NULL;
    }

  info = ico_read_info (fp, icon_count, file_offset ? *file_offset : 0, error);
  if (! info)
    {
      fclose (fp);
      return NULL;
    }

  /* find width and height of image */
  max_width = 0;
  max_height = 0;
  for (i = 0; i < icon_count; i++)
    {
      if (info[i].width > max_width)
        max_width = info[i].width;
      if (info[i].height > max_height)
        max_height = info[i].height;
    }
  if (max_width <= 0 || max_height <= 0)
    {
      g_free (info);
      fclose (fp);
      return NULL;
    }
  D(("image size: %ix%i\n", max_width, max_height));

  image = gimp_image_new (max_width, max_height, GIMP_RGB);

  maxsize = max_width * max_height * 4;
  buf = g_new (guchar, max_width * max_height * 4);
  for (i = 0; i < icon_count; i++)
    {
      GimpLayer *layer;
      gchar     *layer_prefix;
      gchar     *icon_metadata;

      if (info[i].bpp)
        icon_metadata = g_strdup_printf ("(%dx%d, %dbpp)", info[i].width,
                                         info[i].height, info[i].bpp);
      else
        icon_metadata = g_strdup_printf ("(%dx%d)", info[i].width,
                                         info[i].height);

      if (frame_num > -1)
        {
          layer_prefix = g_strdup_printf ("Cursor %s Frame #%i", icon_metadata,
                                          frame_num);
        }
      else
        {
          if (header.resource_type == 1)
            layer_prefix = g_strdup_printf ("Icon #%i %s ", i + 1,
                                            icon_metadata);
          else
            layer_prefix = g_strdup_printf ("Cursor #%i %s ", i + 1,
                                            icon_metadata);
        }

      layer = ico_load_layer (fp, image, i + 1, buf, maxsize,
                              file_offset ? *file_offset : 0,
                              layer_prefix, info + i);
      g_free (icon_metadata);
      g_free (layer_prefix);

      /* Save CUR hot spot information */
      if (header.resource_type == 2)
        {
          GimpParasite *parasite;

          str = g_strdup_printf ("%d %d", info[i].planes, info[i].bpp);
          parasite = gimp_parasite_new ("cur-hot-spot",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (str) + 1, (gpointer) str);
          g_free (str);
          gimp_item_attach_parasite (GIMP_ITEM (layer), parasite);
          gimp_parasite_free (parasite);
        }
    }

  if (file_offset)
    *file_offset = ftell (fp);

  g_free (buf);
  g_free (info);
  fclose (fp);

  /* Don't update progress here if .ani file */
  if (! file_offset)
    gimp_progress_update (1.0);

  return image;
}

/* Ported from James Huang's ani.c code, under the GPL license, version 3
 * or any later version of the license */
GimpImage *
ani_load_image (GFile   *file,
                gboolean load_thumb,
                gint    *width,
                gint    *height,
                GError **error)
{
  FILE         *fp;
  GimpImage    *image = NULL;
  GimpParasite *parasite;
  gchar         id[4];
  guint32       size;
  guint8        padding;
  gint32        file_offset;
  guint         frame = 1;
  AniFileHeader header;
  gchar        *inam  = NULL;
  gchar        *iart  = NULL;
  gchar        *str;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  while (fread (id, 1, 4, fp) == 4)
    {
      if (memcmp (id, "RIFF", 4) == 0 )
        {
          fread (&size, sizeof (size), 1, fp);
        }
      else if (memcmp (id, "anih", 4) == 0)
        {
          fread (&size, sizeof (size), 1, fp);
          fread (&header, sizeof (header), 1, fp);
        }
      else if (memcmp (id, "rate", 4) == 0)
        {
          fread (&size, sizeof (size), 1, fp);
          fseek (fp, size, SEEK_CUR);
        }
      else if (memcmp (id, "seq ", 4) == 0)
        {
          fread (&size, sizeof (size), 1, fp);
          fseek (fp, size, SEEK_CUR);
        }
      else if (memcmp (id, "LIST", 4) == 0)
        {
          fread (&size, sizeof (size), 1, fp);
        }
      else if (memcmp (id, "INAM", 4) == 0)
        {
          gint n_read = -1;

          fread (&size, sizeof (size), 1, fp);
          if (size > 0)
            {
              if (inam)
                g_free (inam);

              inam = g_new0 (gchar, size + 1);
              n_read = fread (inam, sizeof (gchar), size, fp);
              inam[size] = '\0';
            }

          if (n_read < 1 || (inam && ! g_utf8_validate (inam, -1, NULL)))
            {
              fclose (fp);
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Invalid ANI metadata"));
              return NULL;
            }

          /* Metadata length must be even. If data itself is odd,
           * then an extra 0x00 is added for padding. We read in
           * that extra byte to keep loading properly.
           * See discussion in #8562.
           */
          if (size % 2 != 0)
            fread (&padding, sizeof (padding), 1, fp);
        }
      else if (memcmp (id, "IART", 4) == 0)
        {
          gint n_read = -1;

          fread (&size, sizeof (size), 1, fp);
          if (size > 0)
            {
              if (iart)
                g_free (iart);

              iart = g_new0 (gchar, size + 1);
              n_read = fread (iart, sizeof (gchar), size, fp);
              iart[size] = '\0';
            }

          if (n_read < 1 || (iart && ! g_utf8_validate (iart, -1, NULL)))
            {
              fclose (fp);
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Invalid ANI metadata"));
              return NULL;
            }

          if (size % 2 != 0)
            fread (&padding, sizeof (padding), 1, fp);
        }
      else if (memcmp (id, "icon", 4) == 0)
        {
          fread (&size, sizeof (size), 1, fp);
          file_offset = ftell (fp);
          if (load_thumb)
            {
              image = ico_load_thumbnail_image (file, width, height, file_offset, error);
              break;
            }
           else
             {
               if (! image)
                 {
                   image = ico_load_image (file, &file_offset, 1, error);
                 }
               else
                 {
                   GimpImage    *temp_image = NULL;
                   GimpLayer   **layers;
                   GimpLayer    *new_layer;

                   temp_image = ico_load_image (file, &file_offset, frame + 1,
                                                error);
                   layers = gimp_image_get_layers (temp_image);
                   if (layers)
                     {
                       for (gint i = 0; layers[i]; i++)
                         {
                           new_layer = gimp_layer_new_from_drawable (GIMP_DRAWABLE (layers[i]),
                                                                     image);
                           gimp_image_insert_layer (image, new_layer, NULL, frame);
                           frame++;
                         }
                       g_free (layers);
                     }
                   gimp_image_delete (temp_image);
                 }

               /* Update position after reading icon data */
               fseek (fp, file_offset, SEEK_SET);
               if (header.frames > 0)
                 gimp_progress_update ((gdouble) frame /
                                       (gdouble) header.frames);
             }
        }
    }
  fclose (fp);

  /* Saving header metadata */
  str = g_strdup_printf ("%d", header.jif_rate);
  parasite = gimp_parasite_new ("ani-header",
                                GIMP_PARASITE_PERSISTENT,
                                strlen (str) + 1, (gpointer) str);
  g_free (str);
  gimp_image_attach_parasite (image, parasite);
  gimp_parasite_free (parasite);

  /* Saving INFO block */
  if (inam && strlen (inam) > 0)
    {
      str = g_strdup_printf ("%s", inam);
      parasite = gimp_parasite_new ("ani-info-inam",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (str) + 1, (gpointer) str);
      g_free (str);
      g_free (inam);
      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);
    }

  if (iart && strlen (iart) > 0)
    {
      str = g_strdup_printf ("%s", iart);
      parasite = gimp_parasite_new ("ani-info-iart",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (str) + 1, (gpointer) str);
      g_free (str);
      g_free (iart);
      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);
    }

  gimp_progress_update (1.0);

  return image;
}

GimpImage *
ico_load_thumbnail_image (GFile   *file,
                          gint    *width,
                          gint    *height,
                          gint32   file_offset,
                          GError **error)
{
  FILE          *fp;
  IcoLoadInfo   *info;
  IcoFileHeader  header;
  GimpImage     *image;
  gint           max_width;
  gint           max_height;
  gint           w     = 0;
  gint           h     = 0;
  gint           bpp   = 0;
  gint           match = 0;
  gint           i, icon_count;
  guchar        *buf;

  gimp_progress_init_printf (_("Opening thumbnail for '%s'"),
                             gimp_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  if (file_offset > 0)
    fseek (fp, file_offset, SEEK_SET);

  header = ico_read_init (fp);
  icon_count = header.icon_count;
  if (! icon_count)
    {
      fclose (fp);
      return NULL;
    }

  D(("*** %s: Microsoft icon file, containing %i icon(s)\n",
     gimp_file_get_utf8_name (file), icon_count));

  info = ico_read_info (fp, icon_count, file_offset, error);
  if (! info)
    {
      fclose (fp);
      return NULL;
    }

  max_width  = 0;
  max_height = 0;

  /* Do a quick scan of the icons in the file to find the best match */
  for (i = 0; i < icon_count; i++)
    {
      if (info[i].width > max_width)
        max_width = info[i].width;
      if (info[i].height > max_height)
        max_height = info[i].height;

      if ((info[i].width  > w && w < *width) ||
          (info[i].height > h && h < *height))
        {
          w = info[i].width;
          h = info[i].height;
          bpp = info[i].bpp;

          match = i;
        }
      else if (w == info[i].width  &&
               h == info[i].height &&
               info[i].bpp > bpp)
        {
          /* better quality */
          bpp = info[i].bpp;
          match = i;
        }
    }

  if (w <= 0 || h <= 0)
    return NULL;

  image = gimp_image_new (w, h, GIMP_RGB);
  buf = g_new (guchar, w*h*4);
  ico_load_layer (fp, image, match, buf, w*h*4, file_offset,
                  "Thumbnail", info + match);
  g_free (buf);

  *width  = max_width;
  *height = max_height;

  D(("*** thumbnail successfully loaded.\n\n"));

  gimp_progress_update (1.0);

  g_free (info);
  fclose (fp);

  return image;
}
