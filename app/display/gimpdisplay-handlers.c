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

#include "core/ligmaimage.h"

#include "ligmadisplay.h"
#include "ligmadisplay-handlers.h"


/*  local function prototypes  */

static void   ligma_display_update_handler         (LigmaProjection *projection,
                                                   gboolean        now,
                                                   gint            x,
                                                   gint            y,
                                                   gint            w,
                                                   gint            h,
                                                   LigmaDisplay    *display);

static void   ligma_display_bounds_changed_handler (LigmaImage      *image,
                                                   gint            old_x,
                                                   gint            old_y,
                                                   LigmaDisplay    *display);
static void   ligma_display_flush_handler          (LigmaImage      *image,
                                                   gboolean        invalidate_preview,
                                                   LigmaDisplay    *display);


/*  public functions  */

void
ligma_display_connect (LigmaDisplay *display)
{
  LigmaImage *image;

  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  image = ligma_display_get_image (display);

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_connect (ligma_image_get_projection (image), "update",
                    G_CALLBACK (ligma_display_update_handler),
                    display);

  g_signal_connect (image, "bounds-changed",
                    G_CALLBACK (ligma_display_bounds_changed_handler),
                    display);
  g_signal_connect (image, "flush",
                    G_CALLBACK (ligma_display_flush_handler),
                    display);
}

void
ligma_display_disconnect (LigmaDisplay *display)
{
  LigmaImage *image;

  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  image = ligma_display_get_image (display);

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_flush_handler,
                                        display);
  g_signal_handlers_disconnect_by_func (image,
                                        ligma_display_bounds_changed_handler,
                                        display);

  g_signal_handlers_disconnect_by_func (ligma_image_get_projection (image),
                                        ligma_display_update_handler,
                                        display);
}


/*  private functions  */

static void
ligma_display_update_handler (LigmaProjection *projection,
                             gboolean        now,
                             gint            x,
                             gint            y,
                             gint            w,
                             gint            h,
                             LigmaDisplay    *display)
{
  ligma_display_update_area (display, now, x, y, w, h);
}

static void
ligma_display_bounds_changed_handler (LigmaImage   *image,
                                     gint         old_x,
                                     gint         old_y,
                                     LigmaDisplay *display)
{
  ligma_display_update_bounding_box (display);
}

static void
ligma_display_flush_handler (LigmaImage   *image,
                            gboolean     invalidate_preview,
                            LigmaDisplay *display)
{
  ligma_display_flush (display);
}
