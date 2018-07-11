/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-help-domain.c
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

#include "plug-in-types.h"

#include "gimppluginmanager.h"
#include "gimppluginmanager-help-domain.h"


typedef struct _GimpPlugInHelpDomain GimpPlugInHelpDomain;

struct _GimpPlugInHelpDomain
{
  GFile *file;
  gchar *domain_name;
  gchar *domain_uri;
};


void
gimp_plug_in_manager_help_domain_exit (GimpPlugInManager *manager)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));

  for (list = manager->help_domains; list; list = list->next)
    {
      GimpPlugInHelpDomain *domain = list->data;

      g_object_unref (domain->file);
      g_free (domain->domain_name);
      g_free (domain->domain_uri);
      g_slice_free (GimpPlugInHelpDomain, domain);
    }

  g_slist_free (manager->help_domains);
  manager->help_domains = NULL;
}

void
gimp_plug_in_manager_add_help_domain (GimpPlugInManager *manager,
                                      GFile             *file,
                                      const gchar       *domain_name,
                                      const gchar       *domain_uri)
{
  GimpPlugInHelpDomain *domain;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (G_IS_FILE (file));
  g_return_if_fail (domain_name != NULL);

  domain = g_slice_new (GimpPlugInHelpDomain);

  domain->file        = g_object_ref (file);
  domain->domain_name = g_strdup (domain_name);
  domain->domain_uri  = g_strdup (domain_uri);

  manager->help_domains = g_slist_prepend (manager->help_domains, domain);

#ifdef VERBOSE
  g_print ("added help domain \"%s\" for base uri \"%s\"\n",
           domain->domain_name ? domain->domain_name : "(null)",
           domain->domain_uri  ? domain->domain_uri  : "(null)");
#endif
}

const gchar *
gimp_plug_in_manager_get_help_domain (GimpPlugInManager  *manager,
                                      GFile              *file,
                                      const gchar       **domain_uri)
{
  GSList *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (file == NULL || G_IS_FILE (file), NULL);

  if (domain_uri)
    *domain_uri = NULL;

  /*  A NULL prog_name is GIMP itself, return the default domain  */
  if (! file)
    return NULL;

  for (list = manager->help_domains; list; list = list->next)
    {
      GimpPlugInHelpDomain *domain = list->data;

      if (domain && domain->file &&
          g_file_equal (domain->file, file))
        {
          if (domain_uri && domain->domain_uri)
            *domain_uri = domain->domain_uri;

          return domain->domain_name;
        }
    }

  return NULL;
}

gint
gimp_plug_in_manager_get_help_domains (GimpPlugInManager   *manager,
                                       gchar             ***help_domains,
                                       gchar             ***help_uris)
{
  gint n_domains;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), 0);
  g_return_val_if_fail (help_domains != NULL, 0);
  g_return_val_if_fail (help_uris != NULL, 0);

  n_domains = g_slist_length (manager->help_domains);

  if (n_domains > 0)
    {
      GSList *list;
      gint    i;

      *help_domains = g_new0 (gchar *, n_domains + 1);
      *help_uris    = g_new0 (gchar *, n_domains + 1);

      for (list = manager->help_domains, i = 0; list; list = list->next, i++)
        {
          GimpPlugInHelpDomain *domain = list->data;

          (*help_domains)[i] = g_strdup (domain->domain_name);
          (*help_uris)[i]    = g_strdup (domain->domain_uri);
        }
    }
  else
    {
      *help_domains = NULL;
      *help_uris    = NULL;
    }

  return n_domains;
}
