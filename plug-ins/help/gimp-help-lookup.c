/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-help-lookup - a standalone gimp-help ID to filename mapper
 * Copyright (C)  2004 Sven Neumann <sven@gimp.org>
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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "gimphelp.h"


static void    show_version (void) G_GNUC_NORETURN;

static gchar * lookup       (const gchar *help_domain,
                             const gchar *help_locales,
                             const gchar *help_id);


static const gchar  *help_base    = NULL;
static       gchar  *help_root    = NULL;
static const gchar  *help_locales = NULL;
static const gchar **help_ids     = NULL;


static const GOptionEntry entries[] =
{
  { "version", 'v', 0,
    G_OPTION_ARG_CALLBACK, (GOptionArgFunc) show_version,
    "Show version information and exit", NULL
  },
  { "base", 'b', 0,
    G_OPTION_ARG_STRING, &help_base,
    "Speficies base URI", "URI"
  },
  { "root", 'r', 0,
    G_OPTION_ARG_FILENAME, &help_root,
    "Speficies root directory for index files", "DIR"
  },
  { "lang", 'l', 0,
    G_OPTION_ARG_STRING, &help_locales,
    "Specifies help language", "LANG"
  },
  {
    G_OPTION_REMAINING, 0, 0,
    G_OPTION_ARG_STRING_ARRAY, &help_ids,
    NULL, NULL
  },
  { NULL }
};


gint
main (gint   argc,
      gchar *argv[])
{
  GOptionContext *context;
  gchar          *uri;
  GError         *error = NULL;

  help_base = g_getenv (GIMP_HELP_ENV_URI);
  help_root = g_build_path (G_DIR_SEPARATOR_S, gimp_data_directory (), GIMP_HELP_PREFIX, NULL);

  context = g_option_context_new ("HELP-ID");
  g_option_context_add_main_entries (context, entries, NULL);
  if (! g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

  if (help_base)
    uri = g_strdup (help_base);
  else
    uri = g_filename_to_uri (help_root, NULL, NULL);

  gimp_help_register_domain (GIMP_HELP_DEFAULT_DOMAIN, uri, help_root);
  g_free (uri);

  uri = lookup (GIMP_HELP_DEFAULT_DOMAIN,
                help_locales ? help_locales : GIMP_HELP_DEFAULT_LOCALE,
                help_ids     ? help_ids[0]  : GIMP_HELP_DEFAULT_ID);

  if (uri)
    {
      g_print ("%s\n", uri);
      g_free (uri);
    }

  g_option_context_free (context);
  g_free (help_root);

  return uri ? EXIT_SUCCESS : EXIT_FAILURE;
}

static gchar *
lookup (const gchar *help_domain,
        const gchar *help_locales,
        const gchar *help_id)
{
  GimpHelpDomain *domain = gimp_help_lookup_domain (help_domain);

  if (domain)
    {
      GList *locales  = gimp_help_parse_locales (help_locales);
      gchar *full_uri = gimp_help_domain_map (domain, locales, help_id,
                                              NULL, NULL);

      g_list_foreach (locales, (GFunc) g_free, NULL);
      g_list_free (locales);

      return full_uri;
    }

  return NULL;
}

static void
show_version (void)
{
  g_print ("gimp-help-lookup version %s\n", GIMP_VERSION);
  exit (EXIT_SUCCESS);
}
