/*   GIMP - The GNU Image Manipulation Program
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Farbfeld Image Format plug-in
 *
 *   Copyright (C) 2023 Alex S.
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


#define LOAD_PROC      "file-farbfeld-load"
#define EXPORT_PROC    "file-farbfeld-export"
#define PLUG_IN_BINARY "file-farbfeld"
#define PLUG_IN_ROLE   "gimp-file-farbfeld"


typedef struct _Farbfeld      Farbfeld;
typedef struct _FarbfeldClass FarbfeldClass;

struct _Farbfeld
{
  GimpPlugIn      parent_instance;
};

struct _FarbfeldClass
{
  GimpPlugInClass parent_class;
};


#define FARBFELD_TYPE  (farbfeld_get_type ())
#define FARBFELD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), FARBFELD_TYPE, Farbfeld))

GType                   farbfeld_get_type         (void) G_GNUC_CONST;


static GList          * farbfeld_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * farbfeld_create_procedure (GimpPlugIn            *plug_in,
                                                   const gchar           *name);

static GimpValueArray * farbfeld_load             (GimpProcedure         *procedure,
                                                   GimpRunMode            run_mode,
                                                   GFile                 *file,
                                                   GimpMetadata          *metadata,
                                                   GimpMetadataLoadFlags *flags,
                                                   GimpProcedureConfig   *config,
                                                   gpointer               run_data);
static GimpValueArray * farbfeld_export           (GimpProcedure         *procedure,
                                                   GimpRunMode            run_mode,
                                                   GimpImage             *image,
                                                   GFile                 *file,
                                                   GimpExportOptions     *options,
                                                   GimpMetadata          *metadata,
                                                   GimpProcedureConfig   *config,
                                                   gpointer               run_data);

static GimpImage      * load_image                (GFile                 *file,
                                                   GObject               *config,
                                                   GimpRunMode            run_mode,
                                                   GError               **error);
static gboolean         export_image              (GFile                 *file,
                                                   GimpImage             *image,
                                                   GimpDrawable          *drawable,
                                                   GError               **error);


G_DEFINE_TYPE (Farbfeld, farbfeld, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FARBFELD_TYPE)
DEFINE_STD_SET_I18N


static void
farbfeld_class_init (FarbfeldClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = farbfeld_query_procedures;
  plug_in_class->create_procedure = farbfeld_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
farbfeld_init (Farbfeld *farbfeld)
{
}

static GList *
farbfeld_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
farbfeld_create_procedure (GimpPlugIn  *plug_in,
                           const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           farbfeld_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure,
                                     _("Farbfeld"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load file in the Farbfeld file "
                                          "format"),
                                        _("Load file in the Farbfeld file "
                                          "format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2023");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "ff");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,farbfeld");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, farbfeld_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("Farbfeld"));

      gimp_procedure_set_documentation (procedure,
                                        _("Export image in the Farbfeld file "
                                          "format"),
                                        _("Export image in the Farbfeld file "
                                          "format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2023");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "ff");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB     |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);
    }

  return procedure;
}

static GimpValueArray *
farbfeld_load (GimpProcedure         *procedure,
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

static GimpValueArray *
farbfeld_export (GimpProcedure        *procedure,
                 GimpRunMode           run_mode,
                 GimpImage            *image,
                 GFile                *file,
                 GimpExportOptions    *options,
                 GimpMetadata         *metadata,
                 GimpProcedureConfig  *config,
                 gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_IGNORE;
  GList             *drawables;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  export = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);

  if (! export_image (file, image, drawables->data, &error))
    status = GIMP_PDB_EXECUTION_ERROR;

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpImage *
load_image (GFile        *file,
            GObject      *config,
            GimpRunMode   run_mode,
            GError      **error)
{
  GimpImage  *image  = NULL;
  GimpLayer  *layer;
  GeglBuffer *buffer;
  guint16    *pixels;
  guchar      magic_number[8];
  guint32     width;
  guint32     height;
  gsize       row_size;
  const Babl *format = babl_format ("R'G'B'A u16");
  FILE       *fp;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  /* Load the header */
  if (! fread (magic_number, 8, 1, fp)          ||
      ! fread (&width, sizeof (guint32), 1, fp) ||
      ! fread (&height, sizeof (guint32), 1, fp))
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Failed to read Farbfeld header"));
      fclose (fp);
      return NULL;
    }

  /* Header information is stored in Big-Endian format */
  width = GUINT32_FROM_BE (width);
  height = GUINT32_FROM_BE (height);

  if (width > GIMP_MAX_IMAGE_SIZE  ||
      height > GIMP_MAX_IMAGE_SIZE ||
      ! g_size_checked_mul (&row_size, width, (sizeof (guint16) * 4)))
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Image dimensions too large: width %d x height %d"),
                   width, height);
      fclose (fp);
      return NULL;
    }

  image = gimp_image_new_with_precision (width, height, GIMP_RGB,
                                         GIMP_PRECISION_U16_NON_LINEAR);

  layer = gimp_layer_new (image, _("Background"), width, height,
                          GIMP_RGBA_IMAGE, 100,
                          gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, layer, NULL, 0);

  pixels = g_try_malloc (row_size);
  if (pixels == NULL)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("There was not enough memory to complete the "
                     "operation."));
      fclose (fp);
      return NULL;
    }

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
  for (gint i = 0; i < height; i++)
    {
      if (! fread (pixels, row_size, 1, fp))
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Premature end of Farbfeld pixel data"));
          return NULL;
        }

      /* Pixels are also stored in Big-Endian format */
      for (gint j = 0; j < (width * 4); j++)
        pixels[j] = GUINT16_FROM_BE (pixels[j]);

      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, i, width, 1), 0,
                       format, pixels, GEGL_AUTO_ROWSTRIDE);
    }
  g_free (pixels);

  fclose (fp);
  g_object_unref (buffer);

  return image;
}

static gboolean
export_image (GFile         *file,
              GimpImage     *image,
              GimpDrawable  *drawable,
              GError       **error)
{
  FILE       *fp;
  GeglBuffer *buffer;
  guint16    *pixels;
  const Babl *format = babl_format ("R'G'B'A u16");
  gchar      *magic_number;
  guint32     image_width;
  guint32     image_height;
  guint32     export_width;
  guint32     export_height;

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "wb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  buffer = gimp_drawable_get_buffer (drawable);

  image_width = gegl_buffer_get_width (buffer);
  image_height = gegl_buffer_get_height (buffer);
  /* Farbfeld values are Big-Endian */
  export_width = GUINT32_TO_BE (image_width);
  export_height = GUINT32_TO_BE (image_height);

  /* Write header */
  magic_number = "farbfeld";
  for (gint i = 0; i < 8; i++)
    fputc (magic_number[i], fp);
  fwrite ((gchar *) &export_width, 1, 4, fp);
  fwrite ((gchar *) &export_height, 1, 4, fp);

  /* Write pixel data */
  for (gint i = 0; i < image_height; i++)
    {
      pixels = g_malloc (image_width * sizeof (guint16) * 4);

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, image_width, 1),
                       1.0, format, pixels,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (gint j = 0; j < (image_width * 4); j++)
        {
          pixels[j] = GUINT16_TO_BE (pixels[j]);
          fwrite ((gchar *) &pixels[j], 1, 2, fp);
        }

      g_free (pixels);
      gimp_progress_update (i / (gdouble) image_height);
    }

  gimp_progress_update (1.0);

  fclose (fp);

  if (buffer)
    g_object_unref (buffer);

  return TRUE;
}
