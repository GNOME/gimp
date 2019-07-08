/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfontfactoryview.c
 * Copyright (C) 2018 Michael Natterer <mitch@gimp.org>
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

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"

#include "text/gimpfont.h"

#include "gimpfontfactoryview.h"
#include "gimpcontainerview.h"
#include "gimpmenufactory.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"


G_DEFINE_TYPE (GimpFontFactoryView, gimp_font_factory_view,
               GIMP_TYPE_DATA_FACTORY_VIEW)

#define parent_class gimp_font_factory_view_parent_class


static void
gimp_font_factory_view_class_init (GimpFontFactoryViewClass *klass)
{
}

static void
gimp_font_factory_view_init (GimpFontFactoryView *view)
{
}

GtkWidget *
gimp_font_factory_view_new (GimpViewType     view_type,
                            GimpDataFactory *factory,
                            GimpContext     *context,
                            gint             view_size,
                            gint             view_border_width,
                            GimpMenuFactory *menu_factory)
{
  GimpFontFactoryView *factory_view;
  GimpContainerEditor *editor;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  factory_view = g_object_new (GIMP_TYPE_FONT_FACTORY_VIEW,
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

  editor = GIMP_CONTAINER_EDITOR (factory_view);

  gimp_container_editor_bind_to_async_set (editor,
                                           gimp_data_factory_get_async_set (factory),
                                           _("Loading fonts (this may take "
                                             "a while...)"));

  return GTK_WIDGET (factory_view);
}
