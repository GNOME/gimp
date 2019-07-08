/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdynamicsfactoryview.c
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

#include "gimpdynamicsfactoryview.h"
#include "gimpmenufactory.h"
#include "gimpviewrenderer.h"


G_DEFINE_TYPE (GimpDynamicsFactoryView, gimp_dynamics_factory_view,
               GIMP_TYPE_DATA_FACTORY_VIEW)


static void
gimp_dynamics_factory_view_class_init (GimpDynamicsFactoryViewClass *klass)
{
}

static void
gimp_dynamics_factory_view_init (GimpDynamicsFactoryView *view)
{
}

GtkWidget *
gimp_dynamics_factory_view_new (GimpViewType      view_type,
                                GimpDataFactory  *factory,
                                GimpContext      *context,
                                gint              view_size,
                                gint              view_border_width,
                                GimpMenuFactory  *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (GIMP_TYPE_DYNAMICS_FACTORY_VIEW,
                       "view-type",         view_type,
                       "data-factory",      factory,
                       "context",           context,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       "menu-factory",      menu_factory,
                       "menu-identifier",   "<Dynamics>",
                       "ui-path",           "/dynamics-popup",
                       "action-group",      "dynamics",
                       NULL);
}
