/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

/*  This code is written so that it can also be compiled standalone.
 *  It shouldn't depend on libgimp.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>
#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "gimphelp.h"

#ifdef DISABLE_NLS
#define _(String)  (String)
#else
#include "libgimp/stdplugins-intl.h"
#endif


/*  local function prototypes  */

static gboolean   domain_locale_parse (GimpHelpDomain    *domain,
                                       GimpHelpLocale    *locale,
                                       GimpHelpProgress  *progress,
                                       GError           **error);


/*  public functions  */

GimpHelpDomain *
gimp_help_domain_new (const gchar *domain_name,
                      const gchar *domain_uri)
{
  GimpHelpDomain *domain = g_slice_new0 (GimpHelpDomain);

  domain->help_domain = g_strdup (domain_name);
  domain->help_uri    = g_strdup (domain_uri);

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

  g_slice_free (GimpHelpDomain, domain);
}

GimpHelpLocale *
gimp_help_domain_lookup_locale (GimpHelpDomain    *domain,
                                const gchar       *locale_id,
                                GimpHelpProgress  *progress)
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

  domain_locale_parse (domain, locale, progress, NULL);

  return locale;
}

gchar *
gimp_help_domain_map (GimpHelpDomain    *domain,
                      GList             *help_locales,
                      const gchar       *help_id,
                      GimpHelpProgress  *progress,
                      GimpHelpLocale   **ret_locale,
                      gboolean          *fatal_error)
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
                                               (const gchar *) list->data,
                                               progress);
      ref = gimp_help_locale_map (locale, help_id);
    }

  /*  second pass: look for a fallback                 */
  for (list = help_locales; list && !ref; list = list->next)
    {
      locale = gimp_help_domain_lookup_locale (domain,
                                               (const gchar *) list->data,
                                               progress);
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
                                               GIMP_HELP_DEFAULT_LOCALE, NULL);

      if (! domain_locale_parse (domain, locale, NULL, &error))
        {
          switch (error->code)
            {
            case G_IO_ERROR_NOT_FOUND:
              if (domain->help_domain)
                {
                  g_message (_("The help pages for '%s' are not available."),
                             domain->help_domain);
                }
              else
                {
                  g_message ("%s\n\n%s",
                             _("The GIMP user manual is not available."),
                             /* TRANSLATORS: do not end the URL with a dot,
                              * it would be in the link. Because of
                              * technical limitations, make sure the URL
                              * ends with a space, a newline or is end of text.
                              * Cf. bug 762282.
                              */
                             _("Please install the additional help package "
                               "or use the online user manual at: "
                               "https://docs.gimp.org/"));
                }
              break;

            case G_IO_ERROR_NOT_SUPPORTED:
              g_message ("%s\n\n%s",
                         error->message,
                         _("Perhaps you are missing GIO backends and need "
                           "to install GVFS?"));
              break;

            case G_IO_ERROR_CANCELLED:
              break;

            default:
              g_message ("%s", error->message);
              break;
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
domain_locale_parse (GimpHelpDomain    *domain,
                     GimpHelpLocale    *locale,
                     GimpHelpProgress  *progress,
                     GError           **error)
{
  gchar    *uri;
  gboolean  success;

  g_return_val_if_fail (domain != NULL, FALSE);
  g_return_val_if_fail (locale != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  uri = g_strdup_printf ("%s/%s/gimp-help.xml",
                         domain->help_uri, locale->locale_id);

  success = gimp_help_locale_parse (locale, uri, domain->help_domain,
                                    progress, error);

  g_free (uri);

  return success;
}
