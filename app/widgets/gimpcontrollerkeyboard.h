/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacontrollerkeyboard.h
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

#ifndef __LIGMA_CONTROLLER_KEYBOARD_H__
#define __LIGMA_CONTROLLER_KEYBOARD_H__


#define LIGMA_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libligmawidgets/ligmacontroller.h"


#define LIGMA_TYPE_CONTROLLER_KEYBOARD            (ligma_controller_keyboard_get_type ())
#define LIGMA_CONTROLLER_KEYBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTROLLER_KEYBOARD, LigmaControllerKeyboard))
#define LIGMA_CONTROLLER_KEYBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTROLLER_KEYBOARD, LigmaControllerKeyboardClass))
#define LIGMA_IS_CONTROLLER_KEYBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTROLLER_KEYBOARD))
#define LIGMA_IS_CONTROLLER_KEYBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTROLLER_KEYBOARD))
#define LIGMA_CONTROLLER_KEYBOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTROLLER_KEYBOARD, LigmaControllerKeyboardClass))


typedef struct _LigmaControllerKeyboardClass LigmaControllerKeyboardClass;

struct _LigmaControllerKeyboard
{
  LigmaController parent_instance;
};

struct _LigmaControllerKeyboardClass
{
  LigmaControllerClass parent_class;
};


GType      ligma_controller_keyboard_get_type  (void) G_GNUC_CONST;

gboolean   ligma_controller_keyboard_key_press (LigmaControllerKeyboard *keyboard,
                                               const GdkEventKey      *kevent);


#endif /* __LIGMA_CONTROLLER_KEYBOARD_H__ */
