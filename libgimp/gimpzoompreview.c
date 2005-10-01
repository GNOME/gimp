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

#include "libgimp-intl.h"

#include "gimpzoompreview.h"

#define SELECTION_BORDER  2

typedef struct _GimpZoomPreviewPrivate GimpZoomPreviewPrivate;

struct _GimpZoomPreviewPrivate
{
  GimpDrawable  *drawable;
  GimpZoomModel *zoom;
  GdkRectangle   extents;
};

#define GIMP_ZOOM_PREVIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMP_TYPE_ZOOM_PREVIEW, GimpZoomPreviewPrivate))

static void     gimp_zoom_preview_set_adjustments (GimpZoomPreview *preview,
                                                   gdouble          old_factor,
                                                   gdouble          new_factor);
static void     gimp_zoom_preview_size_allocate   (GtkWidget       *widget,
                                                   GtkAllocation   *allocation,
                                                   GimpZoomPreview *preview);
static void     gimp_zoom_preview_style_set       (GtkWidget       *widget,
                                                   GtkStyle        *prev_style);
static void     gimp_zoom_preview_draw            (GimpPreview     *preview);
static void     gimp_zoom_preview_draw_buffer     (GimpPreview     *preview,
                                                   const guchar    *buffer,
                                                   gint             rowstride);
static void     gimp_zoom_preview_draw_thumb      (GimpPreview     *preview,
                                                   GimpPreviewArea *area,
                                                   gint             width,
                                                   gint             height);
static gboolean gimp_zoom_preview_get_bounds      (GimpDrawable    *drawable,
                                                   gint            *xmin,
                                                   gint            *ymin,
                                                   gint            *xmax,
                                                   gint            *ymax);

G_DEFINE_TYPE (GimpZoomPreview, gimp_zoom_preview,
               GIMP_TYPE_SCROLLED_PREVIEW)
#define parent_class gimp_zoom_preview_parent_class

static void
gimp_zoom_preview_class_init (GimpZoomPreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  GimpPreviewClass *preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->style_set    = gimp_zoom_preview_style_set;

  preview_class->draw        = gimp_zoom_preview_draw;
  preview_class->draw_buffer = gimp_zoom_preview_draw_buffer;
  preview_class->draw_thumb  = gimp_zoom_preview_draw_thumb;

  g_type_class_add_private (object_class, sizeof (GimpZoomPreviewPrivate));
}

static void
gimp_zoom_preview_init (GimpZoomPreview *preview)
{
  GtkWidget              *button_bar;
  GtkWidget              *button;
  GtkWidget              *box;
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);

  priv->zoom = gimp_zoom_model_new ();
  gimp_zoom_model_set_range (GIMP_ZOOM_MODEL (priv->zoom), 1.0, 256.0);
  g_signal_connect_swapped (priv->zoom, "zoomed",
                            G_CALLBACK (gimp_zoom_preview_set_adjustments),
                            preview);

  box = gimp_preview_get_controls (GIMP_PREVIEW (preview));
  g_return_if_fail (GTK_IS_BOX (box));

  button_bar = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_end (GTK_BOX (box), button_bar, FALSE, FALSE, 0);
  gtk_widget_show (button_bar);

  /* label */
#if 0
  {
    GtkWidget *label;

    label = gimp_prop_label_new (G_OBJECT (priv->zoom), "fraction");
    gtk_misc_set_padding (GTK_MISC (label), 3, 3);
    gtk_box_pack_start (GTK_BOX (button_bar), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
  }
#endif

  /* zoom out */
  button = gimp_zoom_button_new (priv->zoom,
                                 GIMP_ZOOM_OUT, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (button_bar), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* zoom in */
  button = gimp_zoom_button_new (priv->zoom,
                                 GIMP_ZOOM_IN, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (button_bar), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (GIMP_PREVIEW (preview)->area, "size-allocate",
                    G_CALLBACK (gimp_zoom_preview_size_allocate),
                    preview);

  g_object_set (GIMP_PREVIEW (preview)->area,
                "check-size", gimp_check_size (),
                "check-type", gimp_check_type (),
                NULL);

  gimp_scrolled_preview_set_policy (GIMP_SCROLLED_PREVIEW (preview),
                                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
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

  width  = GIMP_PREVIEW (preview)->width;
  height = GIMP_PREVIEW (preview)->height;
  ratio = new_factor / old_factor;

  adj = gtk_range_get_adjustment (GTK_RANGE (scrolled_preview->hscr));
  adj->lower          = 0;
  adj->page_size      = width;
  adj->upper          = width * new_factor;
  adj->step_increment = new_factor;
  adj->page_increment = MAX (width / 2.0, adj->step_increment);
  adj->value          = CLAMP ((adj->value + width / 2.0) * ratio - width / 2.0,
                               adj->lower,
                               adj->upper - width);
  gtk_adjustment_changed (adj);
  gtk_adjustment_value_changed (adj);

  adj = gtk_range_get_adjustment (GTK_RANGE (scrolled_preview->vscr));
  adj->lower          = 0;
  adj->page_size      = height;
  adj->upper          = height * new_factor;
  adj->step_increment = new_factor;
  adj->page_increment = MAX (height / 2.0, adj->step_increment);
  adj->value          = CLAMP ((adj->value + height / 2.0) *ratio - height / 2.0,
                               adj->lower,
                               adj->upper - height);
  gtk_adjustment_changed (adj);
  gtk_adjustment_value_changed (adj);

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
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

  zoom_factor = gimp_zoom_model_get_factor (priv->zoom);
  gimp_zoom_preview_set_adjustments (preview, zoom_factor, zoom_factor);
}

static void
gimp_zoom_preview_style_set (GtkWidget *widget,
                             GtkStyle  *prev_style)
{
  GimpPreview            *preview  = GIMP_PREVIEW (widget);
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);
  GimpDrawable           *drawable = priv->drawable;
  gint                    size;
  gint                    width, height;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "size", &size,
                        NULL);
  width  = gimp_drawable_width  (drawable->drawable_id);
  height = gimp_drawable_height (drawable->drawable_id);

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

guchar *
gimp_zoom_preview_get_source (GimpZoomPreview *preview,
                              gint            *width,
                              gint            *height,
                              gint            *bpp)
{
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);
  guchar                 *data;
  GimpDrawable           *drawable = priv->drawable;
  gint                    src_x;
  gint                    src_y;
  gint                    src_width;
  gint                    src_height;
  gdouble                 zoom_factor;
  GimpPreview  *gimp_preview = GIMP_PREVIEW (preview);

  zoom_factor = gimp_zoom_model_get_factor (priv->zoom);
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

static void
gimp_zoom_preview_draw (GimpPreview *preview)
{
  guchar                 *data;
  gint                    width;
  gint                    height;
  gint                    bpp;
  gint                    src_x;
  gint                    src_y;
  gint                    src_width;
  gint                    src_height;
  GimpDrawable           *drawable;
  gdouble                 zoom_factor;
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);

  g_return_if_fail (GIMP_IS_ZOOM_PREVIEW (preview));

  drawable = priv->drawable;
  if (!drawable)
    return;
  zoom_factor = gimp_zoom_model_get_factor (priv->zoom);

  width  = preview->width;
  height = preview->height;
  src_x = priv->extents.x +
           preview->xoff * priv->extents.width / width / zoom_factor;
  src_y = priv->extents.y +
           preview->yoff * priv->extents.height / height / zoom_factor;
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
  gint32                  image_id;
  GimpZoomPreviewPrivate *priv     = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);
  GimpDrawable           *drawable = priv->drawable;

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
      guchar *sel;
      guchar *src;
      gint    selection_id;
      gint    width, height;
      gint    bpp;

      selection_id = gimp_image_get_selection (image_id);

      width  = preview->width;
      height = preview->height;

      src = gimp_drawable_get_thumbnail_data (drawable->drawable_id,
                                              &width, &height, &bpp);
      sel = gimp_drawable_get_thumbnail_data (selection_id,
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
  guchar                 *buffer;
  gint                    x1, y1, x2, y2;
  gint                    bpp;

  if (! drawable)
    return;

  if (gimp_zoom_preview_get_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      buffer = gimp_drawable_get_sub_thumbnail_data (drawable->drawable_id,
                                                     x1, y1, x2 - x1, y2 - y1,
                                                     &width, &height, &bpp);
    }
  else
    {
      buffer = gimp_drawable_get_thumbnail_data (drawable->drawable_id,
                                                 &width, &height, &bpp);
    }

  if (buffer)
    {
      GimpImageType  type;

      gtk_widget_set_size_request (GTK_WIDGET (area), width, height);
      gtk_widget_show (GTK_WIDGET (area));
      gtk_widget_realize (GTK_WIDGET (area));

      switch (bpp)
        {
        case 1:  type = GIMP_GRAY_IMAGE;   break;
        case 2:  type = GIMP_GRAYA_IMAGE;  break;
        case 3:  type = GIMP_RGB_IMAGE;    break;
        case 4:  type = GIMP_RGBA_IMAGE;   break;
        default:
          g_free (buffer);
          return;
        }
      gimp_preview_area_draw (area,
                              0, 0, width, height,
                              type, buffer, bpp * width);

      g_free (buffer);
    }
}

#define MAX3(a, b, c)  (MAX (MAX ((a), (b)), (c)))
#define MIN3(a, b, c)  (MIN (MIN ((a), (b)), (c)))

static gboolean
gimp_zoom_preview_get_bounds (GimpDrawable *drawable,
                              gint         *xmin,
                              gint         *ymin,
                              gint         *xmax,
                              gint         *ymax)
{
  gint     width;
  gint     height;
  gint     offset_x;
  gint     offset_y;
  gint     x1, y1;
  gint     x2, y2;
  gboolean retval;

  width  = gimp_drawable_width (drawable->drawable_id);
  height = gimp_drawable_height (drawable->drawable_id);

  retval = gimp_drawable_mask_bounds (drawable->drawable_id,
                                      &x1, &y1, &x2, &y2);

  gimp_drawable_offsets (drawable->drawable_id, &offset_x, &offset_y);

  *xmin = MAX3 (x1 - SELECTION_BORDER, 0, - offset_x);
  *ymin = MAX3 (y1 - SELECTION_BORDER, 0, - offset_y);
  *xmax = MIN3 (x2 + SELECTION_BORDER, width,  width  + offset_x);
  *ymax = MIN3 (y2 + SELECTION_BORDER, height, height + offset_y);

  return retval;
}

GtkWidget *
gimp_zoom_preview_new (GimpDrawable *drawable)
{
  GimpZoomPreview        *preview;
  gint                    width, height;
  gint                    max_width, max_height;
  GimpZoomPreviewPrivate *priv;

  preview = g_object_new (GIMP_TYPE_ZOOM_PREVIEW, NULL);
  priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);

  priv->drawable = drawable;
  width  = gimp_drawable_width  (drawable->drawable_id);
  height = gimp_drawable_height (drawable->drawable_id);

  priv->extents.x = 0;
  priv->extents.y = 0;
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

  gimp_zoom_preview_set_adjustments (preview, 1.0, 1.0);

  return GTK_WIDGET (preview);
}

GimpDrawable *
gimp_zoom_preview_get_drawable (GimpZoomPreview *preview)
{
  GimpZoomPreviewPrivate *priv = GIMP_ZOOM_PREVIEW_GET_PRIVATE (preview);

  return priv->drawable;
}
