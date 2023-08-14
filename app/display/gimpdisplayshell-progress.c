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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimpprogress.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpdisplayshell.h"
#include "gimpdisplayshell-progress.h"
#include "gimpstatusbar.h"


static GimpProgress *
gimp_display_shell_progress_start (GimpProgress *progress,
                                   gboolean      cancellable,
                                   const gchar  *message)
{
  GimpDisplayShell *shell     = GIMP_DISPLAY_SHELL (progress);
  GimpStatusbar    *statusbar = gimp_display_shell_get_statusbar (shell);

  return gimp_progress_start (GIMP_PROGRESS (statusbar), cancellable,
                              "%s", message);
}

static void
gimp_display_shell_progress_end (GimpProgress *progress)
{
  GimpDisplayShell *shell     = GIMP_DISPLAY_SHELL (progress);
  GimpStatusbar    *statusbar = gimp_display_shell_get_statusbar (shell);

  gimp_progress_end (GIMP_PROGRESS (statusbar));
}

static gboolean
gimp_display_shell_progress_is_active (GimpProgress *progress)
{
  GimpDisplayShell *shell     = GIMP_DISPLAY_SHELL (progress);
  GimpStatusbar    *statusbar = gimp_display_shell_get_statusbar (shell);

  return gimp_progress_is_active (GIMP_PROGRESS (statusbar));
}

static void
gimp_display_shell_progress_set_text (GimpProgress *progress,
                                      const gchar  *message)
{
  GimpDisplayShell *shell     = GIMP_DISPLAY_SHELL (progress);
  GimpStatusbar    *statusbar = gimp_display_shell_get_statusbar (shell);

  gimp_progress_set_text_literal (GIMP_PROGRESS (statusbar), message);
}

static void
gimp_display_shell_progress_set_value (GimpProgress *progress,
                                       gdouble       percentage)
{
  GimpDisplayShell *shell     = GIMP_DISPLAY_SHELL (progress);
  GimpStatusbar    *statusbar = gimp_display_shell_get_statusbar (shell);

  gimp_progress_set_value (GIMP_PROGRESS (statusbar), percentage);
}

static gdouble
gimp_display_shell_progress_get_value (GimpProgress *progress)
{
  GimpDisplayShell *shell     = GIMP_DISPLAY_SHELL (progress);
  GimpStatusbar    *statusbar = gimp_display_shell_get_statusbar (shell);

  return gimp_progress_get_value (GIMP_PROGRESS (statusbar));
}

static void
gimp_display_shell_progress_pulse (GimpProgress *progress)
{
  GimpDisplayShell *shell     = GIMP_DISPLAY_SHELL (progress);
  GimpStatusbar    *statusbar = gimp_display_shell_get_statusbar (shell);

  gimp_progress_pulse (GIMP_PROGRESS (statusbar));
}

static GBytes *
gimp_display_shell_progress_get_window_id (GimpProgress *progress)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress);
  GBytes           *handle   = NULL;

  if (shell->window_handle)
    handle = g_bytes_ref (shell->window_handle);

  return handle;
}

static gboolean
gimp_display_shell_progress_message (GimpProgress        *progress,
                                     Gimp                *gimp,
                                     GimpMessageSeverity  severity,
                                     const gchar         *domain,
                                     const gchar         *message)
{
  GimpDisplayShell *shell     = GIMP_DISPLAY_SHELL (progress);
  GimpStatusbar    *statusbar = gimp_display_shell_get_statusbar (shell);

  switch (severity)
    {
    case GIMP_MESSAGE_ERROR:
    case GIMP_MESSAGE_BUG_WARNING:
    case GIMP_MESSAGE_BUG_CRITICAL:
      /* error messages are never handled here */
      break;

    case GIMP_MESSAGE_WARNING:
      /* warning messages go to the statusbar, if it's visible */
      if (! gimp_statusbar_get_visible (statusbar))
        break;
      else
        return gimp_progress_message (GIMP_PROGRESS (statusbar), gimp,
                                      severity, domain, message);

    case GIMP_MESSAGE_INFO:
      /* info messages go to the statusbar;
       * if they are not handled there, they are swallowed
       */
      gimp_progress_message (GIMP_PROGRESS (statusbar), gimp,
                             severity, domain, message);
      return TRUE;
    }

  return FALSE;
}

void
gimp_display_shell_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start         = gimp_display_shell_progress_start;
  iface->end           = gimp_display_shell_progress_end;
  iface->is_active     = gimp_display_shell_progress_is_active;
  iface->set_text      = gimp_display_shell_progress_set_text;
  iface->set_value     = gimp_display_shell_progress_set_value;
  iface->get_value     = gimp_display_shell_progress_get_value;
  iface->pulse         = gimp_display_shell_progress_pulse;
  iface->get_window_id = gimp_display_shell_progress_get_window_id;
  iface->message       = gimp_display_shell_progress_message;
}
