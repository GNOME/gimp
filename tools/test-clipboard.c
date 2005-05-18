/*
 * test-clipboard.c -- do clipboard things
 *
 * Copyright (C) 2005  Michael Natterer <mitch@gimp.org>
 *
 * Use this code for whatever you like.
 */

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>


static void   test_clipboard_list_targets  (GtkClipboard     *clipboard);
static void   test_clipboard_copy          (GtkClipboard     *clipboard,
                                            const gchar      *target,
                                            const gchar      *filename);
static void   test_clipboard_paste         (GtkClipboard     *clipboard,
                                            const gchar      *target,
                                            const gchar      *filename);
static void   test_clipboard_copy_callback (GtkClipboard     *clipboard,
                                            GtkSelectionData *selection,
                                            guint             info,
                                            gpointer          data);


gint
main (gint   argc,
      gchar *argv[])
{
  GOptionContext *context;
  GtkClipboard   *clipboard;
  GError         *error = NULL;

  /*  options  */
  static gboolean  list_targets   = FALSE;
  static gchar    *target         = NULL;
  static gchar    *copy_filename  = NULL;
  static gchar    *paste_filename = NULL;

  static const GOptionEntry main_entries[] =
  {
    {
      "list-targets", 'l', 0,
      G_OPTION_ARG_NONE, &list_targets,
      "List the targets offered by the clipboard", NULL
    },
    {
      "target", 't', 0,
      G_OPTION_ARG_STRING, &target,
      "The target format to copy or paste", "<target>"
    },
    {
      "copy", 'c', 0,
      G_OPTION_ARG_STRING, &copy_filename,
      "Copy <file> to clipboard", "<file>"
    },
    {
      "paste", 'p', 0,
      G_OPTION_ARG_STRING, &paste_filename,
      "Paste clipoard into <file>", "<file>"
    }
  };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, main_entries, NULL);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  if (! g_option_context_parse (context, &argc, &argv, &error))
    {
      if (error)
        {
          g_printerr ("%s\n", error->message);
          g_error_free (error);
        }
      else
        {
          g_print ("%s\n",
                   "Could not initialize the graphical user interface.\n"
                   "Make sure a proper setup for your display environment "
                   "exists.");
        }

      return EXIT_FAILURE;
    }

  gtk_init (&argc, &argv);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (! clipboard)
    g_error ("gtk_clipboard_get_for_display");

  if (list_targets)
    {
      test_clipboard_list_targets (clipboard);
      return EXIT_SUCCESS;
    }

  if (copy_filename)
    {
      if (! target)
        g_printerr ("Usage: %s -t <target> -c <file>\n", argv[0]);

      test_clipboard_copy (clipboard, target, copy_filename);
      return EXIT_SUCCESS;
    }

  if (paste_filename)
    {
      if (! target)
        g_printerr ("Usage: %s -t <target> -p <file>\n", argv[0]);

      test_clipboard_paste (clipboard, target, paste_filename);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

static void
test_clipboard_list_targets (GtkClipboard *clipboard)
{
  GtkSelectionData *data;

  data = gtk_clipboard_wait_for_contents (clipboard,
                                          gdk_atom_intern ("TARGETS",
                                                           FALSE));
  if (data)
    {
      GdkAtom  *targets;
      gint      n_targets;
      gboolean  success;

      success = gtk_selection_data_get_targets (data, &targets, &n_targets);

      gtk_selection_data_free (data);

      if (success)
        {
          gint i;

          for (i = 0; i < n_targets; i++)
            g_print ("%s\n", gdk_atom_name (targets[i]));

          g_free (targets);
        }
    }
}

static void
test_clipboard_copy (GtkClipboard *clipboard,
                     const gchar  *target,
                     const gchar  *filename)
{
  GtkTargetEntry entry;

  entry.target = g_strdup (target);
  entry.flags  = 0;
  entry.info   = 1;

  if (! gtk_clipboard_set_with_data (clipboard, &entry, 1,
                                     test_clipboard_copy_callback,
                                     NULL,
                                     g_strdup (filename)))
    g_error ("gtk_clipboard_set_with_data");

  gtk_main ();
}

static void
test_clipboard_paste (GtkClipboard *clipboard,
                     const gchar  *target,
                     const gchar  *filename)
{
  GtkSelectionData *data;

  data = gtk_clipboard_wait_for_contents (clipboard,
                                          gdk_atom_intern (target,
                                                           FALSE));
  if (data)
    {
      gsize bytes;
      gint  fd;

      fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

      if (fd < 0)
        g_error ("open: %s", g_strerror (errno));

      bytes = data->length * data->format / 8;

      if (write (fd, data->data, bytes) < bytes)
        g_error ("write: %s", g_strerror (errno));

      if (close (fd) < 0)
        g_error ("close: %s", g_strerror (errno));

      gtk_selection_data_free (data);
    }
}

static void
test_clipboard_copy_callback (GtkClipboard     *clipboard,
                              GtkSelectionData *selection,
                              guint             info,
                              gpointer          data)
{
  gchar  *filename = data;
  gchar  *buf;
  gsize   buf_size;
  GError *error = NULL;

  if (! g_file_get_contents (filename, &buf, &buf_size, &error))
    g_error ("g_file_get_contents: %s", error->message);

  g_free (filename);

  gtk_selection_data_set (selection, selection->target,
                          8, (guchar *) buf, buf_size);

  g_free (buf);

  gtk_main_quit ();
}
