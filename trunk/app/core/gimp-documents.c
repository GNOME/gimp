/* GIMP - The GNU Image Manipulation Program
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
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimp-documents.h"
#include "gimpdocumentlist.h"


/* functions to load and save the gimp documents files */

void
gimp_documents_load (Gimp *gimp)
{
  gchar  *filename;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_DOCUMENT_LIST (gimp->documents));

  filename = gimp_personal_rc_file ("documents");

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

  if (! gimp_config_deserialize_file (GIMP_CONFIG (gimp->documents),
                                      filename,
                                      GINT_TO_POINTER (gimp->config->thumbnail_size),
                                      &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR, "%s", error->message);
      g_error_free (error);
    }

  g_free (filename);
}

void
gimp_documents_save (Gimp *gimp)
{
  const gchar *header =
    "GIMP documents\n"
    "\n"
    "This file will be entirely rewritten each time you exit.";
  const gchar *footer =
    "end of documents";

  gchar  *filename;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_DOCUMENT_LIST (gimp->documents));

  filename = gimp_personal_rc_file ("documents");

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

  if (! gimp_config_serialize_to_file (GIMP_CONFIG (gimp->documents),
                                       filename,
                                       header, footer, NULL,
                                       &error))
    {
      gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR, "%s", error->message);
      g_error_free (error);
    }

  g_free (filename);
}
