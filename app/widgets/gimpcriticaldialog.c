/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcriticaldialog.c
 * Copyright (C) 2018  Jehan <jehan@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This widget is particular that I want to be able to use it internally
 * but also from an alternate tool (gimpdebug). It means that the
 * implementation must stay as generic glib/GTK+ as possible.
 */

#include "config.h"

#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#ifdef PLATFORM_OSX
#import <Cocoa/Cocoa.h>
#endif

#ifdef G_OS_WIN32
#undef DATADIR
#include <windows.h>
#endif

#include "gimpcriticaldialog.h"

#include "version.h"

#include "gimp-intl.h"

typedef struct _GimpCriticalDialog       GimpCriticalDialog;

#define GIMP_CRITICAL_RESPONSE_CLIPBOARD 1
#define GIMP_CRITICAL_RESPONSE_URL       2
#define GIMP_CRITICAL_RESPONSE_RESTART   3


static void    gimp_critical_dialog_finalize (GObject     *object);
static void    gimp_critical_dialog_response (GtkDialog   *dialog,
                                              gint         response_id);

static gboolean browser_open_url             (const gchar  *url,
                                              GError      **error);

G_DEFINE_TYPE (GimpCriticalDialog, gimp_critical_dialog, GTK_TYPE_DIALOG)

#define parent_class gimp_critical_dialog_parent_class


static void
gimp_critical_dialog_class_init (GimpCriticalDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->finalize = gimp_critical_dialog_finalize;

  dialog_class->response = gimp_critical_dialog_response;
}

static void
gimp_critical_dialog_init (GimpCriticalDialog *dialog)
{
  PangoAttrList  *attrs;
  PangoAttribute *attr;
  gchar          *text;
  gchar          *version;
  GtkWidget      *widget;
  GtkTextBuffer  *buffer;
  const gchar    *button1 = _("Copy bug information");
  const gchar    *button2 = _("Open bug tracker");

  gtk_window_set_role (GTK_WINDOW (dialog), "gimp-critical");

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          button1,     GIMP_CRITICAL_RESPONSE_CLIPBOARD,
                          button2,     GIMP_CRITICAL_RESPONSE_URL,
                          _("_Close"), GTK_RESPONSE_CLOSE,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

  dialog->vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      dialog->vbox, TRUE, TRUE, 0);
  gtk_widget_show (dialog->vbox);

  /* The error label. */
  dialog->label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (dialog->label), GTK_JUSTIFY_CENTER);
  gtk_label_set_selectable (GTK_LABEL (dialog->label), TRUE);
  gtk_box_pack_start (GTK_BOX (dialog->vbox),
                      dialog->label, FALSE, FALSE, 0);

  attrs = pango_attr_list_new ();
  attr  = pango_attr_weight_new (PANGO_WEIGHT_SEMIBOLD);
  pango_attr_list_insert (attrs, attr);
  gtk_label_set_attributes (GTK_LABEL (dialog->label), attrs);
  pango_attr_list_unref (attrs);

  gtk_widget_show (dialog->label);

  /* Generic "report a bug" instructions. */
  text = g_strdup_printf ("%s\n"
                          " \xe2\x80\xa2 %s %s\n"
                          " \xe2\x80\xa2 %s %s\n"
                          " \xe2\x80\xa2 %s \n"
                          " \xe2\x80\xa2 %s \n"
                          " \xe2\x80\xa2 %s \n"
                          " \xe2\x80\xa2 %s\n\n"
                          "%s",
                          _("To help us improve GIMP, you can report the bug with "
                            "these simple steps:"),
                          _("Copy the bug information to clipboard by clicking: "),
                          button1,
                          _("Open our bug tracker in browser by clicking: "),
                          button2,
                          _("Create a login if you don't have one yet."),
                          _("Paste the clipboard text in a new bug report."),
                          _("Add relevant information in English in the bug report "
                            "explaining what you were doing when this error occurred."),
                          _("This error may have left GIMP in an inconsistent state. "
                            "It is advised to save your work and restart GIMP."),
                          _("Note: you can also close the dialog directly but reporting "
                            "bugs is the best way to make your software awesome."));
  widget = gtk_label_new (text);
  g_free (text);
  gtk_label_set_selectable (GTK_LABEL (widget), TRUE);
  gtk_box_pack_start (GTK_BOX (dialog->vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* Bug details for developers. */
  widget = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (dialog->vbox), widget, TRUE, TRUE, 0);
  gtk_widget_show (widget);

  buffer = gtk_text_buffer_new (NULL);
  version = gimp_version (TRUE, FALSE);
  gtk_text_buffer_set_text (buffer, version, -1);
  g_free (version);

  dialog->details = gtk_text_view_new_with_buffer (buffer);
  g_object_unref (buffer);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (dialog->details), FALSE);
  gtk_widget_show (dialog->details);
  gtk_container_add (GTK_CONTAINER (widget), dialog->details);

  dialog->pid     = 0;
  dialog->program = NULL;
}

static void
gimp_critical_dialog_finalize (GObject *object)
{
  GimpCriticalDialog *dialog = GIMP_CRITICAL_DIALOG (object);

  if (dialog->program)
    g_free (dialog->program);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/* XXX This is taken straight from plug-ins/common/web-browser.c
 * This really sucks but this class also needs to be called by
 * tools/gimpdebug.c as a separate process and therefore cannot make use
 * of the PDB. Anyway shouldn't we just move this as a utils function?
 * Why does such basic feature as opening a URL in a cross-platform way
 * need to be a plug-in?
 */
static gboolean
browser_open_url (const gchar  *url,
                  GError      **error)
{
#ifdef G_OS_WIN32

  HINSTANCE hinst = ShellExecute (GetDesktopWindow(),
                                  "open", url, NULL, NULL, SW_SHOW);

  if ((gint) hinst <= 32)
    {
      const gchar *err;

      switch ((gint) hinst)
        {
          case 0 :
            err = _("The operating system is out of memory or resources.");
            break;
          case ERROR_FILE_NOT_FOUND :
            err = _("The specified file was not found.");
            break;
          case ERROR_PATH_NOT_FOUND :
            err = _("The specified path was not found.");
            break;
          case ERROR_BAD_FORMAT :
            err = _("The .exe file is invalid (non-Microsoft Win32 .exe or error in .exe image).");
            break;
          case SE_ERR_ACCESSDENIED :
            err = _("The operating system denied access to the specified file.");
            break;
          case SE_ERR_ASSOCINCOMPLETE :
            err = _("The file name association is incomplete or invalid.");
            break;
          case SE_ERR_DDEBUSY :
            err = _("DDE transaction busy");
            break;
          case SE_ERR_DDEFAIL :
            err = _("The DDE transaction failed.");
            break;
          case SE_ERR_DDETIMEOUT :
            err = _("The DDE transaction timed out.");
            break;
          case SE_ERR_DLLNOTFOUND :
            err = _("The specified DLL was not found.");
            break;
          case SE_ERR_NOASSOC :
            err = _("There is no application associated with the given file name extension.");
            break;
          case SE_ERR_OOM :
            err = _("There was not enough memory to complete the operation.");
            break;
          case SE_ERR_SHARE:
            err = _("A sharing violation occurred.");
            break;
          default :
            err = _("Unknown Microsoft Windows error.");
        }

      g_set_error (error, 0, 0, _("Failed to open '%s': %s"), url, err);

      return FALSE;
    }

  return TRUE;

#elif defined(PLATFORM_OSX)

  NSURL    *ns_url;
  gboolean  retval;

  @autoreleasepool
    {
      ns_url = [NSURL URLWithString: [NSString stringWithUTF8String: url]];
      retval = [[NSWorkspace sharedWorkspace] openURL: ns_url];
    }

  return retval;

#else

  return gtk_show_uri (gdk_screen_get_default (),
                       url,
                       gtk_get_current_event_time(),
                       error);

#endif
}

static void
gimp_critical_dialog_response (GtkDialog *dialog,
                               gint       response_id)
{
  GimpCriticalDialog *critical = GIMP_CRITICAL_DIALOG (dialog);

  switch (response_id)
    {
    case GIMP_CRITICAL_RESPONSE_CLIPBOARD:
        {
          GtkClipboard  *clipboard;

          clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                                     GDK_SELECTION_CLIPBOARD);
          if (clipboard)
            {
              GtkTextBuffer *buffer;
              gchar         *text;
              GtkTextIter    start;
              GtkTextIter    end;

              buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (critical->details));
              gtk_text_buffer_get_iter_at_offset (buffer, &start, 0);
              gtk_text_buffer_get_iter_at_offset (buffer, &end, -1);
              text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
              gtk_clipboard_set_text (clipboard, text, -1);
              g_free (text);
            }
        }
      break;
    case GIMP_CRITICAL_RESPONSE_URL:
        {
          const gchar *url;

          /* XXX Ideally I'd find a way to prefill the bug report
           * through the URL or with POST data. But I could not find
           * any. Anyway since we may soon ditch bugzilla to follow
           * GNOME infrastructure changes, I don't want to waste too
           * much time digging into it.
           */
          url = "https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP";
          browser_open_url (url, NULL);
        }
      break;
    case GIMP_CRITICAL_RESPONSE_RESTART:
        {
          gchar *args[2] = { critical->program , NULL };

#ifndef G_OS_WIN32
          /* It is unneeded to kill the process on Win32. This was run
           * as an async call and the main process should already be
           * dead by now.
           */
          if (critical->pid > 0)
            kill ((pid_t ) critical->pid, SIGINT);
#endif
          if (critical->program)
            g_spawn_async (NULL, args, NULL, G_SPAWN_DEFAULT,
                           NULL, NULL, NULL, NULL);
        }
      /* Fall through. */
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_CLOSE:
    default:
      gtk_widget_destroy (GTK_WIDGET (dialog));
      break;
    }
}

/*  public functions  */

GtkWidget *
gimp_critical_dialog_new (const gchar *title)
{
  g_return_val_if_fail (title != NULL, NULL);

  return g_object_new (GIMP_TYPE_CRITICAL_DIALOG,
                       "title", title,
                       NULL);
}

void
gimp_critical_dialog_add (GtkWidget  *dialog,
                          const gchar         *message,
                          const gchar         *trace,
                          gboolean             is_fatal,
                          const gchar         *program,
                          gint                 pid)
{
  GimpCriticalDialog *critical;
  GtkTextBuffer *buffer;
  GtkTextIter    end;
  gchar         *text;

  if (! GIMP_IS_CRITICAL_DIALOG (dialog) || ! message || ! trace)
    {
      /* This is a bit hackish. We usually should use
       * g_return_if_fail(). But I don't want to end up in a critical
       * recursing loop if our code had bugs. We would crash GIMP with
       * a CRITICAL which would otherwise not have necessarily ended up
       * in a crash.
       */
      return;
    }
  critical = GIMP_CRITICAL_DIALOG (dialog);

  /* The user text, which should be localized. */
  if (is_fatal)
    {
      text = g_strdup_printf (_("GIMP crashed with a fatal error: %s"),
                              message);
    }
  else if (! gtk_label_get_text (GTK_LABEL (critical->label)) ||
           strlen (gtk_label_get_text (GTK_LABEL (critical->label))) == 0)
    {
      /* First critical error. Let's just display it. */
      text = g_strdup_printf (_("GIMP encountered a critical error: %s"),
                              message);
    }
  else
    {
      /* Let's not display all errors. They will be in the bug report
       * part anyway.
       */
      text = g_strdup_printf (_("GIMP encountered several critical errors!"));
    }
  gtk_label_set_text (GTK_LABEL (critical->label),
                      text);
  g_free (text);

  /* The details text is untranslated on purpose. This is the message
   * meant to go to clipboard for the bug report. It has to be in
   * English.
   */
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (critical->details));
  gtk_text_buffer_get_iter_at_offset (buffer, &end, -1);
  text = g_strdup_printf ("\n\n> %s\n\nStack trace:\n%s", message, trace);
  gtk_text_buffer_insert (buffer, &end, text, -1);
  g_free (text);

  /* Finally when encountering a fatal message, propose one more button
   * to restart GIMP.
   */
  if (is_fatal)
    {
      gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                              _("_Restart GIMP"), GIMP_CRITICAL_RESPONSE_RESTART,
                              NULL);
      critical->program = g_strdup (program);
      critical->pid     = pid;
    }
}
