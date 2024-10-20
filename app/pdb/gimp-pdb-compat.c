/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "pdb-types.h"

#include "gimppdb.h"
#include "gimp-pdb-compat.h"


/*  public functions  */

void
gimp_pdb_compat_procs_register (GimpPDB           *pdb,
                                GimpPDBCompatMode  compat_mode)
{
  static const struct
  {
    const gchar *old_name;
    const gchar *new_name;
  }
  compat_procs[] =
  {
    /*  deprecations since 3.0  */
    /* Intended to be empty, for major release. */

    /* Template:
    { "gimp-edit-paste-as-new",             "gimp-edit-paste-as-new-image"    },
    */
  };

  g_return_if_fail (GIMP_IS_PDB (pdb));

  if (compat_mode != GIMP_PDB_COMPAT_OFF)
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (compat_procs); i++)
        gimp_pdb_register_compat_proc_name (pdb,
                                            compat_procs[i].old_name,
                                            compat_procs[i].new_name);
    }
}
