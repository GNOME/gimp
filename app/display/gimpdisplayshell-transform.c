/* The GIMP -- an image manipulation program
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

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"


void
gimp_display_shell_transform_coords (GimpDisplayShell *shell,
                                     GimpCoords       *image_coords,
                                     GimpCoords       *display_coords)
{
  gdouble scalex;
  gdouble scaley;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (image_coords != NULL);
  g_return_if_fail (display_coords != NULL);

  *display_coords = *image_coords;

  scalex = SCALEFACTOR_X (shell);
  scaley = SCALEFACTOR_Y (shell);

  display_coords->x = scalex * image_coords->x;
  display_coords->y = scaley * image_coords->y;

  display_coords->x += - shell->offset_x + shell->disp_xoffset;
  display_coords->y += - shell->offset_y + shell->disp_yoffset;
}

void
gimp_display_shell_untransform_coords (GimpDisplayShell *shell,
                                       GimpCoords       *display_coords,
                                       GimpCoords       *image_coords)
{
  gdouble scalex;
  gdouble scaley;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (display_coords != NULL);
  g_return_if_fail (image_coords != NULL);

  *image_coords = *display_coords;

  scalex = SCALEFACTOR_X (shell);
  scaley = SCALEFACTOR_Y (shell);

  image_coords->x = display_coords->x - shell->disp_xoffset + shell->offset_x;
  image_coords->y = display_coords->y - shell->disp_yoffset + shell->offset_y;

  image_coords->x /= scalex;
  image_coords->y /= scaley;
}

void
gimp_display_shell_transform_xy (GimpDisplayShell *shell,
                                 gint              x,
                                 gint              y,
                                 gint             *nx,
                                 gint             *ny,
                                 gboolean          use_offsets)
{
  gdouble scalex;
  gdouble scaley;
  gint    offset_x = 0;
  gint    offset_y = 0;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  /*  transform from image coordinates to screen coordinates  */
  scalex = SCALEFACTOR_X (shell);
  scaley = SCALEFACTOR_Y (shell);

  if (use_offsets)
    gimp_item_offsets (GIMP_ITEM (gimp_image_active_drawable (shell->gdisp->gimage)),
                       &offset_x, &offset_y);

  *nx = (gint) (scalex * (x + offset_x) - shell->offset_x);
  *ny = (gint) (scaley * (y + offset_y) - shell->offset_y);

  *nx += shell->disp_xoffset;
  *ny += shell->disp_yoffset;
}

void
gimp_display_shell_untransform_xy (GimpDisplayShell *shell,
                                   gint              x,
                                   gint              y,
                                   gint             *nx,
                                   gint             *ny,
                                   gboolean          round,
                                   gboolean          use_offsets)
{
  gdouble scalex;
  gdouble scaley;
  gint    offset_x = 0;
  gint    offset_y = 0;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  x -= shell->disp_xoffset;
  y -= shell->disp_yoffset;

  /*  transform from screen coordinates to image coordinates  */
  scalex = SCALEFACTOR_X (shell);
  scaley = SCALEFACTOR_Y (shell);

  if (use_offsets)
    gimp_item_offsets (GIMP_ITEM (gimp_image_active_drawable (shell->gdisp->gimage)),
                       &offset_x, &offset_y);

  if (round)
    {
      *nx = ROUND ((x + shell->offset_x) / scalex - offset_x);
      *ny = ROUND ((y + shell->offset_y) / scaley - offset_y);
    }
  else
    {
      *nx = (gint) ((x + shell->offset_x) / scalex - offset_x);
      *ny = (gint) ((y + shell->offset_y) / scaley - offset_y);
    }
}

void
gimp_display_shell_transform_xy_f  (GimpDisplayShell *shell,
                                    gdouble           x,
                                    gdouble           y,
                                    gdouble          *nx,
                                    gdouble          *ny,
                                    gboolean          use_offsets)
{
  gdouble scalex;
  gdouble scaley;
  gint    offset_x = 0;
  gint    offset_y = 0;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  /*  transform from gimp coordinates to screen coordinates  */
  scalex = SCALEFACTOR_X (shell);
  scaley = SCALEFACTOR_Y (shell);

  if (use_offsets)
    gimp_item_offsets (GIMP_ITEM (gimp_image_active_drawable (shell->gdisp->gimage)),
                       &offset_x, &offset_y);

  *nx = scalex * (x + offset_x) - shell->offset_x;
  *ny = scaley * (y + offset_y) - shell->offset_y;

  *nx += shell->disp_xoffset;
  *ny += shell->disp_yoffset;
}

void
gimp_display_shell_untransform_xy_f (GimpDisplayShell *shell,
                                     gdouble           x,
                                     gdouble           y,
                                     gdouble          *nx,
                                     gdouble          *ny,
                                     gboolean          use_offsets)
{
  gdouble scalex;
  gdouble scaley;
  gint    offset_x = 0;
  gint    offset_y = 0;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  x -= shell->disp_xoffset;
  y -= shell->disp_yoffset;

  /*  transform from screen coordinates to gimp coordinates  */
  scalex = SCALEFACTOR_X (shell);
  scaley = SCALEFACTOR_Y (shell);

  if (use_offsets)
    gimp_item_offsets (GIMP_ITEM (gimp_image_active_drawable (shell->gdisp->gimage)),
                       &offset_x, &offset_y);

  *nx = (x + shell->offset_x) / scalex - offset_x;
  *ny = (y + shell->offset_y) / scaley - offset_y;
}

void
gimp_display_shell_untransform_viewport (GimpDisplayShell *shell,
                                         gint             *x,
                                         gint             *y,
                                         gint             *width,
                                         gint             *height)
{
  gint x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_untransform_xy (shell,
                                     0, 0,
                                     &x1, &y1,
                                     FALSE, FALSE);
  gimp_display_shell_untransform_xy (shell,
                                     shell->disp_width, shell->disp_height,
                                     &x2, &y2,
                                     FALSE, FALSE);

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 > shell->gdisp->gimage->width)  x2 = shell->gdisp->gimage->width;
  if (y2 > shell->gdisp->gimage->height) y2 = shell->gdisp->gimage->height;

  if (x)      *x      = x1;
  if (y)      *y      = y1;
  if (width)  *width  = x2 - x1;
  if (height) *height = y2 - y1;
}
