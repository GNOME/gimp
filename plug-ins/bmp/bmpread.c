/* bmpread.c	reads any bitmap I could get for testing */
/*		except OS2 bitmaps (wrong colors)        */
/* Alexander.Schulz@stud.uni-karlsruhe.de                */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgimp/gimp.h>
#include <gtk/gtk.h>
#include "bmp.h"

gint32 ReadBMP (name)
     char *name;
{
  FILE *fd;
  char *temp_buf;
  char buf[5];
  int ColormapSize, SpeicherZeile, Maps, Grey;
  unsigned char ColorMap[256][3];
  guchar puffer[50];
  
  if (interactive_bmp)
    {
      temp_buf = g_malloc (strlen (name) + 11);
      sprintf (temp_buf, "Loading %s:", name);
      gimp_progress_init (temp_buf);
      g_free (temp_buf);
    }

  filename = name;
  fd = fopen (filename, "rb");
  
  /* Is this a valid File? Should never be used because gimp tests it. */
  
  if (!fd)
    {
      printf ("%s: can't open \"%s\"\n", prog_name, filename);
      return -1;
    }

  /* It is a File. Now is it a Bitmap? */
  
  if (!ReadOK(fd,buf,2) || (strncmp(buf,"BM",2)))
    {
      printf ("%s: not a valid BMP file %s\n", prog_name,buf);
      return -1;
    }

  /* How long is the Header? */

  if (!ReadOK (fd, puffer, 0x10))
    {
      printf ("%s: error reading bitmap file header\n", prog_name);
      return -1;
    }

  /* bring them to the rigth byreorder. Not too nice, but it should work */

  Bitmap_File_Head.bfSize=ToL(&puffer[0]);
  Bitmap_File_Head.reserverd=ToL(&puffer[4]);
  Bitmap_File_Head.bfOffs=ToL(&puffer[8]);
  Bitmap_File_Head.biSize=ToL(&puffer[12]);
  
  /* Is it a Windows (R) Bitmap or not */  
  
  if (Bitmap_File_Head.biSize!=40) 
    {
      printf("\nos2 unsupported!\n");
      if (!ReadOK (fd, puffer, Bitmap_File_Head.biSize))
        {
          printf ("%s: error reading bitmap header\n", prog_name);
          return -1;
        }

      Bitmap_OS2_Head.bcWidth=ToS(&puffer[0]);
      Bitmap_OS2_Head.bcHeight=ToS(&puffer[2]);
      Bitmap_OS2_Head.bcPlanes=ToS(&puffer[4]);
      Bitmap_OS2_Head.bcBitCnt=ToS(&puffer[6]);

      Bitmap_Head.biPlanes=Bitmap_OS2_Head.bcPlanes;
      Bitmap_Head.biBitCnt=Bitmap_OS2_Head.bcBitCnt;
      Bitmap_File_Head.bfSize=(Bitmap_File_Head.bfSize*4)-(Bitmap_File_Head.bfOffs*3);
      Bitmap_Head.biHeight=Bitmap_OS2_Head.bcHeight;
      Bitmap_Head.biWidth=Bitmap_OS2_Head.bcWidth;
      Bitmap_Head.biClrUsed=0;
      Bitmap_Head.biCompr=0;
      Maps=3;
    }
  else
    {
      if (!ReadOK (fd, puffer, 36))
        {
          printf ("\n%s: error reading bitmap header\n", prog_name);
          return -1;
        }
      Bitmap_Head.biWidth=ToL(&puffer[0x00]);		/* 12 */
      Bitmap_Head.biHeight=ToL(&puffer[0x04]);		/* 16 */
      Bitmap_Head.biPlanes=ToS(&puffer[0x08]);          /* 1A */
      Bitmap_Head.biBitCnt=ToS(&puffer[0x0A]);	        /* 1C */
      Bitmap_Head.biCompr=ToL(&puffer[0x0C]);		/* 1E */
      Bitmap_Head.biSizeIm=ToL(&puffer[0x10]);		/* 22 */
      Bitmap_Head.biXPels=ToL(&puffer[0x14]);		/* 26 */
      Bitmap_Head.biYPels=ToL(&puffer[0x18]);		/* 2A */
      Bitmap_Head.biClrUsed=ToL(&puffer[0x1C]);		/* 2E */
      Bitmap_Head.biClrImp=ToL(&puffer[0x20]);		/* 32 */
    					                /* 36 */
      Maps=4;
    }
  
  /* This means wrong file Format. I test this because it could crash the */
  /* entire gimp.							  */
  
  if (Bitmap_Head.biBitCnt>24) 
  {
    printf("\n%s: to many colors: %u\n",prog_name,(unsigned int) Bitmap_Head.biBitCnt);
    return -1;
  }

  /* There should be some colors used! */
  
  ColormapSize = (Bitmap_File_Head.bfOffs-Bitmap_File_Head.biSize-14) / Maps;
  if ((Bitmap_Head.biClrUsed==0) && (Bitmap_Head.biBitCnt<24)) Bitmap_Head.biClrUsed=ColormapSize;
  if (Bitmap_Head.biBitCnt==24) SpeicherZeile=((Bitmap_File_Head.bfSize-Bitmap_File_Head.bfOffs)/Bitmap_Head.biHeight);
  else SpeicherZeile=((Bitmap_File_Head.bfSize-Bitmap_File_Head.bfOffs)/Bitmap_Head.biHeight)*(8/Bitmap_Head.biBitCnt);
  
#ifdef DEBUG
  printf("\nSize: %u, Colors: %u, Bits: %u, Width: %u, Height: %u, Comp: %u, Zeile: %u\n",
          Bitmap_File_Head.bfSize,Bitmap_Head.biClrUsed,Bitmap_Head.biBitCnt,Bitmap_Head.biWidth,
          Bitmap_Head.biHeight, Bitmap_Head.biCompr, SpeicherZeile);
#endif
  
  /* Get the Colormap */
  
  if (ReadColorMap(fd, ColorMap, ColormapSize, Maps, &Grey) == -1) return -1;
  
#ifdef DEBUG
  printf("Colormap read\n");
#endif

  /* Get the Image and return the ID or -1 on error*/

  return(ReadImage(fd, Bitmap_Head.biWidth, Bitmap_Head.biHeight, ColorMap, 
      Bitmap_Head.biClrUsed, Bitmap_Head.biBitCnt, Bitmap_Head.biCompr, SpeicherZeile, Grey));

}

gint ReadColorMap (fd, buffer, number, size, grey)
     FILE *fd;
     int number;
     unsigned char buffer[256][3];
     int size;
     int *grey;
{
  int i;
  unsigned char rgb[4];
  
  *grey=(number>2);
  for (i = 0; i < number ; i++)
    {
      if (!ReadOK (fd, rgb, size))
	{
	  printf ("%s: bad colormap\n", prog_name);
	  return -1;
	}
      
      /* Bitmap save the colors in another order! But change only once! */

      if (size==4) {      
      buffer[i][0] = rgb[2];
      buffer[i][1] = rgb[1];
      buffer[i][2] = rgb[0];
      } else {
      /* this one is for old os2 Bitmaps, but it dosn't work well */
      buffer[i][0] = rgb[1];
      buffer[i][1] = rgb[0];
      buffer[i][2] = rgb[2];
      }
      *grey=((*grey) && (rgb[0]==rgb[1]) && (rgb[1]==rgb[2]));
    }
  return(0);
}

Image ReadImage (fd, len, height, cmap, ncols, bpp, compression, spzeile, grey)
     FILE *fd;
     int len, height;
     unsigned char cmap[256][3];
     int ncols, bpp, compression, spzeile, grey;
{
  char *name_buf;
  unsigned char v,wieviel;
  GPixelRgn pixel_rgn;
  char buf[16];
  int xpos = 0, ypos = 0;
  Image image;
  gint32 layer;
  GDrawable *drawable;
  guchar *dest, *temp;
  guchar gimp_cmap[768];
  long rowstride, channels;
  int i, j, cur_progress, max_progress, egal;
  
  /* Make a new image in the gimp */
  
  if (grey)
    {
      image = gimp_image_new (len, height, GRAY);
      layer = gimp_layer_new (image, "Background", len, height, GRAY_IMAGE, 100, NORMAL_MODE);
      channels = 1;
    }
  else 
    {
      if (bpp<24) 
	{
	  image = gimp_image_new (len, height, INDEXED);
	  layer = gimp_layer_new (image, "Background", len, height, INDEXED_IMAGE, 100, NORMAL_MODE);
	  channels = 1;
	}
      else 
	{
	  image = gimp_image_new (len, height, RGB);
	  layer = gimp_layer_new (image, "Background", len, height, RGB_IMAGE, 100, NORMAL_MODE);
	  channels = 3;
	}
    }
  
  name_buf = g_malloc (strlen (filename) + 10);
  sprintf (name_buf, "%s", filename);
  gimp_image_set_filename(image,name_buf);
  free (name_buf);
  
  gimp_image_add_layer(image,layer,0);
  drawable = gimp_drawable_get(layer);
  
  dest = g_malloc(drawable->width*drawable->height*channels);
  rowstride = drawable->width * channels;
  
  ypos=height-1;		/* Bitmaps begin in the lower left corner */
  cur_progress = 0;
  max_progress = height;
  
  if (bpp==24)
    {
      while (ReadOK(fd,buf,3))
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
              egal=ReadOK(fd,buf,spzeile-(len*3));
              ypos--;
              xpos=0;
              cur_progress++;
              if ((interactive_bmp) &&((cur_progress % 5) == 0))
                gimp_progress_update ((double) cur_progress / (double) max_progress);
	    }
	  if (ypos < 0) break;
        }
    }
  else { switch(compression)
    {
    case 0:  						/* uncompressed */
      {
	while (ReadOK(fd,&v,1))
	  {
          for (i=1;(i<=(8/bpp)) && (xpos<len);i++,xpos++)
            {
              temp = dest + (ypos * rowstride) + (xpos * channels);
              /* look at my bitmask !! */
              *temp=( v & ( ((1<<bpp)-1) << (8-(i*bpp)) ) ) >> (8-(i*bpp));
            }
          if (xpos == len)
            {
              egal=ReadOK(fd,buf,(spzeile-len)/(8/bpp));
              ypos--;
              xpos=0;
              cur_progress++;
              if ((interactive_bmp) && ((cur_progress % 5) == 0))
                gimp_progress_update ((double) cur_progress / (double) max_progress);
    	}
	  if (ypos < 0) break;
	  }
	break;
      }
    default:						/* Compressed images */
      {
	while (TRUE)
	  {
	    egal=ReadOK(fd,buf,2);
	    if ((unsigned char) buf[0]!=0) 
	      /* Count + Color - record */
	      {
		for (j=0;((unsigned char) j < (unsigned char) buf[0]) && (xpos<len);)
		  {
#ifdef DEBUG2
		    printf("%u %u | ",xpos,len);
#endif
		    for (i=1;((i<=(8/bpp)) && (xpos<len) && ((unsigned char) j < (unsigned char) buf[0]));i++,xpos++,j++)
		      {
			temp = dest + (ypos * rowstride) + (xpos * channels);
			*temp=( buf[1] & ( ((1<<bpp)-1) << (8-(i*bpp)) ) ) >> (8-(i*bpp));
		      }
		  }
	      }
	    if (((unsigned char) buf[0]==0) && ((unsigned char) buf[1]>2))
	      /* uncompressed record */
	      {
		wieviel=buf[1];
		for (j=0;j<wieviel;j+=(8/bpp))
		  {
		    egal=ReadOK(fd,&v,1);
		    i=1;
		    while ((i<=(8/bpp)) && (xpos<len))
		      {
			temp = dest + (ypos * rowstride) + (xpos * channels);
			*temp=(v & ( ((1<<bpp)-1) << (8-(i*bpp)) ) ) >> (8-(i*bpp));
			i++;
			xpos++;
		      }
		  }
		if ( (wieviel / (8/bpp)) % 2) egal=ReadOK(fd,&v,1);
		/*if odd(x div (8 div bpp )) then blockread(f,z^,1);*/
	      }
	    if (((unsigned char) buf[0]==0) && ((unsigned char) buf[1]==0))
	      /* Zeilenende */
	      {
		ypos--;
		xpos=0;
		cur_progress++;
		if ((interactive_bmp) && ((cur_progress % 5) == 0))
		  gimp_progress_update ((double) cur_progress / (double)  max_progress);
	      }
	    if (((unsigned char) buf[0]==0) && ((unsigned char) buf[1]==1))
	      /* Bitmapende */
	      {
		break;
	      }
	    if (((unsigned char) buf[0]==0) && ((unsigned char) buf[1]==2))
	      /* Deltarecord */
	      {
		xpos+=(unsigned char) buf[2];
		ypos+=(unsigned char) buf[3];
	      }
	  }
	break;
      }
    }
  }    
  
  fclose(fd);
  if (bpp<24) for (i = 0, j = 0; i < ncols; i++)
    {
      gimp_cmap[j++] = cmap[i][0];
      gimp_cmap[j++] = cmap[i][1];
      gimp_cmap[j++] = cmap[i][2];
    }
  
  if (interactive_bmp) gimp_progress_update (1);
  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, FALSE);
  gimp_pixel_rgn_set_rect(&pixel_rgn, dest, 0, 0, drawable->width, drawable->height);
  if (bpp<24) gimp_image_set_cmap(image, gimp_cmap, ncols);
  gimp_drawable_flush(drawable);
  gimp_drawable_detach(drawable);
  g_free(dest);
  return(image);
  
}
