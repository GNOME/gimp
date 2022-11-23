/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-templates.h"
#include "ligmalist.h"
#include "ligmatemplate.h"


/* functions to load and save the ligma templates files */

void
ligma_templates_load (Ligma *ligma)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_LIST (ligma->templates));

  file = ligma_directory_file ("templaterc", NULL);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  if (! ligma_config_deserialize_file (LIGMA_CONFIG (ligma->templates),
                                      file, NULL, &error))
    {
      if (error->code == LIGMA_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_clear_error (&error);
          g_object_unref (file);

          if (g_getenv ("LIGMA_TESTING_ABS_TOP_SRCDIR"))
            {
              gchar *path;
              path = g_build_filename (g_getenv ("LIGMA_TESTING_ABS_TOP_SRCDIR"),
                                       "etc", "templaterc", NULL);
              file = g_file_new_for_path (path);
              g_free (path);
            }
          else
            {
              file = ligma_sysconf_directory_file ("templaterc", NULL);
            }

          if (! ligma_config_deserialize_file (LIGMA_CONFIG (ligma->templates),
                                              file, NULL, &error))
            {
              ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR,
                                    error->message);
            }
        }
      else
        {
          ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
        }

      g_clear_error (&error);
    }

  ligma_list_reverse (LIGMA_LIST (ligma->templates));

  g_object_unref (file);
}

void
ligma_templates_save (Ligma *ligma)
{
  const gchar *header =
    "LIGMA templaterc\n"
    "\n"
    "This file will be entirely rewritten each time you exit.";
  const gchar *footer =
    "end of templaterc";

  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_LIST (ligma->templates));

  file = ligma_directory_file ("templaterc", NULL);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  if (! ligma_config_serialize_to_file (LIGMA_CONFIG (ligma->templates),
                                       file,
                                       header, footer, NULL,
                                       &error))
    {
      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}


/*  just like ligma_list_get_child_by_name() but matches case-insensitive
 *  and dpi/ppi-insensitive
 */
static LigmaObject *
ligma_templates_migrate_get_child_by_name (LigmaContainer *container,
                                          const gchar   *name)
{
  LigmaList   *list   = LIGMA_LIST (container);
  LigmaObject *retval = NULL;
  GList      *glist;

  for (glist = list->queue->head; glist; glist = g_list_next (glist))
    {
      LigmaObject *object = glist->data;
      gchar      *str1   = g_ascii_strdown (ligma_object_get_name (object), -1);
      gchar      *str2   = g_ascii_strdown (name, -1);

      if (! strcmp (str1, str2))
        {
          retval = object;
        }
      else
        {
          gchar *dpi = strstr (str1, "dpi");

          if (dpi)
            {
              memcpy (dpi, "ppi", 3);

              g_print ("replaced: %s\n", str1);

              if (! strcmp (str1, str2))
                retval = object;
            }
        }

      g_free (str1);
      g_free (str2);
    }

  return retval;
}

/**
 * ligma_templates_migrate:
 * @olddir: the old user directory
 *
 * Migrating the templaterc from LIGMA 2.0 to LIGMA 2.2 needs this special
 * hack since we changed the way that units are handled. This function
 * merges the user's templaterc with the systemwide templaterc. The goal
 * is to replace the unit for a couple of default templates with "pixels".
 **/
void
ligma_templates_migrate (const gchar *olddir)
{
  LigmaContainer *templates = ligma_list_new (LIGMA_TYPE_TEMPLATE, TRUE);
  GFile         *file      = ligma_directory_file ("templaterc", NULL);

  if (ligma_config_deserialize_file (LIGMA_CONFIG (templates), file,
                                    NULL, NULL))
    {
      GFile *sysconf_file;

      sysconf_file = ligma_sysconf_directory_file ("templaterc", NULL);

      if (olddir && (strstr (olddir, "2.0") || strstr (olddir, "2.2")))
        {
          /* We changed the spelling of a couple of template names:
           *
           * - from upper to lower case between 2.0 and 2.2
           * - from "dpi" to "ppi" between 2.2 and 2.4
           */
          LigmaContainerClass *class = LIGMA_CONTAINER_GET_CLASS (templates);
          gpointer            func  = class->get_child_by_name;

          class->get_child_by_name = ligma_templates_migrate_get_child_by_name;

          ligma_config_deserialize_file (LIGMA_CONFIG (templates),
                                        sysconf_file, NULL, NULL);

          class->get_child_by_name = func;
        }
      else
        {
          ligma_config_deserialize_file (LIGMA_CONFIG (templates),
                                        sysconf_file, NULL, NULL);
        }

      g_object_unref (sysconf_file);

      ligma_list_reverse (LIGMA_LIST (templates));

      ligma_config_serialize_to_file (LIGMA_CONFIG (templates), file,
                                     NULL, NULL, NULL, NULL);
    }

  g_object_unref (file);
}
