/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpzoompreview.c
 * Copyright (C) 2005  David Odin <dindinx@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimpuitypes.h"

#include "gimp.h"

#include "gimpdrawablepreview.h"
#include "gimpzoompreview.h"


enum
{
  PROP_0,
  PROP_DRAWABLE
};

typedef struct GimpZoomPreviewPrivate
{
  GimpDrawable  *drawable;
  GimpZoomModel *model;
  GdkRectangle   extents;
} GimpZoomPreviewPrivate;


#define GIMP_ZOOM_PREVIEW_GET_PRIVATE(obj) \
  ((GimpZoomPreviewPrivate *) ((GimpZoomPreview *) (obj))->priv)

static GObject * gimp_zoom_preview_constructor (GType                  type,
                                                guint                  n_params,
                                                GObjectConstructParam *params);

static void     gimp_zoom_preview_get_property    (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);
static void     gimp_zoom_preview_set_property    (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
static void     gimp_zoom_preview_set_adjustments (GimpZoomPreview *preview,
                                                   gdouble          old_factor,
                                                   gdouble          new_factor);
static void     gimp_zoom_preview_size_allocate   (GtkWidget       *widget,
                                                   GtkAllocation   *allocation,
                                                   GimpZoomPreview *preview);
static void     gimp_zoom_preview_style_set       (GtkWidget       *widget,
                                                   GtkStyle        *prev_style);
static gboolean gimp_zoom_preview_scroll_event    (GtkWidget       *widget,
                                                   GdkEventScroll  *event,
                                                   GimpZoomPreview *preview);
static void     gimp_zoom_preview_draw            (GimpPreview     *preview);
static void     gimp_zoom_preview_draw_buffer     (GimpPreview     *preview,
                                                   const guchar    *buffer,
                                                   gint             rowstride);
static void     gimp_zoom_preview_draw_thumb      (GimpPreview     *preview,
                                                   GimpPreviewArea *area,
                                                   gint             width,
                                                   gint             height);
static void     gimp_zoom_preview_set_cursor      (GimpPreview     *preview);

static void     gimp_zoom_preview_set_drawable    (GimpZoomPreview *preview,
                                                   GimpDrawable    *drawable);


G_DEFINE_TYPE (GimpZoomPreview, gimp_zoom_preview, GIMP_TYPE_SCROLLED_PREVIEW)

#define parent_class gimp_zoom_preview_parent_class


static void
gimp_zoom_preview_class_init (GimpZoomPreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  GimpPreviewClass *preview_class = GIMP_PREVIEW_CLASS (klass);

  object_class->constructor  = gimp_zoom_preview_constructor;
  object_class->get_property = gimp_zoom_preview_get_property;
  object_class->set_property = gimp_zoom_preview_set_property;

  widget_class->style_set    = gimp_zoom_preview_style_set;

  preview_class->draw        = gimp_zoom_preview_draw;
  preview_class->draw_buffer = gimp_zoom_preview_draw_buffer;
  preview_class->draw_thumb  = gimp_zoom_preview_draw_thumb;
  preview_class->set_cursor  = gimp_zoom_preview_set_cursor;

  g_type_class_add_private (object_class, sizeof (GimpZoomPreviewPrivate));

  /**
   * GimpZoomPreview:drawable:
   *
   * The drawable the #GimpZoomPreview is currently attached to.
   *
   * Since: GIMP 2.4
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_pointer ("drawable", NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_zoom_preview_init (GimpZoomPreview *preview)
{
  GimpZoomPreviewPrivate *priv;
  GtkWidget              *button_bar;
  GtkWidget              *button;
  GtkWidget              *box;

  preview->priv = G_TYPE_INSTANCE_GET_PRIVATE (preview,
                                               GIMP_TYPE_ZOOM_PREVIEW,
                                               GimpZoomPreviewPrivate);

  priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);

  priv->model = gimp_zoom_model_new ();
  gimp_zoom_model_set_range (GIMP_ZOOM_MODEL (priv->model), 1.0, 256.0);
  g_signal_connect_swapped (priv->model, "zoomed",
                            G_CALLBACK (gimp_zoom_preview_set_adjustments),
                            preview);

  box = gimp_preview_get_controls (GIMP_PREVIEW (preview));
  g_return_if_fail (GTK_IS_BOX (box));

  button_bar = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_end (GTK_BOX (box), button_bar, FALSE, FALSE, 0);
  gtk_widget_show (button_bar);

  /* zoom out */
  button = gimp_zoom_button_new (priv->model,
                                 GIMP_ZOOM_OUT, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (button_bar), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* zoom in */
  button = gimp_zoom_button_new (priv->model,
                                 GIMP_ZOOM_IN, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (button_bar), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (GIMP_PREVIEW (preview)->area, "size-allocate",
                    G_CALLBACK (gimp_zoom_preview_size_allocate),
                    preview);
  g_signal_connect (GIMP_PREVIEW (preview)->area, "scroll-event",
                    G_CALLBACK (gimp_zoom_preview_scroll_event),
                    preview);

  g_object_set (GIMP_PREVIEW (preview)->area,
                "check-size", gimp_check_size (),
                "check-type", gimp_check_type (),
                NULL);

  gimp_scrolled_preview_set_policy (GIMP_SCROLLED_PREVIEW (preview),
                                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
}

static GObject *
gimp_zoom_preview_constructor (GType                  type,
                               guint                  n_params,
                               GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_zoom_preview_set_adjustments (GIMP_ZOOM_PREVIEW (object), 1.0, 1.0);

  return object;
}

static void
gimp_zoom_preview_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpZoomPreview *preview = GIMP_ZOOM_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_value_set_pointer (value, gimp_zoom_preview_get_drawable (preview));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_zoom_preview_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpZoomPreview *preview = GIMP_ZOOM_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      gimp_zoom_preview_set_drawable (preview,
                                      g_value_get_pointer (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_zoom_preview_set_adjustments (GimpZoomPreview *preview,
                                   gdouble          old_factor,
                                   gdouble          new_factor)
{
  GimpScrolledPreview *scrolled_preview = GIMP_SCROLLED_PREVIEW (preview);
  GtkAdjustment       *adj;
  gdouble              width;
  gdouble              height;
  gdouble              ratio;

  gimp_scrolled_preview_freeze (scrolled_preview);

  width  = GIMP_PREVIEW (preview)->width;
  height = GIMP_PREVIEW (preview)->height;

  ratio = new_factor / old_factor;

  adj = gtk_range_get_adjustment (GTK_RANGE (scrolled_preview->hscr));
  adj->lower          = 0;
  adj->page_size      = width;
  adj->upper          = width * new_factor;
  adj->step_increment = new_factor;
  adj->page_increment = MAX (width / 2.0, adj->step_increment);
  adj->value          = CLAMP ((adj->value + width / 2.0) * ratio
                               - width / 2.0,
                               adj->lower, adj->upper - width);
  gtk_adjustment_changed (adj);
  gtk_adjustment_value_changed (adj);

  adj = gtk_range_get_adjustment (GTK_RANGE (scrolled_preview->vscr));
  adj->lower          = 0;
  adj->page_size      = height;
  adj->upper          = height * new_factor;
  adj->step_increment = new_factor;
  adj->page_increment = MAX (height / 2.0, adj->step_increment);
  adj->value          = CLAMP ((adj->value + height / 2.0) * ratio
                               - height / 2.0,
                               adj->lower, adj->upper - height);
  gtk_adjustment_changed (adj);
  gtk_adjustment_value_changed (adj);

  gimp_scrolled_preview_thaw (scrolled_preview);
}

static void
gimp_zoom_preview_size_allocate (GtkWidget       *widget,
                                 GtkAllocation   *allocation,
                                 GimpZoomPreview *preview)
{
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);
  gdouble                 zoom_factor;

  gint width  = GIMP_PREVIEW (preview)->xmax - GIMP_PREVIEW (preview)->xmin;
  gint height = GIMP_PREVIEW (preview)->ymax - GIMP_PREVIEW (preview)->ymin;

  GIMP_PREVIEW (preview)->width  = MIN (width,  allocation->width);
  GIMP_PREVIEW (preview)->height = MIN (height, allocation->height);

  zoom_factor = gimp_zoom_model_get_factor (priv->model);
  gimp_zoom_preview_set_adjustments (preview, zoom_factor, zoom_factor);
}

static void
gimp_zoom_preview_style_set (GtkWidget *widget,
                             GtkStyle  *prev_style)
{
  GimpPreview            *preview  = GIMP_PREVIEW (widget);
  GimpZoomPreviewPrivate *priv     = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);
  GimpDrawable           *drawable = priv->drawable;
  gint                    size;
  gint                    width, height;
  gint                    x1, y1;
  gint                    x2, y2;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget, "size", &size, NULL);

  if (_gimp_drawable_preview_get_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      width  = x2 - x1;
      height = y2 - y1;
    }
  else
    {
      width  = gimp_drawable_width  (drawable->drawable_id);
      height = gimp_drawable_height (drawable->drawable_id);
    }

  if (width > height)
    {
      preview->width  = MIN (width, size);
      preview->height = (height * preview->width) / width;
    }
  else
    {
      preview->height = MIN (height, size);
      preview->width  = (width * preview->height) / height;
    }

  gtk_widget_set_size_request (preview->area,
                               preview->width, preview->height);
}

static gboolean
gimp_zoom_preview_scroll_event (GtkWidget       *widget,
                                GdkEventScroll  *event,
                                GimpZoomPreview *preview)
{
  if (event->state & GDK_CONTROL_MASK)
    {
      GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);

      gimp_scrolled_preview_freeze (GIMP_SCROLLED_PREVIEW (preview));

      switch (event->direction)
        {
        case GDK_SCROLL_UP:
          gimp_zoom_model_zoom (priv->model, GIMP_ZOOM_IN, 0.0);
          break;

        case GDK_SCROLL_DOWN:
          gimp_zoom_model_zoom (priv->model, GIMP_ZOOM_OUT, 0.0);
          break;

        default:
          break;
        }

      gimp_scrolled_preview_thaw (GIMP_SCROLLED_PREVIEW (preview));
    }

  return FALSE;
}

static void
gimp_zoom_preview_draw (GimpPreview *preview)
{
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);
  GimpDrawable           *drawable;
  guchar                 *data;
  gint                    width;
  gint                    height;
  gint                    bpp;
  gint                    src_x;
  gint                    src_y;
  gint                    src_width;
  gint                    src_height;
  gdouble                 zoom_factor;

  g_return_if_fail (GIMP_IS_ZOOM_PREVIEW (preview));

  drawable = priv->drawable;
  if (!drawable)
    return;

  zoom_factor = gimp_zoom_model_get_factor (priv->model);

  width  = preview->width;
  height = preview->height;

  src_x      = (priv->extents.x +
                preview->xoff * priv->extents.width / width / zoom_factor);
  src_y      = (priv->extents.y +
                preview->yoff * priv->extents.height / height / zoom_factor);
  src_width  = priv->extents.width / zoom_factor;
  src_height = priv->extents.height / zoom_factor;

  data = gimp_drawable_get_sub_thumbnail_data (drawable->drawable_id,
                                               src_x, src_y,
                                               src_width, src_height,
                                               &width, &height, &bpp);

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview->area),
                          0, 0, width, height,
                          gimp_drawable_type (drawable->drawable_id),
                          data, width * bpp);
  g_free (data);
}

static void
gimp_zoom_preview_draw_buffer (GimpPreview  *preview,
                               const guchar *buffer,
                               gint          rowstride)
{
  GimpZoomPreviewPrivate *priv     = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);
  GimpDrawable           *drawable = priv->drawable;
  gint32                  image_id;

  image_id = gimp_drawable_get_image (drawable->drawable_id);

  if (gimp_selection_is_empty (image_id))
    {
      gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview->area),
                              0, 0,
                              preview->width, preview->height,
                              gimp_drawable_type (drawable->drawable_id),
                              buffer,
                              rowstride);
    }
  else
    {
      guchar  *sel;
      guchar  *src;
      gint     selection_id;
      gint     width, height;
      gint     bpp;
      gint     src_x;
      gint     src_y;
      gint     src_width;
      gint     src_height;
      gdouble  zoom_factor;

      zoom_factor = gimp_zoom_model_get_factor (priv->model);
      selection_id = gimp_image_get_selection (image_id);

      width  = preview->width;
      height = preview->height;

      src_x = priv->extents.x +
           preview->xoff * priv->extents.width / width / zoom_factor;
      src_y = priv->extents.y +
           preview->yoff * priv->extents.height / height / zoom_factor;
      src_width  = priv->extents.width / zoom_factor;
      src_height = priv->extents.height / zoom_factor;

      src = gimp_drawable_get_sub_thumbnail_data (drawable->drawable_id,
                                                  src_x, src_y,
                                                  src_width, src_height,
                                                  &width, &height, &bpp);
      sel = gimp_drawable_get_sub_thumbnail_data (selection_id,
                                                  src_x, src_y,
                                                  src_width, src_height,
                                                  &width, &height, &bpp);

      gimp_preview_area_mask (GIMP_PREVIEW_AREA (preview->area),
                              0, 0, preview->width, preview->height,
                              gimp_drawable_type (drawable->drawable_id),
                              src, width * drawable->bpp,
                              buffer, rowstride,
                              sel, width);

      g_free (sel);
      g_free (src);
    }

}

static void
gimp_zoom_preview_draw_thumb (GimpPreview     *preview,
                              GimpPreviewArea *area,
                              gint             width,
                              gint             height)
{
  GimpZoomPreviewPrivate *priv     = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);
  GimpDrawable           *drawable = priv->drawable;

  if (drawable)
    _gimp_drawable_preview_area_draw_thumb (area, drawable, width, height);
}

static void
gimp_zoom_preview_set_cursor (GimpPreview *preview)
{
  if (! GTK_WIDGET_REALIZED (preview->area))
    return;

  if (gimp_zoom_preview_get_factor (GIMP_ZOOM_PREVIEW (preview)) > 1.0)
    {
      gdk_window_set_cursor (preview->area->window,
                             GIMP_SCROLLED_PREVIEW (preview)->cursor_move);
    }
  else
    {
      gdk_window_set_cursor (preview->area->window, preview->default_cursor);
    }
}

static void
gimp_zoom_preview_set_drawable (GimpZoomPreview *preview,
                                GimpDrawable    *drawable)
{
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);
  gint                    width, height;
  gint                    max_width, max_height;
  gint                    x1, y1;
  gint                    x2, y2;

  priv->drawable = drawable;

  if (_gimp_drawable_preview_get_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      width  = x2 - x1;
      height = y2 - y1;

      priv->extents.x = x1;
      priv->extents.y = y1;
    }
  else
    {
      width  = gimp_drawable_width  (drawable->drawable_id);
      height = gimp_drawable_height (drawable->drawable_id);

      priv->extents.x = 0;
      priv->extents.y = 0;
    }

  priv->extents.width  = width;
  priv->extents.height = height;

  if (width > height)
    {
      max_width  = MIN (width, 512);
      max_height = (height * max_width) / width;
    }
  else
    {
      max_height = MIN (height, 512);
      max_width  = (width * max_height) / height;
    }

  gimp_preview_set_bounds (GIMP_PREVIEW (preview),
                           0, 0, max_width, max_height);

  g_object_set (GIMP_PREVIEW (preview)->frame,
                "ratio", (gdouble) width / (gdouble) height,
                NULL);
}

/**
 * gimp_zoom_preview_new:
 * @drawable: a #GimpDrawable
 *
 * Creates a new #GimpZoomPreview widget for @drawable.
 *
 * Since: GIMP 2.4
 **/
GtkWidget *
gimp_zoom_preview_new (GimpDrawable *drawable)
{
  return g_object_new (GIMP_TYPE_ZOOM_PREVIEW,
                       "drawable", drawable,
                       NULL);
}

/**
 * gimp_zoom_get_drawable:
 * @preview: a #GimpZoomPreview widget
 *
 * Returns the #GimpDrawable the #GimpZoomPreview is attached to.
 * 
 * Return Value: the #GimpDrawable that was passed to gimp_zoom_preview_new().
 *
 * Since: GIMP 2.4
 **/
GimpDrawable *
gimp_zoom_preview_get_drawable (GimpZoomPreview *preview)
{
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);

  return priv->drawable;
}

/**
 * gimp_zoom_get_factor:
 * @preview: a #GimpZoomPreview widget
 *
 * Returns the zoom factor of the zoom model the preview is currently using.
 *
 * Return Value: the current zoom factor
 *
 * Since: GIMP 2.4
 **/
gdouble
gimp_zoom_preview_get_factor (GimpZoomPreview *preview)
{
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);

  return gimp_zoom_model_get_factor (priv->model);
}

/**
 * gimp_zoom_preview_get_source:
 * @preview: a #GimpZoomPreview widget
 * @width: a pointer to an int where the current width of the zoom widget
 *         will be put.
 * @height: a pointer to an int where the current width of the zoom widget
 *          will be put.
 * @bpp: a pointer to an int where the bpp of the current drawable the zoom
 *       widget is using will be put.
 *
 * Returns the scaled image data of the part of the drawable the
 * #GimpZoomPreview is currently showing, as a newly allocated array of guchar.
 * This function also allow to get the current width, height and bpp of the
 * #GimpZoomPreview.
 *
 * Return Value:
 *
 * Since: GIMP 2.4
 */
guchar *
gimp_zoom_preview_get_source (GimpZoomPreview *preview,
                              gint            *width,
                              gint            *height,
                              gint            *bpp)
{
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);
  GimpPreview            *gimp_preview = GIMP_PREVIEW (preview);
  GimpDrawable           *drawable     = priv->drawable;
  guchar                 *data;
  gint                    src_x;
  gint                    src_y;
  gint                    src_width;
  gint                    src_height;
  gdouble                 zoom_factor;

  zoom_factor = gimp_zoom_model_get_factor (priv->model);

  *width  = gimp_preview->width;
  *height = gimp_preview->height;

  src_x = priv->extents.x +
           gimp_preview->xoff * priv->extents.width / *width / zoom_factor;
  src_y = priv->extents.y +
           gimp_preview->yoff * priv->extents.height / *height / zoom_factor;

  src_width  = priv->extents.width / zoom_factor;
  src_height = priv->extents.height / zoom_factor;

  data = gimp_drawable_get_sub_thumbnail_data (drawable->drawable_id,
                                               src_x, src_y,
                                               src_width, src_height,
                                               width, height, bpp);
  return data;
}
