/* bmpwrite.c	Writes Bitmap files. Even RLE encoded ones.	 */
/*		(Windows (TM) doesn't read all of those, but who */
/*		cares? ;-)					 */
/*              I changed a few things over the time, so perhaps */
/*              it dos now, but now there's no Windows left on   */
/*              my computer...                                   */

/* Alexander.Schulz@stud.uni-karlsruhe.de			 */

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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
 
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include "bmp.h"
#include "libgimp/stdplugins-intl.h"

guchar *pixels;
int cur_progress;

int max_progress;

typedef struct
{
  gint run;
} BMPSaveInterface;

static BMPSaveInterface gsint =
{
  FALSE   /*  run  */
};

int encoded = 0;


static gint  save_dialog          (void);
static void  save_close_callback  (GtkWidget *widget, gpointer data);
static void  save_ok_callback     (GtkWidget *widget, gpointer data);
static void  save_toggle_update   (GtkWidget *widget, gpointer data);

gint
WriteBMP (char   *filename,
	  gint32  image,
	  gint32  drawable_ID)
{
  FILE *outfile;
  int Red[MAXCOLORS];
  int Green[MAXCOLORS];
  int Blue[MAXCOLORS];
  unsigned char *cmap;
  int rows, cols, Spcols, channels, MapSize, SpZeile;
  long BitsPerPixel;
  int colors;
  char *temp_buf;
  guchar *pixels;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  guchar puffer[50];
  int i;

  /* first: can we save this image? */
  
  drawable = gimp_drawable_get(drawable_ID);
  drawable_type = gimp_drawable_type(drawable_ID);
  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);
  switch (drawable_type)
    {
    case RGB_IMAGE:
    case GRAY_IMAGE:
    case INDEXED_IMAGE:
      break;
    default:
      g_message(_("bmp: cannot operate on unknown image types or alpha images"));
      gimp_quit ();
      break;
    }

  /* We can save it. So what colors do we use? */

  switch (drawable_type)
    {
    case RGB_IMAGE:
      colors = 0;
      BitsPerPixel = 24;
      MapSize = 0;
      channels = 3;
      break;
    case GRAY_IMAGE:
      colors = 256;
      BitsPerPixel=8;
      MapSize=1024;
      channels = 1;
      for (i = 0; i < colors; i++)
	{
	  Red[i] = i;
	  Green[i] = i;
	  Blue[i] = i;
	}
      break;
    case INDEXED_IMAGE:
      cmap = gimp_image_get_cmap (image, &colors);
      MapSize = 4*colors;
      channels = 1;
      if (colors>16) BitsPerPixel=8; else if (colors>2) BitsPerPixel=4;
         else BitsPerPixel=1;
      
      for (i = 0; i < colors; i++)
	{
	  Red[i] = *cmap++;
	  Green[i] = *cmap++;
	  Blue[i] = *cmap++;
	}
      break;
    default:
      fprintf (stderr, "%s: This should not happen\n", prog_name);
      return FALSE;
    }

  /* Perhaps someone wants RLE encoded Bitmaps */
    encoded = 0;
    if ((BitsPerPixel == 8 || BitsPerPixel == 4) && interactive_bmp)
      {
	if (! save_dialog ())
	  return -1;
      }
    
  /* Let's take some file */  
  outfile = fopen (filename, "wb");
  if (!outfile)
    {
      g_message (_("can't open %s\n"), filename);
      return -1;
    }
  
  /* fetch the image */
  
  pixels = (guchar *) g_malloc(drawable->width*drawable->height*channels);
  gimp_pixel_rgn_get_rect(&pixel_rgn, pixels, 0, 0, drawable->width, drawable->height);
  
  /* And let's begin the progress */
  
  if (interactive_bmp)
    {
      temp_buf = g_strdup_printf (_("Saving %s:"), filename);
      gimp_progress_init (temp_buf);
      g_free (temp_buf);
    }
  cur_progress = 0;
  max_progress = drawable->height;
  
  /* Now, we need some further information ... */
  
  cols = drawable->width;
  rows = drawable->height;
  
  /* ... that we write to our headers. */
  
  if ((BitsPerPixel != 24) && (cols % (8/BitsPerPixel))) Spcols=(((cols / (8/BitsPerPixel))+1)*(8/BitsPerPixel)); else Spcols=cols;
  if ((((Spcols*BitsPerPixel)/8) % 4) == 0) SpZeile=((Spcols*BitsPerPixel)/8);
  else SpZeile=((int)(((Spcols*BitsPerPixel)/8)/4)+1)*4;
  Bitmap_File_Head.bfSize=0x36+MapSize+(rows*SpZeile);
  Bitmap_File_Head.reserverd=0;
  Bitmap_File_Head.bfOffs=0x36+MapSize;
  Bitmap_File_Head.biSize=40;		
  Bitmap_Head.biWidth=cols;
  Bitmap_Head.biHeight=rows;
  Bitmap_Head.biPlanes=1;
  Bitmap_Head.biBitCnt=BitsPerPixel;
  if (encoded==0) Bitmap_Head.biCompr=0;
  else if (BitsPerPixel==8) Bitmap_Head.biCompr=1;
  else if (BitsPerPixel==4) Bitmap_Head.biCompr=2;
  else Bitmap_Head.biCompr=0;
  Bitmap_Head.biSizeIm=SpZeile*rows;
#ifdef GIMP_HAVE_RESOLUTION_INFO
  {
    double xresolution;
    double yresolution;
    gimp_image_get_resolution (image, &xresolution, &yresolution);

    if (xresolution > 1e-5 && yresolution > 1e-5) {
	/*
	 * xresolution and yresolution are in dots per inch.
	 * the BMP spec says that biXPels and biYPels are in
	 * pixels per meter as long ints (actually, "DWORDS"),
	 * so...
	 *    n dots    inch     100 cm   m dots
	 *    ------ * ------- * ------ = ------
	 *     inch    2.54 cm     m       inch
	 */
        Bitmap_Head.biXPels = (long int) xresolution * 100.0 / 2.54;
        Bitmap_Head.biYPels = (long int) yresolution * 100.0 / 2.54;
      }
  }
#else /* GIMP_HAVE_RESOLUTION_INFO */
  Bitmap_Head.biXPels = 1;
  Bitmap_Head.biYPels = 1;
#endif /* GIMP_HAVE_RESOLUTION_INFO */
  if (BitsPerPixel<24) 
    Bitmap_Head.biClrUsed=colors;
  else 
    Bitmap_Head.biClrUsed=0;

  Bitmap_Head.biClrImp=Bitmap_Head.biClrUsed;
  
#ifdef DEBUG
  printf("\nSize: %u, Colors: %u, Bits: %u, Width: %u, Height: %u, Comp: %u, Zeile: %u\n",
         (int)Bitmap_File_Head.bfSize,(int)Bitmap_Head.biClrUsed,Bitmap_Head.biBitCnt,(int)Bitmap_Head.biWidth,
         (int)Bitmap_Head.biHeight, (int)Bitmap_Head.biCompr,SpZeile);
#endif
  
  /* And now write the header and the colormap (if any) to disk */
  
  Write(outfile,"BM",2);

  FromL(Bitmap_File_Head.bfSize,&puffer[0x00]);
  FromL(Bitmap_File_Head.reserverd,&puffer[0x04]);
  FromL(Bitmap_File_Head.bfOffs,&puffer[0x08]);
  FromL(Bitmap_File_Head.biSize,&puffer[0x0C]);

  Write(outfile,puffer,16);

  FromL(Bitmap_Head.biWidth,&puffer[0x00]);
  FromL(Bitmap_Head.biHeight,&puffer[0x04]);
  FromS(Bitmap_Head.biPlanes,&puffer[0x08]);
  FromS(Bitmap_Head.biBitCnt,&puffer[0x0A]);
  FromL(Bitmap_Head.biCompr,&puffer[0x0C]);
  FromL(Bitmap_Head.biSizeIm,&puffer[0x10]);
  FromL(Bitmap_Head.biXPels,&puffer[0x14]);
  FromL(Bitmap_Head.biYPels,&puffer[0x18]);
  FromL(Bitmap_Head.biClrUsed,&puffer[0x1C]);
  FromL(Bitmap_Head.biClrImp,&puffer[0x20]);

  Write(outfile,puffer,36);
  WriteColorMap(outfile,Red,Green,Blue,MapSize);
  
  /* After that is done, we write the image ... */
  
  WriteImage(outfile, pixels, cols, rows, encoded, channels, BitsPerPixel, SpZeile, MapSize);

  /* ... and exit normally */
  
  fclose(outfile);
  gimp_drawable_detach(drawable);
  g_free(pixels);
  return TRUE;
}

void 
WriteColorMap (FILE *f, 
	       int   red[MAXCOLORS], 
	       int   green[MAXCOLORS],
	       int   blue[MAXCOLORS], 
	       int   size)
{
  char trgb[4];
  int i;
  
  size=size/4;
  trgb[3]=0;
  for (i=0;i<size;i++)
    {
      trgb[0]=(unsigned char) blue[i];
      trgb[1]=(unsigned char) green[i];
      trgb[2]=(unsigned char) red[i];
      Write(f,trgb,4);
    }
}

void 
WriteImage (FILE   *f, 
	    guchar *src, 
	    int     width, 
	    int     height,
	    int     encoded, 
	    int     channels, 
	    int     bpp, 
	    int     spzeile, 
	    int     MapSize)
{
  guchar buf[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0};
  guchar puffer[8];
  guchar *temp,v;
  guchar *Zeile,*ketten;
  int xpos,ypos,i,j,rowstride,laenge,thiswidth;
  int breite, n, k;

  xpos=0;
  rowstride=width*channels;
  
  /* We'll begin with the 24 bit Bitmaps, they are easy :-) */
  
  if (bpp==24)
    {
      for (ypos=height-1;ypos>=0;ypos--)	/* for each row   */
	{
	  for (i=0;i<width;i++)			/* for each pixel */
	    {
	      temp = src + (ypos * rowstride) + (xpos * channels);
	      buf[2]=(unsigned char) *temp;
	      temp++;
	      buf[1]=(unsigned char) *temp;
	      temp++;
	      buf[0]=(unsigned char) *temp;
	      xpos++;
	      Write(f,buf,3);
	    }
	  Write(f,&buf[3],spzeile-(width*3));
	  cur_progress++;
	  if ((interactive_bmp) && ((cur_progress % 5) == 0)) gimp_progress_update ((double) cur_progress / (double) max_progress);
	  xpos=0;
	}
    } else {
      switch(encoded)				/* now it gets more difficult */
	{						/* uncompressed 1,4 and 8 bit */
	case 0:
	  {
	    thiswidth=(width/(8/bpp));
	    if (width % (8/bpp)) thiswidth++;
	    for (ypos=height-1;ypos>=0;ypos--)	/* for each row		      */
	      {
		for (xpos=0;xpos<width;)		/* for each _byte_	      */
		  {
		    v=0;
		    for (i=1;(i<=(8/bpp)) && (xpos<width);i++,xpos++)	/* for each pixel */
		      {
			temp = src + (ypos * rowstride) + (xpos * channels);
			v=v | ((unsigned char) *temp << (8-(i*bpp)));
		      }
		    Write(f,&v,1);
		  }
		Write(f,&buf[3],spzeile-thiswidth);
		xpos=0;
		cur_progress++;
		if ((interactive_bmp) && ((cur_progress % 5) == 0))
		  gimp_progress_update ((double) cur_progress / (double) max_progress);
	      }
	    break;
	  }
	default:
	  {						 /* Save RLE encoded file, quite difficult */
	    laenge = 0;
	    buf[12] = 0;
	    buf[13] = 1;
	    buf[14] = 0;
	    buf[15] = 0;
	    Zeile = (guchar *) g_malloc (width / (8 / bpp) + 10);
	    ketten = (guchar *) g_malloc (width / (8 / bpp) + 10);
	    for (ypos = height - 1; ypos >= 0; ypos--) {	/* each row separately */
	      /*printf("Line: %i\n",ypos); */
	      j = 0;
	      /* first copy the pixels to a buffer, making one byte from two 4bit pixels */
	      for (xpos = 0; xpos < width;) {
		v = 0;
		for (i = 1; (i <= (8 / bpp)) && (xpos < width); i++, xpos++) {	/* for each pixel */
		  temp = src + (ypos * rowstride) + (xpos * channels);
		  v = v | ((guchar) * temp << (8 - (i * bpp)));
		}
		Zeile[j++] = v;
	      }
	      breite = width / (8 / bpp);
	      if (width % (8 / bpp))
		breite++;
	      /* then check for strings of equal bytes */
	      for (i = 0; i < breite;) {
		j = 0;
		while ((i + j < breite) && (j < (255 / (8 / bpp))) && (Zeile[i + j] == Zeile[i]))
		  j++;
		ketten[i] = j;
		/*printf("%i:",ketten[i]); */
		i += j;
	      }
	      /*printf("\n"); */
	      /* then write the strings and the other pixels to the file */
	      for (i = 0; i < breite;) {
		if (ketten[i] < 3)
		  /* strings of different pixels ... */
		  {
		    j = 0;
		    while ((i + j < breite) && (j < (255 / (8 / bpp))) && (ketten[i + j] < 3))
		      j += ketten[i + j];
		    /* this can only happen if j jumps over the end with a 2 in ketten[i+j] */
		    if (j > (255 / (8 / bpp)))
		      j -= 2;
		    /* 00 01 and 00 02 are reserved */
		    if (j > 2) {
		      Write (f, &buf[12], 1);
		      n = j * (8 / bpp);
		      if (n + i * (8 / bpp) > width)
			n--;
		      Write (f, &n, 1);
		      laenge += 2;
		      Write (f, &Zeile[i], j);
		      /*printf("0.%i.",n); */
		      /*for (k=j;k;k--) printf("#"); */
		      laenge += j;
		      if ((j) % 2) {
			Write (f, &buf[12], 1);
			laenge++;			 /*printf("0"); */
		      }
		      /*printf("|"); */
		    } else {
		      for (k = i; k < i + j; k++) {
			n = (8 / bpp);
			if (n + i * (8 / bpp) > width)
			  n--;
			Write (f, &n, 1);
			Write (f, &Zeile[k], 1);
			/*printf("%i.#|",n); */
			laenge += 2;
		      }
		    }
		    i += j;
		  } else
		    /* strings of equal pixels */
		    {
		      n = ketten[i] * (8 / bpp);
		      if (n + i * (8 / bpp) > width)
			n--;
		      Write (f, &n, 1);
		      Write (f, &Zeile[i], 1);
		      /*printf("%i.#|",n); */
		      i += ketten[i];
		      laenge += 2;
		    }
	      }
	      /*printf("\n"); */
	      Write (f, &buf[14], 2);		 /* End of row */
	      laenge += 2;
	      cur_progress++;
	      if ((interactive_bmp) && ((cur_progress % 5) == 0))
		gimp_progress_update ((double)cur_progress / max_progress);
	    }
	    fseek (f, -2, SEEK_CUR);		 /* Overwrite last End of row ... */
	    Write (f, &buf[12], 2);			 /* ... with End of file */

	    fseek (f, 0x22, SEEK_SET);               /* Write length of image */
	    FromL (laenge, puffer);
	    Write (f, puffer, 4);
	    fseek (f, 0x02, SEEK_SET);               /* Write length of file */
	    laenge += (0x36 + MapSize);
	    FromL (laenge, puffer);
	    Write (f, puffer, 4);
	    g_free (ketten);
	    g_free (Zeile);
	    break;
	  }
	}
    }
  if (interactive_bmp) gimp_progress_update(1);
}


/* These ones are just copied from the GIF-plugin */

static gint
save_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *hbbox;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  
  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("Save as BMP"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      (GtkSignalFunc) save_close_callback,
                      NULL);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) save_ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Save Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label (_("RLE encoded"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) save_toggle_update,
                      &encoded);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), encoded);
  gtk_widget_show (toggle);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();
  return gsint.run;
}

static void
save_close_callback (GtkWidget *widget,
                     gpointer   data)
{
  gtk_main_quit ();
}
                       
static void
save_ok_callback (GtkWidget *widget,
                  gpointer   data)
{
  gsint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}
                                             
static void
save_toggle_update (GtkWidget *widget,
                    gpointer   data)
{
  int *toggle_val;
                                                                      
  toggle_val = (int *) data;
                                                                        
  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE; 
  else
    *toggle_val = FALSE;
}
