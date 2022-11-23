/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontrollers.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTROLLERS_H__
#define __LIGMA_CONTROLLERS_H__


void             ligma_controllers_init           (Ligma          *ligma);
void             ligma_controllers_exit           (Ligma          *ligma);

void             ligma_controllers_restore        (Ligma          *ligma,
                                                  LigmaUIManager *ui_manager);
void             ligma_controllers_save           (Ligma          *ligma);

LigmaContainer  * ligma_controllers_get_list       (Ligma          *ligma);
LigmaUIManager  * ligma_controllers_get_ui_manager (Ligma          *ligma);
LigmaController * ligma_controllers_get_wheel      (Ligma          *ligma);
LigmaController * ligma_controllers_get_keyboard   (Ligma          *ligma);


#endif /* __LIGMA_CONTROLLERS_H__ */
