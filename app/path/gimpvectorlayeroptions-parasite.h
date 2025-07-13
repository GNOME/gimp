/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   gimpvectorlayeroptions-parasite.h
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

#ifndef __GIMP_VECTOR_LAYER_OPTIONS_PARASITE_H__
#define __GIMP_VECTOR_LAYER_OPTIONS_PARASITE_H__


const gchar            * gimp_vector_layer_options_parasite_name (void) G_GNUC_CONST;
GimpParasite           * gimp_vector_layer_options_to_parasite   (const GimpVectorLayerOptions *text);
GimpVectorLayerOptions * gimp_vector_layer_options_from_parasite (const GimpParasite           *parasite,
                                                                  GError                      **error,
                                                                  Gimp                         *gimp);


#endif /* __GIMP_VECTOR_LAYER_OPTIONS_PARASITE_H__ */
