/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-locale-domain.c
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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "core/gimp.h"

#include "plug-in-locale-domain.h"


typedef struct _PlugInLocaleDomain PlugInLocaleDomain;

struct _PlugInLocaleDomain
{
  gchar *prog_name;
  gchar *domain_name;
  gchar *domain_path;
};


void
plug_in_locale_domain_exit (Gimp *gimp)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  for (list = gimp->plug_in_locale_domains; list; list = list->next)
    {
      PlugInLocaleDomain *domain = list->data;

      g_free (domain->prog_name);
      g_free (domain->domain_name);
      g_free (domain->domain_path);
      g_free (domain);
    }

  g_slist_free (gimp->plug_in_locale_domains);
  gimp->plug_in_locale_domains = NULL;
}

void
plug_in_locale_domain_add (Gimp        *gimp,
                           const gchar *prog_name,
                           const gchar *domain_name,
                           const gchar *domain_path)
{
  PlugInLocaleDomain *domain;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (prog_name != NULL);
  g_return_if_fail (domain_name != NULL);

  domain = g_new (PlugInLocaleDomain, 1);

  domain->prog_name   = g_strdup (prog_name);
  domain->domain_name = g_strdup (domain_name);
  domain->domain_path = g_strdup (domain_path);

  gimp->plug_in_locale_domains = g_slist_prepend (gimp->plug_in_locale_domains,
                                                  domain);

#ifdef VERBOSE
  g_print ("added locale domain \"%s\" for path \"%s\"\n",
           domain->domain_name ? domain->domain_name : "(null)",
           domain->domain_path ?
           gimp_filename_to_utf8 (domain->domain_path) : "(null)");
#endif
}

const gchar *
plug_in_locale_domain (Gimp         *gimp,
                       const gchar  *prog_name,
                       const gchar **domain_path)
{
  GSList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (domain_path)
    *domain_path = gimp_locale_directory ();

  /*  A NULL prog_name is GIMP itself, return the default domain  */
  if (! prog_name)
    return NULL;

  for (list = gimp->plug_in_locale_domains; list; list = list->next)
    {
      PlugInLocaleDomain *domain = list->data;

      if (domain && domain->prog_name &&
          ! strcmp (domain->prog_name, prog_name))
        {
          if (domain_path && domain->domain_path)
            *domain_path = domain->domain_path;

          return domain->domain_name;
        }
    }

  return plug_in_standard_locale_domain ();
}

const gchar *
plug_in_standard_locale_domain (void)
{
  return GETTEXT_PACKAGE "-std-plug-ins";
}
