/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererdrawable.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpasync.h"
#include "core/gimpcancelable.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-preview.h"
#include "core/gimpimage.h"
#include "core/gimptempbuf.h"

#include "gimpviewrendererdrawable.h"


struct _GimpViewRendererDrawablePrivate
{
  GimpAsync *render_async;
  GtkWidget *render_widget;
  gint       render_buf_x;
  gint       render_buf_y;
  gboolean   render_update;

  gint       prev_width;
  gint       prev_height;
};


/*  local function prototypes  */

static void   gimp_view_renderer_drawable_dispose       (GObject                  *object);

static void   gimp_view_renderer_drawable_invalidate    (GimpViewRenderer         *renderer);
static void   gimp_view_renderer_drawable_render        (GimpViewRenderer         *renderer,
                                                         GtkWidget                *widget);

static void   gimp_view_renderer_drawable_cancel_render (GimpViewRendererDrawable *renderdrawable);


G_DEFINE_TYPE_WITH_PRIVATE (GimpViewRendererDrawable,
                            gimp_view_renderer_drawable,
                            GIMP_TYPE_VIEW_RENDERER)

#define parent_class gimp_view_renderer_drawable_parent_class


/*  private functions  */


static void
gimp_view_renderer_drawable_class_init (GimpViewRendererDrawableClass *klass)
{
  GObjectClass          *object_class   = G_OBJECT_CLASS (klass);
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  object_class->dispose      = gimp_view_renderer_drawable_dispose;

  renderer_class->invalidate = gimp_view_renderer_drawable_invalidate;
  renderer_class->render     = gimp_view_renderer_drawable_render;
}

static void
gimp_view_renderer_drawable_init (GimpViewRendererDrawable *renderdrawable)
{
  renderdrawable->priv =
    gimp_view_renderer_drawable_get_instance_private (renderdrawable);
}

static void
gimp_view_renderer_drawable_dispose (GObject *object)
{
  GimpViewRendererDrawable *renderdrawable = GIMP_VIEW_RENDERER_DRAWABLE (object);

  gimp_view_renderer_drawable_cancel_render (renderdrawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_view_renderer_drawable_invalidate (GimpViewRenderer *renderer)
{
  GimpViewRendererDrawable *renderdrawable = GIMP_VIEW_RENDERER_DRAWABLE (renderer);

  gimp_view_renderer_drawable_cancel_render (renderdrawable);

  GIMP_VIEW_RENDERER_CLASS (parent_class)->invalidate (renderer);
}

static void
gimp_view_renderer_drawable_render_async_callback (GimpAsync                *async,
                                                   GimpViewRendererDrawable *renderdrawable)
{
  GtkWidget *widget;

  /* rendering was canceled, and the view renderer is potentially dead (see
   * gimp_view_renderer_drawable_cancel_render()).  bail.
   */
  if (gimp_async_is_canceled (async))
    return;

  widget = renderdrawable->priv->render_widget;

  renderdrawable->priv->render_async  = NULL;
  renderdrawable->priv->render_widget = NULL;

  if (gimp_async_is_finished (async))
    {
      GimpViewRenderer *renderer   = GIMP_VIEW_RENDERER (renderdrawable);
      GimpTempBuf      *render_buf = gimp_async_get_result (async);

      gimp_view_renderer_render_temp_buf (
        renderer,
        widget,
        render_buf,
        renderdrawable->priv->render_buf_x,
        renderdrawable->priv->render_buf_y,
        -1,
        GIMP_VIEW_BG_CHECKS,
        GIMP_VIEW_BG_CHECKS);

      if (renderdrawable->priv->render_update)
        gimp_view_renderer_update (renderer);
    }

  g_object_unref (widget);
}

static void
gimp_view_renderer_drawable_render (GimpViewRenderer *renderer,
                                    GtkWidget        *widget)
{
  GimpViewRendererDrawable *renderdrawable = GIMP_VIEW_RENDERER_DRAWABLE (renderer);
  GimpDrawable             *drawable;
  GimpItem                 *item;
  GimpImage                *image;
  const gchar              *icon_name;
  GimpAsync                *async;
  gint                      image_width;
  gint                      image_height;
  gint                      view_width;
  gint                      view_height;
  gint                      src_x;
  gint                      src_y;
  gint                      src_width;
  gint                      src_height;
  gint                      dst_x;
  gint                      dst_y;
  gint                      dst_width;
  gint                      dst_height;
  gdouble                   xres  = 1.0;
  gdouble                   yres  = 1.0;
  gboolean                  empty = FALSE;

  /* render is already in progress */
  if (renderdrawable->priv->render_async)
    return;

  drawable  = GIMP_DRAWABLE (renderer->viewable);
  item      = GIMP_ITEM (drawable);
  image     = gimp_item_get_image (item);
  icon_name = gimp_viewable_get_icon_name (renderer->viewable);

  if (image && ! image->gimp->config->layer_previews)
    {
      renderdrawable->priv->prev_width  = 0;
      renderdrawable->priv->prev_height = 0;

      gimp_view_renderer_render_icon (renderer, widget, icon_name);

      return;
    }

  if (image)
    gimp_image_get_resolution (image, &xres, &yres);

  if (renderer->is_popup)
    image = NULL;

  if (image)
    {
      image_width  = gimp_image_get_width  (image);
      image_height = gimp_image_get_height (image);
    }
  else
    {
      image_width  = gimp_item_get_width  (item);
      image_height = gimp_item_get_height (item);
    }

  gimp_viewable_calc_preview_size (image_width,
                                   image_height,
                                   renderer->width,
                                   renderer->height,
                                   renderer->dot_for_dot,
                                   xres,
                                   yres,
                                   &view_width,
                                   &view_height,
                                   NULL);

  src_x      = 0;
  src_y      = 0;
  src_width  = gimp_item_get_width  (item);
  src_height = gimp_item_get_height (item);

  if (image)
    {
      gint offset_x;
      gint offset_y;

      gimp_item_get_offset (item, &offset_x, &offset_y);

      if (gimp_rectangle_intersect (src_x, src_y,
                                    src_width, src_height,
                                    -offset_x, -offset_y,
                                    image_width, image_height,
                                    &src_x, &src_y,
                                    &src_width, &src_height))
        {
          offset_x += src_x;
          offset_y += src_y;

          dst_x      = ROUND (((gdouble) view_width  / image_width)  *
                              offset_x);
          dst_y      = ROUND (((gdouble) view_height / image_height) *
                              offset_y);
          dst_width  = ROUND (((gdouble) view_width  / image_width)  *
                              src_width);
          dst_height = ROUND (((gdouble) view_height / image_height) *
                              src_height);
        }
      else
        {
          dst_x      = 0;
          dst_y      = 0;
          dst_width  = 1;
          dst_height = 1;

          empty = TRUE;
        }
    }
  else
    {
      dst_x      = (renderer->width  - view_width)  / 2;
      dst_y      = (renderer->height - view_height) / 2;
      dst_width  = view_width;
      dst_height = view_height;
    }

  dst_width  = MAX (dst_width,  1);
  dst_height = MAX (dst_height, 1);

  if (! empty)
    {
      async = gimp_drawable_get_sub_preview_async (drawable,
                                                   src_x, src_y,
                                                   src_width, src_height,
                                                   dst_width, dst_height);
    }
  else
    {
      const Babl  *format = gimp_drawable_get_preview_format (drawable);
      GimpTempBuf *render_buf;

      async = gimp_async_new ();

      render_buf = gimp_temp_buf_new (dst_width, dst_height, format);
      gimp_temp_buf_data_clear (render_buf);

      gimp_async_finish_full (async,
                              render_buf,
                              (GDestroyNotify) gimp_temp_buf_unref);
    }

  if (async)
    {
      renderdrawable->priv->render_async  = async;
      renderdrawable->priv->render_widget = g_object_ref (widget);
      renderdrawable->priv->render_buf_x  = dst_x;
      renderdrawable->priv->render_buf_y  = dst_y;
      renderdrawable->priv->render_update = FALSE;

      gimp_async_add_callback_for_object (
        async,
        (GimpAsyncCallback) gimp_view_renderer_drawable_render_async_callback,
        renderdrawable,
        renderdrawable);

      /* if rendering isn't done yet, update the render-view once it is, and
       * either keep the old drawable preview for now, or, if size changed (or
       * there's no old preview,) render an icon in the meantime.
       */
      if (renderdrawable->priv->render_async)
        {
          renderdrawable->priv->render_update = TRUE;

          if (renderer->width  != renderdrawable->priv->prev_width ||
              renderer->height != renderdrawable->priv->prev_height)
            {
              gimp_view_renderer_render_icon (renderer, widget, icon_name);
            }
        }

      renderdrawable->priv->prev_width  = renderer->width;
      renderdrawable->priv->prev_height = renderer->height;

      g_object_unref (async);
    }
  else
    {
      renderdrawable->priv->prev_width  = 0;
      renderdrawable->priv->prev_height = 0;

      gimp_view_renderer_render_icon (renderer, widget, icon_name);
    }
}

static void
gimp_view_renderer_drawable_cancel_render (GimpViewRendererDrawable *renderdrawable)
{
  /* cancel the async render operation (if one is ongoing) without actually
   * waiting for it.  if the actual rendering hasn't started yet, it will be
   * immediately aborted; otherwise, it can't really be interrupted, so we just
   * let it go on without blocking the main thread.
   * gimp_drawable_get_sub_preview_async() can continue rendering safely even
   * after the drawable had died, and our completion callback is prepared to
   * handle cancellation.
   */
  if (renderdrawable->priv->render_async)
    {
      gimp_cancelable_cancel (
        GIMP_CANCELABLE (renderdrawable->priv->render_async));

      renderdrawable->priv->render_async = NULL;
    }

  g_clear_object (&renderdrawable->priv->render_widget);
}
