/* The GIMP -- an image manipulation program
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

#include "dialog.h"

#include "libgimp/stdplugins-intl.h"


/*  defines  */

#define GIMP_HELP_BROWSER_EXT_NAME       "extension_gimp_help_browser"
#define GIMP_HELP_BROWSER_TEMP_EXT_NAME  "extension_gimp_help_browser_temp"


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
    { GIMP_PDB_INT32, "run_mode", "Interactive" },
  };

  gimp_install_procedure (GIMP_HELP_BROWSER_EXT_NAME,
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
      if (nparams == 1)
        {
          browser_dialog_open ();

          temp_proc_install ();

          /*  we have installed our temp_proc and are ready to run  */
          gimp_extension_ack ();

          /*  enable asynchronous processing of temp_proc_run requests  */
          gimp_extension_enable ();

          gtk_main ();
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
  static GimpParamDef args[] =
  {
    { GIMP_PDB_STRING, "uri", "Full uri of the file to open" }
  };

  gimp_install_temp_proc (GIMP_HELP_BROWSER_TEMP_EXT_NAME,
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
  if (nparams == 1)
    {
      if (param[0].data.d_string && strlen (param[0].data.d_string))
        {
          browser_dialog_load (param[0].data.d_string, TRUE);
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

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}
