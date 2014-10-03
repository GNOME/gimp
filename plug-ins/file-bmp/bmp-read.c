/* bmpread.c    reads any bitmap I could get for testing */
/* Alexander.Schulz@stud.uni-karlsruhe.de                */

/*
 * GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>

#include "bmp.h"

#include "libgimp/stdplugins-intl.h"


#if !defined(WIN32) || defined(__MINGW32__)
#define BI_RGB            0
#define BI_RLE8           1
#define BI_RLE4           2
#define BI_BITFIELDS      3
#define BI_ALPHABITFIELDS 4
#endif

static gint32 ReadImage (FILE                  *fd,
                         gint                   width,
                         gint                   height,
                         guchar                 cmap[256][3],
                         gint                   ncols,
                         gint                   bpp,
                         gint                   compression,
                         gint                   rowbytes,
                         gboolean               grey,
                         const Bitmap_Channel  *masks,
                         GError               **error);


static void
setMasksDefault (gushort biBitCnt, Bitmap_Channel *masks)
{
  switch (biBitCnt)
    {
    case 32:
      masks[0].mask     = 0x00ff0000;
      masks[0].shiftin  = 16;
      masks[0].max_value= (gfloat)255.0;
      masks[1].mask     = 0x0000ff00;
      masks[1].shiftin  = 8;
      masks[1].max_value= (gfloat)255.0;
      masks[2].mask     = 0x000000ff;
      masks[2].shiftin  = 0;
      masks[2].max_value= (gfloat)255.0;
      masks[3].mask     = 0x00000000;
      masks[3].shiftin  = 0;
      masks[3].max_value= (gfloat)0.0;
      break;
    case 24:
      masks[0].mask     = 0xff0000;
      masks[0].shiftin  = 16;
      masks[0].max_value= (gfloat)255.0;
      masks[1].mask     = 0x00ff00;
      masks[1].shiftin  = 8;
      masks[1].max_value= (gfloat)255.0;
      masks[2].mask     = 0x0000ff;
      masks[2].shiftin  = 0;
      masks[2].max_value= (gfloat)255.0;
      masks[3].mask     = 0x0;
      masks[3].shiftin  = 0;
      masks[3].max_value= (gfloat)0.0;
      break;
    case 16:
      masks[0].mask     = 0x7c00;
      masks[0].shiftin  = 10;
      masks[0].max_value= (gfloat)31.0;
      masks[1].mask     = 0x03e0;
      masks[1].shiftin  = 5;
      masks[1].max_value= (gfloat)31.0;
      masks[2].mask     = 0x001f;
      masks[2].shiftin  = 0;
      masks[2].max_value= (gfloat)31.0;
      masks[3].mask     = 0x0;
      masks[3].shiftin  = 0;
      masks[3].max_value= (gfloat)0.0;
      break;
    default:
      break;
    }
}

static gint32
ToL (const guchar *puffer)
{
  return (puffer[0] | puffer[1] << 8 | puffer[2] << 16 | puffer[3] << 24);
}

static gint16
ToS (const guchar *puffer)
{
  return (puffer[0] | puffer[1] << 8);
}

static gboolean
ReadColorMap (FILE     *fd,
              guchar    buffer[256][3],
              gint      number,
              gint      size,
              gboolean *grey)
{
  gint   i;
  guchar rgb[4];

  *grey = (number > 2);

  for (i = 0; i < number ; i++)
    {
      if (!ReadOK (fd, rgb, size))
        {
          g_message (_("Bad colormap"));
          return FALSE;
        }

      /* Bitmap save the colors in another order! But change only once! */

      buffer[i][0] = rgb[2];
      buffer[i][1] = rgb[1];
      buffer[i][2] = rgb[0];
      *grey = ((*grey) && (rgb[0]==rgb[1]) && (rgb[1]==rgb[2]));
    }

  return TRUE;
}

static gboolean
ReadChannelMasks (guint32 *tmp, Bitmap_Channel *masks, guint channels)
{
  guint32 mask;
  gint   i, nbits, offset, bit;

  for (i = 0; i < channels; i++)
    {
      mask = tmp[i];
      masks[i].mask = mask;
      nbits = 0;
      offset = -1;
      for (bit = 0; bit < 32; bit++)
        {
          if (mask & 1)
            {
              nbits++;
              if (offset == -1)
                offset = bit;
            }
          mask = mask >> 1;
        }
      masks[i].shiftin = offset;
      masks[i].max_value = (gfloat)((1<<(nbits))-1);

#ifdef DEBUG
      g_print ("Channel %d mask %08x in %d max_val %d\n",
               i, masks[i].mask, masks[i].shiftin, (gint)masks[i].max_value);
#endif
    }

  return TRUE;
}

gint32
ReadBMP (const gchar  *name,
         GError      **error)
{
  FILE     *fd;
  guchar    buffer[124];
  gint      ColormapSize, rowbytes, Maps;
  gboolean  Grey = FALSE;
  guchar    ColorMap[256][3];
  gint32    image_ID = -1;
  gchar     magick[2];
  Bitmap_Channel masks[4];

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (name));

  filename = name;
  fd = g_fopen (filename, "rb");

  if (!fd)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      goto out;
    }

  /* It is a File. Now is it a Bitmap? Read the shortest possible header */

  if (!ReadOK (fd, magick, 2) || !(!strncmp (magick, "BA", 2) ||
     !strncmp (magick, "BM", 2) || !strncmp (magick, "IC", 2) ||
     !strncmp (magick, "PT", 2) || !strncmp (magick, "CI", 2) ||
     !strncmp (magick, "CP", 2)))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a valid BMP file"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  while (!strncmp (magick, "BA", 2))
    {
      if (!ReadOK (fd, buffer, 12))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("'%s' is not a valid BMP file"),
                       gimp_filename_to_utf8 (filename));
          goto out;
        }
      if (!ReadOK (fd, magick, 2))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("'%s' is not a valid BMP file"),
                       gimp_filename_to_utf8 (filename));
          goto out;
        }
    }

  if (!ReadOK (fd, buffer, 12))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a valid BMP file"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  /* bring them to the right byteorder. Not too nice, but it should work */

  Bitmap_File_Head.bfSize    = ToL (&buffer[0x00]);
  Bitmap_File_Head.zzHotX    = ToS (&buffer[0x04]);
  Bitmap_File_Head.zzHotY    = ToS (&buffer[0x06]);
  Bitmap_File_Head.bfOffs    = ToL (&buffer[0x08]);

  if (!ReadOK (fd, buffer, 4))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a valid BMP file"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  Bitmap_File_Head.biSize    = ToL (&buffer[0x00]);

  /* What kind of bitmap is it? */

  if (Bitmap_File_Head.biSize == 12) /* OS/2 1.x ? */
    {
      if (!ReadOK (fd, buffer, 8))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading BMP file header from '%s'"),
                       gimp_filename_to_utf8 (filename));
          goto out;
        }

      Bitmap_Head.biWidth   = ToS (&buffer[0x00]);       /* 12 */
      Bitmap_Head.biHeight  = ToS (&buffer[0x02]);       /* 14 */
      Bitmap_Head.biPlanes  = ToS (&buffer[0x04]);       /* 16 */
      Bitmap_Head.biBitCnt  = ToS (&buffer[0x06]);       /* 18 */
      Bitmap_Head.biCompr   = 0;
      Bitmap_Head.biSizeIm  = 0;
      Bitmap_Head.biXPels   = Bitmap_Head.biYPels = 0;
      Bitmap_Head.biClrUsed = 0;
      Bitmap_Head.biClrImp  = 0;
      Bitmap_Head.masks[0]  = 0;
      Bitmap_Head.masks[1]  = 0;
      Bitmap_Head.masks[2]  = 0;
      Bitmap_Head.masks[3]  = 0;

      memset(masks, 0, sizeof(masks));
      Maps = 3;
    }
  else if (Bitmap_File_Head.biSize == 40) /* Windows 3.x */
    {
      if (!ReadOK (fd, buffer, 36))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading BMP file header from '%s'"),
                       gimp_filename_to_utf8 (filename));
          goto out;
        }

      Bitmap_Head.biWidth   = ToL (&buffer[0x00]);      /* 12 */
      Bitmap_Head.biHeight  = ToL (&buffer[0x04]);      /* 16 */
      Bitmap_Head.biPlanes  = ToS (&buffer[0x08]);      /* 1A */
      Bitmap_Head.biBitCnt  = ToS (&buffer[0x0A]);      /* 1C */
      Bitmap_Head.biCompr   = ToL (&buffer[0x0C]);      /* 1E */
      Bitmap_Head.biSizeIm  = ToL (&buffer[0x10]);      /* 22 */
      Bitmap_Head.biXPels   = ToL (&buffer[0x14]);      /* 26 */
      Bitmap_Head.biYPels   = ToL (&buffer[0x18]);      /* 2A */
      Bitmap_Head.biClrUsed = ToL (&buffer[0x1C]);      /* 2E */
      Bitmap_Head.biClrImp  = ToL (&buffer[0x20]);      /* 32 */
      Bitmap_Head.masks[0]  = 0;
      Bitmap_Head.masks[1]  = 0;
      Bitmap_Head.masks[2]  = 0;
      Bitmap_Head.masks[3]  = 0;

      Maps = 4;
      memset(masks, 0, sizeof(masks));

      if (Bitmap_Head.biCompr == BI_BITFIELDS)
        {
#ifdef DEBUG
          g_print ("Got BI_BITFIELDS compression\n");
#endif

          if (!ReadOK (fd, buffer, 3 * sizeof (guint32)))
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Error reading BMP file header from '%s'"),
                           gimp_filename_to_utf8 (filename));
              goto out;
            }

          Bitmap_Head.masks[0] = ToL(&buffer[0x00]);
          Bitmap_Head.masks[1] = ToL(&buffer[0x04]);
          Bitmap_Head.masks[2] = ToL(&buffer[0x08]);
          ReadChannelMasks (&Bitmap_Head.masks[0], masks, 3);
        }
      else if (Bitmap_Head.biCompr == BI_RGB)
        {
#ifdef DEBUG
          g_print ("Got BI_RGB compression\n");
#endif

          setMasksDefault (Bitmap_Head.biBitCnt, masks);
        }
      else if ((Bitmap_Head.biCompr != BI_RLE4) && (Bitmap_Head.biCompr != BI_RLE8))
        {
          /* BI_ALPHABITFIELDS, etc. */
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Unsupported compression (%lu) in BMP file from '%s'"),
                       Bitmap_Head.biCompr,
                       gimp_filename_to_utf8 (filename));
        }

#ifdef DEBUG
      g_print ("Got BI_RLE4 or BI_RLE8 compression\n");
#endif
    }
  else if (Bitmap_File_Head.biSize >= 56 && Bitmap_File_Head.biSize <= 64)
    /* enhanced Windows format with bit masks */
    {
      if (!ReadOK (fd, buffer, Bitmap_File_Head.biSize - 4))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading BMP file header from '%s'"),
                       gimp_filename_to_utf8 (filename));
          goto out;
        }

      Bitmap_Head.biWidth   =ToL (&buffer[0x00]);       /* 12 */
      Bitmap_Head.biHeight  =ToL (&buffer[0x04]);       /* 16 */
      Bitmap_Head.biPlanes  =ToS (&buffer[0x08]);       /* 1A */
      Bitmap_Head.biBitCnt  =ToS (&buffer[0x0A]);       /* 1C */
      Bitmap_Head.biCompr   =ToL (&buffer[0x0C]);       /* 1E */
      Bitmap_Head.biSizeIm  =ToL (&buffer[0x10]);       /* 22 */
      Bitmap_Head.biXPels   =ToL (&buffer[0x14]);       /* 26 */
      Bitmap_Head.biYPels   =ToL (&buffer[0x18]);       /* 2A */
      Bitmap_Head.biClrUsed =ToL (&buffer[0x1C]);       /* 2E */
      Bitmap_Head.biClrImp  =ToL (&buffer[0x20]);       /* 32 */
      Bitmap_Head.masks[0]  =ToL (&buffer[0x24]);       /* 36 */
      Bitmap_Head.masks[1]  =ToL (&buffer[0x28]);       /* 3A */
      Bitmap_Head.masks[2]  =ToL (&buffer[0x2C]);       /* 3E */
      Bitmap_Head.masks[3]  =ToL (&buffer[0x30]);       /* 42 */

      Maps = 4;
      ReadChannelMasks (&Bitmap_Head.masks[0], masks, 4);
    }
  else if (Bitmap_File_Head.biSize == 108 || Bitmap_File_Head.biSize == 124)
    /* BMP Version 4 or 5 */
    {
      if (!ReadOK (fd, buffer, Bitmap_File_Head.biSize - 4))
        {

          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading BMP file header from '%s'"),
                       gimp_filename_to_utf8 (filename));
          goto out;
        }

      Bitmap_Head.biWidth   = ToL (&buffer[0x00]);
      Bitmap_Head.biHeight  = ToL (&buffer[0x04]);
      Bitmap_Head.biPlanes  = ToS (&buffer[0x08]);
      Bitmap_Head.biBitCnt  = ToS (&buffer[0x0A]);
      Bitmap_Head.biCompr   = ToL (&buffer[0x0C]);
      Bitmap_Head.biSizeIm  = ToL (&buffer[0x10]);
      Bitmap_Head.biXPels   = ToL (&buffer[0x14]);
      Bitmap_Head.biYPels   = ToL (&buffer[0x18]);
      Bitmap_Head.biClrUsed = ToL (&buffer[0x1C]);
      Bitmap_Head.biClrImp  = ToL (&buffer[0x20]);
      Bitmap_Head.masks[0]  = ToL (&buffer[0x24]);
      Bitmap_Head.masks[1]  = ToL (&buffer[0x28]);
      Bitmap_Head.masks[2]  = ToL (&buffer[0x2C]);
      Bitmap_Head.masks[3]  = ToL (&buffer[0x30]);

      Maps = 4;

      if (Bitmap_Head.biCompr == BI_BITFIELDS)
        {
#ifdef DEBUG
          g_print ("Got BI_BITFIELDS compression\n");
#endif

          ReadChannelMasks (&Bitmap_Head.masks[0], masks, 4);
        }
      else if (Bitmap_Head.biCompr == BI_RGB)
        {
#ifdef DEBUG
          g_print ("Got BI_RGB compression\n");
#endif

          setMasksDefault (Bitmap_Head.biBitCnt, masks);
        }
    }
  else
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading BMP file header from '%s'"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  /* Valid bit depth is 1, 4, 8, 16, 24, 32 */
  /* 16 is awful, we should probably shoot whoever invented it */

  switch (Bitmap_Head.biBitCnt)
    {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 24:
    case 32:
      break;
    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a valid BMP file"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  /* There should be some colors used! */

  ColormapSize =
    (Bitmap_File_Head.bfOffs - Bitmap_File_Head.biSize - 14) / Maps;

  if ((Bitmap_Head.biClrUsed == 0) && (Bitmap_Head.biBitCnt <= 8))
    ColormapSize = Bitmap_Head.biClrUsed = 1 << Bitmap_Head.biBitCnt;

  if (ColormapSize > 256)
    ColormapSize = 256;

  /* Sanity checks */

  if (Bitmap_Head.biHeight == 0 || Bitmap_Head.biWidth == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a valid BMP file"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  /* biHeight may be negative, but G_MININT32 is dangerous because:
     G_MININT32 == -(G_MININT32) */
  if (Bitmap_Head.biWidth < 0 ||
      Bitmap_Head.biHeight == G_MININT32)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a valid BMP file"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  if (Bitmap_Head.biPlanes != 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a valid BMP file"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  if (Bitmap_Head.biClrUsed > 256 && Bitmap_Head.biBitCnt <= 8)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a valid BMP file"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  /* protect against integer overflows caused by malicious BMPs */
  /* use divisions in comparisons to avoid type overflows */

  if (((guint64) Bitmap_Head.biWidth) > G_MAXINT32 / Bitmap_Head.biBitCnt ||
      ((guint64) Bitmap_Head.biWidth) > (G_MAXINT32 / ABS (Bitmap_Head.biHeight)) / 4)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a valid BMP file"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  /* Windows and OS/2 declare filler so that rows are a multiple of
   * word length (32 bits == 4 bytes)
   */

  rowbytes= ((Bitmap_Head.biWidth * Bitmap_Head.biBitCnt - 1) / 32) * 4 + 4;

#ifdef DEBUG
  printf ("\nSize: %lu, Colors: %lu, Bits: %hu, Width: %ld, Height: %ld, "
          "Comp: %lu, Zeile: %d\n",
          Bitmap_File_Head.bfSize,
          Bitmap_Head.biClrUsed,
          Bitmap_Head.biBitCnt,
          Bitmap_Head.biWidth,
          Bitmap_Head.biHeight,
          Bitmap_Head.biCompr,
          rowbytes);
#endif

  if (Bitmap_Head.biBitCnt <= 8)
    {
#ifdef DEBUG
      printf ("Colormap read\n");
#endif
      /* Get the Colormap */
      if (!ReadColorMap (fd, ColorMap, ColormapSize, Maps, &Grey))
        goto out;
    }

  fseek (fd, Bitmap_File_Head.bfOffs, SEEK_SET);

  /* Get the Image and return the ID or -1 on error*/
  image_ID = ReadImage (fd,
                        Bitmap_Head.biWidth,
                        ABS (Bitmap_Head.biHeight),
                        ColorMap,
                        Bitmap_Head.biClrUsed,
                        Bitmap_Head.biBitCnt,
                        Bitmap_Head.biCompr,
                        rowbytes,
                        Grey,
                        masks,
                        error);

  if (image_ID < 0)
    goto out;

  if (Bitmap_Head.biXPels > 0 && Bitmap_Head.biYPels > 0)
    {
      /* Fixed up from scott@asofyet's changes last year, njl195 */
      gdouble xresolution;
      gdouble yresolution;

      /* I don't agree with scott's feeling that Gimp should be
       * trying to "fix" metric resolution translations, in the
       * long term Gimp should be SI (metric) anyway, but we
       * haven't told the Americans that yet  */

      xresolution = Bitmap_Head.biXPels * 0.0254;
      yresolution = Bitmap_Head.biYPels * 0.0254;

      gimp_image_set_resolution (image_ID, xresolution, yresolution);
    }

  if (Bitmap_Head.biHeight < 0)
    gimp_image_flip (image_ID, GIMP_ORIENTATION_VERTICAL);

out:
  if (fd)
    fclose (fd);

  return image_ID;
}

static gint32
ReadImage (FILE                  *fd,
           gint                   width,
           gint                   height,
           guchar                 cmap[256][3],
           gint                   ncols,
           gint                   bpp,
           gint                   compression,
           gint                   rowbytes,
           gboolean               grey,
           const Bitmap_Channel  *masks,
           GError               **error)
{
  guchar             v, n;
  gint               xpos = 0;
  gint               ypos = 0;
  gint32             image;
  gint32             layer;
  GeglBuffer        *buffer;
  guchar            *dest, *temp, *row_buf;
  guchar             gimp_cmap[768];
  gushort            rgb;
  glong              rowstride, channels;
  gint               i, i_max, j, cur_progress, max_progress;
  gint               total_bytes_read;
  GimpImageBaseType  base_type;
  GimpImageType      image_type;
  guint32            px32;

  if (! (compression == BI_RGB ||
      (bpp == 8 && compression == BI_RLE8) ||
      (bpp == 4 && compression == BI_RLE4) ||
      (bpp == 16 && compression == BI_BITFIELDS) ||
      (bpp == 32 && compression == BI_BITFIELDS)))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s",
                   _("Unrecognized or invalid BMP compression format."));
      return -1;
    }

  /* Make a new image in GIMP */

  switch (bpp)
    {
    case 32:
    case 24:
    case 16:
      base_type = GIMP_RGB;
      if (masks[3].mask != 0)
        {
          image_type = GIMP_RGBA_IMAGE;
          channels = 4;
        }
      else
        {
          image_type = GIMP_RGB_IMAGE;
          channels = 3;
        }
      break;

    case 8:
    case 4:
    case 1:
      if (grey)
        {
          base_type = GIMP_GRAY;
          image_type = GIMP_GRAY_IMAGE;
        }
      else
        {
          base_type = GIMP_INDEXED;
          image_type = GIMP_INDEXED_IMAGE;
        }

      channels = 1;
      break;

    default:
      g_message (_("Unsupported or invalid bitdepth."));
      return -1;
    }

  if ((width < 0) || (width > GIMP_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image width: %d"), width);
      return -1;
    }

  if ((height < 0) || (height > GIMP_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image height: %d"), height);
      return -1;
    }

  image = gimp_image_new (width, height, base_type);
  layer = gimp_layer_new (image, _("Background"),
                          width, height,
                          image_type, 100, GIMP_NORMAL_MODE);

  gimp_image_set_filename (image, filename);

  gimp_image_insert_layer (image, layer, -1, 0);

  /* use g_malloc0 to initialize the dest buffer so that unspecified
     pixels in RLE bitmaps show up as the zeroth element in the palette.
  */
  dest      = g_malloc0 (width * height * channels);
  row_buf   = g_malloc (rowbytes);
  rowstride = width * channels;

  ypos = height - 1;  /* Bitmaps begin in the lower left corner */
  cur_progress = 0;
  max_progress = height;

  switch (bpp)
    {
    case 32:
      {
        while (ReadOK (fd, row_buf, rowbytes))
          {
            temp = dest + (ypos * rowstride);
            for (xpos= 0; xpos < width; ++xpos)
              {
                px32 = ToL(&row_buf[xpos*4]);
                *(temp++) = ((px32 & masks[0].mask) >> masks[0].shiftin) * 255.0 / masks[0].max_value + 0.5;
                *(temp++) = ((px32 & masks[1].mask) >> masks[1].shiftin) * 255.0 / masks[1].max_value + 0.5;
                *(temp++) = ((px32 & masks[2].mask) >> masks[2].shiftin) * 255.0 / masks[2].max_value + 0.5;
                if (channels > 3)
                  *(temp++) = ((px32 & masks[3].mask) >> masks[3].shiftin) * 255.0 / masks[3].max_value + 0.5;
              }
            if (ypos == 0)
              break;
            --ypos; /* next line */
            cur_progress++;
            if ((cur_progress % 5) == 0)
              gimp_progress_update ((gdouble) cur_progress /
                                    (gdouble) max_progress);
          }
      }
      break;

    case 24:
      {
        while (ReadOK (fd, row_buf, rowbytes))
          {
            temp = dest + (ypos * rowstride);
            for (xpos= 0; xpos < width; ++xpos)
              {
                *(temp++) = row_buf[xpos * 3 + 2];
                *(temp++) = row_buf[xpos * 3 + 1];
                *(temp++) = row_buf[xpos * 3];
              }
            if (ypos == 0)
              break;
            --ypos; /* next line */
            cur_progress++;
            if ((cur_progress % 5) == 0)
              gimp_progress_update ((gdouble) cur_progress /
                                    (gdouble) max_progress);
          }
      }
      break;

    case 16:
      {
        while (ReadOK (fd, row_buf, rowbytes))
          {
            temp = dest + (ypos * rowstride);
            for (xpos= 0; xpos < width; ++xpos)
              {
                rgb= ToS(&row_buf[xpos * 2]);
                *(temp++) = ((rgb & masks[0].mask) >> masks[0].shiftin) * 255.0 / masks[0].max_value + 0.5;
                *(temp++) = ((rgb & masks[1].mask) >> masks[1].shiftin) * 255.0 / masks[1].max_value + 0.5;
                *(temp++) = ((rgb & masks[2].mask) >> masks[2].shiftin) * 255.0 / masks[2].max_value + 0.5;
                if (channels > 3)
                  *(temp++) = ((rgb & masks[3].mask) >> masks[3].shiftin) * 255.0 / masks[3].max_value + 0.5;
              }
            if (ypos == 0)
              break;
            --ypos; /* next line */
            cur_progress++;
            if ((cur_progress % 5) == 0)
              gimp_progress_update ((gdouble) cur_progress /
                                    (gdouble) max_progress);
          }
      }
      break;

    case 8:
    case 4:
    case 1:
      {
        if (compression == 0)
          /* no compression */
          {
            while (ReadOK (fd, &v, 1))
              {
                for (i = 1; (i <= (8 / bpp)) && (xpos < width); i++, xpos++)
                  {
                    temp = dest + (ypos * rowstride) + (xpos * channels);
                    *temp=( v & ( ((1<<bpp)-1) << (8-(i*bpp)) ) ) >> (8-(i*bpp));
                    if (grey)
                      *temp = cmap[*temp][0];
                  }
                if (xpos == width)
                  {
                    fread (row_buf, rowbytes - 1 - (width * bpp - 1) / 8, 1, fd);
                    if (ypos == 0)
                      break;
                    ypos--;
                    xpos = 0;

                    cur_progress++;
                    if ((cur_progress % 5) == 0)
                      gimp_progress_update ((gdouble) cur_progress /
                                            (gdouble) max_progress);
                  }
                if (ypos < 0)
                  break;
              }
            break;
          }
        else
          {
            /* compressed image (either RLE8 or RLE4) */
            while (ypos >= 0 && xpos <= width)
              {
                if (!ReadOK (fd, row_buf, 2))
                  {
                    g_message (_("The bitmap ends unexpectedly."));
                    break;
                  }

                if (row_buf[0] != 0)
                  /* Count + Color - record */
                  {
                    /* encoded mode run -
                         row_buf[0] == run_length
                         row_buf[1] == pixel data
                    */
                    for (j = 0;
                         ((guchar) j < row_buf[0]) && (xpos < width);)
                      {
#ifdef DEBUG2
                        printf("%u %u | ",xpos,width);
#endif
                        for (i = 1;
                             ((i <= (8 / bpp)) &&
                              (xpos < width) &&
                              ((guchar) j < row_buf[0]));
                             i++, xpos++, j++)
                          {
                            temp = dest + (ypos * rowstride) + (xpos * channels);
                            *temp = (row_buf[1] &
                                     (((1<<bpp)-1) << (8 - (i * bpp)))) >> (8 - (i * bpp));
                            if (grey)
                              *temp = cmap[*temp][0];
                          }
                      }
                  }
                if ((row_buf[0] == 0) && (row_buf[1] > 2))
                  /* uncompressed record */
                  {
                    n = row_buf[1];
                    total_bytes_read = 0;
                    for (j = 0; j < n; j += (8 / bpp))
                      {
                        /* read the next byte in the record */
                        if (!ReadOK (fd, &v, 1))
                          {
                            g_message (_("The bitmap ends unexpectedly."));
                            break;
                          }
                        total_bytes_read++;

                        /* read all pixels from that byte */
                        i_max = 8 / bpp;
                        if (n - j < i_max)
                          {
                            i_max = n - j;
                          }

                        i = 1;
                        while ((i <= i_max) && (xpos < width))
                          {
                            temp =
                              dest + (ypos * rowstride) + (xpos * channels);
                            *temp = (v >> (8-(i*bpp))) & ((1<<bpp)-1);
                            if (grey)
                              *temp = cmap[*temp][0];
                            i++;
                            xpos++;
                          }
                      }

                    /* absolute mode runs are padded to 16-bit alignment */
                    if (total_bytes_read % 2)
                      fread(&v, 1, 1, fd);
                  }
                if ((row_buf[0] == 0) && (row_buf[1] == 0))
                  /* Line end */
                  {
                    ypos--;
                    xpos = 0;

                    cur_progress++;
                    if ((cur_progress % 5) == 0)
                      gimp_progress_update ((gdouble) cur_progress /
                                            (gdouble)  max_progress);
                  }
                if ((row_buf[0] == 0) && (row_buf[1] == 1))
                  /* Bitmap end */
                  {
                    break;
                  }
                if ((row_buf[0] == 0) && (row_buf[1] == 2))
                  /* Deltarecord */
                  {
                    if (!ReadOK (fd, row_buf, 2))
                      {
                        g_message (_("The bitmap ends unexpectedly."));
                        break;
                      }
                    xpos += row_buf[0];
                    ypos -= row_buf[1];
                  }
              }
            break;
          }
      }
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  if (bpp <= 8)
    for (i = 0, j = 0; i < ncols; i++)
      {
        gimp_cmap[j++] = cmap[i][0];
        gimp_cmap[j++] = cmap[i][1];
        gimp_cmap[j++] = cmap[i][2];
      }

  buffer = gimp_drawable_get_buffer (layer);

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL, dest, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);

  g_free (dest);

  if ((!grey) && (bpp<= 8))
    gimp_image_set_colormap (image, gimp_cmap, ncols);

  gimp_progress_update (1.0);

  return image;
}
