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
#include "gimpimagewindow.h"
#include "gimpstatusbar.h"


/* FIXME: need to store the shell's progress state in the shell itself
 * instead of simply dispatching to the statusbar. Otherwise it's
 * impossible to switch an image window between two shells that both
 * have active progress messages.
 */


static GimpProgress *
gimp_display_shell_progress_get_real_progress (GimpProgress *progress)
{
  GimpDisplayShell *shell    = GIMP_DISPLAY_SHELL (progress);
  GtkWidget        *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  GimpImageWindow  *window   = GIMP_IMAGE_WINDOW (toplevel);

  if (gimp_image_window_get_active_display (window) == shell->display)
    return GIMP_PROGRESS (window->statusbar);
  else
    return NULL;
}

static GimpProgress *
gimp_display_shell_progress_start (GimpProgress *progress,
                                   const gchar  *message,
                                   gboolean      cancelable)
{
  progress = gimp_display_shell_progress_get_real_progress (progress);

  if (progress)
    return gimp_progress_start (progress, message, cancelable);

  return NULL;
}

static void
gimp_display_shell_progress_end (GimpProgress *progress)
{
  progress = gimp_display_shell_progress_get_real_progress (progress);

  if (progress)
    gimp_progress_end (progress);
}

static gboolean
gimp_display_shell_progress_is_active (GimpProgress *progress)
{
  progress = gimp_display_shell_progress_get_real_progress (progress);

  if (progress)
    return gimp_progress_is_active (progress);

  return FALSE;
}

static void
gimp_display_shell_progress_set_text (GimpProgress *progress,
                                      const gchar  *message)
{
  progress = gimp_display_shell_progress_get_real_progress (progress);

  if (progress)
    gimp_progress_set_text (progress, message);
}

static void
gimp_display_shell_progress_set_value (GimpProgress *progress,
                                       gdouble       percentage)
{
  progress = gimp_display_shell_progress_get_real_progress (progress);

  if (progress)
    gimp_progress_set_value (progress, percentage);
}

static gdouble
gimp_display_shell_progress_get_value (GimpProgress *progress)
{
  progress = gimp_display_shell_progress_get_real_progress (progress);

  if (progress)
    return gimp_progress_get_value (progress);

  return 0.0;
}

static void
gimp_display_shell_progress_pulse (GimpProgress *progress)
{
  progress = gimp_display_shell_progress_get_real_progress (progress);

  if (progress)
    gimp_progress_pulse (progress);
}

static guint32
gimp_display_shell_progress_get_window (GimpProgress *progress)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (progress));

  return (guint32) gimp_window_get_native (GTK_WINDOW (toplevel));
}

static gboolean
gimp_display_shell_progress_message (GimpProgress        *progress,
                                     Gimp                *gimp,
                                     GimpMessageSeverity  severity,
                                     const gchar         *domain,
                                     const gchar         *message)
{
  progress = gimp_display_shell_progress_get_real_progress (progress);

  if (progress)
    {
      switch (severity)
        {
        case GIMP_MESSAGE_ERROR:
          /* error messages are never handled here */
          break;

        case GIMP_MESSAGE_WARNING:
          /* warning messages go to the statusbar, if it's visible */
          if (! gimp_statusbar_get_visible (GIMP_STATUSBAR (progress)))
            break;
          else
            return gimp_progress_message (progress, gimp,
                                          severity, domain, message);

        case GIMP_MESSAGE_INFO:
          /* info messages go to the statusbar;
           * if they are not handled there, they are swallowed
           */
          gimp_progress_message (progress, gimp,
                                 severity, domain, message);
          return TRUE;
        }
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
