/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpzoompreview.c
 * Copyright (C) 2005  David Odin <dindinx@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimpuitypes.h"

#include "gimp.h"

#include "gimpdrawablepreview.h"
#include "gimpzoompreview.h"


/**
 * SECTION: gimpzoompreview
 * @title: GimpZoomPreview
 * @short_description: A drawable preview with zooming capabilities.
 *
 * A drawable preview with zooming capabilities.
 **/


enum
{
  PROP_0,
  PROP_DRAWABLE_ID,
  PROP_MODEL
};

typedef struct
{
  gboolean  update;
} PreviewSettings;


struct _GimpZoomPreviewPrivate
{
  gint32         drawable_ID;
  GimpZoomModel *model;
  GdkRectangle   extents;
};

#define GET_PRIVATE(obj) (((GimpZoomPreview *) (obj))->priv)


static void     gimp_zoom_preview_constructed     (GObject         *object);
static void     gimp_zoom_preview_finalize        (GObject         *object);
static void     gimp_zoom_preview_dispose         (GObject         *object);
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
static void     gimp_zoom_preview_style_updated   (GtkWidget       *widget);
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
static void     gimp_zoom_preview_transform       (GimpPreview     *preview,
                                                   gint             src_x,
                                                   gint             src_y,
                                                   gint            *dest_x,
                                                   gint            *dest_y);
static void     gimp_zoom_preview_untransform     (GimpPreview     *preview,
                                                   gint             src_x,
                                                   gint             src_y,
                                                   gint            *dest_x,
                                                   gint            *dest_y);

static void     gimp_zoom_preview_set_drawable_id (GimpZoomPreview *preview,
                                                   gint32           drawable_ID);
static void     gimp_zoom_preview_set_model       (GimpZoomPreview *preview,
                                                   GimpZoomModel   *model);

static void     gimp_zoom_preview_get_source_area (GimpPreview     *preview,
                                                   gint            *x,
                                                   gint            *y,
                                                   gint            *w,
                                                   gint            *h);


G_DEFINE_TYPE (GimpZoomPreview, gimp_zoom_preview, GIMP_TYPE_SCROLLED_PREVIEW)

#define parent_class gimp_zoom_preview_parent_class

static gint gimp_zoom_preview_counter = 0;


static void
gimp_zoom_preview_class_init (GimpZoomPreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  GimpPreviewClass *preview_class = GIMP_PREVIEW_CLASS (klass);

  object_class->constructed   = gimp_zoom_preview_constructed;
  object_class->finalize      = gimp_zoom_preview_finalize;
  object_class->dispose       = gimp_zoom_preview_dispose;
  object_class->get_property  = gimp_zoom_preview_get_property;
  object_class->set_property  = gimp_zoom_preview_set_property;

  widget_class->style_updated = gimp_zoom_preview_style_updated;

  preview_class->draw         = gimp_zoom_preview_draw;
  preview_class->draw_buffer  = gimp_zoom_preview_draw_buffer;
  preview_class->draw_thumb   = gimp_zoom_preview_draw_thumb;
  preview_class->set_cursor   = gimp_zoom_preview_set_cursor;
  preview_class->transform    = gimp_zoom_preview_transform;
  preview_class->untransform  = gimp_zoom_preview_untransform;

  g_type_class_add_private (object_class, sizeof (GimpZoomPreviewPrivate));

  /**
   * GimpZoomPreview:drawable-id:
   *
   * The drawable the #GimpZoomPreview is attached to.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE_ID,
                                   g_param_spec_int ("drawable-id",
                                                     "Drawable ID",
                                                     "The drawable this preview is attached to",
                                                     -1, G_MAXINT, -1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  /**
   * GimpZoomPreview:model:
   *
   * The #GimpZoomModel used by this #GimpZoomPreview.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        "Model",
                                                        "The zoom preview's GimpZoomModel",
                                                        GIMP_TYPE_ZOOM_MODEL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_zoom_preview_init (GimpZoomPreview *preview)
{
  GtkWidget *area = gimp_preview_get_area (GIMP_PREVIEW (preview));

  preview->priv = G_TYPE_INSTANCE_GET_PRIVATE (preview,
                                               GIMP_TYPE_ZOOM_PREVIEW,
                                               GimpZoomPreviewPrivate);

  g_signal_connect (area, "size-allocate",
                    G_CALLBACK (gimp_zoom_preview_size_allocate),
                    preview);
  g_signal_connect (area, "scroll-event",
                    G_CALLBACK (gimp_zoom_preview_scroll_event),
                    preview);

  g_object_set (area,
                "check-size", gimp_check_size (),
                "check-type", gimp_check_type (),
                NULL);

  gimp_scrolled_preview_set_policy (GIMP_SCROLLED_PREVIEW (preview),
                                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
}

static void
gimp_zoom_preview_constructed (GObject *object)
{
  GimpZoomPreviewPrivate *priv = GET_PRIVATE (object);
  gchar                  *data_name;
  PreviewSettings         settings;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  data_name = g_strdup_printf ("%s-zoom-preview-%d",
                               g_get_prgname (),
                               gimp_zoom_preview_counter++);

  if (gimp_get_data (data_name, &settings))
    {
      gimp_preview_set_update (GIMP_PREVIEW (object), settings.update);
    }

  g_object_set_data_full (object, "gimp-zoom-preview-data-name",
                          data_name, (GDestroyNotify) g_free);

  if (! priv->model)
    {
      GimpZoomModel *model = gimp_zoom_model_new ();

      gimp_zoom_model_set_range (model, 1.0, 256.0);
      gimp_zoom_preview_set_model (GIMP_ZOOM_PREVIEW (object), model);

      g_object_unref (model);
    }

  gimp_zoom_preview_set_adjustments (GIMP_ZOOM_PREVIEW (object), 1.0, 1.0);
}

static void
gimp_zoom_preview_finalize (GObject *object)
{
  GimpZoomPreviewPrivate *priv = GET_PRIVATE (object);

  g_clear_object (&priv->model);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_zoom_preview_dispose (GObject *object)
{
  const gchar *data_name = g_object_get_data (G_OBJECT (object),
                                               "gimp-zoom-preview-data-name");

  if (data_name)
    {
      GimpPreview     *preview = GIMP_PREVIEW (object);
      PreviewSettings  settings;

      settings.update = gimp_preview_get_update (preview);

      gimp_set_data (data_name, &settings, sizeof (PreviewSettings));
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
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
    case PROP_DRAWABLE_ID:
      g_value_set_int (value, gimp_zoom_preview_get_drawable_id (preview));
      break;

    case PROP_MODEL:
      g_value_set_object (value, gimp_zoom_preview_get_model (preview));
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
    case PROP_DRAWABLE_ID:
      gimp_zoom_preview_set_drawable_id (preview, g_value_get_int (value));
      break;

    case PROP_MODEL:
      gimp_zoom_preview_set_model (preview, g_value_get_object (value));
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
  GtkAdjustment       *hadj;
  GtkAdjustment       *vadj;
  gint                 width;
  gint                 height;
  gdouble              ratio;

  gimp_scrolled_preview_freeze (scrolled_preview);

  gimp_preview_get_size (GIMP_PREVIEW (preview), &width, &height);

  ratio = new_factor / old_factor;

  gimp_scrolled_preview_get_adjustments (scrolled_preview, &hadj, &vadj);

  gtk_adjustment_configure (hadj,
                            (gtk_adjustment_get_value (vadj) + width / 2.0) * ratio
                            - width / 2.0,
                            0,
                            width * new_factor,
                            new_factor,
                            MAX (width / 2.0, new_factor),
                            width);

  gtk_adjustment_configure (vadj,
                            (gtk_adjustment_get_value (vadj) + height / 2.0) * ratio
                            - height / 2.0,
                            0,
                            height * new_factor,
                            new_factor,
                            MAX (height / 2.0, new_factor),
                            height);

  gimp_scrolled_preview_thaw (scrolled_preview);
}

static void
gimp_zoom_preview_size_allocate (GtkWidget       *widget,
                                 GtkAllocation   *allocation,
                                 GimpZoomPreview *preview)
{
  gdouble zoom;

#if 0
  gint width  = GIMP_PREVIEW (preview)->xmax - GIMP_PREVIEW (preview)->xmin;
  gint height = GIMP_PREVIEW (preview)->ymax - GIMP_PREVIEW (preview)->ymin;

  GIMP_PREVIEW (preview)->width  = MIN (width,  allocation->width);
  GIMP_PREVIEW (preview)->height = MIN (height, allocation->height);
#endif

  zoom = gimp_zoom_model_get_factor (preview->priv->model);

  gimp_zoom_preview_set_adjustments (preview, zoom, zoom);
}

static void
gimp_zoom_preview_style_updated (GtkWidget *widget)
{
  GimpPreview *preview = GIMP_PREVIEW (widget);
  GtkWidget   *area    = gimp_preview_get_area (preview);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (area)
    {
      GimpZoomPreviewPrivate *priv = GET_PRIVATE (preview);
      gint                    size;
      gint                    width;
      gint                    height;
      gint                    preview_width;
      gint                    preview_height;
      gint                    x1, y1;
      gint                    x2, y2;

      gtk_widget_style_get (widget, "size", &size, NULL);

      if (_gimp_drawable_preview_get_bounds (priv->drawable_ID,
                                             &x1, &y1, &x2, &y2))
        {
          width  = x2 - x1;
          height = y2 - y1;
        }
      else
        {
          width  = gimp_drawable_width  (priv->drawable_ID);
          height = gimp_drawable_height (priv->drawable_ID);
        }

      if (width > height)
        {
          preview_width  = MIN (width, size);
          preview_height = (height * preview_width) / width;
        }
      else
        {
          preview_height = MIN (height, size);
          preview_width  = (width * preview_height) / height;
        }

      gimp_preview_set_size (preview, preview_width, preview_height);
    }
}

static gboolean
gimp_zoom_preview_scroll_event (GtkWidget       *widget,
                                GdkEventScroll  *event,
                                GimpZoomPreview *preview)
{
  if (event->state & GDK_CONTROL_MASK)
    {
      GimpZoomPreviewPrivate *priv = GET_PRIVATE (preview);
      gdouble                 delta;

      gimp_scrolled_preview_freeze (GIMP_SCROLLED_PREVIEW (preview));

      switch (event->direction)
        {
        case GDK_SCROLL_UP:
          gimp_zoom_model_zoom (priv->model, GIMP_ZOOM_IN, 0.0);
          break;

        case GDK_SCROLL_DOWN:
          gimp_zoom_model_zoom (priv->model, GIMP_ZOOM_OUT, 0.0);
          break;

        case GDK_SCROLL_SMOOTH:
          gdk_event_get_scroll_deltas ((GdkEvent *) event, NULL, &delta);
          gimp_zoom_model_zoom (priv->model, GIMP_ZOOM_SMOOTH, delta);
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
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW (preview)->priv;
  guchar                 *data;
  gint                    width;
  gint                    height;
  gint                    bpp;

  if (! priv->model)
    return;

  if (priv->drawable_ID < 1)
    return;

  data = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                       &width, &height, &bpp);

  if (data)
    {
      GtkWidget *area = gimp_preview_get_area (preview);

      gimp_preview_area_draw (GIMP_PREVIEW_AREA (area),
                              0, 0, width, height,
                              gimp_drawable_type (priv->drawable_ID),
                              data, width * bpp);
      g_free (data);
    }
}

static void
gimp_zoom_preview_draw_buffer (GimpPreview  *preview,
                               const guchar *buffer,
                               gint          rowstride)
{
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW (preview)->priv;
  GtkWidget              *area = gimp_preview_get_area (preview);
  gint                    width;
  gint                    height;
  gint32                  image_ID;

  gimp_preview_get_size (preview, &width, &height);
  image_ID = gimp_item_get_image (priv->drawable_ID);

  if (gimp_selection_is_empty (image_ID))
    {
      gimp_preview_area_draw (GIMP_PREVIEW_AREA (area),
                              0, 0,
                              width, height,
                              gimp_drawable_type (priv->drawable_ID),
                              buffer,
                              rowstride);
    }
  else
    {
      guchar  *sel;
      guchar  *src;
      gint     selection_ID;
      gint     w, h;
      gint     bpp;
      gint     src_x;
      gint     src_y;
      gint     src_width;
      gint     src_height;
      gint     offsx = 0;
      gint     offsy = 0;

      selection_ID = gimp_image_get_selection (image_ID);

      w = width;
      h = height;

      gimp_zoom_preview_get_source_area (preview,
                                         &src_x, &src_y,
                                         &src_width, &src_height);

      src = gimp_drawable_get_sub_thumbnail_data (priv->drawable_ID,
                                                  src_x, src_y,
                                                  src_width, src_height,
                                                  &w, &h, &bpp);
      gimp_drawable_offsets (priv->drawable_ID, &offsx, &offsy);
      sel = gimp_drawable_get_sub_thumbnail_data (selection_ID,
                                                  src_x + offsx, src_y + offsy,
                                                  src_width, src_height,
                                                  &width, &height, &bpp);

      gimp_preview_area_mask (GIMP_PREVIEW_AREA (area),
                              0, 0, width, height,
                              gimp_drawable_type (priv->drawable_ID),
                              src, width * gimp_drawable_bpp (priv->drawable_ID),
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
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW (preview)->priv;

  if (priv->drawable_ID > 0)
    _gimp_drawable_preview_area_draw_thumb (area, priv->drawable_ID,
                                            width, height);
}

static void
gimp_zoom_preview_set_cursor (GimpPreview *preview)
{
  GtkWidget *area = gimp_preview_get_area (preview);

  if (! gtk_widget_get_realized (area))
    return;

  if (gimp_zoom_preview_get_factor (GIMP_ZOOM_PREVIEW (preview)) > 1.0)
    {
      GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (preview));
      GdkCursor  *cursor;

      cursor = gdk_cursor_new_for_display (display, GDK_HAND1);
      gdk_window_set_cursor (gtk_widget_get_window (area),
                             cursor);
      g_object_unref (cursor);
    }
  else
    {
      gdk_window_set_cursor (gtk_widget_get_window (area),
                             gimp_preview_get_default_cursor (preview));
    }
}

static void
gimp_zoom_preview_transform (GimpPreview *preview,
                             gint         src_x,
                             gint         src_y,
                             gint        *dest_x,
                             gint        *dest_y)
{
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW (preview)->priv;
  gint                    width;
  gint                    height;
  gint                    xoff;
  gint                    yoff;
  gdouble                 zoom;

  gimp_preview_get_size (preview, &width, &height);
  gimp_preview_get_offsets (preview, &xoff, &yoff);

  zoom = gimp_zoom_preview_get_factor (GIMP_ZOOM_PREVIEW (preview));

  *dest_x = ((gdouble) (src_x - priv->extents.x) *
             width / priv->extents.width * zoom) - xoff;

  *dest_y = ((gdouble) (src_y - priv->extents.y) *
             height / priv->extents.height * zoom) - yoff;
}

static void
gimp_zoom_preview_untransform (GimpPreview *preview,
                               gint         src_x,
                               gint         src_y,
                               gint        *dest_x,
                               gint        *dest_y)
{
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW (preview)->priv;
  gint                    width;
  gint                    height;
  gint                    xoff;
  gint                    yoff;
  gdouble                 zoom;

  gimp_preview_get_size (preview, &width, &height);
  gimp_preview_get_offsets (preview, &xoff, &yoff);

  zoom = gimp_zoom_preview_get_factor (GIMP_ZOOM_PREVIEW (preview));

  *dest_x = (priv->extents.x +
             ((gdouble) (src_x + xoff) *
              priv->extents.width / width / zoom));

  *dest_y = (priv->extents.y +
             ((gdouble) (src_y + yoff) *
              priv->extents.height / height / zoom));
}

static void
gimp_zoom_preview_set_drawable_id (GimpZoomPreview *preview,
                                   gint32           drawable_ID)
{
  GimpZoomPreviewPrivate *priv = preview->priv;
  gint                    x, y;
  gint                    width, height;
  gint                    max_width, max_height;

  g_return_if_fail (preview->priv->drawable_ID < 1);

  priv->drawable_ID = drawable_ID;

  if (gimp_drawable_mask_intersect (drawable_ID, &x, &y, &width, &height))
    {
      priv->extents.x = x;
      priv->extents.y = y;
    }
  else
    {
      width  = gimp_drawable_width  (drawable_ID);
      height = gimp_drawable_height (drawable_ID);

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

  g_object_set (gimp_preview_get_frame (GIMP_PREVIEW (preview)),
                "ratio", (gdouble) width / (gdouble) height,
                NULL);
}

static void
gimp_zoom_preview_set_model (GimpZoomPreview *preview,
                             GimpZoomModel   *model)
{
  GimpZoomPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkWidget              *button_bar;
  GtkWidget              *button;
  GtkWidget              *box;

  g_return_if_fail (priv->model == NULL);

  if (! model)
    return;

  priv->model = g_object_ref (model);

  g_signal_connect_swapped (priv->model, "zoomed",
                            G_CALLBACK (gimp_zoom_preview_set_adjustments),
                            preview);

  box = gimp_preview_get_controls (GIMP_PREVIEW (preview));
  g_return_if_fail (GTK_IS_BOX (box));

  button_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
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
}

static void
gimp_zoom_preview_get_source_area (GimpPreview *preview,
                                   gint        *x,
                                   gint        *y,
                                   gint        *w,
                                   gint        *h)
{
  GimpZoomPreviewPrivate *priv = GET_PRIVATE (preview);
  gdouble                 zoom = gimp_zoom_model_get_factor (priv->model);

  gimp_zoom_preview_untransform (preview, 0, 0, x, y);

  *w = priv->extents.width / zoom;
  *h = priv->extents.height / zoom;
}


/**
 * gimp_zoom_preview_new_from_drawable_id:
 * @drawable_ID: a drawable ID
 *
 * Creates a new #GimpZoomPreview widget for @drawable_ID.
 *
 * Since: 2.10
 *
 * Returns: a new #GimpZoomPreview.
 **/
GtkWidget *
gimp_zoom_preview_new_from_drawable_id (gint32 drawable_ID)
{
  g_return_val_if_fail (gimp_item_is_valid (drawable_ID), NULL);
  g_return_val_if_fail (gimp_item_is_drawable (drawable_ID), NULL);

  return g_object_new (GIMP_TYPE_ZOOM_PREVIEW,
                       "drawable-id", drawable_ID,
                       NULL);
}

/**
 * gimp_zoom_preview_new_with_model_from_drawable_id:
 * @drawable_ID: a drawable ID
 * @model:       a #GimpZoomModel
 *
 * Creates a new #GimpZoomPreview widget for @drawable_ID using the
 * given @model.
 *
 * This variant of gimp_zoom_preview_new_from_drawable_id() allows you
 * to create a preview using an existing zoom model. This may be
 * useful if for example you want to have two zoom previews that keep
 * their zoom factor in sync.
 *
 * Since: 2.10
 *
 * Returns: a new #GimpZoomPreview.
 **/
GtkWidget *
gimp_zoom_preview_new_with_model_from_drawable_id (gint32         drawable_ID,
                                                   GimpZoomModel *model)

{
  g_return_val_if_fail (gimp_item_is_valid (drawable_ID), NULL);
  g_return_val_if_fail (gimp_item_is_drawable (drawable_ID), NULL);
  g_return_val_if_fail (GIMP_IS_ZOOM_MODEL (model), NULL);

  return g_object_new (GIMP_TYPE_ZOOM_PREVIEW,
                       "drawable-id", drawable_ID,
                       "model",       model,
                       NULL);
}

/**
 * gimp_zoom_preview_get_drawable_id:
 * @preview: a #GimpZoomPreview widget
 *
 * Returns the drawable_ID the #GimpZoomPreview is attached to.
 *
 * Return Value: the drawable_ID that was passed to
 * gimp_zoom_preview_new_from_drawable_id().
 *
 * Since: 2.10
 **/
gint32
gimp_zoom_preview_get_drawable_id (GimpZoomPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_ZOOM_PREVIEW (preview), -1);

  return GET_PRIVATE (preview)->drawable_ID;
}

/**
 * gimp_zoom_preview_get_model:
 * @preview: a #GimpZoomPreview widget
 *
 * Returns the #GimpZoomModel the preview is using.
 *
 * Return Value: a pointer to the #GimpZoomModel owned by the @preview
 *
 * Since: 2.4
 **/
GimpZoomModel *
gimp_zoom_preview_get_model (GimpZoomPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_ZOOM_PREVIEW (preview), NULL);

  return GET_PRIVATE (preview)->model;
}

/**
 * gimp_zoom_preview_get_factor:
 * @preview: a #GimpZoomPreview widget
 *
 * Returns the zoom factor the preview is currently using.
 *
 * Return Value: the current zoom factor
 *
 * Since: 2.4
 **/
gdouble
gimp_zoom_preview_get_factor (GimpZoomPreview *preview)
{
  GimpZoomPreviewPrivate *priv;

  g_return_val_if_fail (GIMP_IS_ZOOM_PREVIEW (preview), 1.0);

  priv = GET_PRIVATE (preview);

  return priv->model ? gimp_zoom_model_get_factor (priv->model) : 1.0;
}

/**
 * gimp_zoom_preview_get_source:
 * @preview: a #GimpZoomPreview widget
 * @width: a pointer to an int where the current width of the zoom widget
 *         will be put.
 * @height: a pointer to an int where the current width of the zoom widget
 *          will be put.
 * @bpp: return location for the number of bytes per pixel
 *
 * Returns the scaled image data of the part of the drawable the
 * #GimpZoomPreview is currently showing, as a newly allocated array of guchar.
 * This function also allow to get the current width, height and bpp of the
 * #GimpZoomPreview.
 *
 * Return Value: newly allocated data that should be released using g_free()
 *               when it is not any longer needed
 *
 * Since: 2.4
 */
guchar *
gimp_zoom_preview_get_source (GimpZoomPreview *preview,
                              gint            *width,
                              gint            *height,
                              gint            *bpp)
{
  gint32 drawable_ID;

  g_return_val_if_fail (GIMP_IS_ZOOM_PREVIEW (preview), NULL);
  g_return_val_if_fail (width != NULL && height != NULL && bpp != NULL, NULL);

  drawable_ID = gimp_zoom_preview_get_drawable_id (preview);

  if (drawable_ID > 0)
    {
      GimpPreview *gimp_preview = GIMP_PREVIEW (preview);
      gint         src_x;
      gint         src_y;
      gint         src_width;
      gint         src_height;

      gimp_preview_get_size (gimp_preview, width, height);

      gimp_zoom_preview_get_source_area (gimp_preview,
                                         &src_x, &src_y,
                                         &src_width, &src_height);

      return gimp_drawable_get_sub_thumbnail_data (drawable_ID,
                                                   src_x, src_y,
                                                   src_width, src_height,
                                                   width, height, bpp);
    }
  else
    {
      *width  = 0;
      *height = 0;
      *bpp    = 0;

      return NULL;
    }
}
