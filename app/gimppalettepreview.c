/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPalettePreview Widget
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

#include "gimppalette.h"
#include "gimppalettepreview.h"
#include "temp_buf.h"


static void   gimp_palette_preview_class_init (GimpPalettePreviewClass *klass);
static void   gimp_palette_preview_init       (GimpPalettePreview      *preview);

static void        gimp_palette_preview_get_size     (GimpPreview *preview,
                                                      gint         size,
                                                      gint        *width,
                                                      gint        *height);
static void        gimp_palette_preview_render       (GimpPreview *preview);
static GtkWidget * gimp_palette_preview_create_popup (GimpPreview *preview);
static gboolean    gimp_palette_preview_needs_popup  (GimpPreview *preview);


static GimpPreviewClass *parent_class = NULL;


GtkType
gimp_palette_preview_get_type (void)
{
  static GtkType preview_type = 0;

  if (! preview_type)
    {
      GtkTypeInfo preview_info =
      {
	"GimpPalettePreview",
	sizeof (GimpPalettePreview),
	sizeof (GimpPalettePreviewClass),
	(GtkClassInitFunc) gimp_palette_preview_class_init,
	(GtkObjectInitFunc) gimp_palette_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      preview_type = gtk_type_unique (GIMP_TYPE_PREVIEW, &preview_info);
    }
  
  return preview_type;
}

static void
gimp_palette_preview_class_init (GimpPalettePreviewClass *klass)
{
  GtkObjectClass   *object_class;
  GimpPreviewClass *preview_class;

  object_class  = (GtkObjectClass *) klass;
  preview_class = (GimpPreviewClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PREVIEW);

  preview_class->get_size     = gimp_palette_preview_get_size;
  preview_class->render       = gimp_palette_preview_render;
  preview_class->create_popup = gimp_palette_preview_create_popup;
  preview_class->needs_popup  = gimp_palette_preview_needs_popup;
}

static void
gimp_palette_preview_init (GimpPalettePreview *palette_preview)
{
}

static void
gimp_palette_preview_get_size (GimpPreview *preview,
			       gint         size,
			       gint        *width,
			       gint        *height)
{
  *width  = size * 3;
  *height = size;
}

static void
gimp_palette_preview_render (GimpPreview *preview)
{
  GimpPalette *palette;
  TempBuf     *temp_buf;
  gint         width;
  gint         height;

  palette = GIMP_PALETTE (preview->viewable);

  width  = preview->width;
  height = preview->height;

  temp_buf = gimp_viewable_get_new_preview (preview->viewable,
					    width, height);

  gimp_preview_render_and_flush (preview,
                                 temp_buf,
                                 -1);

  temp_buf_free (temp_buf);
}

static GtkWidget *
gimp_palette_preview_create_popup (GimpPreview *preview)
{
  GimpPalette *palette;
  gint         popup_width;
  gint         popup_height;

  palette = GIMP_PALETTE (preview->viewable);

  if (palette->n_columns)
    popup_width = palette->n_columns;
  else
    popup_width = MIN (palette->n_colors, 16);

  popup_height = MAX (1, palette->n_colors / popup_width);

  return gimp_preview_new_full (preview->viewable,
				popup_width  * 4,
				popup_height * 4,
				0,
				TRUE, FALSE, FALSE);
}

static gboolean
gimp_palette_preview_needs_popup (GimpPreview *preview)
{
  GimpPalette *palette;
  gint         popup_width;
  gint         popup_height;

  palette = GIMP_PALETTE (preview->viewable);

  if (! palette->n_colors)
    return FALSE;

  if (palette->n_columns)
    popup_width = palette->n_columns;
  else
    popup_width = MIN (palette->n_colors, 16);

  popup_height = MAX (1, palette->n_colors / popup_width);

  if (popup_width  * 4 > preview->width ||
      popup_height * 4 > preview->height)
    return TRUE;

  return FALSE;
}
