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

#include "base/boundary.h"

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

GdkSegment *
gimp_display_shell_transform_boundary (GimpDisplayShell *shell,
                                       BoundSeg         *bound_segs,
                                       gint              n_bound_segs,
                                       gint              offset_x,
                                       gint              offset_y)
{
  GdkSegment *dest_segs;
  gint        x, y;
  gint        xclamp, yclamp;
  gint        i;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (n_bound_segs > 0 || bound_segs == NULL, NULL);

  dest_segs = g_new0 (GdkSegment, n_bound_segs);

  xclamp = shell->disp_width  + 1;
  yclamp = shell->disp_height + 1;

  for (i = 0; i < n_bound_segs; i++)
    {
      gimp_display_shell_transform_xy (shell,
                                       bound_segs[i].x1 + offset_x,
                                       bound_segs[i].y1 + offset_y,
                                       &x, &y,
                                       FALSE);

      dest_segs[i].x1 = CLAMP (x, -1, xclamp);
      dest_segs[i].y1 = CLAMP (y, -1, yclamp);

      gimp_display_shell_transform_xy (shell,
                                       bound_segs[i].x2 + offset_x,
                                       bound_segs[i].y2 + offset_y,
                                       &x, &y,
                                       FALSE);

      dest_segs[i].x2 = CLAMP (x, -1, xclamp);
      dest_segs[i].y2 = CLAMP (y, -1, yclamp);

      /*  If this segment is a closing segment && the segments lie inside
       *  the region, OR if this is an opening segment and the segments
       *  lie outside the region...
       *  we need to transform it by one display pixel
       */
      if (! bound_segs[i].open)
        {
          /*  If it is vertical  */
          if (dest_segs[i].x1 == dest_segs[i].x2)
            {
              dest_segs[i].x1 -= 1;
              dest_segs[i].x2 -= 1;
            }
          else
            {
              dest_segs[i].y1 -= 1;
              dest_segs[i].y2 -= 1;
            }
        }
    }

  return dest_segs;
}
