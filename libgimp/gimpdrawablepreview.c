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

static void  gimp_drawable_preview_style_set     (GtkWidget   *widget,
                                                  GtkStyle    *prev_style);
static void  gimp_drawable_preview_draw_original (GimpPreview *preview);


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
        NULL            /* instance_init  */
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

  widget_class->style_set = gimp_drawable_preview_style_set;

  preview_class->draw     = gimp_drawable_preview_draw_original;
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

  preview->xmin = MAX (x1 - SELECTION_BORDER, 0);
  preview->ymin = MAX (y1 - SELECTION_BORDER, 0);
  preview->xmax = MIN (x2 + SELECTION_BORDER, width);
  preview->ymax = MIN (y2 + SELECTION_BORDER, height);
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
                            guchar              *buf)
{
  GimpPreview  *gimp_preview;
  GimpDrawable *drawable;

  g_return_if_fail (GIMP_IS_DRAWABLE_PREVIEW (preview));
  g_return_if_fail (buf != NULL);

  gimp_preview = GIMP_PREVIEW (preview);
  drawable     = preview->drawable;

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (gimp_preview->area),
                          0, 0, gimp_preview->width, gimp_preview->height,
                          gimp_drawable_type (drawable->drawable_id),
                          buf,
                          gimp_preview->width * drawable->bpp);
}
