/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <png.h>

/* #define ICO_DBG */

#include "ico.h"
#include "ico-load.h"
#include "ico-save.h"
#include "ico-dialog.h"

#include "libgimp/stdplugins-intl.h"


static gint     ico_write_int8         (FILE   *fp,
                                        guint8 *data,
                                        gint    count);
static gint     ico_write_int16        (FILE *fp,
                                        guint16 *data,
                                        gint count);
static gint     ico_write_int32        (FILE    *fp,
                                        guint32 *data,
                                        gint     count);

/* Helpers to set bits in a *cleared* data chunk */
static void     ico_set_bit_in_data    (guint8 *data,
                                        gint    line_width,
                                        gint    bit_num,
                                        gint    bit_val);
static void     ico_set_nibble_in_data (guint8 *data,
                                        gint    line_width,
                                        gint    nibble_num,
                                        gint    nibble_val);
static void     ico_set_byte_in_data   (guint8 *data,
                                        gint    line_width,
                                        gint    byte_num,
                                        gint    byte_val);

static gint     ico_get_layer_num_colors  (gint32    layer,
                                           gboolean *uses_alpha_levels);
static void     ico_image_get_reduced_buf (guint32   layer,
                                           gint      bpp,
                                           gint     *num_colors,
                                           guchar  **cmap_out,
                                           guchar  **buf_out);


static gint
ico_write_int32 (FILE     *fp,
                 guint32  *data,
                 gint      count)
{
  gint total;

  total = count;
  if (count > 0)
    {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
      gint i;

      for (i = 0; i < count; i++)
        data[i] = GUINT32_FROM_LE (data[i]);
#endif

      ico_write_int8 (fp, (guint8 *) data, count * 4);

#if (G_BYTE_ORDER == G_BIG_ENDIAN)
      /* Put it back like we found it */
      for (i = 0; i < count; i++)
        data[i] = GUINT32_FROM_LE (data[i]);
#endif
    }

  return total * 4;
}


static gint
ico_write_int16 (FILE     *fp,
                 guint16  *data,
                 gint      count)
{
  gint total;

  total = count;
  if (count > 0)
    {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
      gint i;

      for (i = 0; i < count; i++)
        data[i] = GUINT16_FROM_LE (data[i]);
#endif

      ico_write_int8 (fp, (guint8 *) data, count * 2);

#if (G_BYTE_ORDER == G_BIG_ENDIAN)
      /* Put it back like we found it */
      for (i = 0; i < count; i++)
        data[i] = GUINT16_FROM_LE (data[i]);
#endif
    }

  return total * 2;
}


static gint
ico_write_int8 (FILE     *fp,
                guint8   *data,
                gint      count)
{
  gint total;
  gint bytes;

  total = count;
  while (count > 0)
    {
      bytes = fwrite ((gchar *) data, sizeof (gchar), count, fp);
      if (bytes <= 0) /* something bad happened */
        break;
      count -= bytes;
      data += bytes;
    }

  return total;
}


static void
ico_save_init (gint32       image_ID,
               IcoSaveInfo *info)
{
  gint      *layers;
  gint       i, num_colors;
  gboolean   uses_alpha_values = FALSE;

  layers = gimp_image_get_layers (image_ID, &info->num_icons);
  info->layers = layers;
  info->depths = g_new (gint, info->num_icons);
  info->default_depths = g_new (gint, info->num_icons);
  info->compress = g_new (gboolean, info->num_icons);

  /* Limit the color depths to values that don't cause any color loss --
     the user should pick these anyway, so we can save her some time.
     If the user wants to lose some colors, the settings can always be changed
     in the dialog: */
  for (i = 0; i < info->num_icons; i++)
    {
      num_colors = ico_get_layer_num_colors (layers[i], &uses_alpha_values);

      if (!uses_alpha_values)
        {
          if (num_colors <= 2)
            {
              /* Let's suggest monochrome */
              info->default_depths [i] = 1;
            }
          else if (num_colors <= 16)
            {
              /* Let's suggest 4bpp */
              info->default_depths [i] = 4;
            }
          else if (num_colors <= 256)
            {
              /* Let's suggest 8bpp */
              info->default_depths [i] = 8;
            }
          else
            {
              /* Let's suggest 24bpp */
              info->default_depths [i] = 24;
            }
        }
      else
        {
          /* Otherwise, or if real alpha levels are used, stick with 32bpp */
          info->default_depths [i] = 32;
        }

      /* vista icons */
      if (gimp_drawable_width (layers[i]) > 255
          || gimp_drawable_height (layers[i]) > 255 )
        {
          info->compress[i] = TRUE;
        }
      else
        {
          info->compress[i] = FALSE;
        }
    }

  /* set with default values */
  memcpy (info->depths, info->default_depths,
          sizeof (gint) * info->num_icons);
}



static gboolean
ico_save_dialog (gint32          image_ID,
                 IcoSaveInfo    *info)
{
  GtkWidget *dialog;
  gint       i;
  gint       response;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = ico_dialog_new (info);
  for (i = 0; i < info->num_icons; i++)
    {
      /* if (gimp_layer_get_visible(layers[i])) */
      ico_dialog_add_icon (dialog, info->layers[i], i);
    }

  /* Scale the thing to approximately fit its content, but not too large ... */
  gtk_window_set_default_size (GTK_WINDOW (dialog),
                               -1,
                               200 + (info->num_icons > 4 ?
                                      500 : info->num_icons * 120));

  gtk_widget_show (dialog);

  response = gimp_dialog_run (GIMP_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return (response == GTK_RESPONSE_OK);
}

static void
ico_set_bit_in_data (guint8 *data,
                     gint    line_width,
                     gint    bit_num,
                     gint    bit_val)
{
  gint line;
  gint width32;
  gint offset;

  /* width per line in multiples of 32 bits */
  width32 = (line_width % 32 == 0 ? line_width/32 : line_width/32 + 1);

  line = bit_num / line_width;
  offset = bit_num % line_width;
  bit_val = bit_val & 0x00000001;

  data[line * width32 * 4 + offset/8] |= (bit_val << (7 - (offset % 8)));
}


static void
ico_set_nibble_in_data (guint8 *data,
                        gint    line_width,
                        gint    nibble_num,
                        gint    nibble_val)
{
  gint line;
  gint width8;
  gint offset;

  /* width per line in multiples of 32 bits */
  width8 = (line_width % 8 == 0 ? line_width/8 : line_width/8 + 1);

  line = nibble_num / line_width;
  offset = nibble_num % line_width;
  nibble_val = nibble_val & 0x0000000F;

  data[line * width8 * 4 + offset/2] |=
    (nibble_val << (4 * (1 - (offset % 2))));
}


static void
ico_set_byte_in_data (guint8 *data,
                      gint    line_width,
                      gint    byte_num,
                      gint    byte_val)
{
  gint line;
  gint width4;
  gint offset;
  gint byte;

  /* width per line in multiples of 32 bits */
  width4 = (line_width % 4 == 0 ? line_width/4 : line_width/4 + 1);

  line = byte_num / line_width;
  offset = byte_num % line_width;
  byte = byte_val & 0x000000FF;

  data[line * width4 * 4 + offset] = byte;
}


/* Create a colormap from the given buffer data */
static guint32 *
ico_create_palette (const guchar *cmap,
                    gint          num_colors,
                    gint          num_colors_used,
                    gint         *black_slot)
{
  guchar *palette;
  gint    i;

  g_return_val_if_fail (cmap != NULL || num_colors_used == 0, NULL);
  g_return_val_if_fail (num_colors_used <= num_colors, NULL);

  palette = g_new0 (guchar, num_colors * 4);
  *black_slot = -1;

  for (i = 0; i < num_colors_used; i++)
    {
      palette[i * 4 + 2] = cmap[i * 3];
      palette[i * 4 + 1] = cmap[i * 3 + 1];
      palette[i * 4]     = cmap[i * 3 + 2];

      if ((cmap[i*3]     == 0) &&
          (cmap[i*3 + 1] == 0) &&
          (cmap[i*3 + 2] == 0))
        {
          *black_slot = i;
        }
    }

  if (*black_slot == -1)
    {
      if (num_colors_used == num_colors)
        {
          D(("WARNING -- no room for black, this shouldn't happen.\n"));
          *black_slot = num_colors - 1;

          palette[(num_colors-1) * 4]     = 0;
          palette[(num_colors-1) * 4 + 1] = 0;
          palette[(num_colors-1) * 4 + 2] = 0;
        }
      else
        {
          *black_slot = num_colors_used;
        }
    }

  return (guint32 *) palette;
}


static GHashTable *
ico_create_color_to_palette_map (const guint32 *palette,
                                 gint           num_colors)
{
  GHashTable *hash;
  gint        i;

  hash = g_hash_table_new_full (g_int_hash, g_int_equal,
                                (GDestroyNotify) g_free,
                                (GDestroyNotify) g_free);

  for (i = 0; i < num_colors; i++)
    {
      const guint8 *pixel = (const guint8 *) &palette[i];
      gint         *color;
      gint         *slot;

      color = g_new (gint, 1);
      slot = g_new (gint, 1);

      *color = (pixel[2] << 16 | pixel[1] << 8 | pixel[0]);
      *slot = i;

      g_hash_table_insert (hash, color, slot);
    }

  return hash;
}

static gint
ico_get_palette_index (GHashTable *hash,
                       gint        red,
                       gint        green,
                       gint        blue)
{
  gint  color = 0;
  gint *slot;

  color = (red << 16 | green << 8 | blue);
  slot = g_hash_table_lookup (hash, &color);

  if (!slot)
    {
      return 0;
    }

  return *slot;
}

static gint
ico_get_layer_num_colors (gint32    layer,
                          gboolean *uses_alpha_levels)
{
  gint        w, h;
  gint        bpp;
  gint        num_colors = 0;
  guint       num_pixels;
  guchar     *buf;
  guchar     *src;
  guint32    *colors;
  guint32    *c;
  GHashTable *hash;
  GeglBuffer *buffer = gimp_drawable_get_buffer (layer);
  const Babl *format;

  w = gegl_buffer_get_width  (buffer);
  h = gegl_buffer_get_height (buffer);

  num_pixels = w * h;

  switch (gimp_drawable_type (layer))
    {
    case GIMP_RGB_IMAGE:
      format = babl_format ("R'G'B' u8");
      break;

    case GIMP_RGBA_IMAGE:
      format = babl_format ("R'G'B'A u8");
      break;

    case GIMP_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      break;

    case GIMP_GRAYA_IMAGE:
      format = babl_format ("Y'A u8");
      break;

    case GIMP_INDEXED_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      format = gegl_buffer_get_format (buffer);
      /* It is possible to count the colors of indexed image more easily
       * with gimp_image_get_colormap(), but counting only the colors
       * actually used will allow more efficient bpp if possible. */
      break;

    default:
      g_return_val_if_reached (0);
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  buf = src = g_new (guchar, num_pixels * bpp);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0,
                   format, buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (buffer);

  hash = g_hash_table_new (g_int_hash, g_int_equal);
  *uses_alpha_levels = FALSE;

  colors = c = g_new (guint32, num_pixels);

  switch (bpp)
    {
    case 1:
      while (num_pixels--)
        {
          *c = *src;
          g_hash_table_insert (hash, c, c);
          src++;
          c++;
        }
      break;

    case 2:
      while (num_pixels--)
        {
          *c = (src[1] << 8) | src[0];
          if (src[1] != 0 && src[1] != 255)
            *uses_alpha_levels = TRUE;
          g_hash_table_insert (hash, c, c);
          src += 2;
          c++;
        }
      break;

    case 3:
      while (num_pixels--)
        {
          *c = (src[2] << 16) | (src[1] << 8) | src[0];
          g_hash_table_insert (hash, c, c);
          src += 3;
          c++;
        }
      break;

    case 4:
      while (num_pixels--)
        {
          *c = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
          if (src[3] != 0 && src[3] != 255)
            *uses_alpha_levels = TRUE;
          g_hash_table_insert (hash, c, c);
          src += 4;
          c++;
        }
      break;
    }

  num_colors = g_hash_table_size (hash);

  g_hash_table_destroy (hash);

  g_free (colors);
  g_free (buf);

  return num_colors;
}

gboolean
ico_cmap_contains_black (const guchar *cmap,
                         gint          num_colors)
{
  gint i;

  for (i = 0; i < num_colors; i++)
    {
      if ((cmap[3 * i    ] == 0) &&
          (cmap[3 * i + 1] == 0) &&
          (cmap[3 * i + 2] == 0))
        {
          return TRUE;
        }
    }

  return FALSE;
}

static void
ico_image_get_reduced_buf (guint32   layer,
                           gint      bpp,
                           gint     *num_colors,
                           guchar  **cmap_out,
                           guchar  **buf_out)
{
  gint32      tmp_image;
  gint32      tmp_layer;
  gint        w, h;
  guchar     *buf;
  guchar     *cmap   = NULL;
  GeglBuffer *buffer = gimp_drawable_get_buffer (layer);
  const Babl *format;

  w = gegl_buffer_get_width  (buffer);
  h = gegl_buffer_get_height (buffer);

  switch (gimp_drawable_type (layer))
    {
    case GIMP_RGB_IMAGE:
      format = babl_format ("R'G'B' u8");
      break;

    case GIMP_RGBA_IMAGE:
      format = babl_format ("R'G'B'A u8");
      break;

    case GIMP_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      break;

    case GIMP_GRAYA_IMAGE:
      format = babl_format ("Y'A u8");
      break;

    case GIMP_INDEXED_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      format = gegl_buffer_get_format (buffer);
      break;

    default:
      g_return_if_reached ();
    }

  *num_colors = 0;

  buf = g_new (guchar, w * h * 4);

  if (bpp <= 8 || bpp == 24 || babl_format_get_bytes_per_pixel (format) != 4)
    {
      gint32      image = gimp_item_get_image (layer);
      GeglBuffer *tmp;

      tmp_image = gimp_image_new (w, h, gimp_image_base_type (image));
      gimp_image_undo_disable (tmp_image);

      if (gimp_drawable_is_indexed (layer))
        {
          guchar *cmap;
          gint    num_colors;

          cmap = gimp_image_get_colormap (image, &num_colors);
          gimp_image_set_colormap (tmp_image, cmap, num_colors);
          g_free (cmap);
        }

      tmp_layer = gimp_layer_new (tmp_image, "tmp", w, h,
                                  gimp_drawable_type (layer),
                                  100,
                                  gimp_image_get_default_new_layer_mode (tmp_image));
      gimp_image_insert_layer (tmp_image, tmp_layer, -1, 0);

      tmp = gimp_drawable_get_buffer (tmp_layer);

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0,
                       format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE, tmp, NULL);

      g_object_unref (tmp);

      if (! gimp_drawable_is_rgb (tmp_layer))
        gimp_image_convert_rgb (tmp_image);

      if (bpp <= 8)
        {
          gimp_image_convert_indexed (tmp_image,
                                      GIMP_CONVERT_DITHER_FS,
                                      GIMP_CONVERT_PALETTE_GENERATE,
                                      1 << bpp, TRUE, FALSE, "dummy");

          cmap = gimp_image_get_colormap (tmp_image, num_colors);

          if (*num_colors == (1 << bpp) &&
              ! ico_cmap_contains_black (cmap, *num_colors))
            {
              /* Windows icons with color maps need the color black.
               * We need to eliminate one more color to make room for black.
               */

              if (gimp_drawable_is_indexed (layer))
                {
                  g_free (cmap);
                  cmap = gimp_image_get_colormap (image, num_colors);
                  gimp_image_set_colormap (tmp_image, cmap, *num_colors);
                }
              else if (gimp_drawable_is_gray (layer))
                {
                  gimp_image_convert_grayscale (tmp_image);
                }
              else
                {
                  gimp_image_convert_rgb (tmp_image);
                }

              tmp = gimp_drawable_get_buffer (tmp_layer);

              gegl_buffer_set (tmp, GEGL_RECTANGLE (0, 0, w, h), 0,
                               format, buf, GEGL_AUTO_ROWSTRIDE);

              g_object_unref (tmp);

              if (! gimp_drawable_is_rgb (layer))
                gimp_image_convert_rgb (tmp_image);

              gimp_image_convert_indexed (tmp_image,
                                          GIMP_CONVERT_DITHER_FS,
                                          GIMP_CONVERT_PALETTE_GENERATE,
                                          (1<<bpp) - 1, TRUE, FALSE, "dummy");
              g_free (cmap);
              cmap = gimp_image_get_colormap (tmp_image, num_colors);
            }

          gimp_image_convert_rgb (tmp_image);
        }
      else if (bpp == 24)
        {
          GimpParam    *return_vals;
          gint          n_return_vals;

          return_vals =
            gimp_run_procedure ("plug-in-threshold-alpha", &n_return_vals,
                                GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,
                                GIMP_PDB_IMAGE, tmp_image,
                                GIMP_PDB_DRAWABLE, tmp_layer,
                                GIMP_PDB_INT32, ICO_ALPHA_THRESHOLD,
                                GIMP_PDB_END);
          gimp_destroy_params (return_vals, n_return_vals);
        }

      gimp_layer_add_alpha (tmp_layer);

      tmp = gimp_drawable_get_buffer (tmp_layer);

      gegl_buffer_get (tmp, GEGL_RECTANGLE (0, 0, w, h), 1.0,
                       NULL, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      g_object_unref (tmp);

      gimp_image_delete (tmp_image);
    }
  else
    {
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0,
                       format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }

  g_object_unref (buffer);

  *cmap_out = cmap;
  *buf_out = buf;
}

static gboolean
ico_write_png (FILE   *fp,
               gint32  layer,
               gint32  depth)
{
  png_structp png_ptr;
  png_infop   info_ptr;
  png_byte  **row_pointers;
  gint        i, rowstride;
  gint        width, height;
  gint        num_colors_used;
  guchar     *palette;
  guchar     *buf;

  row_pointers = NULL;
  palette = NULL;
  buf = NULL;

  width = gimp_drawable_width (layer);
  height = gimp_drawable_height (layer);

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if ( !png_ptr )
    return FALSE;

  info_ptr = png_create_info_struct (png_ptr);
  if ( !info_ptr )
    {
      png_destroy_write_struct (&png_ptr, NULL);
      return FALSE;
    }

  if (setjmp (png_jmpbuf (png_ptr)))
    {
      png_destroy_write_struct (&png_ptr, &info_ptr);
      if ( row_pointers )
        g_free (row_pointers);
      if (palette)
        g_free (palette);
      if (buf)
        g_free (buf);
      return FALSE;
    }

  ico_image_get_reduced_buf (layer, depth, &num_colors_used,
                             &palette, &buf);

  png_init_io (png_ptr, fp);
  png_set_IHDR (png_ptr, info_ptr, width, height,
                8,
                PNG_COLOR_TYPE_RGBA,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);
  png_write_info (png_ptr, info_ptr);

  rowstride = ico_rowstride (width, 32);
  row_pointers = g_new (png_byte*, height);
  for (i = 0; i < height; i++)
    {
      row_pointers[i] = buf + rowstride * i;
    }
  png_write_image (png_ptr, row_pointers);

  row_pointers = NULL;

  png_write_end (png_ptr, info_ptr);
  png_destroy_write_struct (&png_ptr, &info_ptr);

  g_free (row_pointers);
  g_free (palette);
  g_free (buf);
  return TRUE;
}

static gboolean
ico_write_icon (FILE   *fp,
                gint32  layer,
                gint32  depth)
{
  IcoFileDataHeader  header;
  gint               and_len, xor_len, palette_index, x, y;
  gint               num_colors = 0, num_colors_used = 0, black_index = 0;
  gint               width, height;
  guchar            *buf = NULL, *pixel;
  guint32           *buf32;
  guchar            *palette;
  GHashTable        *color_to_slot = NULL;
  guchar            *xor_map, *and_map;

  guint32           *palette32 = NULL;
  gint               palette_len = 0;

  guint8             alpha_threshold;

  D(("Creating data structures for icon %i ------------------------\n",
     num_icon));

  width = gimp_drawable_width (layer);
  height = gimp_drawable_height (layer);

  header.header_size     = 40;
  header.width          = width;
  header.height         = 2 * height;
  header.planes         = 1;
  header.bpp            = depth;
  header.compression    = 0;
  header.image_size     = 0;
  header.x_res          = 0;
  header.y_res          = 0;
  header.used_clrs      = 0;
  header.important_clrs = 0;

  num_colors = (1L << header.bpp);

  D(("  header size %i, w %i, h %i, planes %i, bpp %i\n",
     header.header_size, header.width, header.height, header.planes,
     header.bpp));

  /* Reduce colors in copy of image */
  ico_image_get_reduced_buf (layer, header.bpp, &num_colors_used,
                             &palette, &buf);
  buf32 = (guint32 *) buf;

  /* Set up colormap and and_map when necessary: */
  if (header.bpp <= 8)
    {
      /* Create a colormap */
      palette32 = ico_create_palette (palette,
                                      num_colors, num_colors_used,
                                      &black_index);
      palette_len = num_colors * 4;

      color_to_slot = ico_create_color_to_palette_map (palette32,
                                                       num_colors_used);
      D(("  created %i-slot colormap with %i colors, black at slot %i\n",
         num_colors, num_colors_used, black_index));
    }

  /* Create and_map. It's padded out to 32 bits per line: */
  and_map = ico_alloc_map (width, height, 1, &and_len);

  /* 32-bit bitmaps have an alpha channel as well as a mask. Any partially or
   * fully opaque pixel should have an opaque mask (some ICO code in Windows
   * draws pixels as black if they have a transparent mask but a non-transparent
   * alpha value).
   *
   * For bitmaps without an alpha channel, we use the normal threshold to build
   * the mask, so that the mask is as close as possible to the original alpha
   * channel.
   */
  alpha_threshold = header.bpp < 32 ? ICO_ALPHA_THRESHOLD : 0;

  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      {
        pixel = (guint8 *) &buf32[y * width + x];

        ico_set_bit_in_data (and_map, width,
                             (height - y -1) * width + x,
                             (pixel[3] > alpha_threshold ? 0 : 1));
      }

  xor_map = ico_alloc_map (width, height, header.bpp, &xor_len);

  /* Now fill in the xor map */
  switch (header.bpp)
    {
    case 1:
      for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
          {
            pixel = (guint8 *) &buf32[y * width + x];
            palette_index = ico_get_palette_index (color_to_slot, pixel[0],
                                                   pixel[1], pixel[2]);

            if (ico_get_bit_from_data (and_map, width,
                                       (height - y - 1) * width + x))
              {
                ico_set_bit_in_data (xor_map, width,
                                     (height - y -1) * width + x,
                                     black_index);
              }
            else
              {
                ico_set_bit_in_data (xor_map, width,
                                     (height - y -1) * width + x,
                                     palette_index);
              }
          }
      break;

    case 4:
      for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
          {
            pixel = (guint8 *) &buf32[y * width + x];
            palette_index = ico_get_palette_index(color_to_slot, pixel[0],
                                                  pixel[1], pixel[2]);

            if (ico_get_bit_from_data (and_map, width,
                                       (height - y - 1) * width + x))
              {
                ico_set_nibble_in_data (xor_map, width,
                                        (height - y -1) * width + x,
                                        black_index);
              }
            else
              {
                ico_set_nibble_in_data (xor_map, width,
                                        (height - y - 1) * width + x,
                                        palette_index);
              }
          }
      break;

    case 8:
      for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
          {
            pixel = (guint8 *) &buf32[y * width + x];
            palette_index = ico_get_palette_index (color_to_slot,
                                                   pixel[0],
                                                   pixel[1],
                                                   pixel[2]);

            if (ico_get_bit_from_data (and_map, width,
                                       (height - y - 1) * width + x))
              {
                ico_set_byte_in_data (xor_map, width,
                                      (height - y - 1) * width + x,
                                      black_index);
              }
            else
              {
                ico_set_byte_in_data (xor_map, width,
                                      (height - y - 1) * width + x,
                                      palette_index);
              }

          }
      break;

    case 24:
      for (y = 0; y < height; y++)
        {
          guchar *row = xor_map + (xor_len * (height - y - 1) / height);

          for (x = 0; x < width; x++)
            {
              pixel = (guint8 *) &buf32[y * width + x];

              row[0] = pixel[2];
              row[1] = pixel[1];
              row[2] = pixel[0];

              row += 3;
            }
        }
      break;

    default:
      for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
          {
            pixel = (guint8 *) &buf32[y * width + x];

            ((guint32 *) xor_map)[(height - y -1) * width + x] =
              GUINT32_TO_LE ((pixel[0] << 16) |
                             (pixel[1] << 8)  |
                             (pixel[2])       |
                             (pixel[3] << 24));
          }
    }

  D(("  filled and_map of length %i, xor_map of length %i\n",
     and_len, xor_len));

  if (color_to_slot)
    g_hash_table_destroy (color_to_slot);

  g_free (palette);
  g_free (buf);

  ico_write_int32 (fp, (guint32*) &header, 3);
  ico_write_int16 (fp, &header.planes, 2);
  ico_write_int32 (fp, &header.compression, 6);

  if (palette_len)
    ico_write_int8 (fp, (guint8 *) palette32, palette_len);

  ico_write_int8 (fp, xor_map, xor_len);
  ico_write_int8 (fp, and_map, and_len);

  g_free (palette32);
  g_free (xor_map);
  g_free (and_map);

  return TRUE;
}

static void
ico_save_info_free (IcoSaveInfo  *info)
{
  g_free (info->depths);
  g_free (info->default_depths);
  g_free (info->compress);
  g_free (info->layers);
  memset (info, 0, sizeof (IcoSaveInfo));
}

GimpPDBStatusType
ico_save_image (const gchar  *filename,
                gint32        image,
                gint32        run_mode,
                GError      **error)
{
  FILE *fp;

  gint i;
  gint width, height;
  IcoSaveInfo      info;
  IcoFileHeader    header;
  IcoFileEntry    *entries;
  gboolean          saved;

  D(("*** Exporting Microsoft icon file %s\n", filename));

  ico_save_init (image, &info);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      /* Allow user to override default values */
      if ( !ico_save_dialog (image, &info))
        return GIMP_PDB_CANCEL;
    }

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_filename_to_utf8 (filename));

  if (! (fp = g_fopen (filename, "wb")))
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  header.reserved = 0;
  header.resource_type = 1;
  header.icon_count = info.num_icons;
  if ( !ico_write_int16 (fp, &header.reserved, 1)
       || !ico_write_int16 (fp, &header.resource_type, 1)
       || !ico_write_int16 (fp, &header.icon_count, 1) )
    {
      ico_save_info_free (&info);
      fclose (fp);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  entries = g_new0 (IcoFileEntry, info.num_icons);
  if (fwrite (entries, sizeof (IcoFileEntry), info.num_icons, fp) <= 0)
    {
      ico_save_info_free (&info);
      g_free (entries);
      fclose (fp);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  for (i = 0; i < info.num_icons; i++)
    {
      gimp_progress_update ((gdouble)i / (gdouble)info.num_icons);

      width = gimp_drawable_width (info.layers[i]);
      height = gimp_drawable_height (info.layers[i]);
      if (width <= 255 && height <= 255)
        {
          entries[i].width = width;
          entries[i].height = height;
        }
      else
        {
          entries[i].width = 0;
          entries[i].height = 0;
        }
      if ( info.depths[i] <= 8 )
        entries[i].num_colors = 1 << info.depths[i];
      else
        entries[i].num_colors = 0;
      entries[i].reserved = 0;
      entries[i].planes = 1;
      entries[i].bpp = info.depths[i];
      entries[i].offset = ftell (fp);

      if (info.compress[i])
        saved = ico_write_png (fp, info.layers[i], info.depths[i]);
      else
        saved = ico_write_icon (fp, info.layers[i], info.depths[i]);

      if (!saved)
        {
          ico_save_info_free (&info);
          fclose (fp);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      entries[i].size = ftell (fp) - entries[i].offset;
    }

  for (i = 0; i < info.num_icons; i++)
    {
      entries[i].planes = GUINT16_TO_LE (entries[i].planes);
      entries[i].bpp    = GUINT16_TO_LE (entries[i].bpp);
      entries[i].size   = GUINT32_TO_LE (entries[i].size);
      entries[i].offset = GUINT32_TO_LE (entries[i].offset);
    }

  if (fseek (fp, sizeof(IcoFileHeader), SEEK_SET) < 0
      || fwrite (entries, sizeof (IcoFileEntry), info.num_icons, fp) <= 0)
    {
      ico_save_info_free (&info);
      fclose (fp);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_progress_update (1.0);

  ico_save_info_free (&info);
  fclose (fp);
  g_free (entries);

  return GIMP_PDB_SUCCESS;
}
