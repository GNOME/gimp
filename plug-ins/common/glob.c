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

/* Original Author: George Hartz <ghartz@usa.net>
 *
 * Ported to GIMP 2.0 by Markus Triska and Sven Neumann.
 */

#include "config.h"

#include <string.h>
#include <glob.h>

#include "libgimp/gimp.h"


#define PROCEDURE_NAME "file_glob"

static void   query  (void);
static void   run    (const gchar      *name,
                      gint              nparams,
                      const GimpParam  *param,
                      gint             *nreturn_vals,
                      GimpParam       **return_vals);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run,
};

MAIN ()

static void
query ()
{
  static GimpParamDef glob_args[] =
  {
    { GIMP_PDB_STRING,      "pattern",   "The glob pattern" }
  };

  static GimpParamDef glob_return_vals[] =
  {
    { GIMP_PDB_INT32,       "num_files", "The number of returned names" },
    { GIMP_PDB_STRINGARRAY, "files",     "The list of matching names"   }
  };


  gimp_install_procedure (PROCEDURE_NAME,
                          "Returns a list of matching filenames",
                          "This can be useful in scripts and other plugins "
                          "(e.g., batch-conversion). See the glob(7) manpage "
                          "for more info.",
                          "George Hartz <ghartz@usa.net>",
                          "George Hartz",
                          "1998",
                          NULL,
			  NULL,
                          GIMP_PLUGIN,
			  G_N_ELEMENTS (glob_args),
			  G_N_ELEMENTS (glob_return_vals),
			  glob_args,
			  glob_return_vals);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam values[3];

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;

  if (strcmp (name, PROCEDURE_NAME) == 0)
    {
      glob_t   files;
      gchar  **out;
      gint     ret;
      gint     i;

      ret = glob (param[0].data.d_string, GLOB_NOESCAPE, 0, &files);

      if (ret == GLOB_ABORTED || ret == GLOB_NOSPACE)
        {
          values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
          return;
	}

      out = g_new (gchar *, files.gl_pathc);

      for (i = 0; i < files.gl_pathc; i++)
        out[i] = g_strdup (files.gl_pathv[i]);

      globfree (&files);

      *nreturn_vals = 3;

      values[0].type               = GIMP_PDB_STATUS;
      values[0].data.d_status      = GIMP_PDB_SUCCESS;

      values[1].type               = GIMP_PDB_INT32;
      values[1].data.d_int32       = i;

      values[2].type               = GIMP_PDB_STRINGARRAY;
      values[2].data.d_stringarray = out;
    }
}
