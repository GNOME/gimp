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


#define PREVIEW_SIZE (128)

static void   gimp_drawable_preview_class_init (GimpDrawablePreviewClass *klass);
static void   gimp_drawable_preview_update     (GimpPreview  *preview);


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
  GimpPreviewClass *preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_class->update = gimp_drawable_preview_update;
}

static void
gimp_drawable_preview_update (GimpPreview *preview)
{
  GimpDrawablePreview *drawable_preview = GIMP_DRAWABLE_PREVIEW (preview);
  GimpDrawable        *drawable         = drawable_preview->drawable;
  guchar              *buffer;
  GimpPixelRgn         srcPR;
  guint                rowstride;

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


/**
 * gimp_drawable_preview_new:
 * @drawable: a #GimpDrawable
 *
 * Creates a new #GimpDrawablePreview widget for @drawable.
 *
 * Returns: A pointer to the new #GimpDrawablePreview widget.
 *
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_drawable_preview_new (GimpDrawable *drawable)
{
  GimpDrawablePreview *drawable_preview;
  GimpPreview         *preview;
  gint                 sel_width, sel_height;

  g_return_val_if_fail (drawable != NULL, NULL);

  drawable_preview = g_object_new (GIMP_TYPE_DRAWABLE_PREVIEW, NULL);
  drawable_preview->drawable = drawable;

  preview = GIMP_PREVIEW (drawable_preview);

  gimp_drawable_mask_bounds (drawable->drawable_id,
                             &preview->xmin, &preview->ymin,
                             &preview->xmax, &preview->ymax);

  sel_width       = preview->xmax - preview->xmin;
  sel_height      = preview->ymax - preview->ymin;
  preview->width  = MIN (sel_width,  PREVIEW_SIZE);
  preview->height = MIN (sel_height, PREVIEW_SIZE);

  gtk_range_set_increments (GTK_RANGE (preview->hscr),
                            1.0,
                            MIN (preview->width, sel_width));
  gtk_range_set_range (GTK_RANGE (preview->hscr),
                       0, sel_width-1);
  gtk_range_set_increments (GTK_RANGE (preview->vscr),
                            1.0,
                            MIN (preview->height, sel_height));
  gtk_range_set_range (GTK_RANGE (preview->vscr),
                       0, sel_height-1);

  gtk_widget_set_size_request (preview->area,
                               preview->width, preview->height);

  return GTK_WIDGET (preview);
}

/**
 * gimp_drawable_preview_new_with_toggle:
 * @drawable: a #GimpDrawable
 * @toggle:
 *
 * Creates a new #GimpDrawablePreview widget for @drawable.
 *
 * Returns: A pointer to the new #GimpDrawablePreview widget.
 *
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_drawable_preview_new_with_toggle (GimpDrawable *drawable,
                                       gboolean     *toggle)
{
  GimpDrawablePreview *drawable_preview;
  GimpPreview         *preview;
  gint                 sel_width, sel_height;

  g_return_val_if_fail (drawable != NULL, NULL);
  g_return_val_if_fail (toggle != NULL, NULL);

  drawable_preview = g_object_new (GIMP_TYPE_DRAWABLE_PREVIEW,
                                   "show_toggle_preview", TRUE,
                                   "update_preview",      *toggle,
                                   NULL);

  drawable_preview->drawable = drawable;
  preview = GIMP_PREVIEW (drawable_preview);

  gimp_drawable_mask_bounds (drawable->drawable_id,
                             &preview->xmin, &preview->ymin,
                             &preview->xmax, &preview->ymax);

  sel_width       = preview->xmax - preview->xmin;
  sel_height      = preview->ymax - preview->ymin;
  preview->width  = MIN (sel_width,  PREVIEW_SIZE);
  preview->height = MIN (sel_height, PREVIEW_SIZE);

  gtk_range_set_increments (GTK_RANGE (preview->hscr),
                            1.0,
                            MIN (preview->width, sel_width));
  gtk_range_set_range (GTK_RANGE (preview->hscr),
                       0, sel_width-1);
  gtk_range_set_increments (GTK_RANGE (preview->vscr),
                            1.0,
                            MIN (preview->height, sel_height));
  gtk_range_set_range (GTK_RANGE (preview->vscr),
                       0, sel_height-1);

  gtk_widget_set_size_request (preview->area,
                               preview->width, preview->height);

  g_signal_connect (preview->toggle_update, "toggled",
                    G_CALLBACK (gimp_toggle_button_update), toggle);

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

