/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontrollers.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "core/gimplist.h"


#define GIMP_TYPE_CONTROLLER_MANAGER (gimp_controller_manager_get_type ())
G_DECLARE_FINAL_TYPE (GimpControllerManager,
                      gimp_controller_manager,
                      GIMP, CONTROLLER_MANAGER,
                      GimpList)


GimpControllerManager * gimp_controller_manager_new     (Gimp                  *gimp);

void             gimp_controller_manager_restore        (GimpControllerManager *self,
                                                         GimpUIManager         *ui_manager);
void             gimp_controller_manager_save           (GimpControllerManager *self);

Gimp           * gimp_controller_manager_get_gimp       (GimpControllerManager *self);
GimpUIManager  * gimp_controller_manager_get_ui_manager (GimpControllerManager *self);
GimpController * gimp_controller_manager_get_wheel      (GimpControllerManager *self);
GimpController * gimp_controller_manager_get_keyboard   (GimpControllerManager *self);
GListModel     * gimp_controller_manager_get_categories (GimpControllerManager *self);
