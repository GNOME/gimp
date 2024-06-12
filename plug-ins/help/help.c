/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

#include <string.h>  /*  strlen  */

#include <glib.h>

#include "libgimp/gimp.h"

#include "gimphelp.h"

#include "libgimp/stdplugins-intl.h"


/*  defines  */

#define GIMP_HELP_EXT_PROC        "extension-gimp-help"
#define GIMP_HELP_TEMP_EXT_PROC   "extension-gimp-help-temp"


typedef struct _Help      Help;
typedef struct _HelpClass HelpClass;

struct _Help
{
  GimpPlugIn parent_instance;
};

struct _HelpClass
{
  GimpPlugInClass parent_class;
};

typedef struct
{
  gchar *procedure;
  gchar *help_domain;
  gchar *help_locales;
  gchar *help_id;
} IdleHelp;


/*  forward declarations  */

#define HELP_TYPE  (help_get_type ())
#define HELP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HELP_TYPE, Help))

GType                   help_get_type          (void) G_GNUC_CONST;

static GList          * help_query_procedures  (GimpPlugIn           *plug_in);
static GimpProcedure  * help_create_procedure  (GimpPlugIn           *plug_in,
                                                const gchar          *name);

static GimpValueArray * help_run               (GimpProcedure        *procedure,
                                                GimpProcedureConfig  *config,
                                                gpointer              run_data);
static GimpValueArray * help_temp_run          (GimpProcedure        *procedure,
                                                GimpProcedureConfig  *config,
                                                gpointer              run_data);

static void             help_temp_proc_install (GimpPlugIn           *plug_in);
static void             help_load              (const gchar          *procedure,
                                                const gchar          *help_domain,
                                                const gchar          *help_locales,
                                                const gchar          *help_id);
static gboolean         help_load_idle         (gpointer              data);

static GimpHelpProgress * help_load_progress_new (void);


static GMainLoop *main_loop = NULL;

G_DEFINE_TYPE (Help, help, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (HELP_TYPE)
DEFINE_STD_SET_I18N


static void
help_class_init (HelpClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = help_query_procedures;
  plug_in_class->create_procedure = help_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
help_init (Help *help)
{
}

static GList *
help_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (GIMP_HELP_EXT_PROC));
}

static GimpProcedure *
help_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, GIMP_HELP_EXT_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_EXTENSION,
                                      help_run, NULL, NULL);

      gimp_procedure_set_attribution (procedure,
                                      "Sven Neumann <sven@gimp.org>, "
                                      "Michael Natterer <mitch@gimp.org>, "
                                      "Henrik Brix Andersen <brix@gimp.org>",
                                      "Sven Neumann, Michael Natterer & Henrik Brix Andersen",
                                      "1999-2008");

      gimp_procedure_add_string_array_argument (procedure, "domain-names",
                                                "Domain Names",
                                                "Domain names",
                                                G_PARAM_READWRITE);

      gimp_procedure_add_string_array_argument (procedure, "domain-uris",
                                                "Domain URIs",
                                                "Domain URIs",
                                                G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
help_run (GimpProcedure        *procedure,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpPDBStatusType   status       = GIMP_PDB_SUCCESS;
  gchar             **domain_names = NULL;
  gchar             **domain_uris  = NULL;

  g_object_get (config,
                "domain-names", &domain_names,
                "domain-uris",  &domain_uris,
                NULL);
  if (! gimp_help_init ((const gchar **) domain_names,
                        (const gchar **) domain_uris))
    status = GIMP_PDB_CALLING_ERROR;

  g_strfreev (domain_names);
  g_strfreev (domain_uris);

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpPlugIn *plug_in = gimp_procedure_get_plug_in (procedure);

      main_loop = g_main_loop_new (NULL, FALSE);

      help_temp_proc_install (plug_in);

      gimp_procedure_extension_ready (procedure);
      gimp_plug_in_extension_enable (plug_in);

      g_main_loop_run (main_loop);

      g_main_loop_unref (main_loop);
      main_loop = NULL;

      gimp_plug_in_remove_temp_procedure (plug_in, GIMP_HELP_TEMP_EXT_PROC);
    }

  return gimp_procedure_new_return_values (procedure, status, NULL);
}

static void
help_temp_proc_install (GimpPlugIn *plug_in)
{
  GimpProcedure *procedure;

  procedure = gimp_procedure_new (plug_in, GIMP_HELP_TEMP_EXT_PROC,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  help_temp_run, NULL, NULL);

  gimp_procedure_set_attribution (procedure,
                                  "Sven Neumann <sven@gimp.org>, "
                                  "Michael Natterer <mitch@gimp.org>"
                                  "Henrik Brix Andersen <brix@gimp.org",
                                  "Sven Neumann, Michael Natterer & "
                                  "Henrik Brix Andersen",
                                  "1999-2008");

  gimp_procedure_add_string_argument (procedure, "help-proc",
                                      "The procedure of the browser to use",
                                      "The procedure of the browser to use",
                                      NULL,
                                      G_PARAM_READWRITE);

  gimp_procedure_add_string_argument (procedure, "help-domain",
                                      "Help domain to use",
                                      "Help domain to use",
                                      NULL,
                                      G_PARAM_READWRITE);

  gimp_procedure_add_string_argument (procedure, "help-locales",
                                      "Language to use",
                                      "Language to use",
                                      NULL,
                                      G_PARAM_READWRITE);

  gimp_procedure_add_string_argument (procedure, "help-id",
                        "Help ID to open",
                        "Help ID to open",
                        NULL,
                        G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
}

static GimpValueArray *
help_temp_run (GimpProcedure        *procedure,
               GimpProcedureConfig  *config,
               gpointer              run_data)
{
  GimpPDBStatusType  status       = GIMP_PDB_SUCCESS;
  gchar             *help_proc    = NULL;
  gchar             *help_domain  = NULL;
  gchar             *help_locales = NULL;
  gchar             *help_id      = NULL;

  g_object_get (config,
                "help-proc",    &help_proc,
                "help-domain",  &help_domain,
                "help-locales", &help_locales,
                "help-id",      &help_id,
                NULL);

  if (help_domain == NULL)
    help_domain = g_strdup (GIMP_HELP_DEFAULT_DOMAIN);

  if (help_id == NULL)
    help_id = g_strdup (GIMP_HELP_DEFAULT_ID);

  if (! help_proc)
    status = GIMP_PDB_CALLING_ERROR;

  if (status == GIMP_PDB_SUCCESS)
    help_load (help_proc, help_domain, help_locales, help_id);

  g_free (help_proc);
  g_free (help_domain);
  g_free (help_locales);
  g_free (help_id);

  return gimp_procedure_new_return_values (procedure, status, NULL);
}

static void
help_load (const gchar *procedure,
           const gchar *help_domain,
           const gchar *help_locales,
           const gchar *help_id)
{
  IdleHelp *idle_help = g_slice_new (IdleHelp);

  idle_help->procedure    = g_strdup (procedure);
  idle_help->help_domain  = g_strdup (help_domain);
  idle_help->help_locales = g_strdup (help_locales);
  idle_help->help_id      = g_strdup (help_id);

  g_idle_add (help_load_idle, idle_help);
}

static gboolean
help_load_idle (gpointer data)
{
  IdleHelp       *idle_help = data;
  GimpHelpDomain *domain;

  domain = gimp_help_lookup_domain (idle_help->help_domain);

  if (domain)
    {
      GimpHelpProgress *progress = NULL;
      GList            *locales;
      gchar            *uri;
      gboolean          fatal_error;

      locales = gimp_help_parse_locales (idle_help->help_locales);

      if (! g_str_has_prefix (domain->help_uri, "file:"))
        progress = help_load_progress_new ();

      uri = gimp_help_domain_map (domain, locales, idle_help->help_id,
                                  progress, NULL, &fatal_error);

      if (progress)
        gimp_help_progress_free (progress);

      g_list_free_full (locales, (GDestroyNotify) g_free);

      if (uri)
        {
          GimpProcedure  *procedure;
          GimpValueArray *return_vals;

#ifdef GIMP_HELP_DEBUG
          g_printerr ("help: calling '%s' for '%s'\n",
                      idle_help->procedure, uri);
#endif

          procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (),
                                                   idle_help->procedure);
          return_vals = gimp_procedure_run (procedure, "url", uri, NULL);
          gimp_value_array_unref (return_vals);

          g_free (uri);
        }
      else if (fatal_error)
        {
          g_main_loop_quit (main_loop);
        }
    }

  g_free (idle_help->procedure);
  g_free (idle_help->help_domain);
  g_free (idle_help->help_locales);
  g_free (idle_help->help_id);

  g_slice_free (IdleHelp, idle_help);

  return FALSE;
}

static void
help_load_progress_start (const gchar *message,
                          gboolean     cancelable,
                          gpointer     user_data)
{
  gimp_progress_init (message);
}

static void
help_load_progress_update (gdouble  value,
                           gpointer user_data)
{
  gimp_progress_update (value);
}

static void
help_load_progress_end (gpointer user_data)
{
  gimp_progress_end ();
}

static GimpHelpProgress *
help_load_progress_new (void)
{
  static const GimpHelpProgressVTable vtable =
  {
    help_load_progress_start,
    help_load_progress_end,
    help_load_progress_update
  };

  return gimp_help_progress_new (&vtable, NULL);
}
