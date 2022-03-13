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

#include "config/gimpguiconfig.h"

#include "core/gimpimage.h"

#include "widgets/gimpcursor.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimpsessioninfo.h"

#include "gimpcanvascursor.h"
#include "gimpdisplay.h"
#include "gimpcursorview.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-cursor.h"
#include "gimpdisplayshell-transform.h"
#include "gimpstatusbar.h"


static void  gimp_display_shell_real_set_cursor (GimpDisplayShell   *shell,
                                                 GimpCursorType      cursor_type,
                                                 GimpToolCursorType  tool_cursor,
                                                 GimpCursorModifier  modifier,
                                                 gboolean            always_install);


/*  public functions  */

void
gimp_display_shell_set_cursor (GimpDisplayShell   *shell,
                               GimpCursorType      cursor_type,
                               GimpToolCursorType  tool_cursor,
                               GimpCursorModifier  modifier)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->using_override_cursor)
    {
      gimp_display_shell_real_set_cursor (shell,
                                          cursor_type,
                                          tool_cursor,
                                          modifier,
                                          FALSE);
    }
}

void
gimp_display_shell_unset_cursor (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->using_override_cursor)
    {
      gimp_display_shell_real_set_cursor (shell,
                                          (GimpCursorType) -1, 0, 0, FALSE);
    }
}

void
gimp_display_shell_set_override_cursor (GimpDisplayShell *shell,
                                        GimpCursorType    cursor_type)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->using_override_cursor ||
      (shell->using_override_cursor &&
       shell->override_cursor != cursor_type))
    {
      shell->override_cursor       = cursor_type;
      shell->using_override_cursor = TRUE;

      gimp_cursor_set (shell->canvas,
                       shell->cursor_handedness,
                       cursor_type,
                       GIMP_TOOL_CURSOR_NONE,
                       GIMP_CURSOR_MODIFIER_NONE);
    }
}

void
gimp_display_shell_unset_override_cursor (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->using_override_cursor)
    {
      shell->using_override_cursor = FALSE;

      gimp_display_shell_real_set_cursor (shell,
                                          shell->current_cursor,
                                          shell->tool_cursor,
                                          shell->cursor_modifier,
                                          TRUE);
    }
}

void
gimp_display_shell_update_software_cursor (GimpDisplayShell    *shell,
                                           GimpCursorPrecision  precision,
                                           gint                 display_x,
                                           gint                 display_y,
                                           gdouble              image_x,
                                           gdouble              image_y)
{
  GimpImageWindow   *image_window;
  GimpDialogFactory *factory;
  GimpStatusbar     *statusbar;
  GtkWidget         *widget;
  GimpImage         *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  if (shell->draw_cursor &&
      shell->proximity   &&
      display_x >= 0     &&
      display_y >= 0)
    {
      gimp_canvas_item_begin_change (shell->cursor);

      gimp_canvas_cursor_set (shell->cursor,
                              display_x,
                              display_y);
      gimp_canvas_item_set_visible (shell->cursor, TRUE);

      gimp_canvas_item_end_change (shell->cursor);
    }
  else
    {
      gimp_canvas_item_set_visible (shell->cursor, FALSE);
    }

  /*  use the passed image_coords for the statusbar because they are
   *  possibly snapped...
   */
  statusbar = gimp_display_shell_get_statusbar (shell);

  gimp_statusbar_update_cursor (statusbar, precision, image_x, image_y);

  image_window = gimp_display_shell_get_window (shell);
  factory = gimp_dock_container_get_dialog_factory (GIMP_DOCK_CONTAINER (image_window));

  widget = gimp_dialog_factory_find_widget (factory, "gimp-cursor-view");

  if (widget)
    {
      GtkWidget *cursor_view = gtk_bin_get_child (GTK_BIN (widget));

      if (cursor_view)
        {
          gint t_x = -1;
          gint t_y = -1;

          /*  ...but use the unsnapped display_coords for the info window  */
          if (display_x >= 0 && display_y >= 0)
            gimp_display_shell_untransform_xy (shell, display_x, display_y,
                                               &t_x, &t_y, FALSE);

          gimp_cursor_view_update_cursor (GIMP_CURSOR_VIEW (cursor_view),
                                          image, shell->unit,
                                          t_x, t_y);
        }
    }
}

void
gimp_display_shell_clear_software_cursor (GimpDisplayShell *shell)
{
  GimpImageWindow   *image_window;
  GimpDialogFactory *factory;
  GimpStatusbar     *statusbar;
  GtkWidget         *widget;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_canvas_item_set_visible (shell->cursor, FALSE);

  statusbar = gimp_display_shell_get_statusbar (shell);

  gimp_statusbar_clear_cursor (statusbar);

  image_window = gimp_display_shell_get_window (shell);
  factory = gimp_dock_container_get_dialog_factory (GIMP_DOCK_CONTAINER (image_window));

  widget = gimp_dialog_factory_find_widget (factory, "gimp-cursor-view");

  if (widget)
    {
      GtkWidget *cursor_view = gtk_bin_get_child (GTK_BIN (widget));

      if (cursor_view)
        gimp_cursor_view_clear_cursor (GIMP_CURSOR_VIEW (cursor_view));
    }
}


/*  private functions  */

static void
gimp_display_shell_real_set_cursor (GimpDisplayShell   *shell,
                                    GimpCursorType      cursor_type,
                                    GimpToolCursorType  tool_cursor,
                                    GimpCursorModifier  modifier,
                                    gboolean            always_install)
{
  GimpHandedness cursor_handedness;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (cursor_type == (GimpCursorType) -1)
    {
      shell->current_cursor = cursor_type;

      if (gtk_widget_is_drawable (shell->canvas))
        gdk_window_set_cursor (gtk_widget_get_window (shell->canvas), NULL);

      return;
    }

  if (cursor_type != GIMP_CURSOR_NONE &&
      cursor_type != GIMP_CURSOR_BAD  &&
      cursor_type != GIMP_CURSOR_SINGLE_DOT)
    {
      switch (shell->display->config->cursor_mode)
        {
        case GIMP_CURSOR_MODE_TOOL_ICON:
          break;

        case GIMP_CURSOR_MODE_TOOL_CROSSHAIR:
          if (cursor_type < GIMP_CURSOR_CORNER_TOP ||
              cursor_type > GIMP_CURSOR_SIDE_TOP_LEFT)
            {
              /* the corner and side cursors count as crosshair, so leave
               * them and override everything else
               */
              cursor_type = GIMP_CURSOR_CROSSHAIR_SMALL;
            }
          break;

        case GIMP_CURSOR_MODE_CROSSHAIR:
          cursor_type = GIMP_CURSOR_CROSSHAIR;
          tool_cursor = GIMP_TOOL_CURSOR_NONE;

          if (modifier != GIMP_CURSOR_MODIFIER_BAD)
            {
              /* the bad modifier is always shown */
              modifier = GIMP_CURSOR_MODIFIER_NONE;
            }
          break;
        }
    }

  cursor_type = gimp_cursor_rotate (cursor_type, shell->rotate_angle);

  cursor_handedness = GIMP_GUI_CONFIG (shell->display->config)->cursor_handedness;

  if (shell->cursor_handedness != cursor_handedness ||
      shell->current_cursor    != cursor_type       ||
      shell->tool_cursor       != tool_cursor       ||
      shell->cursor_modifier   != modifier          ||
      always_install)
    {
      shell->cursor_handedness = cursor_handedness;
      shell->current_cursor    = cursor_type;
      shell->tool_cursor       = tool_cursor;
      shell->cursor_modifier   = modifier;

      gimp_cursor_set (shell->canvas,
                       cursor_handedness,
                       cursor_type, tool_cursor, modifier);
    }
}
