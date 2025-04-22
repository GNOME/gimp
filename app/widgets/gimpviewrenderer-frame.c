/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrenderer-frame.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
 *
 * Contains code taken from eel, the Eazel Extensions Library.
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpviewable.h"

#include "gimpviewrenderer.h"
#include "gimpviewrenderer-frame.h"
#include "gimpwidgets-utils.h"


/* utility to stretch a frame to the desired size */

static void
draw_frame_row (GdkPixbuf *frame_image,
                gint       target_width,
                gint       source_width,
                gint       source_v_position,
                gint       dest_v_position,
                GdkPixbuf *result_pixbuf,
                gint       left_offset,
                gint       height)
{
  gint remaining_width = target_width;
  gint h_offset        = 0;

  while (remaining_width > 0)
    {
      gint slab_width = (remaining_width > source_width ?
                         source_width : remaining_width);
      gdk_pixbuf_copy_area (frame_image,
                            left_offset, source_v_position,
                            slab_width, height,
                            result_pixbuf,
                            left_offset + h_offset, dest_v_position);

      remaining_width -= slab_width;
      h_offset += slab_width;
    }
}

/* utility to draw the middle section of the frame in a loop */
static void
draw_frame_column (GdkPixbuf *frame_image,
                   gint       target_height,
                   gint       source_height,
                   gint       source_h_position,
                   gint       dest_h_position,
                   GdkPixbuf *result_pixbuf,
                   gint       top_offset, int width)
{
  gint remaining_height = target_height;
  gint v_offset         = 0;

  while (remaining_height > 0)
    {
      gint slab_height = (remaining_height > source_height ?
                          source_height : remaining_height);

      gdk_pixbuf_copy_area (frame_image,
                            source_h_position, top_offset,
                            width, slab_height,
                            result_pixbuf,
                            dest_h_position, top_offset + v_offset);

      remaining_height -= slab_height;
      v_offset += slab_height;
    }
}

static GdkPixbuf *
stretch_frame_image (GdkPixbuf *frame_image,
                     gint       left_offset,
                     gint       top_offset,
                     gint       right_offset,
                     gint       bottom_offset,
                     gint       dest_width,
                     gint       dest_height)
{
  GdkPixbuf *pixbuf;
  gint       frame_width, frame_height;
  gint       target_width,  target_frame_width;
  gint       target_height, target_frame_height;

  frame_width  = gdk_pixbuf_get_width  (frame_image);
  frame_height = gdk_pixbuf_get_height (frame_image );

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           dest_width, dest_height);
  gdk_pixbuf_fill (pixbuf, 0);

  target_width  = dest_width - left_offset - right_offset;
  target_height = dest_height - top_offset - bottom_offset;

  target_frame_width  = frame_width - left_offset - right_offset;
  target_frame_height = frame_height - top_offset - bottom_offset;

  left_offset   += MIN (target_width / 4, target_frame_width / 4);
  right_offset  += MIN (target_width / 4, target_frame_width / 4);
  top_offset    += MIN (target_height / 4, target_frame_height / 4);
  bottom_offset += MIN (target_height / 4, target_frame_height / 4);

  target_width  = dest_width - left_offset - right_offset;
  target_height = dest_height - top_offset - bottom_offset;

  target_frame_width  = frame_width - left_offset - right_offset;
  target_frame_height = frame_height - top_offset - bottom_offset;

  /* draw the left top corner  and top row */
  gdk_pixbuf_copy_area (frame_image,
                        0, 0, left_offset, top_offset,
                        pixbuf, 0,  0);
  draw_frame_row (frame_image, target_width, target_frame_width,
                  0, 0,
                  pixbuf,
                  left_offset, top_offset);

  /* draw the right top corner and left column */
  gdk_pixbuf_copy_area (frame_image,
                        frame_width - right_offset, 0,
                        right_offset, top_offset,

                        pixbuf,
                        dest_width - right_offset,  0);
  draw_frame_column (frame_image, target_height, target_frame_height, 0, 0,
                     pixbuf, top_offset, left_offset);

  /* draw the bottom right corner and bottom row */
  gdk_pixbuf_copy_area (frame_image,
                        frame_width - right_offset, frame_height - bottom_offset,
                        right_offset, bottom_offset,
                        pixbuf,
                        dest_width - right_offset, dest_height - bottom_offset);
  draw_frame_row (frame_image, target_width, target_frame_width,
                  frame_height - bottom_offset, dest_height - bottom_offset,
                  pixbuf, left_offset, bottom_offset);

  /* draw the bottom left corner and the right column */
  gdk_pixbuf_copy_area (frame_image,
                        0, frame_height - bottom_offset,
                        left_offset, bottom_offset,
                        pixbuf,
                        0,  dest_height - bottom_offset);
  draw_frame_column (frame_image, target_height, target_frame_height,
                     frame_width - right_offset, dest_width - right_offset,
                     pixbuf, top_offset, right_offset);

  return pixbuf;
}

static GdkPixbuf *
gimp_view_renderer_get_frame (GimpViewRenderer *renderer,
                              gint              width,
                              gint              height)
{
  GimpViewRendererClass *class = GIMP_VIEW_RENDERER_GET_CLASS (renderer);

  return stretch_frame_image (class->frame,
                              class->frame_left,
                              class->frame_top,
                              class->frame_right,
                              class->frame_bottom,
                              width, height);
}

static void
gimp_view_renderer_ensure_frame (GimpViewRenderer *renderer,
                                 GtkWidget        *widget)
{
  GimpViewRendererClass *class = GIMP_VIEW_RENDERER_GET_CLASS (renderer);

  if (! class->frame)
    {
      class->frame = gimp_widget_load_icon (widget, GIMP_ICON_FRAME, 48);

      /*  FIXME: shouldn't be hardcoded  */
      class->frame_left   = 2;
      class->frame_top    = 2;
      class->frame_right  = 4;
      class->frame_bottom = 4;
    }
}

GdkPixbuf *
gimp_view_renderer_get_frame_pixbuf (GimpViewRenderer *renderer,
                                     GtkWidget        *widget,
                                     gint              width,
                                     gint              height)
{
  GimpViewRendererClass *class;
  GdkPixbuf             *frame;
  GdkPixbuf             *pixbuf;
  gint                   w, h;
  gint                   x, y;

  g_return_val_if_fail (GIMP_IS_VIEW_RENDERER (renderer), NULL);
  g_return_val_if_fail (GIMP_IS_VIEWABLE (renderer->viewable), NULL);

  gimp_view_renderer_ensure_frame (renderer, widget);

  class = GIMP_VIEW_RENDERER_GET_CLASS (renderer);

  w = width  - class->frame_left - class->frame_right;
  h = height - class->frame_top  - class->frame_bottom;

  if (w > 12 && h > 12)
    {
      pixbuf = gimp_viewable_get_pixbuf (renderer->viewable,
                                         renderer->context,
                                         w, h, NULL);
      if (!pixbuf)
        return NULL;

      x = class->frame_left;
      y = class->frame_top;
      w = gdk_pixbuf_get_width (pixbuf);
      h = gdk_pixbuf_get_height (pixbuf);

      frame  = gimp_view_renderer_get_frame (renderer,
                                             w + x + class->frame_right,
                                             h + y + class->frame_bottom);
    }
  else
    {
      pixbuf = gimp_viewable_get_pixbuf (renderer->viewable,
                                         renderer->context,
                                         width - 2, height - 2,
                                         NULL);
      if (!pixbuf)
        return NULL;

      /*  as fallback, render the preview with a 1 pixel wide black border  */

      x = 1;
      y = 1;
      w = gdk_pixbuf_get_width (pixbuf);
      h = gdk_pixbuf_get_height (pixbuf);

      frame = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, w + 2, h + 2);
      gdk_pixbuf_fill (frame, 0);
     }

  gdk_pixbuf_copy_area (pixbuf, 0, 0, w, h, frame, x, y);

  return frame;
}


/* This API is somewhat weird but GimpThumbBox needs these values so
 * it can request the GimpImageFile view in the proper size.
 */
void
gimp_view_renderer_get_frame_size (gint *horizontal,
                                   gint *vertical)
{
  GimpViewRendererClass *class;

  class = g_type_class_ref (GIMP_TYPE_VIEW_RENDERER);

  if (horizontal)
    *horizontal = class->frame_left + class->frame_right;

  if (vertical)
    *vertical = class->frame_top + class->frame_bottom;

  g_type_class_unref (class);
}
