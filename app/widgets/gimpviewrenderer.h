/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrenderer.h
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

#ifndef __GIMP_VIEW_RENDERER_H__
#define __GIMP_VIEW_RENDERER_H__


#define GIMP_VIEW_MAX_BORDER_WIDTH 16


#define GIMP_TYPE_VIEW_RENDERER            (gimp_view_renderer_get_type ())
#define GIMP_VIEW_RENDERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEW_RENDERER, GimpViewRenderer))
#define GIMP_VIEW_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEW_RENDERER, GimpViewRendererClass))
#define GIMP_IS_VIEW_RENDERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_VIEW_RENDERER))
#define GIMP_IS_VIEW_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEW_RENDERER))
#define GIMP_VIEW_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEW_RENDERER, GimpViewRendererClass))


typedef struct _GimpViewRendererClass  GimpViewRendererClass;

struct _GimpViewRenderer
{
  GObject             parent_instance;

  GimpContext        *context;

  GType               viewable_type;
  GimpViewable       *viewable;

  gint                width;
  gint                height;
  gint                border_width;
  guint               dot_for_dot : 1;
  guint               is_popup    : 1;

  GimpViewBorderType  border_type;
  GimpRGB             border_color;
  GdkGC              *gc;

  /*< private >*/
  guchar             *buffer;
  gint                rowstride;
  gint                bytes;
  GdkPixbuf          *pixbuf;
  gchar              *bg_stock_id;

  gint                size;
  gboolean            needs_render;
  guint               idle_id;
};

struct _GimpViewRendererClass
{
  GObjectClass   parent_class;

  GdkPixbuf     *frame;
  gint           frame_left;
  gint           frame_right;
  gint           frame_bottom;
  gint           frame_top;

  /*  signals  */
  void (* update)      (GimpViewRenderer   *renderer);

  /*  virtual functions  */
  void (* set_context) (GimpViewRenderer   *renderer,
                        GimpContext        *context);
  void (* invalidate)  (GimpViewRenderer   *renderer);
  void (* draw)        (GimpViewRenderer   *renderer,
                        GdkWindow          *window,
                        GtkWidget          *widget,
                        const GdkRectangle *draw_area,
                        const GdkRectangle *expose_area);
  void (* render)      (GimpViewRenderer   *renderer,
                        GtkWidget          *widget);
};


GType              gimp_view_renderer_get_type (void) G_GNUC_CONST;

GimpViewRenderer * gimp_view_renderer_new      (GimpContext *context,
                                                GType        viewable_type,
                                                gint         size,
                                                gint         border_width,
                                                gboolean     is_popup);
GimpViewRenderer * gimp_view_renderer_new_full (GimpContext *context,
                                                GType        viewable_type,
                                                gint         width,
                                                gint         height,
                                                gint         border_width,
                                                gboolean     is_popup);

void   gimp_view_renderer_set_context      (GimpViewRenderer   *renderer,
                                            GimpContext        *context);
void   gimp_view_renderer_set_viewable     (GimpViewRenderer   *renderer,
                                            GimpViewable       *viewable);
void   gimp_view_renderer_set_size         (GimpViewRenderer   *renderer,
                                            gint                size,
                                            gint                border_width);
void   gimp_view_renderer_set_size_full    (GimpViewRenderer   *renderer,
                                            gint                width,
                                            gint                height,
                                            gint                border_width);
void   gimp_view_renderer_set_dot_for_dot  (GimpViewRenderer   *renderer,
                                            gboolean            dot_for_dot);
void   gimp_view_renderer_set_border_type  (GimpViewRenderer   *renderer,
                                            GimpViewBorderType  border_type);
void   gimp_view_renderer_set_border_color (GimpViewRenderer   *renderer,
                                            const GimpRGB      *border_color);
void   gimp_view_renderer_set_background   (GimpViewRenderer   *renderer,
                                            const gchar        *stock_id);

void   gimp_view_renderer_unrealize        (GimpViewRenderer   *renderer);

void   gimp_view_renderer_invalidate       (GimpViewRenderer   *renderer);
void   gimp_view_renderer_update           (GimpViewRenderer   *renderer);
void   gimp_view_renderer_update_idle      (GimpViewRenderer   *renderer);
void   gimp_view_renderer_remove_idle      (GimpViewRenderer   *renderer);

void   gimp_view_renderer_draw             (GimpViewRenderer   *renderer,
                                            GdkWindow          *window,
                                            GtkWidget          *widget,
                                            const GdkRectangle *draw_area,
                                            const GdkRectangle *expose_area);

/*  protected  */

void   gimp_view_renderer_default_render_buffer (GimpViewRenderer *renderer,
                                                 GtkWidget        *widget,
                                                 TempBuf          *temp_buf);
void   gimp_view_renderer_default_render_stock  (GimpViewRenderer *renderer,
                                                 GtkWidget        *widget,
                                                 const gchar      *stock_id);
void   gimp_view_renderer_render_buffer         (GimpViewRenderer *renderer,
                                                 TempBuf          *temp_buf,
                                                 gint              channel,
                                                 GimpViewBG        inside_bg,
                                                 GimpViewBG        outside_bg);
void    gimp_view_renderer_render_pixbuf        (GimpViewRenderer *renderer,
                                                 GdkPixbuf        *pixbuf);


/*  general purpose temp_buf to buffer projection function  */

void   gimp_view_render_to_buffer          (TempBuf    *temp_buf,
                                            gint        channel,
                                            GimpViewBG  inside_bg,
                                            GimpViewBG  outside_bg,
                                            guchar     *dest_buffer,
                                            gint        dest_width,
                                            gint        dest_height,
                                            gint        dest_rowstride,
                                            gint        dest_bytes);


#endif /* __GIMP_VIEW_RENDERER_H__ */
