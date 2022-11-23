/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * file-icns-load.c
 * Copyright (C) 2004 Brion Vibber <brion@pobox.com>
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include <png.h>

/* #define ICNS_DBG */

#include "file-icns.h"
#include "file-icns-data.h"
#include "file-icns-load.h"

#include "libligma/stdplugins-intl.h"

IcnsResource * resource_load     (FILE         *file);

IcnsResource * resource_find     (IcnsResource *list,
                                  gchar        *type,
                                  gint          max);

gboolean       resource_get_next (IcnsResource *icns,
                                  IcnsResource *res);

void           icns_slurp        (guchar       *dest,
                                  IconType     *icontype,
                                  IcnsResource *icns,
                                  IcnsResource *mask);

gboolean       icns_decompress   (guchar       *dest,
                                  IconType     *icontype,
                                  IcnsResource *image,
                                  IcnsResource *mask);

void           icns_attach_image (LigmaImage    *image,
                                  IconType     *icontype,
                                  IcnsResource *icns,
                                  IcnsResource *mask,
                                  gboolean      isOSX);

LigmaImage *    icns_load         (IcnsResource *icns,
                                  GFile        *file);

/* Ported from Brion Vibber's icnsload.c code, under the GPL license, version 3
 * or any later version of the license */
IcnsResource *
resource_load (FILE *file)
{
  IcnsResource *res = NULL;

  if (file)
    {
      IcnsResourceHeader header;

      if (1 == fread (&header, sizeof (IcnsResourceHeader), 1, file))
        {
          gchar   type[5];
          guint32 size;

          strncpy (type, header.type, 4);
          type[4] = '\0';
          size = GUINT32_FROM_BE (header.size);

          if (! strncmp (header.type, "icns", 4) && size > sizeof (IcnsResourceHeader))
            {
              res = (IcnsResource *) g_new (guchar, sizeof (IcnsResource) + size);
              strncpy (res->type, header.type, 4);
              res->type[4] = '\0';
              res->size = size;
              res->cursor = sizeof (IcnsResourceHeader);
              res->data = (guchar *) res + sizeof (IcnsResource);
              fseek (file, 0, SEEK_SET);

              if (size != fread (res->data, 1, res->size, file))
                {
                  g_message ("** expected %d bytes\n", size);
                  g_free (res);
                  res = NULL;
                }
            }
        }
      else
        {
          g_message (("** couldn't read icns header.\n"));
        }
    }
  else
    {
      g_message (("** couldn't open file.\n"));
    }
  return res;
}

IcnsResource *
resource_find (IcnsResource *list,
               gchar        *type,
               gint          max)
{
  for (gint i = 0; i < max; i++)
    {
      if (! strncmp (list[i].type, type, 4))
        return &list[i];
    }
  return NULL;
}

gboolean
resource_get_next (IcnsResource *icns,
                   IcnsResource *res)
{
  IcnsResourceHeader *header;

  if (icns->size - icns->cursor < sizeof (IcnsResourceHeader))
    return FALSE;

  header = (IcnsResourceHeader *) &(icns->data[icns->cursor]);
  strncpy (res->type, header->type, 4);
  res->size   = GUINT32_FROM_BE (header->size);
  res->cursor = sizeof (IcnsResourceHeader);
  res->data   = &(icns->data[icns->cursor]);

  icns->cursor += res->size;
  if (icns->cursor > icns->size)
    {
      gchar typestring[5];
      fourcc_get_string (icns->type, typestring);
      g_message ("icns resource_get_next: resource too big! type '%s', size %u\n",
                 typestring, icns->size);

      return FALSE;
    }
  return TRUE;
}

LigmaImage *
icns_load (IcnsResource *icns,
           GFile        *file)
{
  IcnsResource *resources;
  guint         nResources;
  gfloat        current_resources = 0;
  LigmaImage    *image;

  resources = g_new (IcnsResource, 256);

  /* Largest .icns icon is 1024 x 1024 */
  image = ligma_image_new (1024, 1024, LIGMA_RGB);
  ligma_image_set_file (image, file);

  nResources = 0;
  while (resource_get_next (icns, &resources[nResources++])) {}

  for (gint i = 0; iconTypes[i].type; i++)
    {
      IcnsResource *icns;
      IcnsResource *mask = NULL;

      if ((icns = resource_find (resources, iconTypes[i].type, nResources)))
        {
          if (! iconTypes[i].isModern)
            mask = resource_find (resources, iconTypes[i].mask, nResources);

          icns_attach_image (image, &iconTypes[i], icns, mask, iconTypes[i].isModern);

          ligma_progress_update (current_resources++ / nResources);
        }
    }

  ligma_image_resize_to_layers (image);
  g_free (resources);
  return image;
}

void
icns_slurp (guchar       *dest,
            IconType     *icontype,
            IcnsResource *icns,
            IcnsResource *mask)
{
  guint  out;
  guint  max;
  guchar bucket = 0;
  guchar bit;
  guint  index;

  max = icontype->width * icontype->height;
  icns->cursor = sizeof (IcnsResourceHeader);

  switch (icontype->bits)
    {
      case 1:
        for (out = 0; out < max; out++)
          {
            if (out % 8 == 0)
              bucket = icns->data[icns->cursor++];

            bit = (bucket & 0x80) ? 0 : 255;
            bucket = bucket << 1;
            dest[out * 4]     = bit;
            dest[out * 4 + 1] = bit;
            dest[out * 4 + 2] = bit;
          }
        break;
      case 4:
        for (out = 0; out < max; out++)
          {
            if (out % 2 == 0)
              bucket = icns->data[icns->cursor++];

            index = 3 * (bucket & 0xf0) >> 4;
            bucket = bucket << 4;
            dest[out * 4]     = icns_colormap_4[index];
            dest[out * 4 + 1] = icns_colormap_4[index + 1];
            dest[out * 4 + 2] = icns_colormap_4[index + 2];
          }
        break;
      case 8:
        for (out = 0; out < max; out++)
          {
            index = 3 * icns->data[icns->cursor++];
            dest[out * 4]     = icns_colormap_8[index];
            dest[out * 4 + 1] = icns_colormap_8[index + 1];
            dest[out * 4 + 2] = icns_colormap_8[index + 2];
            dest[out * 4 + 3] = 255;
          }
        break;
      case 32:
        for (out = 0; out < max; out++)
          {
            dest[out * 4]     = icns->data[icns->cursor++];
            dest[out * 4 + 1] = icns->data[icns->cursor++];
            dest[out * 4 + 2] = icns->data[icns->cursor++];
            /* Throw away alpha, use the mask */
            icns->cursor++;
            if (mask)
              dest[out * 4 + 3] = icns->data[mask->cursor++];
            else
              dest[out * 4 + 3] = 255;
          }
        break;
      }

    /* Now for the mask */
    if (mask && icontype->bits != 32)
      {
        mask->cursor = sizeof (IcnsResourceHeader) + icontype->width * icontype->height / 8;
        for (out = 0; out < max; out++)
          {
            if (out % 8 == 0)
              bucket = mask->data[mask->cursor++];

            bit = (bucket & 0x80) ? 255 : 0;
            bucket = bucket << 1;
            dest[out * 4 + 3] = bit;
          }
      }
}

gboolean
icns_decompress (guchar       *dest,
                 IconType     *icontype,
                 IcnsResource *image,
                 IcnsResource *mask)
{
  guint  max;
  guint  channel;
  guint  out;
  guchar run;
  guchar val;
  gint   n_channels = 3;

  max = icontype->width * icontype->height;
  memset (dest, 255, max * 4);

  /* For some reason there seem to be 4 null bytes at the start of an it32. */
  if (! strncmp (icontype->type, "it32", 4))
    image->cursor += 4;

  for (channel = 0; channel < n_channels; channel++)
    {
      out = 0;
      while (out < max)
        {
          run = image->data[image->cursor++];

          if (run & 0x80)
            {
              /* Compressed */
              if (image->cursor >= image->size)
                {
                  g_message ("Corrupt icon: compressed run overflows input size.");
                  return FALSE;
                }

              val = image->data[image->cursor++];

              for (run -= 125; run > 0; run--)
                {
                  if (out >= max)
                    {
                      g_message ("Corrupt icon? compressed run overflows output size.");
                      return FALSE;
                    }
                  dest[out++ * 4 + channel] = val;
                }
            }
          else
            {
              /* Uncompressed */
              for (run += 1; run > 0; run--)
                {
                  if (image->cursor >= image->size)
                    {
                      g_message ("Corrupt icon: uncompressed run overflows input size.");
                      return FALSE;
                    }
                  if (out >= max)
                    {
                      g_message ("Corrupt icon: uncompressed run overflows output size.");
                      return FALSE;
                    }
                  dest[out++ * 4 + channel] = image->data[image->cursor++];
                }
            }
        }
    }

    if (mask)
      {
        gchar typestring[5];
        fourcc_get_string (mask->type, typestring);

        for (out = 0; out < max; out++)
          dest[out * 4 + 3] = mask->data[mask->cursor++];
      }
    return TRUE;
}

void
icns_attach_image (LigmaImage    *image,
                   IconType     *icontype,
                   IcnsResource *icns,
                   IcnsResource *mask,
                   gboolean      isOSX)
{
  gchar           layer_name[5];
  guchar         *dest;
  LigmaLayer      *layer;
  GeglBuffer     *buffer;
  guint           row;
  guint           expected_size;
  gboolean        layer_loaded = FALSE;

  strncpy (layer_name, icontype->type, 4);
  layer_name[4] = '\0';

  row = 4 * icontype->width;
  dest = g_malloc (row * icontype->height);

  expected_size =
    (icontype->width * icontype->height * icontype->bits) / 8;

  if (icns == mask)
    expected_size *= 2;

  expected_size += sizeof (IcnsResourceHeader);

  if (isOSX)
    {
      gchar           image_type[5];
      LigmaImage      *temp_image;
      GFile          *temp_file      = NULL;
      FILE           *fp;
      LigmaValueArray *return_vals    = NULL;
      LigmaLayer     **layers;
      LigmaLayer      *new_layer;
      gint            n_layers;
      gchar          *temp_file_type = NULL;
      gchar          *procedure_name = NULL;

      temp_image = ligma_image_new (icontype->width, icontype->height,
                                   ligma_image_get_base_type (image));

      strncpy (image_type, (gchar *) icns->data + 8, 4);
      image_type[4] = '\0';

      /* PNG */
      if (! strncmp (image_type, "\x89\x50\x4E\x47", 4))
        {
          temp_file_type = "png";
          procedure_name = "file-png-load";
        }
      /* JPEG 2000 */
      else if (! strncmp (image_type, "\x0CjP", 3))
        {
          temp_file_type = "jp2";
          procedure_name = "file-jp2-load";
        }

      if (temp_file_type && procedure_name)
        {
          temp_file = ligma_temp_file (temp_file_type);
          fp = g_fopen (g_file_peek_path (temp_file), "wb");

          if (! fp)
            {
              g_message (_("Error trying to open temporary %s file '%s' "
                         "for icns loading: %s"),
                         temp_file_type,
                         ligma_file_get_utf8_name (temp_file),
                         g_strerror (errno));
              return;
            }

          fwrite (icns->data + 8, sizeof (guchar), icns->size - 8, fp);
          fclose (fp);

          return_vals =
            ligma_pdb_run_procedure (ligma_get_pdb (),
                                    procedure_name,
                                    LIGMA_TYPE_RUN_MODE, LIGMA_RUN_NONINTERACTIVE,
                                    G_TYPE_FILE,        temp_file,
                                    G_TYPE_NONE);
        }

      if (temp_image && return_vals)
        {
          temp_image = g_value_get_object (ligma_value_array_index (return_vals, 1));

          layers = ligma_image_get_layers (temp_image, &n_layers);
          new_layer = ligma_layer_new_from_drawable (LIGMA_DRAWABLE (layers[0]), image);
          ligma_item_set_name (LIGMA_ITEM (new_layer), layer_name);
          ligma_image_insert_layer (image, new_layer, NULL, 0);

          layer_loaded = TRUE;

          g_file_delete (temp_file, NULL, NULL);
          g_object_unref (temp_file);
          ligma_value_array_unref (return_vals);
          g_free (layers);
        }
    }
  else
    {
      if (icontype->bits != 32 || expected_size == icns->size)
        icns_slurp (dest, icontype, icns, mask);
      else
        icns_decompress (dest, icontype, icns, mask);
    }

  if (! layer_loaded)
    {
      layer = ligma_layer_new (image, layer_name, icontype->width, icontype->height,
                              LIGMA_RGBA_IMAGE, 100,
                              ligma_image_get_default_new_layer_mode (image));

      buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, 0, icontype->width, icontype->height),
                       0, NULL,
                       dest, GEGL_AUTO_ROWSTRIDE);

      ligma_image_insert_layer (image, layer, NULL, 0);

      g_object_unref (buffer);
    }

  g_free (dest);
}

LigmaImage *
icns_load_image (GFile        *file,
                 gint32       *file_offset,
                 GError      **error)
{
  FILE          *fp;
  IcnsResource  *icns;
  LigmaImage     *image;

  gegl_init (NULL, NULL);

  ligma_progress_init_printf (_("Opening '%s'"),
                             ligma_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   ligma_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  icns = resource_load (fp);

  fclose (fp);

  if (! icns)
    {
      g_message ("Invalid or corrupt icns resource file.");
      return NULL;
    }

  image = icns_load (icns, file);

  g_free (icns);

  ligma_progress_update (1.0);

  return image;
}

LigmaImage *
icns_load_thumbnail_image (GFile   *file,
                           gint    *width,
                           gint    *height,
                           gint32   file_offset,
                           GError **error)
{
  gint          w          = 0;
  FILE         *fp;
  LigmaImage    *image      = NULL;
  IcnsResource *icns;
  IcnsResource *resources;
  IcnsResource *mask       = NULL;
  guint         i;
  gint          match      = -1;
  guint         nResources = 0;

  gegl_init (NULL, NULL);

  ligma_progress_init_printf (_("Opening thumbnail for '%s'"),
                             ligma_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   ligma_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  icns = resource_load (fp);
  fclose (fp);

  if (! icns)
  {
    g_message ("Invalid or corrupt icns resource file.");
    return NULL;
  }

  image = ligma_image_new (1024, 1024, LIGMA_RGB);

  resources = g_new (IcnsResource, 256);
  while (resource_get_next (icns, &resources[nResources++])) {}

  for (i = 0; iconTypes[i].type; i++)
    {
      if ((icns = resource_find (resources, iconTypes[i].type, nResources)))
        {
          if (iconTypes[i].width > w)
            {
              w = iconTypes[i].width;
              match = i;
            }
        }
    }

  if (match > -1)
    {
      icns = resource_find (resources, iconTypes[match].type, nResources);

      if (! iconTypes[match].isModern)
        mask = resource_find (resources, iconTypes[match].mask, nResources);

      icns_attach_image (image, &iconTypes[match], icns, mask, iconTypes[match].isModern);

      ligma_image_resize_to_layers (image);
    }
  else
    {
      g_message ("Invalid or corrupt icns resource file.");
      return NULL;
    }

  g_free (resources);

  ligma_progress_update (1.0);

  return image;
}
