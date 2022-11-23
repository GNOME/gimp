/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafiltertool-settings.h
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

#ifndef __LIGMA_FILTER_TOOL_SETTINGS_H__
#define __LIGMA_FILTER_TOOL_SETTINGS_H__


GtkWidget * ligma_filter_tool_get_settings_box     (LigmaFilterTool  *filter_tool);


/*  virtual functions of LigmaSettingsTool, don't call directly  */

gboolean    ligma_filter_tool_real_settings_import (LigmaFilterTool  *filter_tool,
                                                   GInputStream    *input,
                                                   GError         **error);
gboolean    ligma_filter_tool_real_settings_export (LigmaFilterTool  *filter_tool,
                                                   GOutputStream   *output,
                                                   GError         **error);


#endif /* __LIGMA_FILTER_TOOL_SETTINGS_H__ */
