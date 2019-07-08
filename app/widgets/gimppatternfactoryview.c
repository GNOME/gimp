/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppatternfactoryview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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
#include "gimppatternfactoryview.h"
#include "gimpviewrenderer.h"


G_DEFINE_TYPE (GimpPatternFactoryView, gimp_pattern_factory_view,
               GIMP_TYPE_DATA_FACTORY_VIEW)


static void
gimp_pattern_factory_view_class_init (GimpPatternFactoryViewClass *klass)
{
}

static void
gimp_pattern_factory_view_init (GimpPatternFactoryView *view)
{
}

GtkWidget *
gimp_pattern_factory_view_new (GimpViewType      view_type,
                               GimpDataFactory  *factory,
                               GimpContext      *context,
                               gint              view_size,
                               gint              view_border_width,
                               GimpMenuFactory  *menu_factory)
{
  GimpPatternFactoryView *factory_view;
  GimpContainerEditor    *editor;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  factory_view = g_object_new (GIMP_TYPE_PATTERN_FACTORY_VIEW,
                               "view-type",         view_type,
                               "data-factory",      factory,
                               "context",           context,
                               "view-size",         view_size,
                               "view-border-width", view_border_width,
                               "menu-factory",      menu_factory,
                               "menu-identifier",   "<Patterns>",
                               "ui-path",           "/patterns-popup",
                               "action-group",      "patterns",
                               NULL);

  editor = GIMP_CONTAINER_EDITOR (factory_view);

  gimp_editor_add_action_button (GIMP_EDITOR (editor->view),
                                 "patterns", "patterns-open-as-image",
                                 NULL);

  gtk_widget_hide (gimp_data_factory_view_get_edit_button (GIMP_DATA_FACTORY_VIEW (factory_view)));

  return GTK_WIDGET (factory_view);
}
