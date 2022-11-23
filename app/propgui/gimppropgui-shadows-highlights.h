/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmapropgui-shadows-highlights.h
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

#ifndef __LIGMA_PROP_GUI_SHADOWS_HIGHLIGHTS_H__
#define __LIGMA_PROP_GUI_SHADOWS_HIGHLIGHTS_H__


GtkWidget *
_ligma_prop_gui_new_shadows_highlights (GObject                  *config,
                                       GParamSpec              **param_specs,
                                       guint                     n_param_specs,
                                       GeglRectangle            *area,
                                       LigmaContext              *context,
                                       LigmaCreatePickerFunc      create_picker_func,
                                       LigmaCreateControllerFunc  create_controller_func,
                                       gpointer                  creator);


#endif /* __LIGMA_PROP_GUI_SHADOWS_HIGHLIGHTS_H__ */
