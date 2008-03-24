/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help Browser
 * Copyright (C) 1999-2004 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

#include <string.h>  /*  strlen, strcmp  */

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "plug-ins/help/gimphelp.h"

#include "dialog.h"

#include "libgimp/stdplugins-intl.h"


/*  defines  */

#define GIMP_HELP_BROWSER_EXT_PROC       "extension-gimp-help-browser"
#define GIMP_HELP_BROWSER_TEMP_EXT_PROC  "extension-gimp-help-browser-temp"


/*  forward declarations  */

static void   query             (void);
static void   run               (const gchar      *name,
                                 gint              nparams,
                                 const GimpParam  *param,
                                 gint             *nreturn_vals,
                                 GimpParam       **return_vals);

static void   temp_proc_install (void);
static void   temp_proc_run     (const gchar      *name,
                                 gint              nparams,
                                 const GimpParam  *param,
                                 gint             *nreturn_vals,
                                 GimpParam       **return_vals);


/*  local variables  */

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
    { GIMP_PDB_INT32,       "run-mode", "Interactive" },
    { GIMP_PDB_INT32,       "num-domain-names", ""    },
    { GIMP_PDB_STRINGARRAY, "domain-names",     ""    },
    { GIMP_PDB_INT32,       "num-domain-uris",  ""    },
    { GIMP_PDB_STRINGARRAY, "domain-uris",      ""    }
  };

  gimp_install_procedure (GIMP_HELP_BROWSER_EXT_PROC,
                          "Browse the GIMP help pages",
                          "A small and simple HTML browser optimized for "
			  "browsing the GIMP help pages.",
                          "Sven Neumann <sven@gimp.org>, "
			  "Michael Natterer <mitch@gimp.org>"
                          "Henrik Brix Andersen <brix@gimp.org>",
			  "Sven Neumann, Michael Natterer & Henrik Brix Andersen",
                          "1999-2004",
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
  GimpRunMode        run_mode = param[0].data.d_int32;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  static GimpParam   values[1];

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  INIT_I18N ();

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_NONINTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      /*  Make sure all the arguments are there!  */
      if (nparams >= 1)
        {
          if (nparams == 5)
            {
              if (! gimp_help_init (param[1].data.d_int32,
                                    param[2].data.d_stringarray,
                                    param[3].data.d_int32,
                                    param[4].data.d_stringarray))
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }
            }

          if (status == GIMP_PDB_SUCCESS)
            {
              browser_dialog_open ();

              temp_proc_install ();

              gimp_extension_ack ();
              gimp_extension_enable ();

              gtk_main ();
            }
        }
      else
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      break;

    default:
      status = GIMP_PDB_CALLING_ERROR;
      break;
    }

  values[0].data.d_status = status;
}

static void
temp_proc_install (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_STRING, "help-domain",  "Help domain to use" },
    { GIMP_PDB_STRING, "help-locales", "Language to use"    },
    { GIMP_PDB_STRING, "help-id",      "Help ID to open"    }
  };

  gimp_install_temp_proc (GIMP_HELP_BROWSER_TEMP_EXT_PROC,
			  "DON'T USE THIS ONE",
			  "(Temporary procedure)",
			  "Sven Neumann <sven@gimp.org>, "
			  "Michael Natterer <mitch@gimp.org>"
                          "Henrik Brix Andersen <brix@gimp.org>",
			  "Sven Neumann, Michael Natterer & Henrik Brix Andersen",
			  "1999-2004",
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
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  /*  make sure all the arguments are there  */
  if (nparams == 3)
    {
      GimpHelpDomain *domain;
      const gchar    *help_domain  = GIMP_HELP_DEFAULT_DOMAIN;
      const gchar    *help_locales = NULL;
      const gchar    *help_id      = GIMP_HELP_DEFAULT_ID;

      if (param[0].data.d_string && strlen (param[0].data.d_string))
        help_domain = param[0].data.d_string;

      if (param[1].data.d_string && strlen (param[1].data.d_string))
        help_locales = param[1].data.d_string;

      if (param[2].data.d_string && strlen (param[2].data.d_string))
        help_id = param[2].data.d_string;

      domain = gimp_help_lookup_domain (help_domain);

      if (domain)
        {
          GList          *locales = gimp_help_parse_locales (help_locales);
          GimpHelpLocale *locale;
          gchar          *full_uri;
          gboolean        fatal_error;

          full_uri = gimp_help_domain_map (domain, locales, help_id,
                                           &locale, &fatal_error);

          if (full_uri)
            {
              browser_dialog_load (full_uri, TRUE);
              browser_dialog_make_index (domain, locale);
              browser_dialog_goto_index (full_uri);

              g_free (full_uri);
            }
          else if (fatal_error)
            {
              gtk_main_quit ();
            }

          g_list_foreach (locales, (GFunc) g_free, NULL);
          g_list_free (locales);
        }
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}
