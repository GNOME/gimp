/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The LIGMA Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@ligma.org>
 *                         Michael Natterer <mitch@ligma.org>
 *                         Henrik Brix Andersen <brix@ligma.org>
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

#include "libligma/ligma.h"

#include "ligmahelp.h"

#include "libligma/stdplugins-intl.h"


/*  defines  */

#define LIGMA_HELP_EXT_PROC        "extension-ligma-help"
#define LIGMA_HELP_TEMP_EXT_PROC   "extension-ligma-help-temp"


typedef struct _Help      Help;
typedef struct _HelpClass HelpClass;

struct _Help
{
  LigmaPlugIn parent_instance;
};

struct _HelpClass
{
  LigmaPlugInClass parent_class;
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
#define HELP (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HELP_TYPE, Help))

GType                   help_get_type          (void) G_GNUC_CONST;

static GList          * help_query_procedures  (LigmaPlugIn           *plug_in);
static LigmaProcedure  * help_create_procedure  (LigmaPlugIn           *plug_in,
                                                const gchar          *name);

static LigmaValueArray * help_run               (LigmaProcedure        *procedure,
                                                const LigmaValueArray *args,
                                                gpointer              run_data);
static LigmaValueArray * help_temp_run          (LigmaProcedure        *procedure,
                                                const LigmaValueArray *args,
                                                gpointer              run_data);

static void             help_temp_proc_install (LigmaPlugIn           *plug_in);
static void             help_load              (const gchar          *procedure,
                                                const gchar          *help_domain,
                                                const gchar          *help_locales,
                                                const gchar          *help_id);
static gboolean         help_load_idle         (gpointer              data);

static LigmaHelpProgress * help_load_progress_new (void);


static GMainLoop *main_loop = NULL;

G_DEFINE_TYPE (Help, help, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (HELP_TYPE)
DEFINE_STD_SET_I18N


static void
help_class_init (HelpClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = help_query_procedures;
  plug_in_class->create_procedure = help_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
help_init (Help *help)
{
}

static GList *
help_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (LIGMA_HELP_EXT_PROC));
}

static LigmaProcedure *
help_create_procedure (LigmaPlugIn  *plug_in,
                       const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LIGMA_HELP_EXT_PROC))
    {
      procedure = ligma_procedure_new (plug_in, name,
                                      LIGMA_PDB_PROC_TYPE_EXTENSION,
                                      help_run, NULL, NULL);

      ligma_procedure_set_attribution (procedure,
                                      "Sven Neumann <sven@ligma.org>, "
                                      "Michael Natterer <mitch@ligma.org>, "
                                      "Henrik Brix Andersen <brix@ligma.org>",
                                      "Sven Neumann, Michael Natterer & Henrik Brix Andersen",
                                      "1999-2008");

      LIGMA_PROC_ARG_STRV (procedure, "domain-names",
                          "Domain Names",
                          "Domain names",
                          G_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRV (procedure, "domain-uris",
                          "Domain URIs",
                          "Domain URIs",
                          G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
help_run (LigmaProcedure        *procedure,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaPDBStatusType status = LIGMA_PDB_SUCCESS;

  if (! ligma_help_init (LIGMA_VALUES_GET_STRV (args, 0),
                        LIGMA_VALUES_GET_STRV (args, 1)))
    {
      status = LIGMA_PDB_CALLING_ERROR;
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      LigmaPlugIn *plug_in = ligma_procedure_get_plug_in (procedure);

      main_loop = g_main_loop_new (NULL, FALSE);

      help_temp_proc_install (plug_in);

      ligma_procedure_extension_ready (procedure);
      ligma_plug_in_extension_enable (plug_in);

      g_main_loop_run (main_loop);

      g_main_loop_unref (main_loop);
      main_loop = NULL;

      ligma_plug_in_remove_temp_procedure (plug_in, LIGMA_HELP_TEMP_EXT_PROC);
    }

  return ligma_procedure_new_return_values (procedure, status, NULL);
}

static void
help_temp_proc_install (LigmaPlugIn *plug_in)
{
  LigmaProcedure *procedure;

  procedure = ligma_procedure_new (plug_in, LIGMA_HELP_TEMP_EXT_PROC,
                                  LIGMA_PDB_PROC_TYPE_TEMPORARY,
                                  help_temp_run, NULL, NULL);

  ligma_procedure_set_attribution (procedure,
                                  "Sven Neumann <sven@ligma.org>, "
                                  "Michael Natterer <mitch@ligma.org>"
                                  "Henrik Brix Andersen <brix@ligma.org",
                                  "Sven Neumann, Michael Natterer & "
                                  "Henrik Brix Andersen",
                                  "1999-2008");

  LIGMA_PROC_ARG_STRING (procedure, "help-proc",
                        "The procedure of the browser to use",
                        "The procedure of the browser to use",
                        NULL,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_STRING (procedure, "help-domain",
                        "Help domain to use",
                        "Help domain to use",
                        NULL,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_STRING (procedure, "help-locales",
                        "Language to use",
                        "Language to use",
                        NULL,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_STRING (procedure, "help-id",
                        "Help ID to open",
                        "Help ID to open",
                        NULL,
                        G_PARAM_READWRITE);

  ligma_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
}

static LigmaValueArray *
help_temp_run (LigmaProcedure        *procedure,
               const LigmaValueArray *args,
               gpointer              run_data)
{
  LigmaPDBStatusType  status       = LIGMA_PDB_SUCCESS;
  const gchar       *help_proc    = NULL;
  const gchar       *help_domain  = LIGMA_HELP_DEFAULT_DOMAIN;
  const gchar       *help_locales = NULL;
  const gchar       *help_id      = LIGMA_HELP_DEFAULT_ID;

  if (LIGMA_VALUES_GET_STRING (args, 0))
    help_proc = LIGMA_VALUES_GET_STRING (args, 0);

  if (LIGMA_VALUES_GET_STRING (args, 1))
    help_domain = LIGMA_VALUES_GET_STRING (args, 1);

  if (LIGMA_VALUES_GET_STRING (args, 2))
    help_locales = LIGMA_VALUES_GET_STRING (args, 2);

  if (LIGMA_VALUES_GET_STRING (args, 3))
    help_id = LIGMA_VALUES_GET_STRING (args, 3);

  if (! help_proc)
    status = LIGMA_PDB_CALLING_ERROR;

  if (status == LIGMA_PDB_SUCCESS)
    help_load (help_proc, help_domain, help_locales, help_id);

  return ligma_procedure_new_return_values (procedure, status, NULL);
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
  LigmaHelpDomain *domain;

  domain = ligma_help_lookup_domain (idle_help->help_domain);

  if (domain)
    {
      LigmaHelpProgress *progress = NULL;
      GList            *locales;
      gchar            *uri;
      gboolean          fatal_error;

      locales = ligma_help_parse_locales (idle_help->help_locales);

      if (! g_str_has_prefix (domain->help_uri, "file:"))
        progress = help_load_progress_new ();

      uri = ligma_help_domain_map (domain, locales, idle_help->help_id,
                                  progress, NULL, &fatal_error);

      if (progress)
        ligma_help_progress_free (progress);

      g_list_free_full (locales, (GDestroyNotify) g_free);

      if (uri)
        {
          LigmaValueArray *return_vals;

#ifdef LIGMA_HELP_DEBUG
          g_printerr ("help: calling '%s' for '%s'\n",
                      idle_help->procedure, uri);
#endif

          return_vals = ligma_pdb_run_procedure (ligma_get_pdb (),
                                                idle_help->procedure,
                                                G_TYPE_STRING, uri,
                                                G_TYPE_NONE);
          ligma_value_array_unref (return_vals);

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
  ligma_progress_init (message);
}

static void
help_load_progress_update (gdouble  value,
                           gpointer user_data)
{
  ligma_progress_update (value);
}

static void
help_load_progress_end (gpointer user_data)
{
  ligma_progress_end ();
}

static LigmaHelpProgress *
help_load_progress_new (void)
{
  static const LigmaHelpProgressVTable vtable =
  {
    help_load_progress_start,
    help_load_progress_end,
    help_load_progress_update
  };

  return ligma_help_progress_new (&vtable, NULL);
}
