/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpDrawablePreview Widget
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

#include "gimpdrawablepreview.h"


static void   gimp_drawable_preview_class_init (GimpDrawablePreviewClass *klass);
static void   gimp_drawable_preview_init       (GimpDrawablePreview      *preview);

static TempBuf   * gimp_drawable_preview_create_preview (GimpPreview *preview);
static GtkWidget * gimp_drawable_preview_create_popup   (GimpPreview *preview);


static GimpPreviewClass *parent_class = NULL;


GtkType
gimp_drawable_preview_get_type (void)
{
  static GtkType preview_type = 0;

  if (! preview_type)
    {
      GtkTypeInfo preview_info =
      {
	"GimpDrawablePreview",
	sizeof (GimpDrawablePreview),
	sizeof (GimpDrawablePreviewClass),
	(GtkClassInitFunc) gimp_drawable_preview_class_init,
	(GtkObjectInitFunc) gimp_drawable_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      preview_type = gtk_type_unique (GIMP_TYPE_PREVIEW, &preview_info);
    }
  
  return preview_type;
}

static void
gimp_drawable_preview_class_init (GimpDrawablePreviewClass *klass)
{
  GtkObjectClass   *object_class;
  GimpPreviewClass *preview_class;

  object_class  = (GtkObjectClass *) klass;
  preview_class = (GimpPreviewClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PREVIEW);
}

static void
gimp_drawable_preview_init (GimpDrawablePreview *preview)
{
}

static TempBuf *
gimp_drawable_preview_create_preview (GimpPreview *preview)
{
}

static GtkWidget *
gimp_drawable_preview_create_popup (GimpPreview *preview)
{
}
