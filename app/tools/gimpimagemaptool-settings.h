/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagemaptool-settings.h
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_IMAGE_MAP_TOOL_SETTINGS_H__
#define __GIMP_IMAGE_MAP_TOOL_SETTINGS_H__


GtkWidget * gimp_image_map_tool_real_get_settings_ui (GimpImageMapTool *im_tool,
                                                      GimpContainer    *settings,
                                                      GFile            *settings_file,
                                                      const gchar      *import_dialog_title,
                                                      const gchar      *export_dialog_title,
                                                      const gchar      *file_dialog_help_id,
                                                      GFile            *default_folder,
                                                      GtkWidget       **settings_box);
gboolean    gimp_image_map_tool_real_settings_import (GimpImageMapTool *im_tool,
                                                      GInputStream     *input,
                                                      GError          **error);
gboolean    gimp_image_map_tool_real_settings_export (GimpImageMapTool *im_tool,
                                                      GOutputStream    *output,
                                                      GError          **error);


#endif /* __GIMP_IMAGE_MAP_TOOL_SETTINGS_H__ */
