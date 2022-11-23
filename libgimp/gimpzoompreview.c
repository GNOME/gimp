/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmazoompreview.c
 * Copyright (C) 2005  David Odin <dindinx@ligma.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "ligmauitypes.h"

#include "ligma.h"

#include "ligmadrawablepreview.h"
#include "ligmazoompreview.h"


/**
 * SECTION: ligmazoompreview
 * @title: LigmaZoomPreview
 * @short_description: A drawable preview with zooming capabilities.
 *
 * A drawable preview with zooming capabilities.
 **/


enum
{
  PROP_0,
  PROP_DRAWABLE,
  PROP_MODEL
};

typedef struct
{
  gboolean  update;
} PreviewSettings;


struct _LigmaZoomPreviewPrivate
{
  LigmaDrawable  *drawable;
  LigmaZoomModel *model;
  GdkRectangle   extents;
};

#define GET_PRIVATE(obj) (((LigmaZoomPreview *) (obj))->priv)


static void     ligma_zoom_preview_constructed     (GObject         *object);
static void     ligma_zoom_preview_finalize        (GObject         *object);
static void     ligma_zoom_preview_dispose         (GObject         *object);
static void     ligma_zoom_preview_get_property    (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);
static void     ligma_zoom_preview_set_property    (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);

static void     ligma_zoom_preview_set_adjustments (LigmaZoomPreview *preview,
                                                   gdouble          old_factor,
                                                   gdouble          new_factor);
static void     ligma_zoom_preview_size_allocate   (GtkWidget       *widget,
                                                   GtkAllocation   *allocation,
                                                   LigmaZoomPreview *preview);
static void     ligma_zoom_preview_style_updated   (GtkWidget       *widget);
static gboolean ligma_zoom_preview_scroll_event    (GtkWidget       *widget,
                                                   GdkEventScroll  *event,
                                                   LigmaZoomPreview *preview);
static void     ligma_zoom_preview_draw            (LigmaPreview     *preview);
static void     ligma_zoom_preview_draw_buffer     (LigmaPreview     *preview,
                                                   const guchar    *buffer,
                                                   gint             rowstride);
static void     ligma_zoom_preview_draw_thumb      (LigmaPreview     *preview,
                                                   LigmaPreviewArea *area,
                                                   gint             width,
                                                   gint             height);
static void     ligma_zoom_preview_set_cursor      (LigmaPreview     *preview);
static void     ligma_zoom_preview_transform       (LigmaPreview     *preview,
                                                   gint             src_x,
                                                   gint             src_y,
                                                   gint            *dest_x,
                                                   gint            *dest_y);
static void     ligma_zoom_preview_untransform     (LigmaPreview     *preview,
                                                   gint             src_x,
                                                   gint             src_y,
                                                   gint            *dest_x,
                                                   gint            *dest_y);

static void     ligma_zoom_preview_set_drawable    (LigmaZoomPreview *preview,
                                                   LigmaDrawable    *drawable);
static void     ligma_zoom_preview_set_model       (LigmaZoomPreview *preview,
                                                   LigmaZoomModel   *model);

static void     ligma_zoom_preview_get_source_area (LigmaPreview     *preview,
                                                   gint            *x,
                                                   gint            *y,
                                                   gint            *w,
                                                   gint            *h);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaZoomPreview, ligma_zoom_preview,
                            LIGMA_TYPE_SCROLLED_PREVIEW)

#define parent_class ligma_zoom_preview_parent_class

static gint ligma_zoom_preview_counter = 0;


static void
ligma_zoom_preview_class_init (LigmaZoomPreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  LigmaPreviewClass *preview_class = LIGMA_PREVIEW_CLASS (klass);

  object_class->constructed   = ligma_zoom_preview_constructed;
  object_class->finalize      = ligma_zoom_preview_finalize;
  object_class->dispose       = ligma_zoom_preview_dispose;
  object_class->get_property  = ligma_zoom_preview_get_property;
  object_class->set_property  = ligma_zoom_preview_set_property;

  widget_class->style_updated = ligma_zoom_preview_style_updated;

  preview_class->draw         = ligma_zoom_preview_draw;
  preview_class->draw_buffer  = ligma_zoom_preview_draw_buffer;
  preview_class->draw_thumb   = ligma_zoom_preview_draw_thumb;
  preview_class->set_cursor   = ligma_zoom_preview_set_cursor;
  preview_class->transform    = ligma_zoom_preview_transform;
  preview_class->untransform  = ligma_zoom_preview_untransform;

  /**
   * LigmaZoomPreview:drawable-id:
   *
   * The drawable the #LigmaZoomPreview is attached to.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_object ("drawable",
                                                        "Drawable",
                                                        "The drawable this preview is attached to",
                                                        LIGMA_TYPE_DRAWABLE,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /**
   * LigmaZoomPreview:model:
   *
   * The #LigmaZoomModel used by this #LigmaZoomPreview.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        "Model",
                                                        "The zoom preview's LigmaZoomModel",
                                                        LIGMA_TYPE_ZOOM_MODEL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_zoom_preview_init (LigmaZoomPreview *preview)
{
  GtkWidget *area = ligma_preview_get_area (LIGMA_PREVIEW (preview));

  preview->priv = ligma_zoom_preview_get_instance_private (preview);

  g_signal_connect (area, "size-allocate",
                    G_CALLBACK (ligma_zoom_preview_size_allocate),
                    preview);
  g_signal_connect (area, "scroll-event",
                    G_CALLBACK (ligma_zoom_preview_scroll_event),
                    preview);

  g_object_set (area,
                "check-size", ligma_check_size (),
                "check-type", ligma_check_type (),
                "check-custom-color1", ligma_check_custom_color1 (),
                "check-custom-color2", ligma_check_custom_color2 (),
                NULL);

  ligma_scrolled_preview_set_policy (LIGMA_SCROLLED_PREVIEW (preview),
                                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
}

static void
ligma_zoom_preview_constructed (GObject *object)
{
  LigmaZoomPreviewPrivate *priv = GET_PRIVATE (object);
  gchar                  *data_name;
  PreviewSettings         settings;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  data_name = g_strdup_printf ("%s-zoom-preview-%d",
                               g_get_prgname (),
                               ligma_zoom_preview_counter++);

  if (ligma_get_data (data_name, &settings))
    {
      ligma_preview_set_update (LIGMA_PREVIEW (object), settings.update);
    }

  g_object_set_data_full (object, "ligma-zoom-preview-data-name",
                          data_name, (GDestroyNotify) g_free);

  if (! priv->model)
    {
      LigmaZoomModel *model = ligma_zoom_model_new ();

      ligma_zoom_model_set_range (model, 1.0, 256.0);
      ligma_zoom_preview_set_model (LIGMA_ZOOM_PREVIEW (object), model);

      g_object_unref (model);
    }

  ligma_zoom_preview_set_adjustments (LIGMA_ZOOM_PREVIEW (object), 1.0, 1.0);
}

static void
ligma_zoom_preview_finalize (GObject *object)
{
  LigmaZoomPreviewPrivate *priv = GET_PRIVATE (object);

  g_clear_object (&priv->model);
  g_clear_object (&priv->drawable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_zoom_preview_dispose (GObject *object)
{
  const gchar *data_name = g_object_get_data (G_OBJECT (object),
                                               "ligma-zoom-preview-data-name");

  if (data_name)
    {
      LigmaPreview     *preview = LIGMA_PREVIEW (object);
      PreviewSettings  settings;

      settings.update = ligma_preview_get_update (preview);

      ligma_set_data (data_name, &settings, sizeof (PreviewSettings));
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_zoom_preview_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaZoomPreview *preview = LIGMA_ZOOM_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_value_set_object (value, ligma_zoom_preview_get_drawable (preview));
      break;

    case PROP_MODEL:
      g_value_set_object (value, ligma_zoom_preview_get_model (preview));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_zoom_preview_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaZoomPreview *preview = LIGMA_ZOOM_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      ligma_zoom_preview_set_drawable (preview, g_value_dup_object (value));
      break;

    case PROP_MODEL:
      ligma_zoom_preview_set_model (preview, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_zoom_preview_set_adjustments (LigmaZoomPreview *preview,
                                   gdouble          old_factor,
                                   gdouble          new_factor)
{
  LigmaScrolledPreview *scrolled_preview = LIGMA_SCROLLED_PREVIEW (preview);
  GtkAdjustment       *hadj;
  GtkAdjustment       *vadj;
  gint                 width;
  gint                 height;
  gdouble              ratio;

  ligma_scrolled_preview_freeze (scrolled_preview);

  ligma_preview_get_size (LIGMA_PREVIEW (preview), &width, &height);

  ratio = new_factor / old_factor;

  ligma_scrolled_preview_get_adjustments (scrolled_preview, &hadj, &vadj);

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

  ligma_scrolled_preview_thaw (scrolled_preview);
}

static void
ligma_zoom_preview_size_allocate (GtkWidget       *widget,
                                 GtkAllocation   *allocation,
                                 LigmaZoomPreview *preview)
{
  gdouble zoom;

#if 0
  gint width  = LIGMA_PREVIEW (preview)->xmax - LIGMA_PREVIEW (preview)->xmin;
  gint height = LIGMA_PREVIEW (preview)->ymax - LIGMA_PREVIEW (preview)->ymin;

  LIGMA_PREVIEW (preview)->width  = MIN (width,  allocation->width);
  LIGMA_PREVIEW (preview)->height = MIN (height, allocation->height);
#endif

  zoom = ligma_zoom_model_get_factor (preview->priv->model);

  ligma_zoom_preview_set_adjustments (preview, zoom, zoom);
}

static void
ligma_zoom_preview_style_updated (GtkWidget *widget)
{
  LigmaPreview *preview = LIGMA_PREVIEW (widget);
  GtkWidget   *area    = ligma_preview_get_area (preview);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (area)
    {
      LigmaZoomPreviewPrivate *priv = GET_PRIVATE (preview);
      gint                    size;
      gint                    width;
      gint                    height;
      gint                    preview_width;
      gint                    preview_height;
      gint                    x1, y1;
      gint                    x2, y2;

      gtk_widget_style_get (widget, "size", &size, NULL);

      if (_ligma_drawable_preview_get_bounds (priv->drawable,
                                             &x1, &y1, &x2, &y2))
        {
          width  = x2 - x1;
          height = y2 - y1;
        }
      else
        {
          width  = ligma_drawable_get_width  (priv->drawable);
          height = ligma_drawable_get_height (priv->drawable);
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

      ligma_preview_set_size (preview, preview_width, preview_height);
    }
}

static gboolean
ligma_zoom_preview_scroll_event (GtkWidget       *widget,
                                GdkEventScroll  *event,
                                LigmaZoomPreview *preview)
{
  if (event->state & GDK_CONTROL_MASK)
    {
      LigmaZoomPreviewPrivate *priv = GET_PRIVATE (preview);
      gdouble                 delta;

      ligma_scrolled_preview_freeze (LIGMA_SCROLLED_PREVIEW (preview));

      switch (event->direction)
        {
        case GDK_SCROLL_UP:
          ligma_zoom_model_zoom (priv->model, LIGMA_ZOOM_IN, 0.0);
          break;

        case GDK_SCROLL_DOWN:
          ligma_zoom_model_zoom (priv->model, LIGMA_ZOOM_OUT, 0.0);
          break;

        case GDK_SCROLL_SMOOTH:
          gdk_event_get_scroll_deltas ((GdkEvent *) event, NULL, &delta);
          ligma_zoom_model_zoom (priv->model, LIGMA_ZOOM_SMOOTH, delta);
          break;

        default:
          break;
        }

      ligma_scrolled_preview_thaw (LIGMA_SCROLLED_PREVIEW (preview));
    }

  return FALSE;
}

static void
ligma_zoom_preview_draw (LigmaPreview *preview)
{
  LigmaZoomPreviewPrivate *priv = LIGMA_ZOOM_PREVIEW (preview)->priv;
  guchar                 *data;
  gint                    width;
  gint                    height;
  gint                    bpp;

  if (! priv->model)
    return;

  if (priv->drawable == NULL)
    return;

  data = ligma_zoom_preview_get_source (LIGMA_ZOOM_PREVIEW (preview),
                                       &width, &height, &bpp);

  if (data)
    {
      GtkWidget *area = ligma_preview_get_area (preview);

      ligma_preview_area_draw (LIGMA_PREVIEW_AREA (area),
                              0, 0, width, height,
                              ligma_drawable_type (priv->drawable),
                              data, width * bpp);
      g_free (data);
    }
}

static void
ligma_zoom_preview_draw_buffer (LigmaPreview  *preview,
                               const guchar *buffer,
                               gint          rowstride)
{
  LigmaZoomPreviewPrivate *priv = LIGMA_ZOOM_PREVIEW (preview)->priv;
  GtkWidget              *area = ligma_preview_get_area (preview);
  LigmaImage              *image;
  gint                    width;
  gint                    height;

  ligma_preview_get_size (preview, &width, &height);
  image = ligma_item_get_image (LIGMA_ITEM (priv->drawable));

  if (ligma_selection_is_empty (image))
    {
      ligma_preview_area_draw (LIGMA_PREVIEW_AREA (area),
                              0, 0,
                              width, height,
                              ligma_drawable_type (priv->drawable),
                              buffer,
                              rowstride);
    }
  else
    {
      guchar        *sel;
      guchar        *src;
      LigmaSelection *selection;
      gint           w, h;
      gint           bpp;
      gint           src_x;
      gint           src_y;
      gint           src_width;
      gint           src_height;
      gint           offsx = 0;
      gint           offsy = 0;

      selection = ligma_image_get_selection (image);

      w = width;
      h = height;

      ligma_zoom_preview_get_source_area (preview,
                                         &src_x, &src_y,
                                         &src_width, &src_height);

      src = ligma_drawable_get_sub_thumbnail_data (priv->drawable,
                                                  src_x, src_y,
                                                  src_width, src_height,
                                                  &w, &h, &bpp);
      ligma_drawable_get_offsets (priv->drawable, &offsx, &offsy);
      sel = ligma_drawable_get_sub_thumbnail_data (LIGMA_DRAWABLE (selection),
                                                  src_x + offsx, src_y + offsy,
                                                  src_width, src_height,
                                                  &width, &height, &bpp);

      ligma_preview_area_mask (LIGMA_PREVIEW_AREA (area),
                              0, 0, width, height,
                              ligma_drawable_type (priv->drawable),
                              src, width * ligma_drawable_get_bpp (priv->drawable),
                              buffer, rowstride,
                              sel, width);

      g_free (sel);
      g_free (src);
    }
}

static void
ligma_zoom_preview_draw_thumb (LigmaPreview     *preview,
                              LigmaPreviewArea *area,
                              gint             width,
                              gint             height)
{
  LigmaZoomPreviewPrivate *priv = LIGMA_ZOOM_PREVIEW (preview)->priv;

  if (priv->drawable != NULL)
    _ligma_drawable_preview_area_draw_thumb (area, priv->drawable,
                                            width, height);
}

static void
ligma_zoom_preview_set_cursor (LigmaPreview *preview)
{
  GtkWidget *area = ligma_preview_get_area (preview);

  if (! gtk_widget_get_realized (area))
    return;

  if (ligma_zoom_preview_get_factor (LIGMA_ZOOM_PREVIEW (preview)) > 1.0)
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
                             ligma_preview_get_default_cursor (preview));
    }
}

static void
ligma_zoom_preview_transform (LigmaPreview *preview,
                             gint         src_x,
                             gint         src_y,
                             gint        *dest_x,
                             gint        *dest_y)
{
  LigmaZoomPreviewPrivate *priv = LIGMA_ZOOM_PREVIEW (preview)->priv;
  gint                    width;
  gint                    height;
  gint                    xoff;
  gint                    yoff;
  gdouble                 zoom;

  ligma_preview_get_size (preview, &width, &height);
  ligma_preview_get_offsets (preview, &xoff, &yoff);

  zoom = ligma_zoom_preview_get_factor (LIGMA_ZOOM_PREVIEW (preview));

  *dest_x = ((gdouble) (src_x - priv->extents.x) *
             width / priv->extents.width * zoom) - xoff;

  *dest_y = ((gdouble) (src_y - priv->extents.y) *
             height / priv->extents.height * zoom) - yoff;
}

static void
ligma_zoom_preview_untransform (LigmaPreview *preview,
                               gint         src_x,
                               gint         src_y,
                               gint        *dest_x,
                               gint        *dest_y)
{
  LigmaZoomPreviewPrivate *priv = LIGMA_ZOOM_PREVIEW (preview)->priv;
  gint                    width;
  gint                    height;
  gint                    xoff;
  gint                    yoff;
  gdouble                 zoom;

  ligma_preview_get_size (preview, &width, &height);
  ligma_preview_get_offsets (preview, &xoff, &yoff);

  zoom = ligma_zoom_preview_get_factor (LIGMA_ZOOM_PREVIEW (preview));

  *dest_x = (priv->extents.x +
             ((gdouble) (src_x + xoff) *
              priv->extents.width / width / zoom));

  *dest_y = (priv->extents.y +
             ((gdouble) (src_y + yoff) *
              priv->extents.height / height / zoom));
}

static void
ligma_zoom_preview_set_drawable (LigmaZoomPreview *preview,
                                LigmaDrawable    *drawable)
{
  LigmaZoomPreviewPrivate *priv = preview->priv;
  gint                    x, y;
  gint                    width, height;
  gint                    max_width, max_height;

  g_return_if_fail (preview->priv->drawable == NULL);

  priv->drawable = drawable;

  if (ligma_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    {
      priv->extents.x = x;
      priv->extents.y = y;
    }
  else
    {
      width  = ligma_drawable_get_width  (drawable);
      height = ligma_drawable_get_height (drawable);

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

  ligma_preview_set_bounds (LIGMA_PREVIEW (preview),
                           0, 0, max_width, max_height);

  g_object_set (ligma_preview_get_frame (LIGMA_PREVIEW (preview)),
                "ratio", (gdouble) width / (gdouble) height,
                NULL);
}

static void
ligma_zoom_preview_set_model (LigmaZoomPreview *preview,
                             LigmaZoomModel   *model)
{
  LigmaZoomPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkWidget              *button_bar;
  GtkWidget              *button;
  GtkWidget              *box;

  g_return_if_fail (priv->model == NULL);

  if (! model)
    return;

  priv->model = g_object_ref (model);

  g_signal_connect_swapped (priv->model, "zoomed",
                            G_CALLBACK (ligma_zoom_preview_set_adjustments),
                            preview);

  box = ligma_preview_get_controls (LIGMA_PREVIEW (preview));
  g_return_if_fail (GTK_IS_BOX (box));

  button_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_end (GTK_BOX (box), button_bar, FALSE, FALSE, 0);
  gtk_widget_show (button_bar);

  /* zoom out */
  button = ligma_zoom_button_new (priv->model,
                                 LIGMA_ZOOM_OUT, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (button_bar), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* zoom in */
  button = ligma_zoom_button_new (priv->model,
                                 LIGMA_ZOOM_IN, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (button_bar), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
}

static void
ligma_zoom_preview_get_source_area (LigmaPreview *preview,
                                   gint        *x,
                                   gint        *y,
                                   gint        *w,
                                   gint        *h)
{
  LigmaZoomPreviewPrivate *priv = GET_PRIVATE (preview);
  gdouble                 zoom = ligma_zoom_model_get_factor (priv->model);

  ligma_zoom_preview_untransform (preview, 0, 0, x, y);

  *w = priv->extents.width / zoom;
  *h = priv->extents.height / zoom;
}


/**
 * ligma_zoom_preview_new_from_drawable:
 * @drawable: (transfer none): a drawable
 *
 * Creates a new #LigmaZoomPreview widget for @drawable.
 *
 * Since: 3.0
 *
 * Returns: a new #LigmaZoomPreview.
 **/
GtkWidget *
ligma_zoom_preview_new_from_drawable (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_item_is_valid (LIGMA_ITEM (drawable)), NULL);

  return g_object_new (LIGMA_TYPE_ZOOM_PREVIEW,
                       "drawable", drawable,
                       NULL);
}

/**
 * ligma_zoom_preview_new_with_model_from_drawable:
 * @drawable: a #LigmaDrawable
 * @model:    a #LigmaZoomModel
 *
 * Creates a new #LigmaZoomPreview widget for @drawable using the
 * given @model.
 *
 * This variant of ligma_zoom_preview_new_from_drawable() allows you
 * to create a preview using an existing zoom model. This may be
 * useful if for example you want to have two zoom previews that keep
 * their zoom factor in sync.
 *
 * Since: 2.10
 *
 * Returns: a new #LigmaZoomPreview.
 **/
GtkWidget *
ligma_zoom_preview_new_with_model_from_drawable (LigmaDrawable  *drawable,
                                                LigmaZoomModel *model)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_item_is_valid (LIGMA_ITEM (drawable)), NULL);
  g_return_val_if_fail (LIGMA_IS_ZOOM_MODEL (model), NULL);

  return g_object_new (LIGMA_TYPE_ZOOM_PREVIEW,
                       "drawable", drawable,
                       "model",    model,
                       NULL);
}

/**
 * ligma_zoom_preview_get_drawable:
 * @preview: a #LigmaZoomPreview widget
 *
 * Returns the drawable the #LigmaZoomPreview is attached to.
 *
 * Returns: (transfer none): the drawable that was passed to
 *          ligma_zoom_preview_new_from_drawable().
 *
 * Since: 3.0
 **/
LigmaDrawable *
ligma_zoom_preview_get_drawable (LigmaZoomPreview *preview)
{
  g_return_val_if_fail (LIGMA_IS_ZOOM_PREVIEW (preview), NULL);

  return GET_PRIVATE (preview)->drawable;
}

/**
 * ligma_zoom_preview_get_model:
 * @preview: a #LigmaZoomPreview widget
 *
 * Returns the #LigmaZoomModel the preview is using.
 *
 * Returns: (transfer none): a pointer to the #LigmaZoomModel owned
 *          by the @preview
 *
 * Since: 2.4
 **/
LigmaZoomModel *
ligma_zoom_preview_get_model (LigmaZoomPreview *preview)
{
  g_return_val_if_fail (LIGMA_IS_ZOOM_PREVIEW (preview), NULL);

  return GET_PRIVATE (preview)->model;
}

/**
 * ligma_zoom_preview_get_factor:
 * @preview: a #LigmaZoomPreview widget
 *
 * Returns the zoom factor the preview is currently using.
 *
 * Returns: the current zoom factor
 *
 * Since: 2.4
 **/
gdouble
ligma_zoom_preview_get_factor (LigmaZoomPreview *preview)
{
  LigmaZoomPreviewPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_ZOOM_PREVIEW (preview), 1.0);

  priv = GET_PRIVATE (preview);

  return priv->model ? ligma_zoom_model_get_factor (priv->model) : 1.0;
}

/**
 * ligma_zoom_preview_get_source:
 * @preview: a #LigmaZoomPreview widget
 * @width: (out): a pointer to an int where the current width of the zoom widget
 *         will be put.
 * @height: (out): a pointer to an int where the current width of the zoom widget
 *          will be put.
 * @bpp: (out): return location for the number of bytes per pixel
 *
 * Returns the scaled image data of the part of the drawable the
 * #LigmaZoomPreview is currently showing, as a newly allocated array of guchar.
 * This function also allow to get the current width, height and bpp of the
 * #LigmaZoomPreview.
 *
 * Returns: (transfer full) (array): newly allocated data that should be
 *          released using g_free() when it is not any longer needed
 *
 * Since: 2.4
 */
guchar *
ligma_zoom_preview_get_source (LigmaZoomPreview *preview,
                              gint            *width,
                              gint            *height,
                              gint            *bpp)
{
  LigmaDrawable *drawable;

  g_return_val_if_fail (LIGMA_IS_ZOOM_PREVIEW (preview), NULL);
  g_return_val_if_fail (width != NULL && height != NULL && bpp != NULL, NULL);

  drawable = ligma_zoom_preview_get_drawable (preview);

  if (drawable)
    {
      LigmaPreview *ligma_preview = LIGMA_PREVIEW (preview);
      gint         src_x;
      gint         src_y;
      gint         src_width;
      gint         src_height;

      ligma_preview_get_size (ligma_preview, width, height);

      ligma_zoom_preview_get_source_area (ligma_preview,
                                         &src_x, &src_y,
                                         &src_width, &src_height);

      return ligma_drawable_get_sub_thumbnail_data (drawable,
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
