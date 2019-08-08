/* bmpwrite.c   Writes Bitmap files. Even RLE encoded ones.      */
/*              (Windows (TM) doesn't read all of those, but who */
/*              cares? ;-)                                       */
/*              I changed a few things over the time, so perhaps */
/*              it dos now, but now there's no Windows left on   */
/*              my computer...                                   */

/* Alexander.Schulz@stud.uni-karlsruhe.de                        */

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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "bmp.h"
#include "bmp-save.h"

#include "libgimp/stdplugins-intl.h"


typedef enum
{
  RGB_565,
  RGBA_5551,
  RGB_555,
  RGB_888,
  RGBA_8888,
  RGBX_8888
} RGBMode;


static  void      write_image     (FILE   *f,
                                   guchar *src,
                                   gint    width,
                                   gint    height,
                                   gint    use_run_length_encoding,
                                   gint    channels,
                                   gint    bpp,
                                   gint    spzeile,
                                   gint    MapSize,
                                   RGBMode rgb_format,
                                   gint    mask_info_size,
                                   gint    color_space_size);

static  gboolean  save_dialog     (gint    channels);


static struct
{
  RGBMode rgb_format;
  gint    use_run_length_encoding;

  /* Whether or not to write BITMAPV5HEADER color space data */
  gint    dont_write_color_space_data;
} BMPSaveData;


static void
write_color_map (FILE *f,
                 gint  red[MAXCOLORS],
                 gint  green[MAXCOLORS],
                 gint  blue[MAXCOLORS],
                 gint  size)
{
  gchar trgb[4];
  gint  i;

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

static gboolean
warning_dialog (const gchar *primary,
                const gchar *secondary)
{
  GtkWidget *dialog;
  gboolean   ok;

  dialog = gtk_message_dialog_new (NULL, 0,
                                   GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL,
                                   "%s", primary);

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "%s", secondary);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  ok = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return ok;
}

GimpPDBStatusType
save_image (const gchar  *filename,
            gint32        image,
            gint32        drawable_ID,
            GimpRunMode   run_mode,
            GError      **error)
{
  FILE           *outfile;
  BitmapFileHead  bitmap_file_head;
  BitmapHead      bitmap_head;
  gint            Red[MAXCOLORS];
  gint            Green[MAXCOLORS];
  gint            Blue[MAXCOLORS];
  guchar         *cmap;
  gint            rows, cols, Spcols, channels, MapSize, SpZeile;
  glong           BitsPerPixel;
  gint            colors;
  guchar         *pixels;
  GeglBuffer     *buffer;
  const Babl     *format;
  GimpImageType    drawable_type;
  gint            drawable_width;
  gint            drawable_height;
  gint            i;
  gint            mask_info_size;
  gint            color_space_size;
  guint32         Mask[4];

  buffer = gimp_drawable_get_buffer (drawable_ID);

  drawable_type   = gimp_drawable_type   (drawable_ID);
  drawable_width  = gimp_drawable_width  (drawable_ID);
  drawable_height = gimp_drawable_height (drawable_ID);

  switch (drawable_type)
    {
    case GIMP_RGBA_IMAGE:
      format       = babl_format ("R'G'B'A u8");
      colors       = 0;
      BitsPerPixel = 32;
      MapSize      = 0;
      channels     = 4;
      BMPSaveData.rgb_format = RGBA_8888;
      break;

    case GIMP_RGB_IMAGE:
      format       = babl_format ("R'G'B' u8");
      colors       = 0;
      BitsPerPixel = 24;
      MapSize      = 0;
      channels     = 3;
      BMPSaveData.rgb_format = RGB_888;
      break;

    case GIMP_GRAYA_IMAGE:
      if (run_mode == GIMP_RUN_INTERACTIVE &&
          ! warning_dialog (_("Cannot export indexed image with "
                              "transparency in BMP file format."),
                            _("Alpha channel will be ignored.")))
        return GIMP_PDB_CANCEL;

     /* fallthrough */

    case GIMP_GRAY_IMAGE:
      colors       = 256;
      BitsPerPixel = 8;
      MapSize      = 1024;

      if (drawable_type == GIMP_GRAYA_IMAGE)
        {
          format   = babl_format ("Y'A u8");
          channels = 2;
        }
      else
        {
          format   = babl_format ("Y' u8");
          channels = 1;
        }

      for (i = 0; i < colors; i++)
        {
          Red[i]   = i;
          Green[i] = i;
          Blue[i]  = i;
        }
      break;

    case GIMP_INDEXEDA_IMAGE:
      if (run_mode == GIMP_RUN_INTERACTIVE &&
          ! warning_dialog (_("Cannot export indexed image with "
                              "transparency in BMP file format."),
                            _("Alpha channel will be ignored.")))
        return GIMP_PDB_CANCEL;

     /* fallthrough */

    case GIMP_INDEXED_IMAGE:
      format   = gimp_drawable_get_format (drawable_ID);
      cmap     = gimp_image_get_colormap (image, &colors);
      MapSize  = 4 * colors;

      if (drawable_type == GIMP_INDEXEDA_IMAGE)
        channels = 2;
      else
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
      g_assert_not_reached ();
    }

  BMPSaveData.use_run_length_encoding = 0;
  BMPSaveData.dont_write_color_space_data = 0;
  mask_info_size = 0;

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    {
      gimp_get_data (SAVE_PROC, &BMPSaveData);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      (BitsPerPixel == 8 ||
       BitsPerPixel == 4))
    {
      if (! save_dialog (1))
        return GIMP_PDB_CANCEL;
    }
  else if (BitsPerPixel == 24 ||
           BitsPerPixel == 32)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          if (! save_dialog (channels))
            return GIMP_PDB_CANCEL;
        }

      /* mask_info_size is only set to non-zero for 16- and 32-bpp */
      switch (BMPSaveData.rgb_format)
        {
        case RGB_888:
          BitsPerPixel = 24;
          break;
        case RGBA_8888:
          BitsPerPixel   = 32;
          mask_info_size = 16;
          break;
        case RGBX_8888:
          BitsPerPixel   = 32;
          mask_info_size = 16;
          break;
        case RGB_565:
          BitsPerPixel   = 16;
          mask_info_size = 16;
          break;
        case RGBA_5551:
          BitsPerPixel   = 16;
          mask_info_size = 16;
          break;
        case RGB_555:
          BitsPerPixel   = 16;
          mask_info_size = 16;
          break;
        default:
          g_return_val_if_reached (GIMP_PDB_EXECUTION_ERROR);
        }
    }

  gimp_set_data (SAVE_PROC, &BMPSaveData, sizeof (BMPSaveData));

  /* Let's begin the progress */
  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_filename_to_utf8 (filename));

  /* Let's take some file */
  outfile = g_fopen (filename, "wb");
  if (!outfile)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* fetch the image */
  pixels = g_new (guchar, drawable_width * drawable_height * channels);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0,
                                           drawable_width, drawable_height), 1.0,
                   format, pixels,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (buffer);

  /* Now, we need some further information ... */
  cols = drawable_width;
  rows = drawable_height;

  /* ... that we write to our headers. */
  if ((BitsPerPixel <= 8) && (cols % (8 / BitsPerPixel)))
    Spcols = (((cols / (8 / BitsPerPixel)) + 1) * (8 / BitsPerPixel));
  else
    Spcols = cols;

  if ((((Spcols * BitsPerPixel) / 8) % 4) == 0)
    SpZeile = ((Spcols * BitsPerPixel) / 8);
  else
    SpZeile = ((gint) (((Spcols * BitsPerPixel) / 8) / 4) + 1) * 4;

  if (! BMPSaveData.dont_write_color_space_data)
    color_space_size = 68;
  else
    color_space_size = 0;

  bitmap_file_head.bfSize    = (0x36 + MapSize + (rows * SpZeile) +
                                mask_info_size + color_space_size);
  bitmap_file_head.zzHotX    =  0;
  bitmap_file_head.zzHotY    =  0;
  bitmap_file_head.bfOffs    = (0x36 + MapSize +
                                mask_info_size + color_space_size);
  bitmap_file_head.biSize    =  40 + mask_info_size + color_space_size;

  bitmap_head.biWidth  = cols;
  bitmap_head.biHeight = rows;
  bitmap_head.biPlanes = 1;
  bitmap_head.biBitCnt = BitsPerPixel;

  if (BMPSaveData.use_run_length_encoding == 0)
    {
      if (mask_info_size > 0)
        bitmap_head.biCompr = 3; /* BI_BITFIELDS */
      else
        bitmap_head.biCompr = 0; /* BI_RGB */
    }
  else if (BitsPerPixel == 8)
    {
      bitmap_head.biCompr = 1;
    }
  else if (BitsPerPixel == 4)
    {
      bitmap_head.biCompr = 2;
    }
  else
    {
      bitmap_head.biCompr = 0;
    }

  bitmap_head.biSizeIm = SpZeile * rows;

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
         *
         * We add 0.5 for proper rounding.
         */
        bitmap_head.biXPels = (long int) (xresolution * 100.0 / 2.54 + 0.5);
        bitmap_head.biYPels = (long int) (yresolution * 100.0 / 2.54 + 0.5);
      }
  }

  if (BitsPerPixel <= 8)
    bitmap_head.biClrUsed = colors;
  else
    bitmap_head.biClrUsed = 0;

  bitmap_head.biClrImp = bitmap_head.biClrUsed;

#ifdef DEBUG
  printf ("\nSize: %u, Colors: %u, Bits: %u, Width: %u, Height: %u, Comp: %u, Zeile: %u\n",
          (int)bitmap_file_head.bfSize,
          (int)bitmap_head.biClrUsed,
          bitmap_head.biBitCnt,
          (int)bitmap_head.biWidth,
          (int)bitmap_head.biHeight,
          (int)bitmap_head.biCompr,SpZeile);
#endif

  /* And now write the header and the colormap (if any) to disk */

  Write (outfile, "BM", 2);

  bitmap_file_head.bfSize = GUINT32_TO_LE (bitmap_file_head.bfSize);
  bitmap_file_head.zzHotX = GUINT16_TO_LE (bitmap_file_head.zzHotX);
  bitmap_file_head.zzHotY = GUINT16_TO_LE (bitmap_file_head.zzHotY);
  bitmap_file_head.bfOffs = GUINT32_TO_LE (bitmap_file_head.bfOffs);
  bitmap_file_head.biSize = GUINT32_TO_LE (bitmap_file_head.biSize);

  Write (outfile, &bitmap_file_head.bfSize, 16);

  bitmap_head.biWidth   = GINT32_TO_LE  (bitmap_head.biWidth);
  bitmap_head.biHeight  = GINT32_TO_LE  (bitmap_head.biHeight);
  bitmap_head.biPlanes  = GUINT16_TO_LE (bitmap_head.biPlanes);
  bitmap_head.biBitCnt  = GUINT16_TO_LE (bitmap_head.biBitCnt);
  bitmap_head.biCompr   = GUINT32_TO_LE (bitmap_head.biCompr);
  bitmap_head.biSizeIm  = GUINT32_TO_LE (bitmap_head.biSizeIm);
  bitmap_head.biXPels   = GUINT32_TO_LE (bitmap_head.biXPels);
  bitmap_head.biYPels   = GUINT32_TO_LE (bitmap_head.biYPels);
  bitmap_head.biClrUsed = GUINT32_TO_LE (bitmap_head.biClrUsed);
  bitmap_head.biClrImp  = GUINT32_TO_LE (bitmap_head.biClrImp);

  Write (outfile, &bitmap_head, 36);

  if (mask_info_size > 0)
    {
      switch (BMPSaveData.rgb_format)
        {
        default:
        case RGB_888:
        case RGBX_8888:
          Mask[0] = 0x00ff0000;
          Mask[1] = 0x0000ff00;
          Mask[2] = 0x000000ff;
          Mask[3] = 0x00000000;
          break;

        case RGBA_8888:
          Mask[0] = 0x00ff0000;
          Mask[1] = 0x0000ff00;
          Mask[2] = 0x000000ff;
          Mask[3] = 0xff000000;
          break;

        case RGB_565:
          Mask[0] = 0xf800;
          Mask[1] = 0x7e0;
          Mask[2] = 0x1f;
          Mask[3] = 0x0;
          break;

        case RGBA_5551:
          Mask[0] = 0x7c00;
          Mask[1] = 0x3e0;
          Mask[2] = 0x1f;
          Mask[3] = 0x8000;
          break;

        case RGB_555:
          Mask[0] = 0x7c00;
          Mask[1] = 0x3e0;
          Mask[2] = 0x1f;
          Mask[3] = 0x0;
          break;
        }

      Mask[0] = GUINT32_TO_LE (Mask[0]);
      Mask[1] = GUINT32_TO_LE (Mask[1]);
      Mask[2] = GUINT32_TO_LE (Mask[2]);
      Mask[3] = GUINT32_TO_LE (Mask[3]);

      Write (outfile, &Mask, mask_info_size);
    }

  if (! BMPSaveData.dont_write_color_space_data)
    {
      guint32 buf[0x11];

      /* Write V5 color space fields */

      /* bV5CSType = LCS_sRGB */
      buf[0x00] = GUINT32_TO_LE (0x73524742);

      /* bV5Endpoints is set to 0 (ignored) */
      for (i = 0; i < 0x09; i++)
        buf[i + 1] = 0x00;

      /* bV5GammaRed is set to 0 (ignored) */
      buf[0x0a] = GUINT32_TO_LE (0x0);

      /* bV5GammaGreen is set to 0 (ignored) */
      buf[0x0b] = GUINT32_TO_LE (0x0);

      /* bV5GammaBlue is set to 0 (ignored) */
      buf[0x0c] = GUINT32_TO_LE (0x0);

      /* bV5Intent = LCS_GM_GRAPHICS */
      buf[0x0d] = GUINT32_TO_LE (0x00000002);

      /* bV5ProfileData is set to 0 (ignored) */
      buf[0x0e] = GUINT32_TO_LE (0x0);

      /* bV5ProfileSize is set to 0 (ignored) */
      buf[0x0f] = GUINT32_TO_LE (0x0);

      /* bV5Reserved = 0 */
      buf[0x10] = GUINT32_TO_LE (0x0);

      Write (outfile, buf, color_space_size);
    }

  write_color_map (outfile, Red, Green, Blue, MapSize);

  /* After that is done, we write the image ... */

  write_image (outfile,
               pixels, cols, rows,
               BMPSaveData.use_run_length_encoding,
               channels, BitsPerPixel, SpZeile,
               MapSize, BMPSaveData.rgb_format,
               mask_info_size, color_space_size);

  /* ... and exit normally */

  fclose (outfile);
  g_free (pixels);

  return GIMP_PDB_SUCCESS;
}

static inline void
Make565 (guchar  r,
         guchar  g,
         guchar  b,
         guchar *buf)
{
  gint p;

  p = ((((gint) (r / 255.0 * 31.0 + 0.5)) << 11) |
       (((gint) (g / 255.0 * 63.0 + 0.5)) <<  5) |
       (((gint) (b / 255.0 * 31.0 + 0.5))));

  buf[0] = (guchar) (p & 0xff);
  buf[1] = (guchar) (p >> 8);
}

static inline void
Make5551 (guchar  r,
          guchar  g,
          guchar  b,
          guchar  a,
          guchar *buf)
{
  gint p;

  p = ((((gint) (r / 255.0 * 31.0 + 0.5)) << 10) |
       (((gint) (g / 255.0 * 31.0 + 0.5)) <<  5) |
       (((gint) (b / 255.0 * 31.0 + 0.5)))       |
       (((gint) (a / 255.0 +  0.5)        << 15)));

  buf[0] = (guchar) (p & 0xff);
  buf[1] = (guchar) (p >> 8);
}

static void
write_image (FILE   *f,
             guchar *src,
             gint    width,
             gint    height,
             gint    use_run_length_encoding,
             gint    channels,
             gint    bpp,
             gint    spzeile,
             gint    MapSize,
             RGBMode rgb_format,
             gint    mask_info_size,
             gint    color_space_size)
{
  guchar  buf[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0 };
  guint32 uint32buf;
  guchar *temp, v;
  guchar *row, *ketten;
  gint    xpos, ypos, i, j, rowstride, length, thiswidth;
  gint    breite, k;
  guchar  n, r, g, b, a;
  gint    cur_progress;
  gint    max_progress;

  xpos = 0;
  rowstride = width * channels;

  cur_progress = 0;
  max_progress = height;

  /* We'll begin with the 16/24/32 bit Bitmaps, they are easy :-) */

  if (bpp > 8)
    {
      for (ypos = height - 1; ypos >= 0; ypos--)  /* for each row   */
        {
          for (i = 0; i < width; i++)  /* for each pixel */
            {
              temp = src + (ypos * rowstride) + (xpos * channels);
              switch (rgb_format)
                {
                default:
                case RGB_888:
                  buf[2] = *temp++;
                  buf[1] = *temp++;
                  buf[0] = *temp++;
                  xpos++;
                  if (channels > 3 && (guchar) *temp == 0)
                    buf[0] = buf[1] = buf[2] = 0xff;
                  Write (f, buf, 3);
                  break;
                case RGBX_8888:
                  buf[2] = *temp++;
                  buf[1] = *temp++;
                  buf[0] = *temp++;
                  buf[3] = 0;
                  xpos++;
                  if (channels > 3 && (guchar) *temp == 0)
                    buf[0] = buf[1] = buf[2] = 0xff;
                  Write (f, buf, 4);
                  break;
                case RGBA_8888:
                  buf[2] = *temp++;
                  buf[1] = *temp++;
                  buf[0] = *temp++;
                  buf[3] = *temp;
                  xpos++;
                  Write (f, buf, 4);
                  break;
                case RGB_565:
                  r = *temp++;
                  g = *temp++;
                  b = *temp++;
                  if (channels > 3 && (guchar) *temp == 0)
                    r = g = b = 0xff;
                  Make565 (r, g, b, buf);
                  xpos++;
                  Write (f, buf, 2);
                  break;
                case RGB_555:
                  r = *temp++;
                  g = *temp++;
                  b = *temp++;
                  if (channels > 3 && (guchar) *temp == 0)
                    r = g = b = 0xff;
                  Make5551 (r, g, b, 0x0, buf);
                  xpos++;
                  Write (f, buf, 2);
                  break;
                case RGBA_5551:
                  r = *temp++;
                  g = *temp++;
                  b = *temp++;
                  a = *temp;
                  Make5551 (r, g, b, a, buf);
                  xpos++;
                  Write (f, buf, 2);
                  break;
                }
            }

          Write (f, &buf[4], spzeile - (width * (bpp/8)));

          cur_progress++;
          if ((cur_progress % 5) == 0)
            gimp_progress_update ((gdouble) cur_progress /
                                  (gdouble) max_progress);

          xpos = 0;
        }
    }
  else
    {
      switch (use_run_length_encoding)  /* now it gets more difficult */
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
                        if (channels > 1 && *(temp+1) == 0) *temp = 0x0;
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
            length = 0;
            buf[12] = 0;
            buf[13] = 1;
            buf[14] = 0;
            buf[15] = 0;
            row = g_new (guchar, width / (8 / bpp) + 10);
            ketten = g_new (guchar, width / (8 / bpp) + 10);
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
                        if (channels > 1 && *(temp+1) == 0) *temp = 0x0;
                        v = v | ((guchar) * temp << (8 - (i * bpp)));
                      }
                    row[j++] = v;
                  }
                breite = width / (8 / bpp);
                if (width % (8 / bpp))
                  breite++;

                /* then check for strings of equal bytes */
                for (i = 0; i < breite; i += j)
                  {
                    j = 0;
                    while ((i + j < breite) &&
                           (j < (255 / (8 / bpp))) &&
                           (row[i + j] == row[i]))
                      j++;

                    ketten[i] = j;
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
                            length += 2;
                            Write (f, &row[i], j);
                            length += j;
                            if ((j) % 2)
                              {
                                Write (f, &buf[12], 1);
                                length++;
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
                                Write (f, &row[k], 1);
                                /*printf("%i.#|",n); */
                                length += 2;
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
                        Write (f, &row[i], 1);
                        i += ketten[i];
                        length += 2;
                      }
                  }

                Write (f, &buf[14], 2);          /* End of row */
                length += 2;

                cur_progress++;
                if ((cur_progress % 5) == 0)
                  gimp_progress_update ((gdouble) cur_progress /
                                        (gdouble) max_progress);
              }

            fseek (f, -2, SEEK_CUR);     /* Overwrite last End of row ... */
            Write (f, &buf[12], 2);      /* ... with End of file */

            fseek (f, 0x22, SEEK_SET);            /* Write length of image */
            uint32buf = GUINT32_TO_LE (length);
            Write (f, &uint32buf, 4);

            fseek (f, 0x02, SEEK_SET);            /* Write length of file */
            length += (0x36 + MapSize + mask_info_size + color_space_size);
            uint32buf = GUINT32_TO_LE (length);
            Write (f, &uint32buf, 4);

            g_free (ketten);
            g_free (row);
            break;
          }
        }
    }

  gimp_progress_update (1.0);
}

static void
format_callback (GtkToggleButton *toggle,
                 gpointer         data)
{
  if (gtk_toggle_button_get_active (toggle))
    BMPSaveData.rgb_format = GPOINTER_TO_INT (data);
}

static gboolean
save_dialog (gint channels)
{
  GtkWidget *dialog;
  GtkWidget *toggle;
  GtkWidget *vbox_main;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *expander;
  GtkWidget *frame;
  GSList    *group;
  gboolean   run;

  /* Dialog init */
  dialog = gimp_export_dialog_new ("BMP", PLUG_IN_BINARY, SAVE_PROC);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  vbox_main = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox_main), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      vbox_main, TRUE, TRUE, 0);
  gtk_widget_show (vbox_main);

  /* Run-Length Encoded */
  toggle = gtk_check_button_new_with_mnemonic (_("_Run-Length Encoded"));
  gtk_box_pack_start (GTK_BOX (vbox_main), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                BMPSaveData.use_run_length_encoding);
  gtk_widget_show (toggle);
  if (channels > 1)
    gtk_widget_set_sensitive (toggle, FALSE);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &BMPSaveData.use_run_length_encoding);

  /* Compatibility Options */
  expander = gtk_expander_new_with_mnemonic (_("Co_mpatibility Options"));

  gtk_box_pack_start (GTK_BOX (vbox_main), expander, TRUE, TRUE, 0);
  gtk_widget_show (expander);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 12);
  gtk_container_add (GTK_CONTAINER (expander), vbox2);
  gtk_widget_show (vbox2);

  toggle = gtk_check_button_new_with_mnemonic (_("_Do not write color space information"));
  gimp_help_set_help_data (toggle,
                           _("Some applications can not read BMP images that "
                             "include color space information. GIMP writes "
                             "color space information by default. Enabling "
                             "this option will cause GIMP to not write color "
                             "space information to the file."),
                           NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                BMPSaveData.dont_write_color_space_data);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &BMPSaveData.dont_write_color_space_data);

  /* Advanced Options */
  expander = gtk_expander_new_with_mnemonic (_("_Advanced Options"));

  gtk_box_pack_start (GTK_BOX (vbox_main), expander, TRUE, TRUE, 0);
  gtk_widget_show (expander);

  if (channels < 3)
    gtk_widget_set_sensitive (expander, FALSE);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 12);
  gtk_container_add (GTK_CONTAINER (expander), vbox2);
  gtk_widget_show (vbox2);

  group = NULL;

  frame = gimp_frame_new (_("16 bits"));
  gtk_box_pack_start (GTK_BOX (vbox2), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  toggle = gtk_radio_button_new_with_mnemonic (group, "_R5 G6 B5");
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (format_callback),
                    GINT_TO_POINTER (RGB_565));

  toggle = gtk_radio_button_new_with_mnemonic (group, "_A1 R5 G5 B5");
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  if (channels < 4)
    gtk_widget_set_sensitive (toggle, FALSE);

  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (format_callback),
                    GINT_TO_POINTER (RGBA_5551));
  toggle = gtk_radio_button_new_with_mnemonic (group, "_X1 R5 G5 B5");
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (format_callback),
                    GINT_TO_POINTER (RGB_555));

  frame = gimp_frame_new (_("24 bits"));
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle = gtk_radio_button_new_with_mnemonic (group, "R_8 G8 B8");
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON(toggle));
  gtk_container_add (GTK_CONTAINER (frame), toggle);
  gtk_widget_show (toggle);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (format_callback),
                    GINT_TO_POINTER (RGB_888));
  if (channels < 4)
    {
      BMPSaveData.rgb_format = RGB_888;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), TRUE);
    }

  frame = gimp_frame_new (_("32 bits"));
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  toggle = gtk_radio_button_new_with_mnemonic (group, "A8 R8 G8 _B8");
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (format_callback),
                    GINT_TO_POINTER (RGBA_8888));


  if (channels < 4)
    {
      gtk_widget_set_sensitive (toggle, FALSE);
    }
  else
    {
      BMPSaveData.rgb_format = RGBA_8888;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), TRUE);
    }

  toggle = gtk_radio_button_new_with_mnemonic (group, "X8 R8 G8 _B8");
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (format_callback),
                    GINT_TO_POINTER (RGBX_8888));

  /* Dialog show */
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
