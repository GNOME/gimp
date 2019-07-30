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
#define HELP (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HELP_TYPE, Help))

GType                   help_get_type          (void) G_GNUC_CONST;

static gchar         ** help_query_procedures  (GimpPlugIn           *plug_in,
                                                gint                 *n_procedures);
static GimpProcedure  * help_create_procedure  (GimpPlugIn           *plug_in,
                                                const gchar          *name);

static GimpValueArray * help_run               (GimpProcedure        *procedure,
                                                const GimpValueArray *args);
static GimpValueArray * help_temp_run          (GimpProcedure        *procedure,
                                                const GimpValueArray *args);

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


static void
help_class_init (HelpClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = help_query_procedures;
  plug_in_class->create_procedure = help_create_procedure;
}

static void
help_init (Help *help)
{
}

static gchar **
help_query_procedures (GimpPlugIn *plug_in,
                       gint       *n_procedures)
{
  gchar **procedures = g_new0 (gchar *, 2);

  procedures[0] = g_strdup (GIMP_HELP_EXT_PROC);

  *n_procedures = 1;

  return procedures;
}

static GimpProcedure *
help_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, GIMP_HELP_EXT_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name, GIMP_EXTENSION, help_run);

      gimp_procedure_set_strings (procedure,
                                  NULL,
                                  "", /* FIXME */
                                  "", /* FIXME */
                                  NULL,
                                  "Sven Neumann <sven@gimp.org>, "
                                  "Michael Natterer <mitch@gimp.org>, "
                                  "Henrik Brix Andersen <brix@gimp.org>",
                                  "Sven Neumann, Michael Natterer & Henrik Brix Andersen",
                                  "1999-2008",
                                  "");

      gimp_procedure_add_argument (procedure,
                                   gimp_param_spec_int32 ("num-domain-names",
                                                          "Num Domain Names",
                                                          "Num domain names",
                                                          0, G_MAXINT, 0,
                                                          G_PARAM_READWRITE));
      gimp_procedure_add_argument (procedure,
                                   gimp_param_spec_string_array ("domain-names",
                                                                 "Domain Names",
                                                                 "Domain names",
                                                                 G_PARAM_READWRITE));
      gimp_procedure_add_argument (procedure,
                                   gimp_param_spec_int32 ("num-domain-uris",
                                                          "Num Domain URIs",
                                                          "Num domain URIs",
                                                          0, G_MAXINT, 0,
                                                          G_PARAM_READWRITE));
      gimp_procedure_add_argument (procedure,
                                   gimp_param_spec_string_array ("domain-uris",
                                                                 "Domain URIs",
                                                                 "Domain URIs",
                                                                 G_PARAM_READWRITE));
    }

  return procedure;
}

static GimpValueArray *
help_run (GimpProcedure        *procedure,
          const GimpValueArray *args)
{
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  /*  make sure all the arguments are there  */
  if (gimp_value_array_length (args) == 4)
    {
      if (! gimp_help_init
              (g_value_get_int             (gimp_value_array_index (args, 0)),
               gimp_value_get_string_array (gimp_value_array_index (args, 1)),
               g_value_get_int             (gimp_value_array_index (args, 2)),
               gimp_value_get_string_array (gimp_value_array_index (args, 3))))
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
    }
  else
    {
      g_printerr ("help: wrong number of arguments in procedure call.\n");

      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      main_loop = g_main_loop_new (NULL, FALSE);

      help_temp_proc_install (gimp_procedure_get_plug_in (procedure));

      gimp_extension_ack ();
      gimp_extension_enable ();

      g_main_loop_run (main_loop);

      g_main_loop_unref (main_loop);
      main_loop = NULL;

      gimp_uninstall_temp_proc (GIMP_HELP_TEMP_EXT_PROC);
    }

  return gimp_procedure_new_return_values (procedure, status, NULL);
}

static void
help_temp_proc_install (GimpPlugIn *plug_in)
{
  GimpProcedure *procedure;

  procedure = gimp_procedure_new (plug_in, GIMP_HELP_TEMP_EXT_PROC,
                                  GIMP_TEMPORARY, help_temp_run);

  gimp_procedure_set_strings (procedure,
                              NULL,
                              "DON'T USE THIS ONE",
                              "(Temporary procedure)",
                              NULL,
                              "Sven Neumann <sven@gimp.org>, "
                              "Michael Natterer <mitch@gimp.org>"
                              "Henrik Brix Andersen <brix@gimp.org",
                              "Sven Neumann, Michael Natterer & Henrik Brix Andersen",
                              "1999-2008",
                              "");

  gimp_procedure_add_argument (procedure,
                               g_param_spec_string ("help-proc",
                                                    "The procedure of the browser to use",
                                                    "The procedure of the browser to use",
                                                    NULL,
                                                    G_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               g_param_spec_string ("help-domain",
                                                    "Help domain to use",
                                                    "Help domain to use",
                                                    NULL,
                                                    G_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               g_param_spec_string ("help-locales",
                                                    "Language to use",
                                                    "Language to use",
                                                    NULL,
                                                    G_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               g_param_spec_string ("help-id",
                                                    "Help ID to open",
                                                    "Help ID to open",
                                                    NULL,
                                                    G_PARAM_READWRITE));

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
}

static GimpValueArray *
help_temp_run (GimpProcedure        *procedure,
               const GimpValueArray *args)
{
  GimpPDBStatusType  status       = GIMP_PDB_SUCCESS;
  const gchar       *help_proc    = NULL;
  const gchar       *help_domain  = GIMP_HELP_DEFAULT_DOMAIN;
  const gchar       *help_locales = NULL;
  const gchar       *help_id      = GIMP_HELP_DEFAULT_ID;

  /*  make sure all the arguments are there  */
  if (gimp_value_array_length (args) == 4)
    {
      if (g_value_get_string (gimp_value_array_index (args, 0)))
        help_proc = g_value_get_string (gimp_value_array_index (args, 0));

      if (g_value_get_string (gimp_value_array_index (args, 1)))
        help_domain = g_value_get_string (gimp_value_array_index (args, 1));

      if (g_value_get_string (gimp_value_array_index (args, 2)))
        help_locales = g_value_get_string (gimp_value_array_index (args, 2));

      if (g_value_get_string (gimp_value_array_index (args, 3)))
        help_id = g_value_get_string (gimp_value_array_index (args, 3));
    }

  if (! help_proc)
    status = GIMP_PDB_CALLING_ERROR;

  if (status == GIMP_PDB_SUCCESS)
    help_load (help_proc, help_domain, help_locales, help_id);

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
          GimpParam *return_vals;
          gint       n_return_vals;

#ifdef GIMP_HELP_DEBUG
          g_printerr ("help: calling '%s' for '%s'\n",
                      idle_help->procedure, uri);
#endif

          return_vals = gimp_run_procedure (idle_help->procedure,
                                            &n_return_vals,
                                            GIMP_PDB_STRING, uri,
                                            GIMP_PDB_END);

          gimp_destroy_params (return_vals, n_return_vals);

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
