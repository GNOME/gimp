/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help Browser
 * Copyright (C) 1999-2002 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *
 * Some code & ideas stolen from the GNOME help browser.
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

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "dialog.h"
#include "domain.h"

#include "libgimp/stdplugins-intl.h"


/*  defines  */

#define GIMP_HELP_EXT_NAME       "extension_gimp_help_browser"
#define GIMP_HELP_TEMP_EXT_NAME  "extension_gimp_help_browser_temp"

#define GIMP_HELP_PREFIX         "help"
#define GIMP_HELP_DEFAULT_DOMAIN "http://www.gimp.org/help"


typedef struct
{
  gchar *help_domain;
  gchar *help_locale;
  gchar *help_id;
} IdleHelp;


/*  forward declarations  */

static void         query             (void);
static void         run               (const gchar      *name,
                                       gint              nparams,
                                       const GimpParam  *param,
                                       gint             *nreturn_vals,
                                       GimpParam       **return_vals);

static void         temp_proc_install (void);
static void         temp_proc_run     (const gchar      *name,
                                       gint              nparams,
                                       const GimpParam  *param,
                                       gint             *nreturn_vals,
                                       GimpParam       **return_vals);

static void         load_help         (const gchar      *help_domain,
                                       const gchar      *help_locale,
                                       const gchar      *help_id);
static gboolean     load_help_idle    (gpointer          data);


/*  local variables  */

GimpPlugInInfo PLUG_IN_INFO =
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
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,       "run_mode",         "Interactive" },
    { GIMP_PDB_INT32,       "num_domain_names", ""            },
    { GIMP_PDB_STRINGARRAY, "domain_names",     ""            },
    { GIMP_PDB_INT32,       "num_domain_uris",  ""            },
    { GIMP_PDB_STRINGARRAY, "domain_uris",      ""            }
  };

  gimp_install_procedure (GIMP_HELP_EXT_NAME,
                          "Browse the GIMP help pages",
                          "A small and simple HTML browser optimized for "
			  "browsing the GIMP help pages.",
                          "Sven Neumann <sven@gimp.org>, "
			  "Michael Natterer <mitch@gimp.org>",
			  "Sven Neumann & Michael Natterer",
                          "1999-2003",
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
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  INIT_I18N ();

  if (strcmp (name, GIMP_HELP_EXT_NAME) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_NONINTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
          {
            const gchar *default_env_domain_uri;
            gchar       *default_domain_uri;

            /*  set default values  */
            default_env_domain_uri = g_getenv ("GIMP2_HELP_URI");

            if (default_env_domain_uri)
              {
                default_domain_uri = g_strdup (default_env_domain_uri);
              }
            else
              {
                gchar *help_root;

                help_root = g_build_filename (gimp_data_directory (),
                                              GIMP_HELP_PREFIX, NULL);

                default_domain_uri = g_filename_to_uri (help_root, NULL, NULL);

                g_free (help_root);
              }

            /*  Make sure all the arguments are there!  */
            if (nparams == 5)
              {
                if (param[1].data.d_int32 == param[3].data.d_int32)
                  {
                    gint i;

                    domain_register (GIMP_HELP_DEFAULT_DOMAIN,
                                     default_domain_uri);

                    for (i = 0; i < param[1].data.d_int32; i++)
                      domain_register (param[2].data.d_stringarray[i],
                                       param[4].data.d_stringarray[i]);
                  }
                else
                  {
                    status = GIMP_PDB_CALLING_ERROR;
                  }
              }
            else
              {
                status = GIMP_PDB_CALLING_ERROR;
              }

            g_free (default_domain_uri);
          }
          break;

        default:
	  status = GIMP_PDB_CALLING_ERROR;
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          browser_dialog_open ();

          temp_proc_install ();

          /*  we have installed our temp_proc and are ready to run  */
          gimp_extension_ack ();

          /*  enable asynchronous processing of temp_proc_run requests  */
          gimp_extension_enable ();

          gtk_main ();
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static void
temp_proc_install (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_STRING, "help_domain", "Help domain to use" },
    { GIMP_PDB_STRING, "help_locale", "Language to use"    },
    { GIMP_PDB_STRING, "help_id",     "Help ID to open"    }
  };

  gimp_install_temp_proc (GIMP_HELP_TEMP_EXT_NAME,
			  "DON'T USE THIS ONE",
			  "(Temporary procedure)",
			  "Sven Neumann <sven@gimp.org>, "
			  "Michael Natterer <mitch@gimp.org>",
			  "Sven Neumann & Michael Natterer",
			  "1999-2003",
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
  GimpPDBStatusType  status      = GIMP_PDB_SUCCESS;
  const gchar       *help_domain = GIMP_HELP_DEFAULT_DOMAIN;
  const gchar       *help_locale = "C";
  const gchar       *help_id     = "gimp-main";

  /*  Make sure all the arguments are there!  */
  if (nparams == 3)
    {
      if (param[0].data.d_string && strlen (param[0].data.d_string))
        help_domain = param[0].data.d_string;

      if (param[1].data.d_string && strlen (param[1].data.d_string))
        help_locale = param[1].data.d_string;

      if (param[2].data.d_string && strlen (param[2].data.d_string))
        help_id = param[2].data.d_string;
    }

  load_help (help_domain, help_locale, help_id);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
load_help (const gchar *help_domain,
           const gchar *help_locale,
           const gchar *help_id)
{
  IdleHelp *idle_help;

  idle_help = g_new0 (IdleHelp, 1);

  idle_help->help_domain = g_strdup (help_domain);
  idle_help->help_locale = g_strdup (help_locale);
  idle_help->help_id     = g_strdup (help_id);

  g_idle_add (load_help_idle, idle_help);  /* frees idle_help */
}

static gboolean
load_help_idle (gpointer data)
{
  IdleHelp   *idle_help;
  HelpDomain *domain;

  idle_help = (IdleHelp *) data;

  domain = domain_lookup (idle_help->help_domain);

  if (domain)
    {
      gchar *full_uri;

      full_uri = domain_map (domain,
                             idle_help->help_locale,
                             idle_help->help_id);

      if (full_uri)
        {
          browser_dialog_load (full_uri, TRUE);

          g_free (full_uri);
        }
    }

  g_free (idle_help->help_domain);
  g_free (idle_help->help_locale);
  g_free (idle_help->help_id);
  g_free (idle_help);

  return FALSE;
}
