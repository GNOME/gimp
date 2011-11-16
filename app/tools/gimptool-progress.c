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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "tools-types.h"

#include "core/gimpprogress.h"

#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasprogress.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-items.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimpdisplayshell-style.h"

#include "gimptool.h"
#include "gimptool-progress.h"


/*  local function prototypes  */

static GimpProgress * gimp_tool_progress_start     (GimpProgress        *progress,
                                                    const gchar         *message,
                                                    gboolean             cancelable);
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

static GimpProgress *
gimp_tool_progress_start (GimpProgress *progress,
                          const gchar  *message,
                          gboolean      cancelable)
{
  GimpTool         *tool = GIMP_TOOL (progress);
  GimpDisplayShell *shell;
  gint              x, y, w, h;

  g_return_val_if_fail (GIMP_IS_DISPLAY (tool->display), NULL);
  g_return_val_if_fail (tool->progress == NULL, NULL);

  shell = gimp_display_get_shell (tool->display);

  gimp_display_shell_untransform_viewport (shell, &x, &y, &w, &h);

  tool->progress = gimp_canvas_progress_new (shell,
                                             GIMP_HANDLE_ANCHOR_CENTER,
                                             x + w / 2, y + h / 2);
  gimp_display_shell_add_tool_item (shell, tool->progress);
  g_object_unref (tool->progress);

  gimp_progress_start (GIMP_PROGRESS (tool->progress),
                       message, FALSE);
  gimp_widget_flush_expose (shell->canvas);

  tool->progress_display = tool->display;

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
      gimp_display_shell_remove_tool_item (shell, tool->progress);

      tool->progress         = NULL;
      tool->progress_display = NULL;
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
    {
      GimpDisplayShell *shell = gimp_display_get_shell (tool->progress_display);

      gimp_progress_set_text (GIMP_PROGRESS (tool->progress), message);
      gimp_widget_flush_expose (shell->canvas);
    }
}

static void
gimp_tool_progress_set_value (GimpProgress *progress,
                              gdouble       percentage)
{
  GimpTool *tool = GIMP_TOOL (progress);

  if (tool->progress)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (tool->progress_display);

      gimp_progress_set_value (GIMP_PROGRESS (tool->progress), percentage);
      gimp_widget_flush_expose (shell->canvas);
    }
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
