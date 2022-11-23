/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "gui-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmaprogress.h"

#include "plug-in/ligmaplugin.h"

#include "widgets/ligmacriticaldialog.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadockable.h"
#include "widgets/ligmaerrorconsole.h"
#include "widgets/ligmaerrordialog.h"
#include "widgets/ligmaprogressdialog.h"
#include "widgets/ligmasessioninfo.h"
#include "widgets/ligmawidgets-utils.h"
#include "widgets/ligmawindowstrategy.h"

#include "about.h"

#include "gui-message.h"

#include "ligma-intl.h"


#define MAX_TRACES 3
#define MAX_ERRORS 10


typedef struct
{
  Ligma                *ligma;
  gchar               *domain;
  gchar               *message;
  gchar               *trace;
  GObject             *handler;
  LigmaMessageSeverity  severity;
} LigmaLogMessageData;


static gboolean  gui_message_idle           (gpointer             user_data);
static gboolean  gui_message_error_console  (Ligma                *ligma,
                                             LigmaMessageSeverity  severity,
                                             const gchar         *domain,
                                             const gchar         *message);
static gboolean  gui_message_error_dialog   (Ligma                *ligma,
                                             GObject             *handler,
                                             LigmaMessageSeverity  severity,
                                             const gchar         *domain,
                                             const gchar         *message,
                                             const gchar         *trace);
static void      gui_message_console        (LigmaMessageSeverity  severity,
                                             const gchar         *domain,
                                             const gchar         *message);
static gchar *   gui_message_format         (LigmaMessageSeverity  severity,
                                             const gchar         *domain,
                                             const gchar         *message);

static GtkWidget * global_error_dialog      (void);
static GtkWidget * global_critical_dialog   (void);

static void        gui_message_reset_errors (GObject             *object,
                                             gpointer             user_data);


static GMutex mutex;
static gint   n_traces = 0;
static gint   n_errors = 0;


void
gui_message (Ligma                *ligma,
             GObject             *handler,
             LigmaMessageSeverity  severity,
             const gchar         *domain,
             const gchar         *message)
{
  gchar    *trace     = NULL;
  gboolean  gen_trace = FALSE;

  switch (ligma->message_handler)
    {
    case LIGMA_ERROR_CONSOLE:
      if (gui_message_error_console (ligma, severity, domain, message))
        return;

      ligma->message_handler = LIGMA_MESSAGE_BOX;
      /*  fallthru  */

    case LIGMA_MESSAGE_BOX:
      if (severity >= LIGMA_MESSAGE_BUG_WARNING)
        {
          g_mutex_lock (&mutex);
          /* Trace creation can be time consuming so don't block the
           * mutex for too long and only increment and set a boolean
           * here.
           */
          if (n_traces < MAX_TRACES)
            {
              gen_trace = TRUE;
              n_traces++;
            }
          g_mutex_unlock (&mutex);
        }

      if (gen_trace)
        {
          /* We need to create the trace here because for multi-thread
           * errors (i.e. non-LIGMA ones), the backtrace after a idle
           * function will simply be useless. It needs to happen in the
           * buggy thread to be meaningful.
           */
          ligma_stack_trace_print (NULL, NULL, &trace);
        }

      if (g_strcmp0 (LIGMA_ACRONYM, domain) != 0)
        {
          /* Handle non-LIGMA messages in a multi-thread safe way,
           * because we can't know for sure whether the log message may
           * not have been called from a thread other than the main one.
           */
          LigmaLogMessageData *data;

          data = g_new0 (LigmaLogMessageData, 1);
          data->ligma     = ligma;
          data->domain   = g_strdup (domain);
          data->message  = g_strdup (message);
          data->trace    = trace;
          data->handler  = handler? g_object_ref (handler) : NULL;
          data->severity = severity;

          gdk_threads_add_idle_full (G_PRIORITY_DEFAULT_IDLE,
                                     gui_message_idle,
                                     data, g_free);
          return;
        }
      if (gui_message_error_dialog (ligma, handler, severity,
                                    domain, message, trace))
        break;

      ligma->message_handler = LIGMA_CONSOLE;
      /*  fallthru  */

    case LIGMA_CONSOLE:
      gui_message_console (severity, domain, message);
      break;
    }
  if (trace)
    g_free (trace);
}

static gboolean
gui_message_idle (gpointer user_data)
{
  LigmaLogMessageData *data = (LigmaLogMessageData *) user_data;

  if (! gui_message_error_dialog (data->ligma,
                                  data->handler,
                                  data->severity,
                                  data->domain,
                                  data->message,
                                  data->trace))
    {
      gui_message_console (data->severity,
                           data->domain,
                           data->message);
    }
  g_free (data->domain);
  g_free (data->message);
  if (data->trace)
    g_free (data->trace);
  if (data->handler)
    g_object_unref (data->handler);

  return FALSE;
}

static gboolean
gui_message_error_console (Ligma                *ligma,
                           LigmaMessageSeverity  severity,
                           const gchar         *domain,
                           const gchar         *message)
{
  GtkWidget *dockable;

  dockable = ligma_dialog_factory_find_widget (ligma_dialog_factory_get_singleton (),
                                              "ligma-error-console");

  /* avoid raising the error console for unhighlighted messages */
  if (dockable)
    {
      GtkWidget *child = gtk_bin_get_child (GTK_BIN (dockable));

      if (LIGMA_ERROR_CONSOLE (child)->highlight[severity])
        dockable = NULL;
    }

  if (! dockable)
    {
      GdkMonitor *monitor = ligma_get_monitor_at_pointer ();

      dockable =
        ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (ligma)),
                                                   ligma,
                                                   ligma_dialog_factory_get_singleton (),
                                                   monitor,
                                                   "ligma-error-console");
    }

  if (dockable)
    {
      GtkWidget *child = gtk_bin_get_child (GTK_BIN (dockable));

      ligma_error_console_add (LIGMA_ERROR_CONSOLE (child),
                              severity, domain, message);

      return TRUE;
    }

  return FALSE;
}

static void
progress_error_dialog_unset (LigmaProgress *progress)
{
  g_object_set_data (G_OBJECT (progress), "ligma-error-dialog", NULL);
}

static GtkWidget *
progress_error_dialog (LigmaProgress *progress)
{
  GtkWidget *dialog;

  g_return_val_if_fail (LIGMA_IS_PROGRESS (progress), NULL);

  dialog = g_object_get_data (G_OBJECT (progress), "ligma-error-dialog");

  if (! dialog)
    {
      dialog = ligma_error_dialog_new (_("LIGMA Message"));

      g_object_set_data (G_OBJECT (progress), "ligma-error-dialog", dialog);

      g_signal_connect_object (dialog, "destroy",
                               G_CALLBACK (progress_error_dialog_unset),
                               progress, G_CONNECT_SWAPPED);

      if (GTK_IS_WIDGET (progress))
        {
          GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (progress));

          if (GTK_IS_WINDOW (toplevel))
            gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                          GTK_WINDOW (toplevel));
        }
      else
        {
          guint32 window_id = ligma_progress_get_window_id (progress);

          if (window_id)
            ligma_window_set_transient_for (GTK_WINDOW (dialog), window_id);
        }
    }

  return dialog;
}

static gboolean
gui_message_error_dialog (Ligma                *ligma,
                          GObject             *handler,
                          LigmaMessageSeverity  severity,
                          const gchar         *domain,
                          const gchar         *message,
                          const gchar         *trace)
{
  GtkWidget      *dialog;
  GtkMessageType  type   = GTK_MESSAGE_ERROR;

  switch (severity)
    {
    case LIGMA_MESSAGE_INFO:
      type = GTK_MESSAGE_INFO;
      break;
    case LIGMA_MESSAGE_WARNING:
      type = GTK_MESSAGE_WARNING;
      break;
    case LIGMA_MESSAGE_ERROR:
      type = GTK_MESSAGE_ERROR;
      break;
    case LIGMA_MESSAGE_BUG_WARNING:
    case LIGMA_MESSAGE_BUG_CRITICAL:
      type = GTK_MESSAGE_OTHER;
      break;
    }

  if (severity >= LIGMA_MESSAGE_BUG_WARNING)
    {
      /* Process differently programming errors.
       * The reason is that we will generate traces, which will take
       * significant place, and cannot be processed as a progress
       * message or in the global dialog. It will require its own
       * dedicated dialog which will encourage people to report the bug.
       */
      gboolean gui_error = FALSE;

      g_mutex_lock (&mutex);
      if (n_errors < MAX_ERRORS)
        {
          gui_error = TRUE;
          n_errors++;
        }
      g_mutex_unlock (&mutex);

      if (gui_error || trace)
        {
          gchar *text;

          dialog = global_critical_dialog ();

          text = gui_message_format (severity, domain, message);
          ligma_critical_dialog_add (dialog, text, trace, FALSE, NULL, 0);

          gtk_widget_show (dialog);

          g_free (text);

          return TRUE;
        }
      else
        {
          const gchar *reason = "Message";

          ligma_enum_get_value (LIGMA_TYPE_MESSAGE_SEVERITY, severity,
                               NULL, NULL, &reason, NULL);

          /* Since we overridden glib default's WARNING and CRITICAL
           * handler, if we decide not to handle this error in the end,
           * let's just print it in terminal in a similar fashion as
           * glib's default handler (though without the fancy terminal
           * colors right now).
           */
          g_printerr ("%s-%s: %s\n", domain, reason, message);

          return TRUE;
        }
    }
  else if (LIGMA_IS_PROGRESS (handler))
    {
      /* If there's already an error dialog associated with this
       * progress, then continue without trying ligma_progress_message().
       */
      if (! g_object_get_data (handler, "ligma-error-dialog") &&
          ligma_progress_message (LIGMA_PROGRESS (handler), ligma,
                                 severity, domain, message))
        {
          return TRUE;
        }
    }
  else if (GTK_IS_WIDGET (handler))
    {
      GtkWidget *parent = GTK_WIDGET (handler);

      dialog =
        gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (parent)),
                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                type, GTK_BUTTONS_OK,
                                "%s", message);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (gtk_widget_destroy),
                        NULL);

      gtk_widget_show (dialog);

      return TRUE;
    }

  if (LIGMA_IS_PROGRESS (handler) && ! LIGMA_IS_PROGRESS_DIALOG (handler))
    dialog = progress_error_dialog (LIGMA_PROGRESS (handler));
  else
    dialog = global_error_dialog ();

  if (dialog)
    {
      gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
      ligma_error_dialog_add (LIGMA_ERROR_DIALOG (dialog),
                             ligma_get_message_icon_name (severity),
                             domain, message);
      gtk_window_present (GTK_WINDOW (dialog));

      return TRUE;
    }

  return FALSE;
}

static void
gui_message_console (LigmaMessageSeverity  severity,
                     const gchar         *domain,
                     const gchar         *message)
{
  gchar *formatted_message;

  formatted_message = gui_message_format (severity, domain, message);
  g_printerr ("%s\n\n", formatted_message);
  g_free (formatted_message);
}

static gchar *
gui_message_format (LigmaMessageSeverity  severity,
                    const gchar         *domain,
                    const gchar         *message)
{
  const gchar *desc = "Message";
  gchar       *formatted_message;

  ligma_enum_get_value (LIGMA_TYPE_MESSAGE_SEVERITY, severity,
                       NULL, NULL, &desc, NULL);

  formatted_message = g_strdup_printf ("%s-%s: %s", domain, desc, message);

  return formatted_message;
}

static GtkWidget *
global_error_dialog (void)
{
  GdkMonitor *monitor = ligma_get_monitor_at_pointer ();

  return ligma_dialog_factory_dialog_new (ligma_dialog_factory_get_singleton (),
                                         monitor,
                                         NULL /*ui_manager*/,
                                         NULL,
                                         "ligma-error-dialog", -1,
                                         FALSE);
}

static GtkWidget *
global_critical_dialog (void)
{
  GdkMonitor *monitor = ligma_get_monitor_at_pointer ();
  GtkWidget  *dialog;

  dialog = ligma_dialog_factory_dialog_new (ligma_dialog_factory_get_singleton (),
                                           monitor,
                                           NULL /*ui_manager*/,
                                           NULL,
                                           "ligma-critical-dialog", -1,
                                           FALSE);
  g_signal_handlers_disconnect_by_func (dialog,
                                        gui_message_reset_errors,
                                        NULL);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gui_message_reset_errors),
                    NULL);
  return dialog;
}

static void
gui_message_reset_errors (GObject  *object,
                          gpointer  user_data)
{
  g_mutex_lock (&mutex);
  n_errors = 0;
  n_traces = 0;
  g_mutex_unlock (&mutex);
}
