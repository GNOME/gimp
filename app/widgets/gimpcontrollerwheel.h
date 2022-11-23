/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacontrollerwheel.h
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

#ifndef __LIGMA_CONTROLLER_WHEEL_H__
#define __LIGMA_CONTROLLER_WHEEL_H__


#define LIGMA_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libligmawidgets/ligmacontroller.h"


#define LIGMA_TYPE_CONTROLLER_WHEEL            (ligma_controller_wheel_get_type ())
#define LIGMA_CONTROLLER_WHEEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTROLLER_WHEEL, LigmaControllerWheel))
#define LIGMA_CONTROLLER_WHEEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTROLLER_WHEEL, LigmaControllerWheelClass))
#define LIGMA_IS_CONTROLLER_WHEEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTROLLER_WHEEL))
#define LIGMA_IS_CONTROLLER_WHEEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTROLLER_WHEEL))
#define LIGMA_CONTROLLER_WHEEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTROLLER_WHEEL, LigmaControllerWheelClass))


typedef struct _LigmaControllerWheelClass LigmaControllerWheelClass;

struct _LigmaControllerWheel
{
  LigmaController parent_instance;
};

struct _LigmaControllerWheelClass
{
  LigmaControllerClass parent_class;
};


GType      ligma_controller_wheel_get_type (void) G_GNUC_CONST;

gboolean   ligma_controller_wheel_scroll   (LigmaControllerWheel  *wheel,
                                           const GdkEventScroll *sevent);


#endif /* __LIGMA_CONTROLLER_WHEEL_H__ */
