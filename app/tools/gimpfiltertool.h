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

#ifndef __GIMP_FILTER_TOOL_H__
#define __GIMP_FILTER_TOOL_H__


#include "gimpcolortool.h"


#define GIMP_TYPE_FILTER_TOOL            (gimp_filter_tool_get_type ())
#define GIMP_FILTER_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILTER_TOOL, GimpFilterTool))
#define GIMP_FILTER_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILTER_TOOL, GimpFilterToolClass))
#define GIMP_IS_FILTER_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FILTER_TOOL))
#define GIMP_IS_FILTER_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILTER_TOOL))
#define GIMP_FILTER_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FILTER_TOOL, GimpFilterToolClass))

#define GIMP_FILTER_TOOL_GET_OPTIONS(t)  (GIMP_FILTER_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpFilterToolClass GimpFilterToolClass;

struct _GimpFilterTool
{
  GimpColorTool          parent_instance;

  GimpDrawable          *drawable;

  GeglNode              *operation;
  GObject               *config;
  GObject               *default_config;

  gchar                 *title;
  gchar                 *description;
  gchar                 *undo_desc;
  gchar                 *icon_name;
  gchar                 *help_id;

  gboolean               has_settings;
  gchar                 *settings_folder;
  gchar                 *import_dialog_title;
  gchar                 *export_dialog_title;

  GimpDrawableFilter    *filter;

  GimpGuide             *percent_guide;

  /* dialog */
  gboolean               overlay;
  GimpToolGui           *gui;
  GtkWidget             *settings_box;
  GtkWidget             *region_combo;
  GtkWidget             *active_picker;
};

struct _GimpFilterToolClass
{
  GimpColorToolClass  parent_class;

  /* virtual functions */
  gchar     * (* get_operation)   (GimpFilterTool    *filter_tool,
                                   gchar            **title,
                                   gchar            **description,
                                   gchar            **undo_desc,
                                   gchar            **icon_name,
                                   gchar            **help_id,
                                   gboolean          *has_settings,
                                   gchar            **settings_folder,
                                   gchar            **import_dialog_title,
                                   gchar            **export_dialog_title);
  void        (* dialog)          (GimpFilterTool    *filter_tool);
  void        (* reset)           (GimpFilterTool    *filter_tool);

  GtkWidget * (* get_settings_ui) (GimpFilterTool    *filter_tool,
                                   GimpContainer     *settings,
                                   const gchar       *import_dialog_title,
                                   const gchar       *export_dialog_title,
                                   const gchar       *help_id,
                                   GFile             *default_folder,
                                   GtkWidget        **settings_box);

  gboolean    (* settings_import) (GimpFilterTool    *filter_tool,
                                   GInputStream      *input,
                                   GError           **error);
  gboolean    (* settings_export) (GimpFilterTool    *filter_tool,
                                   GOutputStream     *output,
                                   GError           **error);

  void        (* color_picked)    (GimpFilterTool    *filter_tool,
                                   gpointer           identifier,
                                   gdouble            x,
                                   gdouble            y,
                                   const Babl        *sample_format,
                                   const GimpRGB     *color);
};


GType       gimp_filter_tool_get_type         (void) G_GNUC_CONST;

void        gimp_filter_tool_get_operation    (GimpFilterTool   *filter_tool);

void        gimp_filter_tool_edit_as          (GimpFilterTool   *filter_tool,
                                               const gchar      *new_tool_id,
                                               GimpConfig       *config);

gboolean    gimp_filter_tool_on_guide         (GimpFilterTool   *filter_tool,
                                               const GimpCoords *coords,
                                               GimpDisplay      *display);

GtkWidget * gimp_filter_tool_dialog_get_vbox  (GimpFilterTool   *filter_tool);

GtkWidget * gimp_filter_tool_add_color_picker (GimpFilterTool   *filter_tool,
                                               gpointer          identifier,
                                               const gchar      *icon_name,
                                               const gchar      *tooltip);


#endif /* __GIMP_FILTER_TOOL_H__ */
