/* bmpwrite.c   Writes Bitmap files. Even RLE encoded ones.      */
/*              (Windows (TM) doesn't read all of those, but who */
/*              cares? ;-)                                       */
/*              I changed a few things over the time, so perhaps */
/*              it dos now, but now there's no Windows left on   */
/*              my computer...                                   */

/* Alexander.Schulz@stud.uni-karlsruhe.de                        */

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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "bmp.h"

#include "libgimp/stdplugins-intl.h"

static gint cur_progress = 0;
static gint max_progress = 0;
static gint encoded      = 0;


static  void      WriteImage  (FILE   *f,
                               guchar *src,
                               gint    width,
                               gint    height,
                               gint    encoded,
                               gint    channels,
                               gint    bpp,
                               gint    spzeile,
                               gint    MapSize);
static  gboolean  save_dialog (void);


static void
FromL (gint32  wert,
       guchar *bopuffer)
{
  bopuffer[0] = (wert & 0x000000ff)>>0x00;
  bopuffer[1] = (wert & 0x0000ff00)>>0x08;
  bopuffer[2] = (wert & 0x00ff0000)>>0x10;
  bopuffer[3] = (wert & 0xff000000)>>0x18;
}

static void
FromS (gint16  wert,
       guchar *bopuffer)
{
  bopuffer[0] = (wert & 0x00ff)>>0x00;
  bopuffer[1] = (wert & 0xff00)>>0x08;
}

static void
WriteColorMap (FILE *f,
               gint  red[MAXCOLORS],
               gint  green[MAXCOLORS],
               gint  blue[MAXCOLORS],
               gint  size)
{
  gchar trgb[4];
  gint i;

  size /= 4;
  trgb[3] = 0;
  for (i = 0; i < size; i++)
    {
      trgb[0] = (guchar) blue[i];
      trgb[1] = (guchar) green[i];
      trgb[2] = (guchar) red[i];
      Write (f, trgb, 4);
    }
}

GimpPDBStatusType
WriteBMP (const gchar *filename,
          gint32       image,
          gint32       drawable_ID)
{
  FILE         *outfile;
  gint          Red[MAXCOLORS];
  gint          Green[MAXCOLORS];
  gint          Blue[MAXCOLORS];
  guchar       *cmap;
  gint          rows, cols, Spcols, channels, MapSize, SpZeile;
  glong         BitsPerPixel;
  gint          colors;
  gchar        *temp_buf;
  guchar       *pixels;
  GimpPixelRgn  pixel_rgn;
  GimpDrawable *drawable;
  GimpImageType drawable_type;
  guchar        puffer[50];
  gint          i;

  /* first: can we save this image? */

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);

  gimp_pixel_rgn_init (&pixel_rgn, drawable,
                       0, 0, drawable->width, drawable->height, FALSE, FALSE);

  if (gimp_drawable_has_alpha (drawable_ID))
    {
      g_message (_("Cannot save images with alpha channel."));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* We can save it. So what colors do we use? */

  switch (drawable_type)
    {
    case GIMP_RGB_IMAGE:
      colors       = 0;
      BitsPerPixel = 24;
      MapSize      = 0;
      channels     = 3;
      break;
    case GIMP_GRAY_IMAGE:
      colors       = 256;
      BitsPerPixel = 8;
      MapSize      = 1024;
      channels     = 1;
      for (i = 0; i < colors; i++)
        {
          Red[i]   = i;
          Green[i] = i;
          Blue[i]  = i;
        }
      break;
    case GIMP_INDEXED_IMAGE:
      cmap     = gimp_image_get_colormap (image, &colors);
      MapSize  = 4 * colors;
      channels = 1;

      if (colors > 16)
        BitsPerPixel = 8;
      else if (colors > 2)
        BitsPerPixel = 4;
      else
        BitsPerPixel = 1;

      for (i = 0; i < colors; i++)
        {
          Red[i]   = *cmap++;
          Green[i] = *cmap++;
          Blue[i]  = *cmap++;
        }
      break;
    default:
      g_message (_("Cannot operate on unknown image types."));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* Perhaps someone wants RLE encoded Bitmaps */
  encoded = 0;
  if ((BitsPerPixel == 8 || BitsPerPixel == 4) && interactive_bmp)
    {
      if (! save_dialog ())
        return GIMP_PDB_CANCEL;
    }

  /* Let's take some file */
  outfile = fopen (filename, "wb");
  if (!outfile)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                  gimp_filename_to_utf8 (filename), g_strerror (errno));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* fetch the image */
  pixels = g_new (guchar, drawable->width * drawable->height * channels);
  gimp_pixel_rgn_get_rect (&pixel_rgn, pixels,
                           0, 0, drawable->width, drawable->height);

  /* And let's begin the progress */
  temp_buf = g_strdup_printf (_("Saving '%s'..."),
                               gimp_filename_to_utf8 (filename));
  gimp_progress_init (temp_buf);
  g_free (temp_buf);

  cur_progress = 0;
  max_progress = drawable->height;

  /* Now, we need some further information ... */
  cols = drawable->width;
  rows = drawable->height;

  /* ... that we write to our headers. */
  if ((BitsPerPixel != 24) &&
      (cols % (8/BitsPerPixel)))
    Spcols = (((cols / (8 / BitsPerPixel)) + 1) * (8 / BitsPerPixel));
  else
    Spcols = cols;

  if ((((Spcols * BitsPerPixel) / 8) % 4) == 0)
    SpZeile = ((Spcols * BitsPerPixel) / 8);
  else
    SpZeile = ((gint) (((Spcols * BitsPerPixel) / 8) / 4) + 1) * 4;

  Bitmap_File_Head.bfSize    = 0x36 + MapSize + (rows * SpZeile);
  Bitmap_File_Head.zzHotX    = 0;
  Bitmap_File_Head.zzHotY    = 0;
  Bitmap_File_Head.bfOffs    = 0x36 + MapSize;
  Bitmap_File_Head.biSize    = 40;

  Bitmap_Head.biWidth  = cols;
  Bitmap_Head.biHeight = rows;
  Bitmap_Head.biPlanes = 1;
  Bitmap_Head.biBitCnt = BitsPerPixel;

  if (encoded == 0)
    Bitmap_Head.biCompr = 0;
  else if (BitsPerPixel == 8)
    Bitmap_Head.biCompr = 1;
  else if (BitsPerPixel == 4)
    Bitmap_Head.biCompr = 2;
  else
    Bitmap_Head.biCompr = 0;

  Bitmap_Head.biSizeIm = SpZeile * rows;

  {
    gdouble xresolution;
    gdouble yresolution;
    gimp_image_get_resolution (image, &xresolution, &yresolution);

    if (xresolution > GIMP_MIN_RESOLUTION &&
        yresolution > GIMP_MIN_RESOLUTION)
      {
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

  if (BitsPerPixel < 24)
    Bitmap_Head.biClrUsed = colors;
  else
    Bitmap_Head.biClrUsed = 0;

  Bitmap_Head.biClrImp = Bitmap_Head.biClrUsed;

#ifdef DEBUG
  printf("\nSize: %u, Colors: %u, Bits: %u, Width: %u, Height: %u, Comp: %u, Zeile: %u\n",
         (int)Bitmap_File_Head.bfSize,(int)Bitmap_Head.biClrUsed,Bitmap_Head.biBitCnt,(int)Bitmap_Head.biWidth,
         (int)Bitmap_Head.biHeight, (int)Bitmap_Head.biCompr,SpZeile);
#endif

  /* And now write the header and the colormap (if any) to disk */

  Write (outfile, "BM", 2);

  FromL (Bitmap_File_Head.bfSize, &puffer[0x00]);
  FromS (Bitmap_File_Head.zzHotX, &puffer[0x04]);
  FromS (Bitmap_File_Head.zzHotY, &puffer[0x06]);
  FromL (Bitmap_File_Head.bfOffs, &puffer[0x08]);
  FromL (Bitmap_File_Head.biSize, &puffer[0x0C]);

  Write (outfile, puffer, 16);

  FromL (Bitmap_Head.biWidth, &puffer[0x00]);
  FromL (Bitmap_Head.biHeight, &puffer[0x04]);
  FromS (Bitmap_Head.biPlanes, &puffer[0x08]);
  FromS (Bitmap_Head.biBitCnt, &puffer[0x0A]);
  FromL (Bitmap_Head.biCompr, &puffer[0x0C]);
  FromL (Bitmap_Head.biSizeIm, &puffer[0x10]);
  FromL (Bitmap_Head.biXPels, &puffer[0x14]);
  FromL (Bitmap_Head.biYPels, &puffer[0x18]);
  FromL (Bitmap_Head.biClrUsed, &puffer[0x1C]);
  FromL (Bitmap_Head.biClrImp, &puffer[0x20]);

  Write (outfile, puffer, 36);
  WriteColorMap (outfile, Red, Green, Blue, MapSize);

  /* After that is done, we write the image ... */

  WriteImage (outfile,
              pixels, cols, rows,
              encoded, channels, BitsPerPixel, SpZeile, MapSize);

  /* ... and exit normally */

  fclose (outfile);
  gimp_drawable_detach (drawable);
  g_free (pixels);

  return GIMP_PDB_SUCCESS;
}

static void
WriteImage (FILE   *f,
            guchar *src,
            gint    width,
            gint    height,
            gint    encoded,
            gint    channels,
            gint    bpp,
            gint    spzeile,
            gint    MapSize)
{
  guchar buf[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0};
  guchar puffer[8];
  guchar *temp, v;
  guchar *Zeile, *ketten;
  gint xpos, ypos, i, j, rowstride, laenge, thiswidth;
  gint breite, k;
  guchar n;

  xpos = 0;
  rowstride = width * channels;

  /* We'll begin with the 24 bit Bitmaps, they are easy :-) */

  if (bpp == 24)
    {
      for (ypos = height - 1; ypos >= 0; ypos--)  /* for each row   */
        {
          for (i = 0; i < width; i++)  /* for each pixel */
            {
              temp = src + (ypos * rowstride) + (xpos * channels);
              buf[2] = (guchar) *temp;
              temp++;
              buf[1] = (guchar) *temp;
              temp++;
              buf[0] = (guchar) *temp;
              xpos++;
              Write (f, buf, 3);
            }
          Write (f, &buf[3], spzeile - (width * 3));

          cur_progress++;
          if ((cur_progress % 5) == 0)
            gimp_progress_update ((gdouble) cur_progress /
                                  (gdouble) max_progress);

          xpos = 0;
        }
    }
  else
    {
      switch (encoded)  /* now it gets more difficult */
        {               /* uncompressed 1,4 and 8 bit */
        case 0:
          {
            thiswidth = (width / (8 / bpp));
            if (width % (8 / bpp))
              thiswidth++;

            for (ypos = height - 1; ypos >= 0; ypos--) /* for each row */
              {
                for (xpos = 0; xpos < width;)  /* for each _byte_ */
                  {
                    v = 0;
                    for (i = 1;
                         (i <= (8 / bpp)) && (xpos < width);
                         i++, xpos++)  /* for each pixel */
                      {
                        temp = src + (ypos * rowstride) + (xpos * channels);
                        v=v | ((guchar) *temp << (8 - (i * bpp)));
                      }
                    Write (f, &v, 1);
                  }
                Write (f, &buf[3], spzeile - thiswidth);
                xpos = 0;

                cur_progress++;
                if ((cur_progress % 5) == 0)
                  gimp_progress_update ((gdouble) cur_progress /
                                        (gdouble) max_progress);
              }
            break;
          }
        default:
          {              /* Save RLE encoded file, quite difficult */
            laenge = 0;
            buf[12] = 0;
            buf[13] = 1;
            buf[14] = 0;
            buf[15] = 0;
            Zeile = (guchar *) g_malloc (width / (8 / bpp) + 10);
            ketten = (guchar *) g_malloc (width / (8 / bpp) + 10);
            for (ypos = height - 1; ypos >= 0; ypos--)
              { /* each row separately */
                j = 0;
                /* first copy the pixels to a buffer,
                 * making one byte from two 4bit pixels
                 */
                for (xpos = 0; xpos < width;)
                  {
                    v = 0;
                    for (i = 1;
                         (i <= (8 / bpp)) && (xpos < width);
                         i++, xpos++)
                      { /* for each pixel */
                        temp = src + (ypos * rowstride) + (xpos * channels);
                        v = v | ((guchar) * temp << (8 - (i * bpp)));
                      }
                    Zeile[j++] = v;
                  }
                breite = width / (8 / bpp);
                if (width % (8 / bpp))
                  breite++;
                /* then check for strings of equal bytes */
                for (i = 0; i < breite;)
                  {
                    j = 0;
                    while ((i + j < breite) &&
                           (j < (255 / (8 / bpp))) &&
                           (Zeile[i + j] == Zeile[i]))
                      j++;

                    ketten[i] = j;
                    i += j;
                  }

                /* then write the strings and the other pixels to the file */
                for (i = 0; i < breite;)
                  {
                    if (ketten[i] < 3)
                      /* strings of different pixels ... */
                      {
                        j = 0;
                        while ((i + j < breite) &&
                               (j < (255 / (8 / bpp))) &&
                               (ketten[i + j] < 3))
                          j += ketten[i + j];

                        /* this can only happen if j jumps over
                         * the end with a 2 in ketten[i+j]
                         */
                        if (j > (255 / (8 / bpp)))
                          j -= 2;
                        /* 00 01 and 00 02 are reserved */
                        if (j > 2)
                          {
                            Write (f, &buf[12], 1);
                            n = j * (8 / bpp);
                            if (n + i * (8 / bpp) > width)
                              n--;
                            Write (f, &n, 1);
                            laenge += 2;
                            Write (f, &Zeile[i], j);
                            laenge += j;
                            if ((j) % 2)
                              {
                                Write (f, &buf[12], 1);
                                laenge++;
                              }
                          }
                        else
                          {
                            for (k = i; k < i + j; k++)
                              {
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
                      }
                    else
                      /* strings of equal pixels */
                      {
                        n = ketten[i] * (8 / bpp);
                        if (n + i * (8 / bpp) > width)
                          n--;
                        Write (f, &n, 1);
                        Write (f, &Zeile[i], 1);
                        i += ketten[i];
                        laenge += 2;
                      }
                  }
                Write (f, &buf[14], 2);          /* End of row */
                laenge += 2;

                cur_progress++;
                if ((cur_progress % 5) == 0)
                  gimp_progress_update ((gdouble) cur_progress /
                                        (gdouble) max_progress);
              }
            fseek (f, -2, SEEK_CUR);     /* Overwrite last End of row ... */
            Write (f, &buf[12], 2);      /* ... with End of file */

            fseek (f, 0x22, SEEK_SET);            /* Write length of image */
            FromL (laenge, puffer);
            Write (f, puffer, 4);
            fseek (f, 0x02, SEEK_SET);            /* Write length of file */
            laenge += (0x36 + MapSize);
            FromL (laenge, puffer);
            Write (f, puffer, 4);
            g_free (ketten);
            g_free (Zeile);
            break;
          }
        }
    }

  gimp_progress_update (1);
}

static gboolean
save_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *toggle;
  GtkWidget *vbox;
  gboolean   run;

  dlg = gimp_dialog_new (_("Save as BMP"), "bmp",
                         NULL, 0,
                         gimp_standard_help_func, "file-bmp-save",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  toggle = gtk_check_button_new_with_mnemonic (_("_RLE encoded"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), encoded);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &encoded);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}
