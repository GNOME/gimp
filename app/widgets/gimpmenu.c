/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenu.c
 * Copyright (C) 2023 Jehan
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

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimpmenu.h"
#include "gimpmenushell.h"
#include "gimpuimanager.h"


/**
 * GimpMenu:
 *
 * Our own menu widget.
 *
 * We cannot use the simpler gtk_menu_new_from_model() because it lacks
 * tooltip support and unfortunately GTK does not plan to implement this:
 * https://gitlab.gnome.org/GNOME/gtk/-/issues/785
 *
 * This is why we need to implement our own GimpMenu subclass. It looks very
 * minimal right now, but the whole point is that it is also a GimpMenuShell
 * (where all the real work, with proxy items, syncing with actions and such,
 * happens).
 */


G_DEFINE_TYPE_WITH_CODE (GimpMenu, gimp_menu, GTK_TYPE_MENU,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_MENU_SHELL, NULL))

#define parent_class gimp_menu_parent_class


/*  private functions  */

static void
gimp_menu_class_init (GimpMenuClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gimp_menu_shell_get_property;
  object_class->set_property = gimp_menu_shell_set_property;

  gimp_menu_shell_install_properties (object_class);
}

static void
gimp_menu_init (GimpMenu *menu)
{
  menu->priv = gimp_menu_get_instance_private (menu);

  gimp_menu_shell_init (GIMP_MENU_SHELL (menu));
}

/* Public functions */

GtkWidget *
gimp_menu_new (GimpUIManager *manager)
{
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);

  return g_object_new (GIMP_TYPE_MENU,
                       "manager", manager,
                       NULL);
}
