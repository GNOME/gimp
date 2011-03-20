/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
  GimpColorTool          parent_instance;

  GimpDrawable          *drawable;

  GeglNode              *operation;
  GObject               *config;
  GObject               *default_config;
  GimpImageMapApplyFunc  apply_func;
  gpointer               apply_data;

  GimpImageMap          *image_map;

  /* dialog */
  gboolean               overlay;
  GtkWidget             *dialog;
  GtkWidget             *main_vbox;
  GtkWidget             *settings_box;
  GtkSizeGroup          *label_group;
};

struct _GimpImageMapToolClass
{
  GimpColorToolClass  parent_class;

  const gchar        *dialog_desc;
  const gchar        *settings_name;
  const gchar        *import_dialog_title;
  const gchar        *export_dialog_title;

  GimpContainer      *recent_settings;

  /* virtual functions */
  GeglNode * (* get_operation)   (GimpImageMapTool  *image_map_tool,
                                  GObject          **config);
  void       (* map)             (GimpImageMapTool  *image_map_tool);
  void       (* dialog)          (GimpImageMapTool  *image_map_tool);
  void       (* reset)           (GimpImageMapTool  *image_map_tool);

  gboolean   (* settings_import) (GimpImageMapTool  *image_map_tool,
                                  const gchar       *filename,
                                  GError           **error);
  gboolean   (* settings_export) (GimpImageMapTool  *image_map_tool,
                                  const gchar       *filename,
                                  GError           **error);
};


GType   gimp_image_map_tool_get_type   (void) G_GNUC_CONST;

void    gimp_image_map_tool_preview    (GimpImageMapTool *image_map_tool);

/* temp hack for the gegl tool */
void    gimp_image_map_tool_create_map (GimpImageMapTool *image_map_tool);

void    gimp_image_map_tool_edit_as    (GimpImageMapTool *image_map_tool,
                                        const gchar      *new_tool_id,
                                        GimpConfig       *config);

/* accessors for derived classes */
GtkWidget    * gimp_image_map_tool_dialog_get_vbox        (GimpImageMapTool *tool);
GtkSizeGroup * gimp_image_map_tool_dialog_get_label_group (GimpImageMapTool *tool);


#endif  /*  __GIMP_IMAGE_MAP_TOOL_H__  */
