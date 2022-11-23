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

#include "config/ligmaguiconfig.h"

#include "core/ligmaimage.h"

#include "widgets/ligmacursor.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadockcontainer.h"
#include "widgets/ligmasessioninfo.h"

#include "ligmacanvascursor.h"
#include "ligmadisplay.h"
#include "ligmacursorview.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-cursor.h"
#include "ligmadisplayshell-transform.h"
#include "ligmastatusbar.h"


static void  ligma_display_shell_real_set_cursor (LigmaDisplayShell   *shell,
                                                 LigmaCursorType      cursor_type,
                                                 LigmaToolCursorType  tool_cursor,
                                                 LigmaCursorModifier  modifier,
                                                 gboolean            always_install);


/*  public functions  */

void
ligma_display_shell_set_cursor (LigmaDisplayShell   *shell,
                               LigmaCursorType      cursor_type,
                               LigmaToolCursorType  tool_cursor,
                               LigmaCursorModifier  modifier)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (! shell->using_override_cursor)
    {
      ligma_display_shell_real_set_cursor (shell,
                                          cursor_type,
                                          tool_cursor,
                                          modifier,
                                          FALSE);
    }
}

void
ligma_display_shell_unset_cursor (LigmaDisplayShell *shell)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (! shell->using_override_cursor)
    {
      ligma_display_shell_real_set_cursor (shell,
                                          (LigmaCursorType) -1, 0, 0, FALSE);
    }
}

void
ligma_display_shell_set_override_cursor (LigmaDisplayShell *shell,
                                        LigmaCursorType    cursor_type)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (! shell->using_override_cursor ||
      (shell->using_override_cursor &&
       shell->override_cursor != cursor_type))
    {
      shell->override_cursor       = cursor_type;
      shell->using_override_cursor = TRUE;

      ligma_cursor_set (shell->canvas,
                       shell->cursor_handedness,
                       cursor_type,
                       LIGMA_TOOL_CURSOR_NONE,
                       LIGMA_CURSOR_MODIFIER_NONE);
    }
}

void
ligma_display_shell_unset_override_cursor (LigmaDisplayShell *shell)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (shell->using_override_cursor)
    {
      shell->using_override_cursor = FALSE;

      ligma_display_shell_real_set_cursor (shell,
                                          shell->current_cursor,
                                          shell->tool_cursor,
                                          shell->cursor_modifier,
                                          TRUE);
    }
}

void
ligma_display_shell_update_software_cursor (LigmaDisplayShell    *shell,
                                           LigmaCursorPrecision  precision,
                                           gint                 display_x,
                                           gint                 display_y,
                                           gdouble              image_x,
                                           gdouble              image_y)
{
  LigmaImageWindow   *image_window;
  LigmaDialogFactory *factory;
  LigmaStatusbar     *statusbar;
  GtkWidget         *widget;
  LigmaImage         *image;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  image = ligma_display_get_image (shell->display);

  if (shell->draw_cursor &&
      shell->proximity   &&
      display_x >= 0     &&
      display_y >= 0)
    {
      ligma_canvas_item_begin_change (shell->cursor);

      ligma_canvas_cursor_set (shell->cursor,
                              display_x,
                              display_y);
      ligma_canvas_item_set_visible (shell->cursor, TRUE);

      ligma_canvas_item_end_change (shell->cursor);
    }
  else
    {
      ligma_canvas_item_set_visible (shell->cursor, FALSE);
    }

  /*  use the passed image_coords for the statusbar because they are
   *  possibly snapped...
   */
  statusbar = ligma_display_shell_get_statusbar (shell);

  ligma_statusbar_update_cursor (statusbar, precision, image_x, image_y);

  image_window = ligma_display_shell_get_window (shell);
  factory = ligma_dock_container_get_dialog_factory (LIGMA_DOCK_CONTAINER (image_window));

  widget = ligma_dialog_factory_find_widget (factory, "ligma-cursor-view");

  if (widget)
    {
      GtkWidget *cursor_view = gtk_bin_get_child (GTK_BIN (widget));

      if (cursor_view)
        {
          gint t_x = -1;
          gint t_y = -1;

          /*  ...but use the unsnapped display_coords for the info window  */
          if (display_x >= 0 && display_y >= 0)
            ligma_display_shell_untransform_xy (shell, display_x, display_y,
                                               &t_x, &t_y, FALSE);

          ligma_cursor_view_update_cursor (LIGMA_CURSOR_VIEW (cursor_view),
                                          image, shell->unit,
                                          t_x, t_y);
        }
    }
}

void
ligma_display_shell_clear_software_cursor (LigmaDisplayShell *shell)
{
  LigmaImageWindow   *image_window;
  LigmaDialogFactory *factory;
  LigmaStatusbar     *statusbar;
  GtkWidget         *widget;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  ligma_canvas_item_set_visible (shell->cursor, FALSE);

  statusbar = ligma_display_shell_get_statusbar (shell);

  ligma_statusbar_clear_cursor (statusbar);

  image_window = ligma_display_shell_get_window (shell);
  factory = ligma_dock_container_get_dialog_factory (LIGMA_DOCK_CONTAINER (image_window));

  widget = ligma_dialog_factory_find_widget (factory, "ligma-cursor-view");

  if (widget)
    {
      GtkWidget *cursor_view = gtk_bin_get_child (GTK_BIN (widget));

      if (cursor_view)
        ligma_cursor_view_clear_cursor (LIGMA_CURSOR_VIEW (cursor_view));
    }
}


/*  private functions  */

static void
ligma_display_shell_real_set_cursor (LigmaDisplayShell   *shell,
                                    LigmaCursorType      cursor_type,
                                    LigmaToolCursorType  tool_cursor,
                                    LigmaCursorModifier  modifier,
                                    gboolean            always_install)
{
  LigmaHandedness cursor_handedness;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (cursor_type == (LigmaCursorType) -1)
    {
      shell->current_cursor = cursor_type;

      if (gtk_widget_is_drawable (shell->canvas))
        gdk_window_set_cursor (gtk_widget_get_window (shell->canvas), NULL);

      return;
    }

  if (cursor_type != LIGMA_CURSOR_NONE &&
      cursor_type != LIGMA_CURSOR_BAD  &&
      cursor_type != LIGMA_CURSOR_SINGLE_DOT)
    {
      switch (shell->display->config->cursor_mode)
        {
        case LIGMA_CURSOR_MODE_TOOL_ICON:
          break;

        case LIGMA_CURSOR_MODE_TOOL_CROSSHAIR:
          if (cursor_type < LIGMA_CURSOR_CORNER_TOP ||
              cursor_type > LIGMA_CURSOR_SIDE_TOP_LEFT)
            {
              /* the corner and side cursors count as crosshair, so leave
               * them and override everything else
               */
              cursor_type = LIGMA_CURSOR_CROSSHAIR_SMALL;
            }
          break;

        case LIGMA_CURSOR_MODE_CROSSHAIR:
          cursor_type = LIGMA_CURSOR_CROSSHAIR;
          tool_cursor = LIGMA_TOOL_CURSOR_NONE;

          if (modifier != LIGMA_CURSOR_MODIFIER_BAD)
            {
              /* the bad modifier is always shown */
              modifier = LIGMA_CURSOR_MODIFIER_NONE;
            }
          break;
        }
    }

  cursor_type = ligma_cursor_rotate (cursor_type, shell->rotate_angle);

  cursor_handedness = LIGMA_GUI_CONFIG (shell->display->config)->cursor_handedness;

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

      ligma_cursor_set (shell->canvas,
                       cursor_handedness,
                       cursor_type, tool_cursor, modifier);
    }
}
