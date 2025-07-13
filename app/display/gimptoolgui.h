/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolgui.h
 * Copyright (C) 2013  Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "core/gimpobject.h"


#define GIMP_TYPE_TOOL_GUI            (gimp_tool_gui_get_type ())
#define GIMP_TOOL_GUI(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_GUI, GimpToolGui))
#define GIMP_TOOL_GUI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_GUI, GimpToolGuiClass))
#define GIMP_IS_TOOL_GUI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_GUI))
#define GIMP_IS_TOOL_GUI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_GUI))
#define GIMP_TOOL_GUI_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_GUI, GimpToolGuiClass))


typedef struct _GimpToolGuiClass GimpToolGuiClass;

struct _GimpToolGui
{
  GimpObject  parent_instance;
};

struct _GimpToolGuiClass
{
  GimpObjectClass  parent_instance;

  void (* response) (GimpToolGui *gui,
                     gint         response_id);

};


GType         gimp_tool_gui_get_type               (void) G_GNUC_CONST;

GimpToolGui * gimp_tool_gui_new                    (GimpToolInfo     *tool_info,
                                                    const gchar      *title,
                                                    const gchar      *description,
                                                    const gchar      *icon_name,
                                                    const gchar      *help_id,
                                                    GdkMonitor       *monitor,
                                                    gboolean          overlay,
                                                    ...) G_GNUC_NULL_TERMINATED;

void          gimp_tool_gui_set_title              (GimpToolGui      *gui,
                                                    const gchar      *title);
void          gimp_tool_gui_set_description        (GimpToolGui      *gui,
                                                    const gchar      *description);
void          gimp_tool_gui_set_icon_name          (GimpToolGui      *gui,
                                                    const gchar      *icon_name);
void          gimp_tool_gui_set_help_id            (GimpToolGui      *gui,
                                                    const gchar      *help_id);

void          gimp_tool_gui_set_shell              (GimpToolGui      *gui,
                                                    GimpDisplayShell *shell);
void          gimp_tool_gui_set_viewables          (GimpToolGui      *gui,
                                                    GList            *viewables);
void          gimp_tool_gui_set_viewable           (GimpToolGui      *gui,
                                                    GimpViewable     *viewable);

GtkWidget   * gimp_tool_gui_get_dialog             (GimpToolGui      *gui);
GtkWidget   * gimp_tool_gui_get_vbox               (GimpToolGui      *gui);

gboolean      gimp_tool_gui_get_visible            (GimpToolGui      *gui);
void          gimp_tool_gui_show                   (GimpToolGui      *gui);
void          gimp_tool_gui_hide                   (GimpToolGui      *gui);

void          gimp_tool_gui_set_overlay            (GimpToolGui      *gui,
                                                    GdkMonitor       *monitor,
                                                    gboolean          overlay);
gboolean      gimp_tool_gui_get_overlay            (GimpToolGui      *gui);

void          gimp_tool_gui_set_auto_overlay       (GimpToolGui      *gui,
                                                    gboolean          auto_overlay);
gboolean      gimp_tool_gui_get_auto_overlay       (GimpToolGui      *gui);

void          gimp_tool_gui_set_focus_on_map       (GimpToolGui      *gui,
                                                    gboolean          focus_on_map);
gboolean      gimp_tool_gui_get_focus_on_map       (GimpToolGui      *gui);


void          gimp_tool_gui_add_buttons_valist     (GimpToolGui      *gui,
                                                    va_list           args);
void          gimp_tool_gui_add_button             (GimpToolGui      *gui,
                                                    const gchar      *button_text,
                                                    gint              response_id);
void          gimp_tool_gui_set_default_response   (GimpToolGui      *gui,
                                                    gint              response_id);
void          gimp_tool_gui_set_response_sensitive (GimpToolGui      *gui,
                                                    gint              response_id,
                                                    gboolean          sensitive);
void    gimp_tool_gui_set_alternative_button_order (GimpToolGui      *gui,
                                                    ...);
