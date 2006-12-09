/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererdrawable.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawable-preview.h"
#include "core/gimpimage.h"

#include "gimpviewrendererdrawable.h"


static void   gimp_view_renderer_drawable_render (GimpViewRenderer *renderer,
                                                  GtkWidget        *widget);


G_DEFINE_TYPE (GimpViewRendererDrawable, gimp_view_renderer_drawable,
               GIMP_TYPE_VIEW_RENDERER)

#define parent_class gimp_view_renderer_drawable_parent_class


static void
gimp_view_renderer_drawable_class_init (GimpViewRendererDrawableClass *klass)
{
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  renderer_class->render = gimp_view_renderer_drawable_render;
}

static void
gimp_view_renderer_drawable_init (GimpViewRendererDrawable *renderer)
{
}

static void
gimp_view_renderer_drawable_render (GimpViewRenderer *renderer,
                                    GtkWidget        *widget)
{
  GimpDrawable *drawable;
  GimpItem     *item;
  GimpImage    *image;
  gint          width;
  gint          height;
  gint          view_width;
  gint          view_height;
  gboolean      scaling_up;
  TempBuf      *render_buf = NULL;

  drawable = GIMP_DRAWABLE (renderer->viewable);
  item     = GIMP_ITEM (drawable);
  image   = gimp_item_get_image (item);

  width  = renderer->width;
  height = renderer->height;

  if (image && ! renderer->is_popup)
    {
      width  = MAX (1, ROUND ((((gdouble) width / (gdouble) image->width) *
                               (gdouble) item->width)));
      height = MAX (1, ROUND ((((gdouble) height / (gdouble) image->height) *
                              (gdouble) item->height)));

      gimp_viewable_calc_preview_size (item->width,
                                       item->height,
                                       width,
                                       height,
                                       renderer->dot_for_dot,
                                       image->xresolution,
                                       image->yresolution,
                                       &view_width,
                                       &view_height,
                                       &scaling_up);
    }
  else
    {
      gimp_viewable_calc_preview_size (item->width,
                                       item->height,
                                       width,
                                       height,
                                       renderer->dot_for_dot,
                                       image ? image->xresolution : 1.0,
                                       image ? image->yresolution : 1.0,
                                       &view_width,
                                       &view_height,
                                       &scaling_up);
    }

  if ((view_width * view_height) < (item->width * item->height * 4))
    scaling_up = FALSE;

  if (scaling_up)
    {
      if (image && ! renderer->is_popup)
        {
          gint src_x, src_y;
          gint src_width, src_height;

          if (gimp_rectangle_intersect (0, 0,
                                        item->width, item->height,
                                        -item->offset_x, -item->offset_y,
                                        image->width, image->height,
                                        &src_x, &src_y,
                                        &src_width, &src_height))
            {
              gint dest_width;
              gint dest_height;

              dest_width  = ROUND (((gdouble) renderer->width /
                                    (gdouble) image->width) *
                                   (gdouble) src_width);
              dest_height = ROUND (((gdouble) renderer->height /
                                    (gdouble) image->height) *
                                   (gdouble) src_height);

              if (dest_width  < 1) dest_width  = 1;
              if (dest_height < 1) dest_height = 1;

              render_buf = gimp_drawable_get_sub_preview (drawable,
                                                          src_x, src_y,
                                                          src_width, src_height,
                                                          dest_width, dest_height);
            }
          else
            {
              gint   bytes    = gimp_drawable_preview_bytes (drawable);
              guchar empty[4] = { 0, 0, 0, 0 };

              render_buf = temp_buf_new (1, 1, bytes, 0, 0, empty);
            }
        }
      else
        {
          TempBuf *temp_buf;

          temp_buf = gimp_viewable_get_new_preview (renderer->viewable,
                                                    renderer->context,
                                                    item->width,
                                                    item->height);

          if (temp_buf)
            {
              render_buf = temp_buf_scale (temp_buf, view_width, view_height);

              temp_buf_free (temp_buf);
            }
        }
    }
  else
    {
      render_buf = gimp_viewable_get_new_preview (renderer->viewable,
                                                  renderer->context,
                                                  view_width,
                                                  view_height);
    }

  if (render_buf)
    {
      if (image && ! renderer->is_popup)
        {
          if (item->offset_x != 0)
            render_buf->x =
              ROUND ((((gdouble) renderer->width / (gdouble) image->width) *
                      (gdouble) item->offset_x));

          if (item->offset_y != 0)
            render_buf->y =
              ROUND ((((gdouble) renderer->height / (gdouble) image->height) *
                      (gdouble) item->offset_y));

          if (scaling_up)
            {
              if (render_buf->x < 0) render_buf->x = 0;
              if (render_buf->y < 0) render_buf->y = 0;
            }
        }
      else
        {
          if (view_width < width)
            render_buf->x = (width - view_width) / 2;

          if (view_height < height)
            render_buf->y = (height - view_height) / 2;
        }

      gimp_view_renderer_render_buffer (renderer, render_buf, -1,
                                        GIMP_VIEW_BG_CHECKS,
                                        GIMP_VIEW_BG_CHECKS);

      temp_buf_free (render_buf);
    }
  else
    {
      const gchar *stock_id;

      stock_id = gimp_viewable_get_stock_id (renderer->viewable);

      gimp_view_renderer_default_render_stock (renderer, widget, stock_id);
    }
}
