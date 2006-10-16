/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

/* #define ICO_DBG */

#include "main.h"
#include "icoload.h"

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


static guint32
ico_read_init (FILE *fp)
{
  IcoFileHeader header;
  /* read and check file header */
  if (!ico_read_int16 (fp, &header.reserved, 1)
      || !ico_read_int16 (fp, &header.resource_type, 1)
      || !ico_read_int16 (fp, &header.icon_count, 1)
      || header.reserved != 0
      || header.resource_type != 1)
    {
      return 0;
    }
  return header.icon_count;
}


gboolean
ico_read_size (FILE        *fp,
               IcoLoadInfo *info)
{
  guint32     magic;

  if ( fseek (fp, info->offset, SEEK_SET) < 0 )
    return FALSE;

  ico_read_int32 (fp, &magic, 1);
  if (magic == 40)
    {
      if (ico_read_int32 (fp, &info->width, 1)
          && ico_read_int32 (fp, &info->height, 1))
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
ico_read_info (FILE *fp,
               gint  icon_count)
{
  gint            i;
  IcoFileEntry   *entries;
  IcoLoadInfo    *info;

  /* read icon entries */
  entries = g_new (IcoFileEntry, icon_count);
  if ( fread (entries, sizeof(IcoFileEntry), icon_count, fp) <= 0 )
    {
      g_free (entries);
      return NULL;
    }

  info = g_new (IcoLoadInfo, icon_count);
  for (i = 0; i < icon_count; i++)
    {
      info[i].width = entries[i].width;
      info[i].height = entries[i].height;
      info[i].bpp = GUINT16_FROM_LE (entries[i].bpp);
      info[i].size = GUINT32_FROM_LE (entries[i].size);
      info[i].offset = GUINT32_FROM_LE (entries[i].offset);

      if (info[i].width == 0 || info[i].height == 0)
        {
          ico_read_size (fp, info+i);
        }

      D(("ico_read_info: %ix%i (%i bits, size: %i, offset: %i)\n",
         info[i].width, info[i].height, info[i].bpp,
         info[i].size, info[i].offset));
    }

  g_free (entries);
  return info;
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
               guchar  *buffer,
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

  if (data.planes != 1
      || data.compression != 0)
    {
      D(("skipping image: invalid header\n"));
      return FALSE;
    }

  if (data.bpp != 1 && data.bpp != 4
      && data.bpp != 8 && data.bpp != 24
      && data.bpp != 32)
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

  dest_vec = (guint32 *) buffer;
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

gint32
ico_load_layer (FILE        *fp,
                gint32       image,
                gint32       icon_num,
                guchar      *buffer,
                gint         maxsize,
                IcoLoadInfo *info)
{
  gint          width, height;
  gint32        layer;
  guint32       first_bytes;
  GimpDrawable *drawable;
  GimpPixelRgn  pixel_rgn;
  gchar         buf [ICO_MAXBUF];

  if ( fseek (fp, info->offset, SEEK_SET) < 0
       || !ico_read_int32 (fp, &first_bytes, 1) )
    return -1;

  if (first_bytes == 40)
    {
      if (!ico_read_icon (fp, first_bytes, buffer, maxsize, &width, &height))
        return -1;
    }
  else
    {
      return -1;
    }


  /* read successfully. add to image */
  g_snprintf (buf, sizeof (buf), _("Icon #%i"), icon_num+1);
  layer = gimp_layer_new (image, buf, width, height,
                          GIMP_RGBA_IMAGE, 100, GIMP_NORMAL_MODE);
  gimp_image_add_layer (image, layer, icon_num);
  drawable = gimp_drawable_get (layer);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                       drawable->width, drawable->height, TRUE, FALSE);
  gimp_pixel_rgn_set_rect (&pixel_rgn, (const guchar*) buffer,
                           0, 0, drawable->width,
                           drawable->height);
  gimp_drawable_detach (drawable);

  return layer;
}


gint32
ico_load_image (const gchar *filename)
{
  FILE        *fp;
  IcoLoadInfo *info;
  gint         max_width, max_height;
  gint         i;
  gint32       image;
  gint32       layer;
  guchar      *buffer;
  guint        icon_count;
  gint         maxsize;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  fp = g_fopen (filename, "rb");
  if ( !fp )
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  icon_count = ico_read_init (fp);
  if (!icon_count)
    {
      fclose (fp);
      return -1;
    }

  info = ico_read_info (fp, icon_count);
  if (!info)
    {
      fclose (fp);
      return -1;
    }

  /* find width and height of image */
  max_width = 0;
  max_height = 0;
  for (i = 0; i < icon_count; i++)
    {
      if ( info[i].width > max_width )
        max_width = info[i].width;
      if ( info[i].height > max_height )
        max_height = info[i].height;
    }
  if ( max_width <= 0 || max_height <= 0 )
    {
      g_free (info);
      fclose (fp);
      return -1;
    }
  D(("image size: %ix%i\n", max_width, max_height));

  image = gimp_image_new (max_width, max_height, GIMP_RGB);
  gimp_image_set_filename (image, filename);

  maxsize = max_width * max_height * 4;
  buffer = g_new (guchar, max_width * max_height * 4);
  for (i = 0; i < icon_count; i++)
    {
      layer = ico_load_layer (fp, image, i, buffer, maxsize, info+i);
    }
  g_free (buffer);
  g_free (info);
  fclose (fp);

  gimp_progress_update (1.0);

  return image;
}

gint32
ico_load_thumbnail_image (const gchar *filename,
                          gint        *width,
                          gint        *height)
{
  FILE        *fp;
  IcoLoadInfo *info;
  gint32       image;
  gint32       layer;
  gint         w     = 0;
  gint         h     = 0;
  gint         bpp   = 0;
  gint         match = 0;
  gint         i, icon_count;
  guchar      *buffer;

  gimp_progress_init_printf (_("Opening thumbnail for '%s'"),
                             gimp_filename_to_utf8 (filename));

  fp = g_fopen (filename, "rb");
  if ( !fp )
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  icon_count = ico_read_init (fp);
  if ( !icon_count )
    {
      fclose (fp);
      return -1;
    }

  D(("*** %s: Microsoft icon file, containing %i icon(s)\n",
     filename, icon_count));

  info = ico_read_info (fp, icon_count);
  if ( !info )
    {
      fclose (fp);
      return -1;
    }

  /* Do a quick scan of the icons in the file to find the best match */
  for (i = 0; i < icon_count; i++)
    {
      if ((info[i].width  > w && w < *width) ||
          (info[i].height > h && h < *height))
        {
          w = info[i].width;
          h = info[i].height;
          bpp = info[i].bpp;

          match = i;
        }
      else if ( w == info[i].width
                && h == info[i].height
                && info[i].bpp > bpp )
        {
          /* better quality */
          bpp = info[i].bpp;
          match = i;
        }
    }

  if (w <= 0 || h <= 0)
    return -1;

  image = gimp_image_new (w, h, GIMP_RGB);
  buffer = g_new (guchar, w*h*4);
  layer = ico_load_layer (fp, image, match, buffer, w*h*4, info+match);
  g_free (buffer);

  *width  = w;
  *height = h;

  D(("*** thumbnail successfully loaded.\n\n"));

  gimp_progress_update (1.0);

  g_free (info);
  fclose (fp);

  return image;
}
