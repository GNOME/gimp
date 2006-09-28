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


#define PLUG_IN_PROC_SET        "plug-in-icc-set"
#define PLUG_IN_PROC_SET_RGB    "plug-in-icc-set-rgb"

#define PLUG_IN_PROC_APPLY      "plug-in-icc-apply"
#define PLUG_IN_PROC_APPLY_RGB  "plug-in-icc-apply-rgb"

#define PLUG_IN_PROC_INFO       "plug-in-icc-info"


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
  static const GimpParamDef base_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode", "Interactive, non-interactive"     },
    { GIMP_PDB_IMAGE,  "image",    "Input image"                      },
  };
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,  "run-mode", "Interactive, non-interactive"     },
    { GIMP_PDB_IMAGE,  "image",    "Input image"                      },
    { GIMP_PDB_STRING, "profile",  "Filename of an ICC color profile" }
  };
  static const GimpParamDef info_return_vals[] =
  {
    { GIMP_PDB_STRING, "product-name", "Name"         },
    { GIMP_PDB_STRING, "product-desc", "Description"  },
    { GIMP_PDB_STRING, "product-info", "Info"         },
    { GIMP_PDB_STRING, "manufacturer", "Manufacturer" },
    { GIMP_PDB_STRING, "model",        "Model"        },
    { GIMP_PDB_STRING, "copyright",    "Copyright"    }
  };

  gimp_install_procedure (PLUG_IN_PROC_SET,
                          "Set ICC color profile on the image",
                          "This procedure sets an ICC color profile on an "
                          "image using the 'icc-profile' parasite. It does "
                          "not do any color conversion.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          NULL,
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_SET_RGB,
                          "Set the default RGB color profile on the image",
                          "This procedure sets the user-configured RGB "
                          "profile on an image using the 'icc-profile' "
                          "parasite. If no RGB profile is, sRGB is assumed "
                          "and the parasite is unset. This procedure does "
                          "not do any color conversion.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          NULL,
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (base_args), 0,
                          base_args, NULL);

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
                          NULL,
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_APPLY_RGB,
                          "Apply default RGB color profile on the image",
                          "This procedure transform from the image's color "
                          "profile (or the default RGB profile if none is "
                          "set) to the configured default RGB color profile. "
                          "is then set on the image using the 'icc-profile' "
                          "parasite. If no RGB color profile is configured, "
                          "sRGB is assumed and the parasite is unset.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          NULL,
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (base_args), 0,
                          base_args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_INFO,
                          "Retrieve information about an image's color profile",
                          "This procedure returns information about the "
                          "color profile attached to an image. If no profile "
                          "is attached, sRGB is assumed.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          NULL,
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (base_args),
                          G_N_ELEMENTS (info_return_vals),
                          base_args, info_return_vals);
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
  else if (strcmp (name, PLUG_IN_PROC_SET_RGB) == 0)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }
  else if (strcmp (name, PLUG_IN_PROC_APPLY) == 0)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }
  else if (strcmp (name, PLUG_IN_PROC_APPLY_RGB) == 0)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }
  else if (strcmp (name, PLUG_IN_PROC_INFO) == 0)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}
