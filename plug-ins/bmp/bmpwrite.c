/* bmpwrite.c	Writes Bitmap files. Even RLE encoded ones.	 */
/*		(Windows (TM) doesn't read all of those, but who */
/*		cares? ;-)					 */
/* Alexander.Schulz@stud.uni-karlsruhe.de			 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include "bmp.h"

guchar *pixels;
int cur_progress;

int max_progress;

typedef struct
{
  gint run;
} GIFSaveInterface;

static GIFSaveInterface gsint =
{
  FALSE   /*  run  */
};

int encoded = 0;


static gint   save_dialog ();
static void   save_close_callback  (GtkWidget *widget,
                                    gpointer   data);
static void   save_ok_callback     (GtkWidget *widget,
                                    gpointer   data);
static void   save_toggle_update   (GtkWidget *widget,
                                    gpointer   data);

gint
WriteBMP (filename,image,drawable_ID)
     char *filename;
     gint32 image,drawable_ID;
{
  FILE *outfile;
  int Red[MAXCOLORS];
  int Green[MAXCOLORS];
  int Blue[MAXCOLORS];
  unsigned char *cmap;
  int rows, cols, channels, MapSize, SpZeile;
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
      printf("bmp: cannot operate on unknown image types or alpha images");
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
      fprintf (stderr, "%s: you should not receive this error for any reason\n", prog_name);
      break;
    }

  /* Perhaps someone wants RLE encoded Bitmaps */

    encoded = 0;
    if (((BitsPerPixel==8) || (BitsPerPixel==4)) && interactive_bmp) {if (! save_dialog ()) {return -1;}}

  
  /* Let's take some file */
  
  outfile = fopen (filename, "wb");
  if (!outfile)
    {
      fprintf (stderr, "can't open %s\n", filename);
      return -1;
    }
  
  /* fetch the image */
  
  pixels = (guchar *) g_malloc(drawable->width*drawable->height*channels);
  gimp_pixel_rgn_get_rect(&pixel_rgn, pixels, 0, 0, drawable->width, drawable->height);
  
  /* And let's begin the progress */
  
  if (interactive_bmp)
    {
      temp_buf = g_malloc (strlen (filename) + 11);
      sprintf (temp_buf, "Saving %s:", filename);
      gimp_progress_init (temp_buf);
      g_free (temp_buf);
    }
  cur_progress = 0;
  max_progress = drawable->height;
  
  /* Now, we need some further information ... */
  
  cols = drawable->width;
  rows = drawable->height;
  
  /* ... that we write to our headers. */
  
  if ((((cols*BitsPerPixel)/8) % 4) == 0) SpZeile=((cols*BitsPerPixel)/8);
  else SpZeile=((((cols*BitsPerPixel)/8)/4)+1)*4;
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
  Bitmap_Head.biSizeIm=cols*rows;
  Bitmap_Head.biXPels=1;
  Bitmap_Head.biYPels=1;
  if (BitsPerPixel<24) Bitmap_Head.biClrUsed=colors;
  else Bitmap_Head.biClrUsed=0;
  Bitmap_Head.biClrImp=Bitmap_Head.biClrUsed;
  
#ifdef DEBUG
  printf("\nSize: %u, Colors: %u, Bits: %u, Width: %u, Height: %u, Comp: %u, Zeile: %u\n",
         Bitmap_File_Head.bfSize,Bitmap_Head.biClrUsed,Bitmap_Head.biBitCnt,Bitmap_Head.biWidth,
         Bitmap_Head.biHeight, Bitmap_Head.biCompr,SpZeile);
#endif
  
  /* And now write the header and the colormap (if any) to disk */
  
  WriteOK(outfile,"BM",2);

  FromL(Bitmap_File_Head.bfSize,&puffer[0x00]);
  FromL(Bitmap_File_Head.reserverd,&puffer[0x04]);
  FromL(Bitmap_File_Head.bfOffs,&puffer[0x08]);
  FromL(Bitmap_File_Head.biSize,&puffer[0x0C]);

  WriteOK(outfile,puffer,16);

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

  WriteOK(outfile,puffer,36);
  WriteColorMap(outfile,Red,Green,Blue,MapSize);
  
  /* After that is done, we write the image ... */
  
  WriteImage(outfile, pixels, cols, rows, encoded, channels, BitsPerPixel, SpZeile);

  /* ... and exit normally */
  
  fclose(outfile);
  gimp_drawable_detach(drawable);
  g_free(pixels);
  return TRUE;
}

void WriteColorMap(FILE *f, int red[MAXCOLORS], int green[MAXCOLORS], 
                            int blue[MAXCOLORS], int size)
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
      WriteOK(f,trgb,4);
    }
}

void WriteImage(f, src, width, height, encoded, channels, bpp, spzeile)
  FILE *f;
  guchar *src;
  int width,height,encoded,channels,bpp,spzeile;
{
  guchar buf[16];
  char *temp,v,g;
  int xpos,ypos,i,j,rowstride,laenge;
  
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
          WriteOK(f,buf,3);
        }
        WriteOK(f,&buf[3],spzeile-(width*3));
        cur_progress++;
        if ((interactive_bmp) && ((cur_progress % 5) == 0)) gimp_progress_update ((double) cur_progress / (double) max_progress);
        xpos=0;
      }
    } else {
    switch(encoded)				/* now it gets more difficult */
      {						/* uncompressed 1,4 and 8 bit */
      case 0:
        {
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
            WriteOK(f,&v,1);
            }
          WriteOK(f,&buf[3],spzeile-(width/(8/bpp)));
          xpos=0;
          cur_progress++;
          if ((interactive_bmp) && ((cur_progress % 5) == 0))
          gimp_progress_update ((double) cur_progress / (double) max_progress);
          }
        break;
        }
      default:
        {			/* Save RLE encoded file, quite difficult */
        laenge=0;		/* and doesn't work completely		  */
        buf[12]=0;
        buf[13]=1;
        buf[14]=0;
        buf[15]=0;
        for (ypos=height-1;ypos>=0;ypos--)	/* each row separately */
          {
          for (xpos=0;xpos<width;)		/* loop for one row    */
            {
              temp = src + (ypos * rowstride) + (xpos * channels);
              buf[1] = (unsigned char) *temp;
              j=0;
              while (((unsigned char) *temp == (unsigned char) buf[1]) && (j<255) && (xpos<width))
                {
                xpos++;
                if (xpos<width) temp = src + (ypos * rowstride) + (xpos * channels);
                j++;
                }
              if (1) /* (j>2) */
                {
                buf[0]=(unsigned char) j;
                if (bpp==4) 
                  {
                  buf[1]=buf[1] | (buf[1] << 4);
                  }
                WriteOK(f,buf,2);		/* Count -- Color */
                laenge+=2;
                }
              else				/* unpacked */
                {
                xpos-=j;
                j=3;
                xpos+=j;
                v=*(src + (ypos * rowstride) + ((xpos-1) * channels));
                buf[0]=0;
                WriteOK(f,buf,1);
                laenge++;
                temp = src + (ypos * rowstride) + (xpos * channels);
                while ((j<255) && (xpos<width) && ((unsigned char) v != ((unsigned char) *temp)))
                  {
                  v=(unsigned char) *temp;
                  xpos++;
                  j++;
                  temp = src + (ypos * rowstride) + (xpos * channels);
                  }
                xpos-=j;
                buf[0]=(unsigned char) j;
                WriteOK(f,buf,1);
                laenge++;
                for (i=0;i<j;i+=(8/bpp),xpos+=(8/bpp))
                  {
                  temp = src + (ypos * rowstride) + (xpos * channels);
                  v=(unsigned char) *temp;
                  temp++;
                  g=(unsigned char) *temp;
                  if (bpp==4) v=v | (g << 4);
                  WriteOK(f,&v,1);
                  }
                laenge+=j/(8/bpp);
                if ((j/(8/bpp)) % 2) { WriteOK(f,&buf[12],1); laenge++; }
                }
            }
          WriteOK(f,&buf[14],2);		/* End of row */
          laenge+=2;
          cur_progress++;
          if ((interactive_bmp) &&((cur_progress % 5) == 0)) gimp_progress_update ((double) cur_progress / (double)  max_progress);
          }
        fseek(f,-2,SEEK_CUR);			/* Overwrite last End of row */
        WriteOK(f,&buf[12],2);			/* End of file */
        
        fseek(f,0x02,SEEK_SET);
        if (bpp==8) laenge+=0x36+(256*4); else laenge+=0x36+(16*4);
        WriteOK(f,&laenge,4);
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
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("bmp");

  gtk_init (&argc, &argv);

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Save as BMP");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      (GtkSignalFunc) save_close_callback,
                      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) save_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ("Save Options");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label ("RLE encoded");
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) save_toggle_update,
                      &encoded);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), encoded);
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
