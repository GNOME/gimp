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

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "core/ligmaprogress.h"

#include "widgets/ligmawidgets-utils.h"

#include "ligmadisplayshell.h"
#include "ligmadisplayshell-progress.h"
#include "ligmastatusbar.h"


static LigmaProgress *
ligma_display_shell_progress_start (LigmaProgress *progress,
                                   gboolean      cancellable,
                                   const gchar  *message)
{
  LigmaDisplayShell *shell     = LIGMA_DISPLAY_SHELL (progress);
  LigmaStatusbar    *statusbar = ligma_display_shell_get_statusbar (shell);

  return ligma_progress_start (LIGMA_PROGRESS (statusbar), cancellable,
                              "%s", message);
}

static void
ligma_display_shell_progress_end (LigmaProgress *progress)
{
  LigmaDisplayShell *shell     = LIGMA_DISPLAY_SHELL (progress);
  LigmaStatusbar    *statusbar = ligma_display_shell_get_statusbar (shell);

  ligma_progress_end (LIGMA_PROGRESS (statusbar));
}

static gboolean
ligma_display_shell_progress_is_active (LigmaProgress *progress)
{
  LigmaDisplayShell *shell     = LIGMA_DISPLAY_SHELL (progress);
  LigmaStatusbar    *statusbar = ligma_display_shell_get_statusbar (shell);

  return ligma_progress_is_active (LIGMA_PROGRESS (statusbar));
}

static void
ligma_display_shell_progress_set_text (LigmaProgress *progress,
                                      const gchar  *message)
{
  LigmaDisplayShell *shell     = LIGMA_DISPLAY_SHELL (progress);
  LigmaStatusbar    *statusbar = ligma_display_shell_get_statusbar (shell);

  ligma_progress_set_text_literal (LIGMA_PROGRESS (statusbar), message);
}

static void
ligma_display_shell_progress_set_value (LigmaProgress *progress,
                                       gdouble       percentage)
{
  LigmaDisplayShell *shell     = LIGMA_DISPLAY_SHELL (progress);
  LigmaStatusbar    *statusbar = ligma_display_shell_get_statusbar (shell);

  ligma_progress_set_value (LIGMA_PROGRESS (statusbar), percentage);
}

static gdouble
ligma_display_shell_progress_get_value (LigmaProgress *progress)
{
  LigmaDisplayShell *shell     = LIGMA_DISPLAY_SHELL (progress);
  LigmaStatusbar    *statusbar = ligma_display_shell_get_statusbar (shell);

  return ligma_progress_get_value (LIGMA_PROGRESS (statusbar));
}

static void
ligma_display_shell_progress_pulse (LigmaProgress *progress)
{
  LigmaDisplayShell *shell     = LIGMA_DISPLAY_SHELL (progress);
  LigmaStatusbar    *statusbar = ligma_display_shell_get_statusbar (shell);

  ligma_progress_pulse (LIGMA_PROGRESS (statusbar));
}

static guint32
ligma_display_shell_progress_get_window_id (LigmaProgress *progress)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (progress));

  if (GTK_IS_WINDOW (toplevel))
    return ligma_window_get_native_id (GTK_WINDOW (toplevel));

  return 0;
}

static gboolean
ligma_display_shell_progress_message (LigmaProgress        *progress,
                                     Ligma                *ligma,
                                     LigmaMessageSeverity  severity,
                                     const gchar         *domain,
                                     const gchar         *message)
{
  LigmaDisplayShell *shell     = LIGMA_DISPLAY_SHELL (progress);
  LigmaStatusbar    *statusbar = ligma_display_shell_get_statusbar (shell);

  switch (severity)
    {
    case LIGMA_MESSAGE_ERROR:
    case LIGMA_MESSAGE_BUG_WARNING:
    case LIGMA_MESSAGE_BUG_CRITICAL:
      /* error messages are never handled here */
      break;

    case LIGMA_MESSAGE_WARNING:
      /* warning messages go to the statusbar, if it's visible */
      if (! ligma_statusbar_get_visible (statusbar))
        break;
      else
        return ligma_progress_message (LIGMA_PROGRESS (statusbar), ligma,
                                      severity, domain, message);

    case LIGMA_MESSAGE_INFO:
      /* info messages go to the statusbar;
       * if they are not handled there, they are swallowed
       */
      ligma_progress_message (LIGMA_PROGRESS (statusbar), ligma,
                             severity, domain, message);
      return TRUE;
    }

  return FALSE;
}

void
ligma_display_shell_progress_iface_init (LigmaProgressInterface *iface)
{
  iface->start         = ligma_display_shell_progress_start;
  iface->end           = ligma_display_shell_progress_end;
  iface->is_active     = ligma_display_shell_progress_is_active;
  iface->set_text      = ligma_display_shell_progress_set_text;
  iface->set_value     = ligma_display_shell_progress_set_value;
  iface->get_value     = ligma_display_shell_progress_get_value;
  iface->pulse         = ligma_display_shell_progress_pulse;
  iface->get_window_id = ligma_display_shell_progress_get_window_id;
  iface->message       = ligma_display_shell_progress_message;
}
