/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-locale-domain.c
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

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "gimppluginmanager.h"
#include "gimppluginmanager-locale-domain.h"


#define STD_PLUG_INS_LOCALE_DOMAIN GETTEXT_PACKAGE "-std-plug-ins"


typedef struct _GimpPlugInLocaleDomain GimpPlugInLocaleDomain;

struct _GimpPlugInLocaleDomain
{
  GFile *file;
  gchar *domain_name;
  gchar *domain_path;
};


void
gimp_plug_in_manager_locale_domain_exit (GimpPlugInManager *manager)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));

  for (list = manager->locale_domains; list; list = list->next)
    {
      GimpPlugInLocaleDomain *domain = list->data;

      g_object_unref (domain->file);
      g_free (domain->domain_name);
      g_free (domain->domain_path);
      g_slice_free (GimpPlugInLocaleDomain, domain);
    }

  g_slist_free (manager->locale_domains);
  manager->locale_domains = NULL;
}

void
gimp_plug_in_manager_add_locale_domain (GimpPlugInManager *manager,
                                        GFile             *file,
                                        const gchar       *domain_name,
                                        const gchar       *domain_path)
{
  GimpPlugInLocaleDomain *domain;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (G_IS_FILE (file));
  g_return_if_fail (domain_name != NULL);

  domain = g_slice_new (GimpPlugInLocaleDomain);

  domain->file        = g_object_ref (file);
  domain->domain_name = g_strdup (domain_name);
  domain->domain_path = g_strdup (domain_path);

  manager->locale_domains = g_slist_prepend (manager->locale_domains, domain);

#ifdef VERBOSE
  g_print ("added locale domain \"%s\" for path \"%s\"\n",
           domain->domain_name ? domain->domain_name : "(null)",
           domain->domain_path ?
           gimp_filename_to_utf8 (domain->domain_path) : "(null)");
#endif
}

const gchar *
gimp_plug_in_manager_get_locale_domain (GimpPlugInManager  *manager,
                                        GFile              *file,
                                        const gchar       **domain_path)
{
  GSList *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (file == NULL || G_IS_FILE (file), NULL);

  if (domain_path)
    *domain_path = gimp_locale_directory ();

  /*  A NULL prog_name is GIMP itself, return the default domain  */
  if (! file)
    return NULL;

  for (list = manager->locale_domains; list; list = list->next)
    {
      GimpPlugInLocaleDomain *domain = list->data;

      if (domain && domain->file &&
          g_file_equal (domain->file, file))
        {
          if (domain_path && domain->domain_path)
            *domain_path = domain->domain_path;

          return domain->domain_name;
        }
    }

  return STD_PLUG_INS_LOCALE_DOMAIN;
}

gint
gimp_plug_in_manager_get_locale_domains (GimpPlugInManager   *manager,
                                         gchar             ***locale_domains,
                                         gchar             ***locale_paths)
{
  GSList *list;
  GSList *unique = NULL;
  gint    n_domains;
  gint    i;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), 0);
  g_return_val_if_fail (locale_domains != NULL, 0);
  g_return_val_if_fail (locale_paths != NULL, 0);

  for (list = manager->locale_domains; list; list = list->next)
    {
      GimpPlugInLocaleDomain *domain = list->data;
      GSList                 *tmp;

      for (tmp = unique; tmp; tmp = tmp->next)
        if (! strcmp (domain->domain_name, (const gchar *) tmp->data))
          break;

      if (! tmp)
        unique = g_slist_prepend (unique, domain);
    }

  unique = g_slist_reverse (unique);

  n_domains = g_slist_length (unique) + 1;

  *locale_domains = g_new0 (gchar *, n_domains + 1);
  *locale_paths   = g_new0 (gchar *, n_domains + 1);

  (*locale_domains)[0] = g_strdup (STD_PLUG_INS_LOCALE_DOMAIN);
  (*locale_paths)[0]   = g_strdup (gimp_locale_directory ());

  for (list = unique, i = 1; list; list = list->next, i++)
    {
      GimpPlugInLocaleDomain *domain = list->data;

      (*locale_domains)[i] = g_strdup (domain->domain_name);
      (*locale_paths)[i]   = (domain->domain_path ?
                              g_strdup (domain->domain_path) :
                              g_strdup (gimp_locale_directory ()));
    }

  g_slist_free (unique);

  return n_domains;
}
