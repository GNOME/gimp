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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
  GimpColorTool       parent_instance;

  GeglNode           *operation;
  GimpDrawableFilter *existing_filter;
  GObject            *config;
  GObject            *default_config;
  GimpContainer      *settings;

  gchar              *description;

  gboolean            has_settings;

  GimpDrawableFilter *filter;

  GimpGuide          *preview_guide;

  gpointer            pick_identifier;
  gboolean            pick_abyss;

  /* dialog */
  gboolean            overlay;
  GimpToolGui        *gui;
  GtkWidget          *settings_box;
  GtkWidget          *controller_toggle;
  GtkWidget          *operation_settings_box;
  GtkWidget          *clip_combo;
  GtkWidget          *region_combo;
  GtkWidget          *active_picker;

  /* widget */
  GimpToolWidget     *widget;
  GimpToolWidget     *grab_widget;
};

struct _GimpFilterToolClass
{
  GimpColorToolClass  parent_class;

  /* virtual functions */
  gchar     * (* get_operation)   (GimpFilterTool    *filter_tool,
                                   gchar            **description);
  void        (* dialog)          (GimpFilterTool    *filter_tool);
  void        (* reset)           (GimpFilterTool    *filter_tool);
  void        (* set_config)      (GimpFilterTool    *filter_tool,
                                   GimpConfig        *config);
  void        (* config_notify)   (GimpFilterTool    *filter_tool,
                                   GimpConfig        *config,
                                   const GParamSpec  *pspec);

  gboolean    (* settings_import) (GimpFilterTool    *filter_tool,
                                   GInputStream      *input,
                                   GError           **error);
  gboolean    (* settings_export) (GimpFilterTool    *filter_tool,
                                   GOutputStream     *output,
                                   GError           **error);

  void        (* region_changed)  (GimpFilterTool    *filter_tool);
  void        (* color_picked)    (GimpFilterTool    *filter_tool,
                                   gpointer           identifier,
                                   gdouble            x,
                                   gdouble            y,
                                   const Babl        *sample_format,
                                   GeglColor         *color);
};


GType       gimp_filter_tool_get_type              (void) G_GNUC_CONST;

void        gimp_filter_tool_get_operation         (GimpFilterTool     *filter_tool,
                                                    GimpDrawableFilter *existing_filter);

void        gimp_filter_tool_set_config            (GimpFilterTool     *filter_tool,
                                                    GimpConfig         *config);

void        gimp_filter_tool_edit_as               (GimpFilterTool     *filter_tool,
                                                    const gchar        *new_tool_id,
                                                    GimpConfig         *config);

gboolean    gimp_filter_tool_on_guide              (GimpFilterTool     *filter_tool,
                                                    const GimpCoords   *coords,
                                                    GimpDisplay        *display);

GtkWidget * gimp_filter_tool_dialog_get_vbox       (GimpFilterTool     *filter_tool);

void        gimp_filter_tool_enable_color_picking  (GimpFilterTool     *filter_tool,
                                                    gpointer            identifier,
                                                    gboolean            pick_abyss);
void        gimp_filter_tool_disable_color_picking (GimpFilterTool     *filter_tool);

GtkWidget * gimp_filter_tool_add_color_picker      (GimpFilterTool     *filter_tool,
                                                    gpointer            identifier,
                                                    const gchar        *icon_name,
                                                    const gchar        *tooltip,
                                                    gboolean            pick_abyss,
                                                    GimpPickerCallback  callback,
                                                    gpointer            callback_data);
GCallback   gimp_filter_tool_add_controller        (GimpFilterTool     *filter_tool,
                                                    GimpControllerType  controller_type,
                                                    const gchar        *status_title,
                                                    GCallback           callback,
                                                    gpointer            callback_data,
                                                    gpointer           *set_func_data);

void        gimp_filter_tool_set_widget            (GimpFilterTool     *filter_tool,
                                                    GimpToolWidget     *widget);

gboolean    gimp_filter_tool_get_drawable_area     (GimpFilterTool     *filter_tool,
                                                    gint               *drawable_offset_x,
                                                    gint               *drawable_offset_y,
                                                    GeglRectangle      *drawable_area);


#endif /* __GIMP_FILTER_TOOL_H__ */
