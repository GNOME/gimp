/* GIMP - The GNU Image Manipulation Program
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimpcontrollermanager.h"
#include "gimpcontrollers.h"


#define GIMP_CONTROLLER_MANAGER_DATA_KEY "gimp-controller-manager"


/*  public functions  */

void
gimp_controllers_init (Gimp *gimp)
{
  GimpControllerManager *self;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp->controller_manager == NULL);

  self = gimp_controller_manager_new (gimp);

  gimp->controller_manager = G_OBJECT (self);
}

void
gimp_controllers_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_CONTROLLER_MANAGER (gimp->controller_manager));

  g_clear_object (&gimp->controller_manager);
}

GimpControllerManager *
gimp_get_controller_manager (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (gimp->controller_manager != NULL, NULL);

  return GIMP_CONTROLLER_MANAGER (gimp->controller_manager);
}
