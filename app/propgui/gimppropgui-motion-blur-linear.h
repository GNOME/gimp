/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-motion-blur-linear.h
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

#ifndef __GIMP_PROP_GUI_MOTION_BLUR_LINEAR_H__
#define __GIMP_PROP_GUI_MOTION_BLUR_LINEAR_H__


GtkWidget *
_gimp_prop_gui_new_motion_blur_linear (GObject                  *config,
                                       GParamSpec              **param_specs,
                                       guint                     n_param_specs,
                                       GeglRectangle            *area,
                                       GimpContext              *context,
                                       GimpCreatePickerFunc      create_picker_func,
                                       GimpCreateControllerFunc  create_controller_func,
                                       gpointer                  creator);


#endif /* __GIMP_PROP_GUI_MOTION_BLUR_LINEAR_H__ */
