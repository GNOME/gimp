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

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "widgets/gimpcursor.h"
#include "widgets/gimpcursorview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpsessioninfo.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-cursor.h"
#include "gimpdisplayshell-transform.h"
#include "gimpstatusbar.h"


static void  gimp_display_shell_real_set_cursor (GimpDisplayShell   *shell,
                                                 GimpCursorType      cursor_type,
                                                 GimpToolCursorType  tool_cursor,
                                                 GimpCursorModifier  modifier,
                                                 gboolean            always_install);


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
                       shell->cursor_format,
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
gimp_display_shell_update_cursor (GimpDisplayShell *shell,
                                  gint              display_x,
                                  gint              display_y,
                                  gint              image_x,
                                  gint              image_y)
{
  GimpImage *image;
  gboolean   new_cursor;
  gint       t_x = -1;
  gint       t_y = -1;

  GimpDialogFactory *factory;
  GimpSessionInfo   *session_info;
  GtkWidget         *cursor_view = NULL;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = shell->display->image;

  new_cursor = (shell->draw_cursor &&
                shell->proximity   &&
                display_x >= 0     &&
                display_y >= 0);

  /* Erase old cursor, if necessary */

  if (shell->have_cursor && (! new_cursor                 ||
                             display_x != shell->cursor_x ||
                             display_y != shell->cursor_y))
    {
      gimp_display_shell_expose_area (shell,
                                      shell->cursor_x - 7,
                                      shell->cursor_y - 7,
                                      15, 15);
      if (! new_cursor)
        shell->have_cursor = FALSE;
    }

  shell->have_cursor = new_cursor;
  shell->cursor_x    = display_x;
  shell->cursor_y    = display_y;

  /*  use the passed image_coords for the statusbar because they are
   *  possibly snapped...
   */
  gimp_statusbar_set_cursor (GIMP_STATUSBAR (shell->statusbar),
                             image_x, image_y);

  /*  ...but use the unsnapped display_coords for the info window  */
  if (display_x >= 0 && display_y >= 0)
    gimp_display_shell_untransform_xy (shell, display_x, display_y,
                                       &t_x, &t_y, FALSE, FALSE);

  factory = gimp_dialog_factory_from_name ("dock");
  session_info = gimp_dialog_factory_find_session_info (factory,
                                                        "gimp-cursor-view");
  if (session_info && session_info->widget)
    cursor_view = gtk_bin_get_child (GTK_BIN (session_info->widget));

  if (cursor_view)
    gimp_cursor_view_update_cursor (GIMP_CURSOR_VIEW (cursor_view),
                                    shell->display->image, shell->unit,
                                    t_x, t_y);
}

void
gimp_display_shell_clear_cursor (GimpDisplayShell *shell)
{
  GimpDialogFactory *factory;
  GimpSessionInfo   *session_info;
  GtkWidget         *cursor_view = NULL;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_statusbar_clear_cursor (GIMP_STATUSBAR (shell->statusbar));

  factory = gimp_dialog_factory_from_name ("dock");
  session_info = gimp_dialog_factory_find_session_info (factory,
                                                        "gimp-cursor-view");
  if (session_info && session_info->widget)
    cursor_view = gtk_bin_get_child (GTK_BIN (session_info->widget));

  if (cursor_view)
    gimp_cursor_view_update_cursor (GIMP_CURSOR_VIEW (cursor_view),
                                    NULL, GIMP_UNIT_PIXEL, 0, 0);
}

static void
gimp_display_shell_real_set_cursor (GimpDisplayShell   *shell,
                                    GimpCursorType      cursor_type,
                                    GimpToolCursorType  tool_cursor,
                                    GimpCursorModifier  modifier,
                                    gboolean            always_install)
{
  GimpDisplayConfig *config;
  GimpCursorFormat   cursor_format;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  config = GIMP_DISPLAY_CONFIG (shell->display->image->gimp->config);

  if (cursor_type != GIMP_CURSOR_NONE &&
      cursor_type != GIMP_CURSOR_BAD)
    {
      switch (config->cursor_mode)
        {
        case GIMP_CURSOR_MODE_TOOL_ICON:
          break;

        case GIMP_CURSOR_MODE_TOOL_CROSSHAIR:
          if (cursor_type < GIMP_CURSOR_CORNER_TOP_LEFT ||
              cursor_type > GIMP_CURSOR_SIDE_BOTTOM)
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

  cursor_format = GIMP_GUI_CONFIG (config)->cursor_format;

  if (shell->cursor_format   != cursor_format ||
      shell->current_cursor  != cursor_type   ||
      shell->tool_cursor     != tool_cursor   ||
      shell->cursor_modifier != modifier      ||
      always_install)
    {
      shell->cursor_format   = cursor_format;
      shell->current_cursor  = cursor_type;
      shell->tool_cursor     = tool_cursor;
      shell->cursor_modifier = modifier;

      gimp_cursor_set (shell->canvas, cursor_format,
                       cursor_type, tool_cursor, modifier);
    }
}
