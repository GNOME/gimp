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

#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplay-handlers.h"


/*  local function prototypes  */

static void   gimp_display_update_handler         (GimpProjection *projection,
                                                   gboolean        now,
                                                   gint            x,
                                                   gint            y,
                                                   gint            w,
                                                   gint            h,
                                                   GimpDisplay    *display);

static void   gimp_display_bounds_changed_handler (GimpImage      *image,
                                                   gint            old_x,
                                                   gint            old_y,
                                                   GimpDisplay    *display);


/*  public functions  */

void
gimp_display_connect (GimpDisplay *display)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY (display));

  image = gimp_display_get_image (display);

  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_connect (gimp_image_get_projection (image), "update",
                    G_CALLBACK (gimp_display_update_handler),
                    display);

  g_signal_connect (image, "bounds-changed",
                    G_CALLBACK (gimp_display_bounds_changed_handler),
                    display);
  g_signal_connect_swapped (image, "flush",
                            G_CALLBACK (gimp_display_flush),
                            display);
  g_signal_connect_swapped (image, "selected-layers-changed",
                            G_CALLBACK (gimp_display_flush),
                            display);
}

void
gimp_display_disconnect (GimpDisplay *display)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY (display));

  image = gimp_display_get_image (display);

  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_flush,
                                        display);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_bounds_changed_handler,
                                        display);

  g_signal_handlers_disconnect_by_func (gimp_image_get_projection (image),
                                        gimp_display_update_handler,
                                        display);
}


/*  private functions  */

static void
gimp_display_update_handler (GimpProjection *projection,
                             gboolean        now,
                             gint            x,
                             gint            y,
                             gint            w,
                             gint            h,
                             GimpDisplay    *display)
{
  gimp_display_update_area (display, now, x, y, w, h);
}

static void
gimp_display_bounds_changed_handler (GimpImage   *image,
                                     gint         old_x,
                                     gint         old_y,
                                     GimpDisplay *display)
{
  gimp_display_update_bounding_box (display);
}
