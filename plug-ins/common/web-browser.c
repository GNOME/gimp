/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Web Browser Plug-in
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h> /* strlen, strstr */

#include <glib.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#define PLUG_IN_PROC "plug-in-web-browser"


static void     query            (void);
static void     run              (const gchar      *name,
                                  gint              nparams,
                                  const GimpParam  *param,
                                  gint             *nreturn_vals,
                                  GimpParam       **return_vals);
static gboolean browser_open_url (const gchar      *url);

#ifndef G_OS_WIN32
static gchar*   strreplace       (const gchar      *string,
                                  const gchar      *delimiter,
                                  const gchar      *replacement);
#endif

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,	 /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_STRING, "url", "URL to open" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
			  "Open an URL in the user specified web browser",
                          "Opens the given URL in the user specified web browser.",
			  "Henrik Brix Andersen <brix@gimp.org>",
			  "2003",
			  "2003/09/16",
                          NULL, NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint             nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status;

  run_mode = param[0].data.d_int32;
  status   = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  if (nparams == 1 &&
      param[0].data.d_string != NULL &&
      strlen (param[0].data.d_string))
    {
      if (! browser_open_url (param[0].data.d_string))
        status = GIMP_PDB_EXECUTION_ERROR;
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static gboolean
browser_open_url (const gchar *url)
{
#ifdef G_OS_WIN32

  return ((gint) ShellExecute (GetDesktopWindow(), "open", url, NULL, NULL, SW_SHOW) > 32);

#else

  GError    *error = NULL;
  gchar     *browser;
  gchar     *argument;
  gchar     *cmd;
  gchar    **argv;
  gboolean   retval;

  g_return_val_if_fail (url != NULL, FALSE);

  browser = gimp_gimprc_query ("web-browser");

  if (browser == NULL || ! strlen (browser))
    {
      g_message (_("Web browser not specified.\n"
                   "Please specify a web browser using the Preferences Dialog."));
      g_free (browser);
      return FALSE;
    }

  /* quote the url since it might contains special chars */
  argument = g_shell_quote (url);

  /* replace %s with URL */
  if (strstr (browser, "%s"))
    cmd = strreplace (browser, "%s", argument);
  else
    cmd = g_strconcat (browser, " ", argument, NULL);

  g_free (argument);

  /* parse the cmd line */
  if (! g_shell_parse_argv (cmd, NULL, &argv, &error))
    {
      g_message (_("Could not parse specified web browser command:\n%s"),
                 error->message);
      g_error_free (error);
      return FALSE;
    }

  retval = g_spawn_async (NULL, argv, NULL,
                          G_SPAWN_SEARCH_PATH,
                          NULL, NULL,
                          NULL, &error);

  if (! retval)
    {
      g_message (_("Could not execute specified web browser:\n%s"),
                 error->message);
      g_error_free (error);
    }

  g_free (browser);
  g_free (cmd);
  g_strfreev (argv);

  return retval;

#endif
}

#ifndef G_OS_WIN32

static gchar*
strreplace (const gchar *string,
            const gchar *delimiter,
            const gchar *replacement)
{
  gchar  *ret;
  gchar **tmp;

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (delimiter != NULL, NULL);
  g_return_val_if_fail (replacement != NULL, NULL);

  tmp = g_strsplit (string, delimiter, 0);
  ret = g_strjoinv (replacement, tmp);
  g_strfreev (tmp);

  return ret;
}

#endif /* !G_OS_WIN32 */
