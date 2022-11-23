/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_PALETTE_EDITOR_H__
#define __LIGMA_PALETTE_EDITOR_H__


#include "ligmadataeditor.h"


#define LIGMA_TYPE_PALETTE_EDITOR            (ligma_palette_editor_get_type ())
#define LIGMA_PALETTE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PALETTE_EDITOR, LigmaPaletteEditor))
#define LIGMA_PALETTE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PALETTE_EDITOR, LigmaPaletteEditorClass))
#define LIGMA_IS_PALETTE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PALETTE_EDITOR))
#define LIGMA_IS_PALETTE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PALETTE_EDITOR))
#define LIGMA_PALETTE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PALETTE_EDITOR, LigmaPaletteEditorClass))


typedef struct _LigmaPaletteEditorClass LigmaPaletteEditorClass;

struct _LigmaPaletteEditor
{
  LigmaDataEditor    parent_instance;

  GtkWidget        *view;

  GtkWidget        *index_label;
  GtkWidget        *color_name;
  GtkAdjustment    *columns_adj;

  GtkWidget        *color_dialog;

  LigmaPaletteEntry *color;

  gdouble           zoom_factor;  /* range from 0.1 to 4.0 */
  gint              col_width;
  gint              last_width;
  gint              columns;
};

struct _LigmaPaletteEditorClass
{
  LigmaDataEditorClass  parent_class;
};


GType       ligma_palette_editor_get_type   (void) G_GNUC_CONST;

GtkWidget * ligma_palette_editor_new        (LigmaContext        *context,
                                            LigmaMenuFactory    *menu_factory);

void        ligma_palette_editor_edit_color (LigmaPaletteEditor  *editor);
void        ligma_palette_editor_pick_color (LigmaPaletteEditor  *editor,
                                            const LigmaRGB      *color,
                                            LigmaColorPickState  pick_state);
void        ligma_palette_editor_zoom       (LigmaPaletteEditor  *editor,
                                            LigmaZoomType        zoom_type);

gint        ligma_palette_editor_get_index  (LigmaPaletteEditor *editor,
                                            const LigmaRGB     *search);
gboolean    ligma_palette_editor_set_index  (LigmaPaletteEditor *editor,
                                            gint               index,
                                            LigmaRGB           *color);

gint        ligma_palette_editor_max_index  (LigmaPaletteEditor *editor);


#endif /* __LIGMA_PALETTE_EDITOR_H__ */
