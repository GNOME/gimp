/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppreviewrenderer.c
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

#ifdef __GNUC__
#warning FIXME #include "display/display-types.h"
#endif
#include "display/display-types.h"

#include "base/temp-buf.h"

#include "core/gimpmarshal.h"
#include "core/gimpviewable.h"

#include "display/gimpdisplayshell-render.h"

#include "gimppreviewrenderer.h"
#include "gimppreviewrenderer-utils.h"


enum
{
  UPDATE,
  LAST_SIGNAL
};


static void     gimp_preview_renderer_class_init   (GimpPreviewRendererClass *klass);
static void     gimp_preview_renderer_init         (GimpPreviewRenderer      *renderer);

static void     gimp_preview_renderer_finalize     (GObject        *object);

static gboolean gimp_preview_renderer_idle_invalidate (GimpPreviewRenderer *renderer);
static void     gimp_preview_renderer_real_render  (GimpPreviewRenderer *renderer,
                                                    GtkWidget           *widget);

static void     gimp_preview_renderer_size_changed (GimpPreviewRenderer *renderer,
                                                    GimpViewable        *viewable);


static guint renderer_signals[LAST_SIGNAL] = { 0 };

static GObjectClass *parent_class = NULL;


GType
gimp_preview_renderer_get_type (void)
{
  static GType renderer_type = 0;

  if (! renderer_type)
    {
      static const GTypeInfo renderer_info =
      {
        sizeof (GimpPreviewRendererClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_preview_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpPreviewRenderer),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_preview_renderer_init,
      };

      renderer_type = g_type_register_static (G_TYPE_OBJECT,
                                              "GimpPreviewRenderer",
                                              &renderer_info, 0);
    }

  return renderer_type;
}

static void
gimp_preview_renderer_class_init (GimpPreviewRendererClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  renderer_signals[UPDATE] = 
    g_signal_new ("update",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpPreviewRendererClass, update),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize = gimp_preview_renderer_finalize;

  klass->render          = gimp_preview_renderer_real_render;
}

static void
gimp_preview_renderer_init (GimpPreviewRenderer *renderer)
{
  renderer->viewable          = NULL;

  renderer->width             = 8;
  renderer->height            = 8;
  renderer->border_width      = 0;
  renderer->dot_for_dot       = TRUE;
  renderer->is_popup          = FALSE;

  gimp_rgba_set (&renderer->border_color, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
  renderer->border_gc        = NULL;

  renderer->buffer            = NULL;
  renderer->rowstride         = 0;
  renderer->bytes             = 3;

  renderer->no_preview_pixbuf = NULL;

  renderer->size              = -1;
  renderer->needs_render      = TRUE;
  renderer->idle_id           = 0;
}

static void
gimp_preview_renderer_finalize (GObject *object)
{
  GimpPreviewRenderer *renderer;

  renderer = GIMP_PREVIEW_RENDERER (object);

  if (renderer->idle_id)
    {
      g_source_remove (renderer->idle_id);
      renderer->idle_id = 0;
    }

  if (renderer->viewable)
    gimp_preview_renderer_set_viewable (renderer, NULL);

  if (renderer->buffer)
    {
      g_free (renderer->buffer);
      renderer->buffer = NULL;
    }

  if (renderer->no_preview_pixbuf)
    {
      g_object_unref (renderer->no_preview_pixbuf);
      renderer->no_preview_pixbuf = NULL;
    }

  if (renderer->border_gc)
    {
      g_object_unref (renderer->border_gc);
      renderer->border_gc = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

GimpPreviewRenderer *
gimp_preview_renderer_new (GType    viewable_type,
                           gint     size,
                           gint     border_width,
                           gboolean is_popup)
{
  GimpPreviewRenderer *renderer;

  g_return_val_if_fail (g_type_is_a (viewable_type, GIMP_TYPE_VIEWABLE), NULL);
  g_return_val_if_fail (size > 0 && size <= GIMP_PREVIEW_MAX_SIZE, NULL);
  g_return_val_if_fail (border_width >= 0 &&
                        border_width <= GIMP_PREVIEW_MAX_BORDER_WIDTH, NULL);

  renderer = g_object_new (gimp_preview_renderer_type_from_viewable_type (viewable_type),
                           NULL);

  renderer->is_popup = is_popup ? TRUE : FALSE;

  gimp_preview_renderer_set_size (renderer, size, border_width);
  gimp_preview_renderer_remove_idle (renderer);

  return renderer;
}

GimpPreviewRenderer *
gimp_preview_renderer_new_full (GType    viewable_type,
                                gint     width,
                                gint     height,
                                gint     border_width,
                                gboolean is_popup)
{
  GimpPreviewRenderer *renderer;

  g_return_val_if_fail (g_type_is_a (viewable_type, GIMP_TYPE_VIEWABLE), NULL);
  g_return_val_if_fail (width  > 0 && width  <= GIMP_PREVIEW_MAX_SIZE, NULL);
  g_return_val_if_fail (height > 0 && height <= GIMP_PREVIEW_MAX_SIZE, NULL);
  g_return_val_if_fail (border_width >= 0 &&
                        border_width <= GIMP_PREVIEW_MAX_BORDER_WIDTH, NULL);

  renderer = g_object_new (gimp_preview_renderer_type_from_viewable_type (viewable_type),
                           NULL);

  renderer->is_popup = is_popup ? TRUE : FALSE;

  gimp_preview_renderer_set_size_full (renderer, width, height, border_width);
  gimp_preview_renderer_remove_idle (renderer);

  return renderer;
}

void
gimp_preview_renderer_set_viewable (GimpPreviewRenderer *renderer,
                                    GimpViewable        *viewable)
{
  GType viewable_type = G_TYPE_NONE;

  g_return_if_fail (GIMP_IS_PREVIEW_RENDERER (renderer));
  g_return_if_fail (! viewable || GIMP_IS_VIEWABLE (viewable));

  if (viewable)
    {
      viewable_type = G_TYPE_FROM_INSTANCE (viewable);

      g_return_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (renderer),
                                     gimp_preview_renderer_type_from_viewable_type (viewable_type)));
    }

  if (viewable == renderer->viewable)
    return;

  if (renderer->buffer)
    {
      g_free (renderer->buffer);
      renderer->buffer = NULL;
    }

  if (renderer->no_preview_pixbuf)
    {
      g_object_unref (renderer->no_preview_pixbuf);
      renderer->no_preview_pixbuf = NULL;
    }

  if (renderer->viewable)
    {
      g_object_remove_weak_pointer (G_OBJECT (renderer->viewable),
				    (gpointer *) &renderer->viewable);

      g_signal_handlers_disconnect_by_func (renderer->viewable,
                                            G_CALLBACK (gimp_preview_renderer_invalidate),
                                            renderer);

      g_signal_handlers_disconnect_by_func (renderer->viewable,
                                            G_CALLBACK (gimp_preview_renderer_size_changed),
                                            renderer);
    }

  renderer->viewable = viewable;

  if (renderer->viewable)
    {
      g_object_add_weak_pointer (G_OBJECT (renderer->viewable),
				 (gpointer *) &renderer->viewable);

      g_signal_connect_swapped (renderer->viewable,
                                "invalidate_preview",
                                G_CALLBACK (gimp_preview_renderer_invalidate),
                                renderer);

      g_signal_connect_swapped (renderer->viewable,
                                "size_changed",
                                G_CALLBACK (gimp_preview_renderer_size_changed),
                                renderer);

      if (renderer->size != -1)
        gimp_preview_renderer_set_size (renderer, renderer->size,
                                        renderer->border_width);
      else
        gimp_preview_renderer_invalidate (renderer);
    }
}

void
gimp_preview_renderer_set_size (GimpPreviewRenderer *renderer,
                                gint                 preview_size,
                                gint                 border_width)
{
  gint width, height;

  g_return_if_fail (GIMP_IS_PREVIEW_RENDERER (renderer));
  g_return_if_fail (preview_size > 0 && preview_size <= GIMP_PREVIEW_MAX_SIZE);
  g_return_if_fail (border_width >= 0 &&
                    border_width <= GIMP_PREVIEW_MAX_BORDER_WIDTH);

  renderer->size = preview_size;

  if (renderer->viewable)
    {
      gimp_viewable_get_preview_size (renderer->viewable,
                                      preview_size,
                                      renderer->is_popup,
                                      renderer->dot_for_dot,
                                      &width, &height);
    }
  else
    {
      width  = preview_size;
      height = preview_size;
    }

  gimp_preview_renderer_set_size_full (renderer, width, height, border_width);
}

void
gimp_preview_renderer_set_size_full (GimpPreviewRenderer *renderer,
                                     gint                 width,
                                     gint                 height,
                                     gint                 border_width)
{
  g_return_if_fail (GIMP_IS_PREVIEW_RENDERER (renderer));
  g_return_if_fail (width  > 0 && width  <= GIMP_PREVIEW_MAX_SIZE);
  g_return_if_fail (height > 0 && height <= GIMP_PREVIEW_MAX_SIZE);
  g_return_if_fail (border_width >= 0 &&
                    border_width <= GIMP_PREVIEW_MAX_BORDER_WIDTH);

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
        gimp_preview_renderer_invalidate (renderer);
    }
}

void
gimp_preview_renderer_set_dot_for_dot (GimpPreviewRenderer *renderer,
                                       gboolean             dot_for_dot)
{
  g_return_if_fail (GIMP_IS_PREVIEW_RENDERER (renderer));

  if (dot_for_dot != renderer->dot_for_dot)
    {
      renderer->dot_for_dot = dot_for_dot ? TRUE: FALSE;

      if (renderer->size != -1)
        gimp_preview_renderer_set_size (renderer, renderer->size,
                                        renderer->border_width);

      gimp_preview_renderer_invalidate (renderer);
    }
}

void
gimp_preview_renderer_set_border_color (GimpPreviewRenderer *renderer,
                                        const GimpRGB       *color)
{
  g_return_if_fail (GIMP_IS_PREVIEW_RENDERER (renderer));
  g_return_if_fail (color != NULL);

  if (gimp_rgb_distance (&renderer->border_color, color))
    {
      renderer->border_color = *color;

      if (renderer->border_gc)
        {
          GdkColor gdk_color;
          guchar   r, g, b;

          gimp_rgb_get_uchar (&renderer->border_color, &r, &g, &b);

          gdk_color.red   = r | r << 8;
          gdk_color.green = g | g << 8;
          gdk_color.blue  = b | b << 8;

          gdk_gc_set_rgb_fg_color (renderer->border_gc, &gdk_color);
        }

      gimp_preview_renderer_update (renderer);
    }
}

void
gimp_preview_renderer_invalidate (GimpPreviewRenderer *renderer)
{
  g_return_if_fail (GIMP_IS_PREVIEW_RENDERER (renderer));

  if (renderer->idle_id)
    g_source_remove (renderer->idle_id);

  renderer->needs_render = TRUE;

  renderer->idle_id =
    g_idle_add_full (G_PRIORITY_LOW,
                     (GSourceFunc) gimp_preview_renderer_idle_invalidate,
                     renderer, NULL);
}

void
gimp_preview_renderer_update (GimpPreviewRenderer *renderer)
{
  g_return_if_fail (GIMP_IS_PREVIEW_RENDERER (renderer));

  if (renderer->idle_id)
    {
      g_source_remove (renderer->idle_id);
      renderer->idle_id = 0;
    }

  g_signal_emit (renderer, renderer_signals[UPDATE], 0);
}

void
gimp_preview_renderer_remove_idle (GimpPreviewRenderer *renderer)
{
  g_return_if_fail (GIMP_IS_PREVIEW_RENDERER (renderer));

  if (renderer->idle_id)
    {
      g_source_remove (renderer->idle_id);
      renderer->idle_id = 0;
    }
}

void
gimp_preview_renderer_draw (GimpPreviewRenderer *renderer,
                            GdkWindow           *window,
                            GtkWidget           *widget,
                            GdkRectangle        *draw_area,
                            GdkRectangle        *expose_area)
{
  GdkRectangle border_rect;
  GdkRectangle buf_rect;
  GdkRectangle render_rect;

  g_return_if_fail (GIMP_IS_PREVIEW_RENDERER (renderer));
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (draw_area != NULL);
  g_return_if_fail (expose_area != NULL);

  if (! GTK_WIDGET_DRAWABLE (widget) || ! renderer->viewable)
    return;

  if (renderer->needs_render)
    GIMP_PREVIEW_RENDERER_GET_CLASS (renderer)->render (renderer, widget);

  border_rect.x      = draw_area->x;
  border_rect.y      = draw_area->y;
  border_rect.width  = renderer->width  + 2 * renderer->border_width;
  border_rect.height = renderer->height + 2 * renderer->border_width;

  if (draw_area->width > border_rect.width)
    border_rect.x += (draw_area->width - border_rect.width) / 2;

  if (draw_area->height > border_rect.height)
    border_rect.y += (draw_area->height - border_rect.height) / 2;

  if (renderer->no_preview_pixbuf)
    {
      buf_rect.width  = gdk_pixbuf_get_width  (renderer->no_preview_pixbuf);
      buf_rect.height = gdk_pixbuf_get_height (renderer->no_preview_pixbuf);
      buf_rect.x      = (draw_area->width  - buf_rect.width)  / 2;
      buf_rect.y      = (draw_area->height - buf_rect.height) / 2;

      buf_rect.x += draw_area->x;
      buf_rect.y += draw_area->y;

      if (gdk_rectangle_intersect (&buf_rect, expose_area, &render_rect))
        {
          /* FIXME: remove when we no longer support GTK 2.0.x */
#if GTK_CHECK_VERSION(2,2,0)
          gdk_draw_pixbuf (GDK_DRAWABLE (window),
                           widget->style->bg_gc[widget->state],
                           renderer->no_preview_pixbuf,
                           render_rect.x - buf_rect.x,
                           render_rect.y - buf_rect.y,
                           render_rect.x,
                           render_rect.y,
                           render_rect.width,
                           render_rect.height,
                           GDK_RGB_DITHER_NORMAL,
                           expose_area->x - draw_area->x,
                           expose_area->y - draw_area->y);
#else
          gdk_pixbuf_render_to_drawable (renderer->no_preview_pixbuf,
                                         window,
                                         widget->style->bg_gc[widget->state],
                                         render_rect.x - buf_rect.x,
                                         render_rect.y - buf_rect.y,
                                         render_rect.x,
                                         render_rect.y, 
                                         render_rect.width,
                                         render_rect.height,
                                         GDK_RGB_DITHER_NORMAL,
                                         expose_area->x - draw_area->x,
                                         expose_area->y - draw_area->y);
#endif
        }
    }
  else if (renderer->buffer)
    {
      buf_rect.x      = border_rect.x + renderer->border_width;
      buf_rect.y      = border_rect.y + renderer->border_width;
      buf_rect.width  = renderer->width;
      buf_rect.height = renderer->height;

      if (gdk_rectangle_intersect (&buf_rect, expose_area, &render_rect))
        {
          guchar *buf;

          buf = (renderer->buffer +
                 (render_rect.y - buf_rect.y) * renderer->rowstride +
                 (render_rect.x - buf_rect.x) * renderer->bytes);

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

  if (renderer->border_width > 0)
    {
      gint i;

      if (! renderer->border_gc)
        {
          GdkColor color;
          guchar   r, g, b;

          renderer->border_gc = gdk_gc_new (window);

          gimp_rgb_get_uchar (&renderer->border_color, &r, &g, &b);

          color.red   = r | r << 8;
          color.green = g | g << 8;
          color.blue  = b | b << 8;

          gdk_gc_set_rgb_fg_color (renderer->border_gc, &color);
        }

      for (i = 0; i < renderer->border_width; i++)
        gdk_draw_rectangle (window,
                            renderer->border_gc,
                            FALSE,
                            border_rect.x + i,
                            border_rect.y + i,
                            border_rect.width  - 2 * i - 1,
                            border_rect.height - 2 * i - 1);
    }
}

/*  private functions  */

static gboolean
gimp_preview_renderer_idle_invalidate (GimpPreviewRenderer *renderer)
{
  renderer->idle_id = 0;

  if (renderer->viewable)
    gimp_preview_renderer_update (renderer);

  return FALSE;
}

static void
gimp_preview_renderer_real_render (GimpPreviewRenderer *renderer,
                                   GtkWidget           *widget)
{
  TempBuf *temp_buf;

  temp_buf = gimp_viewable_get_preview (renderer->viewable,
					renderer->width,
					renderer->height);

  if (temp_buf)
    {
      gimp_preview_renderer_default_render_buffer (renderer, widget, temp_buf);
    }
  else /* no preview available */
    {
      const gchar  *stock_id;

      stock_id = gimp_viewable_get_stock_id (renderer->viewable);

      gimp_preview_renderer_default_render_stock (renderer, widget, stock_id);
    }
}

static void
gimp_preview_renderer_size_changed (GimpPreviewRenderer *renderer,
                                    GimpViewable        *viewable)
{
  if (renderer->size != -1)
    gimp_preview_renderer_set_size (renderer, renderer->size,
                                    renderer->border_width);
  else
    gimp_preview_renderer_invalidate (renderer);
}


/*  protected functions  */

void
gimp_preview_renderer_default_render_buffer (GimpPreviewRenderer *renderer,
                                             GtkWidget           *widget,
                                             TempBuf             *temp_buf)
{
  g_return_if_fail (GIMP_IS_PREVIEW_RENDERER (renderer));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (temp_buf != NULL);

  if (renderer->no_preview_pixbuf)
    {
      g_object_unref (renderer->no_preview_pixbuf);
      renderer->no_preview_pixbuf = NULL;
    }

  if (temp_buf->width < renderer->width)
    temp_buf->x = (renderer->width - temp_buf->width)  / 2;

  if (temp_buf->height < renderer->height)
    temp_buf->y = (renderer->height - temp_buf->height) / 2;

  gimp_preview_renderer_render_buffer (renderer, temp_buf, -1,
                                       GIMP_PREVIEW_BG_CHECKS,
                                       GIMP_PREVIEW_BG_WHITE);
}

void
gimp_preview_renderer_default_render_stock (GimpPreviewRenderer *renderer,
                                            GtkWidget           *widget,
                                            const gchar         *stock_id)
{
  GdkPixbuf    *pixbuf;
  GtkIconSet   *icon_set;
  GtkIconSize  *sizes;
  gint          n_sizes;
  gint          i;
  gint          width_diff  = 1024;
  gint          height_diff = 1024;
  GtkIconSize   icon_size = GTK_ICON_SIZE_MENU;

  g_return_if_fail (GIMP_IS_PREVIEW_RENDERER (renderer));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (stock_id != NULL);

  if (renderer->no_preview_pixbuf)
    {
      g_object_unref (renderer->no_preview_pixbuf);
      renderer->no_preview_pixbuf = NULL;
    }

  if (renderer->buffer)
    {
      g_free (renderer->buffer);
      renderer->buffer = NULL;
    }

  icon_set = gtk_style_lookup_icon_set (widget->style, stock_id);

  gtk_icon_set_get_sizes (icon_set, &sizes, &n_sizes);

  for (i = 0; i < n_sizes; i++)
    {
      gint icon_width;
      gint icon_height;

      if (gtk_icon_size_lookup (sizes[i], &icon_width, &icon_height))
        {
          if (icon_width  <= renderer->width  &&
              icon_height <= renderer->height &&
              (ABS (icon_width  - renderer->width)  < width_diff ||
               ABS (icon_height - renderer->height) < height_diff))
            {
              width_diff  = ABS (icon_width  - renderer->width);
              height_diff = ABS (icon_height - renderer->height);

              icon_size = sizes[i];
            }
        }
    }

  g_free (sizes);

  pixbuf = gtk_icon_set_render_icon (icon_set,
                                     widget->style,
                                     gtk_widget_get_direction (widget),
                                     widget->state,
                                     icon_size,
                                     widget, NULL);

  if (pixbuf)
    {
      if (gdk_pixbuf_get_width (pixbuf)  > renderer->width ||
          gdk_pixbuf_get_height (pixbuf) > renderer->height)
        {
          GdkPixbuf *scaled_pixbuf;
          gint       pixbuf_width;
          gint       pixbuf_height;

          gimp_viewable_calc_preview_size (renderer->viewable,
                                           gdk_pixbuf_get_width (pixbuf),
                                           gdk_pixbuf_get_height (pixbuf),
                                           renderer->width,
                                           renderer->height,
                                           TRUE, 1.0, 1.0,
                                           &pixbuf_width,
                                           &pixbuf_height,
                                           NULL);

          scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                   pixbuf_width,
                                                   pixbuf_height,
                                                   GDK_INTERP_BILINEAR);

          g_object_unref (pixbuf);
          pixbuf = scaled_pixbuf;
        }

      renderer->no_preview_pixbuf = pixbuf;
    }

  renderer->needs_render = FALSE;
}

void
gimp_preview_renderer_render_buffer (GimpPreviewRenderer *renderer,
                                     TempBuf             *temp_buf,
                                     gint                 channel,
                                     GimpPreviewBG        inside_bg,
                                     GimpPreviewBG        outside_bg)
{
  if (! renderer->buffer)
    renderer->buffer = g_new0 (guchar, renderer->height * renderer->rowstride);

  gimp_preview_render_to_buffer (temp_buf,
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
gimp_preview_render_to_buffer (TempBuf       *temp_buf,
                               gint           channel,
                               GimpPreviewBG  inside_bg,
                               GimpPreviewBG  outside_bg,
                               guchar        *dest_buffer,
                               gint           dest_width,
                               gint           dest_height,
                               gint           dest_rowstride,
                               gint           dest_bytes)
{
  guchar   *src, *s;
  guchar   *cb;
  guchar   *pad_buf;
  gint      a;
  gint      i, j, b;
  gint      x1, y1, x2, y2;
  gint      rowstride;
  gboolean  color;
  gboolean  has_alpha;
  gboolean  render_composite;
  gint      red_component;
  gint      green_component;
  gint      blue_component;
  gint      alpha_component;
  gint      offset;

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
   *  we render a composite preview
   */
  if (has_alpha && render_composite && outside_bg == GIMP_PREVIEW_BG_CHECKS)
    pad_buf = render_check_buf;
  else if (outside_bg == GIMP_PREVIEW_BG_WHITE)
    pad_buf = render_white_buf;
  else
    pad_buf = render_empty_buf;

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
	      render_temp_buf[j * dest_bytes + b] = cb[j * 3 + b];

	  /*  The stuff in the middle  */
	  s = src;
	  for (j = x1; j < x2; j++)
	    {
              if (has_alpha && render_composite)
                {
                  a = s[alpha_component] << 8;

                  if (inside_bg == GIMP_PREVIEW_BG_CHECKS)
                    {
                      if ((j + offset) & 0x4)
                        {
                          render_temp_buf[j * 3 + 0] = 
                            render_blend_dark_check [(a | s[red_component])];
                          render_temp_buf[j * 3 + 1] = 
                            render_blend_dark_check [(a | s[green_component])];
                          render_temp_buf[j * 3 + 2] = 
                            render_blend_dark_check [(a | s[blue_component])];
                        }
                      else
                        {
                          render_temp_buf[j * 3 + 0] = 
                            render_blend_light_check [(a | s[red_component])];
                          render_temp_buf[j * 3 + 1] = 
                            render_blend_light_check [(a | s[green_component])];
                          render_temp_buf[j * 3 + 2] = 
                            render_blend_light_check [(a | s[blue_component])];
                        }
                    }
                  else /* GIMP_PREVIEW_BG_WHITE */
                    {
                      render_temp_buf[j * 3 + 0] = 
                        render_blend_white [(a | s[red_component])];
                      render_temp_buf[j * 3 + 1] = 
                        render_blend_white [(a | s[green_component])];
                      render_temp_buf[j * 3 + 2] = 
                        render_blend_white [(a | s[blue_component])];
                    }
                }
              else
                {
                  render_temp_buf[j * 3 + 0] = s[red_component];
                  render_temp_buf[j * 3 + 1] = s[green_component];
                  render_temp_buf[j * 3 + 2] = s[blue_component];
                }

	      s += temp_buf->bytes;
	    }

	  /*  Handle the trailing transparency  */
	  for (j = x2; j < dest_width; j++)
	    for (b = 0; b < dest_bytes; b++)
	      render_temp_buf[j * dest_bytes + b] = cb[j * 3 + b];

	  src += rowstride;
	}
      else
	{
	  for (j = 0; j < dest_width; j++)
	    for (b = 0; b < dest_bytes; b++)
	      render_temp_buf[j * dest_bytes + b] = cb[j * 3 + b];
	}

      memcpy (dest_buffer + i * dest_rowstride,
              render_temp_buf,
              dest_width * dest_bytes);
    }
}
