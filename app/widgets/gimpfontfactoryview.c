/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafontfactoryview.c
 * Copyright (C) 2018 Michael Natterer <mitch@ligma.org>
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

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadatafactory.h"

#include "text/ligmafont.h"

#include "ligmafontfactoryview.h"
#include "ligmacontainerview.h"
#include "ligmamenufactory.h"
#include "ligmaviewrenderer.h"

#include "ligma-intl.h"


G_DEFINE_TYPE (LigmaFontFactoryView, ligma_font_factory_view,
               LIGMA_TYPE_DATA_FACTORY_VIEW)

#define parent_class ligma_font_factory_view_parent_class


static void
ligma_font_factory_view_class_init (LigmaFontFactoryViewClass *klass)
{
}

static void
ligma_font_factory_view_init (LigmaFontFactoryView *view)
{
}

GtkWidget *
ligma_font_factory_view_new (LigmaViewType     view_type,
                            LigmaDataFactory *factory,
                            LigmaContext     *context,
                            gint             view_size,
                            gint             view_border_width,
                            LigmaMenuFactory *menu_factory)
{
  LigmaFontFactoryView *factory_view;
  LigmaContainerEditor *editor;

  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  factory_view = g_object_new (LIGMA_TYPE_FONT_FACTORY_VIEW,
                               "view-type",         view_type,
                               "data-factory",      factory,
                               "context",           context,
                               "view-size",         view_size,
                               "view-border-width", view_border_width,
                               "menu-factory",      menu_factory,
                               "menu-identifier",   "<Fonts>",
                               "ui-path",           "/fonts-popup",
                               "action-group",      "fonts",
                               NULL);

  editor = LIGMA_CONTAINER_EDITOR (factory_view);

  ligma_container_editor_bind_to_async_set (editor,
                                           ligma_data_factory_get_async_set (factory),
                                           _("Loading fonts (this may take "
                                             "a while...)"));

  return GTK_WIDGET (factory_view);
}
