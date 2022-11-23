/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-help-lookup - a standalone ligma-help ID to filename mapper
 * Copyright (C)  2004-2008 Sven Neumann <sven@ligma.org>
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

#include <stdlib.h>
#include <string.h>

#include "libligma/ligma.h"

#include "ligmahelp.h"


static void               show_version (void) G_GNUC_NORETURN;

static gchar            * lookup       (const gchar *help_domain,
                                        const gchar *help_locales,
                                        const gchar *help_id);

static LigmaHelpProgress * progress_new (void);


static const gchar  *help_base    = NULL;
static       gchar  *help_root    = NULL;
static const gchar  *help_locales = NULL;
static const gchar **help_ids     = NULL;

static gboolean      be_verbose   = FALSE;


static const GOptionEntry entries[] =
{
  { "version", 'v', 0,
    G_OPTION_ARG_CALLBACK, (GOptionArgFunc) show_version,
    "Show version information and exit", NULL
  },
  { "base", 'b', 0,
    G_OPTION_ARG_STRING, &help_base,
    "Specifies base URI", "URI"
  },
  { "root", 'r', 0,
    G_OPTION_ARG_FILENAME, &help_root,
    "Specifies root directory for index files", "DIR"
  },
  { "lang", 'l', 0,
    G_OPTION_ARG_STRING, &help_locales,
    "Specifies help language", "LANG"
  },
  {
    "verbose", 0, 0,
    G_OPTION_ARG_NONE, &be_verbose,
    "Be more verbose", NULL
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

  help_base = g_getenv (LIGMA_HELP_ENV_URI);
  help_root = g_build_filename (ligma_data_directory (), LIGMA_HELP_PREFIX, NULL);

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

  ligma_help_register_domain (LIGMA_HELP_DEFAULT_DOMAIN, uri);
  g_free (uri);

  uri = lookup (LIGMA_HELP_DEFAULT_DOMAIN,
                help_locales ? help_locales : LIGMA_HELP_DEFAULT_LOCALE,
                help_ids     ? help_ids[0]  : LIGMA_HELP_DEFAULT_ID);

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
  LigmaHelpDomain *domain = ligma_help_lookup_domain (help_domain);

  if (domain)
    {
      LigmaHelpProgress *progress = progress_new ();
      GList            *locales;
      gchar            *full_uri;

      locales  = ligma_help_parse_locales (help_locales);
      full_uri = ligma_help_domain_map (domain, locales, help_id, progress,
                                       NULL, NULL);

      ligma_help_progress_free (progress);

      g_list_free_full (locales, (GDestroyNotify) g_free);

      return full_uri;
    }

  return NULL;
}

static void
show_version (void)
{
  g_print ("ligma-help-lookup version %s\n", LIGMA_VERSION);
  exit (EXIT_SUCCESS);
}


static void
progress_start (const gchar *message,
                gboolean     cancelable,
                gpointer     user_data)
{
  if (be_verbose)
    g_printerr ("\n%s\n", message);
}

static void
progress_end (gpointer user_data)
{
  if (be_verbose)
    g_printerr ("done\n");
}

static void
progress_set_value (gdouble  percentage,
                    gpointer user_data)
{
  if (be_verbose)
    g_printerr (".");
}

static LigmaHelpProgress *
progress_new (void)
{
  const LigmaHelpProgressVTable vtable =
    {
      progress_start,
      progress_end,
      progress_set_value
    };

  return ligma_help_progress_new (&vtable, NULL);
}
