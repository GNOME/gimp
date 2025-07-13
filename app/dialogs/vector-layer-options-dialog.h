/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   vector-layer-options-dialog.h
 *
 *   Copyright 2006 Hendrik Boom
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __VECTOR_LAYER_OPTIONS_DIALOG_H__
#define __VECTOR_LAYER_OPTIONS_DIALOG_H__


GtkWidget * vector_layer_options_dialog_new (GimpVectorLayer *layer,
                                             GimpContext     *context,
                                             const gchar     *title,
                                             const gchar     *icon_name,
                                             const gchar     *help_id,
                                             GtkWidget       *parent);


#endif /* __VECTOR_LAYER_OPTIONS_DIALOG_H__ */
