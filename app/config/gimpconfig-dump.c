/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpConfig object property dumper. 
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#include "stdlib.h"
#include "string.h"

#include <glib-object.h>

#include "libgimpbase/gimplimits.h"
#include "libgimpbase/gimpbasetypes.h"

#include "config-types.h"

#include "gimpconfig.h"
#include "gimpconfig-serialize.h"
#include "gimprc.h"


int
main (int   argc,
      char *argv[])
{
  GObject *rc;

  g_type_init ();

  rc = g_object_new (GIMP_TYPE_RC, NULL);

  g_print ("# Dump of the GIMP default configuration\n\n");

  gimp_config_serialize_properties (rc, 1, 0);

  g_print ("\n");

  g_object_unref (rc);

  return EXIT_SUCCESS;
}


/* some dummy funcs so we can properly link this beast */

const gchar *
gimp_unit_get_identifier (GimpUnit unit)
{
  switch (unit)
    {
    case GIMP_UNIT_PIXEL:
      return "pixels";
    case GIMP_UNIT_INCH:
      return "inches";
    case GIMP_UNIT_MM:
      return "millimeters";
    case GIMP_UNIT_POINT:
      return "points";
    case GIMP_UNIT_PICA:
      return "picas";
    default:
      return NULL;
    }
}

gint
gimp_unit_get_number_of_units (void)
{
  return GIMP_UNIT_END;
}
