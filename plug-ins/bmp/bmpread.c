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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "bmp.h"

#include "libgimp/stdplugins-intl.h"


gint32 
ReadBMP (gchar *name)
{
  FILE *fd;
  gchar *temp_buf;
  guchar buffer[64];
  gint ColormapSize, rowbytes, Maps, Grey;
  guchar ColorMap[256][3];
  gint32 image_ID;
  
  if (interactive_bmp)
    {
      temp_buf = g_strdup_printf (_("Loading %s:"), name);
      gimp_progress_init (temp_buf);
      g_free (temp_buf);
    }

  filename = name;
  fd = fopen (filename, "rb");
  
  /* Is this a valid File? Should never be used because Gimp tests it. */
  
  if (!fd)
    {
      g_message (_("%s: can't open \"%s\""), prog_name, filename);
      return -1;
    }

  /* It is a File. Now is it a Bitmap? Read the shortest possible header */
  
  if (!ReadOK(fd, buffer, 18) || (strncmp(buffer,"BM",2)))
    {
      g_message (_("%s: %s is not a valid BMP file"), prog_name, filename);
      return -1;
    }

  /* bring them to the right byteorder. Not too nice, but it should work */

  Bitmap_File_Head.bfSize    = ToL (&buffer[0x02]);
  Bitmap_File_Head.zzHotX    = ToS (&buffer[0x06]);
  Bitmap_File_Head.zzHotY    = ToS (&buffer[0x08]);
  Bitmap_File_Head.bfOffs    = ToL (&buffer[0x0a]);
  Bitmap_File_Head.biSize    = ToL (&buffer[0x0e]);
  
  /* What kind of bitmap is it? */  
  
  if (Bitmap_File_Head.biSize == 12) /* OS/2 1.x ? */
    {
      if (!ReadOK (fd, buffer, 8))
        {
          g_message (_("%s: error reading BMP file header"), prog_name);
          return -1;
        }

      Bitmap_Head.biWidth   =ToS (&buffer[0x00]);       /* 12 */
      Bitmap_Head.biHeight  =ToS (&buffer[0x02]);       /* 14 */
      Bitmap_Head.biPlanes  =ToS (&buffer[0x04]);       /* 16 */
      Bitmap_Head.biBitCnt  =ToS (&buffer[0x06]);       /* 18 */
      Bitmap_Head.biCompr = 0;
      Bitmap_Head.biSizeIm = 0;
      Bitmap_Head.biXPels = Bitmap_Head.biYPels = 0;
      Bitmap_Head.biClrUsed = 0;
      Maps = 3;
    }
  else if (Bitmap_File_Head.biSize == 40) /* Windows 3.x */
    {
      if (!ReadOK (fd, buffer, Bitmap_File_Head.biSize - 4))
        {
          g_message (_("%s: error reading BMP file header"), prog_name);
          return -1;
        }
      Bitmap_Head.biWidth   =ToL (&buffer[0x00]);	/* 12 */
      Bitmap_Head.biHeight  =ToL (&buffer[0x04]);	/* 16 */
      Bitmap_Head.biPlanes  =ToS (&buffer[0x08]);       /* 1A */
      Bitmap_Head.biBitCnt  =ToS (&buffer[0x0A]);	/* 1C */
      Bitmap_Head.biCompr   =ToL (&buffer[0x0C]);	/* 1E */
      Bitmap_Head.biSizeIm  =ToL (&buffer[0x10]);	/* 22 */
      Bitmap_Head.biXPels   =ToL (&buffer[0x14]);	/* 26 */
      Bitmap_Head.biYPels   =ToL (&buffer[0x18]);	/* 2A */
      Bitmap_Head.biClrUsed =ToL (&buffer[0x1C]);	/* 2E */
      Bitmap_Head.biClrImp  =ToL (&buffer[0x20]);	/* 32 */
    					                /* 36 */
      Maps = 4;
    }
  else if (Bitmap_File_Head.biSize <= 64) /* Probably OS/2 2.x */
    {
      if (!ReadOK (fd, buffer, Bitmap_File_Head.biSize - 4))
        {
          g_message (_("%s: error reading BMP file header"), prog_name);
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
      g_message (_("%s: error reading BMP file header"), prog_name);
      return -1;
    }

  /* Valid bitpdepthis 1, 4, 8, 16, 24, 32 */
  /* 16 is awful, we should probably shoot whoever invented it */
  
  /* There should be some colors used! */
  
  ColormapSize = (Bitmap_File_Head.bfOffs - Bitmap_File_Head.biSize - 14) / Maps;

  if ((Bitmap_Head.biClrUsed == 0) && (Bitmap_Head.biBitCnt <= 8))
    Bitmap_Head.biClrUsed = ColormapSize;


  /* Sanity checks */

  if (Bitmap_Head.biHeight == 0 || Bitmap_Head.biWidth == 0) {
      g_message (_("%s: error reading BMP file header"), prog_name);
      return -1;
  }

  if (Bitmap_Head.biPlanes != 1) {
      g_message (_("%s: error reading BMP file header"), prog_name);
      return -1;
  }

  if (ColormapSize > 256 || Bitmap_Head.biClrUsed > 256) {
      g_message (_("%s: error reading BMP file header"), prog_name);
      return -1;
  }

  /* Windows and OS/2 declare filler so that rows are a multiple of
   * word length (32 bits == 4 bytes)
   */

  rowbytes= ( (Bitmap_Head.biWidth * Bitmap_Head.biBitCnt - 1) / 32) * 4 + 4;

#ifdef DEBUG
  printf("\nSize: %u, Colors: %u, Bits: %u, Width: %u, Height: %u, Comp: %u, Zeile: %u\n",
          Bitmap_File_Head.bfSize,Bitmap_Head.biClrUsed,Bitmap_Head.biBitCnt,Bitmap_Head.biWidth,
          Bitmap_Head.biHeight, Bitmap_Head.biCompr, rowbytes);
#endif
  
  /* Get the Colormap */
  
  if (ReadColorMap (fd, ColorMap, ColormapSize, Maps, &Grey) == -1)
    return -1;
  
#ifdef DEBUG
  printf("Colormap read\n");
#endif

  /* Get the Image and return the ID or -1 on error*/
  image_ID = ReadImage (fd, 
			Bitmap_Head.biWidth, 
			Bitmap_Head.biHeight, 
			ColorMap, 
			Bitmap_Head.biClrUsed, 
			Bitmap_Head.biBitCnt, 
			Bitmap_Head.biCompr, 
			rowbytes, 
			Grey);
  
#ifdef GIMP_HAVE_RESOLUTION_INFO
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
#endif /* GIMP_HAVE_RESOLUTION_INFO */

  return (image_ID);
}

gint 
ReadColorMap (FILE   *fd, 
	      guchar  buffer[256][3], 
	      gint    number, 
	      gint    size, 
	      gint   *grey)
{
  gint i;
  guchar rgb[4];
  
  *grey=(number>2);
  for (i = 0; i < number ; i++)
    {
      if (!ReadOK (fd, rgb, size))
	{
	  g_message (_("%s: bad colormap"), prog_name);
	  return -1;
	}
      
      /* Bitmap save the colors in another order! But change only once! */

      buffer[i][0] = rgb[2];
      buffer[i][1] = rgb[1];
      buffer[i][2] = rgb[0];
      *grey = ((*grey) && (rgb[0]==rgb[1]) && (rgb[1]==rgb[2]));
    }
  return 0;
}

Image 
ReadImage (FILE   *fd, 
	   gint    width, 
	   gint    height, 
	   guchar  cmap[256][3], 
	   gint    ncols, 
	   gint    bpp, 
	   gint    compression, 
	   gint    rowbytes, 
	   gint    grey)
{
  guchar v,wieviel;
  GimpPixelRgn pixel_rgn;
  gint xpos = 0, ypos = 0;
  Image image;
  gint32 layer;
  GimpDrawable *drawable;
  guchar *dest, *temp, *buffer;
  guchar gimp_cmap[768];
  gushort rgb;
  long rowstride, channels;
  gint i, j, cur_progress, max_progress, unused;
  
  /* Make a new image in the gimp */
  
  if (bpp >= 16)
    {
      image = gimp_image_new (width, height, GIMP_RGB);
      layer = gimp_layer_new (image, _("Background"),
                              width, height, GIMP_RGB_IMAGE, 100, GIMP_NORMAL_MODE);
      channels = 3;
    }
  else if (grey)
    {
      image = gimp_image_new (width, height, GIMP_GRAY);
      layer = gimp_layer_new (image, _("Background"),
			      width, height, GIMP_GRAY_IMAGE, 100, GIMP_NORMAL_MODE);
      channels = 1;
    }
  else
    {
      image = gimp_image_new (width, height, GIMP_INDEXED);
      layer = gimp_layer_new (image, _("Background"),
                              width, height, GIMP_INDEXED_IMAGE, 100, GIMP_NORMAL_MODE);
      channels = 1;
    }
  
  gimp_image_set_filename(image, filename);
  
  gimp_image_add_layer(image,layer,0);
  drawable = gimp_drawable_get(layer);
  
  dest = g_malloc(drawable->width*drawable->height*channels);
  buffer= g_malloc(rowbytes);
  rowstride = drawable->width * channels;
  
  ypos = height - 1;  /* Bitmaps begin in the lower left corner */
  cur_progress = 0;
  max_progress = height;

  
  switch (bpp) {
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
          --ypos; /* next line */
          cur_progress++;
          if ((interactive_bmp) && ((cur_progress % 5) == 0))
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
          --ypos; /* next line */
          cur_progress++;
          if ((interactive_bmp) && ((cur_progress % 5) == 0))
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
               *(temp++)= ((rgb >> 10) & 0x1f) * 8;
               *(temp++)= ((rgb >> 5)  & 0x1f) * 8;
               *(temp++)= ((rgb)       & 0x1f) * 8;
            }
          --ypos; /* next line */
          cur_progress++;
          if ((interactive_bmp) && ((cur_progress % 5) == 0))
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
		    unused = ReadOK (fd, buffer, rowbytes - 1 -
                                                (width * bpp - 1) / 8);
		    ypos--;
		    xpos = 0;

		    cur_progress++;
		    if ((interactive_bmp) &&
			((cur_progress % 5) == 0))
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
		unused = ReadOK (fd, buffer, 2);
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
		    wieviel = buffer[1];
		    for (j = 0; j < wieviel; j += (8 / bpp))
		      {
			unused = ReadOK (fd, &v, 1);
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

		    if ((wieviel % 2) && (bpp==4))
		      wieviel++;

		    if ((wieviel / (8 / bpp)) % 2)
		      unused = ReadOK (fd, &v, 1);
		    /*if odd(x div (8 div bpp )) then blockread(f,z^,1);*/
		  }
		if (((guchar) buffer[0] == 0) && ((guchar) buffer[1]==0))
		  /* Line end */
		  {
		    ypos--;
		    xpos = 0;

		    cur_progress++;
		    if ((interactive_bmp) &&
			((cur_progress % 5) == 0))
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
		    unused = ReadOK (fd, buffer, 2);
		    xpos += (guchar) buffer[0];
		    ypos -= (guchar) buffer[1];
		  }
	      }
	    break;
	  }
    }
    break;
  default:
    /* This is very bad, we should not be here */
    ;
  }

  fclose (fd);
  if (bpp <= 8)
    for (i = 0, j = 0; i < ncols; i++)
      {
	gimp_cmap[j++] = cmap[i][0];
	gimp_cmap[j++] = cmap[i][1];
	gimp_cmap[j++] = cmap[i][2];
      }

  if (interactive_bmp)
    gimp_progress_update (1);

  gimp_pixel_rgn_init (&pixel_rgn, drawable,
		       0, 0, drawable->width, drawable->height, TRUE, FALSE);
  gimp_pixel_rgn_set_rect (&pixel_rgn, dest,
			   0, 0, drawable->width, drawable->height);

  if ((!grey) && (bpp<= 8))
    gimp_image_set_cmap (image, gimp_cmap, ncols);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);
  g_free (dest);

  return image;
}
