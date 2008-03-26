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

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpviewable.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-icon.h"


#define GIMP_DISPLAY_UPDATE_ICON_TIMEOUT  1000

static gboolean   gimp_display_shell_idle_update_icon  (gpointer data);


void
gimp_display_shell_icon_update (GimpDisplayShell *shell)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = shell->display->image;

  if (image)
    {
      Gimp      *gimp   = shell->display->gimp;
      GdkPixbuf *pixbuf;
      gint       width;
      gint       height;
      gdouble    factor = ((gdouble) gimp_image_get_height (image) /
                           (gdouble) gimp_image_get_width  (image));

      if (factor >= 1)
        {
          height = MAX (shell->icon_size, 1);
          width  = MAX (((gdouble) shell->icon_size) / factor, 1);
        }
      else
        {
          height = MAX (((gdouble) shell->icon_size) * factor, 1);
          width  = MAX (shell->icon_size, 1);
        }

      pixbuf = gimp_viewable_get_pixbuf (GIMP_VIEWABLE (image),
                                         gimp_get_user_context (gimp),
                                         width, height);

      gtk_window_set_icon (GTK_WINDOW (shell), pixbuf);
    }
  else
    {
      gtk_window_set_icon (GTK_WINDOW (shell), NULL);
    }
}

void
gimp_display_shell_icon_idle_update (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->icon_idle_id)
    g_source_remove (shell->icon_idle_id);

  shell->icon_idle_id = g_timeout_add_full (G_PRIORITY_LOW,
                                            GIMP_DISPLAY_UPDATE_ICON_TIMEOUT,
                                            gimp_display_shell_idle_update_icon,
                                            shell,
                                            NULL);
}

void
gimp_display_shell_icon_idle_stop (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->icon_idle_id)
    {
      g_source_remove (shell->icon_idle_id);
      shell->icon_idle_id = 0;
    }
}

static gboolean
gimp_display_shell_idle_update_icon (gpointer data)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);

  shell->icon_idle_id = 0;

  gimp_display_shell_icon_update (shell);

  return FALSE;
}
