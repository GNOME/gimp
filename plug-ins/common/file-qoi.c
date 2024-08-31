/*   GIMP - The GNU Image Manipulation Program
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Quite OK Image (QOI) plug-in
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

#define QOI_IMPLEMENTATION
#include "qoi.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-qoi-load"
#define EXPORT_PROC    "file-qoi-export"
#define PLUG_IN_BINARY "file-qoi"
#define PLUG_IN_ROLE   "gimp-file-qoi"

typedef struct _Qoi      Qoi;
typedef struct _QoiClass QoiClass;

struct _Qoi
{
  GimpPlugIn      parent_instance;
};

struct _QoiClass
{
  GimpPlugInClass parent_class;
};


#define QOI_TYPE  (qoi_get_type ())
#define QOI(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), QOI_TYPE, Qoi))

GType                   qoi_get_type         (void) G_GNUC_CONST;


static GList          * qoi_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * qoi_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static GimpValueArray * qoi_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);
static GimpValueArray * qoi_export           (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GimpImage             *image,
                                              GFile                 *file,
                                              GimpExportOptions     *options,
                                              GimpMetadata          *metadata,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage      * load_image           (GFile                 *file,
                                              GObject               *config,
                                              GimpRunMode            run_mode,
                                              GError               **error);
static gboolean         export_image         (GFile                 *file,
                                              GimpImage             *image,
                                              GimpDrawable          *drawable,
                                              GError               **error);


G_DEFINE_TYPE (Qoi, qoi, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (QOI_TYPE)
DEFINE_STD_SET_I18N


static void
qoi_class_init (QoiClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = qoi_query_procedures;
  plug_in_class->create_procedure = qoi_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
qoi_init (Qoi *qoi)
{
}

static GList *
qoi_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
qoi_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           qoi_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure,
                                     N_("Quite OK Image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load file in the QOI file format"),
                                        _("Load file in the QOI file format "
                                          "(Quite OK Image)"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2023");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/qoi");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "qoi");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,qoif");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, qoi_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("Quite OK Image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Export image in the QOI file format"),
                                        _("Export image in the QOI file format "
                                          "(Quite OK Image)"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2023");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/qoi");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "qoi");

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
qoi_load (GimpProcedure         *procedure,
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
qoi_export (GimpProcedure        *procedure,
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
  qoi_desc    desc;
  void       *pixels = NULL;
  const Babl *format = NULL;
  FILE       *fp;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  fclose (fp);

  pixels = qoi_read (g_file_peek_path (file), &desc, 0);

  if (! pixels)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Failed to read QOI file"));
      return NULL;
    }

  image = gimp_image_new_with_precision (desc.width, desc.height, GIMP_RGB,
                                         desc.colorspace ?
                                         GIMP_PRECISION_U8_LINEAR :
                                         GIMP_PRECISION_U8_NON_LINEAR);

  layer = gimp_layer_new (image, _("Background"), desc.width, desc.height,
                          (desc.channels == 4) ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE,
                          100, gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, layer, NULL, 0);

  format = babl_format ((desc.channels == 4) ? "R'G'B'A u8" : "R'G'B' u8");

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, desc.width, desc.height), 0,
                   format, pixels, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);
  g_free (pixels);

  return image;
}

static gboolean
export_image (GFile         *file,
              GimpImage     *image,
              GimpDrawable  *drawable,
              GError       **error)
{
  GeglBuffer *buffer;
  const Babl *format;
  qoi_desc    desc;
  void       *pixels = NULL;
  gboolean    has_alpha;
  gint        success;

  has_alpha = gimp_drawable_has_alpha (drawable);

  buffer = gimp_drawable_get_buffer (drawable);

  desc.width    = gegl_buffer_get_width  (buffer);
  desc.height   = gegl_buffer_get_height (buffer);
  desc.channels = has_alpha ? 4 : 3;
  switch (gimp_image_get_precision (image))
    {
      case GIMP_PRECISION_U8_LINEAR:
      case GIMP_PRECISION_U16_LINEAR:
      case GIMP_PRECISION_U32_LINEAR:
      case GIMP_PRECISION_HALF_LINEAR:
      case GIMP_PRECISION_FLOAT_LINEAR:
      case GIMP_PRECISION_DOUBLE_LINEAR:
        desc.colorspace = QOI_LINEAR;
        break;

      default:
        desc.colorspace = QOI_SRGB;
        break;
    }

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  format = babl_format (has_alpha ? "R'G'B'A u8" : "R'G'B' u8");

  pixels = (guchar *) g_malloc (desc.width * desc.height * desc.channels);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, desc.width, desc.height),
                   1.0, format, pixels,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  success = qoi_write (g_file_peek_path (file), pixels, &desc);

  g_object_unref (buffer);
  g_free (pixels);

  if (! success)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Writing to file '%s' failed: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}
