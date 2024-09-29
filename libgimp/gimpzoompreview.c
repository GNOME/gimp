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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimpuitypes.h"

#include "gimp.h"
#include "gimppdb-private.h"

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
  PROP_DRAWABLE,
  PROP_MODEL
};

typedef struct
{
  gboolean  update;
} PreviewSettings;


struct _GimpZoomPreview
{
  GimpScrolledPreview  parent_instance;

  GimpDrawable        *drawable;
  GimpZoomModel       *model;
  GdkRectangle         extents;
};


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

static void     gimp_zoom_preview_set_drawable    (GimpZoomPreview *preview,
                                                   GimpDrawable    *drawable);
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

  /**
   * GimpZoomPreview:drawable-id:
   *
   * The drawable the #GimpZoomPreview is attached to.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_object ("drawable",
                                                        "Drawable",
                                                        "The drawable this preview is attached to",
                                                        GIMP_TYPE_DRAWABLE,
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

  g_signal_connect (area, "size-allocate",
                    G_CALLBACK (gimp_zoom_preview_size_allocate),
                    preview);
  g_signal_connect (area, "scroll-event",
                    G_CALLBACK (gimp_zoom_preview_scroll_event),
                    preview);

  g_object_set (area,
                "check-size", gimp_check_size (),
                "check-type", gimp_check_type (),
                "check-custom-color1", gimp_check_custom_color1 (),
                "check-custom-color2", gimp_check_custom_color2 (),
                NULL);

  gimp_scrolled_preview_set_policy (GIMP_SCROLLED_PREVIEW (preview),
                                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
}

static void
gimp_zoom_preview_constructed (GObject *object)
{
  GimpZoomPreview *preview = GIMP_ZOOM_PREVIEW (object);
  gchar           *data_name;
  PreviewSettings  settings;
  GBytes          *settings_bytes = NULL;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  data_name = g_strdup_printf ("%s-zoom-preview-%d",
                               g_get_prgname (),
                               gimp_zoom_preview_counter++);

  if (gimp_pdb_get_data (data_name, &settings_bytes) &&
      g_bytes_get_size (settings_bytes) == sizeof (PreviewSettings))
    {
      settings = *((PreviewSettings *) g_bytes_get_data (settings_bytes, NULL));
      gimp_preview_set_update (GIMP_PREVIEW (object), settings.update);
    }
  g_bytes_unref (settings_bytes);

  g_object_set_data_full (object, "gimp-zoom-preview-data-name",
                          data_name, (GDestroyNotify) g_free);

  if (! preview->model)
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
  GimpZoomPreview *preview = GIMP_ZOOM_PREVIEW (object);

  g_clear_object (&preview->model);
  g_clear_object (&preview->drawable);

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
      GBytes          *bytes;
      PreviewSettings  settings;

      settings.update = gimp_preview_get_update (preview);

      bytes = g_bytes_new_static (&settings, sizeof (PreviewSettings));
      gimp_pdb_set_data (data_name, bytes);
      g_bytes_unref (bytes);
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
    case PROP_DRAWABLE:
      g_value_set_object (value, gimp_zoom_preview_get_drawable (preview));
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
    case PROP_DRAWABLE:
      gimp_zoom_preview_set_drawable (preview, g_value_dup_object (value));
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

  zoom = gimp_zoom_model_get_factor (preview->model);

  gimp_zoom_preview_set_adjustments (preview, zoom, zoom);
}

static void
gimp_zoom_preview_style_updated (GtkWidget *widget)
{
  GimpZoomPreview *zoom_preview = GIMP_ZOOM_PREVIEW (widget);
  GimpPreview     *preview      = GIMP_PREVIEW (widget);
  GtkWidget       *area         = gimp_preview_get_area (preview);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (area)
    {
      gint size;
      gint width;
      gint height;
      gint preview_width;
      gint preview_height;
      gint x1, y1;
      gint x2, y2;

      gtk_widget_style_get (widget, "size", &size, NULL);

      if (_gimp_drawable_preview_get_bounds (zoom_preview->drawable,
                                             &x1, &y1, &x2, &y2))
        {
          width  = x2 - x1;
          height = y2 - y1;
        }
      else
        {
          width  = gimp_drawable_get_width  (zoom_preview->drawable);
          height = gimp_drawable_get_height (zoom_preview->drawable);
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
      gdouble delta;

      gimp_scrolled_preview_freeze (GIMP_SCROLLED_PREVIEW (preview));

      switch (event->direction)
        {
        case GDK_SCROLL_UP:
          gimp_zoom_model_zoom (preview->model, GIMP_ZOOM_IN, 0.0);
          break;

        case GDK_SCROLL_DOWN:
          gimp_zoom_model_zoom (preview->model, GIMP_ZOOM_OUT, 0.0);
          break;

        case GDK_SCROLL_SMOOTH:
          gdk_event_get_scroll_deltas ((GdkEvent *) event, NULL, &delta);
          gimp_zoom_model_zoom (preview->model, GIMP_ZOOM_SMOOTH, delta);
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
  GimpZoomPreview *zoom_preview = GIMP_ZOOM_PREVIEW (preview);
  guchar          *data;
  gint             width;
  gint             height;
  gint             bpp;

  if (! zoom_preview->model)
    return;

  if (zoom_preview->drawable == NULL)
    return;

  data = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                       &width, &height, &bpp);

  if (data)
    {
      GtkWidget *area = gimp_preview_get_area (preview);

      gimp_preview_area_draw (GIMP_PREVIEW_AREA (area),
                              0, 0, width, height,
                              gimp_drawable_type (zoom_preview->drawable),
                              data, width * bpp);
      g_free (data);
    }
}

static void
gimp_zoom_preview_draw_buffer (GimpPreview  *preview,
                               const guchar *buffer,
                               gint          rowstride)
{
  GimpZoomPreview *zoom_preview = GIMP_ZOOM_PREVIEW (preview);
  GtkWidget       *area         = gimp_preview_get_area (preview);
  GimpImage       *image;
  gint             width;
  gint             height;

  gimp_preview_get_size (preview, &width, &height);
  image = gimp_item_get_image (GIMP_ITEM (zoom_preview->drawable));

  if (gimp_selection_is_empty (image))
    {
      gimp_preview_area_draw (GIMP_PREVIEW_AREA (area),
                              0, 0,
                              width, height,
                              gimp_drawable_type (zoom_preview->drawable),
                              buffer,
                              rowstride);
    }
  else
    {
      GBytes        *sel;
      GBytes        *src;
      GimpSelection *selection;
      gint           w, h;
      gint           bpp;
      gint           src_x;
      gint           src_y;
      gint           src_width;
      gint           src_height;
      gint           offsx = 0;
      gint           offsy = 0;

      selection = gimp_image_get_selection (image);

      gimp_zoom_preview_get_source_area (preview,
                                         &src_x, &src_y,
                                         &src_width, &src_height);

      src = gimp_drawable_get_sub_thumbnail_data (zoom_preview->drawable,
                                                  src_x, src_y,
                                                  src_width, src_height,
                                                  width, height,
                                                  &w, &h, &bpp);
      gimp_drawable_get_offsets (zoom_preview->drawable, &offsx, &offsy);
      sel = gimp_drawable_get_sub_thumbnail_data (GIMP_DRAWABLE (selection),
                                                  src_x + offsx, src_y + offsy,
                                                  src_width, src_height,
                                                  width, height,
                                                  &w, &h, &bpp);

      gimp_preview_area_mask (GIMP_PREVIEW_AREA (area),
                              0, 0, width, height,
                              gimp_drawable_type (zoom_preview->drawable),
                              g_bytes_get_data (src, NULL), width * gimp_drawable_get_bpp (zoom_preview->drawable),
                              buffer, rowstride,
                              g_bytes_get_data (sel, NULL), width);

      g_bytes_unref (sel);
      g_bytes_unref (src);
    }
}

static void
gimp_zoom_preview_draw_thumb (GimpPreview     *preview,
                              GimpPreviewArea *area,
                              gint             width,
                              gint             height)
{
  GimpZoomPreview *zoom_preview = GIMP_ZOOM_PREVIEW (preview);

  if (zoom_preview->drawable != NULL)
    _gimp_drawable_preview_area_draw_thumb (area, zoom_preview->drawable,
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
  GimpZoomPreview *zoom_preview = GIMP_ZOOM_PREVIEW (preview);
  gint             width;
  gint             height;
  gint             xoff;
  gint             yoff;
  gdouble          zoom;

  gimp_preview_get_size (preview, &width, &height);
  gimp_preview_get_offsets (preview, &xoff, &yoff);

  zoom = gimp_zoom_preview_get_factor (GIMP_ZOOM_PREVIEW (preview));

  *dest_x = ((gdouble) (src_x - zoom_preview->extents.x) *
             width / zoom_preview->extents.width * zoom) - xoff;

  *dest_y = ((gdouble) (src_y - zoom_preview->extents.y) *
             height / zoom_preview->extents.height * zoom) - yoff;
}

static void
gimp_zoom_preview_untransform (GimpPreview *preview,
                               gint         src_x,
                               gint         src_y,
                               gint        *dest_x,
                               gint        *dest_y)
{
  GimpZoomPreview *zoom_preview = GIMP_ZOOM_PREVIEW (preview);
  gint             width;
  gint             height;
  gint             xoff;
  gint             yoff;
  gdouble          zoom;

  gimp_preview_get_size (preview, &width, &height);
  gimp_preview_get_offsets (preview, &xoff, &yoff);

  zoom = gimp_zoom_preview_get_factor (GIMP_ZOOM_PREVIEW (preview));

  *dest_x = (zoom_preview->extents.x +
             ((gdouble) (src_x + xoff) *
              zoom_preview->extents.width / width / zoom));

  *dest_y = (zoom_preview->extents.y +
             ((gdouble) (src_y + yoff) *
              zoom_preview->extents.height / height / zoom));
}

static void
gimp_zoom_preview_set_drawable (GimpZoomPreview *preview,
                                GimpDrawable    *drawable)
{
  gint x, y;
  gint width, height;
  gint max_width, max_height;

  g_return_if_fail (preview->drawable == NULL);

  preview->drawable = drawable;

  if (gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    {
      preview->extents.x = x;
      preview->extents.y = y;
    }
  else
    {
      width  = gimp_drawable_get_width  (drawable);
      height = gimp_drawable_get_height (drawable);

      preview->extents.x = 0;
      preview->extents.y = 0;
    }

  preview->extents.width  = width;
  preview->extents.height = height;

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
  GtkWidget *button_bar;
  GtkWidget *button;
  GtkWidget *box;

  g_return_if_fail (preview->model == NULL);

  if (! model)
    return;

  preview->model = g_object_ref (model);

  g_signal_connect_swapped (preview->model, "zoomed",
                            G_CALLBACK (gimp_zoom_preview_set_adjustments),
                            preview);

  box = gimp_preview_get_controls (GIMP_PREVIEW (preview));
  g_return_if_fail (GTK_IS_BOX (box));

  button_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_end (GTK_BOX (box), button_bar, FALSE, FALSE, 0);
  gtk_widget_show (button_bar);

  /* zoom out */
  button = gimp_zoom_button_new (preview->model,
                                 GIMP_ZOOM_OUT, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (button_bar), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* zoom in */
  button = gimp_zoom_button_new (preview->model,
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
  GimpZoomPreview *zoom_preview = GIMP_ZOOM_PREVIEW (preview);
  gdouble          zoom         = gimp_zoom_model_get_factor (zoom_preview->model);

  gimp_zoom_preview_untransform (preview, 0, 0, x, y);

  *w = zoom_preview->extents.width / zoom;
  *h = zoom_preview->extents.height / zoom;
}


/**
 * gimp_zoom_preview_new_from_drawable:
 * @drawable: (transfer none): a drawable
 *
 * Creates a new #GimpZoomPreview widget for @drawable.
 *
 * Since: 3.0
 *
 * Returns: a new #GimpZoomPreview.
 **/
GtkWidget *
gimp_zoom_preview_new_from_drawable (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_valid (GIMP_ITEM (drawable)), NULL);

  return g_object_new (GIMP_TYPE_ZOOM_PREVIEW,
                       "drawable", drawable,
                       NULL);
}

/**
 * gimp_zoom_preview_new_with_model_from_drawable:
 * @drawable: a #GimpDrawable
 * @model:    a #GimpZoomModel
 *
 * Creates a new #GimpZoomPreview widget for @drawable using the
 * given @model.
 *
 * This variant of gimp_zoom_preview_new_from_drawable() allows you
 * to create a preview using an existing zoom model. This may be
 * useful if for example you want to have two zoom previews that keep
 * their zoom factor in sync.
 *
 * Since: 2.10
 *
 * Returns: a new #GimpZoomPreview.
 **/
GtkWidget *
gimp_zoom_preview_new_with_model_from_drawable (GimpDrawable  *drawable,
                                                GimpZoomModel *model)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_valid (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_ZOOM_MODEL (model), NULL);

  return g_object_new (GIMP_TYPE_ZOOM_PREVIEW,
                       "drawable", drawable,
                       "model",    model,
                       NULL);
}

/**
 * gimp_zoom_preview_get_drawable:
 * @preview: a #GimpZoomPreview widget
 *
 * Returns the drawable the #GimpZoomPreview is attached to.
 *
 * Returns: (transfer none): the drawable that was passed to
 *          gimp_zoom_preview_new_from_drawable().
 *
 * Since: 3.0
 **/
GimpDrawable *
gimp_zoom_preview_get_drawable (GimpZoomPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_ZOOM_PREVIEW (preview), NULL);

  return preview->drawable;
}

/**
 * gimp_zoom_preview_get_model:
 * @preview: a #GimpZoomPreview widget
 *
 * Returns the #GimpZoomModel the preview is using.
 *
 * Returns: (transfer none): a pointer to the #GimpZoomModel owned
 *          by the @preview
 *
 * Since: 2.4
 **/
GimpZoomModel *
gimp_zoom_preview_get_model (GimpZoomPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_ZOOM_PREVIEW (preview), NULL);

  return preview->model;
}

/**
 * gimp_zoom_preview_get_factor:
 * @preview: a #GimpZoomPreview widget
 *
 * Returns the zoom factor the preview is currently using.
 *
 * Returns: the current zoom factor
 *
 * Since: 2.4
 **/
gdouble
gimp_zoom_preview_get_factor (GimpZoomPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_ZOOM_PREVIEW (preview), 1.0);

  return preview->model ? gimp_zoom_model_get_factor (preview->model) : 1.0;
}

/**
 * gimp_zoom_preview_get_source:
 * @preview: a #GimpZoomPreview widget
 * @width: (out): a pointer to an int where the current width of the zoom widget
 *         will be put.
 * @height: (out): a pointer to an int where the current width of the zoom widget
 *          will be put.
 * @bpp: (out): return location for the number of bytes per pixel
 *
 * Returns the scaled image data of the part of the drawable the
 * #GimpZoomPreview is currently showing, as a newly allocated array of guchar.
 * This function also allow to get the current width, height and bpp of the
 * #GimpZoomPreview.
 *
 * Returns: (transfer full) (array): newly allocated data that should be
 *          released using g_free() when it is not any longer needed
 *
 * Since: 2.4
 */
guchar *
gimp_zoom_preview_get_source (GimpZoomPreview *preview,
                              gint            *width,
                              gint            *height,
                              gint            *bpp)
{
  GimpDrawable *drawable;

  g_return_val_if_fail (GIMP_IS_ZOOM_PREVIEW (preview), NULL);
  g_return_val_if_fail (width != NULL && height != NULL && bpp != NULL, NULL);

  drawable = gimp_zoom_preview_get_drawable (preview);

  if (drawable)
    {
      GimpPreview *gimp_preview = GIMP_PREVIEW (preview);
      gint         src_x;
      gint         src_y;
      gint         src_width;
      gint         src_height;
      GBytes      *src_bytes;
      gsize        src_bytes_size;

      gimp_preview_get_size (gimp_preview, width, height);

      gimp_zoom_preview_get_source_area (gimp_preview,
                                         &src_x, &src_y,
                                         &src_width, &src_height);

      src_bytes =  gimp_drawable_get_sub_thumbnail_data (drawable,
                                                         src_x, src_y,
                                                         src_width, src_height,
                                                         *width, *height,
                                                         width, height, bpp);
      return g_bytes_unref_to_data (src_bytes, &src_bytes_size);
    }
  else
    {
      *width  = 0;
      *height = 0;
      *bpp    = 0;

      return NULL;
    }
}
