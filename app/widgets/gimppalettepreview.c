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

#include "gimpdnd.h"
#include "gimppalette.h"
#include "gimppalettepreview.h"
#include "temp_buf.h"


static void   gimp_palette_preview_class_init (GimpPalettePreviewClass *klass);
static void   gimp_palette_preview_init       (GimpPalettePreview      *preview);

static void          gimp_palette_preview_render       (GimpPreview *preview);
static GtkWidget   * gimp_palette_preview_create_popup (GimpPreview *preview);
static gboolean      gimp_palette_preview_needs_popup  (GimpPreview *preview);

static GimpPalette * gimp_palette_preview_drag_palette (GtkWidget   *widget,
							gpointer     data);


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

  preview_class->render       = gimp_palette_preview_render;
  preview_class->create_popup = gimp_palette_preview_create_popup;
  preview_class->needs_popup  = gimp_palette_preview_needs_popup;
}

static void
gimp_palette_preview_init (GimpPalettePreview *palette_preview)
{
  static GtkTargetEntry preview_target_table[] =
  {
    GIMP_TARGET_PALETTE
  };
  static guint preview_n_targets = (sizeof (preview_target_table) /
				    sizeof (preview_target_table[0]));

  gtk_drag_source_set (GTK_WIDGET (palette_preview),
                       GDK_BUTTON2_MASK,
                       preview_target_table, preview_n_targets,
                       GDK_ACTION_COPY);
  gimp_dnd_palette_source_set (GTK_WIDGET (palette_preview),
			       gimp_palette_preview_drag_palette,
			       palette_preview);
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

  popup_width  = MIN (palette->n_colors, 16);
  popup_height = MAX (1, palette->n_colors / popup_width);

  return gimp_preview_new_full (preview->viewable,
				popup_width  * 3,
				popup_height * 3,
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

  popup_width  = MIN (palette->n_colors, 16);
  popup_height = MAX (1, palette->n_colors / popup_width);

  if (popup_width  * 3 > preview->width ||
      popup_height * 3 > preview->height)
    return TRUE;

  return FALSE;
}

static GimpPalette *
gimp_palette_preview_drag_palette (GtkWidget *widget,
				   gpointer   data)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (data);

  return GIMP_PALETTE (preview->viewable);
}
