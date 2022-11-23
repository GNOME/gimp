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

#ifndef __LIGMA_PALETTE_H__
#define __LIGMA_PALETTE_H__


#include "ligmadata.h"


#define LIGMA_TYPE_PALETTE            (ligma_palette_get_type ())
#define LIGMA_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PALETTE, LigmaPalette))
#define LIGMA_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PALETTE, LigmaPaletteClass))
#define LIGMA_IS_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PALETTE))
#define LIGMA_IS_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PALETTE))
#define LIGMA_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PALETTE, LigmaPaletteClass))


struct _LigmaPaletteEntry
{
  LigmaRGB  color;
  gchar   *name;
};


typedef struct _LigmaPaletteClass LigmaPaletteClass;

struct _LigmaPalette
{
  LigmaData  parent_instance;

  GList    *colors;
  gint      n_colors;

  gint      n_columns;
};

struct _LigmaPaletteClass
{
  LigmaDataClass  parent_class;
};


GType              ligma_palette_get_type        (void) G_GNUC_CONST;

LigmaData         * ligma_palette_new             (LigmaContext      *context,
                                                 const gchar      *name);
LigmaData         * ligma_palette_get_standard    (LigmaContext      *context);

GList            * ligma_palette_get_colors      (LigmaPalette      *palette);
gint               ligma_palette_get_n_colors    (LigmaPalette      *palette);

void               ligma_palette_move_entry      (LigmaPalette      *palette,
                                                 LigmaPaletteEntry *entry,
                                                 gint              position);

LigmaPaletteEntry * ligma_palette_add_entry       (LigmaPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name,
                                                 const LigmaRGB    *color);
void               ligma_palette_delete_entry    (LigmaPalette      *palette,
                                                 LigmaPaletteEntry *entry);

gboolean           ligma_palette_set_entry       (LigmaPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name,
                                                 const LigmaRGB    *color);
gboolean           ligma_palette_set_entry_color (LigmaPalette      *palette,
                                                 gint              position,
                                                 const LigmaRGB    *color);
gboolean           ligma_palette_set_entry_name  (LigmaPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name);
LigmaPaletteEntry * ligma_palette_get_entry       (LigmaPalette      *palette,
                                                 gint              position);
gint               ligma_palette_get_entry_position (LigmaPalette   *palette,
                                                 LigmaPaletteEntry *entry);

void               ligma_palette_set_columns     (LigmaPalette      *palette,
                                                 gint              columns);
gint               ligma_palette_get_columns     (LigmaPalette      *palette);

LigmaPaletteEntry * ligma_palette_find_entry      (LigmaPalette      *palette,
                                                 const LigmaRGB    *color,
                                                 LigmaPaletteEntry *start_from);


#endif /* __LIGMA_PALETTE_H__ */
