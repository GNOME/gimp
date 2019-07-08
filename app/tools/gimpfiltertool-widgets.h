/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfiltertool-widgets.h
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

#ifndef __GIMP_FILTER_TOOL_WIDGETS_H__
#define __GIMP_FILTER_TOOL_WIDGETS_H__


GimpToolWidget * gimp_filter_tool_create_widget (GimpFilterTool     *filter_tool,
                                                 GimpControllerType  controller_type,
                                                 const gchar        *status_title,
                                                 GCallback           callback,
                                                 gpointer            callback_data,
                                                 GCallback          *set_func,
                                                 gpointer           *set_func_data);

void             gimp_filter_tool_reset_widget  (GimpFilterTool     *filter_tool,
                                                 GimpToolWidget     *widget);


#endif /* __GIMP_FILTER_TOOL_WIDGETS_H__ */
