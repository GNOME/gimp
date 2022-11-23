/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_FILTER_TOOL_H__
#define __LIGMA_FILTER_TOOL_H__


#include "ligmacolortool.h"


#define LIGMA_TYPE_FILTER_TOOL            (ligma_filter_tool_get_type ())
#define LIGMA_FILTER_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FILTER_TOOL, LigmaFilterTool))
#define LIGMA_FILTER_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FILTER_TOOL, LigmaFilterToolClass))
#define LIGMA_IS_FILTER_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FILTER_TOOL))
#define LIGMA_IS_FILTER_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FILTER_TOOL))
#define LIGMA_FILTER_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FILTER_TOOL, LigmaFilterToolClass))

#define LIGMA_FILTER_TOOL_GET_OPTIONS(t)  (LIGMA_FILTER_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaFilterToolClass LigmaFilterToolClass;

struct _LigmaFilterTool
{
  LigmaColorTool       parent_instance;

  GeglNode           *operation;
  GObject            *config;
  GObject            *default_config;
  LigmaContainer      *settings;

  gchar              *description;

  gboolean            has_settings;

  LigmaDrawableFilter *filter;

  LigmaGuide          *preview_guide;

  gpointer            pick_identifier;
  gboolean            pick_abyss;

  /* dialog */
  gboolean            overlay;
  LigmaToolGui        *gui;
  GtkWidget          *settings_box;
  GtkWidget          *controller_toggle;
  GtkWidget          *operation_settings_box;
  GtkWidget          *clip_combo;
  GtkWidget          *region_combo;
  GtkWidget          *active_picker;

  /* widget */
  LigmaToolWidget     *widget;
  LigmaToolWidget     *grab_widget;
};

struct _LigmaFilterToolClass
{
  LigmaColorToolClass  parent_class;

  /* virtual functions */
  gchar     * (* get_operation)   (LigmaFilterTool    *filter_tool,
                                   gchar            **description);
  void        (* dialog)          (LigmaFilterTool    *filter_tool);
  void        (* reset)           (LigmaFilterTool    *filter_tool);
  void        (* set_config)      (LigmaFilterTool    *filter_tool,
                                   LigmaConfig        *config);
  void        (* config_notify)   (LigmaFilterTool    *filter_tool,
                                   LigmaConfig        *config,
                                   const GParamSpec  *pspec);

  gboolean    (* settings_import) (LigmaFilterTool    *filter_tool,
                                   GInputStream      *input,
                                   GError           **error);
  gboolean    (* settings_export) (LigmaFilterTool    *filter_tool,
                                   GOutputStream     *output,
                                   GError           **error);

  void        (* region_changed)  (LigmaFilterTool    *filter_tool);
  void        (* color_picked)    (LigmaFilterTool    *filter_tool,
                                   gpointer           identifier,
                                   gdouble            x,
                                   gdouble            y,
                                   const Babl        *sample_format,
                                   const LigmaRGB     *color);
};


GType       ligma_filter_tool_get_type              (void) G_GNUC_CONST;

void        ligma_filter_tool_get_operation         (LigmaFilterTool     *filter_tool);

void        ligma_filter_tool_set_config            (LigmaFilterTool     *filter_tool,
                                                    LigmaConfig         *config);

void        ligma_filter_tool_edit_as               (LigmaFilterTool     *filter_tool,
                                                    const gchar        *new_tool_id,
                                                    LigmaConfig         *config);

gboolean    ligma_filter_tool_on_guide              (LigmaFilterTool     *filter_tool,
                                                    const LigmaCoords   *coords,
                                                    LigmaDisplay        *display);

GtkWidget * ligma_filter_tool_dialog_get_vbox       (LigmaFilterTool     *filter_tool);

void        ligma_filter_tool_enable_color_picking  (LigmaFilterTool     *filter_tool,
                                                    gpointer            identifier,
                                                    gboolean            pick_abyss);
void        ligma_filter_tool_disable_color_picking (LigmaFilterTool     *filter_tool);

GtkWidget * ligma_filter_tool_add_color_picker      (LigmaFilterTool     *filter_tool,
                                                    gpointer            identifier,
                                                    const gchar        *icon_name,
                                                    const gchar        *tooltip,
                                                    gboolean            pick_abyss,
                                                    LigmaPickerCallback  callback,
                                                    gpointer            callback_data);
GCallback   ligma_filter_tool_add_controller        (LigmaFilterTool     *filter_tool,
                                                    LigmaControllerType  controller_type,
                                                    const gchar        *status_title,
                                                    GCallback           callback,
                                                    gpointer            callback_data,
                                                    gpointer           *set_func_data);

void        ligma_filter_tool_set_widget            (LigmaFilterTool     *filter_tool,
                                                    LigmaToolWidget     *widget);

gboolean    ligma_filter_tool_get_drawable_area     (LigmaFilterTool     *filter_tool,
                                                    gint               *drawable_offset_x,
                                                    gint               *drawable_offset_y,
                                                    GeglRectangle      *drawable_area);


#endif /* __LIGMA_FILTER_TOOL_H__ */
