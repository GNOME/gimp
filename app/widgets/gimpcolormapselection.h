/* GIMP - The GNU Image Manipulation Program
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

#pragma once


#define GIMP_TYPE_COLORMAP_SELECTION            (gimp_colormap_selection_get_type ())
#define GIMP_COLORMAP_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLORMAP_SELECTION, GimpColormapSelection))
#define GIMP_COLORMAP_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLORMAP_SELECTION, GimpColormapSelectionClass))
#define GIMP_IS_COLORMAP_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLORMAP_SELECTION))
#define GIMP_IS_COLORMAP_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLORMAP_SELECTION))
#define GIMP_COLORMAP_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLORMAP_SELECTION, GimpColormapSelectionClass))


typedef struct _GimpColormapSelectionClass GimpColormapSelectionClass;

struct _GimpColormapSelection
{
  GtkBox  parent_instance;

  GimpContext     *context;
  GimpImage       *active_image;
  GimpPalette     *active_palette;

  GtkWidget       *view;
  gint             col_index;

  PangoLayout     *layout;

  GtkAdjustment   *index_adjustment;
  GtkWidget       *index_spinbutton;
  GtkWidget       *color_entry;

  GtkWidget       *color_dialog;
  GtkWidget       *right_vbox;
};

struct _GimpColormapSelectionClass
{
  GtkBoxClass  parent_class;

  void (* color_clicked)   (GimpColormapSelection *selection,
                            GimpPaletteEntry      *entry,
                            GdkModifierType        state);
  void (* color_activated) (GimpColormapSelection *selection,
                            GimpPaletteEntry      *entry);
};


GType       gimp_colormap_selection_get_type   (void) G_GNUC_CONST;

GtkWidget * gimp_colormap_selection_new        (GimpContext *context);

gint        gimp_colormap_selection_get_index  (GimpColormapSelection  *selection,
                                                GeglColor              *search);
gboolean    gimp_colormap_selection_set_index  (GimpColormapSelection  *selection,
                                                gint                    index,
                                                GeglColor              *color);

gint        gimp_colormap_selection_max_index  (GimpColormapSelection *selection);

GimpPaletteEntry * gimp_colormap_selection_get_selected_entry (GimpColormapSelection *selection);

void               gimp_colormap_selection_get_entry_rect (GimpColormapSelection *selection,
                                                           GimpPaletteEntry      *entry,
                                                           GdkRectangle          *rect);


