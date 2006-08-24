/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

/* #define ICO_DBG */

#include "main.h"
#include "icoload.h"
#include "icodialog.h"

#include "libgimp/stdplugins-intl.h"


static gint     ico_write_int8         (FILE *fp, guint8 *data, gint count);
static gint     ico_write_int16        (FILE *fp, guint16 *data, gint count);
static gint     ico_write_int32        (FILE *fp, guint32 *data, gint count);

static gint *   ico_show_icon_dialog   (gint32 image_ID, gint *num_icons);

/* Helpers to set bits in a *cleared* data chunk */
static void     ico_set_bit_in_data    (guint8 *data, gint line_width,
                                        gint bit_num, gint bit_val);
static void     ico_set_nibble_in_data (guint8 *data, gint line_width,
                                        gint nibble_num, gint nibble_val);
static void     ico_set_byte_in_data   (guint8 *data, gint line_width,
                                        gint byte_num, gint byte_val);

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


static gint*
ico_show_icon_dialog (gint32  image_ID,
                      gint   *num_icons)
{
  GtkWidget *dialog, *hbox;
  GtkWidget *icon_menu;
  gint      *layers, *icon_depths = NULL;
  gint       num_layers, i, num_colors;
  gboolean   uses_alpha_values;
  gchar      key[MAXLEN];

  *num_icons = 0;

  gimp_ui_init ("winicon", TRUE);

  layers = gimp_image_get_layers (image_ID, &num_layers);
  dialog = ico_specs_dialog_new (num_layers);

  for (i = 0; i < num_layers; i++)
    {
      /* if (gimp_layer_get_visible(layers[i])) */
      ico_specs_dialog_add_icon (dialog, layers[i], i);
    }

  /* Scale the thing to approximately fit its content, but not too large ... */
  gtk_window_set_default_size (GTK_WINDOW (dialog),
                               -1,
                               120 + (num_layers > 4 ? 500 : num_layers * 120));

  icon_depths = g_object_get_data (G_OBJECT (dialog), "icon_depths");

  /* Limit the color depths to values that don't cause any color loss --
     the user should pick these anyway, so we can save her some time.
     If the user wants to lose some colors, the settings can always be changed
     in the dialog: */
  for (i = 0; i < num_layers; i++)
    {
      num_colors = ico_get_layer_num_colors (layers[i], &uses_alpha_values);

      g_snprintf (key, MAXLEN, "layer_%i_hbox", layers[i]);
      hbox = g_object_get_data (G_OBJECT (dialog), key);
      icon_menu = g_object_get_data (G_OBJECT (hbox), "icon_menu");

      if (!uses_alpha_values)
        {
          if (num_colors <= 2)
            {
              /* Let's suggest monochrome */
              icon_depths[i] = 1;
              icon_depths[num_layers + i] = 1;
              ico_specs_dialog_update_icon_preview (dialog, layers[i], 2);
              gtk_combo_box_set_active (GTK_COMBO_BOX (icon_menu), 0);
            }
          else if (num_colors <= 16)
            {
              /* Let's suggest 4bpp */
              icon_depths[i] = 4;
              icon_depths[num_layers + i] = 4;
              ico_specs_dialog_update_icon_preview (dialog, layers[i], 4);
              gtk_combo_box_set_active (GTK_COMBO_BOX (icon_menu), 1);
            }
          else if (num_colors <= 256)
            {
              /* Let's suggest 8bpp */
              icon_depths[i] = 8;
              icon_depths[num_layers + i] = 8;
              ico_specs_dialog_update_icon_preview (dialog, layers[i], 8);
              gtk_combo_box_set_active (GTK_COMBO_BOX (icon_menu), 2);
            }
        }

      /* Otherwise, or if real alpha levels are used, stick with 32bpp */
    }

  g_free (layers);

  gtk_widget_show (dialog);

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK)
    *num_icons = num_layers;
  else
    icon_depths = NULL;

  gtk_widget_destroy (dialog);

  return icon_depths;
}


static GimpPDBStatusType
ico_init (const gchar *filename,
          MsIcon      *ico,
          gint         num_icons)
{
  memset (ico, 0, sizeof (MsIcon));

  if (! (ico->fp = g_fopen (filename, "wb")))
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  ico->filename      = filename;
  ico->reserved      = 0;
  ico->resource_type = 1;
  ico->icon_count    = num_icons;
  ico->icon_dir      = g_new0 (MsIconEntry, num_icons);
  ico->icon_data     = g_new0 (MsIconData, num_icons);

  return GIMP_PDB_SUCCESS;
}


static void
ico_init_direntry (MsIconEntry *entry,
                   gint32       layer,
                   gint         bpp)
{
  /* Was calloc'd, so initialized to 0. */
  entry->width  = gimp_drawable_width (layer);
  entry->height = gimp_drawable_height (layer);

  if (bpp < 8)
    entry->num_colors = (1 << bpp);

  entry->num_planes = 1;
  entry->bpp        = bpp;

  D(("Initialized entry to w %i, h %i, bpp %i\n",
     gimp_drawable_width (layer), entry->width, entry->bpp));

  /* We'll set size and offset when writing things out */
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

  data[line * width8 * 4 + offset/2] |= (nibble_val << (4 * (1 - (offset % 2))));
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
ico_create_palette(guchar *cmap,
                   gint    num_colors,
                   gint    num_colors_used,
                   gint   *black_slot)
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
ico_create_color_to_palette_map (guint32 *palette,
                                 gint     num_colors)
{
  GHashTable *hash;
  gint i;

  hash = g_hash_table_new (g_int_hash, g_int_equal);

  for (i = 0; i < num_colors; i++)
    {
      gint *color, *slot;
      guint8 *pixel = (guint8 *) &palette[i];

      color = g_new (gint, 1);
      slot = g_new (gint, 1);

      *color = (pixel[2] << 16 | pixel[1] << 8 | pixel[0]);
      *slot = i;

      g_hash_table_insert (hash, color, slot);
    }

  return hash;
}


static void
ico_free_hash_item (gpointer data1,
                    gpointer data2,
                    gpointer data3)
{
  g_free (data1);
  g_free (data2);

  /* Shut up warnings: */
  data3 = NULL;
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

static void
ico_init_data (MsIcon *ico,
               gint    num_icon,
               gint32  layer,
               gint    bpp)
{
  MsIconEntry    *entry;
  MsIconData     *data;
  gint            and_len, xor_len, palette_index, x, y;
  gint            num_colors = 0, num_colors_used = 0, black_index = 0;
  guchar         *buffer = NULL, *pixel;
  guint32        *buffer32;
  guchar         *palette;
  GHashTable     *color_to_slot = NULL;

  D(("Creating data structures for icon %i ------------------------\n", num_icon));

  /* Shortcuts, for convenience */
  entry = &ico->icon_dir[num_icon];
  data  = &ico->icon_data[num_icon];

  /* Entries and data were calloc'd, so initialized to 0. */

  data->header_size = 40;
  data->width       = gimp_drawable_width (layer);
  data->height      = 2 * gimp_drawable_height (layer);
  data->planes      = 1;
  data->bpp         = bpp;

  num_colors = (1L << bpp);

  D(("  header size %i, w %i, h %i, planes %i, bpp %i\n",
     data->header_size, data->width, data->height, data->planes, data->bpp));

  /* Reduce colors in copy of image */
  ico_image_get_reduced_buf (layer, bpp, &num_colors_used, &palette, &buffer);
  buffer32 = (guint32 *) buffer;

  /* Set up colormap and andmap when necessary: */
  if (bpp <= 8)
    {
      /* Create a colormap */
      data->palette = ico_create_palette (palette,
                                          num_colors, num_colors_used,
                                          &black_index);
      data->palette_len = num_colors * 4;

      color_to_slot = ico_create_color_to_palette_map (data->palette,
                                                       num_colors_used);
      D(("  created %i-slot colormap with %i colors, black at slot %i\n",
         num_colors, num_colors_used, black_index));
    }

  /* Create and_map. It's padded out to 32 bits per line: */
  data->and_map = ico_alloc_map (entry->width, entry->height, 1, &and_len);
  data->and_len = and_len;

  for (y = 0; y < entry->height; y++)
    for (x = 0; x < entry->width; x++)
      {
        pixel = (guint8 *) &buffer32[y * entry->width + x];

        ico_set_bit_in_data (data->and_map, entry->width,
                             (entry->height-y-1) * entry->width + x,
                             (pixel[3] == 255 ? 0 : 1));
      }

  data->xor_map = ico_alloc_map(entry->width, entry->height, bpp, &xor_len);
  data->xor_len = xor_len;

  /* Now fill in the xor map */
  switch (bpp)
    {
    case 1:
      for (y = 0; y < entry->height; y++)
        for (x = 0; x < entry->width; x++)
          {
            pixel = (guint8 *) &buffer32[y * entry->width + x];
            palette_index = ico_get_palette_index (color_to_slot,
                                                   pixel[0], pixel[1], pixel[2]);

            if (ico_get_bit_from_data (data->and_map, entry->width,
                                       (entry->height-y-1) * entry->width + x))
              {
                ico_set_bit_in_data (data->xor_map, entry->width,
                                     (entry->height-y-1) * entry->width + x,
                                     black_index);
              }
            else
              {
                ico_set_bit_in_data (data->xor_map, entry->width,
                                     (entry->height-y-1) * entry->width + x,
                                     palette_index);
              }
          }
      break;

    case 4:
      for (y = 0; y < entry->height; y++)
        for (x = 0; x < entry->width; x++)
          {
            pixel = (guint8 *) &buffer32[y * entry->width + x];
            palette_index = ico_get_palette_index(color_to_slot,
                                                  pixel[0], pixel[1], pixel[2]);

            if (ico_get_bit_from_data (data->and_map, entry->width,
                                       (entry->height-y-1) * entry->width + x))
              {
                ico_set_nibble_in_data (data->xor_map, entry->width,
                                        (entry->height-y-1) * entry->width + x,
                                        black_index);
              }
            else
              {
                ico_set_nibble_in_data (data->xor_map, entry->width,
                                        (entry->height-y-1) * entry->width + x,
                                        palette_index);
              }
          }
      break;

    case 8:
      for (y = 0; y < entry->height; y++)
        for (x = 0; x < entry->width; x++)
          {
            pixel = (guint8 *) &buffer32[y * entry->width + x];
            palette_index = ico_get_palette_index (color_to_slot,
                                                   pixel[0],
                                                   pixel[1],
                                                   pixel[2]);

            if (ico_get_bit_from_data (data->and_map, entry->width,
                                       (entry->height-y-1) * entry->width + x))
              {
                ico_set_byte_in_data (data->xor_map, entry->width,
                                      (entry->height-y-1) * entry->width + x,
                                      black_index);
              }
            else
              {
                ico_set_byte_in_data (data->xor_map, entry->width,
                                      (entry->height-y-1) * entry->width + x,
                                      palette_index);
              }

          }
      break;

    default:
      for (y = 0; y < entry->height; y++)
        for (x = 0; x < entry->width; x++)
          {
            pixel = (guint8 *) &buffer32[y * entry->width + x];

            ((guint32 *) data->xor_map)[(entry->height-y-1) * entry->width + x] =
              GUINT32_TO_LE ((pixel[0] << 16) |
                             (pixel[1] << 8)  |
                             (pixel[2])       |
                             (pixel[3] << 24));
          }
    }

  D(("  filled and_map of length %i, xor_map of length %i\n",
     data->and_len, data->xor_len));

  if (color_to_slot)
    {
      g_hash_table_foreach (color_to_slot, ico_free_hash_item, NULL);
      g_hash_table_destroy (color_to_slot);
    }

  g_free(palette);
  g_free(buffer);
}


static void
ico_setup (MsIcon *ico,
           gint32  image,
           gint   *icon_depths,
           gint    num_icons)
{
  gint *layers;
  gint  i;
  gint  offset;

  layers = gimp_image_get_layers (image, &num_icons);

  /* Set up icon entries */
  for (i = 0; i < num_icons; i++)
    {
      ico_init_direntry (&ico->icon_dir[i], layers[i], icon_depths[i]);
      gimp_progress_update ((gdouble) i / (gdouble) num_icons * 0.3);
    }

  /* Set up data entries (the actual icons), and calculate each one's size */
  for (i = 0; i < num_icons; i++)
    {
      ico_init_data (ico, i, layers[i], icon_depths[i]);

      ico->icon_dir[i].size =
        ico->icon_data[i].header_size +
        ico->icon_data[i].palette_len +
        ico->icon_data[i].xor_len +
        ico->icon_data[i].and_len;

      gimp_progress_update (0.3 + (gdouble) i / (gdouble) num_icons * 0.3);
    }

  /* Finally, calculate offsets for each icon and store them in each entry */
  offset = 3 * sizeof (guint16) + ico->icon_count * sizeof (MsIconEntry);

  for (i = 0; i < num_icons; i++)
    {
      ico->icon_dir[i].offset = offset;
      offset += ico->icon_dir[i].size;

      gimp_progress_update (0.6 + (gdouble) i / (gdouble) num_icons * 0.3);
    }

  gimp_progress_update (1.0);

  g_free (layers);
}


static GimpPDBStatusType
ico_save (MsIcon *ico)
{
  MsIconEntry *entry;
  MsIconData *data;
  int i;

  ico->cp += ico_write_int16 (ico->fp, &ico->reserved, 3);

  for (i = 0; i < ico->icon_count; i++)
    {
      entry = &ico->icon_dir[i];

      ico->cp += ico_write_int8 (ico->fp, (guint8 *) entry, 4);
      ico->cp += ico_write_int16 (ico->fp, &entry->num_planes, 2);
      ico->cp += ico_write_int32 (ico->fp, &entry->size, 2);
    }

  for (i = 0; i < ico->icon_count; i++)
    {
      data  = &ico->icon_data[i];

      ico->cp += ico_write_int32 (ico->fp, (guint32 *) data, 3);
      ico->cp += ico_write_int16 (ico->fp, &data->planes, 2);
      ico->cp += ico_write_int32 (ico->fp, &data->compression, 6);

      if (data->palette)
        ico->cp += ico_write_int8 (ico->fp,
                                   (guint8 *) data->palette, data->palette_len);

      ico->cp += ico_write_int8 (ico->fp, data->xor_map, data->xor_len);
      ico->cp += ico_write_int8 (ico->fp, data->and_map, data->and_len);
    }

  return GIMP_PDB_SUCCESS;
}

static gboolean
ico_layers_too_big (gint32 image)
{
  gint *layers;
  gint  i, num_layers;

  layers = gimp_image_get_layers (image, &num_layers);

  for (i = 0; i < num_layers; i++)
    {
      if ((gimp_drawable_width (layers[i])  > 255) ||
          (gimp_drawable_height (layers[i]) > 255))
        {
          g_free (layers);
          return TRUE;
        }
    }

  g_free (layers);
  return FALSE;
}

static gint
ico_get_layer_num_colors (gint32    layer,
                          gboolean *uses_alpha_levels)
{
  GimpPixelRgn    pixel_rgn;
  gint            w, h;
  gint            bpp;
  gint            num_colors = 0;
  guint           num_pixels;
  guchar         *buffer;
  guchar         *src;
  guint32        *colors;
  guint32        *c;
  GHashTable     *hash;
  GimpDrawable   *drawable = gimp_drawable_get (layer);

  w = gimp_drawable_width (layer);
  h = gimp_drawable_height (layer);

  num_pixels = w * h;

  bpp = gimp_drawable_bpp (layer);

  buffer = src = g_new (guchar, num_pixels * bpp);

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, w, h, FALSE, FALSE);
  gimp_pixel_rgn_get_rect (&pixel_rgn, buffer, 0, 0, w, h);

  gimp_drawable_detach (drawable);

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
  g_free (buffer);

  return num_colors;
}

static gboolean
ico_cmap_contains_black (guchar *cmap,
                         gint    num_colors)
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
  GimpPixelRgn    src_pixel_rgn, dst_pixel_rgn;
  gint32          tmp_image;
  gint32          tmp_layer;
  gint            w, h;
  guchar         *buffer;
  guchar         *cmap = NULL;
  GimpDrawable   *drawable = gimp_drawable_get (layer);

  w = gimp_drawable_width (layer);
  h = gimp_drawable_height (layer);

  *num_colors = 0;

  buffer = g_new (guchar, w * h * 4);

  if (bpp <= 8 || drawable->bpp != 4)
    {
      gint32        image = gimp_drawable_get_image (layer);
      GimpDrawable *tmp;

      tmp_image = gimp_image_new (gimp_drawable_width (layer),
                                  gimp_drawable_height (layer),
                                  gimp_image_base_type (image));
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
                                  100, GIMP_NORMAL_MODE);
      gimp_image_add_layer (tmp_image, tmp_layer, 0);

      tmp = gimp_drawable_get (tmp_layer);

      gimp_pixel_rgn_init (&src_pixel_rgn, drawable, 0, 0, w, h, FALSE, FALSE);
      gimp_pixel_rgn_init (&dst_pixel_rgn, tmp,      0, 0, w, h, TRUE, FALSE);
      gimp_pixel_rgn_get_rect (&src_pixel_rgn, buffer, 0, 0, w, h);
      gimp_pixel_rgn_set_rect (&dst_pixel_rgn, buffer, 0, 0, w, h);
      gimp_drawable_detach (tmp);

      if (! gimp_drawable_is_rgb (tmp_layer))
        gimp_image_convert_rgb (tmp_image);

      if (bpp <= 8)
        {
          gimp_image_convert_indexed (tmp_image,
                                      GIMP_FS_DITHER, GIMP_MAKE_PALETTE,
                                      1 << bpp, TRUE, FALSE, "dummy");

          cmap = gimp_image_get_colormap (tmp_image, num_colors);

          if (*num_colors == (1 << bpp) &&
              !ico_cmap_contains_black (cmap, *num_colors))
            {
              /* Windows icons with color maps need the color black.
               * We need to eliminate one more color to make room for black.
               */

              gimp_image_convert_rgb (tmp_image);

              tmp = gimp_drawable_get (tmp_layer);
              gimp_pixel_rgn_init (&dst_pixel_rgn,
                                   tmp, 0, 0, w, h, TRUE, FALSE);
              gimp_pixel_rgn_set_rect (&dst_pixel_rgn, buffer, 0, 0, w, h);
              gimp_drawable_detach (tmp);

              gimp_image_convert_indexed (tmp_image,
                                          GIMP_FS_DITHER, GIMP_MAKE_PALETTE,
                                          (1 << bpp) - 1, TRUE, FALSE, "dummy");
              g_free (cmap);
              cmap = gimp_image_get_colormap (tmp_image, num_colors);
            }

          gimp_image_convert_rgb (tmp_image);
        }

      gimp_layer_add_alpha (tmp_layer);

      tmp = gimp_drawable_get (tmp_layer);
      gimp_pixel_rgn_init (&src_pixel_rgn, tmp, 0, 0, w, h, FALSE, FALSE);
      gimp_pixel_rgn_get_rect (&src_pixel_rgn, buffer, 0, 0, w, h);
      gimp_drawable_detach (tmp);

      gimp_image_delete (tmp_image);
    }
  else
    {
      gimp_pixel_rgn_init (&src_pixel_rgn, drawable, 0, 0, w, h, FALSE, FALSE);
      gimp_pixel_rgn_get_rect (&src_pixel_rgn, buffer, 0, 0, w, h);
    }

  gimp_drawable_detach (drawable);

  *cmap_out = cmap;
  *buf_out = buffer;
}

GimpPDBStatusType
SaveICO (const gchar *filename,
         gint32       image)
{
  MsIcon             ico;
  gint              *icon_depths = NULL;
  gint               num_icons;
  GimpPDBStatusType  exit_state;

  D(("*** Saving Microsoft icon file %s\n", filename));

  if (ico_layers_too_big (image))
    {
      g_message (_("Windows icons cannot be higher or wider than 255 pixels."));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* First, set up the icon specs dialog and show it: */
  if ((icon_depths = ico_show_icon_dialog (image, &num_icons)) == NULL)
    return GIMP_PDB_CANCEL;

  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));

  /* Okay, let's actually save the thing with the depths the
     user specified. */

  if ((exit_state = ico_init (filename, &ico, num_icons) != GIMP_PDB_SUCCESS))
    return exit_state;

  D(("icon initialized ...\n"));

  ico_setup (&ico, image, icon_depths, num_icons);

  D(("icon data created ...\n"));

  exit_state = ico_save(&ico);

  D(("*** icon saved, exit status %i.\n\n", exit_state));

  ico_cleanup (&ico);
  g_free (icon_depths);

  return exit_state;
}
