/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppreviewrenderer.h
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

#ifndef __GIMP_PREVIEW_RENDERER_H__
#define __GIMP_PREVIEW_RENDERER_H__


#define GIMP_PREVIEW_MAX_BORDER_WIDTH 16


#define GIMP_TYPE_PREVIEW_RENDERER            (gimp_preview_renderer_get_type ())
#define GIMP_PREVIEW_RENDERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PREVIEW_RENDERER, GimpPreviewRenderer))
#define GIMP_PREVIEW_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PREVIEW_RENDERER, GimpPreviewRendererClass))
#define GIMP_IS_PREVIEW_RENDERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_PREVIEW_RENDERER))
#define GIMP_IS_PREVIEW_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PREVIEW_RENDERER))
#define GIMP_PREVIEW_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PREVIEW_RENDERER, GimpPreviewRendererClass))


typedef struct _GimpPreviewRendererClass  GimpPreviewRendererClass;

struct _GimpPreviewRenderer
{
  GObject       parent_instance;

  GType         viewable_type;
  GimpViewable *viewable;

  gint          width;
  gint          height;
  gint          border_width;
  gboolean      dot_for_dot;
  gboolean      is_popup;

  GimpRGB       border_color;
  GdkGC        *gc;

  /*< private >*/
  guchar       *buffer;
  gint          rowstride;
  gint          bytes;

  GdkPixbuf    *no_preview_pixbuf;
  gchar        *bg_stock_id;

  gint          size;
  gboolean      needs_render;
  guint         idle_id;
};

struct _GimpPreviewRendererClass
{
  GObjectClass  parent_class;

  /*  signals  */
  void (* update) (GimpPreviewRenderer *renderer);

  /*  virtual functions  */
  void (* draw)   (GimpPreviewRenderer *renderer,
                   GdkWindow           *window,
                   GtkWidget           *widget,
                   const GdkRectangle  *draw_area,
                   const GdkRectangle  *expose_area);
  void (* render) (GimpPreviewRenderer *renderer,
                   GtkWidget           *widget);
};


GType                 gimp_preview_renderer_get_type (void) G_GNUC_CONST;

GimpPreviewRenderer * gimp_preview_renderer_new      (GType         viewable_type,
                                                      gint          size,
                                                      gint          border_width,
                                                      gboolean      is_popup);
GimpPreviewRenderer * gimp_preview_renderer_new_full (GType         viewable_type,
                                                      gint          width,
                                                      gint          height,
                                                      gint          border_width,
                                                      gboolean      is_popup);

void   gimp_preview_renderer_set_viewable     (GimpPreviewRenderer *renderer,
                                               GimpViewable        *viewable);
void   gimp_preview_renderer_set_size         (GimpPreviewRenderer *renderer,
                                               gint                 size,
                                               gint                 border_width);
void   gimp_preview_renderer_set_size_full    (GimpPreviewRenderer *renderer,
                                               gint                 width,
                                               gint                 height,
                                               gint                 border_width);
void   gimp_preview_renderer_set_dot_for_dot  (GimpPreviewRenderer *renderer,
                                               gboolean             dot_for_dot);
void   gimp_preview_renderer_set_border_color (GimpPreviewRenderer *renderer,
                                               const GimpRGB       *border_color);
void   gimp_preview_renderer_set_background   (GimpPreviewRenderer *renderer,
                                               const gchar         *stock_id);

void   gimp_preview_renderer_unrealize        (GimpPreviewRenderer *renderer);

void   gimp_preview_renderer_invalidate       (GimpPreviewRenderer *renderer);
void   gimp_preview_renderer_update           (GimpPreviewRenderer *renderer);
void   gimp_preview_renderer_update_idle      (GimpPreviewRenderer *renderer);
void   gimp_preview_renderer_remove_idle      (GimpPreviewRenderer *renderer);

void   gimp_preview_renderer_draw             (GimpPreviewRenderer *renderer,
                                               GdkWindow           *window,
                                               GtkWidget           *widget,
                                               const GdkRectangle  *draw_area,
                                               const GdkRectangle  *expose_area);


/*  protected  */

void   gimp_preview_renderer_default_render_buffer (GimpPreviewRenderer *renderer,
                                                    GtkWidget           *widget,
                                                    TempBuf             *temp_buf);
void   gimp_preview_renderer_default_render_stock (GimpPreviewRenderer  *renderer,
                                                   GtkWidget            *widget,
                                                   const gchar          *stock_id);
void   gimp_preview_renderer_render_buffer        (GimpPreviewRenderer *renderer,
                                                   TempBuf             *temp_buf,
                                                   gint                 channel,
                                                   GimpPreviewBG        inside_bg,
                                                   GimpPreviewBG        outside_bg);


/*  general purpose temp_buf to buffer projection function  */

void   gimp_preview_render_to_buffer          (TempBuf             *temp_buf,
                                               gint                 channel,
                                               GimpPreviewBG        inside_bg,
                                               GimpPreviewBG        outside_bg,
                                               guchar              *dest_buffer,
                                               gint                 dest_width,
                                               gint                 dest_height,
                                               gint                 dest_rowstride,
                                               gint                 dest_bytes);


#endif /* __GIMP_PREVIEW_RENDERER_H__ */
