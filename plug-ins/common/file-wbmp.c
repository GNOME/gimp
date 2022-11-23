/*
 * LIGMA - The GNU Image Manipulation Program
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
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"

#define LOAD_PROC      "file-wbmp-load"
#define PLUG_IN_BINARY "file-wbmp"
#define PLUG_IN_ROLE   "ligma-file-wbmp"

#define ReadOK(file,buffer,len)  (fread(buffer, len, 1, file) != 0)

typedef struct _Wbmp      Wbmp;
typedef struct _WbmpClass WbmpClass;

struct _Wbmp
{
  LigmaPlugIn      parent_instance;
};

struct _WbmpClass
{
  LigmaPlugInClass parent_class;
};

#define WBMP_TYPE  (wbmp_get_type ())
#define WBMP (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WBMP_TYPE, Wbmp))

GType                   wbmp_get_type         (void) G_GNUC_CONST;

static GList          * wbmp_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * wbmp_create_procedure (LigmaPlugIn           *plug_in,
                                               const gchar          *name);

static LigmaValueArray * wbmp_load             (LigmaProcedure        *procedure,
                                               LigmaRunMode           run_mode,
                                               GFile                *file,
                                               const LigmaValueArray *args,
                                               gpointer              run_data);

LigmaImage             * load_image            (GFile                *file,
                                               GError              **error);

static LigmaImage      * read_image            (FILE                 *fd,
                                               GFile                *file,
                                               gint                  width,
                                               gint                  height,
                                               GError              **error);

G_DEFINE_TYPE (Wbmp, wbmp, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (WBMP_TYPE)
DEFINE_STD_SET_I18N

static void
wbmp_class_init (WbmpClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = wbmp_query_procedures;
  plug_in_class->create_procedure = wbmp_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
wbmp_init (Wbmp *wmp)
{
}

static GList *
wbmp_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static LigmaProcedure *
wbmp_create_procedure (LigmaPlugIn  *plug_in,
                       const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           wbmp_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("Wireless BMP image"));

      ligma_procedure_set_documentation (procedure,
                                        _("Loads files of Wireless BMP file format"),
                                        _("Loads files of Wireless BMP file format"),
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Kevin Toyle",
                                      "Kevin Toyle",
                                      "2022");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/vnd.wap.wbmp");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "wbmp");
    }

  return procedure;
}

static LigmaValueArray *
wbmp_load (LigmaProcedure        *procedure,
           LigmaRunMode           run_mode,
           GFile                *file,
           const LigmaValueArray *args,
           gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

LigmaImage *
load_image (GFile   *file,
            GError **error)
{
  FILE      *fd;
  LigmaImage *image  = NULL;
  gint       width  = 0;
  gint       height = 0;
  gint8      magic;
  guchar     value;

  ligma_progress_init_printf (_("Opening '%s'"),
                             ligma_file_get_utf8_name (file));

  fd = g_fopen (g_file_peek_path (file), "rb");

  if (! fd)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   ligma_file_get_utf8_name (file), g_strerror (errno));
      goto out;
    }

  /* Checking the type field to make sure it is 0. */
  if (! ReadOK (fd, &magic, 1) || magic != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s': Invalid WBMP type value"),
                   ligma_file_get_utf8_name (file));
      goto out;
    }

  /* Checking the fixed header field to make sure it is 0 */
  if (! ReadOK (fd, &magic, 1) || magic != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s': Unsupported WBMP fixed header value"),
                   ligma_file_get_utf8_name (file));
      goto out;
    }

  /* The width and height are stored as uintvar values */
  while (ReadOK (fd, &value, 1))
    {
      width = (width << 7) | (value & 0x7F);
      if (value >> 7 != 1)
        break;
    }
  while (ReadOK (fd, &value, 1))
    {
      height = (height << 7) | (value & 0x7F);
      if (value >> 7 != 1)
        break;
    }

  if (width <= 0 || height <= 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a valid WBMP file"),
                   ligma_file_get_utf8_name (file));
      goto out;
    }

  image = read_image (fd, file, width, height, error);

out:
  if (fd)
    fclose (fd);

  return image;
}

/* Code referenced from /plug-ins/file-bmp/bmp-load.c */
static LigmaImage *
read_image (FILE    *fd,
            GFile   *file,
            gint     width,
            gint     height,
            GError **error)
{
  const guchar mono[6] = { 0, 0, 0, 255, 255, 255 };
  guchar       v;
  gint         xpos    = 0;
  gint         ypos    = 0;
  LigmaImage   *image;
  LigmaLayer   *layer;
  GeglBuffer  *buffer;
  guchar      *dest, *temp;
  gint         i, cur_progress, max_progress;

  /* Make a new image in LIGMA */
  if ((width < 0) || (width > LIGMA_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image width: %d"), width);
      return NULL;
    }

  if ((height < 0) || (height > LIGMA_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image height: %d"), height);
      return NULL;
    }

  image = ligma_image_new (width, height, LIGMA_INDEXED);
  layer = ligma_layer_new (image, _("Background"), width, height,
                          LIGMA_INDEXED_IMAGE, 100,
                          ligma_image_get_default_new_layer_mode (image));

  ligma_image_set_file (image, file);
  ligma_image_set_colormap (image, mono, 2);

  ligma_image_insert_layer (image, layer, NULL, 0);

  dest = g_malloc0 (width * height);

  ypos = 0;

  cur_progress = 0;
  max_progress = height;

  while (ReadOK (fd, &v, 1))
    {
      for (i = 1; (i <= 8) && (xpos < width); i++, xpos++)
        {
          temp = dest + (ypos * width) + xpos;
          *temp = (v & (((1 << 1) - 1) << (8 - i))) >> (8 - i);
        }

      if (xpos == width)
        {
          if (ypos == height - 1)
            break;

          ypos++;
          xpos = 0;

          cur_progress++;
          if ((cur_progress % 5) == 0)
            ligma_progress_update ((gdouble) cur_progress / (gdouble) max_progress);
        }

      if (ypos > height - 1)
        break;
    }

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0, NULL, dest,
                   GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);

  g_free (dest);

  return image;
}

