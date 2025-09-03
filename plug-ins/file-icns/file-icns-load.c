/* GIMP - The GNU Image Manipulation Program
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <png.h>

/* #define ICNS_DBG */

#include "file-icns.h"
#include "file-icns-data.h"
#include "file-icns-load.h"

#include "libgimp/stdplugins-intl.h"

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
                                  gint          n_channels,
                                  IcnsResource *image,
                                  IcnsResource *mask);

void           icns_attach_image (GimpImage    *image,
                                  IconType     *icontype,
                                  IcnsResource *icns,
                                  IcnsResource *mask,
                                  gboolean      isOSX);

GimpImage *    icns_load         (IcnsResource *icns,
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

GimpImage *
icns_load (IcnsResource *icns,
           GFile        *file)
{
  IcnsResource *resources;
  guint         nResources;
  gfloat        current_resources = 0;
  GimpImage    *image;

  resources = g_new (IcnsResource, 256);

  /* Largest .icns icon is 1024 x 1024 */
  image = gimp_image_new (1024, 1024, GIMP_RGB);

  nResources = 0;
  while (resource_get_next (icns, &resources[nResources++])) {}

  for (gint i = 0; iconTypes[i].type; i++)
    {
      IcnsResource *icns;
      IcnsResource *mask = NULL;

      if ((icns = resource_find (resources, iconTypes[i].type, nResources)))
        {
          if (! iconTypes[i].isModern && iconTypes[i].mask)
            mask = resource_find (resources, iconTypes[i].mask, nResources);

          icns_attach_image (image, &iconTypes[i], icns, mask, iconTypes[i].isModern);

          gimp_progress_update (current_resources++ / nResources);
        }
    }

  gimp_image_resize_to_layers (image);
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
            if (! mask)
              dest[out * 4 + 3] = 255;
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
                 gint          n_channels,
                 IcnsResource *image,
                 IcnsResource *mask)
{
  guint  max;
  guint  channel;
  guint  out;
  guchar run;
  guchar val;

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

  /* If we have four channels, this is compressed ARGB data and
     we need to rotate the channels */
  if (n_channels == 4)
    {
      gint pixel_max = max * 4;

      for (gint i = 0; i < pixel_max; i += 4)
        {
          guchar alpha = dest[i];

          for (gint j = 0; j < 3; j++)
            dest[i + j] = dest[i + j + 1];

          dest[i + 3] = alpha;
        }
    }
  else if (mask)
    {
      gchar typestring[5];
      fourcc_get_string (mask->type, typestring);

      for (out = 0; out < max; out++)
        dest[out * 4 + 3] = mask->data[mask->cursor++];
    }
  return TRUE;
}

void
icns_attach_image (GimpImage    *image,
                   IconType     *icontype,
                   IcnsResource *icns,
                   IcnsResource *mask,
                   gboolean      isOSX)
{
  gchar           layer_name[5];
  guchar         *dest;
  GimpLayer      *layer;
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
      GimpImage      *temp_image;
      GFile          *temp_file      = NULL;
      FILE           *fp;
      GimpValueArray *return_vals    = NULL;
      GimpLayer     **layers;
      GimpLayer      *new_layer;
      gchar          *temp_file_type = NULL;
      gchar          *procedure_name = NULL;

      temp_image = gimp_image_new (icontype->width, icontype->height,
                                   gimp_image_get_base_type (image));

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
      /* ARGB (compressed */
      else if (! strncmp (image_type, "ARGB", 4))
        {
          icns->cursor += 4;
          icns_decompress (dest, icontype, 4, icns, FALSE);
        }

      if (temp_file_type && procedure_name)
        {
          GimpProcedure *procedure;

          temp_file = gimp_temp_file (temp_file_type);
          fp = g_fopen (g_file_peek_path (temp_file), "wb");

          if (! fp)
            {
              g_message (_("Error trying to open temporary %s file '%s' "
                         "for icns loading: %s"),
                         temp_file_type,
                         gimp_file_get_utf8_name (temp_file),
                         g_strerror (errno));
              return;
            }

          fwrite (icns->data + 8, sizeof (guchar), icns->size - 8, fp);
          fclose (fp);

          procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (), procedure_name);
          return_vals = gimp_procedure_run (procedure,
                                            "run-mode", GIMP_RUN_NONINTERACTIVE,
                                            "file",     temp_file,
                                            NULL);
        }

      if (temp_image && return_vals)
        {
          temp_image = g_value_get_object (gimp_value_array_index (return_vals, 1));

          layers = gimp_image_get_layers (temp_image);
          new_layer = gimp_layer_new_from_drawable (GIMP_DRAWABLE (layers[0]), image);
          gimp_item_set_name (GIMP_ITEM (new_layer), layer_name);
          gimp_image_insert_layer (image, new_layer, NULL, 0);

          layer_loaded = TRUE;

          /* Use the first color profile we encounter */
          if (! gimp_image_get_color_profile (image) &&
              gimp_image_get_color_profile (temp_image))
            {
              GimpColorProfile *profile;

              profile = gimp_image_get_color_profile (temp_image);
              gimp_image_set_color_profile (image, profile);

              g_object_unref (profile);
            }

          g_file_delete (temp_file, NULL, NULL);
          g_object_unref (temp_file);
          g_free (layers);
        }
      g_clear_pointer (&return_vals, gimp_value_array_unref);
    }
  else
    {
      if (icontype->bits != 32 || expected_size == icns->size)
        icns_slurp (dest, icontype, icns, mask);
      else
        icns_decompress (dest, icontype, 3, icns, mask);
    }

  if (! layer_loaded)
    {
      layer = gimp_layer_new (image, layer_name, icontype->width, icontype->height,
                              GIMP_RGBA_IMAGE, 100,
                              gimp_image_get_default_new_layer_mode (image));

      buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, 0, icontype->width, icontype->height),
                       0, NULL,
                       dest, GEGL_AUTO_ROWSTRIDE);

      gimp_image_insert_layer (image, layer, NULL, 0);

      g_object_unref (buffer);
    }

  g_free (dest);
}

GimpImage *
icns_load_image (GFile        *file,
                 gint32       *file_offset,
                 GError      **error)
{
  FILE          *fp;
  IcnsResource  *icns;
  GimpImage     *image;

  gegl_init (NULL, NULL);

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
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

  gimp_progress_update (1.0);

  return image;
}

GimpImage *
icns_load_thumbnail_image (GFile   *file,
                           gint    *width,
                           gint    *height,
                           gint32   file_offset,
                           GError **error)
{
  gint          w          = 0;
  gint          target_w   = *width;
  FILE         *fp;
  GimpImage    *image      = NULL;
  IcnsResource *icns;
  IcnsResource *resources;
  IcnsResource *mask       = NULL;
  guint         i;
  gint          match      = -1;
  guint         nResources = 0;

  gegl_init (NULL, NULL);

  gimp_progress_init_printf (_("Opening thumbnail for '%s'"),
                             gimp_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  icns = resource_load (fp);
  fclose (fp);

  if (! icns)
  {
    g_message ("Invalid or corrupt icns resource file.");
    return NULL;
  }

  image = gimp_image_new (1024, 1024, GIMP_RGB);

  resources = g_new (IcnsResource, 256);
  while (resource_get_next (icns, &resources[nResources++])) {}

  *width  = 0;
  *height = 0;
  for (i = 0; iconTypes[i].type; i++)
    {
      if ((icns = resource_find (resources, iconTypes[i].type, nResources)))
        {
          if (iconTypes[i].width > w && iconTypes[i].width <= target_w)
            {
              w = iconTypes[i].width;
              match = i;
            }
        }
      *width  = MAX (*width, iconTypes[i].width);
      *height = MAX (*height, iconTypes[i].height);
    }

  if (match == -1)
    {
      /* We didn't find any icon with size smaller or equal to the target.
       * Settle with the smallest bigger icon instead.
       */
      for (i = 0; iconTypes[i].type; i++)
        {
          if ((icns = resource_find (resources, iconTypes[i].type, nResources)))
            {
              if (match == -1 || iconTypes[i].width < w)
                {
                  w = iconTypes[i].width;
                  match = i;
                }
            }
        }
    }

  if (match > -1)
    {
      icns = resource_find (resources, iconTypes[match].type, nResources);

      if (! iconTypes[match].isModern && iconTypes[i].mask)
        mask = resource_find (resources, iconTypes[match].mask, nResources);

      icns_attach_image (image, &iconTypes[match], icns, mask, iconTypes[match].isModern);

      gimp_image_resize_to_layers (image);
    }
  else
    {
      g_message ("Invalid or corrupt icns resource file.");
      return NULL;
    }

  g_free (resources);

  gimp_progress_update (1.0);

  return image;
}
