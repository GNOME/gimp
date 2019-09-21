/*
 * gimp-test-clipboard.c -- do clipboard things
 *
 * Copyright (C) 2005  Michael Natterer <mitch@gimp.org>
 *
 * Use this code for whatever you like.
 */

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>
#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"


typedef struct _CopyData CopyData;

struct _CopyData
{
  const gchar *filename;
  gboolean     file_copied;
  GError      *error;
};


static void     test_clipboard_show_version    (void) G_GNUC_NORETURN;
static gboolean test_clipboard_parse_selection (const gchar       *option_name,
                                                const gchar       *value,
                                                gpointer           data,
                                                GError           **error);
static gboolean test_clipboard_list_targets    (GtkClipboard      *clipboard);
static gboolean test_clipboard_copy            (GtkClipboard      *clipboard,
                                                const gchar       *target,
                                                const gchar       *filename);
static gboolean test_clipboard_store           (GtkClipboard      *clipboard,
                                                const gchar       *target,
                                                const gchar       *filename);
static gboolean test_clipboard_paste           (GtkClipboard      *clipboard,
                                                const gchar       *target,
                                                const gchar       *filename);
static void     test_clipboard_copy_callback   (GtkClipboard      *clipboard,
                                                GtkSelectionData  *selection,
                                                guint              info,
                                                gpointer           data);


static GdkAtom   option_selection_type = GDK_SELECTION_CLIPBOARD;
static gboolean  option_list_targets   = FALSE;
static gchar    *option_target         = NULL;
static gchar    *option_copy_filename  = NULL;
static gchar    *option_store_filename = NULL;
static gchar    *option_paste_filename = NULL;

static const GOptionEntry main_entries[] =
{
  {
    "selection-type", 's', 0,
    G_OPTION_ARG_CALLBACK, test_clipboard_parse_selection,
    "Selection type (primary|secondary|clipboard)", "<type>"
  },
  {
    "list-targets", 'l', 0,
    G_OPTION_ARG_NONE, &option_list_targets,
    "List the targets offered by the clipboard", NULL
  },
  {
    "target", 't', 0,
    G_OPTION_ARG_STRING, &option_target,
    "The target format to copy or paste", "<target>"
  },
  {
    "copy", 'c', 0,
    G_OPTION_ARG_STRING, &option_copy_filename,
    "Copy <file> to clipboard", "<file>"
  },
  {
    "store", 'S', 0,
    G_OPTION_ARG_STRING, &option_store_filename,
    "Store <file> in the clipboard manager", "<file>"
  },
  {
    "paste", 'p', 0,
    G_OPTION_ARG_STRING, &option_paste_filename,
    "Paste clipboard into <file> ('-' pastes to STDOUT)", "<file>"
  },
  {
    "version", 'v', G_OPTION_FLAG_NO_ARG,
    G_OPTION_ARG_CALLBACK, test_clipboard_show_version,
    "Show version information and exit", NULL
  },
  { NULL }
};


gint
main (gint   argc,
      gchar *argv[])
{
  GOptionContext *context;
  GtkClipboard   *clipboard;
  GError         *error = NULL;

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
                                             option_selection_type);

  if (! clipboard)
    g_error ("gtk_clipboard_get_for_display");

  if (option_list_targets)
    {
      if (! test_clipboard_list_targets (clipboard))
        return EXIT_FAILURE;

      return EXIT_SUCCESS;
    }

  if ((option_copy_filename  && option_paste_filename) ||
      (option_copy_filename  && option_store_filename) ||
      (option_paste_filename && option_store_filename))
    {
      g_printerr ("Can't perform two operations at the same time\n");
      return EXIT_FAILURE;
    }

  if (option_copy_filename)
    {
      if (! option_target)
        {
          g_printerr ("Usage: %s -t <target> -c <file>\n", argv[0]);
          return EXIT_FAILURE;
        }

      if (! test_clipboard_copy (clipboard, option_target,
                                 option_copy_filename))
        return EXIT_FAILURE;
    }

  if (option_store_filename)
    {
      if (! option_target)
        {
          g_printerr ("Usage: %s -t <target> -S <file>\n", argv[0]);
          return EXIT_FAILURE;
        }

      if (! test_clipboard_store (clipboard, option_target,
                                  option_store_filename))
        return EXIT_FAILURE;
    }

  if (option_paste_filename)
    {
      if (! option_target)
        {
          g_printerr ("Usage: %s -t <target> -p <file>\n", argv[0]);
          return EXIT_FAILURE;
        }

      if (! test_clipboard_paste (clipboard, option_target,
                                  option_paste_filename))
        return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

static void
test_clipboard_show_version (void)
{
  g_print ("gimp-test-clipboard (GIMP clipboard testbed) version %s\n",
           GIMP_VERSION);

  exit (EXIT_SUCCESS);
}

static gboolean
test_clipboard_parse_selection (const gchar  *option_name,
                                const gchar  *value,
                                gpointer      data,
                                GError      **error)
{
  if (! strcmp (value, "primary"))
    option_selection_type = GDK_SELECTION_PRIMARY;
  else if (! strcmp (value, "secondary"))
    option_selection_type = GDK_SELECTION_SECONDARY;
  else if (! strcmp (value, "clipboard"))
    option_selection_type = GDK_SELECTION_CLIPBOARD;
  else
    return FALSE;

  return TRUE;
}

static gboolean
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

  return TRUE;
}

static gboolean
test_clipboard_copy (GtkClipboard *clipboard,
                     const gchar  *target,
                     const gchar  *filename)
{
  GtkTargetEntry entry;
  CopyData       data;

  entry.target = g_strdup (target);
  entry.flags  = 0;
  entry.info   = 1;

  data.filename    = filename;
  data.file_copied = FALSE;
  data.error       = NULL;

  if (! gtk_clipboard_set_with_data (clipboard, &entry, 1,
                                     test_clipboard_copy_callback,
                                     NULL,
                                     &data))
    {
      g_printerr ("%s: gtk_clipboard_set_with_data() failed\n",
                  g_get_prgname());
      return FALSE;
    }

  gtk_main ();

  if (! data.file_copied)
    {
      if (data.error)
        {
          g_printerr ("%s: copying failed: %s\n",
                      g_get_prgname (), data.error->message);
          g_error_free (data.error);
        }
      else
        {
          g_printerr ("%s: copying failed\n",
                      g_get_prgname ());
        }

      return FALSE;
    }

  return TRUE;
}

static gboolean
test_clipboard_store (GtkClipboard *clipboard,
                      const gchar  *target,
                      const gchar  *filename)
{
  GtkTargetEntry entry;
  CopyData       data;

  entry.target = g_strdup (target);
  entry.flags  = 0;
  entry.info   = 1;

  data.filename    = filename;
  data.file_copied = FALSE;
  data.error       = NULL;

  if (! gtk_clipboard_set_with_data (clipboard, &entry, 1,
                                     test_clipboard_copy_callback,
                                     NULL,
                                     &data))
    {
      g_printerr ("%s: gtk_clipboard_set_with_data() failed\n",
                  g_get_prgname ());
      return FALSE;
    }

  gtk_clipboard_set_can_store (clipboard, &entry, 1);
  gtk_clipboard_store (clipboard);

  if (! data.file_copied)
    {
      if (data.error)
        {
          g_printerr ("%s: storing failed: %s\n",
                      g_get_prgname (), data.error->message);
          g_error_free (data.error);
        }
      else
        {
          g_printerr ("%s: could not contact clipboard manager\n",
                      g_get_prgname ());
        }

      return FALSE;
    }

  return TRUE;
}

static gboolean
test_clipboard_paste (GtkClipboard *clipboard,
                      const gchar  *target,
                      const gchar  *filename)
{
  GtkSelectionData *sel_data;

  sel_data = gtk_clipboard_wait_for_contents (clipboard,
                                              gdk_atom_intern (target,
                                                               FALSE));
  if (sel_data)
    {
      const guchar *data;
      gint          length;
      gint          fd;

      if (! strcmp (filename, "-"))
        fd = 1;
      else
        fd = g_open (filename, O_WRONLY | O_CREAT | O_TRUNC | _O_BINARY, 0666);

      if (fd < 0)
        {
          g_printerr ("%s: open() filed: %s",
                      g_get_prgname (), g_strerror (errno));
          return FALSE;
        }

      data   = gtk_selection_data_get_data (sel_data);
      length = gtk_selection_data_get_length (sel_data);

      if (write (fd, data, length) < length)
        {
          close (fd);
          g_printerr ("%s: write() failed: %s",
                      g_get_prgname (), g_strerror (errno));
          return FALSE;
        }

      if (close (fd) < 0)
        {
          g_printerr ("%s: close() failed: %s",
                      g_get_prgname (), g_strerror (errno));
          return FALSE;
        }

      gtk_selection_data_free (sel_data);
    }

  return TRUE;
}

static void
test_clipboard_copy_callback (GtkClipboard     *clipboard,
                              GtkSelectionData *selection,
                              guint             info,
                              gpointer          data)
{
  CopyData *copy_data = data;
  gchar    *buf;
  gsize     buf_size;

  if (! g_file_get_contents (copy_data->filename, &buf, &buf_size,
                             &copy_data->error))
    {
      if (! option_store_filename)
        gtk_main_quit ();

      return;
    }

  gtk_selection_data_set (selection,
                          gtk_selection_data_get_target (selection),
                          8, (guchar *) buf, buf_size);

  g_free (buf);

  copy_data->file_copied = TRUE;

  g_print ("%s: data transfer in progress, hit <ctrl>+c when pasted...",
           G_STRFUNC);
}
