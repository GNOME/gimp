/* bmpread.c	reads any bitmap I could get for testing */
/* Alexander.Schulz@stud.uni-karlsruhe.de                */

/*
 * The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * ----------------------------------------------------------------------------
 */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "bmp.h"

#include "libgimp/stdplugins-intl.h"

#if !defined(WIN32) || defined(__MINGW32__)
#define BI_RGB 		0
#define BI_RLE8  	1
#define BI_RLE4  	2
#define BI_BITFIELDS  	3
#endif

static gint32 ReadImage (FILE     *fd,
                         gint      width,
                         gint      height,
                         guchar    cmap[256][3],
                         gint      ncols,
                         gint      bpp,
                         gint      compression,
                         gint      rowbytes,
                         gboolean  grey,
                         const Bitmap_Channel *masks);


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
ReadChannelMasks (FILE *fd, Bitmap_Channel *masks, guint channels)
{
  guint32 tmp[3];
  guint32 mask;
  gint   i, nbits, offset, bit;

  if (!ReadOK (fd, tmp, 3 * sizeof (guint32)))
    return FALSE;

  for (i = 0; i < channels; i++)
    {
      mask = ToL ((guchar*) &tmp[i]);
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
      masks[i].shiftout = 8 - nbits;
#ifdef DEBUG
      g_print ("Channel %d mask %08x in %d out %d\n", i, masks[i].mask, masks[i].shiftin, masks[i].shiftout);
#endif
    }
  return TRUE;
}

gint32
ReadBMP (const gchar *name)
{
  FILE     *fd;
  gchar    *temp_buf;
  guchar    buffer[64];
  gint      ColormapSize, rowbytes, Maps;
  gboolean  Grey;
  guchar    ColorMap[256][3];
  gint32    image_ID;
  guchar    magick[2];
  Bitmap_Channel *masks;

  filename = name;
  fd = fopen (filename, "rb");

  if (!fd)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  temp_buf = g_strdup_printf (_("Opening '%s'..."),
                              gimp_filename_to_utf8 (name));
  gimp_progress_init (temp_buf);
  g_free (temp_buf);

  /* It is a File. Now is it a Bitmap? Read the shortest possible header */

  if (!ReadOK (fd, magick, 2) || !(!strncmp (magick,"BA",2) ||
     !strncmp (magick,"BM",2) || !strncmp (magick,"IC",2)   ||
     !strncmp (magick,"PI",2) || !strncmp (magick,"CI",2)   ||
     !strncmp (magick,"CP",2)))
    {
      g_message (_("'%s' is not a valid BMP file"),
                 gimp_filename_to_utf8 (filename));
      return -1;
    }

  while (!strncmp(magick,"BA",2))
    {
      if (!ReadOK (fd, buffer, 12))
	{
          g_message (_("'%s' is not a valid BMP file"),
                      gimp_filename_to_utf8 (filename));
          return -1;
        }
      if (!ReadOK (fd, magick, 2))
	{
          g_message (_("'%s' is not a valid BMP file"),
                      gimp_filename_to_utf8 (filename));
          return -1;
        }
    }

  if (!ReadOK (fd, buffer, 12))
    {
      g_message (_("'%s' is not a valid BMP file"),
                  gimp_filename_to_utf8 (filename));
      return -1;
    }

  /* bring them to the right byteorder. Not too nice, but it should work */

  Bitmap_File_Head.bfSize    = ToL (&buffer[0x00]);
  Bitmap_File_Head.zzHotX    = ToS (&buffer[0x04]);
  Bitmap_File_Head.zzHotY    = ToS (&buffer[0x06]);
  Bitmap_File_Head.bfOffs    = ToL (&buffer[0x08]);

  if (!ReadOK (fd, buffer, 4))
    {
      g_message (_("'%s' is not a valid BMP file"),
                  gimp_filename_to_utf8 (filename));
      return -1;
    }

  Bitmap_File_Head.biSize    = ToL (&buffer[0x00]);

  /* What kind of bitmap is it? */

  if (Bitmap_File_Head.biSize == 12) /* OS/2 1.x ? */
    {
      if (!ReadOK (fd, buffer, 8))
        {
          g_message (_("Error reading BMP file header from '%s'"),
                      gimp_filename_to_utf8 (filename));
          return -1;
        }

      Bitmap_Head.biWidth   = ToS (&buffer[0x00]);       /* 12 */
      Bitmap_Head.biHeight  = ToS (&buffer[0x02]);       /* 14 */
      Bitmap_Head.biPlanes  = ToS (&buffer[0x04]);       /* 16 */
      Bitmap_Head.biBitCnt  = ToS (&buffer[0x06]);       /* 18 */
      Bitmap_Head.biCompr   = 0;
      Bitmap_Head.biSizeIm  = 0;
      Bitmap_Head.biXPels   = Bitmap_Head.biYPels = 0;
      Bitmap_Head.biClrUsed = 0;
      Maps = 3;
    }
  else if (Bitmap_File_Head.biSize == 40) /* Windows 3.x */
    {
      if (!ReadOK (fd, buffer, Bitmap_File_Head.biSize - 4))
        {
          g_message (_("Error reading BMP file header from '%s'"),
                      gimp_filename_to_utf8 (filename));
          return -1;
        }
      Bitmap_Head.biWidth   = ToL (&buffer[0x00]);	/* 12 */
      Bitmap_Head.biHeight  = ToL (&buffer[0x04]);	/* 16 */
      Bitmap_Head.biPlanes  = ToS (&buffer[0x08]);       /* 1A */
      Bitmap_Head.biBitCnt  = ToS (&buffer[0x0A]);	/* 1C */
      Bitmap_Head.biCompr   = ToL (&buffer[0x0C]);	/* 1E */
      Bitmap_Head.biSizeIm  = ToL (&buffer[0x10]);	/* 22 */
      Bitmap_Head.biXPels   = ToL (&buffer[0x14]);	/* 26 */
      Bitmap_Head.biYPels   = ToL (&buffer[0x18]);	/* 2A */
      Bitmap_Head.biClrUsed = ToL (&buffer[0x1C]);	/* 2E */
      Bitmap_Head.biClrImp  = ToL (&buffer[0x20]);	/* 32 */
    					                /* 36 */
      Maps = 4;
    }
  else if (Bitmap_File_Head.biSize <= 64) /* Probably OS/2 2.x */
    {
      if (!ReadOK (fd, buffer, Bitmap_File_Head.biSize - 4))
        {
          g_message (_("Error reading BMP file header from '%s'"),
                      gimp_filename_to_utf8 (filename));
          return -1;
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
                                                        /* 36 */
      Maps = 3;
    }
  else
    {
      g_message (_("Error reading BMP file header from '%s'"),
                  gimp_filename_to_utf8 (filename));
      return -1;
    }

  /* Valid bitpdepthis 1, 4, 8, 16, 24, 32 */
  /* 16 is awful, we should probably shoot whoever invented it */

  /* There should be some colors used! */

  ColormapSize = (Bitmap_File_Head.bfOffs - Bitmap_File_Head.biSize - 14) / Maps;

  if ((Bitmap_Head.biClrUsed == 0) && (Bitmap_Head.biBitCnt <= 8))
    ColormapSize = Bitmap_Head.biClrUsed = 1 << Bitmap_Head.biBitCnt;

  if (ColormapSize > 256)
    ColormapSize = 256;

  /* Sanity checks */

  if (Bitmap_Head.biHeight == 0 || Bitmap_Head.biWidth == 0) {
      g_message (_("Error reading BMP file header from '%s'"),
                  gimp_filename_to_utf8 (filename));
      return -1;
  }

  if (Bitmap_Head.biPlanes != 1) {
      g_message (_("Error reading BMP file header from '%s'"),
                  gimp_filename_to_utf8 (filename));
      return -1;
  }

  if (Bitmap_Head.biClrUsed > 256) {
      g_message (_("Error reading BMP file header from '%s'"),
                  gimp_filename_to_utf8 (filename));
      return -1;
  }

  /* Windows and OS/2 declare filler so that rows are a multiple of
   * word length (32 bits == 4 bytes)
   */

  rowbytes= ((Bitmap_Head.biWidth * Bitmap_Head.biBitCnt - 1) / 32) * 4 + 4;

#ifdef DEBUG
  printf ("\nSize: %u, Colors: %u, Bits: %u, Width: %u, Height: %u, "
          "Comp: %u, Zeile: %u\n",
          Bitmap_File_Head.bfSize,
          Bitmap_Head.biClrUsed,
          Bitmap_Head.biBitCnt,
          Bitmap_Head.biWidth,
          Bitmap_Head.biHeight,
          Bitmap_Head.biCompr,
          rowbytes);
#endif

  if (Bitmap_Head.biCompr == BI_BITFIELDS &&
      Bitmap_Head.biBitCnt == 16)
    {
      masks = g_new (Bitmap_Channel, 3);
      if (! ReadChannelMasks (fd, masks, 3))
        {
          g_message (_("'%s' is not a valid BMP file"),
                      gimp_filename_to_utf8 (filename));
          return -1;
        }
    }
  else if (Bitmap_Head.biBitCnt == 16)
    {
      /* Default is 5.5.5 if no masks are given */
      masks = g_new (Bitmap_Channel, 3);
      masks[0].mask     = 0x7c00;
      masks[0].shiftin  = 10;
      masks[0].shiftout = 3;
      masks[1].mask     = 0x03e0;
      masks[1].shiftin  = 5;
      masks[1].shiftout = 3;
      masks[2].mask     = 0x001f;
      masks[2].shiftin  = 0;
      masks[2].shiftout = 3;
    }
  else
    {
      /* Get the Colormap */
      if (!ReadColorMap (fd, ColorMap, ColormapSize, Maps, &Grey))
        return -1;
      masks = NULL;
    }

#ifdef DEBUG
  printf ("Colormap read\n");
#endif

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
			masks);

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

  return image_ID;
}

static gint32
ReadImage (FILE     *fd,
	   gint      width,
	   gint      height,
	   guchar    cmap[256][3],
	   gint      ncols,
	   gint      bpp,
	   gint      compression,
	   gint      rowbytes,
	   gboolean  grey,
	   const Bitmap_Channel *masks)
{
  guchar             v, n;
  GimpPixelRgn       pixel_rgn;
  gint               xpos = 0;
  gint               ypos = 0;
  gint32             image;
  gint32             layer;
  GimpDrawable      *drawable;
  guchar            *dest, *temp, *buffer;
  guchar             gimp_cmap[768];
  gushort            rgb;
  glong              rowstride, channels;
  gint               i, j, cur_progress, max_progress;
  GimpImageBaseType  base_type;
  GimpImageType      image_type;

  if (! (compression == BI_RGB ||
      (bpp == 8 && compression == BI_RLE8) ||
      (bpp == 4 && compression == BI_RLE4) ||
      (bpp == 16 && compression == BI_BITFIELDS) ||
      (bpp == 32 && compression == BI_BITFIELDS)))
    {
      g_message (_("Unrecognized or invalid BMP compression format."));
      return -1;
    }

  /* Make a new image in the gimp */

  switch (bpp)
    {
    case 32:
    case 24:
    case 16:
      base_type = GIMP_RGB;
      image_type = GIMP_RGB_IMAGE;

      channels = 3;
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
      g_message (_("Unrecognized or invalid BMP compression format."));
      return -1;
    }

  image = gimp_image_new (width, height, base_type);
  layer = gimp_layer_new (image, _("Background"),
                          width, height,
                          image_type, 100, GIMP_NORMAL_MODE);

  gimp_image_set_filename (image, filename);

  gimp_image_add_layer (image, layer, 0);
  drawable = gimp_drawable_get (layer);

  dest      = g_malloc (drawable->width * drawable->height * channels);
  buffer    = g_malloc (rowbytes);
  rowstride = drawable->width * channels;

  ypos = height - 1;  /* Bitmaps begin in the lower left corner */
  cur_progress = 0;
  max_progress = height;

  switch (bpp)
    {
    case 32:
      {
        while (ReadOK (fd, buffer, rowbytes))
          {
            temp = dest + (ypos * rowstride);
            for (xpos= 0; xpos < width; ++xpos)
              {
                *(temp++)= buffer[xpos * 4 + 2];
                *(temp++)= buffer[xpos * 4 + 1];
                *(temp++)= buffer[xpos * 4];
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
        while (ReadOK (fd, buffer, rowbytes))
          {
            temp = dest + (ypos * rowstride);
            for (xpos= 0; xpos < width; ++xpos)
              {
                *(temp++)= buffer[xpos * 3 + 2];
                *(temp++)= buffer[xpos * 3 + 1];
                *(temp++)= buffer[xpos * 3];
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
        while (ReadOK (fd, buffer, rowbytes))
          {
            temp = dest + (ypos * rowstride);
            for (xpos= 0; xpos < width; ++xpos)
              {
                rgb= ToS(&buffer[xpos * 2]);
                *(temp++) = ((rgb & masks[0].mask) >> masks[0].shiftin) << masks[0].shiftout;
                *(temp++) = ((rgb & masks[1].mask) >> masks[1].shiftin) << masks[1].shiftout;
                *(temp++) = ((rgb & masks[2].mask) >> masks[2].shiftin) << masks[2].shiftout;
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
		    ReadOK (fd, buffer, rowbytes - 1 - (width * bpp - 1) / 8);
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
	    while (ypos >= 0 && xpos <= width)
	      {
		ReadOK (fd, buffer, 2);
		if ((guchar) buffer[0] != 0)
		  /* Count + Color - record */
		  {
		    for (j = 0; ((guchar) j < (guchar) buffer[0]) && (xpos < width);)
		      {
#ifdef DEBUG2
			printf("%u %u | ",xpos,width);
#endif
			for (i = 1;
			     ((i <= (8 / bpp)) &&
			      (xpos < width) &&
			      ((guchar) j < (unsigned char) buffer[0]));
			     i++, xpos++, j++)
			  {
			    temp = dest + (ypos * rowstride) + (xpos * channels);
			    *temp = (buffer[1] & (((1<<bpp)-1) << (8 - (i * bpp)))) >> (8 - (i * bpp));
			    if (grey)
			      *temp = cmap[*temp][0];
			  }
		      }
		  }
		if (((guchar) buffer[0] == 0) && ((guchar) buffer[1] > 2))
		  /* uncompressed record */
		  {
		    n = buffer[1];
		    for (j = 0; j < n; j += (8 / bpp))
		      {
			ReadOK (fd, &v, 1);
			i = 1;
			while ((i <= (8 / bpp)) && (xpos < width))
			  {
			    temp = dest + (ypos * rowstride) + (xpos * channels);
			    *temp = (v & (((1<<bpp)-1) << (8-(i*bpp)))) >> (8-(i*bpp));
			    if (grey)
			      *temp = cmap[*temp][0];
			    i++;
			    xpos++;
			  }
		      }

		    if ((n % 2) && (bpp==4))
		      n++;

		    if ((n / (8 / bpp)) % 2)
		      ReadOK (fd, &v, 1);

		    /*if odd(x div (8 div bpp )) then blockread(f,z^,1);*/
		  }
		if (((guchar) buffer[0] == 0) && ((guchar) buffer[1]==0))
		  /* Line end */
		  {
		    ypos--;
		    xpos = 0;

		    cur_progress++;
		    if ((cur_progress % 5) == 0)
		      gimp_progress_update ((gdouble) cur_progress /
					    (gdouble)  max_progress);
		  }
		if (((guchar) buffer[0]==0) && ((guchar) buffer[1]==1))
		  /* Bitmap end */
		  {
		    break;
		  }
		if (((guchar) buffer[0]==0) && ((guchar) buffer[1]==2))
		  /* Deltarecord */
		  {
		    ReadOK (fd, buffer, 2);
		    xpos += (guchar) buffer[0];
		    ypos -= (guchar) buffer[1];
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

  fclose (fd);
  if (bpp <= 8)
    for (i = 0, j = 0; i < ncols; i++)
      {
	gimp_cmap[j++] = cmap[i][0];
	gimp_cmap[j++] = cmap[i][1];
	gimp_cmap[j++] = cmap[i][2];
      }

  gimp_progress_update (1);

  gimp_pixel_rgn_init (&pixel_rgn, drawable,
		       0, 0, drawable->width, drawable->height, TRUE, FALSE);
  gimp_pixel_rgn_set_rect (&pixel_rgn, dest,
			   0, 0, drawable->width, drawable->height);

  if ((!grey) && (bpp<= 8))
    gimp_image_set_colormap (image, gimp_cmap, ncols);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);
  g_free (dest);

  return image;
}
