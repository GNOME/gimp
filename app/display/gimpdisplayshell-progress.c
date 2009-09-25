/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimpprogress.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpdisplayshell.h"
#include "gimpdisplayshell-progress.h"
#include "gimpstatusbar.h"


static GimpProgress *
gimp_display_shell_progress_start (GimpProgress *progress,
                                   const gchar  *message,
                                   gboolean      cancelable)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  /* FIXME image window */
  progress = gimp_progress_start (GIMP_PROGRESS (GIMP_IMAGE_WINDOW (shell)->statusbar),
                                  message, cancelable);

  return progress;
}

static void
gimp_display_shell_progress_end (GimpProgress *progress)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  /* FIXME image window */
  gimp_progress_end (GIMP_PROGRESS (GIMP_IMAGE_WINDOW (shell)->statusbar));
}

static gboolean
gimp_display_shell_progress_is_active (GimpProgress *progress)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  /* FIXME image window */
  return gimp_progress_is_active (GIMP_PROGRESS (GIMP_IMAGE_WINDOW (shell)->statusbar));
}

static void
gimp_display_shell_progress_set_text (GimpProgress *progress,
                                      const gchar  *message)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  /* FIXME image window */
  gimp_progress_set_text (GIMP_PROGRESS (GIMP_IMAGE_WINDOW (shell)->statusbar), message);
}

static void
gimp_display_shell_progress_set_value (GimpProgress *progress,
                                       gdouble       percentage)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  /* FIXME image window */
  gimp_progress_set_value (GIMP_PROGRESS (GIMP_IMAGE_WINDOW (shell)->statusbar), percentage);
}

static gdouble
gimp_display_shell_progress_get_value (GimpProgress *progress)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  /* FIXME image window */
  return gimp_progress_get_value (GIMP_PROGRESS (GIMP_IMAGE_WINDOW (shell)->statusbar));
}

static void
gimp_display_shell_progress_pulse (GimpProgress *progress)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);

  /* FIXME image window */
  gimp_progress_pulse (GIMP_PROGRESS (GIMP_IMAGE_WINDOW (shell)->statusbar));
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
      /* FIXME image window */
      if (! gimp_statusbar_get_visible (GIMP_STATUSBAR (GIMP_IMAGE_WINDOW (shell)->statusbar)))
        break;
      else
        /* FIXME image window */
	return gimp_progress_message (GIMP_PROGRESS (GIMP_IMAGE_WINDOW (shell)->statusbar), gimp,
				      severity, domain, message);

    case GIMP_MESSAGE_INFO:
      /* info messages go to the statusbar;
       * if they are not handled there, they are swallowed
       */
      /* FIXME image window */
      gimp_progress_message (GIMP_PROGRESS (GIMP_IMAGE_WINDOW (shell)->statusbar), gimp,
			     severity, domain, message);
      return TRUE;
    }

  return FALSE;
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
