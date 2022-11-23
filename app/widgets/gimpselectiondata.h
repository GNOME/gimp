/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_SELECTION_DATA_H__
#define __LIGMA_SELECTION_DATA_H__


/*  uri list  */

void            ligma_selection_data_set_uri_list  (GtkSelectionData *selection,
                                                   GList            *uris);
GList         * ligma_selection_data_get_uri_list  (GtkSelectionData *selection);


/*  color  */

void            ligma_selection_data_set_color     (GtkSelectionData *selection,
                                                   const LigmaRGB    *color);
gboolean        ligma_selection_data_get_color     (GtkSelectionData *selection,
                                                   LigmaRGB          *color);


/*  image (xcf)  */

void            ligma_selection_data_set_xcf       (GtkSelectionData *selection,
                                                   LigmaImage        *image);
LigmaImage     * ligma_selection_data_get_xcf       (GtkSelectionData *selection,
                                                   Ligma             *ligma);


/*  stream (svg/png)  */

void            ligma_selection_data_set_stream    (GtkSelectionData *selection,
                                                   const guchar     *stream,
                                                   gsize             stream_length);
const guchar  * ligma_selection_data_get_stream    (GtkSelectionData *selection,
                                                   gsize            *stream_length);


/*  curve  */

void            ligma_selection_data_set_curve     (GtkSelectionData *selection,
                                                   LigmaCurve        *curve);
LigmaCurve     * ligma_selection_data_get_curve     (GtkSelectionData *selection);


/*  image  */

void            ligma_selection_data_set_image     (GtkSelectionData *selection,
                                                   LigmaImage        *image);
LigmaImage     * ligma_selection_data_get_image     (GtkSelectionData *selection,
                                                   Ligma             *ligma);


/*  component  */

void            ligma_selection_data_set_component (GtkSelectionData *selection,
                                                   LigmaImage        *image,
                                                   LigmaChannelType   channel);
LigmaImage     * ligma_selection_data_get_component (GtkSelectionData *selection,
                                                   Ligma             *ligma,
                                                   LigmaChannelType  *channel);


/*  item  */

void            ligma_selection_data_set_item      (GtkSelectionData *selection,
                                                   LigmaItem         *item);
LigmaItem      * ligma_selection_data_get_item      (GtkSelectionData *selection,
                                                   Ligma             *ligma);


/*  item list  */

void            ligma_selection_data_set_item_list (GtkSelectionData *selection,
                                                   GList            *items);
GList         * ligma_selection_data_get_item_list (GtkSelectionData *selection,
                                                   Ligma             *ligma);


/*  various data  */

void            ligma_selection_data_set_object    (GtkSelectionData *selection,
                                                   LigmaObject       *object);

LigmaBrush     * ligma_selection_data_get_brush     (GtkSelectionData *selection,
                                                   Ligma             *ligma);
LigmaPattern   * ligma_selection_data_get_pattern   (GtkSelectionData *selection,
                                                   Ligma             *ligma);
LigmaGradient  * ligma_selection_data_get_gradient  (GtkSelectionData *selection,
                                                   Ligma             *ligma);
LigmaPalette   * ligma_selection_data_get_palette   (GtkSelectionData *selection,
                                                   Ligma             *ligma);
LigmaFont      * ligma_selection_data_get_font      (GtkSelectionData *selection,
                                                   Ligma             *ligma);
LigmaBuffer    * ligma_selection_data_get_buffer    (GtkSelectionData *selection,
                                                   Ligma             *ligma);
LigmaImagefile * ligma_selection_data_get_imagefile (GtkSelectionData *selection,
                                                   Ligma             *ligma);
LigmaTemplate  * ligma_selection_data_get_template  (GtkSelectionData *selection,
                                                   Ligma             *ligma);
LigmaToolItem  * ligma_selection_data_get_tool_item (GtkSelectionData *selection,
                                                   Ligma             *ligma);


#endif /* __LIGMA_SELECTION_DATA_H__ */
