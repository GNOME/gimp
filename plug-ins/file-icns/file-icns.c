/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * file-icns.c
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

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

/* #define ICNS_DBG */

#include "file-icns.h"
#include "file-icns-load.h"
#include "file-icns-export.h"

#include "libgimp/stdplugins-intl.h"

#define LOAD_PROC           "file-icns-load"
#define LOAD_THUMB_PROC     "file-icns-load-thumb"
#define EXPORT_PROC         "file-icns-export"


typedef struct _Icns      Icns;
typedef struct _IcnsClass IcnsClass;

struct _Icns
{
  GimpPlugIn      parent_instance;
};

struct _IcnsClass
{
  GimpPlugInClass parent_class;
};


#define ICNS_TYPE (icns_get_type ())
#define ICNS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ICNS_TYPE, Icns))

GType                   icns_get_type         (void) G_GNUC_CONST;

static GList          * icns_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * icns_create_procedure (GimpPlugIn            *plug_in,
                                               const gchar           *name);

static GimpValueArray * icns_load             (GimpProcedure         *procedure,
                                               GimpRunMode            run_mode,
                                               GFile                 *file,
                                               GimpMetadata          *metadata,
                                               GimpMetadataLoadFlags *flags,
                                               GimpProcedureConfig   *config,
                                               gpointer               run_data);
static GimpValueArray * icns_load_thumb       (GimpProcedure         *procedure,
                                               GFile                 *file,
                                               gint                   size,
                                               GimpProcedureConfig   *config,
                                               gpointer               run_data);
static GimpValueArray * icns_export           (GimpProcedure         *procedure,
                                               GimpRunMode            run_mode,
                                               GimpImage             *image,
                                               GFile                 *file,
                                               GimpExportOptions     *options,
                                               GimpMetadata          *metadata,
                                               GimpProcedureConfig   *config,
                                               gpointer               run_data);


G_DEFINE_TYPE (Icns, icns, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (ICNS_TYPE)
DEFINE_STD_SET_I18N

static void
icns_class_init (IcnsClass *klass)
{
  GimpPlugInClass *plug_in_class  = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = icns_query_procedures;
  plug_in_class->create_procedure = icns_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
icns_init (Icns *icns)
{
}

static GList *
icns_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
icns_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           icns_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("Icns"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads files in Apple Icon Image format"),
                                        _("Loads Apple Icon Image files."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Brion Vibber <brion@pobox.com>",
                                      "Brion Vibber <brion@pobox.com>",
                                      "2004");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-icns");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "icns");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,\x69\x63\x6E\x73");

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                icns_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Loads a preview from an Apple Icon Image file",
                                        "",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Brion Vibber <brion@pobox.com>",
                                      "Brion Vibber <brion@pobox.com>",
                                      "2004");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, icns_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("Apple Icon Image"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_BRUSH);

      gimp_procedure_set_documentation (procedure,
                                        "Saves files in Apple Icon Image file format",
                                        "Saves files in Apple Icon Image file format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Brion Vibber <brion@pobox.com>",
                                      "Brion Vibber <brion@pobox.com>",
                                      "2004");

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           "Apple Icon Image");
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-icns");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "icns");

      gimp_export_procedure_set_support_profile (GIMP_EXPORT_PROCEDURE (procedure), TRUE);
    }

  return procedure;
}

static GimpValueArray *
icns_load (GimpProcedure         *procedure,
           GimpRunMode            run_mode,
           GFile                 *file,
           GimpMetadata          *metadata,
           GimpMetadataLoadFlags *flags,
           GimpProcedureConfig   *config,
           gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error       = NULL;

  gegl_init (NULL, NULL);

  image = icns_load_image (file, NULL, &error);

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
icns_load_thumb (GimpProcedure       *procedure,
                 GFile               *file,
                 gint                 size,
                 GimpProcedureConfig *config,
                 gpointer             run_data)
{
  GimpValueArray *return_vals;
  gint            width;
  gint            height;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  width  = size;
  height = size;

  image = icns_load_thumbnail_image (file,
                                     &width, &height, 0, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);
  GIMP_VALUES_SET_INT   (return_vals, 2, width);
  GIMP_VALUES_SET_INT   (return_vals, 3, height);

  gimp_value_array_truncate (return_vals, 4);

  return return_vals;
}

static GimpValueArray *
icns_export (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GimpImage            *image,
             GFile                *file,
             GimpExportOptions    *options,
             GimpMetadata         *metadata,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  GimpPDBStatusType  status;
  GError            *error = NULL;

  gegl_init (NULL, NULL);

  status = icns_save_image (file, image, procedure, config, run_mode, &error);

  return gimp_procedure_new_return_values (procedure, status, error);
}

/* Buffer should point to *at least 5 byte buffer*! */
void
fourcc_get_string (gchar *fourcc,
                   gchar *buf)
{
  buf = fourcc;
  buf[4] = 0;
}
