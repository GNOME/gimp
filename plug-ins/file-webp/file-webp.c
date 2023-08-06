/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-webp - WebP file format plug-in for the GIMP
 * Copyright (C) 2015  Nathan Osman
 * Copyright (C) 2016  Ben Touchette
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <webp/encode.h>

#include "file-webp-dialog.h"
#include "file-webp-load.h"
#include "file-webp-save.h"
#include "file-webp.h"

#include "libgimp/stdplugins-intl.h"


typedef struct _Webp      Webp;
typedef struct _WebpClass WebpClass;

struct _Webp
{
  GimpPlugIn      parent_instance;
};

struct _WebpClass
{
  GimpPlugInClass parent_class;
};


#define WEBP_TYPE  (webp_get_type ())
#define WEBP (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBP_TYPE, Webp))

GType                   webp_get_type         (void) G_GNUC_CONST;

static GList          * webp_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * webp_create_procedure (GimpPlugIn            *plug_in,
                                               const gchar           *name);

static GimpValueArray * webp_load             (GimpProcedure         *procedure,
                                               GimpRunMode            run_mode,
                                               GFile                 *file,
                                               GimpMetadata          *metadata,
                                               GimpMetadataLoadFlags *flags,
                                               GimpProcedureConfig   *config,
                                               gpointer               run_data);
static GimpValueArray * webp_save             (GimpProcedure         *procedure,
                                               GimpRunMode            run_mode,
                                               GimpImage             *image,
                                               gint                   n_drawables,
                                               GimpDrawable         **drawables,
                                               GFile                 *file,
                                               GimpMetadata          *metadata,
                                               GimpProcedureConfig   *config,
                                               gpointer               run_data);


G_DEFINE_TYPE (Webp, webp, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (WEBP_TYPE)
DEFINE_STD_SET_I18N


static void
webp_class_init (WebpClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = webp_query_procedures;
  plug_in_class->create_procedure = webp_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
webp_init (Webp *webp)
{
}

static GList *
webp_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static GimpProcedure *
webp_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           webp_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("WebP image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads images in the WebP file format",
                                        "Loads images in the WebP file format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Nathan Osman, Ben Touchette",
                                      "(C) 2015-2016 Nathan Osman, "
                                      "(C) 2016 Ben Touchette",
                                      "2015,2016");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/webp");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "webp");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "8,string,WEBP");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           TRUE, webp_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("WebP image"));

      gimp_procedure_set_documentation (procedure,
                                        "Saves files in the WebP image format",
                                        "Saves files in the WebP image format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Nathan Osman, Ben Touchette",
                                      "(C) 2015-2016 Nathan Osman, "
                                      "(C) 2016 Ben Touchette",
                                      "2015,2016");

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("WebP"));
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/webp");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "webp");

      GIMP_PROC_ARG_INT (procedure, "preset",
                         _("Source _type"),
                         _("WebP encoder preset (Default=0, Picture=1, Photo=2, Drawing=3, "
                           "Icon=4, Text=5)"),
                         0, 5, WEBP_PRESET_DEFAULT,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "lossless",
                             _("L_ossless"),
                             _("Use lossless encoding"),
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "quality",
                            _("Image _quality"),
                            _("Quality of the image"),
                            0, 100, 90,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "alpha-quality",
                            _("Alpha q_uality"),
                            _("Quality of the image's alpha channel"),
                            0, 100, 100,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "use-sharp-yuv",
                             _("Use Sharp YU_V"),
                             /* TRANSLATORS: \xe2\x86\x92 is a Unicode
                              * "Rightward Arrow" in UTF-8 encoding.
                              */
                             _("Use sharper (but slower) RGB\xe2\x86\x92YUV conversion"),
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "animation-loop",
                             _("Loop _forever"),
                             _("Loop animation infinitely"),
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "minimize-size",
                             _("_Minimize output size (slower)"),
                             _("Minimize output file size"),
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "keyframe-distance",
                         _("Max distance between _key-frames"),
                         _("Maximum distance between keyframes"),
                         0, G_MAXINT, 50,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "default-delay",
                         _("_Default delay between frames"),
                         _("Default delay (in milliseconds) to use when timestamps"
                           " for frames are not available or forced."),
                         0, G_MAXINT, 200,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "force-delay",
                             _("Use default dela_y for all frames"),
                             _("Force default delay on all frames"),
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "animation",
                             _("Save a_nimation"),
                             _("Use layers for animation"),
                             FALSE,
                             G_PARAM_READWRITE);

      gimp_save_procedure_set_support_exif      (GIMP_SAVE_PROCEDURE (procedure), TRUE);
      gimp_save_procedure_set_support_iptc      (GIMP_SAVE_PROCEDURE (procedure), TRUE);
      gimp_save_procedure_set_support_xmp       (GIMP_SAVE_PROCEDURE (procedure), TRUE);
      gimp_save_procedure_set_support_profile   (GIMP_SAVE_PROCEDURE (procedure), TRUE);
      gimp_save_procedure_set_support_thumbnail (GIMP_SAVE_PROCEDURE (procedure), TRUE);
    }

  return procedure;
}

static GimpValueArray *
webp_load (GimpProcedure         *procedure,
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

  image = load_image (file, FALSE, flags, &error);

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
webp_save (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GimpImage            *image,
           gint                  n_drawables,
           GimpDrawable        **drawables,
           GFile                *file,
           GimpMetadata         *metadata,
           GimpProcedureConfig  *config,
           gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  gboolean           animation;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE ||
      run_mode == GIMP_RUN_WITH_LAST_VALS)
    gimp_ui_init (PLUG_IN_BINARY);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! save_dialog (image, procedure, G_OBJECT (config)))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
    }

  g_object_get (config,
                "animation", &animation,
                NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE ||
      run_mode == GIMP_RUN_WITH_LAST_VALS)
    {
      GimpExportCapabilities capabilities = (GIMP_EXPORT_CAN_HANDLE_RGB     |
                                             GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                             GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                             GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (animation)
        capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION;

      export = gimp_export_image (&image, &n_drawables, &drawables, "WebP",
                                  capabilities);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
    }

  if (animation)
    {
      if (! save_animation (file, image, n_drawables, drawables, G_OBJECT (config),
                            &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else
    {
      if (n_drawables != 1)
        {
          g_set_error (&error, G_FILE_ERROR, 0,
                       _("The WebP plug-in cannot export multiple layer, except in animation mode."));

          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CALLING_ERROR,
                                                   error);
        }

      if (! save_layer (file, image, drawables[0], G_OBJECT (config),
                        &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (status == GIMP_PDB_SUCCESS && metadata)
    {
      gboolean save_xmp;

      /* WebP doesn't support iptc natively and sets it via xmp */
      g_object_get (config,
                    "save-xmp", &save_xmp,
                    NULL);
      g_object_set (config,
                    "save-iptc", save_xmp,
                    NULL);

      gimp_metadata_set_bits_per_sample (metadata, 8);
    }

  if (export == GIMP_EXPORT_EXPORT)
    {
      gimp_image_delete (image);
      g_free (drawables);
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}
