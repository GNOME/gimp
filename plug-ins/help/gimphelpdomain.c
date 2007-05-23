/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help plug-in
 * Copyright (C) 1999-2004 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

/*  This code is written so that it can also be compiled standalone.
 *  It shouldn't depend on libgimp.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "gimphelp.h"

#ifdef DISABLE_NLS
#define _(String)  (String)
#else
#include "libgimp/stdplugins-intl.h"
#endif


/*  local function prototypes  */

static gboolean   domain_locale_parse      (GimpHelpDomain  *domain,
                                            GimpHelpLocale  *locale,
                                            GError         **error);

static gchar    * domain_filename_from_uri (const gchar     *uri);


/*  public functions  */

GimpHelpDomain *
gimp_help_domain_new (const gchar *domain_name,
                      const gchar *domain_uri,
                      const gchar *domain_root)
{
  GimpHelpDomain *domain = g_slice_new0 (GimpHelpDomain);

  domain->help_domain = g_strdup (domain_name);
  domain->help_uri    = g_strdup (domain_uri);
  domain->help_root   = g_strdup (domain_root);

  if (domain_uri)
    {
      /*  strip trailing slash  */
      if (g_str_has_suffix (domain->help_uri, "/"))
        domain->help_uri[strlen (domain->help_uri) - 1] = '\0';
    }

  return domain;
}

void
gimp_help_domain_free (GimpHelpDomain *domain)
{
  g_return_if_fail (domain != NULL);

  if (domain->help_locales)
    g_hash_table_destroy (domain->help_locales);

  g_free (domain->help_domain);
  g_free (domain->help_uri);
  g_free (domain->help_root);

  g_slice_free (GimpHelpDomain, domain);
}

GimpHelpLocale *
gimp_help_domain_lookup_locale (GimpHelpDomain *domain,
                                const gchar    *locale_id)
{
  GimpHelpLocale *locale = NULL;

  if (domain->help_locales)
    locale = g_hash_table_lookup (domain->help_locales, locale_id);
  else
    domain->help_locales =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             g_free,
                             (GDestroyNotify) gimp_help_locale_free);

  if (locale)
    return locale;

  locale = gimp_help_locale_new (locale_id);
  g_hash_table_insert (domain->help_locales, g_strdup (locale_id), locale);

  domain_locale_parse (domain, locale, NULL);

  return locale;
}

gchar *
gimp_help_domain_map (GimpHelpDomain  *domain,
                      GList           *help_locales,
                      const gchar     *help_id,
                      GimpHelpLocale **ret_locale,
                      gboolean        *fatal_error)
{
  GimpHelpLocale *locale = NULL;
  const gchar    *ref    = NULL;
  GList          *list;

  g_return_val_if_fail (domain != NULL, NULL);
  g_return_val_if_fail (help_locales != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  if (fatal_error)
    *fatal_error = FALSE;

  /*  first pass: look for a reference matching the help_id  */
  for (list = help_locales; list && !ref; list = list->next)
    {
      locale = gimp_help_domain_lookup_locale (domain,
                                               (const gchar *) list->data);
      ref = gimp_help_locale_map (locale, help_id);
    }

  /*  second pass: look for a fallback                 */
  for (list = help_locales; list && !ref; list = list->next)
    {
      locale = gimp_help_domain_lookup_locale (domain,
                                               (const gchar *) list->data);
      ref = locale->help_missing;
    }

  if (ret_locale)
    *ret_locale = locale;

  if (ref)
    {
      return g_strconcat (domain->help_uri,  "/",
                          locale->locale_id, "/",
                          ref,
                          NULL);
    }
  else  /*  try to assemble a useful error message  */
    {
      GError *error = NULL;

#ifdef GIMP_HELP_DEBUG
      g_printerr ("help: help_id lookup and all fallbacks failed for '%s'\n",
                  help_id);
#endif

      locale = gimp_help_domain_lookup_locale (domain,
                                               GIMP_HELP_DEFAULT_LOCALE);

      if (! domain_locale_parse (domain, locale, &error))
        {
          if (error->code == G_FILE_ERROR_NOENT)
            {
              g_message ("%s\n\n%s",
                         _("The GIMP help files are not found."),
                         _("Please install the additional help package or use "
                           "the online user manual at http://docs.gimp.org/."));
            }
          else
            {
              g_message ("%s\n\n%s\n\n%s",
                         _("There is a problem with the GIMP help files."),
                         error->message,
                         _("Please check your installation."));
            }

          g_error_free (error);

          if (fatal_error)
            *fatal_error = TRUE;
        }
      else
        {
          g_message (_("Help ID '%s' unknown"), help_id);
        }

      return NULL;
    }
}


/*  private functions  */

static gboolean
domain_locale_parse (GimpHelpDomain  *domain,
                     GimpHelpLocale  *locale,
                     GError         **error)
{
  gchar    *filename;
  gboolean  success;

  g_return_val_if_fail (domain != NULL, FALSE);
  g_return_val_if_fail (locale != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! domain->help_root)
    domain->help_root = domain_filename_from_uri (domain->help_uri);

  if (! domain->help_root)
    {
      g_set_error (error, 0, 0,
                   "Cannot determine location of gimp-help.xml from '%s'",
                   domain->help_uri);
      return FALSE;
    }

  filename = g_build_filename (domain->help_root,
                               locale->locale_id,
                               "gimp-help.xml",
                               NULL);

  success = gimp_help_locale_parse (locale,
                                    filename,
                                    domain->help_domain,
                                    error);

  g_free (filename);

  return success;
}

static gchar *
domain_filename_from_uri (const gchar *uri)
{
  gchar *filename;
  gchar *hostname;

  g_return_val_if_fail (uri != NULL, NULL);

  filename = g_filename_from_uri (uri, &hostname, NULL);

  if (!filename)
    return NULL;

  if (hostname)
    {
      /*  we have a file: URI with a hostname                           */
#ifdef G_OS_WIN32
      /*  on Win32, create a valid UNC path and use it as the filename  */

      gchar *tmp = g_build_filename ("//", hostname, filename, NULL);

      g_free (filename);
      filename = tmp;
#else
      /*  otherwise return NULL, caller should use URI then             */
      g_free (filename);
      filename = NULL;
#endif

      g_free (hostname);
    }

  return filename;
}
