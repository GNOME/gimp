/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGradientPreview Widget
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpgradient.h"

#include "gimpgradientpreview.h"


static void   gimp_gradient_preview_class_init (GimpGradientPreviewClass *klass);
static void   gimp_gradient_preview_init       (GimpGradientPreview      *preview);

static void        gimp_gradient_preview_render       (GimpPreview *preview);
static void        gimp_gradient_preview_get_size     (GimpPreview *preview,
                                                       gint         size,
                                                       gint        *width,
                                                       gint        *height);
static gboolean    gimp_gradient_preview_needs_popup  (GimpPreview *preview);
static GtkWidget * gimp_gradient_preview_create_popup (GimpPreview *preview);


static GimpPreviewClass *parent_class = NULL;


GType
gimp_gradient_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpGradientPreviewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_gradient_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpGradientPreview),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_gradient_preview_init,
      };

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
                                             "GimpGradientPreview",
                                             &preview_info, 0);
    }
  
  return preview_type;
}

static void
gimp_gradient_preview_class_init (GimpGradientPreviewClass *klass)
{
  GimpPreviewClass *preview_class;

  preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_class->get_size     = gimp_gradient_preview_get_size;
  preview_class->render       = gimp_gradient_preview_render;
  preview_class->needs_popup  = gimp_gradient_preview_needs_popup;
  preview_class->create_popup = gimp_gradient_preview_create_popup;
}

static void
gimp_gradient_preview_init (GimpGradientPreview *gradient_preview)
{
}

static void
gimp_gradient_preview_get_size (GimpPreview *preview,
				gint         size,
				gint        *width,
				gint        *height)
{
  *width  = size * 3;
  *height = size;
}

static void
gimp_gradient_preview_render (GimpPreview *preview)
{
  GimpGradient *gradient;
  TempBuf      *temp_buf;
  gint          width;
  gint          height;

  gradient = GIMP_GRADIENT (preview->viewable);

  width  = preview->width;
  height = preview->height;

  temp_buf = gimp_viewable_get_new_preview (preview->viewable,
					    width, height);

  gimp_preview_render_and_flush (preview,
                                 temp_buf,
                                 -1);

  temp_buf_free (temp_buf);
}

static gboolean
gimp_gradient_preview_needs_popup (GimpPreview *preview)
{
  GimpGradient *gradient;
  gint          popup_width;
  gint          popup_height;

  gradient = GIMP_GRADIENT (preview->viewable);

  popup_width  = 48;
  popup_height = 24;

  if (popup_width  > preview->width ||
      popup_height > preview->height)
    return TRUE;

  return FALSE;
}

static GtkWidget *
gimp_gradient_preview_create_popup (GimpPreview *preview)
{
  GimpGradient *gradient;
  gint          popup_width;
  gint          popup_height;

  gradient = GIMP_GRADIENT (preview->viewable);

  popup_width  = 128;
  popup_height =  32;

  return gimp_preview_new_full (preview->viewable,
				popup_width,
				popup_height,
				0,
				TRUE, FALSE, FALSE);
}
