/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-error.h"

#include "gimp.h"
#include "gimp-templates.h"
#include "gimplist.h"
#include "gimptemplate.h"


/* functions to load and save the gimp templates files */

void
gimp_templates_load (Gimp *gimp)
{
  gchar  *filename;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_LIST (gimp->templates));

  filename = gimp_personal_rc_file ("templaterc");

  if (!gimp_config_deserialize_file (G_OBJECT (gimp->templates),
				     filename,
				     NULL,
				     &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        g_message (error->message);
      g_error_free (error);
    }

  gimp_list_reverse (GIMP_LIST (gimp->templates));

  g_free (filename);
}

void
gimp_templates_save (Gimp *gimp)
{
  const gchar *header =
    "GIMP templaterc\n"
    "\n"
    "This file will be entirely rewritten every time you quit the gimp.";
  const gchar *footer =
    "end of templaterc";

  gchar  *filename;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_LIST (gimp->templates));

  filename = gimp_personal_rc_file ("templaterc");

  if (! gimp_config_serialize_to_file (G_OBJECT (gimp->templates),
				       filename,
				       header, footer, NULL,
				       &error))
    {
      g_message (error->message);
      g_error_free (error);
    }

  g_free (filename);
}
