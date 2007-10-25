/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-remote.c
 * Copyright (C) 2000-2007  Sven Neumann <sven@gimp.org>
 *                          Simon Budig <simon@gimp.org>
 *
 * Tells a running gimp to open files by creating a synthetic drop-event.
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

/* Disclaimer:
 *
 * It is a really bad idea to use Drag'n'Drop for inter-client
 * communication. Dont even think about doing this in your own newly
 * created application.
 *                                                Simon
 */

#include "config.h"

#include <stdlib.h>
#include <locale.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpversion.h"

#include <glib/gi18n.h>

#include "gimp-remote.h"


static void  show_version (void) G_GNUC_NORETURN;


static gboolean      existing  = FALSE;
static gboolean      query     = FALSE;
static gboolean      no_splash = FALSE;
static gboolean      print_xid = FALSE;
static const gchar **filenames = NULL;

static const GOptionEntry main_entries[] =
{
  { "version", 'v', G_OPTION_FLAG_NO_ARG,
    G_OPTION_ARG_CALLBACK, (GOptionArgFunc) show_version,
    N_("Show version information and exit"), NULL
  },
  {
    "existing", 'e', 0,
    G_OPTION_ARG_NONE, &existing,
    N_("Use a running GIMP only, never start a new one"), NULL
  },
  {
    "query", 'q', 0,
    G_OPTION_ARG_NONE, &query,
    N_("Only check if GIMP is running, then quit"), NULL
  },
#ifdef GDK_WINDOWING_X11
  {
    "print-xid", 'p', 0,
    G_OPTION_ARG_NONE, &print_xid,
    N_("Print X window ID of GIMP toolbox window, then quit"), NULL
  },
#endif
  {
    "no-splash", 's', 0,
    G_OPTION_ARG_NONE, &no_splash,
    N_("Start GIMP without showing the startup window"), NULL
  },
  {
    G_OPTION_REMAINING, 0, 0,
    G_OPTION_ARG_FILENAME_ARRAY, &filenames,
    NULL, NULL
  },
  { NULL }
};

static void
show_version (void)
{
  g_print (_("%s version %s"), "gimp-remote", GIMP_VERSION);
  g_print ("\n");

  exit (EXIT_SUCCESS);
}

static void
init_i18n (void)
{
  setlocale (LC_ALL, "");

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  textdomain (GETTEXT_PACKAGE);
}

/* Check name for a valid URI scheme as described in RFC 2396.
 * URI scheme:suffix with scheme = [alpha][alphanum|digit|+|-|.]*
 * Note that a null suffix will be considered as an invalid URI.
 * return 1 if valid scheme found, else 0
 */
static gboolean
is_valid_uri (const gchar *name)
{
  const gchar *c = name;

  g_return_val_if_fail (name != NULL, FALSE);

  /* first character must be alpha */
  if (! g_ascii_isalpha (*c))
    return FALSE;

  c++;
  while (*c != '\0' && *c != ':')
    {
      if (! g_ascii_isalnum (*c) && *c != '+' && *c != '-' && *c != '.')
	return FALSE;
      c++;
    }

  if (*c == ':')
    {
      c++;
      if (*c != '\0')
	return TRUE;
    }

  return FALSE;
}

gint
main (gint    argc,
      gchar **argv)
{
  GOptionContext *context;
  GError         *error = NULL;
  GdkDisplay     *display;
  GdkScreen      *screen;
  GdkWindow      *toolbox;
  const gchar    *startup_id;
  gchar          *desktop_startup_id = NULL;
  GString        *file_list          = g_string_new (NULL);

  init_i18n ();

  /* we save the startup_id before calling gtk_init()
     because GTK+ will unset it  */

  startup_id = g_getenv ("DESKTOP_STARTUP_ID");
  if (startup_id && *startup_id)
    desktop_startup_id = g_strdup (startup_id);

  /* parse the command-line options */
  context = g_option_context_new ("[FILE|URI...]");
  g_option_context_add_main_entries (context, main_entries, NULL);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  if (! g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);

      exit (EXIT_FAILURE);
    }

  gtk_init (&argc, &argv);

  if (filenames)
    {
      gchar *cwd = g_get_current_dir ();
      gint   i;

      for (i = 0; filenames[i]; i++)
        {
          const gchar *name = filenames[i];
          gchar       *uri;

          /* If not already a valid URI */
          if (! is_valid_uri (name))
            {
              if (g_path_is_absolute (name))
                {
                  uri = g_filename_to_uri (name, NULL, NULL);
                }
              else
                {
                  gchar *abs = g_build_filename (cwd, name, NULL);

                  uri = g_filename_to_uri (abs, NULL, NULL);
                  g_free (abs);
                }
            }
          else
            {
              uri = g_strdup (name);
            }

          if (uri)
            {
              if (file_list->len > 0)
                file_list = g_string_append_c (file_list, '\n');

              file_list = g_string_append (file_list, uri);
              g_free (uri);
            }
        }

      g_free (cwd);
    }

  display = gdk_display_get_default ();
  screen  = gdk_screen_get_default ();

  /*  if called without any filenames, always start a new GIMP  */
  if (file_list->len == 0 && !query && !print_xid && !existing)
    {
      gimp_remote_launch (screen,
                          argv[0], desktop_startup_id, no_splash,
                          file_list);
    }

  toolbox = gimp_remote_find_toolbox (display, screen);

  if (! query && ! print_xid)
    {
      if (toolbox)
        {
          if (! gimp_remote_drop_files (display, toolbox, file_list))
            return EXIT_FAILURE;
        }
      else if (! existing)
        {
          gimp_remote_launch (screen,
                              argv[0], desktop_startup_id, no_splash,
                              file_list);
        }
    }
  else if (print_xid && toolbox)
    {
      gimp_remote_print_id (toolbox);
    }

  gdk_notify_startup_complete ();

  g_option_context_free (context);

  g_string_free (file_list, TRUE);
  g_free (desktop_startup_id);

  return (toolbox ? EXIT_SUCCESS : EXIT_FAILURE);
}
