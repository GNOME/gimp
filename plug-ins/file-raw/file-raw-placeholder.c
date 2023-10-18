/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-raw-placeholder.c -- raw file format plug-in that does nothing
 *                           except warning that there is no raw plug-in
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2016 Tobias Ellinghaus <me@houz.org>
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

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"

#include "file-raw-formats.h"


typedef struct _Placeholder      Placeholder;
typedef struct _PlaceholderClass PlaceholderClass;

struct _Placeholder
{
  GimpPlugIn      parent_instance;
};

struct _PlaceholderClass
{
  GimpPlugInClass parent_class;
};


#define PLACEHOLDER_TYPE  (placeholder_get_type ())
#define PLACEHOLDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLACEHOLDER_TYPE, Placeholder))

GType                   placeholder_get_type         (void) G_GNUC_CONST;

static GList          * placeholder_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * placeholder_create_procedure (GimpPlugIn            *plug_in,
                                                      const gchar           *name);

static GimpValueArray * placeholder_load             (GimpProcedure         *procedure,
                                                      GimpRunMode            run_mode,
                                                      GFile                 *file,
                                                      GimpMetadata          *metadata,
                                                      GimpMetadataLoadFlags *flags,
                                                      GimpProcedureConfig   *config,
                                                      gpointer               run_data);


G_DEFINE_TYPE (Placeholder, placeholder, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PLACEHOLDER_TYPE)
DEFINE_STD_SET_I18N


static void
placeholder_class_init (PlaceholderClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = placeholder_query_procedures;
  plug_in_class->create_procedure = placeholder_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
placeholder_init (Placeholder *placeholder)
{
}

static GList *
placeholder_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;
  gint   i;

  for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];
      gchar            *load_proc;

      load_proc = g_strdup_printf (format->load_proc_format,
                                   "raw-placeholder");

      list = g_list_append (list, load_proc);
    }

  return list;
}

static GimpProcedure *
placeholder_create_procedure (GimpPlugIn  *plug_in,
                              const gchar *name)
{
  GimpProcedure *procedure = NULL;
  gint           i;

  for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];
      gchar            *load_proc;
      gchar            *load_blurb;
      gchar            *load_help;

      load_proc = g_strdup_printf (format->load_proc_format,
                                   "raw-placeholder");

      if (strcmp (name, load_proc))
        {
          g_free (load_proc);
          continue;
        }

      load_blurb = g_strdup_printf (format->load_blurb_format, "placeholder");
      load_help  = g_strdup_printf (format->load_help_format,  "placeholder");

      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           placeholder_load,
                                           (gpointer) format, NULL);

      gimp_procedure_set_documentation (procedure,
                                        load_blurb, load_help, name);
      gimp_procedure_set_attribution (procedure,
                                      "Tobias Ellinghaus",
                                      "Tobias Ellinghaus",
                                      "2016");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          format->mime_type);
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          format->extensions);
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      format->magic);

      gimp_load_procedure_set_handles_raw (GIMP_LOAD_PROCEDURE (procedure),
                                           TRUE);

      g_free (load_proc);
      g_free (load_blurb);
      g_free (load_help);

      break;
    }

  return procedure;
}

static GimpValueArray *
placeholder_load (GimpProcedure         *procedure,
                  GimpRunMode            run_mode,
                  GFile                 *file,
                  GimpMetadata          *metadata,
                  GimpMetadataLoadFlags *flags,
                  GimpProcedureConfig   *config,
                  gpointer               run_data)
{
  const FileFormat *format = run_data;
  GError           *error = NULL;

  g_set_error (&error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
               _("There is no RAW loader installed to open '%s' files.\n"
                 "\n"
                 "GIMP currently supports these RAW loaders:\n"
                 "- darktable (http://www.darktable.org/), at least 1.7\n"
                 "- RawTherapee (http://rawtherapee.com/), at least 5.2\n"
                 "\n"
                 "Please install one of them in order to "
                 "load RAW files."),
               gettext (format->file_type));

  return gimp_procedure_new_return_values (procedure,
                                           GIMP_PDB_EXECUTION_ERROR,
                                           error);
}
