/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimppalette.h"

#include "widgets/gimppaletteeditor.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimptoolbox-color-area.h"

#include "color-notebook.h"
#include "palette-editor-commands.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   palette_editor_color_notebook_callback (ColorNotebook      *color_notebook,
                                                      const GimpRGB      *color,
                                                      ColorNotebookState  state,
                                                      gpointer            data);


/*  public functions  */

void
palette_editor_new_color_cmd_callback (GtkWidget *widget,
                                       gpointer   data,
                                       guint      action)
{
  GimpPaletteEditor *editor;
  GimpPalette       *palette;
  GimpContext       *user_context;
  GimpRGB            color;

  editor = GIMP_PALETTE_EDITOR (data);

  palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

  if (! palette)
    return;

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->gimp);

  if (active_color == FOREGROUND)
    gimp_context_get_foreground (user_context, &color);
  else if (active_color == BACKGROUND)
    gimp_context_get_background (user_context, &color);

  editor->color = gimp_palette_add_entry (palette, NULL, &color);
}

void
palette_editor_edit_color_cmd_callback (GtkWidget *widget,
                                        gpointer   data,
                                        guint      action)
{
  GimpPaletteEditor *editor;
  GimpPalette       *palette;

  editor = GIMP_PALETTE_EDITOR (data);

  palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

  if (! (palette && editor->color))
    return;

  if (! editor->color_notebook)
    {
      editor->color_notebook =
	color_notebook_viewable_new (GIMP_VIEWABLE (palette),
                                     _("Edit Palette Color"),
                                     GTK_STOCK_SELECT_COLOR,
                                     _("Edit Color Palette Entry"),
                                     (const GimpRGB *) &editor->color->color,
                                     palette_editor_color_notebook_callback,
                                     editor,
                                     FALSE, FALSE);
      editor->color_notebook_active = TRUE;
    }
  else
    {
      if (! editor->color_notebook_active)
	{
          color_notebook_set_viewable (editor->color_notebook,
                                       GIMP_VIEWABLE (palette));
	  color_notebook_show (editor->color_notebook);
	  editor->color_notebook_active = TRUE;
	}

      color_notebook_set_color (editor->color_notebook,
				&editor->color->color);
    }
}

void
palette_editor_delete_color_cmd_callback (GtkWidget *widget,
                                          gpointer   data,
                                          guint      action)
{
  GimpPaletteEditor *editor;
  GimpPalette       *palette;

  editor = GIMP_PALETTE_EDITOR (data);

  palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);


  if (! (palette && editor->color))
    return;

  gimp_palette_delete_entry (palette, editor->color);
}

void
palette_editor_menu_update (GtkItemFactory *factory,
                            gpointer        data)
{
  GimpPaletteEditor *editor;

  editor = GIMP_PALETTE_EDITOR (data);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/New Color",     TRUE);
  SET_SENSITIVE ("/Edit Color...", editor->color);
  SET_SENSITIVE ("/Delete Color",  editor->color);

#undef SET_SENSITIVE
}


/*  private functions  */

static void
palette_editor_color_notebook_callback (ColorNotebook      *color_notebook,
					const GimpRGB      *color,
					ColorNotebookState  state,
					gpointer            data)
{
  GimpPaletteEditor *editor;
  GimpPalette       *palette;
  GimpContext       *user_context;

  editor = GIMP_PALETTE_EDITOR (data);

  palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->gimp);

  switch (state)
    {
    case COLOR_NOTEBOOK_UPDATE:
      break;

    case COLOR_NOTEBOOK_OK:
      if (editor->color)
	{
	  editor->color->color = *color;

	  /*  Update either foreground or background colors  */
	  if (active_color == FOREGROUND)
	    gimp_context_set_foreground (user_context, color);
	  else if (active_color == BACKGROUND)
	    gimp_context_set_background (user_context, color);

	  gimp_data_dirty (GIMP_DATA (palette));
	}

      /* Fallthrough */
    case COLOR_NOTEBOOK_CANCEL:
      if (editor->color_notebook_active)
	{
	  color_notebook_hide (editor->color_notebook);
	  editor->color_notebook_active = FALSE;
	}
    }
}
