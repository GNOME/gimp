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

#pragma once

/*  this check is needed for the extern declaration below to be correct  */
#ifndef __GIMP_ACTION_GROUP_H__
#error "widgets/gimpactiongroup.h must be included prior to dialogs-actions.h"
#endif

extern const GimpStringActionEntry dialogs_dockable_actions[];
extern gint                        n_dialogs_dockable_actions;


void       dialogs_actions_setup          (GimpActionGroup *group);
void       dialogs_actions_update         (GimpActionGroup *group,
                                           gpointer         data);

gboolean   dialogs_actions_toolbox_exists (Gimp            *gimp);
