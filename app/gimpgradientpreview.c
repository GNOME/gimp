/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGradientPreview Widget
 * Copyright (C) 2001 Michael Natterer
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

#include "apptypes.h"

#include "gimpdnd.h"
#include "gimpgradient.h"
#include "gimpgradientpreview.h"
#include "temp_buf.h"


static void   gimp_gradient_preview_class_init (GimpGradientPreviewClass *klass);
static void   gimp_gradient_preview_init       (GimpGradientPreview      *preview);

static void          gimp_gradient_preview_render       (GimpPreview *preview);
static void          gimp_gradient_preview_get_size     (GimpPreview *preview,
							 gint         size,
							 gint        *width,
							 gint        *height);
static gboolean      gimp_gradient_preview_needs_popup  (GimpPreview *preview);
static GtkWidget   * gimp_gradient_preview_create_popup (GimpPreview *preview);

static GimpGradient * gimp_gradient_preview_drag_gradient (GtkWidget   *widget,
							   gpointer     data);


static GimpPreviewClass *parent_class = NULL;


GtkType
gimp_gradient_preview_get_type (void)
{
  static GtkType preview_type = 0;

  if (! preview_type)
    {
      GtkTypeInfo preview_info =
      {
	"GimpGradientPreview",
	sizeof (GimpGradientPreview),
	sizeof (GimpGradientPreviewClass),
	(GtkClassInitFunc) gimp_gradient_preview_class_init,
	(GtkObjectInitFunc) gimp_gradient_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      preview_type = gtk_type_unique (GIMP_TYPE_PREVIEW, &preview_info);
    }
  
  return preview_type;
}

static void
gimp_gradient_preview_class_init (GimpGradientPreviewClass *klass)
{
  GtkObjectClass   *object_class;
  GimpPreviewClass *preview_class;

  object_class  = (GtkObjectClass *) klass;
  preview_class = (GimpPreviewClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PREVIEW);

  preview_class->get_size     = gimp_gradient_preview_get_size;
  preview_class->render       = gimp_gradient_preview_render;
  preview_class->needs_popup  = gimp_gradient_preview_needs_popup;
  preview_class->create_popup = gimp_gradient_preview_create_popup;
}

static void
gimp_gradient_preview_init (GimpGradientPreview *gradient_preview)
{
  static GtkTargetEntry preview_target_table[] =
  {
    GIMP_TARGET_GRADIENT
  };
  static guint preview_n_targets = (sizeof (preview_target_table) /
				    sizeof (preview_target_table[0]));

  gtk_drag_source_set (GTK_WIDGET (gradient_preview),
                       GDK_BUTTON2_MASK,
                       preview_target_table, preview_n_targets,
                       GDK_ACTION_COPY);
  gimp_dnd_gradient_source_set (GTK_WIDGET (gradient_preview),
				gimp_gradient_preview_drag_gradient,
				gradient_preview);
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

static GimpGradient *
gimp_gradient_preview_drag_gradient (GtkWidget *widget,
				     gpointer   data)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (data);

  return GIMP_GRADIENT (preview->viewable);
}
