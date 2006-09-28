/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <glib.h>  /* lcms.h uses the "inline" keyword */

#ifdef HAVE_LCMS_LCMS_H
#include <lcms/lcms.h>
#else
#include <lcms.h>
#endif

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC_SET    "plug-in-icc-set"
#define PLUG_IN_PROC_APPLY  "plug-in-icc-apply"


static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);


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
    { GIMP_PDB_INT32,  "run-mode", "Interactive, non-interactive"     },
    { GIMP_PDB_IMAGE,  "image",    "Input image"                      },
    { GIMP_PDB_STRING, "profile",  "Filename of an ICC color profile" }
  };

  gimp_install_procedure (PLUG_IN_PROC_SET,
                          "Set a color profile on the image w/o applying it",
                          "This procedure sets an ICC color profile on an "
                          "image using the 'icc-profile' parasite. It does "
                          "not do any color conversion.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          "Set ICC Color Profile",
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_APPLY,
                          "Apply a color profile on the image",
                          "This procedure transform from the image's color "
                          "profile (or the default RGB profile if none is "
                          "set) to the given ICC color profile. The profile "
                          "is then set on the image using the 'icc-profile' "
                          "parasite.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          "Apply ICC Color Profile",
                          "RGB*",
                          GIMP_PLUGIN,
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
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  static GimpParam   values[1];

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  if (nparams != 3)
    {
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      return;
    }

  if (strcmp (name, PLUG_IN_PROC_SET) == 0)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }
  else if (strcmp (name, PLUG_IN_PROC_APPLY) == 0)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}
