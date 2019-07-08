/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolpresetfactoryview.c
 * Copyright (C) 2010 Alexia Death
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpviewable.h"

#include "gimpeditor.h"
#include "gimpmenufactory.h"
#include "gimptoolpresetfactoryview.h"
#include "gimpviewrenderer.h"


G_DEFINE_TYPE (GimpToolPresetFactoryView, gimp_tool_preset_factory_view,
               GIMP_TYPE_DATA_FACTORY_VIEW)


static void
gimp_tool_preset_factory_view_class_init (GimpToolPresetFactoryViewClass *klass)
{
}

static void
gimp_tool_preset_factory_view_init (GimpToolPresetFactoryView *view)
{
}

GtkWidget *
gimp_tool_preset_factory_view_new (GimpViewType      view_type,
                                   GimpDataFactory  *factory,
                                   GimpContext      *context,
                                   gint              view_size,
                                   gint              view_border_width,
                                   GimpMenuFactory  *menu_factory)
{
  GimpToolPresetFactoryView *factory_view;
  GimpEditor                *editor;
  GtkWidget                 *button;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  factory_view = g_object_new (GIMP_TYPE_TOOL_PRESET_FACTORY_VIEW,
                               "view-type",         view_type,
                               "data-factory",      factory,
                               "context",           context,
                               "view-size",         view_size,
                               "view-border-width", view_border_width,
                               "menu-factory",      menu_factory,
                               "menu-identifier",   "<ToolPresets>",
                               "ui-path",           "/tool-presets-popup",
                               "action-group",      "tool-presets",
                               NULL);

  gtk_widget_hide (gimp_data_factory_view_get_duplicate_button (GIMP_DATA_FACTORY_VIEW (factory_view)));

  editor = GIMP_EDITOR (GIMP_CONTAINER_EDITOR (factory_view)->view);

  button = gimp_editor_add_action_button (editor, "tool-presets",
                                          "tool-presets-save", NULL);
  gtk_box_reorder_child (gimp_editor_get_button_box (editor),
                         button, 2);

  button = gimp_editor_add_action_button (editor, "tool-presets",
                                          "tool-presets-restore", NULL);
  gtk_box_reorder_child (gimp_editor_get_button_box (editor),
                         button, 3);

  return GTK_WIDGET (factory_view);
}
