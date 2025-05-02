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
#include "bmp-export.h"

#include "libgimp/stdplugins-intl.h"


static  gboolean  write_image     (FILE          *f,
                                   guchar        *src,
                                   gint           width,
                                   gint           height,
                                   gboolean       use_run_length_encoding,
                                   gint           channels,
                                   gint           bpp,
                                   gint           spzeile,
                                   gint           MapSize,
                                   RGBMode        rgb_format,
                                   gint           mask_info_size,
                                   gint           color_space_size);

static  gboolean  save_dialog     (GimpProcedure *procedure,
                                   GObject       *config,
                                   GimpImage     *image,
                                   gint           channels,
                                   gint           bpp);


static gboolean
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
      if (fwrite (trgb, 1, 4, f) != 4)
        return FALSE;
    }
  return TRUE;
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
  gtk_widget_show (dialog);

  ok = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return ok;
}

GimpPDBStatusType
export_image (GFile         *file,
              GimpImage     *image,
              GimpDrawable  *drawable,
              GimpRunMode    run_mode,
              GimpProcedure *procedure,
              GObject       *config,
              GError       **error)
{
  FILE           *outfile = NULL;
  BitmapFileHead  bitmap_file_head;
  BitmapHead      bitmap_head;
  gint            Red[MAXCOLORS];
  gint            Green[MAXCOLORS];
  gint            Blue[MAXCOLORS];
  guchar         *cmap;
  gint            rows, cols, channels, MapSize;
  gint            bytes_per_row;
  glong           BitsPerPixel;
  gint            colors;
  guchar         *pixels = NULL;
  GeglBuffer     *buffer;
  const Babl     *format;
  GimpImageType   drawable_type;
  gint            drawable_width;
  gint            drawable_height;
  gint            i;
  gint            mask_info_size;
  gint            color_space_size;
  guint32         Mask[4];
  gboolean        use_rle;
  gboolean        write_color_space;
  RGBMode         rgb_format;

  buffer = gimp_drawable_get_buffer (drawable);

  drawable_type   = gimp_drawable_type   (drawable);
  drawable_width  = gimp_drawable_get_width  (drawable);
  drawable_height = gimp_drawable_get_height (drawable);

  switch (drawable_type)
    {
    case GIMP_RGBA_IMAGE:
      format       = babl_format ("R'G'B'A u8");
      colors       = 0;
      BitsPerPixel = 32;
      MapSize      = 0;
      channels     = 4;

      if (run_mode == GIMP_RUN_INTERACTIVE)
        g_object_set (config,
                      "rgb-format", "rgba-8888",
                      "use-rle",    FALSE,
                      NULL);
      else
        g_object_set (config,
                      "use-rle",    FALSE,
                      NULL);
      break;

    case GIMP_RGB_IMAGE:
      format       = babl_format ("R'G'B' u8");
      colors       = 0;
      BitsPerPixel = 24;
      MapSize      = 0;
      channels     = 3;

      if (run_mode == GIMP_RUN_INTERACTIVE)
        g_object_set (config,
                      "rgb-format", "rgb-888",
                      "use-rle",    FALSE,
                      NULL);
      else
        g_object_set (config,
                      "use-rle", FALSE,
                      NULL);
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
      format   = gimp_drawable_get_format (drawable);
      cmap     = gimp_palette_get_colormap (gimp_image_get_palette (image), babl_format ("R'G'B' u8"), &colors, NULL);
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
        {
          BitsPerPixel = 1;
          g_object_set (config,
                        "use-rle", FALSE,
                        NULL);
        }

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

  mask_info_size = 0;

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      (BitsPerPixel == 8 ||
       BitsPerPixel == 4 ||
       BitsPerPixel == 1))
    {
      if (! save_dialog (procedure, config, image, 1, BitsPerPixel))
        return GIMP_PDB_CANCEL;
    }
  else if (BitsPerPixel == 24 ||
           BitsPerPixel == 32)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          if (! save_dialog (procedure, config, image, channels, BitsPerPixel))
            return GIMP_PDB_CANCEL;
        }

      rgb_format =
        gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                             "rgb-format");

      /* mask_info_size is only set to non-zero for 16- and 32-bpp */
      switch (rgb_format)
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

  g_object_get (config,
                "use-rle",           &use_rle,
                "write-color-space", &write_color_space,
                NULL);
  rgb_format =
    gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                         "rgb-format");

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  outfile = g_fopen (g_file_peek_path (file), "wb");

  if (! outfile)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* fetch the image */
  pixels = g_new (guchar, (gsize) drawable_width * drawable_height * channels);

  gegl_buffer_get (buffer,
                   GEGL_RECTANGLE (0, 0, drawable_width, drawable_height), 1.0,
                   format, pixels,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (buffer);

  /* Now, we need some further information ... */
  cols = drawable_width;
  rows = drawable_height;

  /* ... that we write to our headers. */
  /* We should consider rejecting any width > (INT32_MAX - 31) / BitsPerPixel, as
   * the resulting BMP will likely cause integer overflow in other readers.
   * TODO: Revisit as soon as we can add strings again !!! */

  bytes_per_row = (( (guint64) cols * BitsPerPixel + 31) / 32) * 4;

  if (write_color_space)
    {
      /* Always include color mask for BITMAPV5HEADER, see #4155. */
      mask_info_size   = 16;
      color_space_size = 68;
    }
  else
    {
      color_space_size = 0;
    }

  bitmap_file_head.bfSize    = (54 + MapSize + (rows * bytes_per_row) +
                                mask_info_size + color_space_size);
  bitmap_file_head.zzHotX    =  0;
  bitmap_file_head.zzHotY    =  0;
  bitmap_file_head.bfOffs    = (54 + MapSize +
                                mask_info_size + color_space_size);

  bitmap_head.biSize   = 40 + mask_info_size + color_space_size;
  bitmap_head.biWidth  = cols;
  bitmap_head.biHeight = rows;
  bitmap_head.biPlanes = 1;
  bitmap_head.biBitCnt = BitsPerPixel;

  if (! use_rle)
    {
      /* The Microsoft specification for BITMAPV5HEADER says that
       * BI_BITFIELDS is valid for 16 and 32-bits per pixel,
       * Since it doesn't mention 24 bpp or other numbers
       * use BI_RGB for that. See issue #6114. */
      if (mask_info_size > 0 && (BitsPerPixel == 16 || BitsPerPixel == 32))
        bitmap_head.biCompr = BI_BITFIELDS;
      else
        bitmap_head.biCompr = BI_RGB;
    }
  else if (BitsPerPixel == 8)
    {
      bitmap_head.biCompr = BI_RLE8;
    }
  else if (BitsPerPixel == 4)
    {
      bitmap_head.biCompr = BI_RLE4;
    }
  else
    {
      bitmap_head.biCompr = BI_RGB;
    }

  bitmap_head.biSizeIm = bytes_per_row * rows;

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
          (gint) bitmap_file_head.bfSize,
          (gint) bitmap_head.biClrUsed,
          bitmap_head.biBitCnt,
          (gint) bitmap_head.biWidth,
          (gint) bitmap_head.biHeight,
          (gint) bitmap_head.biCompr, bytes_per_row);
#endif

  /* And now write the header and the colormap (if any) to disk */

  if (fwrite ("BM", 2, 1, outfile) != 1)
    goto abort;

  bitmap_file_head.bfSize = GUINT32_TO_LE (bitmap_file_head.bfSize);
  bitmap_file_head.zzHotX = GUINT16_TO_LE (bitmap_file_head.zzHotX);
  bitmap_file_head.zzHotY = GUINT16_TO_LE (bitmap_file_head.zzHotY);
  bitmap_file_head.bfOffs = GUINT32_TO_LE (bitmap_file_head.bfOffs);

  if (fwrite (&bitmap_file_head.bfSize, 12, 1, outfile) != 1)
    goto abort;

  bitmap_head.biSize    = GUINT32_TO_LE (bitmap_head.biSize);
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

  if (fwrite (&bitmap_head, 40, 1, outfile) != 1)
    goto abort;

  if (mask_info_size > 0)
    {
      switch (rgb_format)
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

      if (fwrite (&Mask, mask_info_size, 1, outfile) != 1)
        goto abort;
    }

  if (write_color_space)
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

      if (fwrite (buf, color_space_size, 1, outfile) != 1)
        goto abort;
    }

  if (! write_color_map (outfile, Red, Green, Blue, MapSize))
    goto abort;

  /* After that is done, we write the image ... */

  if (! write_image (outfile,
                     pixels, cols, rows,
                     use_rle,
                     channels, BitsPerPixel, bytes_per_row,
                     MapSize, rgb_format,
                     mask_info_size, color_space_size))
    goto abort;

  /* ... and exit normally */

  fclose (outfile);
  g_free (pixels);

  return GIMP_PDB_SUCCESS;

abort:
  if (outfile)
    fclose (outfile);
  g_free(pixels);

  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
               _("Error writing to file."));

  return GIMP_PDB_EXECUTION_ERROR;
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

static gboolean
write_image (FILE     *f,
             guchar   *src,
             gint      width,
             gint      height,
             gboolean  use_run_length_encoding,
             gint      channels,
             gint      bpp,
             gint      bytes_per_row,
             gint      MapSize,
             RGBMode   rgb_format,
             gint      mask_info_size,
             gint      color_space_size)
{
  guchar  buf[16];
  guint32 uint32buf;
  guchar *temp, v;
  guchar *row    = NULL;
  guchar *chains = NULL;
  gint    xpos, ypos, i, j, rowstride, length, thiswidth;
  gint    breite, k;
  guchar  n, r, g, b, a;
  gint    cur_progress;
  gint    max_progress;
  gint    padding;

  xpos = 0;
  rowstride = width * channels;

  cur_progress = 0;
  max_progress = height;

  /* We'll begin with the 16/24/32 bit Bitmaps, they are easy :-) */

  if (bpp > 8)
    {
      padding = bytes_per_row - (width * (bpp / 8));
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

                  if (fwrite (buf, 1, 3, f) != 3)
                    goto abort;
                  break;
                case RGBX_8888:
                  buf[2] = *temp++;
                  buf[1] = *temp++;
                  buf[0] = *temp++;
                  buf[3] = 0;
                  xpos++;
                  if (channels > 3 && (guchar) *temp == 0)
                    buf[0] = buf[1] = buf[2] = 0xff;

                  if (fwrite (buf, 1, 4, f) != 4)
                    goto abort;
                  break;
                case RGBA_8888:
                  buf[2] = *temp++;
                  buf[1] = *temp++;
                  buf[0] = *temp++;
                  buf[3] = *temp;
                  xpos++;

                  if (fwrite (buf, 1, 4, f) != 4)
                    goto abort;
                  break;
                case RGB_565:
                  r = *temp++;
                  g = *temp++;
                  b = *temp++;
                  if (channels > 3 && (guchar) *temp == 0)
                    r = g = b = 0xff;
                  Make565 (r, g, b, buf);
                  xpos++;

                  if (fwrite (buf, 1, 2, f) != 2)
                    goto abort;
                  break;
                case RGB_555:
                  r = *temp++;
                  g = *temp++;
                  b = *temp++;
                  if (channels > 3 && (guchar) *temp == 0)
                    r = g = b = 0xff;
                  Make5551 (r, g, b, 0x0, buf);
                  xpos++;

                  if (fwrite (buf, 1, 2, f) != 2)
                    goto abort;
                  break;
                case RGBA_5551:
                  r = *temp++;
                  g = *temp++;
                  b = *temp++;
                  a = *temp;
                  Make5551 (r, g, b, a, buf);
                  xpos++;

                  if (fwrite (buf, 1, 2, f) != 2)
                    goto abort;
                  break;
                }
            }

          for (int j = 0; j < padding; j++)
            {
              if (EOF == putc (0, f))
                goto abort;
            }

          cur_progress++;
          if ((cur_progress % 5) == 0)
            gimp_progress_update ((gdouble) cur_progress /
                                  (gdouble) max_progress);

          xpos = 0;
        }
    }
  else
    {
      /* now it gets more difficult */
      if (! use_run_length_encoding || bpp == 1)
        {
          /* uncompressed 1,4 and 8 bit */

          thiswidth = (width * bpp + 7) / 8;
          padding = bytes_per_row - thiswidth;

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

                  if (fwrite (&v, 1, 1, f) != 1)
                    goto abort;
                }

              for (int j = 0; j < padding; j++)
                {
                  if (EOF == putc (0, f))
                    goto abort;
                }

              xpos = 0;

              cur_progress++;
              if ((cur_progress % 5) == 0)
                gimp_progress_update ((gdouble) cur_progress /
                                        (gdouble) max_progress);
            }
        }
      else
        {
          /* Save RLE encoded file, quite difficult */

          length = 0;

          row    = g_new (guchar, width / (8 / bpp) + 10);
          chains = g_new (guchar, width / (8 / bpp) + 10);

          for (ypos = height - 1; ypos >= 0; ypos--)
            {
              /* each row separately */
              j = 0;

              /* first copy the pixels to a buffer, making one byte
               * from two 4bit pixels
               */
              for (xpos = 0; xpos < width;)
                {
                  v = 0;

                  for (i = 1;
                       (i <= (8 / bpp)) && (xpos < width);
                       i++, xpos++)
                    {
                      /* for each pixel */

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

                  chains[i] = j;
                }

              /* then write the strings and the other pixels to the file */
              for (i = 0; i < breite;)
                {
                  if (chains[i] < 3)
                    {
                      /* strings of different pixels ... */

                      j = 0;

                      while ((i + j < breite) &&
                             (j < (255 / (8 / bpp))) &&
                             (chains[i + j] < 3))
                        j += chains[i + j];

                      /* this can only happen if j jumps over the end
                       * with a 2 in chains[i+j]
                       */
                      if (j > (255 / (8 / bpp)))
                        j -= 2;

                      /* 00 01 and 00 02 are reserved */
                      if (j > 2)
                        {
                          n = j * (8 / bpp);
                          if (n + i * (8 / bpp) > width)
                            n--;

                          if (EOF == putc (0, f) || EOF == putc (n, f))
                            goto abort;

                          length += 2;

                          if (fwrite (&row[i], 1, j, f) != j)
                            goto abort;

                          length += j;
                          if ((j) % 2)
                            {
                              if (EOF == putc (0, f))
                                goto abort;
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

                              if (EOF == putc (n, f) || EOF == putc (row[k], f))
                                goto abort;
                              length += 2;
                            }
                        }

                      i += j;
                    }
                  else
                    {
                      /* strings of equal pixels */

                      n = chains[i] * (8 / bpp);
                      if (n + i * (8 / bpp) > width)
                        n--;

                      if (EOF == putc (n, f) || EOF == putc (row[i], f))
                        goto abort;

                      i += chains[i];
                      length += 2;
                    }
                }

              if (EOF == putc (0, f) || EOF == putc (0, f))  /* End of row */
                goto abort;
              length += 2;

              cur_progress++;
              if ((cur_progress % 5) == 0)
                gimp_progress_update ((gdouble) cur_progress /
                                      (gdouble) max_progress);
            }

          if (fseek (f, -2, SEEK_CUR))                  /* Overwrite last End of row ... */
            goto abort;
          if (EOF == putc (0, f) || EOF == putc (1, f))   /* ... with End of file */
            goto abort;

          if (fseek (f, 0x22, SEEK_SET))                /* Write length of image */
            goto abort;
          uint32buf = GUINT32_TO_LE (length);
          if (fwrite (&uint32buf, 4, 1, f) != 1)
            goto abort;

          if (fseek (f, 0x02, SEEK_SET))                /* Write length of file */
            goto abort;
          length += (0x36 + MapSize + mask_info_size + color_space_size);
          uint32buf = GUINT32_TO_LE (length);
          if (fwrite (&uint32buf, 4, 1, f) != 1)
            goto abort;

          g_free (chains);
          g_free (row);
        }
    }

  gimp_progress_update (1.0);
  return TRUE;

abort:
  g_free (chains);
  g_free (row);

  return FALSE;
}

static gboolean
format_sensitive_callback (GObject *config,
                           gpointer data)
{
  gint value;
  gint channels = GPOINTER_TO_INT (data);

  value = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                               "rgb-format");

  switch (value)
    {
    case RGBA_5551:
    case RGBA_8888:
      return channels == 4;

    default:
      return TRUE;
    };
}

static void
config_notify (GObject          *config,
               const GParamSpec *pspec,
               gpointer          data)
{
  gint    channels = GPOINTER_TO_INT (data);
  RGBMode format;

  format = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                "rgb-format");

  switch (format)
    {
    case RGBA_5551:
    case RGBA_8888:
      if (channels != 4)
        {
          g_signal_handlers_block_by_func (config, config_notify, data);

          if (format == RGBA_5551)
            g_object_set (config, "rgb-format", "rgb-565", NULL);
          else if (format == RGBA_8888)
            g_object_set (config, "rgb-format", "rgb-888", NULL);

          g_signal_handlers_unblock_by_func (config, config_notify, data);
        }
      break;

    default:
      break;
    };
}

static gboolean
save_dialog (GimpProcedure *procedure,
             GObject       *config,
             GimpImage     *image,
             gint           channels,
             gint           bpp)
{
  GtkWidget *dialog;
  GtkWidget *toggle;
  GtkWidget *vbox;
  GtkWidget *combo;
  gboolean   is_format_sensitive;
  gboolean   run;

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  /* Run-Length Encoded */
  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "use-rle",
                                       ! (channels > 1 || bpp == 1),
                                       NULL, NULL, FALSE);

  /* Compatibility Options */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "color-space-title", _("Compatibility"),
                                   FALSE, FALSE);
  toggle = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                             "write-color-space",
                                             GTK_TYPE_CHECK_BUTTON);
  gimp_help_set_help_data (toggle,
                           _("Some applications can not read BMP images that "
                             "include color space information. GIMP writes "
                             "color space information by default. Disabling "
                             "this option will cause GIMP to not write color "
                             "space information to the file."),
                           NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "color-space-frame", "color-space-title",
                                    FALSE, "write-color-space");

  /* RGB Encoding Options */
  combo = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                            "rgb-format", G_TYPE_NONE);
  g_object_set (combo, "margin", 12, NULL);

  /* Determine if RGB Format combo should be initially sensitive */
  is_format_sensitive = format_sensitive_callback (config,
                                                   GINT_TO_POINTER (channels));
  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "rgb-format",
                                       is_format_sensitive,
                                       NULL, NULL, FALSE);

  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "rgb-format",
                                       (channels >= 3),
                                       NULL, NULL, FALSE);

  /* Formatting the dialog */
  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "bmp-save-vbox",
                                         "use-rle",
                                         "color-space-frame",
                                         "rgb-format",
                                         NULL);
  gtk_box_set_spacing (GTK_BOX (vbox), 12);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "bmp-save-vbox",
                              NULL);

  gtk_widget_show (dialog);

  g_signal_connect (config, "notify::rgb-format",
                    G_CALLBACK (config_notify),
                    GINT_TO_POINTER (channels));

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  g_signal_handlers_disconnect_by_func (config,
                                        config_notify,
                                        GINT_TO_POINTER (channels));

  gtk_widget_destroy (dialog);

  return run;
}
