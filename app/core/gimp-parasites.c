/* ligmaparasite.c: Copyright 1998 Jay Cox <jaycox@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-parasites.h"
#include "ligmaparasitelist.h"


gboolean
ligma_parasite_validate (Ligma                *ligma,
                        const LigmaParasite  *parasite,
                        GError             **error)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return TRUE;
}

void
ligma_parasite_attach (Ligma               *ligma,
                      const LigmaParasite *parasite)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (parasite != NULL);

  ligma_parasite_list_add (ligma->parasites, parasite);
}

void
ligma_parasite_detach (Ligma        *ligma,
                      const gchar *name)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (name != NULL);

  ligma_parasite_list_remove (ligma->parasites, name);
}

const LigmaParasite *
ligma_parasite_find (Ligma        *ligma,
                    const gchar *name)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return ligma_parasite_list_find (ligma->parasites, name);
}

static void
list_func (const gchar    *key,
           LigmaParasite   *parasite,
           gchar        ***current)
{
  *(*current)++ = g_strdup (key);
}

gchar **
ligma_parasite_list (Ligma *ligma)
{
  gint    count;
  gchar **list;
  gchar **current;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  count = ligma_parasite_list_length (ligma->parasites);

  list = current = g_new0 (gchar *, count + 1);

  ligma_parasite_list_foreach (ligma->parasites, (GHFunc) list_func, &current);

  return list;
}


/*  FIXME: this doesn't belong here  */

void
ligma_parasite_shift_parent (LigmaParasite *parasite)
{
  g_return_if_fail (parasite != NULL);

  parasite->flags = (parasite->flags >> 8);
}


/*  parasiterc functions  */

void
ligma_parasiterc_load (Ligma *ligma)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  file = ligma_directory_file ("parasiterc", NULL);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  if (! ligma_config_deserialize_file (LIGMA_CONFIG (ligma->parasites),
                                      file, NULL, &error))
    {
      if (error->code != LIGMA_CONFIG_ERROR_OPEN_ENOENT)
        ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);

      g_error_free (error);
    }

  g_object_unref (file);
}

void
ligma_parasiterc_save (Ligma *ligma)
{
  const gchar *header =
    "LIGMA parasiterc\n"
    "\n"
    "This file will be entirely rewritten each time you exit.";
  const gchar *footer =
    "end of parasiterc";

  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_PARASITE_LIST (ligma->parasites));

  file = ligma_directory_file ("parasiterc", NULL);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  if (! ligma_config_serialize_to_file (LIGMA_CONFIG (ligma->parasites),
                                       file,
                                       header, footer, NULL,
                                       &error))
    {
      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}
