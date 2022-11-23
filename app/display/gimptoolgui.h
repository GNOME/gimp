/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolgui.h
 * Copyright (C) 2013  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TOOL_GUI_H__
#define __LIGMA_TOOL_GUI_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_TOOL_GUI            (ligma_tool_gui_get_type ())
#define LIGMA_TOOL_GUI(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_GUI, LigmaToolGui))
#define LIGMA_TOOL_GUI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_GUI, LigmaToolGuiClass))
#define LIGMA_IS_TOOL_GUI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_GUI))
#define LIGMA_IS_TOOL_GUI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_GUI))
#define LIGMA_TOOL_GUI_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_GUI, LigmaToolGuiClass))


typedef struct _LigmaToolGuiClass LigmaToolGuiClass;

struct _LigmaToolGui
{
  LigmaObject  parent_instance;
};

struct _LigmaToolGuiClass
{
  LigmaObjectClass  parent_instance;

  void (* response) (LigmaToolGui *gui,
                     gint         response_id);

};


GType         ligma_tool_gui_get_type               (void) G_GNUC_CONST;

LigmaToolGui * ligma_tool_gui_new                    (LigmaToolInfo     *tool_info,
                                                    const gchar      *title,
                                                    const gchar      *description,
                                                    const gchar      *icon_name,
                                                    const gchar      *help_id,
                                                    GdkMonitor       *monitor,
                                                    gboolean          overlay,
                                                    ...) G_GNUC_NULL_TERMINATED;

void          ligma_tool_gui_set_title              (LigmaToolGui      *gui,
                                                    const gchar      *title);
void          ligma_tool_gui_set_description        (LigmaToolGui      *gui,
                                                    const gchar      *description);
void          ligma_tool_gui_set_icon_name          (LigmaToolGui      *gui,
                                                    const gchar      *icon_name);
void          ligma_tool_gui_set_help_id            (LigmaToolGui      *gui,
                                                    const gchar      *help_id);

void          ligma_tool_gui_set_shell              (LigmaToolGui      *gui,
                                                    LigmaDisplayShell *shell);
void          ligma_tool_gui_set_viewables          (LigmaToolGui      *gui,
                                                    GList            *viewables);
void          ligma_tool_gui_set_viewable           (LigmaToolGui      *gui,
                                                    LigmaViewable     *viewable);

GtkWidget   * ligma_tool_gui_get_dialog             (LigmaToolGui      *gui);
GtkWidget   * ligma_tool_gui_get_vbox               (LigmaToolGui      *gui);

gboolean      ligma_tool_gui_get_visible            (LigmaToolGui      *gui);
void          ligma_tool_gui_show                   (LigmaToolGui      *gui);
void          ligma_tool_gui_hide                   (LigmaToolGui      *gui);

void          ligma_tool_gui_set_overlay            (LigmaToolGui      *gui,
                                                    GdkMonitor       *monitor,
                                                    gboolean          overlay);
gboolean      ligma_tool_gui_get_overlay            (LigmaToolGui      *gui);

void          ligma_tool_gui_set_auto_overlay       (LigmaToolGui      *gui,
                                                    gboolean          auto_overlay);
gboolean      ligma_tool_gui_get_auto_overlay       (LigmaToolGui      *gui);

void          ligma_tool_gui_set_focus_on_map       (LigmaToolGui      *gui,
                                                    gboolean          focus_on_map);
gboolean      ligma_tool_gui_get_focus_on_map       (LigmaToolGui      *gui);


void          ligma_tool_gui_add_buttons_valist     (LigmaToolGui      *gui,
                                                    va_list           args);
void          ligma_tool_gui_add_button             (LigmaToolGui      *gui,
                                                    const gchar      *button_text,
                                                    gint              response_id);
void          ligma_tool_gui_set_default_response   (LigmaToolGui      *gui,
                                                    gint              response_id);
void          ligma_tool_gui_set_response_sensitive (LigmaToolGui      *gui,
                                                    gint              response_id,
                                                    gboolean          sensitive);
void    ligma_tool_gui_set_alternative_button_order (LigmaToolGui      *gui,
                                                    ...);


#endif /* __LIGMA_TOOL_GUI_H__ */
