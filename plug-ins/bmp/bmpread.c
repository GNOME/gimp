/* bmpread.c	reads any bitmap I could get for testing */
/*		except OS2 bitmaps (wrong colors)        */
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
#include "bmpos2.h"

#include "libgimp/stdplugins-intl.h"


gint32 
ReadBMP (gchar *name)
{
  FILE *fd;
  gchar *temp_buf;
  gchar buf[5];
  gint ColormapSize, SpeicherZeile, Maps, Grey;
  guchar ColorMap[256][3];
  guchar puffer[50];
  gint32 image_ID;
  
  if (interactive_bmp)
    {
      temp_buf = g_strdup_printf (_("Loading %s:"), name);
      gimp_progress_init (temp_buf);
      g_free (temp_buf);
    }

  filename = name;
  fd = fopen (filename, "rb");
  
  /* Is this a valid File? Should never be used because gimp tests it. */
  
  if (!fd)
    {
      g_message (_("%s: can't open \"%s\"\n"), prog_name, filename);
      return -1;
    }

  /* It is a File. Now is it a Bitmap? */
  
  if (!ReadOK(fd,buf,2) || (strncmp(buf,"BM",2)))
    {
      g_message (_("%s: not a valid BMP file %s\n"), prog_name,buf);
      return -1;
    }

  /* How long is the Header? */

  if (!ReadOK (fd, puffer, 0x10))
    {
      g_message (_("%s: error reading BMP file header\n"), prog_name);
      return -1;
    }

  /* bring them to the right byteorder. Not too nice, but it should work */

  Bitmap_File_Head.bfSize    = ToL (&puffer[0]);
  Bitmap_File_Head.reserverd = ToL (&puffer[4]);
  Bitmap_File_Head.bfOffs    = ToL (&puffer[8]);
  Bitmap_File_Head.biSize    = ToL (&puffer[12]);
  
  /* Is it a Windows (R) Bitmap or not */  
  
  if (Bitmap_File_Head.biSize == 12) /* OS/2 */
    {
      if (!read_os2_head1 (fd, Bitmap_File_Head.biSize - 4, &Bitmap_Head))
        {
          g_message (_("%s: error reading BMP file header\n"), prog_name);
          return -1;
        }
      Maps = 3;
    }
  else if (Bitmap_File_Head.biSize != 40) 
    {
      g_warning ("OS/2 unsupported!\n");
      if (!ReadOK (fd, puffer, Bitmap_File_Head.biSize))
        {
          g_message (_("%s: error reading BMP file header\n"), prog_name);
          return -1;
        }

      Bitmap_OS2_Head.bcWidth  = ToS (&puffer[0]);
      Bitmap_OS2_Head.bcHeight = ToS (&puffer[2]);
      Bitmap_OS2_Head.bcPlanes = ToS (&puffer[4]);
      Bitmap_OS2_Head.bcBitCnt = ToS (&puffer[6]);

      Bitmap_Head.biPlanes    = Bitmap_OS2_Head.bcPlanes;
      Bitmap_Head.biBitCnt    = Bitmap_OS2_Head.bcBitCnt;
      Bitmap_File_Head.bfSize = ((Bitmap_File_Head.bfSize * 4) -
				 (Bitmap_File_Head.bfOffs * 3));
      Bitmap_Head.biHeight    = Bitmap_OS2_Head.bcHeight;
      Bitmap_Head.biWidth     = Bitmap_OS2_Head.bcWidth;
      Bitmap_Head.biClrUsed   = 0;
      Bitmap_Head.biCompr     = 0;
      Maps = 3;
    }
  else
    {
      if (!ReadOK (fd, puffer, 36))
        {
          g_message (_("%s: error reading BMP file header\n"), prog_name);
          return -1;
        }
      Bitmap_Head.biWidth   =ToL (&puffer[0x00]);	/* 12 */
      Bitmap_Head.biHeight  =ToL (&puffer[0x04]);	/* 16 */
      Bitmap_Head.biPlanes  =ToS (&puffer[0x08]);       /* 1A */
      Bitmap_Head.biBitCnt  =ToS (&puffer[0x0A]);	/* 1C */
      Bitmap_Head.biCompr   =ToL (&puffer[0x0C]);	/* 1E */
      Bitmap_Head.biSizeIm  =ToL (&puffer[0x10]);	/* 22 */
      Bitmap_Head.biXPels   =ToL (&puffer[0x14]);	/* 26 */
      Bitmap_Head.biYPels   =ToL (&puffer[0x18]);	/* 2A */
      Bitmap_Head.biClrUsed =ToL (&puffer[0x1C]);	/* 2E */
      Bitmap_Head.biClrImp  =ToL (&puffer[0x20]);	/* 32 */
    					                /* 36 */
      Maps = 4;
    }
  
  /* This means wrong file Format. I test this because it could crash the
   * entire gimp.
   */
  
  if (Bitmap_Head.biBitCnt > 24) 
    {
      g_message (_("%s: too many colors: %u\n"), prog_name,
		 (guint) Bitmap_Head.biBitCnt);
      return -1;
    }

  /* There should be some colors used! */
  
  ColormapSize = (Bitmap_File_Head.bfOffs - Bitmap_File_Head.biSize - 14) / Maps;

  if ((Bitmap_Head.biClrUsed == 0) &&
      (Bitmap_Head.biBitCnt < 24))
    Bitmap_Head.biClrUsed = ColormapSize;

  if (Bitmap_Head.biBitCnt == 24)
    SpeicherZeile = ((Bitmap_File_Head.bfSize - Bitmap_File_Head.bfOffs) /
		     Bitmap_Head.biHeight);
  else
    SpeicherZeile = ((Bitmap_File_Head.bfSize - Bitmap_File_Head.bfOffs) /
		     Bitmap_Head.biHeight) * (8 / Bitmap_Head.biBitCnt);
  
#ifdef DEBUG
  printf("\nSize: %u, Colors: %u, Bits: %u, Width: %u, Height: %u, Comp: %u, Zeile: %u\n",
          Bitmap_File_Head.bfSize,Bitmap_Head.biClrUsed,Bitmap_Head.biBitCnt,Bitmap_Head.biWidth,
          Bitmap_Head.biHeight, Bitmap_Head.biCompr, SpeicherZeile);
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
			SpeicherZeile, 
			Grey);
  
#ifdef GIMP_HAVE_RESOLUTION_INFO
  {
    /* quick hack by the muppet, scott@asofyet.org, 19 dec 1999 */
    gdouble xresolution;
    gdouble yresolution;

    /*
     * xresolution and yresolution are in dots per inch.
     * the BMP spec says that biXPels and biYPels are in
     * pixels per meter as long ints (actually, "DWORDS").
     * this means we've lost some accuracy in the numbers.
     * typically, the dots per inch settings on BMPs will
     * be integer numbers of dots per inch, which is freaky
     * because they're stored in the BMP as metric.  *sigh*
     * so, we'll round this off, even though the gimp wants
     * a floating point number...
     */
    #define LROUND(x)   ((long int)((x)+0.5))
    xresolution = LROUND((Bitmap_Head.biXPels * 2.54 / 100.0));
    yresolution = LROUND((Bitmap_Head.biYPels * 2.54 / 100.0));
    #undef LROUND

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
	  g_message (_("%s: bad colormap\n"), prog_name);
	  return -1;
	}
      
      /* Bitmap save the colors in another order! But change only once! */

      if (size == 4)
	{
	  buffer[i][0] = rgb[2];
	  buffer[i][1] = rgb[1];
	  buffer[i][2] = rgb[0];
	}
      else
	{
	  /* this one is for old os2 Bitmaps */
	  buffer[i][0] = rgb[2];
	  buffer[i][1] = rgb[1];
	  buffer[i][2] = rgb[0];
	}
      *grey = ((*grey) && (rgb[0]==rgb[1]) && (rgb[1]==rgb[2]));
    }
  return 0;
}

Image 
ReadImage (FILE   *fd, 
	   gint    len, 
	   gint    height, 
	   guchar  cmap[256][3], 
	   gint    ncols, 
	   gint    bpp, 
	   gint    compression, 
	   gint    spzeile, 
	   gint    grey)
{
  gchar *name_buf;
  guchar v,wieviel;
  GPixelRgn pixel_rgn;
  gchar buf[16];
  gint xpos = 0, ypos = 0;
  Image image;
  gint32 layer;
  GDrawable *drawable;
  guchar *dest, *temp;
  guchar gimp_cmap[768];
  long rowstride, channels;
  gint i, j, cur_progress, max_progress, egal;
  
  /* Make a new image in the gimp */
  
  if (grey)
    {
      image = gimp_image_new (len, height, GRAY);
      layer = gimp_layer_new (image, _("Background"),
			      len, height, GRAY_IMAGE, 100, NORMAL_MODE);
      channels = 1;
    }
  else
    {
      if (bpp<24) 
	{
	  image = gimp_image_new (len, height, INDEXED);
	  layer = gimp_layer_new (image, _("Background"),
				  len, height, INDEXED_IMAGE, 100, NORMAL_MODE);
	  channels = 1;
	}
      else 
	{
	  image = gimp_image_new (len, height, RGB);
	  layer = gimp_layer_new (image, _("Background"),
				  len, height, RGB_IMAGE, 100, NORMAL_MODE);
	  channels = 3;
	}
    }
  
  name_buf = g_malloc (strlen (filename) + 10);
  strcpy (name_buf, filename);
  gimp_image_set_filename(image,name_buf);
  g_free (name_buf);
  
  gimp_image_add_layer(image,layer,0);
  drawable = gimp_drawable_get(layer);
  
  dest = g_malloc(drawable->width*drawable->height*channels);
  rowstride = drawable->width * channels;
  
  ypos = height - 1;  /* Bitmaps begin in the lower left corner */
  cur_progress = 0;
  max_progress = height;
  
  if (bpp == 24)
    {
      while (ReadOK (fd, buf, 3))
        {
          temp = dest + (ypos * rowstride) + (xpos * channels);
          *temp=buf[2];
          temp++;
          *temp=buf[1];
          temp++;
          *temp=buf[0];
          xpos++;
          if (xpos == len)
            {
              egal=ReadOK (fd, buf, spzeile - (len * 3));
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
    }
  else
    {
      switch(compression)
	{
	case 0:  			/* uncompressed */
	  {
	    while (ReadOK (fd, &v, 1))
	      {
		for (i = 1; (i <= (8 / bpp)) && (xpos < len); i++, xpos++)
		  {
		    temp = dest + (ypos * rowstride) + (xpos * channels);
		    *temp=( v & ( ((1<<bpp)-1) << (8-(i*bpp)) ) ) >> (8-(i*bpp));
		    if (grey)
		      *temp = cmap[*temp][0];
		  }
		if (xpos == len)
		  {
		    egal = ReadOK (fd, buf, (spzeile - len) / (8 / bpp));
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
	default:			/* Compressed images */
	  {
	    while (TRUE)
	      {
		egal = ReadOK (fd, buf, 2);
		if ((guchar) buf[0] != 0) 
		  /* Count + Color - record */
		  {
		    for (j = 0; ((guchar) j < (guchar) buf[0]) && (xpos < len);)
		      {
#ifdef DEBUG2
			printf("%u %u | ",xpos,len);
#endif
			for (i = 1;
			     ((i <= (8 / bpp)) &&
			      (xpos < len) &&
			      ((guchar) j < (unsigned char) buf[0]));
			     i++, xpos++, j++)
			  {
			    temp = dest + (ypos * rowstride) + (xpos * channels);
			    *temp = (buf[1] & (((1<<bpp)-1) << (8 - (i * bpp)))) >> (8 - (i * bpp));
			    if (grey)
			      *temp = cmap[*temp][0];
			  }
		      }
		  }
		if (((guchar) buf[0] == 0) && ((guchar) buf[1] > 2))
		  /* uncompressed record */
		  {
		    wieviel = buf[1];
		    for (j = 0; j < wieviel; j += (8 / bpp))
		      {
			egal = ReadOK (fd, &v, 1);
			i = 1;
			while ((i <= (8 / bpp)) && (xpos < len))
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
		      egal = ReadOK (fd, &v, 1);
		    /*if odd(x div (8 div bpp )) then blockread(f,z^,1);*/
		  }
		if (((guchar) buf[0] == 0) && ((guchar) buf[1]==0))
		  /* Zeilenende */
		  {
		    ypos--;
		    xpos = 0;

		    cur_progress++;
		    if ((interactive_bmp) &&
			((cur_progress % 5) == 0))
		      gimp_progress_update ((gdouble) cur_progress /
					    (gdouble)  max_progress);
		  }
		if (((guchar) buf[0]==0) && ((guchar) buf[1]==1))
		  /* Bitmapende */
		  {
		    break;
		  }
		if (((guchar) buf[0]==0) && ((guchar) buf[1]==2))
		  /* Deltarecord */
		  {
		    xpos += (guchar) buf[2];
		    ypos += (guchar) buf[3];
		  }
	      }
	    break;
	  }
	}
    }

  fclose (fd);
  if (bpp < 24)
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

  if ((!grey) && (bpp<24))
    gimp_image_set_cmap (image, gimp_cmap, ncols);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);
  g_free (dest);

  return image;
}
