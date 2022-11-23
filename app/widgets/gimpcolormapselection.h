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

#ifndef __LIGMA_COLORMAP_SELECTION_H__
#define __LIGMA_COLORMAP_SELECTION_H__


#define LIGMA_TYPE_COLORMAP_SELECTION            (ligma_colormap_selection_get_type ())
#define LIGMA_COLORMAP_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLORMAP_SELECTION, LigmaColormapSelection))
#define LIGMA_COLORMAP_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLORMAP_SELECTION, LigmaColormapSelectionClass))
#define LIGMA_IS_COLORMAP_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLORMAP_SELECTION))
#define LIGMA_IS_COLORMAP_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLORMAP_SELECTION))
#define LIGMA_COLORMAP_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLORMAP_SELECTION, LigmaColormapSelectionClass))


typedef struct _LigmaColormapSelectionClass LigmaColormapSelectionClass;

struct _LigmaColormapSelection
{
  GtkBox  parent_instance;

  LigmaContext     *context;
  LigmaImage       *active_image;
  LigmaPalette     *active_palette;

  GtkWidget       *view;
  gint             col_index;

  PangoLayout     *layout;

  GtkAdjustment   *index_adjustment;
  GtkWidget       *index_spinbutton;
  GtkWidget       *color_entry;

  GtkWidget       *color_dialog;
  GtkWidget       *right_vbox;
};

struct _LigmaColormapSelectionClass
{
  GtkBoxClass  parent_class;

  void (* color_clicked)   (LigmaColormapSelection *selection,
                            LigmaPaletteEntry      *entry,
                            GdkModifierType        state);
  void (* color_activated) (LigmaColormapSelection *selection,
                            LigmaPaletteEntry      *entry);
};


GType       ligma_colormap_selection_get_type   (void) G_GNUC_CONST;

GtkWidget * ligma_colormap_selection_new        (LigmaContext *context);

gint        ligma_colormap_selection_get_index  (LigmaColormapSelection *selection,
                                                const LigmaRGB         *search);
gboolean    ligma_colormap_selection_set_index  (LigmaColormapSelection *selection,
                                                gint                   index,
                                                LigmaRGB               *color);

gint        ligma_colormap_selection_max_index  (LigmaColormapSelection *selection);

LigmaPaletteEntry * ligma_colormap_selection_get_selected_entry (LigmaColormapSelection *selection);

void               ligma_colormap_selection_get_entry_rect (LigmaColormapSelection *selection,
                                                           LigmaPaletteEntry      *entry,
                                                           GdkRectangle          *rect);

#endif /* __LIGMA_COLORMAP_SELECTION_H__ */

