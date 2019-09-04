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

#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimpboundary.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimp-utils.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-transform.h"


/*  local function prototypes  */

static void gimp_display_shell_transform_xy_f_noround (GimpDisplayShell *shell,
                                                       gdouble           x,
                                                       gdouble           y,
                                                       gdouble          *nx,
                                                       gdouble          *ny);

/*  public functions  */

/**
 * gimp_display_shell_zoom_coords:
 * @shell:          a #GimpDisplayShell
 * @image_coords:   image coordinates
 * @display_coords: returns the corresponding display coordinates
 *
 * Zooms from image coordinates to display coordinates, so that
 * objects can be rendered at the correct points on the display.
 **/
void
gimp_display_shell_zoom_coords (GimpDisplayShell *shell,
                                const GimpCoords *image_coords,
                                GimpCoords       *display_coords)
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
 * gimp_display_shell_unzoom_coords:
 * @shell:          a #GimpDisplayShell
 * @display_coords: display coordinates
 * @image_coords:   returns the corresponding image coordinates
 *
 * Zooms from display coordinates to image coordinates, so that
 * points on the display can be mapped to points in the image.
 **/
void
gimp_display_shell_unzoom_coords (GimpDisplayShell *shell,
                                  const GimpCoords *display_coords,
                                  GimpCoords       *image_coords)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (display_coords != NULL);
  g_return_if_fail (image_coords != NULL);

  *image_coords = *display_coords;

  image_coords->x += shell->offset_x;
  image_coords->y += shell->offset_y;

  image_coords->x /= shell->scale_x;
  image_coords->y /= shell->scale_y;
}

/**
 * gimp_display_shell_zoom_xy:
 * @shell:
 * @x:
 * @y:
 * @nx:
 * @ny:
 *
 * Zooms an image coordinate to a shell coordinate.
 **/
void
gimp_display_shell_zoom_xy (GimpDisplayShell *shell,
                            gdouble           x,
                            gdouble           y,
                            gint             *nx,
                            gint             *ny)
{
  gint64 tx;
  gint64 ty;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  tx = x * shell->scale_x;
  ty = y * shell->scale_y;

  tx -= shell->offset_x;
  ty -= shell->offset_y;

  /*  The projected coordinates might overflow a gint in the case of
   *  big images at high zoom levels, so we clamp them here to avoid
   *  problems.
   */
  *nx = CLAMP (tx, G_MININT, G_MAXINT);
  *ny = CLAMP (ty, G_MININT, G_MAXINT);
}

/**
 * gimp_display_shell_unzoom_xy:
 * @shell:       a #GimpDisplayShell
 * @x:           x coordinate in display coordinates
 * @y:           y coordinate in display coordinates
 * @nx:          returns x oordinate in image coordinates
 * @ny:          returns y coordinate in image coordinates
 * @round:       if %TRUE, round the results to the nearest integer;
 *               if %FALSE, simply cast them to @gint.
 *
 * Zoom from display coordinates to image coordinates, so that
 * points on the display can be mapped to the corresponding points
 * in the image.
 **/
void
gimp_display_shell_unzoom_xy (GimpDisplayShell *shell,
                              gint              x,
                              gint              y,
                              gint             *nx,
                              gint             *ny,
                              gboolean          round)
{
  gint64 tx;
  gint64 ty;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  if (round)
    {
      tx = SIGNED_ROUND (((gdouble) x + shell->offset_x) / shell->scale_x);
      ty = SIGNED_ROUND (((gdouble) y + shell->offset_y) / shell->scale_y);
    }
  else
    {
      tx = ((gint64) x + shell->offset_x) / shell->scale_x;
      ty = ((gint64) y + shell->offset_y) / shell->scale_y;
    }

  *nx = CLAMP (tx, G_MININT, G_MAXINT);
  *ny = CLAMP (ty, G_MININT, G_MAXINT);
}

/**
 * gimp_display_shell_zoom_xy_f:
 * @shell: a #GimpDisplayShell
 * @x:     image x coordinate of point
 * @y:     image y coordinate of point
 * @nx:    returned shell canvas x coordinate
 * @ny:    returned shell canvas y coordinate
 *
 * Zooms from image coordinates to display shell canvas
 * coordinates.
 **/
void
gimp_display_shell_zoom_xy_f (GimpDisplayShell *shell,
                              gdouble           x,
                              gdouble           y,
                              gdouble          *nx,
                              gdouble          *ny)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  *nx = SCALEX (shell, x) - shell->offset_x;
  *ny = SCALEY (shell, y) - shell->offset_y;
}

/**
 * gimp_display_shell_unzoom_xy_f:
 * @shell:       a #GimpDisplayShell
 * @x:           x coordinate in display coordinates
 * @y:           y coordinate in display coordinates
 * @nx:          place to return x coordinate in image coordinates
 * @ny:          place to return y coordinate in image coordinates
 *
 * This function is identical to gimp_display_shell_unzoom_xy(),
 * except that the input and output coordinates are doubles rather than
 * ints, and consequently there is no option related to rounding.
 **/
void
gimp_display_shell_unzoom_xy_f (GimpDisplayShell *shell,
                                gdouble           x,
                                gdouble           y,
                                gdouble          *nx,
                                gdouble          *ny)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  *nx = (x + shell->offset_x) / shell->scale_x;
  *ny = (y + shell->offset_y) / shell->scale_y;
}

/**
 * gimp_display_shell_zoom_segments:
 * @shell:       a #GimpDisplayShell
 * @src_segs:    array of segments in image coordinates
 * @dest_segs:   returns the corresponding segments in display coordinates
 * @n_segs:      number of segments
 *
 * Zooms from image coordinates to display coordinates, so that
 * objects can be rendered at the correct points on the display.
 **/
void
gimp_display_shell_zoom_segments (GimpDisplayShell   *shell,
                                  const GimpBoundSeg *src_segs,
                                  GimpSegment        *dest_segs,
                                  gint                n_segs,
                                  gdouble             offset_x,
                                  gdouble             offset_y)
{
  gint i;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  for (i = 0; i < n_segs ; i++)
    {
      gdouble x1, x2;
      gdouble y1, y2;

      x1 = src_segs[i].x1 + offset_x;
      x2 = src_segs[i].x2 + offset_x;
      y1 = src_segs[i].y1 + offset_y;
      y2 = src_segs[i].y2 + offset_y;

      dest_segs[i].x1 = SCALEX (shell, x1) - shell->offset_x;
      dest_segs[i].x2 = SCALEX (shell, x2) - shell->offset_x;
      dest_segs[i].y1 = SCALEY (shell, y1) - shell->offset_y;
      dest_segs[i].y2 = SCALEY (shell, y2) - shell->offset_y;
    }
}

/**
 * gimp_display_shell_rotate_coords:
 * @shell:          a #GimpDisplayShell
 * @image_coords:   unrotated display coordinates
 * @display_coords: returns the corresponding rotated display coordinates
 *
 * Rotates from unrotated display coordinates to rotated display
 * coordinates, so that objects can be rendered at the correct points
 * on the display.
 **/
void
gimp_display_shell_rotate_coords (GimpDisplayShell *shell,
                                  const GimpCoords *unrotated_coords,
                                  GimpCoords       *rotated_coords)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (unrotated_coords != NULL);
  g_return_if_fail (rotated_coords != NULL);

  *rotated_coords = *unrotated_coords;

  if (shell->rotate_transform)
    cairo_matrix_transform_point (shell->rotate_transform,
                                  &rotated_coords->x,
                                  &rotated_coords->y);
}

/**
 * gimp_display_shell_unrotate_coords:
 * @shell:          a #GimpDisplayShell
 * @display_coords: rotated display coordinates
 * @image_coords:   returns the corresponding unrotated display coordinates
 *
 * Rotates from rotated display coordinates to unrotated display coordinates.
 **/
void
gimp_display_shell_unrotate_coords (GimpDisplayShell *shell,
                                    const GimpCoords *rotated_coords,
                                    GimpCoords       *unrotated_coords)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (rotated_coords != NULL);
  g_return_if_fail (unrotated_coords != NULL);

  *unrotated_coords = *rotated_coords;

  if (shell->rotate_untransform)
    cairo_matrix_transform_point (shell->rotate_untransform,
                                  &unrotated_coords->x,
                                  &unrotated_coords->y);
}

/**
 * gimp_display_shell_rotate_xy:
 * @shell:
 * @x:
 * @y:
 * @nx:
 * @ny:
 *
 * Rotates an unrotated display coordinate to a rotated shell coordinate.
 **/
void
gimp_display_shell_rotate_xy (GimpDisplayShell *shell,
                              gdouble           x,
                              gdouble           y,
                              gint             *nx,
                              gint             *ny)
{
  gint64 tx;
  gint64 ty;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  if (shell->rotate_transform)
    cairo_matrix_transform_point (shell->rotate_transform, &x, &y);

  tx = x;
  ty = y;

  /*  The projected coordinates might overflow a gint in the case of
   *  big images at high zoom levels, so we clamp them here to avoid
   *  problems.
   */
  *nx = CLAMP (tx, G_MININT, G_MAXINT);
  *ny = CLAMP (ty, G_MININT, G_MAXINT);
}

/**
 * gimp_display_shell_unrotate_xy:
 * @shell:       a #GimpDisplayShell
 * @x:           x coordinate in rotated display coordinates
 * @y:           y coordinate in rotated display coordinates
 * @nx:          returns x oordinate in unrotated display coordinates
 * @ny:          returns y coordinate in unrotated display coordinates
 *
 * Rotate from rotated display coordinates to unrotated display
 * coordinates.
 **/
void
gimp_display_shell_unrotate_xy (GimpDisplayShell *shell,
                                gint              x,
                                gint              y,
                                gint             *nx,
                                gint             *ny)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  if (shell->rotate_untransform)
    {
      gdouble fx = x;
      gdouble fy = y;

      cairo_matrix_transform_point (shell->rotate_untransform, &fx, &fy);

      *nx = CLAMP (fx, G_MININT, G_MAXINT);
      *ny = CLAMP (fy, G_MININT, G_MAXINT);
    }
  else
    {
      *nx = x;
      *ny = y;
    }
}

/**
 * gimp_display_shell_rotate_xy_f:
 * @shell: a #GimpDisplayShell
 * @x:     image x coordinate of point
 * @y:     image y coordinate of point
 * @nx:    returned shell canvas x coordinate
 * @ny:    returned shell canvas y coordinate
 *
 * Rotates from untransformed display coordinates to rotated display
 * coordinates.
 **/
void
gimp_display_shell_rotate_xy_f (GimpDisplayShell *shell,
                                gdouble           x,
                                gdouble           y,
                                gdouble          *nx,
                                gdouble          *ny)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  *nx = x;
  *ny = y;

  if (shell->rotate_transform)
    cairo_matrix_transform_point (shell->rotate_transform, nx, ny);
}

/**
 * gimp_display_shell_unrotate_xy_f:
 * @shell:       a #GimpDisplayShell
 * @x:           x coordinate in rotated display coordinates
 * @y:           y coordinate in rotated display coordinates
 * @nx:          place to return x coordinate in unrotated display coordinates
 * @ny:          place to return y coordinate in unrotated display  coordinates
 *
 * This function is identical to gimp_display_shell_unrotate_xy(),
 * except that the input and output coordinates are doubles rather
 * than ints.
 **/
void
gimp_display_shell_unrotate_xy_f (GimpDisplayShell *shell,
                                  gdouble           x,
                                  gdouble           y,
                                  gdouble          *nx,
                                  gdouble          *ny)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  *nx = x;
  *ny = y;

  if (shell->rotate_untransform)
    cairo_matrix_transform_point (shell->rotate_untransform, nx, ny);
}

void
gimp_display_shell_rotate_bounds (GimpDisplayShell *shell,
                                  gdouble           x1,
                                  gdouble           y1,
                                  gdouble           x2,
                                  gdouble           y2,
                                  gdouble          *nx1,
                                  gdouble          *ny1,
                                  gdouble          *nx2,
                                  gdouble          *ny2)
{

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->rotate_transform)
    {
      gdouble tx1 = x1;
      gdouble ty1 = y1;
      gdouble tx2 = x1;
      gdouble ty2 = y2;
      gdouble tx3 = x2;
      gdouble ty3 = y1;
      gdouble tx4 = x2;
      gdouble ty4 = y2;

      cairo_matrix_transform_point (shell->rotate_transform, &tx1, &ty1);
      cairo_matrix_transform_point (shell->rotate_transform, &tx2, &ty2);
      cairo_matrix_transform_point (shell->rotate_transform, &tx3, &ty3);
      cairo_matrix_transform_point (shell->rotate_transform, &tx4, &ty4);

      *nx1 = MIN4 (tx1, tx2, tx3, tx4);
      *ny1 = MIN4 (ty1, ty2, ty3, ty4);
      *nx2 = MAX4 (tx1, tx2, tx3, tx4);
      *ny2 = MAX4 (ty1, ty2, ty3, ty4);
    }
  else
    {
      *nx1 = x1;
      *ny1 = y1;
      *nx2 = x2;
      *ny2 = y2;
    }
}

void
gimp_display_shell_unrotate_bounds (GimpDisplayShell *shell,
                                    gdouble           x1,
                                    gdouble           y1,
                                    gdouble           x2,
                                    gdouble           y2,
                                    gdouble          *nx1,
                                    gdouble          *ny1,
                                    gdouble          *nx2,
                                    gdouble          *ny2)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->rotate_untransform)
    {
      gdouble tx1 = x1;
      gdouble ty1 = y1;
      gdouble tx2 = x1;
      gdouble ty2 = y2;
      gdouble tx3 = x2;
      gdouble ty3 = y1;
      gdouble tx4 = x2;
      gdouble ty4 = y2;

      cairo_matrix_transform_point (shell->rotate_untransform, &tx1, &ty1);
      cairo_matrix_transform_point (shell->rotate_untransform, &tx2, &ty2);
      cairo_matrix_transform_point (shell->rotate_untransform, &tx3, &ty3);
      cairo_matrix_transform_point (shell->rotate_untransform, &tx4, &ty4);

      *nx1 = MIN4 (tx1, tx2, tx3, tx4);
      *ny1 = MIN4 (ty1, ty2, ty3, ty4);
      *nx2 = MAX4 (tx1, tx2, tx3, tx4);
      *ny2 = MAX4 (ty1, ty2, ty3, ty4);
    }
  else
    {
      *nx1 = x1;
      *ny1 = y1;
      *nx2 = x2;
      *ny2 = y2;
    }
}

/**
 * gimp_display_shell_transform_coords:
 * @shell:          a #GimpDisplayShell
 * @image_coords:   image coordinates
 * @display_coords: returns the corresponding display coordinates
 *
 * Transforms from image coordinates to display coordinates, so that
 * objects can be rendered at the correct points on the display.
 **/
void
gimp_display_shell_transform_coords (GimpDisplayShell *shell,
                                     const GimpCoords *image_coords,
                                     GimpCoords       *display_coords)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (image_coords != NULL);
  g_return_if_fail (display_coords != NULL);

  *display_coords = *image_coords;

  display_coords->x = SCALEX (shell, image_coords->x);
  display_coords->y = SCALEY (shell, image_coords->y);

  display_coords->x -= shell->offset_x;
  display_coords->y -= shell->offset_y;

  if (shell->rotate_transform)
    cairo_matrix_transform_point (shell->rotate_transform,
                                  &display_coords->x,
                                  &display_coords->y);
}

/**
 * gimp_display_shell_untransform_coords:
 * @shell:          a #GimpDisplayShell
 * @display_coords: display coordinates
 * @image_coords:   returns the corresponding image coordinates
 *
 * Transforms from display coordinates to image coordinates, so that
 * points on the display can be mapped to points in the image.
 **/
void
gimp_display_shell_untransform_coords (GimpDisplayShell *shell,
                                       const GimpCoords *display_coords,
                                       GimpCoords       *image_coords)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (display_coords != NULL);
  g_return_if_fail (image_coords != NULL);

  *image_coords = *display_coords;

  if (shell->rotate_untransform)
    cairo_matrix_transform_point (shell->rotate_untransform,
                                  &image_coords->x,
                                  &image_coords->y);

  image_coords->x += shell->offset_x;
  image_coords->y += shell->offset_y;

  image_coords->x /= shell->scale_x;
  image_coords->y /= shell->scale_y;

  image_coords->xscale  = shell->scale_x;
  image_coords->yscale  = shell->scale_y;
  image_coords->angle   = shell->rotate_angle / 360.0;
  image_coords->reflect = shell->flip_horizontally ^ shell->flip_vertically;

  if (shell->flip_vertically)
    image_coords->angle += 0.5;
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
gimp_display_shell_transform_xy (GimpDisplayShell *shell,
                                 gdouble           x,
                                 gdouble           y,
                                 gint             *nx,
                                 gint             *ny)
{
  gint64 tx;
  gint64 ty;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  tx = x * shell->scale_x;
  ty = y * shell->scale_y;

  tx -= shell->offset_x;
  ty -= shell->offset_y;

  if (shell->rotate_transform)
    {
      gdouble fx = tx;
      gdouble fy = ty;

      cairo_matrix_transform_point (shell->rotate_transform, &fx, &fy);

      tx = fx;
      ty = fy;
    }

  /*  The projected coordinates might overflow a gint in the case of
   *  big images at high zoom levels, so we clamp them here to avoid
   *  problems.
   */
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
gimp_display_shell_untransform_xy (GimpDisplayShell *shell,
                                   gint              x,
                                   gint              y,
                                   gint             *nx,
                                   gint             *ny,
                                   gboolean          round)
{
  gint64 tx;
  gint64 ty;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  if (shell->rotate_untransform)
    {
      gdouble fx = x;
      gdouble fy = y;

      cairo_matrix_transform_point (shell->rotate_untransform, &fx, &fy);

      x = fx;
      y = fy;
    }

  if (round)
    {
      tx = SIGNED_ROUND (((gdouble) x + shell->offset_x) / shell->scale_x);
      ty = SIGNED_ROUND (((gdouble) y + shell->offset_y) / shell->scale_y);
    }
  else
    {
      tx = ((gint64) x + shell->offset_x) / shell->scale_x;
      ty = ((gint64) y + shell->offset_y) / shell->scale_y;
    }

  *nx = CLAMP (tx, G_MININT, G_MAXINT);
  *ny = CLAMP (ty, G_MININT, G_MAXINT);
}

/**
 * gimp_display_shell_transform_xy_f:
 * @shell: a #GimpDisplayShell
 * @x:     image x coordinate of point
 * @y:     image y coordinate of point
 * @nx:    returned shell canvas x coordinate
 * @ny:    returned shell canvas y coordinate
 *
 * Transforms from image coordinates to display shell canvas
 * coordinates.
 **/
void
gimp_display_shell_transform_xy_f (GimpDisplayShell *shell,
                                   gdouble           x,
                                   gdouble           y,
                                   gdouble          *nx,
                                   gdouble          *ny)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  *nx = SCALEX (shell, x) - shell->offset_x;
  *ny = SCALEY (shell, y) - shell->offset_y;

  if (shell->rotate_transform)
    cairo_matrix_transform_point (shell->rotate_transform, nx, ny);
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
gimp_display_shell_untransform_xy_f (GimpDisplayShell *shell,
                                     gdouble           x,
                                     gdouble           y,
                                     gdouble          *nx,
                                     gdouble          *ny)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  if (shell->rotate_untransform)
    cairo_matrix_transform_point (shell->rotate_untransform, &x, &y);

  *nx = (x + shell->offset_x) / shell->scale_x;
  *ny = (y + shell->offset_y) / shell->scale_y;
}

void
gimp_display_shell_transform_bounds (GimpDisplayShell *shell,
                                     gdouble           x1,
                                     gdouble           y1,
                                     gdouble           x2,
                                     gdouble           y2,
                                     gdouble          *nx1,
                                     gdouble          *ny1,
                                     gdouble          *nx2,
                                     gdouble          *ny2)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx1 != NULL);
  g_return_if_fail (ny1 != NULL);
  g_return_if_fail (nx2 != NULL);
  g_return_if_fail (ny2 != NULL);

  if (shell->rotate_transform)
    {
      gdouble tx1, ty1;
      gdouble tx2, ty2;
      gdouble tx3, ty3;
      gdouble tx4, ty4;

      gimp_display_shell_transform_xy_f_noround (shell, x1, y1, &tx1, &ty1);
      gimp_display_shell_transform_xy_f_noround (shell, x1, y2, &tx2, &ty2);
      gimp_display_shell_transform_xy_f_noround (shell, x2, y1, &tx3, &ty3);
      gimp_display_shell_transform_xy_f_noround (shell, x2, y2, &tx4, &ty4);

      *nx1 = MIN4 (tx1, tx2, tx3, tx4);
      *ny1 = MIN4 (ty1, ty2, ty3, ty4);
      *nx2 = MAX4 (tx1, tx2, tx3, tx4);
      *ny2 = MAX4 (ty1, ty2, ty3, ty4);
    }
  else
    {
      gimp_display_shell_transform_xy_f_noround (shell, x1, y1, nx1, ny1);
      gimp_display_shell_transform_xy_f_noround (shell, x2, y2, nx2, ny2);
    }
}

void
gimp_display_shell_untransform_bounds (GimpDisplayShell *shell,
                                       gdouble           x1,
                                       gdouble           y1,
                                       gdouble           x2,
                                       gdouble           y2,
                                       gdouble          *nx1,
                                       gdouble          *ny1,
                                       gdouble          *nx2,
                                       gdouble          *ny2)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (nx1 != NULL);
  g_return_if_fail (ny1 != NULL);
  g_return_if_fail (nx2 != NULL);
  g_return_if_fail (ny2 != NULL);

  if (shell->rotate_untransform)
    {
      gdouble tx1, ty1;
      gdouble tx2, ty2;
      gdouble tx3, ty3;
      gdouble tx4, ty4;

      gimp_display_shell_untransform_xy_f (shell, x1, y1, &tx1, &ty1);
      gimp_display_shell_untransform_xy_f (shell, x1, y2, &tx2, &ty2);
      gimp_display_shell_untransform_xy_f (shell, x2, y1, &tx3, &ty3);
      gimp_display_shell_untransform_xy_f (shell, x2, y2, &tx4, &ty4);

      *nx1 = MIN4 (tx1, tx2, tx3, tx4);
      *ny1 = MIN4 (ty1, ty2, ty3, ty4);
      *nx2 = MAX4 (tx1, tx2, tx3, tx4);
      *ny2 = MAX4 (ty1, ty2, ty3, ty4);
    }
  else
    {
      gimp_display_shell_untransform_xy_f (shell, x1, y1, nx1, ny1);
      gimp_display_shell_untransform_xy_f (shell, x2, y2, nx2, ny2);
    }
}

/* transforms a bounding box from image-space, uniformly scaled by a factor of
 * 'scale', to display-space.  this is equivalent to, but more accurate than,
 * dividing the input by 'scale', and using
 * gimp_display_shell_transform_bounds(), in particular, in that if 'scale'
 * equals 'shell->scale_x' or 'shell->scale_y', there is no loss in accuracy
 * in the corresponding dimension due to scaling (although there might be loss
 * of accuracy due to rotation or translation.)
 */
void
gimp_display_shell_transform_bounds_with_scale (GimpDisplayShell *shell,
                                                gdouble           scale,
                                                gdouble           x1,
                                                gdouble           y1,
                                                gdouble           x2,
                                                gdouble           y2,
                                                gdouble          *nx1,
                                                gdouble          *ny1,
                                                gdouble          *nx2,
                                                gdouble          *ny2)
{
  gdouble factor_x;
  gdouble factor_y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (scale > 0.0);
  g_return_if_fail (nx1 != NULL);
  g_return_if_fail (ny1 != NULL);
  g_return_if_fail (nx2 != NULL);
  g_return_if_fail (ny2 != NULL);

  factor_x = shell->scale_x / scale;
  factor_y = shell->scale_y / scale;

  x1 = x1 * factor_x - shell->offset_x;
  y1 = y1 * factor_y - shell->offset_y;
  x2 = x2 * factor_x - shell->offset_x;
  y2 = y2 * factor_y - shell->offset_y;

  gimp_display_shell_rotate_bounds (shell,
                                    x1,  y1,  x2,  y2,
                                    nx1, ny1, nx2, ny2);
}

/* transforms a bounding box from display-space to image-space, uniformly
 * scaled by a factor of 'scale'.  this is equivalent to, but more accurate
 * than, using gimp_display_shell_untransform_bounds(), and multiplying the
 * output by 'scale', in particular, in that if 'scale' equals 'shell->scale_x'
 * or 'shell->scale_y', there is no loss in accuracy in the corresponding
 * dimension due to scaling (although there might be loss of accuracy due to
 * rotation or translation.)
 */
void
gimp_display_shell_untransform_bounds_with_scale (GimpDisplayShell *shell,
                                                  gdouble           scale,
                                                  gdouble           x1,
                                                  gdouble           y1,
                                                  gdouble           x2,
                                                  gdouble           y2,
                                                  gdouble          *nx1,
                                                  gdouble          *ny1,
                                                  gdouble          *nx2,
                                                  gdouble          *ny2)
{
  gdouble factor_x;
  gdouble factor_y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (scale > 0.0);
  g_return_if_fail (nx1 != NULL);
  g_return_if_fail (ny1 != NULL);
  g_return_if_fail (nx2 != NULL);
  g_return_if_fail (ny2 != NULL);

  factor_x = scale / shell->scale_x;
  factor_y = scale / shell->scale_y;

  gimp_display_shell_unrotate_bounds (shell,
                                      x1,  y1,  x2,  y2,
                                      nx1, ny1, nx2, ny2);

  *nx1 = (*nx1 + shell->offset_x) * factor_x;
  *ny1 = (*ny1 + shell->offset_y) * factor_y;
  *nx2 = (*nx2 + shell->offset_x) * factor_x;
  *ny2 = (*ny2 + shell->offset_y) * factor_y;
}

/**
 * gimp_display_shell_untransform_viewport:
 * @shell:  a #GimpDisplayShell
 * @clip:   whether to clip the result to the image bounds
 * @x:      returns image x coordinate of display upper left corner
 * @y:      returns image y coordinate of display upper left corner
 * @width:  returns width of display measured in image coordinates
 * @height: returns height of display measured in image coordinates
 *
 * This function calculates the part of the image, in image coordinates,
 * that corresponds to the display viewport.
 **/
void
gimp_display_shell_untransform_viewport (GimpDisplayShell *shell,
                                         gboolean          clip,
                                         gint             *x,
                                         gint             *y,
                                         gint             *width,
                                         gint             *height)
{
  gdouble x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_untransform_bounds (shell,
                                         0, 0,
                                         shell->disp_width, shell->disp_height,
                                         &x1, &y1,
                                         &x2, &y2);

  x1 = floor (x1);
  y1 = floor (y1);
  x2 = ceil (x2);
  y2 = ceil (y2);

  if (clip)
    {
      GimpImage *image = gimp_display_get_image (shell->display);

      x1 = MAX (x1, 0);
      y1 = MAX (y1, 0);
      x2 = MIN (x2, gimp_image_get_width  (image));
      y2 = MIN (y2, gimp_image_get_height (image));
    }

  if (x)      *x      = x1;
  if (y)      *y      = y1;
  if (width)  *width  = x2 - x1;
  if (height) *height = y2 - y1;
}


/*  private functions  */

/* Same as gimp_display_shell_transform_xy_f(), but doesn't do any rounding
 * for the transformed coordinates.
 */
static void
gimp_display_shell_transform_xy_f_noround (GimpDisplayShell *shell,
                                           gdouble           x,
                                           gdouble           y,
                                           gdouble          *nx,
                                           gdouble          *ny)
{
  *nx = shell->scale_x * x - shell->offset_x;
  *ny = shell->scale_y * y - shell->offset_y;

  if (shell->rotate_transform)
    cairo_matrix_transform_point (shell->rotate_transform, nx, ny);
}
