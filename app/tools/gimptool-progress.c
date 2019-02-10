/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptool-progress.c
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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
#include <gdk/gdkkeysyms.h>

#include "tools-types.h"

#include "core/gimpprogress.h"

#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasprogress.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-items.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimptool.h"
#include "gimptool-progress.h"


/*  local function prototypes  */

static GimpProgress * gimp_tool_progress_start     (GimpProgress        *progress,
                                                    gboolean             cancelable,
                                                    const gchar         *message);
static void           gimp_tool_progress_end       (GimpProgress        *progress);
static gboolean       gimp_tool_progress_is_active (GimpProgress        *progress);
static void           gimp_tool_progress_set_text  (GimpProgress        *progress,
                                                    const gchar         *message);
static void           gimp_tool_progress_set_value (GimpProgress        *progress,
                                                    gdouble              percentage);
static gdouble        gimp_tool_progress_get_value (GimpProgress        *progress);
static void           gimp_tool_progress_pulse     (GimpProgress        *progress);
static gboolean       gimp_tool_progress_message   (GimpProgress        *progress,
                                                    Gimp                *gimp,
                                                    GimpMessageSeverity  severity,
                                                    const gchar         *domain,
                                                    const gchar         *message);


/*  public functions  */

void
gimp_tool_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start     = gimp_tool_progress_start;
  iface->end       = gimp_tool_progress_end;
  iface->is_active = gimp_tool_progress_is_active;
  iface->set_text  = gimp_tool_progress_set_text;
  iface->set_value = gimp_tool_progress_set_value;
  iface->get_value = gimp_tool_progress_get_value;
  iface->pulse     = gimp_tool_progress_pulse;
  iface->message   = gimp_tool_progress_message;
}


/*  private functions  */

static gboolean
gimp_tool_progress_button_press (GtkWidget            *widget,
                                 const GdkEventButton *bevent,
                                 GimpTool             *tool)
{
  if (tool->progress_cancelable          &&
      bevent->type   == GDK_BUTTON_PRESS &&
      bevent->button == 1)
    {
      GtkWidget        *event_widget;
      GimpDisplayShell *shell;

      event_widget = gtk_get_event_widget ((GdkEvent *) bevent);
      shell        = gimp_display_get_shell (tool->progress_display);

      if (shell->canvas == event_widget)
        {
          gint x, y;

          gimp_display_shell_unzoom_xy (shell, bevent->x, bevent->y,
                                        &x, &y, FALSE);

          if (gimp_canvas_item_hit (tool->progress, x, y))
            {
              gimp_progress_cancel (GIMP_PROGRESS (tool));
            }
        }
    }

  return TRUE;
}

static gboolean
gimp_tool_progress_key_press (GtkWidget         *widget,
                              const GdkEventKey *kevent,
                              GimpTool          *tool)
{
  if (tool->progress_cancelable &&
      kevent->keyval == GDK_KEY_Escape)
    {
      gimp_progress_cancel (GIMP_PROGRESS (tool));
    }

  return TRUE;
}

static GimpProgress *
gimp_tool_progress_start (GimpProgress *progress,
                          gboolean      cancelable,
                          const gchar  *message)
{
  GimpTool         *tool = GIMP_TOOL (progress);
  GimpDisplayShell *shell;
  gint              x, y;

  g_return_val_if_fail (GIMP_IS_DISPLAY (tool->display), NULL);
  g_return_val_if_fail (tool->progress == NULL, NULL);

  shell = gimp_display_get_shell (tool->display);

  x = shell->disp_width  / 2;
  y = shell->disp_height / 2;

  gimp_display_shell_unzoom_xy (shell, x, y, &x, &y, FALSE);

  tool->progress = gimp_canvas_progress_new (shell,
                                             GIMP_HANDLE_ANCHOR_CENTER,
                                             x, y);
  gimp_display_shell_add_unrotated_item (shell, tool->progress);
  g_object_unref (tool->progress);

  gimp_progress_start (GIMP_PROGRESS (tool->progress), FALSE,
                       "%s", message);

  tool->progress_display = tool->display;

  tool->progress_grab_widget = gtk_invisible_new ();
  gtk_widget_show (tool->progress_grab_widget);
  gtk_grab_add (tool->progress_grab_widget);

  g_signal_connect (tool->progress_grab_widget, "button-press-event",
                    G_CALLBACK (gimp_tool_progress_button_press),
                    tool);
  g_signal_connect (tool->progress_grab_widget, "key-press-event",
                    G_CALLBACK (gimp_tool_progress_key_press),
                    tool);

  tool->progress_cancelable = cancelable;

  return progress;
}

static void
gimp_tool_progress_end (GimpProgress *progress)
{
  GimpTool *tool = GIMP_TOOL (progress);

  if (tool->progress)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (tool->progress_display);

      gimp_progress_end (GIMP_PROGRESS (tool->progress));
      gimp_display_shell_remove_unrotated_item (shell, tool->progress);

      gtk_grab_remove (tool->progress_grab_widget);
      gtk_widget_destroy (tool->progress_grab_widget);

      tool->progress             = NULL;
      tool->progress_display     = NULL;
      tool->progress_grab_widget = NULL;
      tool->progress_cancelable  = FALSE;
    }
}

static gboolean
gimp_tool_progress_is_active (GimpProgress *progress)
{
  GimpTool *tool = GIMP_TOOL (progress);

  return tool->progress != NULL;
}

static void
gimp_tool_progress_set_text (GimpProgress *progress,
                             const gchar  *message)
{
  GimpTool *tool = GIMP_TOOL (progress);

  if (tool->progress)
    gimp_progress_set_text_literal (GIMP_PROGRESS (tool->progress), message);
}

static void
gimp_tool_progress_set_value (GimpProgress *progress,
                              gdouble       percentage)
{
  GimpTool *tool = GIMP_TOOL (progress);

  if (tool->progress)
    gimp_progress_set_value (GIMP_PROGRESS (tool->progress), percentage);
}

static gdouble
gimp_tool_progress_get_value (GimpProgress *progress)
{
  GimpTool *tool = GIMP_TOOL (progress);

  if (tool->progress)
    return gimp_progress_get_value (GIMP_PROGRESS (tool->progress));

  return 0.0;
}

static void
gimp_tool_progress_pulse (GimpProgress *progress)
{
}

static gboolean
gimp_tool_progress_message (GimpProgress        *progress,
                            Gimp                *gimp,
                            GimpMessageSeverity  severity,
                            const gchar         *domain,
                            const gchar         *message)
{
  return FALSE;
}
