/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_SELECTION_DATA_H__
#define __GIMP_SELECTION_DATA_H__


/*  uri list  */

void            gimp_selection_data_set_uri_list  (GtkSelectionData *selection,
                                                   GList            *uris);
GList         * gimp_selection_data_get_uri_list  (GtkSelectionData *selection);


/*  color  */

void            gimp_selection_data_set_color     (GtkSelectionData *selection,
                                                   GeglColor        *color);
GeglColor     * gimp_selection_data_get_color     (GtkSelectionData *selection);


/*  image (xcf)  */

void            gimp_selection_data_set_xcf       (GtkSelectionData *selection,
                                                   GimpImage        *image);
GimpImage     * gimp_selection_data_get_xcf       (GtkSelectionData *selection,
                                                   Gimp             *gimp);


/*  stream (svg/png)  */

void            gimp_selection_data_set_stream    (GtkSelectionData *selection,
                                                   const guchar     *stream,
                                                   gsize             stream_length);
const guchar  * gimp_selection_data_get_stream    (GtkSelectionData *selection,
                                                   gsize            *stream_length);


/*  curve  */

void            gimp_selection_data_set_curve     (GtkSelectionData *selection,
                                                   GimpCurve        *curve);
GimpCurve     * gimp_selection_data_get_curve     (GtkSelectionData *selection);


/*  image  */

void            gimp_selection_data_set_image     (GtkSelectionData *selection,
                                                   GimpImage        *image);
GimpImage     * gimp_selection_data_get_image     (GtkSelectionData *selection,
                                                   Gimp             *gimp);


/*  component  */

void            gimp_selection_data_set_component (GtkSelectionData *selection,
                                                   GimpImage        *image,
                                                   GimpChannelType   channel);
GimpImage     * gimp_selection_data_get_component (GtkSelectionData *selection,
                                                   Gimp             *gimp,
                                                   GimpChannelType  *channel);


/*  item  */

void            gimp_selection_data_set_item      (GtkSelectionData *selection,
                                                   GimpItem         *item);
GimpItem      * gimp_selection_data_get_item      (GtkSelectionData *selection,
                                                   Gimp             *gimp);


/*  item list  */

void            gimp_selection_data_set_item_list (GtkSelectionData *selection,
                                                   GList            *items);
GList         * gimp_selection_data_get_item_list (GtkSelectionData *selection,
                                                   Gimp             *gimp);


/*  various data  */

void            gimp_selection_data_set_object    (GtkSelectionData *selection,
                                                   GimpObject       *object);

GimpBrush     * gimp_selection_data_get_brush     (GtkSelectionData *selection,
                                                   Gimp             *gimp);
GimpPattern   * gimp_selection_data_get_pattern   (GtkSelectionData *selection,
                                                   Gimp             *gimp);
GimpGradient  * gimp_selection_data_get_gradient  (GtkSelectionData *selection,
                                                   Gimp             *gimp);
GimpPalette   * gimp_selection_data_get_palette   (GtkSelectionData *selection,
                                                   Gimp             *gimp);
GimpFont      * gimp_selection_data_get_font      (GtkSelectionData *selection,
                                                   Gimp             *gimp);
GimpBuffer    * gimp_selection_data_get_buffer    (GtkSelectionData *selection,
                                                   Gimp             *gimp);
GimpImagefile * gimp_selection_data_get_imagefile (GtkSelectionData *selection,
                                                   Gimp             *gimp);
GimpTemplate  * gimp_selection_data_get_template  (GtkSelectionData *selection,
                                                   Gimp             *gimp);
GimpToolItem  * gimp_selection_data_get_tool_item (GtkSelectionData *selection,
                                                   Gimp             *gimp);


#endif /* __GIMP_SELECTION_DATA_H__ */
