/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrenderer.h
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

#ifndef __LIGMA_VIEW_RENDERER_H__
#define __LIGMA_VIEW_RENDERER_H__


#define LIGMA_VIEW_MAX_BORDER_WIDTH 16


#define LIGMA_TYPE_VIEW_RENDERER            (ligma_view_renderer_get_type ())
#define LIGMA_VIEW_RENDERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VIEW_RENDERER, LigmaViewRenderer))
#define LIGMA_VIEW_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VIEW_RENDERER, LigmaViewRendererClass))
#define LIGMA_IS_VIEW_RENDERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_VIEW_RENDERER))
#define LIGMA_IS_VIEW_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VIEW_RENDERER))
#define LIGMA_VIEW_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VIEW_RENDERER, LigmaViewRendererClass))


typedef struct _LigmaViewRendererPrivate LigmaViewRendererPrivate;
typedef struct _LigmaViewRendererClass   LigmaViewRendererClass;

struct _LigmaViewRenderer
{
  GObject             parent_instance;

  LigmaContext        *context;

  GType               viewable_type;
  LigmaViewable       *viewable;

  gint                width;
  gint                height;
  gint                border_width;
  guint               dot_for_dot : 1;
  guint               is_popup    : 1;

  LigmaViewBorderType  border_type;
  LigmaRGB             border_color;

  /*< protected >*/
  cairo_surface_t    *surface;

  gint                size;

  /*< private >*/
  LigmaViewRendererPrivate *priv;
};

struct _LigmaViewRendererClass
{
  GObjectClass   parent_class;

  GdkPixbuf     *frame;
  gint           frame_left;
  gint           frame_right;
  gint           frame_bottom;
  gint           frame_top;

  /*  signals  */
  void (* update)      (LigmaViewRenderer *renderer);

  /*  virtual functions  */
  void (* set_context) (LigmaViewRenderer *renderer,
                        LigmaContext      *context);
  void (* invalidate)  (LigmaViewRenderer *renderer);
  void (* draw)        (LigmaViewRenderer *renderer,
                        GtkWidget        *widget,
                        cairo_t          *cr,
                        gint              available_width,
                        gint              available_height);
  void (* render)      (LigmaViewRenderer *renderer,
                        GtkWidget        *widget);
};


GType              ligma_view_renderer_get_type (void) G_GNUC_CONST;

LigmaViewRenderer * ligma_view_renderer_new      (LigmaContext *context,
                                                GType        viewable_type,
                                                gint         size,
                                                gint         border_width,
                                                gboolean     is_popup);
LigmaViewRenderer * ligma_view_renderer_new_full (LigmaContext *context,
                                                GType        viewable_type,
                                                gint         width,
                                                gint         height,
                                                gint         border_width,
                                                gboolean     is_popup);

void   ligma_view_renderer_set_context      (LigmaViewRenderer   *renderer,
                                            LigmaContext        *context);
void   ligma_view_renderer_set_viewable     (LigmaViewRenderer   *renderer,
                                            LigmaViewable       *viewable);
void   ligma_view_renderer_set_size         (LigmaViewRenderer   *renderer,
                                            gint                size,
                                            gint                border_width);
void   ligma_view_renderer_set_size_full    (LigmaViewRenderer   *renderer,
                                            gint                width,
                                            gint                height,
                                            gint                border_width);
void   ligma_view_renderer_set_dot_for_dot  (LigmaViewRenderer   *renderer,
                                            gboolean            dot_for_dot);
void   ligma_view_renderer_set_border_type  (LigmaViewRenderer   *renderer,
                                            LigmaViewBorderType  border_type);
void   ligma_view_renderer_set_border_color (LigmaViewRenderer   *renderer,
                                            const LigmaRGB      *border_color);
void   ligma_view_renderer_set_background   (LigmaViewRenderer   *renderer,
                                            const gchar        *icon_name);
void   ligma_view_renderer_set_color_config (LigmaViewRenderer   *renderer,
                                            LigmaColorConfig    *color_config);

void   ligma_view_renderer_invalidate       (LigmaViewRenderer   *renderer);
void   ligma_view_renderer_update           (LigmaViewRenderer   *renderer);
void   ligma_view_renderer_update_idle      (LigmaViewRenderer   *renderer);
void   ligma_view_renderer_remove_idle      (LigmaViewRenderer   *renderer);

void   ligma_view_renderer_draw             (LigmaViewRenderer   *renderer,
                                            GtkWidget          *widget,
                                            cairo_t            *cr,
                                            gint                available_width,
                                            gint                available_height);

/*  protected  */

void   ligma_view_renderer_render_temp_buf_simple (LigmaViewRenderer *renderer,
                                                  GtkWidget        *widget,
                                                  LigmaTempBuf      *temp_buf);
void   ligma_view_renderer_render_temp_buf        (LigmaViewRenderer *renderer,
                                                  GtkWidget        *widget,
                                                  LigmaTempBuf      *temp_buf,
                                                  gint              temp_buf_x,
                                                  gint              temp_buf_y,
                                                  gint              channel,
                                                  LigmaViewBG        inside_bg,
                                                  LigmaViewBG        outside_bg);
void   ligma_view_renderer_render_pixbuf          (LigmaViewRenderer *renderer,
                                                  GtkWidget        *widget,
                                                  GdkPixbuf        *pixbuf);
void   ligma_view_renderer_render_icon            (LigmaViewRenderer *renderer,
                                                  GtkWidget        *widget,
                                                  const gchar      *icon_name);
LigmaColorTransform *
       ligma_view_renderer_get_color_transform    (LigmaViewRenderer *renderer,
                                                  GtkWidget        *widget,
                                                  const Babl       *src_format,
                                                  const Babl       *dest_format);
void   ligma_view_renderer_free_color_transform   (LigmaViewRenderer *renderer);


#endif /* __LIGMA_VIEW_RENDERER_H__ */
