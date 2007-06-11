/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrenderer.c
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"
#include "core/gimpviewable.h"

#include "gimprender.h"
#include "gimpviewrenderer.h"
#include "gimpviewrenderer-utils.h"
#include "gimpwidgets-utils.h"


enum
{
  UPDATE,
  LAST_SIGNAL
};


static void      gimp_view_renderer_dispose          (GObject            *object);
static void      gimp_view_renderer_finalize         (GObject            *object);

static gboolean  gimp_view_renderer_idle_update      (GimpViewRenderer   *renderer);
static void      gimp_view_renderer_real_set_context (GimpViewRenderer   *renderer,
                                                      GimpContext        *context);
static void      gimp_view_renderer_real_invalidate  (GimpViewRenderer   *renderer);
static void      gimp_view_renderer_real_draw        (GimpViewRenderer   *renderer,
                                                      GdkWindow          *window,
                                                      GtkWidget          *widget,
                                                      const GdkRectangle *draw_area,
                                                      const GdkRectangle *expose_area);
static void      gimp_view_renderer_real_render      (GimpViewRenderer   *renderer,
                                                      GtkWidget          *widget);

static void      gimp_view_renderer_size_changed     (GimpViewRenderer   *renderer,
                                                      GimpViewable       *viewable);
static GdkGC   * gimp_view_renderer_create_gc        (GimpViewRenderer   *renderer,
                                                      GdkWindow          *window,
                                                      GtkWidget          *widget);


G_DEFINE_TYPE (GimpViewRenderer, gimp_view_renderer, G_TYPE_OBJECT)

#define parent_class gimp_view_renderer_parent_class

static guint renderer_signals[LAST_SIGNAL] = { 0 };

static GimpRGB  black_color;
static GimpRGB  white_color;
static GimpRGB  green_color;
static GimpRGB  red_color;


static void
gimp_view_renderer_class_init (GimpViewRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  renderer_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpViewRendererClass, update),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->dispose  = gimp_view_renderer_dispose;
  object_class->finalize = gimp_view_renderer_finalize;

  klass->update          = NULL;
  klass->set_context     = gimp_view_renderer_real_set_context;
  klass->invalidate      = gimp_view_renderer_real_invalidate;
  klass->draw            = gimp_view_renderer_real_draw;
  klass->render          = gimp_view_renderer_real_render;

  klass->frame           = NULL;
  klass->frame_left      = 0;
  klass->frame_right     = 0;
  klass->frame_top       = 0;
  klass->frame_bottom    = 0;

  gimp_rgba_set (&black_color, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&white_color, 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&green_color, 0.0, 0.94, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&red_color,   1.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
}

static void
gimp_view_renderer_init (GimpViewRenderer *renderer)
{
  renderer->context       = NULL;

  renderer->viewable_type = G_TYPE_NONE;
  renderer->viewable      = NULL;

  renderer->width         = 0;
  renderer->height        = 0;
  renderer->border_width  = 0;
  renderer->dot_for_dot   = TRUE;
  renderer->is_popup      = FALSE;

  renderer->border_type   = GIMP_VIEW_BORDER_BLACK;
  renderer->border_color  = black_color;
  renderer->gc            = NULL;

  renderer->buffer        = NULL;
  renderer->rowstride     = 0;
  renderer->bytes         = 3;

  renderer->pixbuf        = NULL;
  renderer->bg_stock_id   = NULL;

  renderer->size          = -1;
  renderer->needs_render  = TRUE;
  renderer->idle_id       = 0;
}

static void
gimp_view_renderer_dispose (GObject *object)
{
  GimpViewRenderer *renderer = GIMP_VIEW_RENDERER (object);

  if (renderer->viewable)
    gimp_view_renderer_set_viewable (renderer, NULL);

  if (renderer->context)
    gimp_view_renderer_set_context (renderer, NULL);

  gimp_view_renderer_remove_idle (renderer);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_view_renderer_finalize (GObject *object)
{
  GimpViewRenderer *renderer = GIMP_VIEW_RENDERER (object);

  if (renderer->buffer)
    {
      g_free (renderer->buffer);
      renderer->buffer = NULL;
    }

  if (renderer->pixbuf)
    {
      g_object_unref (renderer->pixbuf);
      renderer->pixbuf = NULL;
    }

  if (renderer->bg_stock_id)
    {
      g_free (renderer->bg_stock_id);
      renderer->bg_stock_id = NULL;
    }

  if (renderer->gc)
    {
      g_object_unref (renderer->gc);
      renderer->gc = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GimpViewRenderer *
gimp_view_renderer_new_internal (GimpContext *context,
                                 GType        viewable_type,
                                 gboolean     is_popup)
{
  GimpViewRenderer *renderer;

  renderer = g_object_new (gimp_view_renderer_type_from_viewable_type (viewable_type),
                           NULL);

  renderer->viewable_type = viewable_type;
  renderer->is_popup      = is_popup ? TRUE : FALSE;

  if (context)
    gimp_view_renderer_set_context (renderer, context);

  return renderer;
}


/*  public functions  */

GimpViewRenderer *
gimp_view_renderer_new (GimpContext *context,
                        GType        viewable_type,
                        gint         size,
                        gint         border_width,
                        gboolean    is_popup)
{
  GimpViewRenderer *renderer;

  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (g_type_is_a (viewable_type, GIMP_TYPE_VIEWABLE), NULL);
  g_return_val_if_fail (size >  0 &&
                        size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (border_width >= 0 &&
                        border_width <= GIMP_VIEW_MAX_BORDER_WIDTH, NULL);

  renderer = gimp_view_renderer_new_internal (context, viewable_type,
                                              is_popup);

  gimp_view_renderer_set_size (renderer, size, border_width);
  gimp_view_renderer_remove_idle (renderer);

  return renderer;
}

GimpViewRenderer *
gimp_view_renderer_new_full (GimpContext *context,
                             GType        viewable_type,
                             gint         width,
                             gint         height,
                             gint         border_width,
                             gboolean     is_popup)
{
  GimpViewRenderer *renderer;

  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (g_type_is_a (viewable_type, GIMP_TYPE_VIEWABLE), NULL);
  g_return_val_if_fail (width >  0 &&
                        width <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (height > 0 &&
                        height <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (border_width >= 0 &&
                        border_width <= GIMP_VIEW_MAX_BORDER_WIDTH, NULL);

  renderer = gimp_view_renderer_new_internal (context, viewable_type,
                                              is_popup);

  gimp_view_renderer_set_size_full (renderer, width, height, border_width);
  gimp_view_renderer_remove_idle (renderer);

  return renderer;
}

void
gimp_view_renderer_set_context (GimpViewRenderer *renderer,
                                GimpContext      *context)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (context != renderer->context)
    {
      GIMP_VIEW_RENDERER_GET_CLASS (renderer)->set_context (renderer,
                                                            context);

      if (renderer->viewable)
        gimp_view_renderer_invalidate (renderer);
    }
}

void
gimp_view_renderer_set_viewable (GimpViewRenderer *renderer,
                                 GimpViewable     *viewable)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable));

  if (viewable)
    g_return_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (viewable),
                                   renderer->viewable_type));

  if (viewable == renderer->viewable)
    return;

  if (renderer->buffer)
    {
      g_free (renderer->buffer);
      renderer->buffer = NULL;
    }

  if (renderer->pixbuf)
    {
      g_object_unref (renderer->pixbuf);
      renderer->pixbuf = NULL;
    }

  if (renderer->viewable)
    {
      g_object_remove_weak_pointer (G_OBJECT (renderer->viewable),
                                    (gpointer) &renderer->viewable);

      g_signal_handlers_disconnect_by_func (renderer->viewable,
                                            G_CALLBACK (gimp_view_renderer_invalidate),
                                            renderer);

      g_signal_handlers_disconnect_by_func (renderer->viewable,
                                            G_CALLBACK (gimp_view_renderer_size_changed),
                                            renderer);
    }

  renderer->viewable = viewable;

  if (renderer->viewable)
    {
      g_object_add_weak_pointer (G_OBJECT (renderer->viewable),
                                 (gpointer) &renderer->viewable);

      g_signal_connect_swapped (renderer->viewable,
                                "invalidate-preview",
                                G_CALLBACK (gimp_view_renderer_invalidate),
                                renderer);

      g_signal_connect_swapped (renderer->viewable,
                                "size-changed",
                                G_CALLBACK (gimp_view_renderer_size_changed),
                                renderer);

      if (renderer->size != -1)
        gimp_view_renderer_set_size (renderer, renderer->size,
                                     renderer->border_width);

      gimp_view_renderer_invalidate (renderer);
    }
  else
    {
      gimp_view_renderer_update_idle (renderer);
    }
}

void
gimp_view_renderer_set_size (GimpViewRenderer *renderer,
                             gint              view_size,
                             gint              border_width)
{
  gint width, height;

  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (view_size >  0 &&
                    view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (border_width >= 0 &&
                    border_width <= GIMP_VIEW_MAX_BORDER_WIDTH);

  renderer->size = view_size;

  if (renderer->viewable)
    {
      gimp_viewable_get_preview_size (renderer->viewable,
                                      view_size,
                                      renderer->is_popup,
                                      renderer->dot_for_dot,
                                      &width, &height);
    }
  else
    {
      width  = view_size;
      height = view_size;
    }

  gimp_view_renderer_set_size_full (renderer, width, height, border_width);
}

void
gimp_view_renderer_set_size_full (GimpViewRenderer *renderer,
                                  gint              width,
                                  gint              height,
                                  gint              border_width)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (width >  0 &&
                    width <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (height > 0 &&
                    height <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (border_width >= 0 &&
                    border_width <= GIMP_VIEW_MAX_BORDER_WIDTH);

  if (width        != renderer->width  ||
      height       != renderer->height ||
      border_width != renderer->border_width)
    {
      renderer->width        = width;
      renderer->height       = height;
      renderer->border_width = border_width;

      renderer->rowstride = (renderer->width * renderer->bytes + 3) & ~3;

      if (renderer->buffer)
        {
          g_free (renderer->buffer);
          renderer->buffer = NULL;
        }

      if (renderer->viewable)
        gimp_view_renderer_invalidate (renderer);
    }
}

void
gimp_view_renderer_set_dot_for_dot (GimpViewRenderer *renderer,
                                    gboolean             dot_for_dot)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));

  if (dot_for_dot != renderer->dot_for_dot)
    {
      renderer->dot_for_dot = dot_for_dot ? TRUE: FALSE;

      if (renderer->size != -1)
        gimp_view_renderer_set_size (renderer, renderer->size,
                                     renderer->border_width);

      gimp_view_renderer_invalidate (renderer);
    }
}

void
gimp_view_renderer_set_border_type (GimpViewRenderer   *renderer,
                                    GimpViewBorderType  border_type)
{
  GimpRGB *border_color = &black_color;

  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));

  renderer->border_type = border_type;

  switch (border_type)
    {
    case GIMP_VIEW_BORDER_BLACK:
      border_color = &black_color;
      break;
    case GIMP_VIEW_BORDER_WHITE:
      border_color = &white_color;
      break;
    case GIMP_VIEW_BORDER_GREEN:
      border_color = &green_color;
      break;
    case GIMP_VIEW_BORDER_RED:
      border_color = &red_color;
      break;
    }

  gimp_view_renderer_set_border_color (renderer, border_color);
}

void
gimp_view_renderer_set_border_color (GimpViewRenderer *renderer,
                                     const GimpRGB    *color)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (color != NULL);

  if (gimp_rgb_distance (&renderer->border_color, color))
    {
      renderer->border_color = *color;

      if (renderer->gc)
        {
          GdkColor gdk_color;

          gimp_rgb_get_gdk_color (&renderer->border_color, &gdk_color);
          gdk_gc_set_rgb_fg_color (renderer->gc, &gdk_color);
        }

      gimp_view_renderer_update_idle (renderer);
    }
}

void
gimp_view_renderer_set_background (GimpViewRenderer *renderer,
                                   const gchar      *stock_id)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));

  if (renderer->bg_stock_id)
    g_free (renderer->bg_stock_id);

  renderer->bg_stock_id = g_strdup (stock_id);

  if (renderer->gc)
    {
      g_object_unref (renderer->gc);
      renderer->gc = NULL;
    }
}

void
gimp_view_renderer_unrealize (GimpViewRenderer *renderer)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));

  if (renderer->gc)
    {
      g_object_unref (renderer->gc);
      renderer->gc = NULL;
    }
}

void
gimp_view_renderer_invalidate (GimpViewRenderer *renderer)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));

  if (renderer->idle_id)
    {
      g_source_remove (renderer->idle_id);
      renderer->idle_id = 0;
    }

  GIMP_VIEW_RENDERER_GET_CLASS (renderer)->invalidate (renderer);

  renderer->idle_id =
    g_idle_add_full (GIMP_VIEWABLE_PRIORITY_IDLE,
                     (GSourceFunc) gimp_view_renderer_idle_update,
                     renderer, NULL);
}

void
gimp_view_renderer_update (GimpViewRenderer *renderer)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));

  if (renderer->idle_id)
    {
      g_source_remove (renderer->idle_id);
      renderer->idle_id = 0;
    }

  g_signal_emit (renderer, renderer_signals[UPDATE], 0);
}

void
gimp_view_renderer_update_idle (GimpViewRenderer *renderer)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));

  if (renderer->idle_id)
    g_source_remove (renderer->idle_id);

  renderer->idle_id =
    g_idle_add_full (GIMP_VIEWABLE_PRIORITY_IDLE,
                     (GSourceFunc) gimp_view_renderer_idle_update,
                     renderer, NULL);
}

void
gimp_view_renderer_remove_idle (GimpViewRenderer *renderer)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));

  if (renderer->idle_id)
    {
      g_source_remove (renderer->idle_id);
      renderer->idle_id = 0;
    }
}

void
gimp_view_renderer_draw (GimpViewRenderer   *renderer,
                         GdkWindow          *window,
                         GtkWidget          *widget,
                         const GdkRectangle *draw_area,
                         const GdkRectangle *expose_area)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (draw_area != NULL);
  g_return_if_fail (expose_area != NULL);

  if (! GTK_WIDGET_DRAWABLE (widget))
    return;

  if (renderer->viewable)
    {
      GIMP_VIEW_RENDERER_GET_CLASS (renderer)->draw (renderer,
                                                     window, widget,
                                                     draw_area,
                                                     expose_area);
    }
  else
    {
      GimpViewableClass *viewable_class;

      viewable_class = g_type_class_ref (renderer->viewable_type);

      gimp_view_renderer_default_render_stock (renderer,
                                               widget,
                                               viewable_class->default_stock_id);

      g_type_class_unref (viewable_class);

      gimp_view_renderer_real_draw (renderer,
                                    window, widget, draw_area, expose_area);
    }

  if (renderer->border_width > 0)
    {
      GdkRectangle rect;
      gint         i;

      rect.width  = renderer->width  + 2 * renderer->border_width;
      rect.height = renderer->height + 2 * renderer->border_width;
      rect.x      = draw_area->x + (draw_area->width  - rect.width)  / 2;
      rect.y      = draw_area->y + (draw_area->height - rect.height) / 2;

      if (! renderer->gc)
        renderer->gc = gimp_view_renderer_create_gc (renderer,
                                                     window, widget);

      for (i = 0; i < renderer->border_width; i++)
        gdk_draw_rectangle (window,
                            renderer->gc,
                            FALSE,
                            rect.x + i,
                            rect.y + i,
                            rect.width  - 2 * i - 1,
                            rect.height - 2 * i - 1);
    }

  if (! renderer->context)
    {
      GdkRectangle rect;

      rect.width  = renderer->width  + 2 * renderer->border_width;
      rect.height = renderer->height + 2 * renderer->border_width;
      rect.x      = draw_area->x + (draw_area->width  - rect.width)  / 2;
      rect.y      = draw_area->y + (draw_area->height - rect.height) / 2;

      if (! renderer->gc)
        renderer->gc = gimp_view_renderer_create_gc (renderer,
                                                     window, widget);

      gdk_draw_line (window,
                     renderer->gc,
                     rect.x,
                     rect.y,
                     rect.x + rect.width  - 1,
                     rect.y + rect.height - 1);
    }
}


/*  private functions  */

static gboolean
gimp_view_renderer_idle_update (GimpViewRenderer *renderer)
{
  renderer->idle_id = 0;

  gimp_view_renderer_update (renderer);

  return FALSE;
}

static void
gimp_view_renderer_real_set_context (GimpViewRenderer *renderer,
                                     GimpContext      *context)
{
  if (renderer->context)
    g_object_unref (renderer->context);

  renderer->context = context;

  if (renderer->context)
    g_object_ref (renderer->context);
}

static void
gimp_view_renderer_real_invalidate (GimpViewRenderer *renderer)
{
  renderer->needs_render = TRUE;
}

static void
gimp_view_renderer_real_draw (GimpViewRenderer   *renderer,
                              GdkWindow          *window,
                              GtkWidget          *widget,
                              const GdkRectangle *draw_area,
                              const GdkRectangle *expose_area)
{
  GdkRectangle rect;
  GdkRectangle render_rect;

  if (renderer->needs_render)
    GIMP_VIEW_RENDERER_GET_CLASS (renderer)->render (renderer, widget);

  if (renderer->pixbuf)
    {
      if (renderer->bg_stock_id)
        {
          if (!renderer->gc)
            renderer->gc = gimp_view_renderer_create_gc (renderer,
                                                         window, widget);

          if (gdk_rectangle_intersect ((GdkRectangle *) draw_area,
                                       (GdkRectangle *) expose_area,
                                       &render_rect))
            {
              gdk_draw_rectangle (GDK_DRAWABLE (window), renderer->gc,
                                  TRUE,
                                  render_rect.x,     render_rect.y,
                                  render_rect.width, render_rect.height);
            }
        }

      rect.width  = gdk_pixbuf_get_width  (renderer->pixbuf);
      rect.height = gdk_pixbuf_get_height (renderer->pixbuf);
      rect.x      = draw_area->x + (draw_area->width  - rect.width)  / 2;
      rect.y      = draw_area->y + (draw_area->height - rect.height) / 2;

      if (gdk_rectangle_intersect (&rect, (GdkRectangle *) expose_area,
                                   &render_rect))
        {
          gdk_draw_pixbuf (GDK_DRAWABLE (window),
                           widget->style->bg_gc[widget->state],
                           renderer->pixbuf,
                           render_rect.x - rect.x,
                           render_rect.y - rect.y,
                           render_rect.x,
                           render_rect.y,
                           render_rect.width,
                           render_rect.height,
                           GDK_RGB_DITHER_NORMAL,
                           0, 0);
        }
    }
  else if (renderer->buffer)
    {
      rect.width  = renderer->width;
      rect.height = renderer->height;
      rect.x      = draw_area->x + (draw_area->width  - rect.width)  / 2;
      rect.y      = draw_area->y + (draw_area->height - rect.height) / 2;

      if (gdk_rectangle_intersect (&rect, (GdkRectangle *) expose_area,
                                   &render_rect))
        {
          guchar *buf;

          buf = (renderer->buffer +
                 (render_rect.y - rect.y) * renderer->rowstride +
                 (render_rect.x - rect.x) * renderer->bytes);

          gdk_draw_rgb_image_dithalign (window,
                                        widget->style->black_gc,
                                        render_rect.x,
                                        render_rect.y,
                                        render_rect.width,
                                        render_rect.height,
                                        GDK_RGB_DITHER_NORMAL,
                                        buf,
                                        renderer->rowstride,
                                        expose_area->x - draw_area->x,
                                        expose_area->y - draw_area->y);
        }
    }
}

static void
gimp_view_renderer_real_render (GimpViewRenderer *renderer,
                                GtkWidget        *widget)
{
  GdkPixbuf   *pixbuf;
  TempBuf     *temp_buf;
  const gchar *stock_id;

  pixbuf = gimp_viewable_get_pixbuf (renderer->viewable,
                                     renderer->context,
                                     renderer->width,
                                     renderer->height);
  if (pixbuf)
    {
      gimp_view_renderer_render_pixbuf (renderer, pixbuf);
      return;
    }

  temp_buf = gimp_viewable_get_preview (renderer->viewable,
                                        renderer->context,
                                        renderer->width,
                                        renderer->height);
  if (temp_buf)
    {
      gimp_view_renderer_default_render_buffer (renderer, widget, temp_buf);
      return;
    }

  stock_id = gimp_viewable_get_stock_id (renderer->viewable);
  gimp_view_renderer_default_render_stock (renderer, widget, stock_id);
}

static void
gimp_view_renderer_size_changed (GimpViewRenderer *renderer,
                                 GimpViewable     *viewable)
{
  if (renderer->size != -1)
    gimp_view_renderer_set_size (renderer, renderer->size,
                                 renderer->border_width);

  gimp_view_renderer_invalidate (renderer);
}


/*  protected functions  */

void
gimp_view_renderer_default_render_buffer (GimpViewRenderer *renderer,
                                          GtkWidget        *widget,
                                          TempBuf          *temp_buf)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (temp_buf != NULL);

  if (temp_buf->width < renderer->width)
    temp_buf->x = (renderer->width - temp_buf->width)  / 2;

  if (temp_buf->height < renderer->height)
    temp_buf->y = (renderer->height - temp_buf->height) / 2;

  gimp_view_renderer_render_buffer (renderer, temp_buf, -1,
                                    GIMP_VIEW_BG_CHECKS,
                                    GIMP_VIEW_BG_WHITE);
}

void
gimp_view_renderer_default_render_stock (GimpViewRenderer *renderer,
                                         GtkWidget        *widget,
                                         const gchar      *stock_id)
{
  GdkPixbuf   *pixbuf = NULL;
  GtkIconSize  icon_size;

  g_return_if_fail (GIMP_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (stock_id != NULL);

  if (renderer->pixbuf)
    {
      g_object_unref (renderer->pixbuf);
      renderer->pixbuf = NULL;
    }

  if (renderer->buffer)
    {
      g_free (renderer->buffer);
      renderer->buffer = NULL;
    }

  icon_size = gimp_get_icon_size (widget, stock_id, GTK_ICON_SIZE_INVALID,
                                  renderer->width, renderer->height);

  if (icon_size)
    pixbuf = gtk_widget_render_icon (widget, stock_id, icon_size, NULL);

  if (pixbuf)
    {
      gint  width  = gdk_pixbuf_get_width (pixbuf);
      gint  height = gdk_pixbuf_get_height (pixbuf);

      if (width > renderer->width || height > renderer->height)
        {
          GdkPixbuf *scaled_pixbuf;

          gimp_viewable_calc_preview_size (width, height,
                                           renderer->width, renderer->height,
                                           TRUE, 1.0, 1.0,
                                           &width, &height,
                                           NULL);

          scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                   width, height,
                                                   GDK_INTERP_BILINEAR);

          g_object_unref (pixbuf);
          pixbuf = scaled_pixbuf;
        }

      renderer->pixbuf = pixbuf;
    }

  renderer->needs_render = FALSE;
}

void
gimp_view_renderer_render_buffer (GimpViewRenderer *renderer,
                                  TempBuf          *temp_buf,
                                  gint              channel,
                                  GimpViewBG        inside_bg,
                                  GimpViewBG        outside_bg)
{
  if (renderer->pixbuf)
    {
      g_object_unref (renderer->pixbuf);
      renderer->pixbuf = NULL;
    }

  if (! renderer->buffer)
    renderer->buffer = g_new0 (guchar, renderer->height * renderer->rowstride);

  gimp_view_render_to_buffer (temp_buf,
                              channel,
                              inside_bg,
                              outside_bg,
                              renderer->buffer,
                              renderer->width,
                              renderer->height,
                              renderer->rowstride,
                              renderer->bytes);

  renderer->needs_render = FALSE;
}

void
gimp_view_render_to_buffer (TempBuf    *temp_buf,
                            gint        channel,
                            GimpViewBG  inside_bg,
                            GimpViewBG  outside_bg,
                            guchar     *dest_buffer,
                            gint        dest_width,
                            gint        dest_height,
                            gint        dest_rowstride,
                            gint        dest_bytes)
{
  const guchar *src, *s;
  guchar       *cb;
  guchar       *pad_buf;
  gint          a;
  gint          i, j, b;
  gint          x1, y1, x2, y2;
  gint          rowstride;
  gboolean      color;
  gboolean      has_alpha;
  gboolean      render_composite;
  gint          red_component;
  gint          green_component;
  gint          blue_component;
  gint          alpha_component;
  gint          offset;

  g_return_if_fail (temp_buf != NULL);
  g_return_if_fail (dest_buffer != NULL);

  /*  Here are the different cases this functions handles correctly:
   *  1)  Offset temp_buf which does not necessarily cover full image area
   *  2)  Color conversion of temp_buf if it is gray and image is color
   *  3)  Background check buffer for transparent temp_bufs
   *  4)  Using the optional "channel" argument, one channel can be extracted
   *      from a multi-channel temp_buf and composited as a grayscale
   *  Prereqs:
   *  1)  Grayscale temp_bufs have bytes == {1, 2}
   *  2)  Color temp_bufs have bytes == {3, 4}
   *  3)  If image is gray, then temp_buf should have bytes == {1, 2}
   */

  color            = (temp_buf->bytes == 3 || temp_buf->bytes == 4);
  has_alpha        = (temp_buf->bytes == 2 || temp_buf->bytes == 4);
  render_composite = (channel == -1);
  rowstride        = temp_buf->width * temp_buf->bytes;

  /*  render the checkerboard only if the temp_buf has alpha *and*
   *  we render a composite view
   */
  if (has_alpha && render_composite && outside_bg == GIMP_VIEW_BG_CHECKS)
    pad_buf = gimp_render_check_buf;
  else if (outside_bg == GIMP_VIEW_BG_WHITE)
    pad_buf = gimp_render_white_buf;
  else
    pad_buf = gimp_render_empty_buf;

  if (render_composite)
    {
      if (color)
        {
          red_component   = RED_PIX;
          green_component = GREEN_PIX;
          blue_component  = BLUE_PIX;
          alpha_component = ALPHA_PIX;
        }
      else
        {
          red_component   = GRAY_PIX;
          green_component = GRAY_PIX;
          blue_component  = GRAY_PIX;
          alpha_component = ALPHA_G_PIX;
        }
    }
  else
    {
      red_component   = channel;
      green_component = channel;
      blue_component  = channel;
      alpha_component = 0;
    }

  x1 = CLAMP (temp_buf->x, 0, dest_width);
  y1 = CLAMP (temp_buf->y, 0, dest_height);
  x2 = CLAMP (temp_buf->x + temp_buf->width,  0, dest_width);
  y2 = CLAMP (temp_buf->y + temp_buf->height, 0, dest_height);

  src = temp_buf_data (temp_buf) + ((y1 - temp_buf->y) * rowstride +
                                    (x1 - temp_buf->x) * temp_buf->bytes);

  for (i = 0; i < dest_height; i++)
    {
      if (i & 0x4)
        {
          offset = 4;
          cb = pad_buf + offset * 3;
        }
      else
        {
          offset = 0;
          cb = pad_buf;
        }

      /*  The interesting stuff between leading & trailing
       *  vertical transparency
       */
      if (i >= y1 && i < y2)
        {
          /*  Handle the leading transparency  */
          for (j = 0; j < x1; j++)
            for (b = 0; b < dest_bytes; b++)
              gimp_render_temp_buf[j * dest_bytes + b] = cb[j * 3 + b];

          /*  The stuff in the middle  */
          s = src;
          for (j = x1; j < x2; j++)
            {
              if (has_alpha && render_composite)
                {
                  a = s[alpha_component] << 8;

                  if (inside_bg == GIMP_VIEW_BG_CHECKS)
                    {
                      if ((j + offset) & 0x4)
                        {
                          gimp_render_temp_buf[j * 3 + 0] =
                            gimp_render_blend_dark_check [(a | s[red_component])];
                          gimp_render_temp_buf[j * 3 + 1] =
                            gimp_render_blend_dark_check [(a | s[green_component])];
                          gimp_render_temp_buf[j * 3 + 2] =
                            gimp_render_blend_dark_check [(a | s[blue_component])];
                        }
                      else
                        {
                          gimp_render_temp_buf[j * 3 + 0] =
                            gimp_render_blend_light_check [(a | s[red_component])];
                          gimp_render_temp_buf[j * 3 + 1] =
                            gimp_render_blend_light_check [(a | s[green_component])];
                          gimp_render_temp_buf[j * 3 + 2] =
                            gimp_render_blend_light_check [(a | s[blue_component])];
                        }
                    }
                  else /* GIMP_VIEW_BG_WHITE */
                    {
                      gimp_render_temp_buf[j * 3 + 0] =
                        gimp_render_blend_white [(a | s[red_component])];
                      gimp_render_temp_buf[j * 3 + 1] =
                        gimp_render_blend_white [(a | s[green_component])];
                      gimp_render_temp_buf[j * 3 + 2] =
                        gimp_render_blend_white [(a | s[blue_component])];
                    }
                }
              else
                {
                  gimp_render_temp_buf[j * 3 + 0] = s[red_component];
                  gimp_render_temp_buf[j * 3 + 1] = s[green_component];
                  gimp_render_temp_buf[j * 3 + 2] = s[blue_component];
                }

              s += temp_buf->bytes;
            }

          /*  Handle the trailing transparency  */
          for (j = x2; j < dest_width; j++)
            for (b = 0; b < dest_bytes; b++)
              gimp_render_temp_buf[j * dest_bytes + b] = cb[j * 3 + b];

          src += rowstride;
        }
      else
        {
          for (j = 0; j < dest_width; j++)
            for (b = 0; b < dest_bytes; b++)
              gimp_render_temp_buf[j * dest_bytes + b] = cb[j * 3 + b];
        }

      memcpy (dest_buffer + i * dest_rowstride,
              gimp_render_temp_buf,
              dest_width * dest_bytes);
    }
}

void
gimp_view_renderer_render_pixbuf (GimpViewRenderer *renderer,
                                  GdkPixbuf        *pixbuf)
{
  if (renderer->buffer)
    {
      g_free (renderer->buffer);
      renderer->buffer = NULL;
    }

  g_object_ref (pixbuf);

  if (renderer->pixbuf)
    g_object_unref (renderer->pixbuf);

  renderer->pixbuf = pixbuf;

  renderer->needs_render = FALSE;
}

static GdkGC *
gimp_view_renderer_create_gc (GimpViewRenderer *renderer,
                              GdkWindow        *window,
                              GtkWidget        *widget)
{
  GdkGC           *gc;
  GdkPixmap       *pixmap = NULL;
  GdkColormap     *colormap;
  GdkGCValues      values;
  GdkGCValuesMask  mask;

  gimp_rgb_get_gdk_color (&renderer->border_color, &values.foreground);

  colormap = gdk_drawable_get_colormap (window);
  gdk_rgb_find_color (colormap, &values.foreground);

  mask = GDK_GC_FOREGROUND;

  if (renderer->bg_stock_id)
    {
      GdkPixbuf *pixbuf;

      pixbuf = gtk_widget_render_icon (widget,
                                       renderer->bg_stock_id,
                                       GTK_ICON_SIZE_DIALOG, NULL);

      if (pixbuf)
        {
          gint         width;
          gint         height;

          width  = gdk_pixbuf_get_width (pixbuf);
          height = gdk_pixbuf_get_height (pixbuf);

          pixmap = gdk_pixmap_new (window, width, height,
                                   gdk_colormap_get_visual (colormap)->depth);
          gdk_drawable_set_colormap (pixmap, colormap);

          gdk_draw_rectangle (pixmap, widget->style->white_gc,
                              TRUE,
                              0, 0, width, height);

          gdk_draw_pixbuf (pixmap, widget->style->white_gc,
                           pixbuf, 0, 0,
                           0, 0, width, height,
                           GDK_RGB_DITHER_NORMAL, 0, 0);

          g_object_unref (pixbuf);
        }

      if (pixmap)
        {
          values.fill        = GDK_TILED;
          values.tile        = pixmap;
          values.ts_x_origin = 0;
          values.ts_y_origin = 0;

          mask |= (GDK_GC_FILL |
                   GDK_GC_TILE | GDK_GC_TS_X_ORIGIN | GDK_GC_TS_Y_ORIGIN);
        }
    }

  gc = gdk_gc_new_with_values (GDK_DRAWABLE (window), &values, mask);

  if (pixmap)
    g_object_unref (pixmap);

  return gc;
}
