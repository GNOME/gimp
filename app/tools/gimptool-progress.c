/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatool-progress.c
 * Copyright (C) 2011 Michael Natterer <mitch@ligma.org>
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

#include "core/ligmaprogress.h"

#include "widgets/ligmawidgets-utils.h"

#include "display/ligmacanvasprogress.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-items.h"
#include "display/ligmadisplayshell-transform.h"

#include "ligmatool.h"
#include "ligmatool-progress.h"


/*  local function prototypes  */

static LigmaProgress * ligma_tool_progress_start     (LigmaProgress        *progress,
                                                    gboolean             cancelable,
                                                    const gchar         *message);
static void           ligma_tool_progress_end       (LigmaProgress        *progress);
static gboolean       ligma_tool_progress_is_active (LigmaProgress        *progress);
static void           ligma_tool_progress_set_text  (LigmaProgress        *progress,
                                                    const gchar         *message);
static void           ligma_tool_progress_set_value (LigmaProgress        *progress,
                                                    gdouble              percentage);
static gdouble        ligma_tool_progress_get_value (LigmaProgress        *progress);
static void           ligma_tool_progress_pulse     (LigmaProgress        *progress);
static gboolean       ligma_tool_progress_message   (LigmaProgress        *progress,
                                                    Ligma                *ligma,
                                                    LigmaMessageSeverity  severity,
                                                    const gchar         *domain,
                                                    const gchar         *message);


/*  public functions  */

void
ligma_tool_progress_iface_init (LigmaProgressInterface *iface)
{
  iface->start     = ligma_tool_progress_start;
  iface->end       = ligma_tool_progress_end;
  iface->is_active = ligma_tool_progress_is_active;
  iface->set_text  = ligma_tool_progress_set_text;
  iface->set_value = ligma_tool_progress_set_value;
  iface->get_value = ligma_tool_progress_get_value;
  iface->pulse     = ligma_tool_progress_pulse;
  iface->message   = ligma_tool_progress_message;
}


/*  private functions  */

static gboolean
ligma_tool_progress_button_press (GtkWidget            *widget,
                                 const GdkEventButton *bevent,
                                 LigmaTool             *tool)
{
  if (tool->progress_cancelable          &&
      bevent->type   == GDK_BUTTON_PRESS &&
      bevent->button == 1)
    {
      GtkWidget        *event_widget;
      LigmaDisplayShell *shell;

      event_widget = gtk_get_event_widget ((GdkEvent *) bevent);
      shell        = ligma_display_get_shell (tool->progress_display);

      if (shell->canvas == event_widget)
        {
          gint x, y;

          ligma_display_shell_unzoom_xy (shell, bevent->x, bevent->y,
                                        &x, &y, FALSE);

          if (ligma_canvas_item_hit (tool->progress, x, y))
            {
              ligma_progress_cancel (LIGMA_PROGRESS (tool));
            }
        }
    }

  return TRUE;
}

static gboolean
ligma_tool_progress_key_press (GtkWidget         *widget,
                              const GdkEventKey *kevent,
                              LigmaTool          *tool)
{
  if (tool->progress_cancelable &&
      kevent->keyval == GDK_KEY_Escape)
    {
      ligma_progress_cancel (LIGMA_PROGRESS (tool));
    }

  return TRUE;
}

static LigmaProgress *
ligma_tool_progress_start (LigmaProgress *progress,
                          gboolean      cancelable,
                          const gchar  *message)
{
  LigmaTool         *tool = LIGMA_TOOL (progress);
  LigmaDisplayShell *shell;
  gint              x, y;

  g_return_val_if_fail (LIGMA_IS_DISPLAY (tool->display), NULL);
  g_return_val_if_fail (tool->progress == NULL, NULL);

  shell = ligma_display_get_shell (tool->display);

  x = shell->disp_width  / 2;
  y = shell->disp_height / 2;

  ligma_display_shell_unzoom_xy (shell, x, y, &x, &y, FALSE);

  tool->progress = ligma_canvas_progress_new (shell,
                                             LIGMA_HANDLE_ANCHOR_CENTER,
                                             x, y);
  ligma_display_shell_add_unrotated_item (shell, tool->progress);
  g_object_unref (tool->progress);

  ligma_progress_start (LIGMA_PROGRESS (tool->progress), FALSE,
                       "%s", message);

  tool->progress_display = tool->display;

  tool->progress_grab_widget = gtk_invisible_new ();
  gtk_widget_show (tool->progress_grab_widget);
  gtk_grab_add (tool->progress_grab_widget);

  g_signal_connect (tool->progress_grab_widget, "button-press-event",
                    G_CALLBACK (ligma_tool_progress_button_press),
                    tool);
  g_signal_connect (tool->progress_grab_widget, "key-press-event",
                    G_CALLBACK (ligma_tool_progress_key_press),
                    tool);

  tool->progress_cancelable = cancelable;

  return progress;
}

static void
ligma_tool_progress_end (LigmaProgress *progress)
{
  LigmaTool *tool = LIGMA_TOOL (progress);

  if (tool->progress)
    {
      LigmaDisplayShell *shell = ligma_display_get_shell (tool->progress_display);

      ligma_progress_end (LIGMA_PROGRESS (tool->progress));
      ligma_display_shell_remove_unrotated_item (shell, tool->progress);

      gtk_grab_remove (tool->progress_grab_widget);
      gtk_widget_destroy (tool->progress_grab_widget);

      tool->progress             = NULL;
      tool->progress_display     = NULL;
      tool->progress_grab_widget = NULL;
      tool->progress_cancelable  = FALSE;
    }
}

static gboolean
ligma_tool_progress_is_active (LigmaProgress *progress)
{
  LigmaTool *tool = LIGMA_TOOL (progress);

  return tool->progress != NULL;
}

static void
ligma_tool_progress_set_text (LigmaProgress *progress,
                             const gchar  *message)
{
  LigmaTool *tool = LIGMA_TOOL (progress);

  if (tool->progress)
    ligma_progress_set_text_literal (LIGMA_PROGRESS (tool->progress), message);
}

static void
ligma_tool_progress_set_value (LigmaProgress *progress,
                              gdouble       percentage)
{
  LigmaTool *tool = LIGMA_TOOL (progress);

  if (tool->progress)
    ligma_progress_set_value (LIGMA_PROGRESS (tool->progress), percentage);
}

static gdouble
ligma_tool_progress_get_value (LigmaProgress *progress)
{
  LigmaTool *tool = LIGMA_TOOL (progress);

  if (tool->progress)
    return ligma_progress_get_value (LIGMA_PROGRESS (tool->progress));

  return 0.0;
}

static void
ligma_tool_progress_pulse (LigmaProgress *progress)
{
}

static gboolean
ligma_tool_progress_message (LigmaProgress        *progress,
                            Ligma                *ligma,
                            LigmaMessageSeverity  severity,
                            const gchar         *domain,
                            const gchar         *message)
{
  return FALSE;
}
