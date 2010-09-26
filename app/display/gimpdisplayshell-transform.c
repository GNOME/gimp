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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "base/boundary.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-transform.h"


/**
 * gimp_display_shell_transform_coordinate:
 * @shell:          a #GimpDisplayShell
 * @image_coords:   image coordinates
 * @display_coords: returns the corresponding display coordinates
 *
 * Transforms from image coordinates to display coordinates, so that
 * objects can be rendered at the correct points on the display.
 **/
void
gimp_display_shell_transform_coordinate (const GimpDisplayShell *shell,
                                         GimpCoords             *image_coords,
                                         GimpCoords             *display_coords)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (image_coords != NULL);
  g_return_if_fail (display_coords != NULL);

  *display_coords = *image_coords;

  display_coords->x = SCALEX (shell, image_coords->x);
  display_coords->y = SCALEY (shell, image_coords->y);

  display_coords->x -= shell->offset_x;
  display_coords->y -= shell->offset_y;
}

/**
 * gimp_display_shell_untransform_coordinate:
 * @shell:          a #GimpDisplayShell
 * @display_coords: display coordinates
 * @image_coords:   returns the corresponding image coordinates
 *
 * Transforms from display coordinates to image coordinates, so that
 * points on the display can be mapped to points in the image.
 **/
void
gimp_display_shell_untransform_coordinate (const GimpDisplayShell *shell,
                                           GimpCoords             *display_coords,
                                           GimpCoords             *image_coords)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (display_coords != NULL);
  g_return_if_fail (image_coords != NULL);

  *image_coords = *display_coords;

  image_coords->x = display_coords->x + shell->offset_x;
  image_coords->y = display_coords->y + shell->offset_y;

  image_coords->x /= shell->scale_x;
  image_coords->y /= shell->scale_y;
}

/**
 * gimp_display_shell_transform_xy:
 * @shell:
 * @x:
 * @y:
 * @nx:
 * @ny:
 *
 * Transforms an image coordinate to a shell coordinate.
 **/
void
gimp_display_shell_transform_xy (const GimpDisplayShell *shell,
                                 gdouble                 x,
                                 gdouble                 y,
                                 gint                   *nx,
                                 gint                   *ny)
{
  gint64 tx;
  gint64 ty;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  tx = ((gint64) x * shell->x_src_dec) / shell->x_dest_inc;
  ty = ((gint64) y * shell->y_src_dec) / shell->y_dest_inc;

  tx -= shell->offset_x;
  ty -= shell->offset_y;

  /* The projected coordinates might overflow a gint in the case of big
     images at high zoom levels, so we clamp them here to avoid problems.  */
  *nx = CLAMP (tx, G_MININT, G_MAXINT);
  *ny = CLAMP (ty, G_MININT, G_MAXINT);
}

/**
 * gimp_display_shell_untransform_xy:
 * @shell:       a #GimpDisplayShell
 * @x:           x coordinate in display coordinates
 * @y:           y coordinate in display coordinates
 * @nx:          returns x oordinate in image coordinates
 * @ny:          returns y coordinate in image coordinates
 * @round:       if %TRUE, round the results to the nearest integer;
 *               if %FALSE, simply cast them to @gint.
 *
 * Transform from display coordinates to image coordinates, so that
 * points on the display can be mapped to the corresponding points
 * in the image.
 **/
void
gimp_display_shell_untransform_xy (const GimpDisplayShell *shell,
                                   gint                    x,
                                   gint                    y,
                                   gint                   *nx,
                                   gint                   *ny,
                                   gboolean                round)
{
  gint64 tx;
  gint64 ty;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  tx = (gint64) x + shell->offset_x;
  ty = (gint64) y + shell->offset_y;

  tx *= shell->x_dest_inc;
  ty *= shell->y_dest_inc;

  tx += round ? shell->x_dest_inc : shell->x_dest_inc >> 1;
  ty += round ? shell->y_dest_inc : shell->y_dest_inc >> 1;

  tx /= shell->x_src_dec;
  ty /= shell->y_src_dec;

  *nx = CLAMP (tx, G_MININT, G_MAXINT);
  *ny = CLAMP (ty, G_MININT, G_MAXINT);
}

/**
 * gimp_display_shell_transform_xy_f:
 * @shell:       a #GimpDisplayShell
 * @x:           x coordinate of point in image coordinates
 * @y:           y coordinate of point in image coordinate
 * @nx:          returns the transformed x coordinate
 * @ny:          returns the transformed y coordinate
 *
 * This function is identical to gimp_display_shell_transfrom_xy(),
 * except that it returns its results as doubles rather than ints.
 **/
void
gimp_display_shell_transform_xy_f  (const GimpDisplayShell *shell,
                                    gdouble                 x,
                                    gdouble                 y,
                                    gdouble                *nx,
                                    gdouble                *ny)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  *nx = SCALEX (shell, x) - shell->offset_x;
  *ny = SCALEY (shell, y) - shell->offset_y;
}

/**
 * gimp_display_shell_untransform_xy_f:
 * @shell:       a #GimpDisplayShell
 * @x:           x coordinate in display coordinates
 * @y:           y coordinate in display coordinates
 * @nx:          place to return x coordinate in image coordinates
 * @ny:          place to return y coordinate in image coordinates
 *
 * This function is identical to gimp_display_shell_untransform_xy(),
 * except that the input and output coordinates are doubles rather than
 * ints, and consequently there is no option related to rounding.
 **/
void
gimp_display_shell_untransform_xy_f (const GimpDisplayShell *shell,
                                     gdouble                 x,
                                     gdouble                 y,
                                     gdouble                *nx,
                                     gdouble                *ny)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  *nx = (x + shell->offset_x) / shell->scale_x;
  *ny = (y + shell->offset_y) / shell->scale_y;
}

/**
 * gimp_display_shell_transform_points:
 * @shell:       a #GimpDisplayShell
 * @points:      array of GimpVectors2 coordinate pairs
 * @coords:      returns the corresponding display coordinates
 * @n_points:    number of points
 *
 * Transforms from image coordinates to display coordinates, so that
 * objects can be rendered at the correct points on the display.
 **/
void
gimp_display_shell_transform_points (const GimpDisplayShell *shell,
                                     const GimpVector2      *points,
                                     GdkPoint               *coords,
                                     gint                    n_points)
{
  gint i;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  for (i = 0; i < n_points ; i++)
    {
      gdouble x = points[i].x;
      gdouble y = points[i].y;

      x = x * shell->x_src_dec / shell->x_dest_inc;
      y = y * shell->y_src_dec / shell->y_dest_inc;

      coords[i].x = CLAMP (PROJ_ROUND64 (x) - shell->offset_x,
                           G_MININT, G_MAXINT);
      coords[i].y = CLAMP (PROJ_ROUND64 (y) - shell->offset_y,
                           G_MININT, G_MAXINT);
    }
}

/**
 * gimp_display_shell_transform_coords:
 * @shell:        a #GimpDisplayShell
 * @image_coords: array of image coordinates
 * @disp_coords:  returns the corresponding display coordinates
 * @n_coords:     number of coordinates
 *
 * Transforms from image coordinates to display coordinates, so that
 * objects can be rendered at the correct points on the display.
 **/
void
gimp_display_shell_transform_coords (const GimpDisplayShell *shell,
                                     const GimpCoords       *image_coords,
                                     GdkPoint               *disp_coords,
                                     gint                    n_coords)
{
  gint i;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  for (i = 0; i < n_coords ; i++)
    {
      gdouble x = image_coords[i].x;
      gdouble y = image_coords[i].y;

      x = x * shell->x_src_dec / shell->x_dest_inc;
      y = y * shell->y_src_dec / shell->y_dest_inc;

      disp_coords[i].x = CLAMP (PROJ_ROUND64 (x) - shell->offset_x,
                                G_MININT, G_MAXINT);
      disp_coords[i].y = CLAMP (PROJ_ROUND64 (y) - shell->offset_y,
                                G_MININT, G_MAXINT);
    }
}

/**
 * gimp_display_shell_transform_segments:
 * @shell:       a #GimpDisplayShell
 * @src_segs:    array of segments in image coordinates
 * @dest_segs:   returns the corresponding segments in display coordinates
 * @n_segs:      number of segments
 *
 * Transforms from image coordinates to display coordinates, so that
 * objects can be rendered at the correct points on the display.
 **/
void
gimp_display_shell_transform_segments (const GimpDisplayShell *shell,
                                       const BoundSeg         *src_segs,
                                       GdkSegment             *dest_segs,
                                       gint                    n_segs)
{
  gint i;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  for (i = 0; i < n_segs ; i++)
    {
      gint64 x1, x2;
      gint64 y1, y2;

      x1 = src_segs[i].x1;
      x2 = src_segs[i].x2;
      y1 = src_segs[i].y1;
      y2 = src_segs[i].y2;

      x1 = (x1 * shell->x_src_dec) / shell->x_dest_inc;
      x2 = (x2 * shell->x_src_dec) / shell->x_dest_inc;
      y1 = (y1 * shell->y_src_dec) / shell->y_dest_inc;
      y2 = (y2 * shell->y_src_dec) / shell->y_dest_inc;

      dest_segs[i].x1 = CLAMP (x1 - shell->offset_x, G_MININT, G_MAXINT);
      dest_segs[i].x2 = CLAMP (x2 - shell->offset_x, G_MININT, G_MAXINT);
      dest_segs[i].y1 = CLAMP (y1 - shell->offset_y, G_MININT, G_MAXINT);
      dest_segs[i].y2 = CLAMP (y2 - shell->offset_y, G_MININT, G_MAXINT);
    }
}

/**
 * gimp_display_shell_untransform_viewport:
 * @shell:  a #GimpDisplayShell
 * @x:      returns image x coordinate of display upper left corner
 * @y:      returns image y coordinate of display upper left corner
 * @width:  returns width of display measured in image coordinates
 * @height: returns height of display measured in image coordinates
 *
 * This function calculates the part of the image, im image coordinates,
 * that corresponds to the display viewport.
 **/
void
gimp_display_shell_untransform_viewport (const GimpDisplayShell *shell,
                                         gint                   *x,
                                         gint                   *y,
                                         gint                   *width,
                                         gint                   *height)
{
  GimpImage *image;
  gint       x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_untransform_xy (shell,
                                     0, 0,
                                     &x1, &y1,
                                     FALSE);
  gimp_display_shell_untransform_xy (shell,
                                     shell->disp_width, shell->disp_height,
                                     &x2, &y2,
                                     FALSE);

  image = gimp_display_get_image (shell->display);

  if (x1 < 0)
    x1 = 0;

  if (y1 < 0)
    y1 = 0;

  if (x2 > gimp_image_get_width (image))
    x2 = gimp_image_get_width (image);

  if (y2 > gimp_image_get_height (image))
    y2 = gimp_image_get_height (image);

  if (x)      *x      = x1;
  if (y)      *y      = y1;
  if (width)  *width  = x2 - x1;
  if (height) *height = y2 - y1;
}
