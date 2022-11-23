/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapatternfactoryview.c
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontext.h"
#include "core/ligmadatafactory.h"
#include "core/ligmaviewable.h"

#include "ligmaeditor.h"
#include "ligmamenufactory.h"
#include "ligmapatternfactoryview.h"
#include "ligmaviewrenderer.h"


G_DEFINE_TYPE (LigmaPatternFactoryView, ligma_pattern_factory_view,
               LIGMA_TYPE_DATA_FACTORY_VIEW)


static void
ligma_pattern_factory_view_class_init (LigmaPatternFactoryViewClass *klass)
{
}

static void
ligma_pattern_factory_view_init (LigmaPatternFactoryView *view)
{
}

GtkWidget *
ligma_pattern_factory_view_new (LigmaViewType      view_type,
                               LigmaDataFactory  *factory,
                               LigmaContext      *context,
                               gint              view_size,
                               gint              view_border_width,
                               LigmaMenuFactory  *menu_factory)
{
  LigmaPatternFactoryView *factory_view;
  LigmaContainerEditor    *editor;

  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  factory_view = g_object_new (LIGMA_TYPE_PATTERN_FACTORY_VIEW,
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

  editor = LIGMA_CONTAINER_EDITOR (factory_view);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor->view),
                                 "patterns", "patterns-open-as-image",
                                 NULL);

  gtk_widget_hide (ligma_data_factory_view_get_edit_button (LIGMA_DATA_FACTORY_VIEW (factory_view)));

  return GTK_WIDGET (factory_view);
}
