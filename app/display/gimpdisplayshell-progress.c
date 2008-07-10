/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimpprogress.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpdisplayshell.h"
#include "gimpdisplayshell-progress.h"
#include "gimpdisplayshell-title.h"
#include "gimpstatusbar.h"


/*  Progress is shown in the status-bar. If the image window is iconified,
 *  the progress messages are also shown in the window title so that they
 *  appear in the task bar.
 */

static gboolean gimp_display_shell_is_iconified (GimpDisplayShell *shell);


static GimpProgress *
gimp_display_shell_progress_start (GimpProgress *progress,
                                   const gchar  *message,
                                   gboolean      cancelable)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  progress = gimp_progress_start (GIMP_PROGRESS (shell->statusbar),
                                  message, cancelable);

  if (progress && gimp_display_shell_is_iconified (shell))
    {
      gdk_window_set_title (GTK_WIDGET (shell)->window, message);
    }

  return progress;
}

static void
gimp_display_shell_progress_end (GimpProgress *progress)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  gimp_progress_end (GIMP_PROGRESS (shell->statusbar));

  if (gimp_display_shell_is_iconified (shell))
    gimp_display_shell_title_update (shell);
}

static gboolean
gimp_display_shell_progress_is_active (GimpProgress *progress)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  return gimp_progress_is_active (GIMP_PROGRESS (shell->statusbar));
}

static void
gimp_display_shell_progress_set_text (GimpProgress *progress,
                                      const gchar  *message)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  gimp_progress_set_text (GIMP_PROGRESS (shell->statusbar), message);

  if (gimp_progress_is_active (GIMP_PROGRESS (shell->statusbar)) &&
      gimp_display_shell_is_iconified (shell))
    {
      gdk_window_set_title (GTK_WIDGET (shell)->window, message);
    }
}

static void
gimp_display_shell_progress_set_value (GimpProgress *progress,
                                       gdouble       percentage)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  gimp_progress_set_value (GIMP_PROGRESS (shell->statusbar), percentage);
}

static gdouble
gimp_display_shell_progress_get_value (GimpProgress *progress)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  return gimp_progress_get_value (GIMP_PROGRESS (shell->statusbar));
}

static void
gimp_display_shell_progress_pulse (GimpProgress *progress)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  gimp_progress_pulse (GIMP_PROGRESS (shell->statusbar));
}

static guint32
gimp_display_shell_progress_get_window (GimpProgress *progress)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  return (guint32) gimp_window_get_native (GTK_WINDOW (shell));
}

static gboolean
gimp_display_shell_progress_message (GimpProgress        *progress,
                                     Gimp                *gimp,
                                     GimpMessageSeverity  severity,
                                     const gchar         *domain,
                                     const gchar         *message)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  switch (severity)
    {
    case GIMP_MESSAGE_ERROR:
      /* error messages are never handled here */
      break;

    case GIMP_MESSAGE_WARNING:
      /* warning messages go to the statusbar, if it's visible */
      if (! gimp_statusbar_get_visible (GIMP_STATUSBAR (shell->statusbar)))
        break;
      else
	return gimp_progress_message (GIMP_PROGRESS (shell->statusbar), gimp,
				      severity, domain, message);

    case GIMP_MESSAGE_INFO:
      /* info messages go to the statusbar;
       * if they are not handled there, they are swallowed
       */
      gimp_progress_message (GIMP_PROGRESS (shell->statusbar), gimp,
			     severity, domain, message);
      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_display_shell_is_iconified (GimpDisplayShell *shell)
{
  GtkWidget *widget = GTK_WIDGET (shell);

  return (GTK_WIDGET_DRAWABLE (widget) &&
          gdk_window_get_state (widget->window) == GDK_WINDOW_STATE_ICONIFIED);
}


void
gimp_display_shell_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start      = gimp_display_shell_progress_start;
  iface->end        = gimp_display_shell_progress_end;
  iface->is_active  = gimp_display_shell_progress_is_active;
  iface->set_text   = gimp_display_shell_progress_set_text;
  iface->set_value  = gimp_display_shell_progress_set_value;
  iface->get_value  = gimp_display_shell_progress_get_value;
  iface->pulse      = gimp_display_shell_progress_pulse;
  iface->get_window = gimp_display_shell_progress_get_window;
  iface->message    = gimp_display_shell_progress_message;
}

void
gimp_display_shell_progress_window_state_changed (GimpDisplayShell *shell)
{
  if (! gimp_progress_is_active (GIMP_PROGRESS (shell)))
    return;

  if (gimp_display_shell_is_iconified (shell))
    {
      const gchar *msg = gimp_statusbar_peek (GIMP_STATUSBAR (shell->statusbar),
                                              "progress");
      if (msg)
        {
          gdk_window_set_title (GTK_WIDGET (shell)->window, msg);
          return;
        }
    }

  gimp_display_shell_title_update (shell);
}
