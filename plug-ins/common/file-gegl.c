/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-gegl.c -- GEGL based file format plug-in
 * Copyright (C) 2012 Simon Budig <simon@gimp.org>
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

#include <stdlib.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_BINARY "file-gegl"


typedef struct _FileFormat FileFormat;

struct _FileFormat
{
  const gchar *file_type;
  const gchar *mime_type;
  const gchar *extensions;
  const gchar *magic;

  const gchar *load_proc;
  const gchar *load_blurb;
  const gchar *load_help;

  const gchar *save_proc;
  const gchar *save_blurb;
  const gchar *save_help;
};


typedef struct _Goat      Goat;
typedef struct _GoatClass GoatClass;

struct _Goat
{
  GimpPlugIn      parent_instance;
};

struct _GoatClass
{
  GimpPlugInClass parent_class;
};


#define GOAT_TYPE  (goat_get_type ())
#define GOAT (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOAT_TYPE, Goat))

GType                   goat_get_type         (void) G_GNUC_CONST;

static GList          * goat_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * goat_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * goat_load             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);
static GimpValueArray * goat_save             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               gint                  n_drawables,
                                               GimpDrawable        **drawables,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);

static GimpImage      * load_image            (GFile                *file,
                                               GError              **error);
static gboolean         save_image            (GFile                *file,
                                               GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               GError              **error);


G_DEFINE_TYPE (Goat, goat, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GOAT_TYPE)


static const FileFormat file_formats[] =
{
  {
    N_("Radiance RGBE"),
    "image/vnd.radiance",
    "hdr",
    "0,string,?#",

    "file-load-rgbe",
    "Load files in the RGBE file format",
    "This procedure loads images in the RGBE format, using gegl:load",

    "file-save-rgbe",
    "Saves files in the RGBE file format",
    "This procedure exports images in the RGBE format, using gegl:save"
  },
  {
    N_("OpenEXR image"),
    "image/x-exr",
    "exr",
    "0,lelong,20000630",

    /* no EXR loading (implemented in native GIMP plug-in) */
    NULL, NULL, NULL,

    "file-exr-save",
    "Saves files in the OpenEXR file format",
    "This procedure saves images in the OpenEXR format, using gegl:save"
  }
};


static void
goat_class_init (GoatClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = goat_query_procedures;
  plug_in_class->create_procedure = goat_create_procedure;
}

static void
goat_init (Goat *goat)
{
}

static GList *
goat_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;
  gint   i;

  for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];

      if (format->load_proc)
        list = g_list_append (list, g_strdup (format->load_proc));

      if (format->save_proc)
        list = g_list_append (list, g_strdup (format->save_proc));
    }

  return list;
}

static GimpProcedure *
goat_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;
  gint           i;

  for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];

      if (! g_strcmp0 (name, format->load_proc))
        {
          procedure = gimp_load_procedure_new (plug_in, name,
                                               GIMP_PDB_PROC_TYPE_PLUGIN,
                                               goat_load,
                                               (gpointer) format, NULL);

          gimp_procedure_set_menu_label (procedure, format->file_type);

          gimp_procedure_set_documentation (procedure,
                                            format->load_blurb,
                                            format->load_help,
                                            name);

          gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                              format->mime_type);
          gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                              format->extensions);
          gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                          format->magic);
        }
      else if (! g_strcmp0 (name, format->save_proc))
        {
          procedure = gimp_save_procedure_new (plug_in, name,
                                               GIMP_PDB_PROC_TYPE_PLUGIN,
                                               goat_save,
                                               (gpointer) format, NULL);

          gimp_procedure_set_image_types (procedure, "*");

          gimp_procedure_set_menu_label (procedure, format->file_type);

          gimp_procedure_set_documentation (procedure,
                                            format->save_blurb,
                                            format->save_help,
                                            name);

          gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                              format->mime_type);
          gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                              format->extensions);
        }
    }

  return procedure;
}

static GimpValueArray *
goat_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  image = load_image (file, &error);

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
goat_save (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GimpImage            *image,
           gint                  n_drawables,
           GimpDrawable        **drawables,
           GFile                *file,
           const GimpValueArray *args,
           gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);

      export = gimp_export_image (&image, &n_drawables, &drawables, "GEGL",
                                  GIMP_EXPORT_CAN_HANDLE_RGB     |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("GEGL export plug-in does not support multiple layers."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }

  if (! save_image (file, image, drawables[0],
                    &error))
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  if (export == GIMP_EXPORT_EXPORT)
    {
      gimp_image_delete (image);
      g_free (drawables);
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpImage *
load_image (GFile   *file,
            GError **error)
{
  GimpImage         *image;
  GimpLayer         *layer;
  gchar             *filename;
  GimpImageType      image_type;
  GimpImageBaseType  base_type;
  GimpPrecision      precision;
  gint               width;
  gint               height;
  GeglNode          *graph;
  GeglNode          *sink;
  GeglNode          *source;
  GeglBuffer        *src_buf  = NULL;
  GeglBuffer        *dest_buf = NULL;
  const Babl        *format;

  filename = g_file_get_path (file);

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  graph = gegl_node_new ();

  source = gegl_node_new_child (graph,
                                "operation", "gegl:load",
                                "path",      filename,
                                NULL);
  sink = gegl_node_new_child (graph,
                              "operation", "gegl:buffer-sink",
                              "buffer",    &src_buf,
                              NULL);

  gegl_node_connect_to (source, "output",
                        sink,   "input");

  gegl_node_process (sink);

  g_object_unref (graph);

  if (! src_buf)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s'"),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  gimp_progress_update (0.33);

  width  = gegl_buffer_get_width (src_buf);
  height = gegl_buffer_get_height (src_buf);
  format = gegl_buffer_get_format (src_buf);

  if (babl_format_is_palette (format))
    {
      base_type = GIMP_INDEXED;

      if (babl_format_has_alpha (format))
        image_type = GIMP_INDEXEDA_IMAGE;
      else
        image_type = GIMP_INDEXED_IMAGE;

      precision = GIMP_PRECISION_U8_NON_LINEAR;
    }
  else
    {
      const Babl *model  = babl_format_get_model (format);
      const Babl *type   = babl_format_get_type (format, 0);
      gboolean    linear = TRUE;

      if (model == babl_model ("Y")  ||
          model == babl_model ("Y'") ||
          model == babl_model ("YA") ||
          model == babl_model ("Y'A"))
        {
          base_type = GIMP_GRAY;

          if (babl_format_has_alpha (format))
            image_type = GIMP_GRAYA_IMAGE;
          else
            image_type = GIMP_GRAY_IMAGE;

          if (model == babl_model ("Y'") ||
              model == babl_model ("Y'A"))
            linear = FALSE;
        }
      else
        {
          base_type = GIMP_RGB;

          if (babl_format_has_alpha (format))
            image_type = GIMP_RGBA_IMAGE;
          else
            image_type = GIMP_RGB_IMAGE;

          if (model == babl_model ("R'G'B'") ||
              model == babl_model ("R'G'B'A"))
            linear = FALSE;
        }

      if (linear)
        {
          if (type == babl_type ("u8"))
            precision = GIMP_PRECISION_U8_LINEAR;
          else if (type == babl_type ("u16"))
            precision = GIMP_PRECISION_U16_LINEAR;
          else if (type == babl_type ("u32"))
            precision = GIMP_PRECISION_U32_LINEAR;
          else if (type == babl_type ("half"))
            precision = GIMP_PRECISION_HALF_LINEAR;
          else
            precision = GIMP_PRECISION_FLOAT_LINEAR;
        }
      else
        {
          if (type == babl_type ("u8"))
            precision = GIMP_PRECISION_U8_NON_LINEAR;
          else if (type == babl_type ("u16"))
            precision = GIMP_PRECISION_U16_NON_LINEAR;
          else if (type == babl_type ("u32"))
            precision = GIMP_PRECISION_U32_NON_LINEAR;
          else if (type == babl_type ("half"))
            precision = GIMP_PRECISION_HALF_NON_LINEAR;
          else
            precision = GIMP_PRECISION_FLOAT_NON_LINEAR;
        }
    }


  image = gimp_image_new_with_precision (width, height,
                                         base_type, precision);
  gimp_image_set_file (image, file);

  layer = gimp_layer_new (image,
                          _("Background"),
                          width, height,
                          image_type,
                          100,
                          gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, layer, NULL, 0);

  dest_buf = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gimp_progress_update (0.66);

  gegl_buffer_copy (src_buf, NULL, GEGL_ABYSS_NONE, dest_buf, NULL);

  g_object_unref (src_buf);
  g_object_unref (dest_buf);

  gimp_progress_update (1.0);

  return image;
}

static gboolean
save_image (GFile         *file,
            GimpImage     *image,
            GimpDrawable  *drawable,
            GError       **error)
{
  gchar      *filename;
  GeglNode   *graph;
  GeglNode   *source;
  GeglNode   *sink;
  GeglBuffer *src_buf;

  filename = g_file_get_path (file);

  src_buf = gimp_drawable_get_buffer (drawable);

  graph = gegl_node_new ();

  source = gegl_node_new_child (graph,
                                "operation", "gegl:buffer-source",
                                "buffer",    src_buf,
                                NULL);
  sink = gegl_node_new_child (graph,
                              "operation", "gegl:save",
                              "path",      filename,
                              NULL);

  gegl_node_connect_to (source, "output",
                        sink,   "input");

  gegl_node_process (sink);

  g_object_unref (graph);
  g_object_unref (src_buf);

  return TRUE;
}
