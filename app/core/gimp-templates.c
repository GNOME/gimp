/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-templates.h"
#include "gimplist.h"
#include "gimptemplate.h"


/* functions to load and save the gimp templates files */

void
gimp_templates_load (Gimp *gimp)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_LIST (gimp->templates));

  file = gimp_directory_file ("templaterc", NULL);

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  if (! gimp_config_deserialize_gfile (GIMP_CONFIG (gimp->templates),
                                       file, NULL, &error))
    {
      if (error->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_clear_error (&error);
          g_object_unref (file);

          file = gimp_sysconf_directory_file ("templaterc", NULL);

          if (! gimp_config_deserialize_gfile (GIMP_CONFIG (gimp->templates),
                                               file, NULL, &error))
            {
              gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR,
                                    error->message);
            }
        }
      else
        {
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
        }

      g_clear_error (&error);
    }

  gimp_list_reverse (GIMP_LIST (gimp->templates));

  g_object_unref (file);
}

void
gimp_templates_save (Gimp *gimp)
{
  const gchar *header =
    "GIMP templaterc\n"
    "\n"
    "This file will be entirely rewritten each time you exit.";
  const gchar *footer =
    "end of templaterc";

  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_LIST (gimp->templates));

  file = gimp_directory_file ("templaterc", NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  if (! gimp_config_serialize_to_gfile (GIMP_CONFIG (gimp->templates),
                                        file,
                                        header, footer, NULL,
                                        &error))
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}


/*  just like gimp_list_get_child_by_name() but matches case-insensitive
 *  and dpi/ppi-insensitive
 */
static GimpObject *
gimp_templates_migrate_get_child_by_name (GimpContainer *container,
                                          const gchar   *name)
{
  GimpList   *list   = GIMP_LIST (container);
  GimpObject *retval = NULL;
  GList      *glist;

  for (glist = list->queue->head; glist; glist = g_list_next (glist))
    {
      GimpObject *object = glist->data;
      gchar      *str1   = g_ascii_strdown (gimp_object_get_name (object), -1);
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
 * gimp_templates_migrate:
 * @olddir: the old user directory
 *
 * Migrating the templaterc from GIMP 2.0 to GIMP 2.2 needs this special
 * hack since we changed the way that units are handled. This function
 * merges the user's templaterc with the systemwide templaterc. The goal
 * is to replace the unit for a couple of default templates with "pixels".
 **/
void
gimp_templates_migrate (const gchar *olddir)
{
  GimpContainer *templates = gimp_list_new (GIMP_TYPE_TEMPLATE, TRUE);
  GFile         *file      = gimp_directory_file ("templaterc", NULL);

  if (gimp_config_deserialize_gfile (GIMP_CONFIG (templates), file,
                                     NULL, NULL))
    {
      GFile *sysconf_file;

      sysconf_file = gimp_sysconf_directory_file ("templaterc", NULL);

      if (olddir && (strstr (olddir, "2.0") || strstr (olddir, "2.2")))
        {
          /* We changed the spelling of a couple of template names:
           *
           * - from upper to lower case between 2.0 and 2.2
           * - from "dpi" to "ppi" between 2.2 and 2.4
           */
          GimpContainerClass *class = GIMP_CONTAINER_GET_CLASS (templates);
          gpointer            func  = class->get_child_by_name;

          class->get_child_by_name = gimp_templates_migrate_get_child_by_name;

          gimp_config_deserialize_gfile (GIMP_CONFIG (templates),
                                         sysconf_file, NULL, NULL);

          class->get_child_by_name = func;
        }
      else
        {
          gimp_config_deserialize_gfile (GIMP_CONFIG (templates),
                                         sysconf_file, NULL, NULL);
        }

      g_object_unref (sysconf_file);

      gimp_list_reverse (GIMP_LIST (templates));

      gimp_config_serialize_to_gfile (GIMP_CONFIG (templates), file,
                                      NULL, NULL, NULL, NULL);
    }

  g_object_unref (file);
}
