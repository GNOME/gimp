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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * This widget is particular that I want to be able to use it
 * internally but also from an alternate tool (gimp-debug-tool). It
 * means that the implementation must stay as generic glib/GTK+ as
 * possible.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>
#include <gegl.h>

#ifdef PLATFORM_OSX
#import <Cocoa/Cocoa.h>
#endif

#ifdef G_OS_WIN32
#undef DATADIR
#include <windows.h>
#endif

#include "gimpcriticaldialog.h"

#include "gimp-intl.h"
#include "gimp-version.h"


#define GIMP_CRITICAL_RESPONSE_CLIPBOARD 1
#define GIMP_CRITICAL_RESPONSE_URL       2
#define GIMP_CRITICAL_RESPONSE_RESTART   3
#define GIMP_CRITICAL_RESPONSE_DOWNLOAD  4

#define BUTTON1_TEXT _("Copy Bug Information")
#define BUTTON2_TEXT _("Open Bug Tracker")

enum
{
  PROP_0,
  PROP_LAST_VERSION,
  PROP_RELEASE_DATE
};

static void     gimp_critical_dialog_constructed  (GObject      *object);
static void     gimp_critical_dialog_finalize     (GObject      *object);
static void     gimp_critical_dialog_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void     gimp_critical_dialog_response     (GtkDialog    *dialog,
                                                   gint          response_id);

static void     gimp_critical_dialog_copy_info    (GimpCriticalDialog *dialog);
static gboolean browser_open_url                  (GtkWindow    *window,
                                                   const gchar  *url,
                                                   GError      **error);


G_DEFINE_TYPE (GimpCriticalDialog, gimp_critical_dialog, GTK_TYPE_DIALOG)

#define parent_class gimp_critical_dialog_parent_class


static void
gimp_critical_dialog_class_init (GimpCriticalDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->constructed  = gimp_critical_dialog_constructed;
  object_class->finalize     = gimp_critical_dialog_finalize;
  object_class->set_property = gimp_critical_dialog_set_property;

  dialog_class->response = gimp_critical_dialog_response;

  g_object_class_install_property (object_class, PROP_LAST_VERSION,
                                   g_param_spec_string ("last-version",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_RELEASE_DATE,
                                   g_param_spec_string ("release-date",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_critical_dialog_init (GimpCriticalDialog *dialog)
{
  PangoAttrList  *attrs;
  PangoAttribute *attr;

  gtk_window_set_role (GTK_WINDOW (dialog), "gimp-critical");

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

  dialog->main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (dialog->main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      dialog->main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (dialog->main_vbox);

  /* The error label. */
  dialog->top_label = gtk_label_new (NULL);
  gtk_widget_set_halign (dialog->top_label, GTK_ALIGN_START);
  gtk_label_set_ellipsize (GTK_LABEL (dialog->top_label), PANGO_ELLIPSIZE_END);
  gtk_label_set_selectable (GTK_LABEL (dialog->top_label), TRUE);
  gtk_box_pack_start (GTK_BOX (dialog->main_vbox), dialog->top_label,
                      FALSE, FALSE, 0);

  attrs = pango_attr_list_new ();
  attr  = pango_attr_weight_new (PANGO_WEIGHT_SEMIBOLD);
  pango_attr_list_insert (attrs, attr);
  gtk_label_set_attributes (GTK_LABEL (dialog->top_label), attrs);
  pango_attr_list_unref (attrs);

  gtk_widget_show (dialog->top_label);

  dialog->center_label = gtk_label_new (NULL);

  gtk_widget_set_halign (dialog->center_label, GTK_ALIGN_START);
  gtk_label_set_selectable (GTK_LABEL (dialog->center_label), TRUE);
  gtk_box_pack_start (GTK_BOX (dialog->main_vbox), dialog->center_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (dialog->center_label);

  dialog->bottom_label = gtk_label_new (NULL);
  gtk_widget_set_halign (dialog->bottom_label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (dialog->main_vbox), dialog->bottom_label, FALSE, FALSE, 0);

  attrs = pango_attr_list_new ();
  attr  = pango_attr_style_new (PANGO_STYLE_ITALIC);
  pango_attr_list_insert (attrs, attr);
  gtk_label_set_attributes (GTK_LABEL (dialog->bottom_label), attrs);
  pango_attr_list_unref (attrs);
  gtk_widget_show (dialog->bottom_label);

  dialog->pid      = 0;
  dialog->program  = NULL;
}

static void
gimp_critical_dialog_constructed (GObject *object)
{
  GimpCriticalDialog *dialog = GIMP_CRITICAL_DIALOG (object);
  GtkWidget          *scrolled;
  GtkTextBuffer      *buffer;
  gchar              *version;
  gchar              *text;

  /* Bug details for developers. */
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                       GTK_SHADOW_IN);
  gtk_widget_set_size_request (scrolled, -1, 200);

  if (dialog->last_version)
    {
      GtkWidget *expander;
      GtkWidget *vbox;
      GtkWidget *button;

      expander = gtk_expander_new (_("See bug details"));
      gtk_box_pack_start (GTK_BOX (dialog->main_vbox), expander, TRUE, TRUE, 0);
      gtk_widget_show (expander);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
      gtk_container_add (GTK_CONTAINER (expander), vbox);
      gtk_widget_show (vbox);

      gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
      gtk_widget_show (scrolled);

      button = gtk_button_new_with_label (BUTTON1_TEXT);
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (gimp_critical_dialog_copy_info),
                                dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                              _("Go to _Download page"), GIMP_CRITICAL_RESPONSE_DOWNLOAD,
                              _("_Close"),               GTK_RESPONSE_CLOSE,
                              NULL);

      /* Recommend an update. */
      text = g_strdup_printf (_("A new version of GIMP (%s) was released on %s.\n"
                                "It is recommended to update."),
                              dialog->last_version, dialog->release_date);
      gtk_label_set_text (GTK_LABEL (dialog->center_label), text);
      g_free (text);

      text = _("You are running an unsupported version!");
      gtk_label_set_text (GTK_LABEL (dialog->bottom_label), text);
    }
  else
    {
      /* Pack directly (and well visible) the bug details. */
      gtk_box_pack_start (GTK_BOX (dialog->main_vbox), scrolled, TRUE, TRUE, 0);
      gtk_widget_show (scrolled);

      gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                              BUTTON1_TEXT, GIMP_CRITICAL_RESPONSE_CLIPBOARD,
                              BUTTON2_TEXT, GIMP_CRITICAL_RESPONSE_URL,
                              _("_Close"),  GTK_RESPONSE_CLOSE,
                              NULL);

      /* Generic "report a bug" instructions. */
      text = g_strdup_printf ("%s\n"
                              " \xe2\x80\xa2 %s %s\n"
                              " \xe2\x80\xa2 %s %s\n"
                              " \xe2\x80\xa2 %s\n"
                              " \xe2\x80\xa2 %s\n"
                              " \xe2\x80\xa2 %s\n"
                              " \xe2\x80\xa2 %s",
                              _("To help us improve GIMP, you can report the bug with "
                                "these simple steps:"),
                              _("Copy the bug information to the clipboard by clicking: "),
                              BUTTON1_TEXT,
                              _("Open our bug tracker in the browser by clicking: "),
                              BUTTON2_TEXT,
                              _("Create a login if you don't have one yet."),
                              _("Paste the clipboard text in a new bug report."),
                              _("Add relevant information in English in the bug report "
                                "explaining what you were doing when this error occurred."),
                              _("This error may have left GIMP in an inconsistent state. "
                                "It is advised to save your work and restart GIMP."));
      gtk_label_set_text (GTK_LABEL (dialog->center_label), text);
      g_free (text);

      text = _("You can also close the dialog directly but "
               "reporting bugs is the best way to make your "
               "software awesome.");
      gtk_label_set_text (GTK_LABEL (dialog->bottom_label), text);
    }

  buffer = gtk_text_buffer_new (NULL);
  version = gimp_version (TRUE, FALSE);
  text = g_strdup_printf ("<!-- %s -->\n\n\n```\n%s\n```",
                          _("Copy-paste this whole debug data to report to developers"),
                          version);
  gtk_text_buffer_set_text (buffer, text, -1);
  g_free (version);
  g_free (text);

  dialog->details = gtk_text_view_new_with_buffer (buffer);
  g_object_unref (buffer);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (dialog->details), FALSE);
  gtk_widget_show (dialog->details);
  gtk_container_add (GTK_CONTAINER (scrolled), dialog->details);
}

static void
gimp_critical_dialog_finalize (GObject *object)
{
  GimpCriticalDialog *dialog = GIMP_CRITICAL_DIALOG (object);

  if (dialog->program)
    g_free (dialog->program);
  if (dialog->last_version)
    g_free (dialog->last_version);
  if (dialog->release_date)
    g_free (dialog->release_date);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_critical_dialog_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpCriticalDialog *dialog = GIMP_CRITICAL_DIALOG (object);

  switch (property_id)
    {
    case PROP_LAST_VERSION:
      dialog->last_version = g_value_dup_string (value);
      break;
    case PROP_RELEASE_DATE:
      dialog->release_date = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_critical_dialog_copy_info (GimpCriticalDialog *dialog)
{
  GtkClipboard *clipboard;

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (clipboard)
    {
      GtkTextBuffer *buffer;
      gchar         *text;
      GtkTextIter    start;
      GtkTextIter    end;

      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (dialog->details));
      gtk_text_buffer_get_iter_at_offset (buffer, &start, 0);
      gtk_text_buffer_get_iter_at_offset (buffer, &end, -1);
      text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
      gtk_clipboard_set_text (clipboard, text, -1);
      g_free (text);
    }
}

/* XXX This is taken straight from plug-ins/common/web-browser.c
 *
 * This really sucks but this class also needs to be called by
 * tools/gimp-debug-tool.c as a separate process and therefore cannot
 * make use of the PDB. Anyway shouldn't we just move this as a utils
 * function?  Why does such basic feature as opening a URL in a
 * cross-platform way need to be a plug-in?
 */
static gboolean
browser_open_url (GtkWindow    *window,
                  const gchar  *url,
                  GError      **error)
{
#ifdef G_OS_WIN32

  HINSTANCE hinst = ShellExecute (GetDesktopWindow(),
                                  "open", url, NULL, NULL, SW_SHOW);

  if ((intptr_t) hinst <= 32)
    {
      const gchar *err;

      switch ((intptr_t) hinst)
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

  return gtk_show_uri_on_window (window,
                                 url,
                                 GDK_CURRENT_TIME,
                                 error);

#endif
}

static void
gimp_critical_dialog_response (GtkDialog *dialog,
                               gint       response_id)
{
  GimpCriticalDialog *critical = GIMP_CRITICAL_DIALOG (dialog);
  const gchar        *url      = NULL;

  switch (response_id)
    {
    case GIMP_CRITICAL_RESPONSE_CLIPBOARD:
      gimp_critical_dialog_copy_info (critical);
      break;

    case GIMP_CRITICAL_RESPONSE_DOWNLOAD:
#ifdef GIMP_UNSTABLE
      url = "https://www.gimp.org/downloads/devel/";
#else
      url = "https://www.gimp.org/downloads/";
#endif
    case GIMP_CRITICAL_RESPONSE_URL:
      if (url == NULL)
        {
          gchar *temp = g_ascii_strdown (BUG_REPORT_URL, -1);

          /* Only accept custom web links. */
          if (g_str_has_prefix (temp, "http://") ||
              g_str_has_prefix (temp, "https://"))
            url = BUG_REPORT_URL;
          else
            /* XXX Ideally I'd find a way to prefill the bug report
             * through the URL or with POST data. But I could not find
             * any. Anyway since we may soon ditch bugzilla to follow
             * GNOME infrastructure changes, I don't want to waste too
             * much time digging into it.
             */
            url = PACKAGE_BUGREPORT;

          g_free (temp);
        }

      browser_open_url (GTK_WINDOW (dialog), url, NULL);
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
gimp_critical_dialog_new (const gchar *title,
                          const gchar *last_version,
                          gint64       release_timestamp)
{
  GtkWidget *dialog;
  gchar     *date = NULL;

  g_return_val_if_fail (title != NULL, NULL);

  if (release_timestamp > 0)
    {
      GDateTime *datetime;

      datetime = g_date_time_new_from_unix_local (release_timestamp);
      date = g_date_time_format (datetime, "%x");
      g_date_time_unref (datetime);
    }

  dialog = g_object_new (GIMP_TYPE_CRITICAL_DIALOG,
                         "title",        title,
                         "last-version", last_version,
                         "release-date", date,
                         NULL);
  g_free (date);

  return dialog;
}

void
gimp_critical_dialog_add (GtkWidget   *dialog,
                          const gchar *message,
                          const gchar *trace,
                          gboolean     is_fatal,
                          const gchar *program,
                          gint         pid)
{
  GimpCriticalDialog *critical;
  GtkTextBuffer      *buffer;
  GtkTextIter         end;
  gchar              *text;

  if (! GIMP_IS_CRITICAL_DIALOG (dialog) || ! message)
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
  else if (! gtk_label_get_text (GTK_LABEL (critical->top_label)) ||
           strlen (gtk_label_get_text (GTK_LABEL (critical->top_label))) == 0)
    {
      /* First error. Let's just display it. */
      text = g_strdup_printf (_("GIMP encountered an error: %s"),
                              message);
    }
  else
    {
      /* Let's not display all errors. They will be in the bug report
       * part anyway.
       */
      text = g_strdup_printf (_("GIMP encountered several critical errors!"));
    }
  gtk_label_set_text (GTK_LABEL (critical->top_label),
                      text);
  g_free (text);

  if (is_fatal && ! critical->last_version)
    {
      /* Same text as before except that we don't need the last point
       * about saving and restarting since anyway we are crashing and
       * manual saving is not possible anymore (or even advisable since
       * if it fails, one may corrupt files).
       */
      text = g_strdup_printf ("%s\n"
                              " \xe2\x80\xa2 %s \"%s\"\n"
                              " \xe2\x80\xa2 %s \"%s\"\n"
                              " \xe2\x80\xa2 %s\n"
                              " \xe2\x80\xa2 %s\n"
                              " \xe2\x80\xa2 %s",
                              _("To help us improve GIMP, you can report the bug with "
                                "these simple steps:"),
                              _("Copy the bug information to the clipboard by clicking: "),
                              BUTTON1_TEXT,
                              _("Open our bug tracker in the browser by clicking: "),
                              BUTTON2_TEXT,
                              _("Create a login if you don't have one yet."),
                              _("Paste the clipboard text in a new bug report."),
                              _("Add relevant information in English in the bug report "
                                "explaining what you were doing when this error occurred."));
      gtk_label_set_text (GTK_LABEL (critical->center_label), text);
      g_free (text);
    }

  /* The details text is untranslated on purpose. This is the message
   * meant to go to clipboard for the bug report. It has to be in
   * English.
   */
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (critical->details));
  gtk_text_buffer_get_iter_at_offset (buffer, &end, -1);
  if (trace)
    text = g_strdup_printf ("\n> %s\n\nStack trace:\n```\n%s\n```", message, trace);
  else
    text = g_strdup_printf ("\n> %s\n", message);
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
