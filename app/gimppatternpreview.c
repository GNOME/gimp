/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPatternPreview Widget
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

#include "gimppattern.h"
#include "gimppatternpreview.h"
#include "temp_buf.h"


static void   gimp_pattern_preview_class_init (GimpPatternPreviewClass *klass);
static void   gimp_pattern_preview_init       (GimpPatternPreview      *preview);

static GtkWidget * gimp_pattern_preview_create_popup (GimpPreview *preview);
static gboolean    gimp_pattern_preview_needs_popup  (GimpPreview *preview);


static GimpPreviewClass *parent_class = NULL;


GtkType
gimp_pattern_preview_get_type (void)
{
  static GtkType preview_type = 0;

  if (! preview_type)
    {
      GtkTypeInfo preview_info =
      {
	"GimpPatternPreview",
	sizeof (GimpPatternPreview),
	sizeof (GimpPatternPreviewClass),
	(GtkClassInitFunc) gimp_pattern_preview_class_init,
	(GtkObjectInitFunc) gimp_pattern_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      preview_type = gtk_type_unique (GIMP_TYPE_PREVIEW, &preview_info);
    }
  
  return preview_type;
}

static void
gimp_pattern_preview_class_init (GimpPatternPreviewClass *klass)
{
  GtkObjectClass   *object_class;
  GimpPreviewClass *preview_class;

  object_class  = (GtkObjectClass *) klass;
  preview_class = (GimpPreviewClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PREVIEW);

  preview_class->create_popup = gimp_pattern_preview_create_popup;
  preview_class->needs_popup  = gimp_pattern_preview_needs_popup;
}

static void
gimp_pattern_preview_init (GimpPatternPreview *preview)
{
}

static GtkWidget *
gimp_pattern_preview_create_popup (GimpPreview *preview)
{
  gint popup_width;
  gint popup_height;

  popup_width  = GIMP_PATTERN (preview->viewable)->mask->width;
  popup_height = GIMP_PATTERN (preview->viewable)->mask->height;

  return gimp_preview_new (preview->viewable,
			   TRUE,
			   popup_width,
			   popup_height,
			   FALSE, FALSE);
}

static gboolean
gimp_pattern_preview_needs_popup (GimpPreview *preview)
{
  GimpPattern *pattern;
  gint         pattern_width;
  gint         pattern_height;
  gint         width;
  gint         height;

  pattern        = GIMP_PATTERN (preview->viewable);
  pattern_width  = pattern->mask->width;
  pattern_height = pattern->mask->height;

  width  = GTK_WIDGET (preview)->requisition.width;
  height = GTK_WIDGET (preview)->requisition.height;

  if (pattern_width > width || pattern_height > height)
    return TRUE;

  return FALSE;
}
