/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef  __GIMP_IMAGE_MAP_TOOL_H__
#define  __GIMP_IMAGE_MAP_TOOL_H__


#include "gimpcolortool.h"


#define GIMP_TYPE_IMAGE_MAP_TOOL            (gimp_image_map_tool_get_type ())
#define GIMP_IMAGE_MAP_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_MAP_TOOL, GimpImageMapTool))
#define GIMP_IMAGE_MAP_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_MAP_TOOL, GimpImageMapToolClass))
#define GIMP_IS_IMAGE_MAP_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_MAP_TOOL))
#define GIMP_IS_IMAGE_MAP_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_MAP_TOOL))
#define GIMP_IMAGE_MAP_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_MAP_TOOL, GimpImageMapToolClass))

#define GIMP_IMAGE_MAP_TOOL_GET_OPTIONS(t)  (GIMP_IMAGE_MAP_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpImageMapToolClass GimpImageMapToolClass;

struct _GimpImageMapTool
{
  GimpColorTool  parent_instance;

  GimpDrawable  *drawable;
  GimpImageMap  *image_map;

  /* dialog */
  GtkWidget     *shell;
  GtkWidget     *main_vbox;
  GtkWidget     *load_button;
  GtkWidget     *save_button;

  /* settings file dialog */
  GtkWidget     *settings_dialog;
};

struct _GimpImageMapToolClass
{
  GimpColorToolClass  parent_class;

  const gchar        *shell_desc;
  const gchar        *settings_name;
  const gchar        *load_button_tip;
  const gchar        *load_dialog_title;
  const gchar        *save_button_tip;
  const gchar        *save_dialog_title;

  /* virtual functions */
  void     (* map)           (GimpImageMapTool  *image_map_tool);
  void     (* dialog)        (GimpImageMapTool  *image_map_tool);
  void     (* reset)         (GimpImageMapTool  *image_map_tool);

  gboolean (* settings_load) (GimpImageMapTool  *image_map_tool,
                              gpointer           file,
                              GError           **error);
  gboolean (* settings_save) (GimpImageMapTool  *image_map_tool,
                              gpointer           file);
};


GType   gimp_image_map_tool_get_type (void) G_GNUC_CONST;

void    gimp_image_map_tool_preview  (GimpImageMapTool *image_map_tool);


#endif  /*  __GIMP_IMAGE_MAP_TOOL_H__  */
