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


static GimpPreviewClass *parent_class = NULL;


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

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
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

  widget_class->style_set   = gimp_drawable_preview_style_set;

  preview_class->draw       = gimp_drawable_preview_draw_original;
  preview_class->draw_thumb = gimp_drawable_preview_draw_thumb;
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
gimp_drawable_preview_set_drawable (GimpDrawablePreview *drawable_preview,
                                    GimpDrawable        *drawable)
{
  GimpPreview *preview = GIMP_PREVIEW (drawable_preview);
  gint         width   = gimp_drawable_width (drawable->drawable_id);
  gint         height  = gimp_drawable_height (drawable->drawable_id);
  gint         x1, x2;
  gint         y1, y2;

  drawable_preview->drawable = drawable;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  gimp_preview_set_bounds (preview,
                           MAX (x1 - SELECTION_BORDER, 0),
                           MAX (y1 - SELECTION_BORDER, 0),
                           MIN (x2 + SELECTION_BORDER, width),
                           MIN (y2 + SELECTION_BORDER, height));

  if (gimp_drawable_is_indexed (drawable->drawable_id))
    {
      guint32  image = gimp_drawable_get_image (drawable->drawable_id);
      guchar  *cmap;
      gint     num_colors;

      cmap = gimp_image_get_cmap (image, &num_colors);
      gimp_preview_area_set_cmap (GIMP_PREVIEW_AREA (preview->area),
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
 * gimp_drawable_preview_draw:
 * @preview: a #GimpDrawablePreview widget
 * @buf:
 *
 * Since: GIMP 2.2
 **/
void
gimp_drawable_preview_draw (GimpDrawablePreview *preview,
                            const guchar        *buf)
{
  GimpPreview  *gimp_preview;
  GimpDrawable *drawable;
  gint32        image_id;
  gint          width, height;
  gint          bytes;

  g_return_if_fail (GIMP_IS_DRAWABLE_PREVIEW (preview));
  g_return_if_fail (preview->drawable != NULL);
  g_return_if_fail (buf != NULL);

  gimp_preview = GIMP_PREVIEW (preview);
  drawable     = preview->drawable;

  width  = gimp_preview->width;
  height = gimp_preview->height;
  bytes  = drawable->bpp;

  image_id = gimp_drawable_get_image (drawable->drawable_id);

  if (gimp_selection_is_empty (image_id))
    {
      gimp_preview_area_draw (GIMP_PREVIEW_AREA (gimp_preview->area),
                              0, 0, width, height,
                              gimp_drawable_type (drawable->drawable_id),
                              buf,
                              width * bytes);
    }
  else
    {
      GimpPixelRgn  selection_rgn;
      GimpPixelRgn  drawable_rgn;
      guchar       *sel;
      guchar       *src;
      gint          selection_id;
      gint          x1, y1;

      selection_id = gimp_image_get_selection (image_id);

      x1 = gimp_preview->xoff + gimp_preview->xmin;
      y1 = gimp_preview->yoff + gimp_preview->ymin;

      gimp_pixel_rgn_init (&drawable_rgn, drawable,
                           x1, y1, width, height,
                           FALSE, FALSE);
      gimp_pixel_rgn_init (&selection_rgn, gimp_drawable_get (selection_id),
                           x1, y1, width, height,
                           FALSE, FALSE);

      sel = g_new (guchar, width * height);
      src = g_new (guchar, width * height * bytes);

      gimp_pixel_rgn_get_rect (&drawable_rgn, src,  x1, y1, width, height);
      gimp_pixel_rgn_get_rect (&selection_rgn, sel, x1, y1, width, height);

      gimp_preview_area_mask (GIMP_PREVIEW_AREA (gimp_preview->area),
                              0, 0, width, height,
                              gimp_drawable_type (drawable->drawable_id),
                              src, width * bytes,
                              buf, width * bytes,
                              sel, width);

      g_free (sel);
      g_free (src);
    }
}
