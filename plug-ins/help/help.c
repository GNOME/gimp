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


typedef struct
{
  gchar *procedure;
  gchar *help_domain;
  gchar *help_locales;
  gchar *help_id;
} IdleHelp;


/*  forward declarations  */

static void     query             (void);
static void     run               (const gchar      *name,
                                   gint              nparams,
                                   const GimpParam  *param,
                                   gint             *nreturn_vals,
                                   GimpParam       **return_vals);

static void     temp_proc_install (void);
static void     temp_proc_run     (const gchar      *name,
                                   gint              nparams,
                                   const GimpParam  *param,
                                   gint             *nreturn_vals,
                                   GimpParam       **return_vals);

static void     load_help         (const gchar      *procedure,
                                   const gchar      *help_domain,
                                   const gchar      *help_locales,
                                   const gchar      *help_id);
static gboolean load_help_idle    (gpointer          data);

static GimpHelpProgress * load_help_progress_new (void);


/*  local variables  */

static GMainLoop *main_loop = NULL;

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,       "num-domain-names", "" },
    { GIMP_PDB_STRINGARRAY, "domain-names",     "" },
    { GIMP_PDB_INT32,       "num-domain-uris",  "" },
    { GIMP_PDB_STRINGARRAY, "domain-uris",      "" }
  };

  gimp_install_procedure (GIMP_HELP_EXT_PROC,
                          "", /* FIXME */
                          "", /* FIXME */
                          "Sven Neumann <sven@gimp.org>, "
                          "Michael Natterer <mitch@gimp.org>, "
                          "Henrik Brix Andersen <brix@gimp.org>",
                          "Sven Neumann, Michael Natterer & Henrik Brix Andersen",
                          "1999-2008",
                          NULL,
                          "",
                          GIMP_EXTENSION,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  /*  make sure all the arguments are there  */
  if (nparams == 4)
    {
      if (! gimp_help_init (param[0].data.d_int32,
                            param[1].data.d_stringarray,
                            param[2].data.d_int32,
                            param[3].data.d_stringarray))
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

      temp_proc_install ();

      gimp_extension_ack ();
      gimp_extension_enable ();

      g_main_loop_run (main_loop);

      g_main_loop_unref (main_loop);
      main_loop = NULL;

      gimp_uninstall_temp_proc (GIMP_HELP_TEMP_EXT_PROC);
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;
}

static void
temp_proc_install (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_STRING, "procedure",    "The procedure of the browser to use" },
    { GIMP_PDB_STRING, "help-domain",  "Help domain to use" },
    { GIMP_PDB_STRING, "help-locales", "Language to use"    },
    { GIMP_PDB_STRING, "help-id",      "Help ID to open"    }
  };

  gimp_install_temp_proc (GIMP_HELP_TEMP_EXT_PROC,
                          "DON'T USE THIS ONE",
                          "(Temporary procedure)",
                          "Sven Neumann <sven@gimp.org>, "
                          "Michael Natterer <mitch@gimp.org>"
                          "Henrik Brix Andersen <brix@gimp.org",
                          "Sven Neumann, Michael Natterer & Henrik Brix Andersen",
                          "1999-2008",
                          NULL,
                          "",
                          GIMP_TEMPORARY,
                          G_N_ELEMENTS (args), 0,
                          args, NULL,
                          temp_proc_run);
}

static void
temp_proc_run (const gchar      *name,
               gint              nparams,
               const GimpParam  *param,
               gint             *nreturn_vals,
               GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status       = GIMP_PDB_SUCCESS;
  const gchar       *procedure    = NULL;
  const gchar       *help_domain  = GIMP_HELP_DEFAULT_DOMAIN;
  const gchar       *help_locales = NULL;
  const gchar       *help_id      = GIMP_HELP_DEFAULT_ID;

  *nreturn_vals = 1;
  *return_vals  = values;

  /*  make sure all the arguments are there  */
  if (nparams == 4)
    {
      if (param[0].data.d_string && strlen (param[0].data.d_string))
        procedure = param[0].data.d_string;

      if (param[1].data.d_string && strlen (param[1].data.d_string))
        help_domain = param[1].data.d_string;

      if (param[2].data.d_string && strlen (param[2].data.d_string))
        help_locales = param[2].data.d_string;

      if (param[3].data.d_string && strlen (param[3].data.d_string))
        help_id = param[3].data.d_string;
    }

  if (! procedure)
    status = GIMP_PDB_CALLING_ERROR;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  if (status == GIMP_PDB_SUCCESS)
    load_help (procedure, help_domain, help_locales, help_id);
}

static void
load_help (const gchar *procedure,
           const gchar *help_domain,
           const gchar *help_locales,
           const gchar *help_id)
{
  IdleHelp *idle_help = g_slice_new (IdleHelp);

  idle_help->procedure    = g_strdup (procedure);
  idle_help->help_domain  = g_strdup (help_domain);
  idle_help->help_locales = g_strdup (help_locales);
  idle_help->help_id      = g_strdup (help_id);

  g_idle_add (load_help_idle, idle_help);
}

static gboolean
load_help_idle (gpointer data)
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
        progress = load_help_progress_new ();

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
load_help_progress_start (const gchar *message,
                          gboolean     cancelable,
                          gpointer     user_data)
{
  gimp_progress_init (message);
}

static void
load_help_progress_update (gdouble  value,
                           gpointer user_data)
{
  gimp_progress_update (value);
}

static void
load_help_progress_end (gpointer user_data)
{
  gimp_progress_end ();
}

static GimpHelpProgress *
load_help_progress_new (void)
{
  static const GimpHelpProgressVTable vtable =
  {
    load_help_progress_start,
    load_help_progress_end,
    load_help_progress_update
  };

  return gimp_help_progress_new (&vtable, NULL);
}
