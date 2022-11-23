/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrendererdrawable.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmaasync.h"
#include "core/ligmacancelable.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawable-preview.h"
#include "core/ligmaimage.h"
#include "core/ligmatempbuf.h"

#include "ligmaviewrendererdrawable.h"


struct _LigmaViewRendererDrawablePrivate
{
  LigmaAsync *render_async;
  GtkWidget *render_widget;
  gint       render_buf_x;
  gint       render_buf_y;
  gboolean   render_update;

  gint       prev_width;
  gint       prev_height;
};


/*  local function prototypes  */

static void   ligma_view_renderer_drawable_dispose       (GObject                  *object);

static void   ligma_view_renderer_drawable_invalidate    (LigmaViewRenderer         *renderer);
static void   ligma_view_renderer_drawable_render        (LigmaViewRenderer         *renderer,
                                                         GtkWidget                *widget);

static void   ligma_view_renderer_drawable_cancel_render (LigmaViewRendererDrawable *renderdrawable);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaViewRendererDrawable,
                            ligma_view_renderer_drawable,
                            LIGMA_TYPE_VIEW_RENDERER)

#define parent_class ligma_view_renderer_drawable_parent_class


/*  private functions  */


static void
ligma_view_renderer_drawable_class_init (LigmaViewRendererDrawableClass *klass)
{
  GObjectClass          *object_class   = G_OBJECT_CLASS (klass);
  LigmaViewRendererClass *renderer_class = LIGMA_VIEW_RENDERER_CLASS (klass);

  object_class->dispose      = ligma_view_renderer_drawable_dispose;

  renderer_class->invalidate = ligma_view_renderer_drawable_invalidate;
  renderer_class->render     = ligma_view_renderer_drawable_render;
}

static void
ligma_view_renderer_drawable_init (LigmaViewRendererDrawable *renderdrawable)
{
  renderdrawable->priv =
    ligma_view_renderer_drawable_get_instance_private (renderdrawable);
}

static void
ligma_view_renderer_drawable_dispose (GObject *object)
{
  LigmaViewRendererDrawable *renderdrawable = LIGMA_VIEW_RENDERER_DRAWABLE (object);

  ligma_view_renderer_drawable_cancel_render (renderdrawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_view_renderer_drawable_invalidate (LigmaViewRenderer *renderer)
{
  LigmaViewRendererDrawable *renderdrawable = LIGMA_VIEW_RENDERER_DRAWABLE (renderer);

  ligma_view_renderer_drawable_cancel_render (renderdrawable);

  LIGMA_VIEW_RENDERER_CLASS (parent_class)->invalidate (renderer);
}

static void
ligma_view_renderer_drawable_render_async_callback (LigmaAsync                *async,
                                                   LigmaViewRendererDrawable *renderdrawable)
{
  GtkWidget *widget;

  /* rendering was canceled, and the view renderer is potentially dead (see
   * ligma_view_renderer_drawable_cancel_render()).  bail.
   */
  if (ligma_async_is_canceled (async))
    return;

  widget = renderdrawable->priv->render_widget;

  renderdrawable->priv->render_async  = NULL;
  renderdrawable->priv->render_widget = NULL;

  if (ligma_async_is_finished (async))
    {
      LigmaViewRenderer *renderer   = LIGMA_VIEW_RENDERER (renderdrawable);
      LigmaTempBuf      *render_buf = ligma_async_get_result (async);

      ligma_view_renderer_render_temp_buf (
        renderer,
        widget,
        render_buf,
        renderdrawable->priv->render_buf_x,
        renderdrawable->priv->render_buf_y,
        -1,
        LIGMA_VIEW_BG_CHECKS,
        LIGMA_VIEW_BG_CHECKS);

      if (renderdrawable->priv->render_update)
        ligma_view_renderer_update (renderer);
    }

  g_object_unref (widget);
}

static void
ligma_view_renderer_drawable_render (LigmaViewRenderer *renderer,
                                    GtkWidget        *widget)
{
  LigmaViewRendererDrawable *renderdrawable = LIGMA_VIEW_RENDERER_DRAWABLE (renderer);
  LigmaDrawable             *drawable;
  LigmaItem                 *item;
  LigmaImage                *image;
  const gchar              *icon_name;
  LigmaAsync                *async;
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

  drawable  = LIGMA_DRAWABLE (renderer->viewable);
  item      = LIGMA_ITEM (drawable);
  image     = ligma_item_get_image (item);
  icon_name = ligma_viewable_get_icon_name (renderer->viewable);

  if (image && ! image->ligma->config->layer_previews)
    {
      renderdrawable->priv->prev_width  = 0;
      renderdrawable->priv->prev_height = 0;

      ligma_view_renderer_render_icon (renderer, widget, icon_name);

      return;
    }

  if (image)
    ligma_image_get_resolution (image, &xres, &yres);

  if (renderer->is_popup)
    image = NULL;

  if (image)
    {
      image_width  = ligma_image_get_width  (image);
      image_height = ligma_image_get_height (image);
    }
  else
    {
      image_width  = ligma_item_get_width  (item);
      image_height = ligma_item_get_height (item);
    }

  ligma_viewable_calc_preview_size (image_width,
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
  src_width  = ligma_item_get_width  (item);
  src_height = ligma_item_get_height (item);

  if (image)
    {
      gint offset_x;
      gint offset_y;

      ligma_item_get_offset (item, &offset_x, &offset_y);

      if (ligma_rectangle_intersect (src_x, src_y,
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
      async = ligma_drawable_get_sub_preview_async (drawable,
                                                   src_x, src_y,
                                                   src_width, src_height,
                                                   dst_width, dst_height);
    }
  else
    {
      const Babl  *format = ligma_drawable_get_preview_format (drawable);
      LigmaTempBuf *render_buf;

      async = ligma_async_new ();

      render_buf = ligma_temp_buf_new (dst_width, dst_height, format);
      ligma_temp_buf_data_clear (render_buf);

      ligma_async_finish_full (async,
                              render_buf,
                              (GDestroyNotify) ligma_temp_buf_unref);
    }

  if (async)
    {
      renderdrawable->priv->render_async  = async;
      renderdrawable->priv->render_widget = g_object_ref (widget);
      renderdrawable->priv->render_buf_x  = dst_x;
      renderdrawable->priv->render_buf_y  = dst_y;
      renderdrawable->priv->render_update = FALSE;

      ligma_async_add_callback_for_object (
        async,
        (LigmaAsyncCallback) ligma_view_renderer_drawable_render_async_callback,
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
              ligma_view_renderer_render_icon (renderer, widget, icon_name);
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

      ligma_view_renderer_render_icon (renderer, widget, icon_name);
    }
}

static void
ligma_view_renderer_drawable_cancel_render (LigmaViewRendererDrawable *renderdrawable)
{
  /* cancel the async render operation (if one is ongoing) without actually
   * waiting for it.  if the actual rendering hasn't started yet, it will be
   * immediately aborted; otherwise, it can't really be interrupted, so we just
   * let it go on without blocking the main thread.
   * ligma_drawable_get_sub_preview_async() can continue rendering safely even
   * after the drawable had died, and our completion callback is prepared to
   * handle cancellation.
   */
  if (renderdrawable->priv->render_async)
    {
      ligma_cancelable_cancel (
        LIGMA_CANCELABLE (renderdrawable->priv->render_async));

      renderdrawable->priv->render_async = NULL;
    }

  g_clear_object (&renderdrawable->priv->render_widget);
}
