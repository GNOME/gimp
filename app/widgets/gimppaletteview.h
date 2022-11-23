/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapaletteview.h
 * Copyright (C) 2005 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PALETTE_VIEW_H__
#define __LIGMA_PALETTE_VIEW_H__

#include "ligmaview.h"


#define LIGMA_TYPE_PALETTE_VIEW            (ligma_palette_view_get_type ())
#define LIGMA_PALETTE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PALETTE_VIEW, LigmaPaletteView))
#define LIGMA_PALETTE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PALETTE_VIEW, LigmaPaletteViewClass))
#define LIGMA_IS_PALETTE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_PALETTE_VIEW))
#define LIGMA_IS_PALETTE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PALETTE_VIEW))
#define LIGMA_PALETTE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PALETTE_VIEW, LigmaPaletteViewClass))


typedef struct _LigmaPaletteViewClass  LigmaPaletteViewClass;

struct _LigmaPaletteView
{
  LigmaView          parent_instance;

  LigmaPaletteEntry *selected;
  LigmaPaletteEntry *dnd_entry;
};

struct _LigmaPaletteViewClass
{
  LigmaViewClass  parent_class;

  void (* entry_clicked)   (LigmaPaletteView  *view,
                            LigmaPaletteEntry *entry,
                            GdkModifierType   state);
  void (* entry_selected)  (LigmaPaletteView  *view,
                            LigmaPaletteEntry *entry);
  void (* entry_activated) (LigmaPaletteView  *view,
                            LigmaPaletteEntry *entry);
  void (* entry_context)   (LigmaPaletteView  *view,
                            LigmaPaletteEntry *entry);
  void (* color_dropped)   (LigmaPaletteView  *view,
                            LigmaPaletteEntry *entry,
                            const LigmaRGB    *color);
};


GType   ligma_palette_view_get_type     (void) G_GNUC_CONST;

void    ligma_palette_view_select_entry (LigmaPaletteView  *view,
                                        LigmaPaletteEntry *entry);

LigmaPaletteEntry * ligma_palette_view_get_selected_entry (LigmaPaletteView *view);

void               ligma_palette_view_get_entry_rect     (LigmaPaletteView  *view,
                                                         LigmaPaletteEntry *entry,
                                                         GdkRectangle     *rect);


#endif /* __LIGMA_PALETTE_VIEW_H__ */
