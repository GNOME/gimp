/*   GIMP - The GNU Image Manipulation Program
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Dr. Halo plug-in
 *
 *   Copyright (C) 2026 Jacob Boerema.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-drhalo-cut-load"
#define PLUG_IN_BINARY "file-drhalo"
#define PLUG_IN_ROLE   "gimp-file-drhalo"

typedef struct _Drhalo      Drhalo;
typedef struct _DrhaloClass DrhaloClass;

struct _Drhalo
{
  GimpPlugIn      parent_instance;
};

struct _DrhaloClass
{
  GimpPlugInClass parent_class;
};


#define DRHALO_TYPE  (drhalo_get_type ())
#define DRHALO(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DRHALO_TYPE, Drhalo))

GType                   drhalo_get_type         (void) G_GNUC_CONST;


static GList          * drhalo_query_procedures   (GimpPlugIn            *plug_in);
static GimpProcedure  * drhalo_create_procedure   (GimpPlugIn            *plug_in,
                                                   const gchar           *name);

static GimpValueArray * drhalo_load               (GimpProcedure         *procedure,
                                                   GimpRunMode            run_mode,
                                                   GFile                 *file,
                                                   GimpMetadata          *metadata,
                                                   GimpMetadataLoadFlags *flags,
                                                   GimpProcedureConfig   *config,
                                                   gpointer               run_data);

static GimpImage      * load_image                (GFile                 *file,
                                                   GObject               *config,
                                                   GimpRunMode            run_mode,
                                                   GError               **error);


G_DEFINE_TYPE (Drhalo, drhalo, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (DRHALO_TYPE)
DEFINE_STD_SET_I18N

typedef struct _HaloHeader
{
  guint16   Width;          /* 00h   Image Width in Pixels */
  guint16   Height;         /* 02h   Image Height in Scan Lines */
  guint16   Reserved;       /* 04h   Reserved Field (set to 0) */
} HaloHeader;

static void
drhalo_class_init (DrhaloClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = drhalo_query_procedures;
  plug_in_class->create_procedure = drhalo_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
drhalo_init (Drhalo *drhalo)
{
}

static GList *
drhalo_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static GimpProcedure *
drhalo_create_procedure (GimpPlugIn  *plug_in,
                         const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           drhalo_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure,
                                     _("Dr. Halo CUT image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load Dr. Halo CUT image format"),
                                        _("Load Dr. Halo CUT image format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Jacob Boerema",
                                      "Jacob Boerema",
                                      "2026");
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-cut");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "cut");
    }

  return procedure;
}

static GimpValueArray *
drhalo_load (GimpProcedure         *procedure,
             GimpRunMode            run_mode,
             GFile                 *file,
             GimpMetadata          *metadata,
             GimpMetadataLoadFlags *flags,
             GimpProcedureConfig   *config,
             gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, G_OBJECT (config), run_mode, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpImage *
load_image (GFile        *file,
            GObject      *config,
            GimpRunMode   run_mode,
            GError      **error)
{
  GimpImage  *image      = NULL;
  GimpLayer  *layer;
  GeglBuffer *buffer;
  guchar     *pixels     = NULL;
  guchar     *line_data  = NULL;
  FILE       *fp;
  HaloHeader  header;
  gsize       file_size;
  gint        pixel;
  gint        max_pixels;
  gint        y;
  gint        highest_index = 0;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  fseek (fp, 0, SEEK_END);
  file_size = ftell (fp);
  fseek (fp, 0, SEEK_SET);

  if (fread (&header, sizeof (HaloHeader), 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Cannot open file for reading: %s\n"),
                   g_file_peek_path (file));
      fclose (fp);
      return NULL;
    }
  file_size -= sizeof (HaloHeader);

  header.Width  = GUINT16_FROM_LE (header.Width);
  header.Height = GUINT16_FROM_LE (header.Height);

  if (header.Width == 0 || header.Height == 0)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Invalid dimensions: %d x %d"),
                   (gint) header.Width, (gint) header.Height * 2);
      fclose (fp);
      return NULL;
    }

  /* FIXME: Check if there is a .PAL palette file with the same name.
   * If found: load the palette and use that for this image
   * and set the mode to INDEXED in that case.
   * NB Check KISS image plug-in if there are similarities...
   */

  image = gimp_image_new_with_precision (header.Width, header.Height, GIMP_GRAY,
                                         GIMP_PRECISION_U8_NON_LINEAR);

  layer = gimp_layer_new (image, _("Background"), header.Width, header.Height,
                          GIMP_GRAY_IMAGE,
                          100, gimp_image_get_default_new_layer_mode (image));

  gimp_image_insert_layer (image, layer, NULL, 0);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  max_pixels = (gint) header.Width * header.Height;
  pixels     = g_try_malloc0 (max_pixels);
  /* We don't know the size of a compressed line in advance. Assume 3 times width */
  line_data  = g_try_malloc0 (3 * header.Width);

  if (! pixels || ! line_data)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("There was not enough memory to complete the "
                     "operation."));
      fclose (fp);
      g_free (pixels);
      g_free (line_data);
      return NULL;
    }

  pixel = 0;
  y     = 0;
  while (y < header.Height && pixel < max_pixels)
    {
      guint16 line_size/*, iline, line_bytes_used*/;
      gint8   run_count, value;
      gint    i, x;

      if (fread (&line_size, 2, 1, fp) == 0)
        {
          /* Only return if no lines have been processed yet.
           * Otherwise, just show partial image with a message.
           */
          if (y == 0)
            {
              g_set_error (error, G_FILE_ERROR, 0,
                           _("Error reading data. Image may be corrupt."));
              fclose (fp);
              g_free (pixels);
              g_free (line_data);
              return NULL;
            }
          gimp_message (_("Error reading data. Image may be corrupt."));
          break;
        }

      line_size = GUINT16_FROM_LE (line_size);

      if (line_size > 3 * header.Width ||
          fread (line_data, line_size, 1, fp) == 0)
        {
          /* Only return if no lines have been processed yet.
           * Otherwise, just show partial image with a message.
           */
          if (y == 0)
            {
              g_set_error (error, G_FILE_ERROR, 0,
                           _("Error reading data. Image may be corrupt."));
              fclose (fp);
              g_free (pixels);
              g_free (line_data);
              return NULL;
            }
          gimp_message (_("Error reading data. Image may be corrupt."));
          break;
        }

      i = 0;
      x = 0;
      while (i < line_size)
        {
          run_count = line_data[i++];
          if (run_count == 0) /* End of line */
            {
              break;
            }
          else if (x >= header.Width || i >= line_size)
            {
              gimp_message (_("Invalid data: image may be corrupt."));
              break;
            }

          value = run_count & 0x7f;

          if ((run_count & 0x80) == 0x80)
            {
              /* Repeat next byte value times*/
              guchar repeat_val = line_data[i++];

              if (repeat_val > highest_index)
                highest_index = repeat_val;

              memset (&pixels[pixel], repeat_val, value);
              pixel += value;
              x += value;
            }
          else /* if ((run_count & 0x80) == 0) */
            {
              if (i + value >= line_size)
                {
                  gimp_message (_("Invalid data: image may be corrupt."));
                  x += value;
                  break;
                }
              for (gint j = 0; j < value; j ++)
                {
                  pixels[pixel++] = line_data[i++];
                  if (line_data[i - 1] > highest_index)
                    highest_index = line_data[i - 1];
                }
              x += value;
            }
        }

      y++;
    }

  fclose (fp);

  /* If we don't have a palette (grayscale), and highest_index == 1,
   * we have apparently a B/W image: Set the grayscale values to 255.
   */
  if (highest_index == 1)
    {
      for (gint pix = 0; pix < (gint) header.Width * header.Height; pix++)
        {
          if (pixels[pix] == 1)
            pixels[pix] = 255;
        }
    }
  else if (highest_index < 128)
    {
      gint shift;

      /* If not the entire 8-bit width is used, upscale the pixel values. */
      if (highest_index < 4)
        shift = 6;
      else if (highest_index < 8)
        shift = 5;
      else if (highest_index < 16)
        shift = 4;
      else if (highest_index < 32)
        shift = 3;
      else if (highest_index < 64)
        shift = 2;
      else
        shift = 1;

      for (gint pix = 0; pix < (gint) header.Width * header.Height; pix++)
        pixels[pix] = (guchar) ((((guint16) pixels[pix] + 1) << shift) - 1);
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, header.Width, header.Height), 0,
                   NULL, pixels, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);
  g_free (pixels);
  g_free (line_data);

  return image;
}
