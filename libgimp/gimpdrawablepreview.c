/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawablepreview.c
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


#define SELECTION_BORDER  2


static void  gimp_drawable_preview_class_init    (GimpDrawablePreviewClass *klass);
static void  gimp_drawable_preview_init          (GimpDrawablePreview *preview);

static void  gimp_drawable_preview_style_set     (GtkWidget           *widget,
                                                  GtkStyle            *prev_style);

static void  gimp_drawable_preview_draw_original (GimpPreview         *preview);
static void  gimp_drawable_preview_draw_thumb    (GimpPreview         *preview,
                                                  GimpPreviewArea     *area,
                                                  gint                 width,
                                                  gint                 height);
static void  gimp_drawable_preview_draw_buffer   (GimpPreview         *preview,
                                                  const guchar        *buffer,
                                                  gint                 rowstride);


static GimpScrolledPreviewClass *parent_class = NULL;


GType
gimp_drawable_preview_get_type (void)
{
  static GType preview_type = 0;

  if (!preview_type)
    {
      static const GTypeInfo drawable_preview_info =
      {
        sizeof (GimpDrawablePreviewClass),
        (GBaseInitFunc)     NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_drawable_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpDrawablePreview),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_drawable_preview_init
      };

      preview_type = g_type_register_static (GIMP_TYPE_SCROLLED_PREVIEW,
                                             "GimpDrawablePreview",
                                             &drawable_preview_info, 0);
    }

  return preview_type;
}

static void
gimp_drawable_preview_class_init (GimpDrawablePreviewClass *klass)
{
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  GimpPreviewClass *preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->style_set    = gimp_drawable_preview_style_set;

  preview_class->draw        = gimp_drawable_preview_draw_original;
  preview_class->draw_thumb  = gimp_drawable_preview_draw_thumb;
  preview_class->draw_buffer = gimp_drawable_preview_draw_buffer;
}

static void
gimp_drawable_preview_init (GimpDrawablePreview *preview)
{
  g_object_set (GIMP_PREVIEW (preview)->area,
                "check-size", gimp_check_size (),
                "check-type", gimp_check_type (),
                NULL);
}

static void
gimp_drawable_preview_style_set (GtkWidget *widget,
                                 GtkStyle  *prev_style)
{
  GimpPreview *preview = GIMP_PREVIEW (widget);
  gint         width   = preview->xmax - preview->xmin;
  gint         height  = preview->ymax - preview->ymin;
  gint         size;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "size", &size,
                        NULL);

  gtk_widget_set_size_request (GIMP_PREVIEW (preview)->area,
                               MIN (width, size), MIN (height, size));
}

static void
gimp_drawable_preview_draw_original (GimpPreview *preview)
{
  GimpDrawablePreview *drawable_preview = GIMP_DRAWABLE_PREVIEW (preview);
  GimpDrawable        *drawable         = drawable_preview->drawable;
  guchar              *buffer;
  GimpPixelRgn         srcPR;
  guint                rowstride;

  if (! drawable)
    return;

  rowstride = preview->width * drawable->bpp;
  buffer    = g_new (guchar, rowstride * preview->height);

  preview->xoff = CLAMP (preview->xoff,
                         0, preview->xmax - preview->xmin - preview->width);
  preview->yoff = CLAMP (preview->yoff,
                         0, preview->ymax - preview->ymin - preview->height);

  gimp_pixel_rgn_init (&srcPR, drawable,
                       preview->xoff + preview->xmin,
                       preview->yoff + preview->ymin,
                       preview->width, preview->height,
                       FALSE, FALSE);

  gimp_pixel_rgn_get_rect (&srcPR, buffer,
                           preview->xoff + preview->xmin,
                           preview->yoff + preview->ymin,
                           preview->width, preview->height);

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview->area),
                          0, 0, preview->width, preview->height,
                          gimp_drawable_type (drawable->drawable_id),
                          buffer,
                          rowstride);
  g_free (buffer);
}

static void
gimp_drawable_preview_draw_thumb (GimpPreview     *preview,
                                  GimpPreviewArea *area,
                                  gint             width,
                                  gint             height)
{
  GimpDrawablePreview *drawable_preview = GIMP_DRAWABLE_PREVIEW (preview);
  GimpDrawable        *drawable         = drawable_preview->drawable;
  guchar              *buffer;
  gint                 bpp;

  if (! drawable)
    return;

  buffer = gimp_drawable_get_thumbnail_data (drawable->drawable_id,
                                             &width, &height, &bpp);

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

static void
gimp_drawable_preview_draw_area (GimpDrawablePreview *preview,
                                 gint                 x,
                                 gint                 y,
                                 gint                 width,
                                 gint                 height,
                                 const guchar        *buf,
                                 gint                 rowstride)
{
  GimpPreview  *gimp_preview = GIMP_PREVIEW (preview);
  GimpDrawable *drawable     = preview->drawable;
  gint32        image_id;

  image_id = gimp_drawable_get_image (drawable->drawable_id);

  if (gimp_selection_is_empty (image_id))
    {
      gimp_preview_area_draw (GIMP_PREVIEW_AREA (gimp_preview->area),
                              x - gimp_preview->xoff - gimp_preview->xmin,
                              y - gimp_preview->yoff - gimp_preview->ymin,
                              width,
                              height,
                              gimp_drawable_type (drawable->drawable_id),
                              buf, rowstride);
    }
  else
    {
      GimpPixelRgn  drawable_rgn;
      GimpPixelRgn  selection_rgn;
      guchar       *src;
      guchar       *sel;
      gint          offset_x;
      gint          offset_y;
      gint          selection_id;

      selection_id = gimp_image_get_selection (image_id);

      gimp_drawable_offsets (drawable->drawable_id, &offset_x, &offset_y);

      gimp_pixel_rgn_init (&drawable_rgn, drawable,
                           x, y, width, height,
                           FALSE, FALSE);
      gimp_pixel_rgn_init (&selection_rgn, gimp_drawable_get (selection_id),
                           x + offset_x, y + offset_y, width, height,
                           FALSE, FALSE);

      src = g_new (guchar, width * height * drawable->bpp);
      sel = g_new (guchar, width * height);

      gimp_pixel_rgn_get_rect (&drawable_rgn,
                               src, x, y, width, height);
      gimp_pixel_rgn_get_rect (&selection_rgn,
                               sel, x + offset_x, y + offset_y, width, height);

      gimp_preview_area_mask (GIMP_PREVIEW_AREA (gimp_preview->area),
                              x - gimp_preview->xoff - gimp_preview->xmin,
                              y - gimp_preview->yoff - gimp_preview->ymin,
                              width,
                              height,
                              gimp_drawable_type (drawable->drawable_id),
                              src, width * drawable->bpp,
                              buf, rowstride,
                              sel, width);

      g_free (sel);
      g_free (src);
    }
}

static void
gimp_drawable_preview_draw_buffer (GimpPreview  *preview,
                                   const guchar *buffer,
                                   gint          rowstride)
{
  gimp_drawable_preview_draw_area (GIMP_DRAWABLE_PREVIEW (preview),
                                   preview->xmin + preview->xoff,
                                   preview->ymin + preview->yoff,
                                   preview->width,
                                   preview->height,
                                   buffer, rowstride);
}

#define MAX3(a, b, c)  (MAX (MAX ((a), (b)), (c)))
#define MIN3(a, b, c)  (MIN (MIN ((a), (b)), (c)))

static void
gimp_drawable_preview_set_drawable (GimpDrawablePreview *drawable_preview,
                                    GimpDrawable        *drawable)
{
  GimpPreview *preview = GIMP_PREVIEW (drawable_preview);
  gint         width   = gimp_drawable_width (drawable->drawable_id);
  gint         height  = gimp_drawable_height (drawable->drawable_id);
  gint         x1, x2;
  gint         y1, y2;
  gint         xmin, ymin, xmax, ymax;
  gint         offset_x, offset_y;

  drawable_preview->drawable = drawable;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
  gimp_drawable_offsets (drawable->drawable_id, &offset_x, &offset_y);

  xmin = MAX3 (x1 - SELECTION_BORDER, 0, - offset_x);
  ymin = MAX3 (y1 - SELECTION_BORDER, 0, - offset_y);
  xmax = MIN3 (x2 + SELECTION_BORDER, width, width + offset_x);
  ymax = MIN3 (y2 + SELECTION_BORDER, height, height + offset_y);

  gimp_preview_set_bounds (preview, xmin, ymin, xmax, ymax);

  if (gimp_drawable_is_indexed (drawable->drawable_id))
    {
      guint32  image = gimp_drawable_get_image (drawable->drawable_id);
      guchar  *cmap;
      gint     num_colors;

      cmap = gimp_image_get_colormap (image, &num_colors);
      gimp_preview_area_set_colormap (GIMP_PREVIEW_AREA (preview->area),
                                      cmap, num_colors);
      g_free (cmap);
    }
}

static void
gimp_drawable_preview_notify_update (GimpPreview *preview,
                                     GParamSpec  *pspec,
                                     gboolean    *toggle)
{
  *toggle = gimp_preview_get_update (preview);
}


/**
 * gimp_drawable_preview_new:
 * @drawable: a #GimpDrawable
 * @toggle:   pointer to a #gboolean variable to sync with the "Preview"
 *            check-button or %NULL
 *
 * Creates a new #GimpDrawablePreview widget for @drawable. If
 * updating the preview takes considerable time, you will want to
 * store the state of the "Preview" check-button in the plug-in
 * data. For convenience you can pass a pointer to the #gboolean as
 * @toggle.
 *
 * Returns: A pointer to the new #GimpDrawablePreview widget.
 *
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_drawable_preview_new (GimpDrawable *drawable,
                           gboolean     *toggle)
{
  GimpDrawablePreview *preview;

  g_return_val_if_fail (drawable != NULL, NULL);

  preview = g_object_new (GIMP_TYPE_DRAWABLE_PREVIEW, NULL);

  if (toggle)
    {
      gimp_preview_set_update (GIMP_PREVIEW (preview), *toggle);

      g_signal_connect (preview, "notify::update",
                        G_CALLBACK (gimp_drawable_preview_notify_update),
                        toggle);
    }

  gimp_drawable_preview_set_drawable (preview, drawable);

  return GTK_WIDGET (preview);
}

/**
 * gimp_drawable_preview_get_drawable:
 * @preview:   a #GimpDrawablePreview widget
 *
 * Return value: the #GimpDrawable that has been passed to
 *               gimp_drawable_preview_new().
 *
 * Since: GIMP 2.2
 **/
GimpDrawable *
gimp_drawable_preview_get_drawable (GimpDrawablePreview *preview)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_PREVIEW (preview), NULL);

  return preview->drawable;
}

/**
 * gimp_drawable_preview_draw_region:
 * @preview: a #GimpDrawablePreview widget
 * @region:  a #GimpPixelRgn
 *
 * Since: GIMP 2.2
 **/
void
gimp_drawable_preview_draw_region (GimpDrawablePreview *preview,
                                   const GimpPixelRgn  *region)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_PREVIEW (preview));
  g_return_if_fail (preview->drawable != NULL);
  g_return_if_fail (region != NULL);

  /*  If the data field is initialized, this region is currently being
   *  processed and we can access it directly.
   */
  if (region->data)
    {
      gimp_drawable_preview_draw_area (preview,
                                       region->x,
                                       region->y,
                                       region->w,
                                       region->h,
                                       region->data,
                                       region->rowstride);
    }
  else
    {
      GimpPixelRgn  src = *region;
      gpointer      iter;

      src.dirty = FALSE; /* we don't dirty the tiles, just read them */

      for (iter = gimp_pixel_rgns_register (1, &src);
           iter != NULL;
           iter = gimp_pixel_rgns_process (iter))
        {
          gimp_drawable_preview_draw_area (preview,
                                           src.x,
                                           src.y,
                                           src.w,
                                           src.h,
                                           src.data,
                                           src.rowstride);
        }
    }
}
