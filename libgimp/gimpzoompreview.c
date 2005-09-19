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

static void     gimp_zoom_preview_set_adjustments    (GimpZoomPreview      *preview);
static void     gimp_zoom_preview_size_allocate      (GtkWidget            *widget,
                                                      GtkAllocation        *allocation,
                                                      GimpZoomPreview      *preview);
static void     gimp_zoom_preview_style_set          (GtkWidget            *widget,
                                                      GtkStyle             *prev_style);
static void     gimp_zoom_preview_draw               (GimpPreview          *preview);
static void     gimp_zoom_preview_draw_buffer        (GimpPreview          *preview,
                                                      const guchar         *buffer,
                                                      gint                  rowstride);
static void     gimp_zoom_preview_draw_thumb         (GimpPreview          *preview,
                                                      GimpPreviewArea      *area,
                                                      gint                  width,
                                                      gint                  height);
static gboolean gimp_zoom_preview_get_bounds         (GimpDrawable         *drawable,
                                                      gint                 *xmin,
                                                      gint                 *ymin,
                                                      gint                 *xmax,
                                                      gint                 *ymax);

G_DEFINE_TYPE (GimpZoomPreview, gimp_zoom_preview,
               GIMP_TYPE_SCROLLED_PREVIEW)
#define parent_class gimp_zoom_preview_parent_class

static void
gimp_zoom_preview_class_init (GimpZoomPreviewClass *klass)
{
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  GimpPreviewClass *preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->style_set    = gimp_zoom_preview_style_set;

  preview_class->draw        = gimp_zoom_preview_draw;
  preview_class->draw_buffer = gimp_zoom_preview_draw_buffer;
  preview_class->draw_thumb  = gimp_zoom_preview_draw_thumb;
}

static void
gimp_zoom_preview_init (GimpZoomPreview *preview)
{
  GtkWidget *button_bar;
  GtkWidget *button;
  GtkWidget *label;

  preview->zoom = gimp_zoom_model_new (1.1);
  g_signal_connect_swapped (preview->zoom, "notify::zoom-factor",
                            G_CALLBACK (gimp_zoom_preview_set_adjustments),
                            preview);

  button_bar = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (preview), button_bar, FALSE, FALSE, 0);
  gtk_widget_show (button_bar);

  /* zoom out */
  button = gimp_zoom_widget_new (preview->zoom, GIMP_ZOOM_OUT_BUTTON);
  gtk_box_pack_start (GTK_BOX (button_bar), button,
                      FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* zoom in */
  button = gimp_zoom_widget_new (preview->zoom, GIMP_ZOOM_IN_BUTTON);
  gtk_box_pack_start (GTK_BOX (button_bar), button,
                      FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* label */
  label = gimp_zoom_widget_new (preview->zoom, GIMP_ZOOM_LABEL);
  gtk_box_pack_start (GTK_BOX (button_bar), label,
                      FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_signal_connect (GIMP_PREVIEW (preview)->area, "size-allocate",
                    G_CALLBACK (gimp_zoom_preview_size_allocate),
                    preview);

  g_object_set (GIMP_PREVIEW (preview)->area,
                "check-size", gimp_check_size (),
                "check-type", gimp_check_type (),
                NULL);
}

static void
gimp_zoom_preview_set_adjustments (GimpZoomPreview *preview)
{
  GimpScrolledPreview *scrolled_preview;
  GtkAdjustment       *adj;
  gdouble              zoom_factor;

  scrolled_preview = GIMP_SCROLLED_PREVIEW (preview);
  zoom_factor = gimp_zoom_model_get_factor (preview->zoom);

  if (fabs (zoom_factor - 1.0) < 0.05)
    {
      gtk_widget_hide (scrolled_preview->vscr);
      gtk_widget_hide (scrolled_preview->hscr);
      gtk_widget_hide (scrolled_preview->nav_icon);
    }
  else
    {
      adj = gtk_range_get_adjustment (GTK_RANGE (scrolled_preview->hscr));
      adj->lower          = 0;
      adj->page_size      = GIMP_PREVIEW (preview)->width;
      adj->upper          = GIMP_PREVIEW (preview)->width * zoom_factor;
      adj->step_increment = 1.0;
      adj->page_increment = MAX (adj->page_size / 2.0, adj->step_increment);
      adj->value          = CLAMP (adj->value,
                                   adj->lower,
                                   adj->upper - adj->page_size);
      gtk_adjustment_changed (adj);

      adj = gtk_range_get_adjustment (GTK_RANGE (scrolled_preview->vscr));
      adj->lower          = 0;
      adj->page_size      = GIMP_PREVIEW (preview)->height;
      adj->upper          = GIMP_PREVIEW (preview)->height * zoom_factor;
      adj->step_increment = 1.0;
      adj->page_increment = MAX (adj->page_size / 2.0, adj->step_increment);
      adj->value          = CLAMP (adj->value,
                                   adj->lower,
                                   adj->upper - adj->page_size);
      gtk_adjustment_changed (adj);

      gtk_widget_show (scrolled_preview->vscr);
      gtk_widget_show (scrolled_preview->hscr);
      gtk_widget_show (scrolled_preview->nav_icon);
    }

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
gimp_zoom_preview_size_allocate (GtkWidget       *widget,
                                 GtkAllocation   *allocation,
                                 GimpZoomPreview *preview)
{
  gint       width  = GIMP_PREVIEW (preview)->xmax - GIMP_PREVIEW (preview)->xmin;
  gint       height = GIMP_PREVIEW (preview)->ymax - GIMP_PREVIEW (preview)->ymin;

  GIMP_PREVIEW (preview)->width  = MIN (width,  allocation->width);
  GIMP_PREVIEW (preview)->height = MIN (height, allocation->height);

  gimp_zoom_preview_set_adjustments (preview);
}

static void
gimp_zoom_preview_style_set (GtkWidget *widget,
                             GtkStyle  *prev_style)
{
  GimpPreview  *preview  = GIMP_PREVIEW (widget);
  GimpDrawable *drawable = GIMP_ZOOM_PREVIEW (preview)->drawable;
  gint          size;
  gint          width, height;

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
gimp_zoom_preview_get_data (GimpZoomPreview *preview,
                            gint            *width,
                            gint            *height,
                            gint            *bpp)
{
  guchar       *data;
  GimpDrawable *drawable = preview->drawable;
  gint          src_x;
  gint          src_y;
  gint          src_width;
  gint          src_height;
  gdouble       zoom_factor = gimp_zoom_model_get_factor (preview->zoom);
  GimpPreview  *gimp_preview = GIMP_PREVIEW (preview);

  *width  = gimp_preview->width;
  *height = gimp_preview->height;

  src_x = preview->extents.x +
           gimp_preview->xoff * preview->extents.width / *width / zoom_factor;
  src_y = preview->extents.y +
           gimp_preview->yoff * preview->extents.height / *height / zoom_factor;
  src_width  = preview->extents.width / zoom_factor;
  src_height = preview->extents.height / zoom_factor;

  data = gimp_drawable_get_sub_thumbnail_data (drawable->drawable_id,
                                               src_x, src_y,
                                               src_width, src_height,
                                               width, height, bpp);
  return data;
}

static void
gimp_zoom_preview_draw (GimpPreview *gimp_preview)
{
  guchar          *data;
  gint             width;
  gint             height;
  gint             bpp;
  gint             src_x;
  gint             src_y;
  gint             src_width;
  gint             src_height;
  GimpZoomPreview *preview;
  GimpDrawable    *drawable;
  gdouble          zoom_factor;

  g_return_if_fail (GIMP_IS_ZOOM_PREVIEW (gimp_preview));

  preview = GIMP_ZOOM_PREVIEW (gimp_preview);
  drawable = preview->drawable;
  if (!drawable)
    return;
  zoom_factor = gimp_zoom_model_get_factor (preview->zoom);

  width  = gimp_preview->width;
  height = gimp_preview->height;
  src_x = preview->extents.x +
           gimp_preview->xoff * preview->extents.width / width / zoom_factor;
  src_y = preview->extents.y +
           gimp_preview->yoff * preview->extents.height / height / zoom_factor;
  src_width  = preview->extents.width / zoom_factor;
  src_height = preview->extents.height / zoom_factor;

  data = gimp_drawable_get_sub_thumbnail_data (drawable->drawable_id,
                                               src_x, src_y,
                                               src_width, src_height,
                                               &width, &height, &bpp);
  gimp_preview_area_draw (GIMP_PREVIEW_AREA (gimp_preview->area),
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
  gint32        image_id;
  GimpDrawable *drawable = GIMP_ZOOM_PREVIEW (preview)->drawable;

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
  GimpZoomPreview *zoom_preview = GIMP_ZOOM_PREVIEW (preview);
  GimpDrawable    *drawable       = zoom_preview->drawable;
  guchar          *buffer;
  gint             x1, y1, x2, y2;
  gint             bpp;

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
  GimpZoomPreview *preview;
  gint             width, height;
  gint             max_width, max_height;

  preview = g_object_new (GIMP_TYPE_ZOOM_PREVIEW, NULL);

  preview->drawable = drawable;
  width  = gimp_drawable_width  (drawable->drawable_id);
  height = gimp_drawable_height (drawable->drawable_id);

  preview->extents.x = 0;
  preview->extents.y = 0;
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

  g_object_set (GIMP_PREVIEW (preview)->frame,
                "ratio", (gdouble) width / (gdouble) height,
                NULL);

  gimp_zoom_preview_set_adjustments (preview);

  return GTK_WIDGET (preview);
}
