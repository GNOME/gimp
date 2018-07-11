/* gimpparasite.c: Copyright 1998 Jay Cox <jaycox@gimp.org>
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

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-parasites.h"
#include "gimpparasitelist.h"


gboolean
gimp_parasite_validate (Gimp                *gimp,
                        const GimpParasite  *parasite,
                        GError             **error)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return TRUE;
}

void
gimp_parasite_attach (Gimp               *gimp,
                      const GimpParasite *parasite)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (parasite != NULL);

  gimp_parasite_list_add (gimp->parasites, parasite);
}

void
gimp_parasite_detach (Gimp        *gimp,
                      const gchar *name)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (name != NULL);

  gimp_parasite_list_remove (gimp->parasites, name);
}

const GimpParasite *
gimp_parasite_find (Gimp        *gimp,
                    const gchar *name)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return gimp_parasite_list_find (gimp->parasites, name);
}

static void
list_func (const gchar    *key,
           GimpParasite   *parasite,
           gchar        ***current)
{
  *(*current)++ = g_strdup (key);
}

gchar **
gimp_parasite_list (Gimp *gimp,
                    gint *count)
{
  gchar **list;
  gchar **current;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (count != NULL, NULL);

  *count = gimp_parasite_list_length (gimp->parasites);

  list = current = g_new (gchar *, *count);

  gimp_parasite_list_foreach (gimp->parasites, (GHFunc) list_func, &current);

  return list;
}


/*  FIXME: this doesn't belong here  */

void
gimp_parasite_shift_parent (GimpParasite *parasite)
{
  g_return_if_fail (parasite != NULL);

  parasite->flags = (parasite->flags >> 8);
}


/*  parasiterc functions  */

void
gimp_parasiterc_load (Gimp *gimp)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  file = gimp_directory_file ("parasiterc", NULL);

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  if (! gimp_config_deserialize_gfile (GIMP_CONFIG (gimp->parasites),
                                       file, NULL, &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);

      g_error_free (error);
    }

  g_object_unref (file);
}

void
gimp_parasiterc_save (Gimp *gimp)
{
  const gchar *header =
    "GIMP parasiterc\n"
    "\n"
    "This file will be entirely rewritten each time you exit.";
  const gchar *footer =
    "end of parasiterc";

  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_PARASITE_LIST (gimp->parasites));

  file = gimp_directory_file ("parasiterc", NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  if (! gimp_config_serialize_to_gfile (GIMP_CONFIG (gimp->parasites),
                                        file,
                                        header, footer, NULL,
                                        &error))
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}
